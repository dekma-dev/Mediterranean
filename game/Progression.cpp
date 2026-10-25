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
  return 40*((frontierCompleted_ || frontierActive_)?3:DefeatedCount())+250*frontierCompleted_+(quest_==Quest::Complete?60:0)+(HasWardArmor()?120:0);
}

int Simulation::Level() const {
  const int xp=Experience();
  int level=1;while(level<levelCap && xp>=levelExperience[level]) ++level;return level;
}

int Simulation::TalentPoints() const { return EarnedPoints()-TalentCount(build_); }

BuildStats Simulation::PreviewBuild(unsigned mask) const {
 // Preview locked level-three presets too, using current level and equipment.
 // Derive differences from the live accessors so equipment stays authoritative.
 if (!ValidBuild(mask,levelCap)) mask=build_;
 const auto delta=[&](float TalentInfo::*stat) { return BuildBonus(mask,stat)-BuildBonus(build_,stat); };
 return {MaxHealth()+delta(&TalentInfo::health),StrikeDamage()+delta(&TalentInfo::strike),
  HeavyDamage()+delta(&TalentInfo::heavy),DamageReduction()+delta(&TalentInfo::armor),
  MendHealing()+delta(&TalentInfo::mend),AbilityCooldown(2)-delta(&TalentInfo::dashCooldown),
  AbilityCooldown(3)-delta(&TalentInfo::mendCooldown),AbilityCooldown(0)-delta(&TalentInfo::basicCooldown),
  AbilityCooldown(1)-delta(&TalentInfo::heavyCooldown),ClassRecoveryCast(class_)/(1+BuildBonus(mask,&TalentInfo::castSpeed))};
}

const char* Simulation::LoadoutStatus(const BuildCommand& command) const {
 if (command.action!=BuildAction::Apply && command.action!=BuildAction::Store) return "Unknown build action";
 if (command.action==BuildAction::Store && (command.slot<0 || command.slot>=int(loadouts_.size()))) return "Choose a loadout slot";
 if (command.mask<0) return "Choose a build to preview";
 if (!ValidBuild(unsigned(command.mask),levelCap)) return "Invalid talents or prerequisites";
 if (!ValidBuild(unsigned(command.mask),Level())) return "Earn more levels to use this build";
 if (health_<=0) return "Recover before managing builds";
 if (command.action==BuildAction::Store)
  return loadouts_[command.slot]==command.mask?"Already saved in this slot":"Ready to store";
 if (unsigned(command.mask)==build_) return "Already your active build";
 const auto blocked=BuildChangeStatus();
 return *blocked?blocked:"Ready to apply";
}

void Simulation::ChangeBuild(const BuildCommand& command) {
 const auto status=LoadoutStatus(command);
 if (command.action==BuildAction::Store && std::string(status)=="Ready to store") {
  loadouts_[command.slot]=command.mask;
  message_="Build saved in loadout "+std::to_string(command.slot+1)+". Use F5 to save your expedition to disk.";
 } else if (command.action==BuildAction::Apply && std::string(status)=="Ready to apply") {
  build_=unsigned(command.mask);
  health_=std::min(health_,MaxHealth());
  message_="Build applied. Health and active cooldowns retained; unused points refunded.";
 } else message_=status;
}

const char* Simulation::TalentStatus(Talent talent) const {
 if (!ValidTalent(talent)) return "Unknown talent";
 if (HasTalent(talent)) return "Learned";
 const auto info=TalentDefinition(int(talent));
 if (Level()<info.level) return "Requires a higher level";
 if (info.prerequisite!=Talent::Count && !HasTalent(info.prerequisite)) return "Learn the previous talent first";
 if (TalentPoints()==0) return "No talent points available";
 const auto blocked=BuildChangeStatus();
 if (*blocked) return blocked;
 return "Ready to learn";
}

const char* Simulation::BuildChangeStatus() const {
 if (health_<=0) return "Recover before changing talents";
 if (!Grounded() || Dashing() || Channeling() || Attacking() || CastingRecovery()) return "Finish your current action";
 if (sparring_ || attackFlash_>0 || hurtFlash_>0 ||
   std::any_of(enemies_.begin(),enemies_.end(),[](const Enemy& e) { return e.health>0 && e.state==EnemyState::Pursuing; }))
  return "Finish combat before changing talents";
 return "";
}

const char* Simulation::TalentResetStatus() const {
 if (!build_) return "No learned talents to reset";
 const auto blocked=BuildChangeStatus();
 return *blocked?blocked:"Ready to reset";
}

void Simulation::ResetTalents() {
 const auto status=TalentResetStatus();
 if (std::string(status)!="Ready to reset") { message_=status; return; }
 build_=0;
 health_=std::min(health_,MaxHealth());
 message_="Talents reset. Points refunded; health and cooldowns are not restored.";
}

void Simulation::LearnTalent(int index) {
 const auto talent=Talent(index);
 const auto status=TalentStatus(talent);
 if (std::string(status)!="Ready to learn") { message_=status; return; }
 build_|=1u<<unsigned(talent);
 // Point allocation changes future stats, never health or active cooldowns.
 message_=std::string("Learned ")+TalentDefinition(index).name+".";
}

void Simulation::Emit(FeedbackKind kind,Vec2 position,int amount) {
  if (feedback_.size()==24) feedback_.erase(feedback_.begin());
  feedback_.push_back({position,kind,amount,0});
}

void Simulation::EmitVisual(CombatVisualKind kind,Vec2 position,float height,float facing,EntityId actor,float elapsed) {
  if (visualEventCount_<visualEvents_.size()) {
    auto& event=visualEvents_[visualEventCount_++];event={kind,position,height,facing,actor,elapsed};event.school=class_;
    event.source=player_;event.sourceHeight=height_;
    const bool attack=kind==CombatVisualKind::SpellCast || kind==CombatVisualKind::Strike ||
      kind==CombatVisualKind::HeavyStrike || kind==CombatVisualKind::Hit || kind==CombatVisualKind::HeavyHit;
    event.heavy=attack && HeavyAttack();event.empowered=attack && attackEmpowered_;
    if(kind==CombatVisualKind::SpellCast) {
      for(const auto& enemy:enemies_) if(enemy.id==attackTarget_) event.destination=enemy.position;
      if(sparring_ && trainer_.id==attackTarget_) event.destination=trainer_.position;
      event.flight=(combat::Contact(HeavyAttack())-combat::StrokeStart(HeavyAttack()))/AttackRate();
    }
  }
}

void Simulation::RestoreForLevel(int previousLevel) {
  if (Level()<=previousLevel) return;
  health_=MaxHealth();
  Emit(FeedbackKind::LevelUp,player_,Level());
}

void Simulation::DamagePlayer(float damage,float incomingFacing,float visualElapsed) {
  if (health_<=0) return;
  if(class_==ClassId::Scout && evasion_>0) {
    evasion_=0;Emit(FeedbackKind::Miss,player_);
    EmitVisual(CombatVisualKind::BarrierHit,player_,height_+1.2f,incomingFacing,0,visualElapsed);
    message_="Evaded the incoming attack.";return;
  }
  const float incoming=std::max(1.f,damage-DamageReduction());
  const float absorbed=std::min(barrier_,incoming);barrier_-=absorbed;
  if(absorbed>0 && class_==ClassId::Warrior) classResource_=std::min(CombatStyleInfo().capacity,classResource_+1);
  const float applied=std::min(sparring_?std::max(0.f,health_-1):health_,incoming-absorbed);
  health_-=applied;
  if(applied>0) hurtFlash_=std::max(0.f,.25f-visualElapsed);
  damagedThisUpdate_=damagedThisUpdate_ || applied>0;
  if (sparring_) sparringTaken_+=int(applied);
  if(applied>0) Emit(FeedbackKind::Hurt,player_,static_cast<int>(applied));
  if(absorbed>0) Emit(FeedbackKind::Absorb,player_,static_cast<int>(absorbed));
  if(absorbed>0) EmitVisual(CombatVisualKind::BarrierHit,player_,height_+1.2f,incomingFacing,0,visualElapsed);
  if (applied>0) EmitVisual(CombatVisualKind::Hurt,player_,height_+1.2f,incomingFacing,0,visualElapsed);
  if (Channeling()) { channelRemaining_=0; message_="Damage interrupted stabilization."; }
  if (health_==0) {
    classResource_=0;barrier_=0;evasion_=0;
    target_=-1; height_=0; velocityY_=0; attackFlash_=0; StopMovement();
    // Defeat suspends combat. Cancel every pending warning, including actors
    // already processed this update, before the remaining actors can advance.
    for (auto& enemy : enemies_) {
      enemy.windup=enemy.movementSpeed=enemy.movementDistance=0;
    }
  }
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
  if (attackTarget_==trainer_.id) CancelAttack();
  sparring_=false; trainer_.windup=0; trainer_.path.clear(); target_=-1;
  sparringResult_=result; sparringReportTime_=8;
  message_=std::string("Sparring: ")+result+". Dealt "+std::to_string(sparringDealt_)+", taken "+std::to_string(sparringTaken_)+". No rewards or resource refunds.";
}

void Simulation::Respawn() {
  classResource_=0;barrier_=0;evasion_=0;
  player_=WorldMap::spawn; height_=velocityY_=facing_=0; health_=MaxHealth();
  StopMovement();
  target_=-1; attackFlash_=hurtFlash_=channelRemaining_=0;
  cooldowns_.fill(0); potionCooldown_=0; feedback_.clear();
  for (auto& enemy : enemies_) {
    if (enemy.health<=0) continue;
    enemy.position=enemy.home; enemy.health=enemy.MaxHealth(); enemy.state=EnemyState::Idle;
    enemy.windup=enemy.attackCooldown=enemy.hitFlash=enemy.repathTimer=0;
    enemy.snared=enemy.rooted=enemy.stunned=enemy.exposed=0;
    enemy.movementSpeed=enemy.movementDistance=0;
    enemy.path.clear(); enemy.waypoint=0;
  }
  message_="Returned to Dawnwatch. Quests, XP, equipment, gold, and loot retained.";
}

std::string Simulation::Objective() const {
  if(anchorQuest_==AnchorQuest::Complete) {
    if(frontierCompleted_==frontierQuestCount) return "THE FRONTIER RECONNECTED\nTwelve assignments complete / level cap 10";
    const auto& quest=DescribeQuest(QuestId(int(QuestId::Watch1)+frontierCompleted_));
    const int bearings=int(bool(surveyMask_&1))+int(bool(surveyMask_&2))+int(bool(surveyMask_&4));
    return std::string(quest.title)+"\n"+(!frontierActive_?std::string("Speak to ")+quest.speaker:FrontierReady()?std::string("Report to ")+quest.speaker:
      SurveyActive()?"Bearings recorded "+std::to_string(bearings)+" / 3":"Enemies defeated "+std::to_string(DefeatedCount())+" / 3");
  }
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
  if(SurveyActive() && surveyMask_!=7) {
    Vec2 nearest=WorldMap::surveySites[0];float best=1000;
    for(unsigned i=0;i<WorldMap::surveySites.size();++i) if(!(surveyMask_&(1u<<i))) {
      const auto p=WorldMap::surveySites[i];const float d=Distance(player_,p);
      if(d<best) {nearest=p;best=d;}
    }
    return nearest;
  }
  if ((frontierActive_ && !SurveyActive() && DefeatedCount()<3) || quest_==Quest::Active || (anchorQuest_==AnchorQuest::Active && DefeatedCount()<3)) {
    Vec2 nearest=WorldMap::astralAnchor; float best=1000;
    for (const auto& enemy : enemies_) if (enemy.health>0 && Distance(player_,enemy.position)<best) {
      nearest=enemy.position; best=Distance(player_,nearest);
    }
    return nearest;
  }
  return frontierCompleted_==frontierQuestCount?WorldMap::trainingPost:anchorQuest_==AnchorQuest::Active?WorldMap::astralAnchor:QuestLocation(CurrentQuest());
}

std::string Simulation::InteractionPrompt() const {
  if (health_<=0) return "ENTER RETURN TO DAWNWATCH";
  if (sparring_) return "E END SPARRING ROUND";
  if (Channeling()) return "STABILIZING - STAND STILL";
  if(SurveyActive()) for(unsigned i=0;i<WorldMap::surveySites.size();++i)
    if(CanReadSurvey(WorldMap::surveySites[i]))
      return surveyMask_&(1u<<i)?"SURVEY BEARING ALREADY RECORDED":"E RECORD SURVEY BEARING";
  for(const auto& npc:WorldMap::residents)
    if(Distance(player_,npc.position)<=3 && HasLineOfSight(player_,npc.position)) return std::string("E SPEAK TO ")+npc.name;
  if (Distance(player_,questGiver)<=3) {
    if (quest_==Quest::Available) return "E READ FRONTIER PATROL";
    if (quest_==Quest::Return || anchorQuest_==AnchorQuest::Return) return "E DISCUSS QUEST REWARD";
    if (anchorQuest_==AnchorQuest::Available) return "E READ ASTRAL ANCHOR QUEST";
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
