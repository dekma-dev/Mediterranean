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
  obstacles_ = {{{-8,-8},2.8f,2.5f,4.f},{{8,-8},2.8f,2.5f,4.f},
    {{-10,3},1.3f,1.3f,3.f},{{10,1},1.5f,1.5f,3.4f},{{0,15},2,1,2}};
  for (Vec2 home : {Vec2{-4,7},Vec2{4,9},Vec2{0,12}})
    enemies_.push_back({home,home});
}

bool Simulation::CanStand(Vec2 p, float radius) const {
  if (std::abs(p.x) > 17-radius || std::abs(p.z) > 19-radius) return false;
  for (const auto& o : obstacles_) {
    const float nearX = std::clamp(p.x,o.center.x-o.halfX,o.center.x+o.halfX);
    const float nearZ = std::clamp(p.z,o.center.z-o.halfZ,o.center.z+o.halfZ);
    const float dx=p.x-nearX, dz=p.z-nearZ;
    if (dx*dx+dz*dz < radius*radius) return false;
  }
  return true;
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
    for (const auto& o : obstacles_)
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
  const Vec2 delta=Difference(to,from);
  const int steps=std::max(1,static_cast<int>(std::ceil(Length(delta)/.1f)));
  for (int i=1;i<steps;++i) {
    const float t=static_cast<float>(i)/steps;
    if (!CanStand({from.x+delta.x*t,from.z+delta.z*t},.05f)) return false;
  }
  return true;
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
    enemy.health=std::max(0.f,enemy.health-(index==0?20.f:35.f));
    enemy.hitFlash=.2f; attackFlash_=.2f;
    message_="Hit.";
    if (enemy.health<=0) {
      target_=-1; message_="Raider defeated.";
      if (quest_==Quest::Active) { quest_=Quest::Return; message_="Patrol complete. Return to the gold beacon and press E."; }
    }
  }
  cooldowns_[index]=abilityCooldowns[index];
}

void Simulation::Interact() {
  if (Length(Difference(player_,questGiver))>3) { message_="Move closer to the gold beacon to interact."; return; }
  if (quest_==Quest::Available) {
    const bool alreadyDefeated=std::any_of(enemies_.begin(),enemies_.end(),[](const Enemy& enemy) { return enemy.health<=0; });
    quest_=alreadyDefeated?Quest::Return:Quest::Active;
    message_=alreadyDefeated?"Patrol threat already defeated. Press E to claim the reward.":
      "Patrol accepted: defeat one raider beyond the outpost.";
  }
  else if (quest_==Quest::Return) { quest_=Quest::Complete; gold_+=25; health_=100; message_="Patrol rewarded: 25 gold. Press R to reset the slice."; }
  else if (quest_==Quest::Active) message_="Defeat one raider, then return here.";
  else message_="The patrol is complete. Reward already received.";
}

void Simulation::Update(float seconds,const Command& command) {
  if (!std::isfinite(seconds) || seconds<=0) return;
  const float dt=std::min(seconds,.05f);
  if (command.restart) { *this=Simulation{}; return; }
  if (health_<=0) { message_="You have fallen. Press R to restart."; return; }
  for (auto& cooldown : cooldowns_) cooldown=std::max(0.f,cooldown-dt);
  attackFlash_=std::max(0.f,attackFlash_-dt);
  const float length=Length(command.movement);
  if (length>.001f) {
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
  UseAbility(command.ability);
  for (auto& enemy : enemies_) {
    enemy.hitFlash=std::max(0.f,enemy.hitFlash-dt);
    if (enemy.health<=0) continue;
    enemy.attackCooldown=std::max(0.f,enemy.attackCooldown-dt);
    Vec2 toPlayer=Difference(player_,enemy.position);
    const float distance=Length(toPlayer);
    const bool aggro=distance<9 && Length(Difference(player_,enemy.home))<13;
    Vec2 goal=aggro?player_:enemy.home;
    Vec2 direction=Difference(goal,enemy.position);
    const float remaining=Length(direction);
    if (remaining>(aggro?1.6f:.1f)) {
      const float step=std::min(remaining,2.2f*dt);
      Move(enemy.position,{direction.x/remaining*step,direction.z/remaining*step},.45f);
    }
    if (aggro && distance<=2 && height_<1.5f && enemy.attackCooldown<=0 && HasLineOfSight(enemy.position,player_)) {
      health_=std::max(0.f,health_-8); enemy.attackCooldown=1.4f;
    }
    if (!aggro && remaining<.2f) enemy.health=80;
  }
}

std::string Simulation::Save() const {
  std::ostringstream out;
  out << std::setprecision(9) << "MEDITERRANEAN 1\n" << player_.x << ' ' << player_.z << ' '
    << health_ << ' ' << facing_ << ' ' << static_cast<int>(quest_) << ' ' << gold_ << '\n';
  for (float cooldown : cooldowns_) out << cooldown << ' ';
  out << '\n';
  for (const auto& enemy : enemies_)
    out << enemy.position.x << ' ' << enemy.position.z << ' ' << enemy.health << ' ' << enemy.attackCooldown << '\n';
  return out.str();
}

bool Simulation::Load(const std::string& data) {
  if (data.size()>4096) return false;
  std::istringstream input(data);
  std::string magic; int version=0, quest=0;
  Simulation candidate;
  if (!(input >> magic >> version) || magic!="MEDITERRANEAN" || version!=1) return false;
  if (!(input >> candidate.player_.x >> candidate.player_.z >> candidate.health_ >> candidate.facing_ >> quest >> candidate.gold_)) return false;
  auto valid=[](float value,float min,float max) { return std::isfinite(value) && value>=min && value<=max; };
  if (!valid(candidate.player_.x,-17,17) || !valid(candidate.player_.z,-19,19) ||
    !candidate.CanStand(candidate.player_) || !valid(candidate.health_,0,100) ||
    !valid(candidate.facing_,-3.142f,3.142f) || quest<0 || quest>3 ||
    candidate.gold_!=(quest==3?25:0)) return false;
  candidate.quest_=static_cast<Quest>(quest);
  for (int i=0;i<4;++i)
    if (!(input >> candidate.cooldowns_[i]) || !valid(candidate.cooldowns_[i],0,abilityCooldowns[i])) return false;
  int defeated=0;
  for (auto& enemy : candidate.enemies_) {
    if (!(input >> enemy.position.x >> enemy.position.z >> enemy.health >> enemy.attackCooldown)) return false;
    if (!valid(enemy.position.x,-17,17) || !valid(enemy.position.z,-19,19) || !candidate.CanStand(enemy.position,.45f) ||
      !valid(enemy.health,0,80) || !valid(enemy.attackCooldown,0,1.4f)) return false;
    if (enemy.health==0) ++defeated;
  }
  if (quest>=2 && defeated==0) return false;
  input >> std::ws;
  if (!input.eof()) return false;
  candidate.message_="Progress loaded. Jump and camera state reset.";
  *this=std::move(candidate);
  return true;
}
}
