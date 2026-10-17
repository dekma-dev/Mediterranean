#include "game/Simulation.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <utility>

namespace rpg {
namespace {
float Length(Vec2 v) { return std::sqrt(v.x*v.x + v.z*v.z); }
Vec2 Difference(Vec2 a, Vec2 b) { return {a.x-b.x,a.z-b.z}; }
}

Simulation::Simulation() {
  EntityId id=100;
  for (Vec2 home : {Vec2{-4,7},Vec2{4,9},Vec2{0,12}}) {
    Enemy enemy; enemy.position=home; enemy.home=home; enemy.id=id++;
    enemies_.push_back(enemy);
  }
}

bool Simulation::CanStand(Vec2 p, float radius) const {
  return map_.CanStand(p,radius);
}

void Simulation::Move(Vec2& p, Vec2 displacement, float radius) {
  // Small steps prevent sprint/dash tunneling through thin obstacles.
  const int steps = std::max(1,static_cast<int>(std::ceil(Length(displacement)/.15f)));
  displacement.x /= steps; displacement.z /= steps;
  for (int i=0;i<steps;++i) {
    Vec2 next{p.x+displacement.x,p.z}; if (CanStand(next,radius)) p=next;
    next={p.x,p.z+displacement.z}; if (CanStand(next,radius)) p=next;
  }
}

float Simulation::CameraDistance(Vec2 origin,float originY,Vec2 direction,float slope,float desired) const {
  // Conservative ray sampling against expanded building bounds.
  for (float d=.3f;d<desired;d+=.1f) {
    const Vec2 p{origin.x+direction.x*d,origin.z+direction.z*d};
    const float y=originY+slope*d;
    for (const auto& o : map_.Obstacles())
      if (y < o.height+1.f && y > -.2f && std::abs(p.x-o.center.x)<o.halfX+.3f &&
        std::abs(p.z-o.center.z)<o.halfZ+.3f) return std::max(.25f,d-.2f);
  }
  return desired;
}

void Simulation::SelectTarget() {
  const int count=static_cast<int>(enemies_.size());
  for (int step=1;step<=count;++step) {
    const int index=(target_+step)%count;
    if (enemies_[index].health>0 && Length(Difference(enemies_[index].position,player_))<=22) {
      target_=index; message_="Target selected. 1: Strike | 2: Heavy strike | 3: Dash | 4: Mend"; return;
    }
  }
  target_=-1; message_="No living target nearby.";
}

bool Simulation::HasLineOfSight(Vec2 from,Vec2 to) const {
  return map_.CanTravel(from,to,.05f);
}

void Simulation::UseAbility(int index) {
  if (index<0 || index>=4 || health_<=0) return;
  if (cooldowns_[index]>0) { message_="That ability is cooling down."; return; }
  if (index==3) {
    if (health_>=100) { message_="Health is already full."; return; }
    health_=std::min(100.f,health_+35); message_="Mend restored health.";
  } else if (index==2) {
    Move(player_,{std::sin(facing_)*3.5f,std::cos(facing_)*3.5f},.4f);
    message_="Dash.";
  } else {
    if (target_<0 || enemies_[target_].health<=0) { message_="Press Tab to select an enemy."; return; }
    auto& enemy=enemies_[target_];
    if (Length(Difference(enemy.position,player_))>2.5f || height_>1.5f) {
      message_="Target is out of melee range."; return;
    }
    if (!HasLineOfSight(player_,enemy.position)) { message_="The target is behind an obstacle."; return; }
    const auto direction=Difference(enemy.position,player_);
    facing_=std::atan2(direction.x,direction.z);
    enemy.health=std::max(0.f,enemy.health-(index==0?StrikeDamage():HeavyDamage()));
    enemy.hitFlash=.2f; attackFlash_=.2f;
    message_="Hit.";
    if (enemy.health<=0) {
      target_=-1; enemy.state=EnemyState::Dead; enemy.path.clear();
      message_="Raider defeated. Press E nearby to loot.";
      if (quest_==Quest::Active) { quest_=Quest::Return; message_="Patrol complete. Return to the gold beacon and press E."; }
    }
  }
  cooldowns_[index]=abilityCooldowns[index];
}

const char* Simulation::AbilityStatus(int index) const {
  if (index<0 || index>=4 || health_<=0) return "DISABLED";
  if (cooldowns_[index]>0) return "COOLDOWN";
  if (index==3) return health_>=100?"FULL HEALTH":"READY";
  if (index==2) return "READY";
  if (target_<0 || enemies_[target_].health<=0) return "NO TARGET";
  if (Length(Difference(enemies_[target_].position,player_))>2.5f || height_>1.5f) return "OUT OF RANGE";
  return HasLineOfSight(player_,enemies_[target_].position)?"READY":"BLOCKED";
}

void Simulation::Interact() {
  if (Length(Difference(player_,questGiver))>3) {
    Enemy* nearest=nullptr; float best=2.5f;
    for (auto& enemy : enemies_) {
      const float distance=Length(Difference(player_,enemy.position));
      if (enemy.health<=0 && !enemy.looted && distance<=best && HasLineOfSight(player_,enemy.position)) {
        nearest=&enemy; best=distance;
      }
    }
    if (nearest) {
      nearest->looted=true; gold_+=5; ++potions_;
      message_="Looted 5 gold and one healing potion. Q uses a potion.";
    } else message_="Move near the gold beacon or an unclaimed loot bag and press E.";
    return;
  }
  if (quest_==Quest::Available) {
    const bool alreadyDefeated=std::any_of(enemies_.begin(),enemies_.end(),[](const Enemy& enemy) { return enemy.health<=0; });
    quest_=alreadyDefeated?Quest::Return:Quest::Active;
    message_=alreadyDefeated?"Patrol threat already defeated. Press E to claim the reward.":
      "Patrol accepted: defeat one raider beyond the outpost.";
  }
  else if (quest_==Quest::Return) {
    quest_=Quest::Complete; gold_+=25; health_=100; rewardWeapon_=true;
    message_="Reward: 25 gold and a watchguard blade. Press G to equip.";
  }
  else if (quest_==Quest::Active) message_="Defeat one raider, then return here.";
  else message_="The patrol is complete. Explore the map or press R for a new patrol.";
}

void Simulation::Update(float seconds,const Command& command) {
  if (!std::isfinite(seconds) || seconds<=0) return;
  const float dt=std::min(seconds,.05f);
  if (command.restart) { *this=Simulation{}; return; }
  if (health_<=0) { message_="You have fallen. Press R to restart."; return; }
  for (auto& cooldown : cooldowns_) cooldown=std::max(0.f,cooldown-dt);
  potionCooldown_=std::max(0.f,potionCooldown_-dt);
  attackFlash_=std::max(0.f,attackFlash_-dt);
  const float length=Length(command.movement);
  if (std::isfinite(command.movement.x) && std::isfinite(command.movement.z) && std::isfinite(length) && length>.001f) {
    const float speed=command.sprint?7.f:4.f;
    Vec2 direction{command.movement.x/std::max(1.f,length),command.movement.z/std::max(1.f,length)};
    facing_=std::atan2(direction.x,direction.z);
    Move(player_,{direction.x*speed*dt,direction.z*speed*dt},.4f);
  }
  if (command.jump && height_==0) velocityY_=6;
  velocityY_-=18*dt; height_+=velocityY_*dt;
  if (height_<=0) { height_=0; velocityY_=0; }
  if (command.nextTarget) SelectTarget();
  if (command.interact) Interact();
  if (command.equipWeapon) {
    if (!rewardWeapon_) message_="Complete the patrol to earn the watchguard blade.";
    else {
      equipped_=!equipped_;
      message_=equipped_?"Watchguard blade equipped. Melee damage increased.":"Starter sword equipped.";
    }
  }
  if (command.usePotion) {
    if (potions_==0) message_="No healing potions. Loot defeated raiders with E.";
    else if (health_>=100) message_="Health is already full. Potion retained.";
    else if (potionCooldown_>0) message_="Potion is cooling down.";
    else { --potions_; health_=std::min(100.f,health_+40); potionCooldown_=3; message_="Healing potion restored health."; }
  }
  UseAbility(command.ability);
  for (auto& enemy : enemies_) UpdateEnemy(enemy,dt);
  SeparateEnemies();
}

void Simulation::SeparateEnemies() {
  for (std::size_t i=0;i<enemies_.size();++i) for (std::size_t j=i+1;j<enemies_.size();++j) {
    auto& a=enemies_[i]; auto& b=enemies_[j];
    if (a.health<=0 || b.health<=0) continue;
    auto delta=Difference(a.position,b.position);
    const float distance=Length(delta);
    if (distance>=.9f) continue;
    if (distance<.0001f) delta={1,0};
    else { delta.x/=distance; delta.z/=distance; }
    const float correction=(.9f-distance)*.5f;
    Move(a.position,{delta.x*correction,delta.z*correction},.45f);
    Move(b.position,{-delta.x*correction,-delta.z*correction},.45f);
  }
}

void Simulation::UpdateEnemy(Enemy& enemy,float dt) {
  enemy.hitFlash=std::max(0.f,enemy.hitFlash-dt);
  if (enemy.health<=0) { enemy.state=EnemyState::Dead; return; }
  enemy.attackCooldown=std::max(0.f,enemy.attackCooldown-dt);
  const float distance=Length(Difference(player_,enemy.position));
  const EnemyState previous=enemy.state;
  if (enemy.state==EnemyState::Idle && distance<9 && HasLineOfSight(enemy.position,player_))
    enemy.state=EnemyState::Pursuing;
  if (enemy.state==EnemyState::Pursuing && (Length(Difference(player_,enemy.home))>13 || health_<=0))
    enemy.state=EnemyState::Returning;
  if (enemy.state==EnemyState::Returning && Length(Difference(enemy.position,enemy.home))<.15f) {
    enemy.position=enemy.home; enemy.health=80; enemy.state=EnemyState::Idle; enemy.path.clear();
  }
  if (enemy.state==EnemyState::Idle) return;
  Vec2 goal=enemy.state==EnemyState::Pursuing?player_:enemy.home;
  if (enemy.state==EnemyState::Pursuing && !map_.CanStand(goal,.45f)) {
    // The player has a slightly smaller collision radius. Find a reachable approach
    // position when the player stands too close to a wall for an enemy center.
    float nearest=1000;
    for (int i=0;i<16;++i) {
      const float angle=i*6.2831853f/16;
      const Vec2 approach{player_.x+std::cos(angle)*1.3f,player_.z+std::sin(angle)*1.3f};
      const float distanceToEnemy=Length(Difference(approach,enemy.position));
      if (distanceToEnemy<nearest && map_.CanStand(approach,.45f) && HasLineOfSight(approach,player_)) {
        nearest=distanceToEnemy; goal=approach;
      }
    }
  }
  const float remaining=Length(Difference(goal,enemy.position));
  enemy.repathTimer-=dt;
  if (previous!=enemy.state || enemy.repathTimer<=0) {
    // Stop just short of the player's center; navigation uses static world clearance.
    enemy.path=map_.FindPath(enemy.position,goal,.45f);
    enemy.waypoint=0; enemy.repathTimer=.65f;
  }
  const bool pursuing=enemy.state==EnemyState::Pursuing;
  if ((pursuing?distance>1.6f:remaining>.1f) && enemy.waypoint<enemy.path.size()) {
    const auto waypoint=enemy.path[enemy.waypoint];
    const auto direction=Difference(waypoint,enemy.position);
    const float length=Length(direction);
    if (length<.15f) ++enemy.waypoint;
    else {
      const float step=std::min(length,2.2f*dt);
      Move(enemy.position,{direction.x/length*step,direction.z/length*step},.45f);
    }
  }
  if (pursuing && distance<=2 && height_<1.5f && health_>0 && enemy.attackCooldown<=0 &&
    HasLineOfSight(enemy.position,player_)) {
    health_=std::max(0.f,health_-8); enemy.attackCooldown=1.4f;
  }
}

std::string Simulation::Save() const {
  std::ostringstream out;
  out << std::setprecision(9) << "MEDITERRANEAN 2\n" << player_.x << ' ' << player_.z << ' '
    << health_ << ' ' << facing_ << ' ' << static_cast<int>(quest_) << ' ' << gold_ << '\n';
  out << potions_ << ' ' << rewardWeapon_ << ' ' << equipped_ << ' ' << potionCooldown_ << '\n';
  for (float cooldown : cooldowns_) out << cooldown << ' ';
  out << '\n';
  for (const auto& enemy : enemies_)
    out << enemy.id << ' ' << enemy.position.x << ' ' << enemy.position.z << ' ' << enemy.health << ' '
      << enemy.attackCooldown << ' ' << enemy.looted << ' ' << static_cast<int>(enemy.state) << '\n';
  return out.str();
}

bool Simulation::Load(const std::string& data) {
  if (data.size()>4096) return false;
  std::istringstream input(data);
  std::string magic; int version=0, quest=0;
  Simulation candidate;
  if (!(input >> magic >> version) || magic!="MEDITERRANEAN" || (version!=1 && version!=2)) return false;
  if (!(input >> candidate.player_.x >> candidate.player_.z >> candidate.health_ >> candidate.facing_ >> quest >> candidate.gold_)) return false;
  auto valid=[](float value,float min,float max) { return std::isfinite(value) && value>=min && value<=max; };
  if (!valid(candidate.player_.x,-17,17) || !valid(candidate.player_.z,-19,19) ||
    !candidate.CanStand(candidate.player_) || !valid(candidate.health_,0,100) ||
    !valid(candidate.facing_,-3.142f,3.142f) || quest<0 || quest>3 || candidate.gold_<0 || candidate.gold_>40) return false;
  candidate.quest_=static_cast<Quest>(quest);
  if (version==2) {
    int owned=0,equipped=0;
    if (!(input >> candidate.potions_ >> owned >> equipped >> candidate.potionCooldown_) ||
      candidate.potions_<0 || candidate.potions_>5 || owned<0 || owned>1 || equipped<0 || equipped>1 ||
      (equipped && !owned) || bool(owned)!=(quest==3) || !valid(candidate.potionCooldown_,0,3)) return false;
    candidate.rewardWeapon_=owned!=0; candidate.equipped_=equipped!=0;
  } else candidate.rewardWeapon_=quest==3;
  for (int i=0;i<4;++i)
    if (!(input >> candidate.cooldowns_[i]) || !valid(candidate.cooldowns_[i],0,abilityCooldowns[i])) return false;
  int defeated=0, looted=0;
  for (auto& enemy : candidate.enemies_) {
    if (version==2) {
      EntityId id=0;
      if (!(input >> id) || id!=enemy.id) return false;
    }
    if (!(input >> enemy.position.x >> enemy.position.z >> enemy.health >> enemy.attackCooldown)) return false;
    if (!valid(enemy.position.x,-17,17) || !valid(enemy.position.z,-19,19) || !candidate.CanStand(enemy.position,.45f) ||
      !valid(enemy.health,0,80) || !valid(enemy.attackCooldown,0,1.4f)) return false;
    if (enemy.health==0) ++defeated;
    if (version==2) {
      int claimed=0,state=0;
      if (!(input >> claimed >> state) || claimed<0 || claimed>1 || state<0 || state>3 ||
        (claimed && enemy.health>0) || ((state==3)!=(enemy.health==0))) return false;
      enemy.looted=claimed!=0; looted+=claimed; enemy.state=static_cast<EnemyState>(state);
    } else enemy.state=enemy.health==0?EnemyState::Dead:EnemyState::Idle;
  }
  if ((quest>=2 && defeated==0) || candidate.gold_!=(quest==3?25:0)+5*looted || candidate.potions_>2+looted) return false;
  input >> std::ws;
  if (!input.eof()) return false;
  candidate.message_="Progress loaded. Jump and camera state reset.";
  *this=std::move(candidate);
  return true;
}
}
