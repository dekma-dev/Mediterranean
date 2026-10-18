#include "game/Simulation.h"
#include <algorithm>
#include <cmath>

namespace rpg {
namespace {
float Distance(Vec2 a,Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
}

int Simulation::DefeatedCount() const {
  return static_cast<int>(std::count_if(enemies_.begin(),enemies_.end(),[](const Enemy& e) { return e.health<=0; }));
}

int Simulation::Experience() const {
  // Each persistent objective contributes once. Loading or respawning cannot farm XP.
  return 40*DefeatedCount()+(quest_==Quest::Complete?60:0)+(HasWardArmor()?120:0);
}

int Simulation::Level() const {
  const int xp=Experience();
  return xp>=250?3:xp>=100?2:1;
}

void Simulation::Emit(FeedbackKind kind,Vec2 position,int amount) {
  if (feedback_.size()==24) feedback_.erase(feedback_.begin());
  feedback_.push_back({position,kind,amount,0});
}

void Simulation::RestoreForLevel(int previousLevel) {
  if (Level()<=previousLevel) return;
  health_=MaxHealth();
  Emit(FeedbackKind::LevelUp,player_,Level());
}

void Simulation::DamagePlayer(float damage) {
  if (health_<=0) return;
  const float applied=std::min(sparring_?std::max(0.f,health_-1):health_,std::max(1.f,damage-DamageReduction()));
  health_-=applied; hurtFlash_=.25f;
  if (sparring_) sparringTaken_+=int(applied);
  Emit(FeedbackKind::Hurt,player_,static_cast<int>(applied));
  if (Channeling()) { channelRemaining_=0; message_="Damage interrupted stabilization."; }
  if (health_==0) { target_=-1; height_=0; velocityY_=0; attackFlash_=0; movementSpeed_=movementDistance_=0; }
  if (sparring_ && health_<=1) EndSparring("Yielded");
}
void Simulation::StartSparring() {
  trainer_=Enemy{}; trainer_.kind=EnemyKind::Trainer; trainer_.id=200;
  trainer_.position=trainer_.home=WorldMap::trainingPost; trainer_.health=trainer_.MaxHealth();
  trainer_.state=EnemyState::Pursuing; sparring_=true; sparringRemaining_=30; sparringReportTime_=0;
  sparringDealt_=sparringTaken_=0; sparringResult_.clear(); target_=int(enemies_.size());
  message_="Sparring: 30 seconds. E stops. Nonlethal; no XP or loot. Potions are consumed normally.";
}
void Simulation::EndSparring(const char* result) {
  sparring_=false; trainer_.windup=0; trainer_.path.clear(); target_=-1;
  sparringResult_=result; sparringReportTime_=8;
  message_=std::string("Sparring: ")+result+". Dealt "+std::to_string(sparringDealt_)+", taken "+std::to_string(sparringTaken_)+". No rewards or resource refunds.";
}

void Simulation::Respawn() {
  player_=WorldMap::spawn; height_=velocityY_=facing_=0; health_=MaxHealth();
  movementSpeed_=movementDistance_=0;
  target_=-1; attackFlash_=hurtFlash_=channelRemaining_=0;
  cooldowns_.fill(0); potionCooldown_=0; feedback_.clear();
  for (auto& enemy : enemies_) {
    if (enemy.health<=0) continue;
    enemy.position=enemy.home; enemy.health=enemy.MaxHealth(); enemy.state=EnemyState::Idle;
    enemy.windup=enemy.attackCooldown=enemy.hitFlash=enemy.repathTimer=0;
    enemy.movementSpeed=enemy.movementDistance=0;
    enemy.path.clear(); enemy.waypoint=0;
  }
  message_="Returned to Dawnwatch. Quests, XP, equipment, gold, and loot retained.";
}

std::string Simulation::Objective() const {
  if (quest_==Quest::Available) return "PATROL THE FRONTIER\nSpeak to the gold beacon keeper";
  if (quest_==Quest::Active) return "PATROL THE FRONTIER\nDefeat one raider north of the village";
  if (quest_==Quest::Return) return "PATROL THE FRONTIER\nReturn to the beacon keeper";
  if (anchorQuest_==AnchorQuest::Available) return "SECURE THE ASTRAL ANCHOR\nAccept the next quest at the beacon";
  if (anchorQuest_==AnchorQuest::Active) {
    if (DefeatedCount()<3) return "SECURE THE ASTRAL ANCHOR\nRaiders defeated "+std::to_string(DefeatedCount())+" OF 3";
    return "SECURE THE ASTRAL ANCHOR\nPress E at the northern crystal";
  }
  if (anchorQuest_==AnchorQuest::Return) return "SECURE THE ASTRAL ANCHOR\nReturn to the beacon for your reward";
  return "DAWNWATCH SECURED\nOptional: spar at the eastern training post";
}

Vec2 Simulation::ObjectivePosition() const {
  if (quest_==Quest::Active || (anchorQuest_==AnchorQuest::Active && DefeatedCount()<3)) {
    Vec2 nearest=WorldMap::astralAnchor; float best=1000;
    for (const auto& enemy : enemies_) if (enemy.health>0 && Distance(player_,enemy.position)<best) {
      nearest=enemy.position; best=Distance(player_,nearest);
    }
    return nearest;
  }
  return anchorQuest_==AnchorQuest::Complete?WorldMap::trainingPost:anchorQuest_==AnchorQuest::Active?WorldMap::astralAnchor:questGiver;
}

std::string Simulation::InteractionPrompt() const {
  if (health_<=0) return "ENTER RETURN TO DAWNWATCH";
  if (sparring_) return "E END SPARRING ROUND";
  if (Channeling()) return "STABILIZING - STAND STILL";
  if (Distance(player_,questGiver)<=3) {
    if (quest_==Quest::Available) return "E ACCEPT FRONTIER PATROL";
    if (quest_==Quest::Return || anchorQuest_==AnchorQuest::Return) return "E CLAIM QUEST REWARD";
    if (anchorQuest_==AnchorQuest::Available) return "E ACCEPT ASTRAL ANCHOR QUEST";
    return "E SPEAK TO BEACON KEEPER";
  }
  if (anchorQuest_==AnchorQuest::Active && DefeatedCount()==3 &&
    Distance(player_,WorldMap::astralAnchor)<=2.5f && HasLineOfSight(player_,WorldMap::astralAnchor))
    return "E STABILIZE ASTRAL ANCHOR";
  for (const auto& enemy : enemies_)
    if (enemy.health<=0 && !enemy.looted && Distance(player_,enemy.position)<=2.5f && HasLineOfSight(player_,enemy.position))
      return "E LOOT - 5 GOLD AND A POTION";
  if (NearQuartermaster()) return "E BROWSE QUARTERMASTER SUPPLIES";
  if (Distance(player_,WorldMap::trainingPost)<=2.5f && HasLineOfSight(player_,WorldMap::trainingPost))
    return anchorQuest_==AnchorQuest::Complete?"E START SPARRING - NO XP OR LOOT":"TRAINING UNLOCKS AFTER BOTH QUESTS";
  return "";
}
}
