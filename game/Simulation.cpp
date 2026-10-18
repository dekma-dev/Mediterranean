#include "game/Simulation.h"
#include "game/Motion.h"
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
  trainer_.position=trainer_.home=WorldMap::trainingPost; trainer_.id=200; trainer_.kind=EnemyKind::Trainer; trainer_.health=trainer_.MaxHealth();
  EntityId id=100;
  for (Vec2 home : {Vec2{-4,7},Vec2{4,9},Vec2{0,12}}) {
    Enemy enemy; enemy.position=home; enemy.home=home; enemy.id=id++;
    if (enemy.id==102) { enemy.kind=EnemyKind::Captain; enemy.health=enemy.MaxHealth(); }
    enemies_.push_back(enemy);
  }
}
float Simulation::DamageReduction() const {
  float value=0;
  for (int i=0;i<Inventory::gearCount;++i) value+=DescribeItem(inventory_.Worn(GearSlot(i))).armor;
  return value;
}

bool Simulation::CanStand(Vec2 p, float radius) const {
  return map_.CanStand(p,radius);
}

void Simulation::Move(Vec2& p, Vec2 displacement, float radius) {
  p=map_.MoveAndSlide(p,displacement,radius);
}

float Simulation::CameraDistance(Vec2 origin,float originY,Vec2 direction,float slope,float desired) const {
  // Conservative camera bounds include pitched roofs, chimneys and tree crowns.
  for (float d=.3f;d<desired;d+=.1f) {
    const Vec2 p{origin.x+direction.x*d,origin.z+direction.z*d};
    const float y=originY+slope*d;
    for (const auto& o : map_.Obstacles()) {
      // Above the counter, test the open frame and bowed cloth separately.
      // A full-height box would pull the ordinary shop camera into the player.
      if (o.kind==PropKind::Counter && y>o.height+.1f) {
        const float dx=std::abs(p.x-o.center.x),dz=p.z-o.center.z;
        for (float side : {-1.f,1.f})
          if (y<2.72f && std::abs(p.x-o.center.x-side*(o.halfX-.06f))<.18f && std::abs(dz)<.21f)
            return std::max(.25f,d-.2f);
        const bool brace=y>2.05f && y<2.7f && dx>.5f && dx<o.halfX+.15f && std::abs(dz)<.2f;
        const float depth=std::clamp(dz/1.35f,-.5f,.5f);
        const float roofY=2.63f+.10f*std::cos(depth*3.14159265f)-.12f*depth;
        const bool cloth=dx<1.435f && std::abs(dz)<.735f &&
          (std::abs(y-roofY)<.06f || (dz<-.60f && y>2.50f && y<2.75f));
        if (brace || cloth) return std::max(.25f,d-.2f);
        continue;
      }
      // The crown begins above the character's shoulders. Extending its wide
      // bounds down to the ground traps the camera beside the training post.
      const bool belowCrown=o.kind==PropKind::Tree && y<1.65f;
      const float halfX=belowCrown?o.halfX:o.CameraHalfX();
      const float halfZ=belowCrown?o.halfZ:o.CameraHalfZ();
      if (y < o.CameraHeight() && y > -.2f && std::abs(p.x-o.center.x)<halfX+.15f &&
        std::abs(p.z-o.center.z)<halfZ+.15f) return std::max(.25f,d-.2f);
    }
  }
  return desired;
}

void Simulation::SelectTarget() {
  if (sparring_) { target_=int(enemies_.size()); return; }
  const int count=static_cast<int>(enemies_.size());
  for (int step=1;step<=count;++step) {
    const int index=(target_+step)%count;
    if (enemies_[index].health>0 && enemies_[index].state!=EnemyState::Returning &&
      Length(Difference(enemies_[index].position,player_))<=22) {
      target_=index; message_="Target selected. 1: Strike | 2: Heavy strike | 3: Dash | 4: Mend"; return;
    }
  }
  target_=-1; message_="No living target nearby.";
}
const Enemy* Simulation::TargetActor() const {
  if (target_==int(enemies_.size()) && sparring_) return &trainer_;
  return target_>=0 && target_<int(enemies_.size())?&enemies_[target_]:nullptr;
}

bool Simulation::HasLineOfSight(Vec2 from,Vec2 to) const {
  return map_.CanTravel(from,to,.05f);
}

void Simulation::UseAbility(int index) {
  if (index<0 || index>=4 || health_<=0) return;
  if (cooldowns_[index]>0) { message_="That ability is cooling down."; return; }
  if (index==3) {
    if (health_>=MaxHealth()) { message_="Health is already full."; return; }
    const float healed=std::min(35.f,MaxHealth()-health_);
    health_+=healed; Emit(FeedbackKind::Heal,player_,static_cast<int>(healed));
    EmitVisual(CombatVisualKind::Mend,player_,height_,facing_);
    message_="Mend restored health.";
  } else if (index==2) {
    Move(player_,{std::sin(facing_)*3.5f,std::cos(facing_)*3.5f},.4f);
    dashedThisUpdate_=true;
    message_="Dash.";
  } else {
    if (!TargetActor() || TargetActor()->health<=0) { message_="Press Tab to select an enemy."; return; }
    const bool trainingTarget=sparring_ && target_==int(enemies_.size());
    auto& enemy=trainingTarget?trainer_:enemies_[target_];
    if (enemy.state==EnemyState::Returning) { message_="That enemy is returning home."; return; }
    if (Length(Difference(enemy.position,player_))>2.5f || height_>1.5f) {
      message_="Target is out of melee range."; return;
    }
    if (!HasLineOfSight(player_,enemy.position)) { message_="The target is behind an obstacle."; return; }
    const auto direction=Difference(enemy.position,player_);
    facing_=std::atan2(direction.x,direction.z);
    const int previousLevel=Level();
    const float damage=std::min(enemy.health,index==0?StrikeDamage():HeavyDamage());
    enemy.health-=damage;
    if (trainingTarget) sparringDealt_+=int(damage);
    Emit(FeedbackKind::Damage,enemy.position,static_cast<int>(damage));
    EmitVisual(index==0?CombatVisualKind::Strike:CombatVisualKind::HeavyStrike,player_,height_,facing_);
    EmitVisual(index==0?CombatVisualKind::Hit:CombatVisualKind::HeavyHit,enemy.position,1.25f,facing_,enemy.id);
    enemy.hitFlash=.2f; enemy.damagedThisUpdate=true; attackFlash_=.2f;
    message_="Hit.";
    enemy.state=EnemyState::Pursuing;
    if (index==1 && enemy.windup>0) {
      enemy.windup=0; enemy.attackCooldown=.8f;
      Emit(FeedbackKind::Interrupt,enemy.position);
      EmitVisual(CombatVisualKind::Interrupt,enemy.position,1.4f,facing_,enemy.id);
      message_="Heavy strike interrupted the enemy attack.";
    }
    if (enemy.health<=0) {
      target_=-1; enemy.state=EnemyState::Dead; enemy.path.clear(); enemy.windup=0;
      if (trainingTarget) { EndSparring("Victory"); cooldowns_[index]=abilityCooldowns[index]; return; }
      Emit(FeedbackKind::Experience,enemy.position,40);
      message_="Raider defeated. Press E nearby to loot.";
      if (quest_==Quest::Active) { quest_=Quest::Return; message_="Patrol complete. Return to the gold beacon and press E."; }
      RestoreForLevel(previousLevel);
    }
  }
  cooldowns_[index]=abilityCooldowns[index];
}

const char* Simulation::AbilityStatus(int index) const {
  if (index<0 || index>=4 || health_<=0) return "DISABLED";
  if (cooldowns_[index]>0) return "COOLDOWN";
  if (index==3) return health_>=MaxHealth()?"FULL HEALTH":"READY";
  if (index==2) return "READY";
  const auto* target=TargetActor();
  if (!target || target->health<=0) return "NO TARGET";
  if (target->state==EnemyState::Returning) return "RETURNING";
  if (Length(Difference(target->position,player_))>2.5f || height_>1.5f) return "OUT OF RANGE";
  return HasLineOfSight(player_,target->position)?"READY":"BLOCKED";
}

void Simulation::Interact() {
  if (sparring_) { EndSparring("Stopped"); return; }
  const bool nearbyLoot=std::any_of(enemies_.begin(),enemies_.end(),[&](const Enemy& e) {
    return e.health<=0 && !e.looted && Length(Difference(player_,e.position))<=2.5f && HasLineOfSight(player_,e.position);
  });
  if (!nearbyLoot && NearQuartermaster()) {
    if (CanTrade()) { tradeRequested_=true; message_="Quartermaster: supplies and equipment for the Dawnwatch patrol."; }
    else message_="Land before speaking to the quartermaster.";
    return;
  }
  if (!nearbyLoot && Length(Difference(player_,WorldMap::trainingPost))<=2.5f && HasLineOfSight(player_,WorldMap::trainingPost)) {
    if (anchorQuest_!=AnchorQuest::Complete) message_="Secure Dawnwatch before starting a sparring round.";
    else if (!Grounded()) message_="Land before starting a sparring round.";
    else StartSparring();
    return;
  }
  if (Length(Difference(player_,WorldMap::astralAnchor))<=2.5f &&
    HasLineOfSight(player_,WorldMap::astralAnchor)) {
    if (anchorQuest_!=AnchorQuest::Active) {
      message_=anchorQuest_>=AnchorQuest::Return?"The astral anchor is stable.":"Speak to the beacon keeper about the astral anchor.";
    } else if (DefeatedCount()!=static_cast<int>(enemies_.size())) {
      message_="Defeat all three raiders before stabilizing the anchor.";
    } else if (!Grounded()) message_="Land before stabilizing the anchor.";
    else if (!Channeling()) { channelRemaining_=channelDuration; message_="Stabilizing the anchor. Stand still for three seconds."; }
    // Loot still takes precedence over an unavailable/already stable anchor.
    if (Channeling()) return;
  }
  if (Length(Difference(player_,questGiver))>3) {
    Enemy* nearest=nullptr; float best=2.5f;
    for (auto& enemy : enemies_) {
      const float distance=Length(Difference(player_,enemy.position));
      if (enemy.health<=0 && !enemy.looted && distance<=best && HasLineOfSight(player_,enemy.position)) {
        nearest=&enemy; best=distance;
      }
    }
    if (nearest) {
      if (!inventory_.Add(Item::Potion)) { message_="Your bag is full. Make room before looting."; return; }
      nearest->looted=true; gold_+=5;
      message_="Looted 5 gold and one healing potion. Q uses a potion.";
    } else if (Length(Difference(player_,WorldMap::astralAnchor))>2.5f)
      message_="Move near the gold beacon, astral anchor, or loot bag and press E.";
    return;
  }
  if (quest_==Quest::Available) {
    const bool alreadyDefeated=std::any_of(enemies_.begin(),enemies_.end(),[](const Enemy& enemy) { return enemy.health<=0; });
    quest_=alreadyDefeated?Quest::Return:Quest::Active;
    message_=alreadyDefeated?"Patrol threat already defeated. Press E to claim the reward.":
      "Patrol accepted: defeat one raider beyond the outpost.";
  }
  else if (quest_==Quest::Return) {
    const int previousLevel=Level();
    if (!inventory_.Add(Item::Watchguard)) { message_="Make room in your bag to claim the blade."; return; }
    quest_=Quest::Complete; gold_+=25; anchorQuest_=AnchorQuest::Available;
    Emit(FeedbackKind::Experience,player_,60); RestoreForLevel(previousLevel); health_=MaxHealth();
    message_="Patrol reward: blade, 25 gold, 60 XP. G equip. E for the next quest.";
  }
  else if (quest_==Quest::Active) message_="Defeat one raider, then return here.";
  else if (anchorQuest_==AnchorQuest::Available) {
    anchorQuest_=AnchorQuest::Active;
    message_="Secure the anchor: defeat the camp and stabilize the crystal in the north.";
  } else if (anchorQuest_==AnchorQuest::Return) {
    const int previousLevel=Level();
    if (!inventory_.Add(Item::Ward)) { message_="Make room in your bag to claim the armor."; return; }
    inventory_.Equip(Item::Ward);
    anchorQuest_=AnchorQuest::Complete; gold_+=50;
    Emit(FeedbackKind::Experience,player_,120); RestoreForLevel(previousLevel); health_=MaxHealth();
    message_="Outpost secured: 50 gold, 120 XP, ward armor. Damage taken reduced by 2.";
  } else if (anchorQuest_==AnchorQuest::Active) message_="Clear the raider camp, then press E at the northern crystal.";
  else message_="Dawnwatch is secure. The eastern training post offers repeatable sparring.";
}

void Simulation::Update(float seconds,const Command& command) {
  visualEventCount_=0;
  tradeRequested_=false;
  movementDistance_=0;
  groundMovementDistance_=0;
  landingSpeed_=landingElapsed_=0; damagedThisUpdate_=false;
  jumpedThisUpdate_=dashedThisUpdate_=false;
  for (auto& enemy : enemies_) { enemy.movementDistance=0; enemy.damagedThisUpdate=false; }
  trainer_.movementDistance=0; trainer_.damagedThisUpdate=false;
  if (!std::isfinite(seconds) || seconds<=0) return;
  const float dt=std::min(seconds,.05f);
  if (command.restart) { *this=Simulation{}; return; }
  for (auto& feedback : feedback_) feedback.age+=dt;
  feedback_.erase(std::remove_if(feedback_.begin(),feedback_.end(),[](const CombatFeedback& f) { return f.age>=1.5f; }),feedback_.end());
  hurtFlash_=std::max(0.f,hurtFlash_-dt);
  // Cosmetic flashes must finish even while combat is suspended after defeat.
  for (auto& enemy : enemies_) enemy.hitFlash=std::max(0.f,enemy.hitFlash-dt);
  trainer_.hitFlash=std::max(0.f,trainer_.hitFlash-dt);
  sparringReportTime_=std::max(0.f,sparringReportTime_-dt);
  if (health_<=0) {
    jumpBuffer_=0;
    if (command.respawn) Respawn();
    else message_="You have fallen. Enter returns to Dawnwatch with progress retained. R resets all.";
    return;
  }
  for (auto& cooldown : cooldowns_) cooldown=std::max(0.f,cooldown-dt);
  potionCooldown_=std::max(0.f,potionCooldown_-dt);
  attackFlash_=std::max(0.f,attackFlash_-dt);
  const float length=Length(command.movement);
  const bool moving=std::isfinite(command.movement.x) && std::isfinite(command.movement.z) &&
    std::isfinite(length) && length>.001f;
  const bool changingGear=command.equipWeapon || command.inventory.action==InventoryAction::Equip || command.inventory.action==InventoryAction::Unequip;
  if (Channeling() && (moving || command.jump || command.ability>=0 || command.usePotion || changingGear)) {
    channelRemaining_=0; message_="Stabilization interrupted. Press E at the anchor to try again.";
  }
  // Apply takeoff before horizontal integration, and switch traction at the
  // actual landing instant rather than at the following frame boundary.
  if (command.haltMovement) jumpBuffer_=0;
  else if (command.jump) jumpBuffer_=.12f;
  const auto launch=[&]() {
    jumpBuffer_=0;
    velocityY_=motion::jumpSpeed; jumpedThisUpdate_=true;
    airSpeedLimit_=command.sprint?7.f:4.f;
    // A slow directional takeoff has a leg-driven launch impulse. Running
    // jumps preserve existing momentum, including momentum against new input.
    // Taper the push continuously with speed rather than using a rest threshold.
    if (moving) {
      const float push=airSpeedLimit_*.6f*std::min(1.f,length);
      const float speed=Length(movementVelocity_);
      if (speed<push && movementVelocity_.x*command.movement.x+movementVelocity_.z*command.movement.z>=0) {
        movementVelocity_.x+=command.movement.x/length*(push-speed);
        movementVelocity_.z+=command.movement.z/length*(push-speed);
      }
    }
  };
  if (jumpBuffer_>0 && Grounded()) launch();
  const bool inFlight=!Grounded();
  float contact=0,impact=0;
  if (inFlight) {
    impact=std::sqrt(velocityY_*velocityY_+2*motion::gravity*height_);
    contact=velocityY_>=0?(velocityY_+impact)/motion::gravity:2*height_/(impact-velocityY_);
  }
  const auto integrateHorizontal=[&](float interval,bool airborne) {
    if (interval<=0) return;
    Vec2 target{};
    if (moving) {
      const float speed=(airborne?airSpeedLimit_:command.sprint?7.f:4.f)*std::min(1.f,length);
      target={command.movement.x/length*speed,command.movement.z/length*speed};
      facing_=std::atan2(target.x,target.z);
    } else if (airborne) target=movementVelocity_; // Keep takeoff momentum without input.
    const float response=airborne?3.f:moving?24.f:32.f;
    const Vec2 travel{motion::IntegrateVelocity(movementVelocity_.x,target.x,response,interval),
      motion::IntegrateVelocity(movementVelocity_.z,target.z,response,interval)};
    const auto before=player_;
    Move(player_,travel,.4f);
    const auto actual=Difference(player_,before);
    movementDistance_+=Length(actual);
    if (!airborne) groundMovementDistance_+=Length(actual);
    // Remove velocity into the contact, preserving motion along the wall.
    const Vec2 blocked{travel.x-actual.x,travel.z-actual.z};
    const float blockedLength=Length(blocked);
    if (blockedLength>.00001f) {
      const Vec2 normal{blocked.x/blockedLength,blocked.z/blockedLength};
      const float into=std::max(0.f,movementVelocity_.x*normal.x+movementVelocity_.z*normal.z);
      movementVelocity_.x-=normal.x*into; movementVelocity_.z-=normal.z*into;
    }
    if (!airborne && !moving && Length(movementVelocity_)<.01f) movementVelocity_={};
  };
  if (command.haltMovement) movementVelocity_={};
  else if (inFlight) integrateHorizontal(std::min(dt,contact),true);
  else integrateHorizontal(dt,false);
  // Solve contact inside the step, retaining its speed and elapsed time for
  // presentation. End-of-frame velocity has already been zeroed at the floor.
  if (inFlight) {
    if (contact<=dt+1e-6f) {
      landingSpeed_=impact; landingElapsed_=std::max(0.f,dt-contact);
      height_=velocityY_=0;
      // Consume a recent press at the exact contact time, not one frame later.
      // A rebound gets air steering for the remainder, never ground traction.
      if (jumpBuffer_>0 && contact<=jumpBuffer_) {
        launch();
        integrateHorizontal(landingElapsed_,true);
        height_=velocityY_*landingElapsed_-.5f*motion::gravity*landingElapsed_*landingElapsed_;
        velocityY_-=motion::gravity*landingElapsed_;
      } else if (!command.haltMovement) integrateHorizontal(landingElapsed_,false);
    } else {
      height_+=velocityY_*dt-.5f*motion::gravity*dt*dt; velocityY_-=motion::gravity*dt;
    }
  }
  jumpBuffer_=std::max(0.f,jumpBuffer_-dt);
  if (command.nextTarget) SelectTarget();
  if (command.interact) Interact();
  if (command.equipWeapon) {
    const auto next=HasRewardWeapon() && !RewardWeaponEquipped()?Item::Watchguard:Item::Starter;
    if (inventory_.Equip(next)) message_=std::string(DescribeItem(next).name)+" equipped.";
    else message_="No other sword in your bag. Reclaim sold gear at the quartermaster.";
  }
  if (command.inventory.action!=InventoryAction::None) {
    if (inventory_.Apply(command.inventory)) message_=command.inventory.action==InventoryAction::Sort?"Bag sorted.":
      command.inventory.action==InventoryAction::Move?"Item moved.":command.inventory.action==InventoryAction::Split?"Stack split. Drag either stack to move it.":"Equipment updated.";
    else message_="That item cannot go there. Equipment and bag are unchanged.";
  }
  if (command.purchase!=-1) Buy(command.purchase);
  if (command.trade.action!=TradeAction::None) Trade(command.trade);
  if (command.usePotion) {
    if (Potions()==0) message_="No healing potions. Loot defeated raiders with E.";
    else if (health_>=MaxHealth()) message_="Health is already full. Potion retained.";
    else if (potionCooldown_>0) message_="Potion is cooling down.";
    else if (inventory_.Consume(Item::Potion,command.potionSource)) {
      const float healed=std::min(40.f,MaxHealth()-health_); health_+=healed;
      Emit(FeedbackKind::Heal,player_,static_cast<int>(healed));
      EmitVisual(CombatVisualKind::Potion,player_,height_,facing_);
      potionCooldown_=3; message_="Healing potion restored health.";
    } else message_="That potion stack has moved. Select it again.";
  }
  UseAbility(command.ability);
  for (auto& enemy : enemies_) UpdateEnemy(enemy,dt);
  if (sparring_) {
    sparringRemaining_=std::max(0.f,sparringRemaining_-dt);
    if (Length(Difference(player_,WorldMap::trainingPost))>7) EndSparring("Left the arena");
    else if (sparringRemaining_==0) EndSparring("Time complete");
    else UpdateEnemy(trainer_,dt);
  }
  if (health_>0) SeparateEnemies(dt);
  if (Channeling()) {
    if (moving || !Grounded() || command.ability>=0 || command.usePotion || changingGear ||
      Length(Difference(player_,WorldMap::astralAnchor))>2.5f || !HasLineOfSight(player_,WorldMap::astralAnchor)) {
      channelRemaining_=0; message_="Stabilization interrupted. Stand still at the anchor.";
    } else {
      // Only a validated stationary channel ends approach inertia. Retain the
      // current frame's measured travel for animation; rejected casts do not stop movement.
      movementVelocity_={};
      channelRemaining_=std::max(0.f,channelRemaining_-dt);
      if (channelRemaining_==0) {
        anchorQuest_=AnchorQuest::Return;
        message_="Astral anchor stabilized. Return to the beacon keeper for your reward.";
      }
    }
  }
}

void Simulation::SeparateEnemies(float dt) {
  for (std::size_t i=0;i<enemies_.size();++i) for (std::size_t j=i+1;j<enemies_.size();++j) {
    auto& a=enemies_[i]; auto& b=enemies_[j];
    if (a.health<=0 || b.health<=0 || a.windup>0 || b.windup>0) continue;
    auto delta=Difference(a.position,b.position);
    const float distance=Length(delta);
    if (distance>=.9f) continue;
    if (distance<.0001f) delta={1,0};
    else { delta.x/=distance; delta.z/=distance; }
    const float correction=std::min((.9f-distance)*.5f,1.5f*dt);
    const auto beforeA=a.position,beforeB=b.position;
    Move(a.position,{delta.x*correction,delta.z*correction},.45f);
    Move(b.position,{-delta.x*correction,-delta.z*correction},.45f);
    a.movementDistance+=Length(Difference(a.position,beforeA));
    b.movementDistance+=Length(Difference(b.position,beforeB));
  }
}

void Simulation::UpdateEnemy(Enemy& enemy,float dt) {
  if (enemy.health<=0) { enemy.state=EnemyState::Dead; enemy.windup=enemy.movementSpeed=0; return; }
  if (health_<=0) return;
  enemy.attackCooldown=std::max(0.f,enemy.attackCooldown-dt);
  const float distance=Length(Difference(player_,enemy.position));
  const EnemyState previous=enemy.state;
  if (enemy.state==EnemyState::Idle && distance<9 && HasLineOfSight(enemy.position,player_))
    enemy.state=EnemyState::Pursuing;
  if (enemy.state==EnemyState::Pursuing && (Length(Difference(player_,enemy.home))>13 || health_<=0))
    enemy.state=EnemyState::Returning;
  if (enemy.state==EnemyState::Returning) enemy.windup=0;
  if (enemy.state==EnemyState::Returning && Length(Difference(enemy.position,enemy.home))<.015f) {
    enemy.position=enemy.home; enemy.health=enemy.MaxHealth(); enemy.state=EnemyState::Idle; enemy.path.clear();
  }
  if (enemy.state==EnemyState::Idle) { enemy.movementSpeed=0; return; }
  // Commit facing and position during a windup. Players can step outside its cone,
  // dash away, or interrupt it with heavy strike before the impact is resolved.
  if (enemy.windup>0) {
    enemy.movementSpeed=0;
    enemy.windup=std::max(0.f,enemy.windup-dt);
    if (enemy.windup==0) {
      EmitVisual(CombatVisualKind::EnemySwing,enemy.position,0,enemy.facing,enemy.id);
      const auto delta=Difference(player_,enemy.position);
      const float dot=distance<.001f?1.f:(std::sin(enemy.facing)*delta.x+std::cos(enemy.facing)*delta.z)/distance;
      if (distance<=Enemy::attackReach && dot>=std::cos(Enemy::attackHalfAngle) && height_<1.5f &&
        health_>0 && HasLineOfSight(enemy.position,player_)) DamagePlayer(enemy.Damage(),enemy.facing);
      else Emit(FeedbackKind::Miss,enemy.position);
      enemy.attackCooldown=enemy.AttackInterval();
    }
    return;
  }
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
  if (map_.CanTravel(enemy.position,goal,.45f)) {
    // Track a visible moving destination every update; a cached goal produced
    // sideways pursuit and sharp course corrections every 650 ms.
    enemy.path.resize(1); enemy.path[0]=goal;
    enemy.waypoint=0; enemy.repathTimer=.65f;
  } else if (previous!=enemy.state || enemy.repathTimer<=0 || enemy.waypoint>=enemy.path.size()) {
    // Stop just short of the player's center; navigation uses static world clearance.
    enemy.path=map_.FindPath(enemy.position,goal,.45f);
    enemy.waypoint=0; enemy.repathTimer=.65f;
  }
  const bool pursuing=enemy.state==EnemyState::Pursuing;
  const bool clearSight=pursuing && HasLineOfSight(enemy.position,player_);
  // Consume reached waypoints in the same update instead of idling for a frame.
  while (enemy.waypoint<enemy.path.size() && Length(Difference(enemy.path[enemy.waypoint],enemy.position))<.001f)
    ++enemy.waypoint;
  const bool navigating=!(clearSight && distance<=1.6f) && remaining>.001f && enemy.waypoint<enemy.path.size();
  const auto heading=navigating?Difference(enemy.path[enemy.waypoint],enemy.position):Difference(player_,enemy.position);
  const float desiredFacing=std::atan2(heading.x,heading.z);
  enemy.facing=motion::Turn(enemy.facing,desiredFacing,10.f,dt);
  const float alignment=std::max(0.f,std::cos(motion::AngleDelta(enemy.facing,desiredFacing)));
  // Close range alone is not arrival: a trunk/corner can still block the swing.
  const float arrival=clearSight?std::max(0.f,distance-1.6f):remaining;
  float budget=motion::Travel(enemy.movementSpeed,navigating?std::min(2.2f,arrival*8.f)*alignment:0.f,16.f,dt);
  budget=std::min(budget,arrival);
  while (budget>.00001f && enemy.waypoint<enemy.path.size()) {
    const auto direction=Difference(enemy.path[enemy.waypoint],enemy.position);
    const float length=Length(direction);
    if (length<.001f) { ++enemy.waypoint; continue; }
    const float step=std::min(length,budget);
    const auto before=enemy.position;
    Move(enemy.position,{direction.x/length*step,direction.z/length*step},.45f);
    const float travel=Length(Difference(enemy.position,before));
    enemy.movementDistance+=travel; budget-=step;
    if (travel<step*.5f) { enemy.movementSpeed=0; enemy.repathTimer=0; break; }
    if (Length(Difference(enemy.path[enemy.waypoint],enemy.position))<.001f) ++enemy.waypoint;
  }
  if (enemy.waypoint==enemy.path.size()) enemy.repathTimer=0;
  const float attackFacing=std::atan2(player_.x-enemy.position.x,player_.z-enemy.position.z);
  if (pursuing && Length(Difference(player_,enemy.position))<=2 && height_<1.5f && health_>0 && enemy.attackCooldown<=0 &&
    std::abs(motion::AngleDelta(enemy.facing,attackFacing))<.1f && HasLineOfSight(enemy.position,player_)) {
    enemy.windup=enemy.WindupDuration(); enemy.path.clear(); enemy.movementSpeed=0;
  }
}

std::string Simulation::Save() const {
  std::ostringstream out;
  out << std::setprecision(9) << "MEDITERRANEAN 6\n" << player_.x << ' ' << player_.z << ' '
    << health_ << ' ' << facing_ << ' ' << static_cast<int>(quest_) << ' ' << gold_ << '\n';
  out << Potions() << ' ' << HasRewardWeapon() << ' ' << RewardWeaponEquipped() << ' ' << potionCooldown_ << '\n';
  out << static_cast<int>(anchorQuest_) << '\n';
  for (float cooldown : cooldowns_) out << cooldown << ' ';
  out << '\n';
  for (const auto& enemy : enemies_)
    out << enemy.id << ' ' << enemy.position.x << ' ' << enemy.position.z << ' ' << enemy.health << ' '
      << enemy.attackCooldown << ' ' << enemy.looted << ' ' << static_cast<int>(enemy.state) << ' '
      << enemy.windup << ' ' << enemy.facing << '\n';
  out<<"SHOP"; for (auto count : purchases_) out<<' '<<count; out<<'\n';
  out<<"BUYBACK"; for (auto count : buyback_) out<<' '<<count; out<<'\n';
  inventory_.Write(out);
  return out.str();
}

bool Simulation::Load(const std::string& data) {
  if (data.size()>4096) return false;
  std::istringstream input(data);
  std::string magic; int version=0, quest=0,potions=2,owned=0,equipped=0;
  Simulation candidate;
  if (!(input >> magic >> version) || magic!="MEDITERRANEAN" || version<1 || version>6) return false;
  // The shop counter occupies formerly walkable ground. Preserve old saves by
  // moving only actors overlapping that new prop to its nearest free edge.
  const auto migrateCounter=[&](Vec2& position,float radius) {
    if (version>=5 || !std::isfinite(position.x) || !std::isfinite(position.z) || candidate.map_.CanStand(position,radius)) return;
    for (const auto& prop : candidate.map_.Obstacles()) {
      if (prop.kind!=PropKind::Counter) continue;
      const Vec2 nearest{std::clamp(position.x,prop.center.x-prop.halfX,prop.center.x+prop.halfX),
        std::clamp(position.z,prop.center.z-prop.halfZ,prop.center.z+prop.halfZ)};
      if (Length(Difference(position,nearest))>=radius) continue;
      const float x=prop.halfX+radius+.01f,z=prop.halfZ+radius+.01f;
      const Vec2 edges[]={{prop.center.x-x,position.z},{prop.center.x+x,position.z},
        {position.x,prop.center.z-z},{position.x,prop.center.z+z}};
      Vec2 result=position; float distance=100;
      for (const auto edge : edges) {
        const float delta=Length(Difference(position,edge));
        if (delta<distance && candidate.map_.CanStand(edge,radius)) { result=edge; distance=delta; }
      }
      position=result;
    }
  };
  if (!(input >> candidate.player_.x >> candidate.player_.z >> candidate.health_ >> candidate.facing_ >> quest >> candidate.gold_)) return false;
  migrateCounter(candidate.player_,.4f);
  auto valid=[](float value,float min,float max) { return std::isfinite(value) && value>=min && value<=max; };
  if (!valid(candidate.player_.x,-17,17) || !valid(candidate.player_.z,-19,19) ||
    !candidate.CanStand(candidate.player_) || !valid(candidate.health_,0,version<3?100.f:140.f) ||
    !valid(candidate.facing_,-3.142f,3.142f) || quest<0 || quest>3 || candidate.gold_<0 || (version<6 && candidate.gold_>90)) return false;
  candidate.quest_=static_cast<Quest>(quest);
  if (version>=2) {
    if (!(input >> potions >> owned >> equipped >> candidate.potionCooldown_) ||
      potions<0 || potions>(version>=5?5+shopOffers[0].stock:5) || owned<0 || owned>1 || equipped<0 || equipped>1 ||
      (equipped && !owned) || (version<6 && bool(owned)!=(quest==3)) || !valid(candidate.potionCooldown_,0,3)) return false;
  } else owned=quest==3;
  if (version>=3) {
    int anchor=0;
    if (!(input >> anchor) || anchor<0 || anchor>4 || ((quest==3)!=(anchor>0))) return false;
    candidate.anchorQuest_=static_cast<AnchorQuest>(anchor);
  } else candidate.anchorQuest_=quest==3?AnchorQuest::Available:AnchorQuest::Locked;
  for (int i=0;i<4;++i)
    if (!(input >> candidate.cooldowns_[i]) || !valid(candidate.cooldowns_[i],0,abilityCooldowns[i])) return false;
  int defeated=0, looted=0;
  for (auto& enemy : candidate.enemies_) {
    if (version>=2) {
      EntityId id=0;
      if (!(input >> id) || id!=enemy.id) return false;
    }
    if (!(input >> enemy.position.x >> enemy.position.z >> enemy.health >> enemy.attackCooldown)) return false;
    migrateCounter(enemy.position,.45f);
    if (!valid(enemy.position.x,-17,17) || !valid(enemy.position.z,-19,19) || !candidate.CanStand(enemy.position,.45f) ||
      !valid(enemy.health,0,version<3?80.f:enemy.MaxHealth()) ||
      !valid(enemy.attackCooldown,0,version<3?1.4f:enemy.AttackInterval())) return false;
    if (version<3) enemy.health=enemy.health/80.f*enemy.MaxHealth();
    if (enemy.health==0) ++defeated;
    if (version>=2) {
      int claimed=0,state=0;
      if (!(input >> claimed >> state) || claimed<0 || claimed>1 || state<0 || state>3 ||
        (claimed && enemy.health>0) || ((state==3)!=(enemy.health==0))) return false;
      enemy.looted=claimed!=0; looted+=claimed; enemy.state=static_cast<EnemyState>(state);
    } else enemy.state=enemy.health==0?EnemyState::Dead:EnemyState::Idle;
    if (version>=3) {
      if (!(input >> enemy.windup >> enemy.facing) || !valid(enemy.windup,0,enemy.WindupDuration()) ||
        !valid(enemy.facing,-3.142f,3.142f) ||
        (enemy.windup>0 && (enemy.state!=EnemyState::Pursuing || enemy.attackCooldown>0))) return false;
    }
  }
  if (version>=5) {
    std::string tag; if (!(input>>tag) || tag!="SHOP") return false;
    for (int i=0;i<int(shopOffers.size());++i) {
      auto& count=candidate.purchases_[i];
      if (!(input>>count) || count<0 || count>shopOffers[i].stock ||
        (count>0 && shopOffers[i].patrolRequired && quest!=int(Quest::Complete))) return false;
    }
  }
  if (version>=6) {
    std::string tag; if (!(input>>tag) || tag!="BUYBACK") return false;
    for (int i=0;i<int(Item::Count);++i)
      if (!(input>>candidate.buyback_[i]) || candidate.buyback_[i]<0 || candidate.buyback_[i]>(Item(i)==Item::Potion?11:1)) return false;
  }
  if ((quest==int(Quest::Active) && defeated>0) || (quest>=2 && defeated==0) ||
    candidate.gold_!=(quest==3?25:0)+5*looted+(candidate.HasWardArmor()?50:0)-candidate.GoldSpent()+candidate.BuybackValue() ||
    potions+candidate.BuybackCount(Item::Potion)>2+looted+candidate.purchases_[0] || candidate.health_>candidate.MaxHealth() ||
    (candidate.anchorQuest_>=AnchorQuest::Return && defeated!=3)) return false;
  if (version>=4) {
    if (!candidate.inventory_.Read(input,version==4?4:Inventory::gearCount) ||
      !candidate.inventory_.MatchesCampaign(quest==3,candidate.HasWardArmor(),potions,candidate.purchases_[1]!=0,candidate.purchases_[2]!=0,candidate.buyback_) ||
      candidate.HasRewardWeapon()!=(owned!=0) ||
      candidate.RewardWeaponEquipped()!=(equipped!=0)) return false;
  } else candidate.inventory_=Inventory::Migrate(potions,owned!=0,equipped!=0,candidate.HasWardArmor());
  input >> std::ws;
  if (!input.eof()) return false;
  candidate.message_="Progress loaded. Camera, jump, and unfinished anchor channel reset.";
  *this=std::move(candidate);
  return true;
}
}
