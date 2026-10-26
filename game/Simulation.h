#pragma once
#include <array>
#include <string>
#include <vector>
#include "game/WorldMap.h"
#include "game/Inventory.h"
#include "game/Trade.h"
#include "game/Talents.h"
#include "game/Classes.h"
#include "game/Appearance.h"
#include "game/ClassEquipment.h"
#include "game/Quests.h"
#include "game/Motion.h"
#include "game/AttackMotion.h"

namespace rpg {
enum class EnemyState { Idle, Pursuing, Returning, Dead };
enum class EnemyKind { Raider, Captain,Trainer,Reaver,Wraith };
struct Enemy {
  Vec2 position, home;
  int rank=0;
  float health = 80, attackCooldown = 0, hitFlash = 0;
  bool damagedThisUpdate = false; // transient event, independent of the fading tint
  EntityId id = 0;
  EnemyState state = EnemyState::Idle;
  bool looted = false;
  float repathTimer = 0;
  // Successful navigation travel in the latest update; transient, never saved.
  float movementDistance = 0;
  float movementSpeed = 0; // transient acceleration state
  std::vector<Vec2> path;
  std::size_t waypoint = 0;
  EnemyKind kind = EnemyKind::Raider;
  float windup = 0, facing = 0;
  unsigned bossAttack=0,nextBossAttack=0; // committed warning and next rotation slot
  float snared=0,rooted=0,stunned=0,exposed=0; // transient combat effects
  bool Boss() const { return kind==EnemyKind::Captain && rank>0; }
  bool Enraged() const { return Boss() && health>0 && health<=MaxHealth()*.4f; }
  float MaxHealth() const { return Boss()?240.f+rank*32.f:(kind==EnemyKind::Trainer?180.f:kind==EnemyKind::Captain?120.f:kind==EnemyKind::Reaver?105.f:80.f)+rank*18.f; }
  float WindupDuration() const { return Boss()?1.5f:kind==EnemyKind::Trainer?.9f:kind==EnemyKind::Captain?1.f:kind==EnemyKind::Reaver?1.15f:kind==EnemyKind::Wraith?1.1f:.7f; }
  float AttackInterval() const { return kind==EnemyKind::Captain?2.4f:kind==EnemyKind::Reaver?2.7f:kind==EnemyKind::Wraith?1.2f:1.4f; }
  float Damage() const { return (Boss()?28.f:kind==EnemyKind::Captain?16.f:kind==EnemyKind::Reaver?20.f:kind==EnemyKind::Wraith?15.f:8.f)+rank*2.f+(Enraged()?10.f:0.f); }
  bool Interruptible() const { return !Boss() || bossAttack!=1; }
  float CastRange() const { return Boss()?6.5f:kind==EnemyKind::Wraith?6.5f:kind==EnemyKind::Reaver?2.7f:2.f; }
  float Reach() const { return Boss() && bossAttack==1?3.8f:Boss() && bossAttack==2?8.f:kind==EnemyKind::Wraith?7.f:kind==EnemyKind::Reaver?3.2f:attackReach; }
  float HalfAngle() const { return Boss() && bossAttack==1?3.14159265f:Boss() && bossAttack==2?.22f:kind==EnemyKind::Wraith?.16f:kind==EnemyKind::Reaver?1.65f:attackHalfAngle; }
  static constexpr float shockInner=2.8f,shockOuter=5.8f;
  bool Hits(float distance,float dot) const {
    return (distance<=Reach() && dot>=std::cos(HalfAngle())) ||
      (Boss() && bossAttack==0 && distance>=shockInner && distance<=shockOuter);
  }
  const char* AttackName() const { return Boss()?(bossAttack==1?"Iron tempest":bossAttack==2?"Execution line":"Doomquake"):kind==EnemyKind::Wraith?"Veil lance":kind==EnemyKind::Reaver?"Reaping cleave":"Incoming strike"; }
  const char* Counterplay() const { return Boss()?(bossAttack==1?"UNINTERRUPTIBLE - MOVE OUT":bossAttack==2?"SIDESTEP / INTERRUPT":"INNER FLANK / OR OUTSIDE RING"):kind==EnemyKind::Wraith?"SIDESTEP / INTERRUPT":kind==EnemyKind::Reaver?"GET BEHIND / INTERRUPT":"DODGE / INTERRUPT"; }
  const char* Name() const { if(Boss()) return rank>=8?"BOSS: ASHEN OVERLORD":"BOSS: OATHBREAKER";if(kind==EnemyKind::Reaver) return "ASH REAVER";if(kind==EnemyKind::Wraith) return "VEIL HUNTER";return kind==EnemyKind::Trainer?"SPARRING GUARD":kind==EnemyKind::Captain?"RAIDER CAPTAIN":"RAIDER"; }
  static constexpr float attackReach = 2.4f, attackHalfAngle = 1.f;
};
struct Command {
  Vec2 movement;
  bool sprint = false, jump = false, nextTarget = false, interact = false, restart = false;
  EntityId selectThreat=0; // Explicit caster identity from the incoming-attack HUD.
  int ability = -1;
  bool queueAttack = false; // Physical/UI input may buffer one near-ready attack.
  int learnTalent = -1;
  bool resetTalents = false;
  BuildCommand build;
  QuestAction questAction=QuestAction::None;
  bool usePotion = false, equipWeapon = false;
  int potionSource=-1;
  bool respawn = false;
  bool uiClick=false;
  bool haltMovement = false; // Explicit UI control capture, not ordinary key release.
  InventoryCommand inventory;
  int purchase=-1;
  TradeCommand trade;
  bool steerFacing=false;
  float facingYaw=0;
};
enum class Quest { Available, Active, Return, Complete };
enum class AnchorQuest { Locked, Available, Active, Return, Complete };
enum class FeedbackKind { Damage, Hurt, Heal, Experience, LevelUp, Miss, Interrupt, Absorb };
struct CombatFeedback {
  Vec2 position;
  FeedbackKind kind;
  int amount = 0;
  float age = 0;
};

// One-update presentation events. Never serialized or used to resolve combat.
enum class CombatVisualKind { Strike, HeavyStrike, EnemySwing, Hit, HeavyHit, Hurt, Interrupt, Mend, Potion, DashStart, DashStop, DashImpact,SpellCast,BarrierHit,CastStart,UiClick,ReserveReady,CastCancelled,EnemyLance,EnemyQuake,EnemyTempest,EnemyExecution,EnemyPrepare,BossPrepare };
struct CombatVisualEvent {
  CombatVisualKind kind = CombatVisualKind::Hit;
  Vec2 position;
  float height = 0, facing = 0;
  EntityId actor = 0; // zero is the player; enemies retain their stable IDs
  float elapsed = 0; // time from emission to the end of this update
  Vec2 destination{};
  ClassId school=ClassId::Warrior;
  float flight=.1f;
  // Snapshot the accepted action: presentation can outlive recovery/selection.
  Vec2 source{};
  float sourceHeight=0;
  bool heavy=false,empowered=false,technique=false,signature=false,opening=false;
};

class Simulation {
public:
  Simulation();
  bool CreateCharacter(ClassId id,const std::string& nickname,Appearance look={});
  const Appearance& Look() const { return appearance_; }
  ClassId Class() const { return class_; }
  const ClassInfo& ClassInfo() const { return DescribeClass(class_); }
  TalentInfo TalentDefinition(int index) const { return ClassTalent(class_,index); }
  SpellInfo Spell(int index) const { auto spell=ClassSpell(class_,index);
    if(index==3) {spell.contact=RecoveryCastDuration();spell.recovery=spell.contact+.3f;} return spell; }
  const CombatStyle& CombatStyleInfo() const { return ClassCombat(class_); }
  int ClassResource() const { return classResource_; }
  bool EmpoweredReady() const { return classResource_>=CombatStyleInfo().capacity; }
  bool EmpoweredAttack() const { return Attacking() && attackEmpowered_; }
  float EvasionSeconds() const { return evasion_; }
  float Barrier() const { return barrier_; }
  bool RetreatMobility() const { return CombatStyleInfo().retreatDistance>0; }
  float MobilityDuration() const { return CombatStyleInfo().retreatDuration; }
  float AttackRate() const { return ClassAttackRate(class_,HeavyAttack())*(attackAbility_==7?.85f:attackAbility_==6?.7f:attackAbility_==5?.8f:1.f); }
  float BuildBonus(unsigned mask,float TalentInfo::*stat) const;
  int FrontierCompleted() const { return frontierCompleted_; }
  bool FrontierActive() const { return frontierActive_; }
  int EarnedPoints() const { return EarnedTalentPoints(Level()); }
  void Update(float seconds, const Command& command);
  bool CanStand(Vec2 position, float radius = .4f) const;
  float CameraDistance(Vec2 origin, float originY, Vec2 direction, float slope, float desired) const;
  const WorldMap& Map() const { return map_; }
  const std::vector<Obstacle>& Obstacles() const { return map_.Obstacles(); }
  const std::vector<Enemy>& Enemies() const { return enemies_; }
  Vec2 Player() const { return player_; }
  float Height() const { return height_; }
  float VerticalSpeed() const { return velocityY_; }
  bool Grounded() const { return height_==0 && velocityY_<=0; }
  // Impact speed and time since contact within the latest update; zero otherwise.
  float LandingSpeed() const { return landingSpeed_; }
  float LandingElapsed() const { return landingElapsed_; }
  bool JumpedThisUpdate() const { return jumpedThisUpdate_; }
  bool DashedThisUpdate() const { return dashedThisUpdate_; }
  bool Dashing() const { return dashRemaining_>0; }
  float DashPhase() const { return Dashing()?1-dashRemaining_/MobilityDuration():0; }
  float Facing() const { return facing_; }
  float MovementDistance() const { return movementDistance_; } // excludes dash/respawn
  Vec2 MovementVelocity() const { return movementVelocity_; }
  float GroundMovementDistance() const { return groundMovementDistance_; }
  void StopMovement() { queuedAttack_=-1; queueRemaining_=0; recoveryCast_=0; movementVelocity_={}; movementDistance_=groundMovementDistance_=0; jumpBuffer_=0; dashRemaining_=dashTravelLeft_=0; dashTarget_=0; CancelAttack(); }
  int QueuedAttack() const { return queuedAttack_; }
  bool CanBufferAttack(int index) const {
    if(index<0 || index>1 || health_<=0 || Dashing() || CastingRecovery() || !TargetActor() || TargetActor()->health<=0) return false;
    const float remaining=Attacking()?(combat::Duration(HeavyAttack())-attackAge_)/AttackRate():0;
    const float wait=std::max(remaining,cooldowns_[index]);return wait>0 && wait<=.22f;
  }
  bool CastingRecovery() const { return recoveryCast_>0; }
  float RecoveryCastDuration() const { return ClassRecoveryCast(class_)/(1+BuildBonus(build_,&TalentInfo::castSpeed)); }
  float RecoveryProgress() const { return CastingRecovery()?1-recoveryCast_/RecoveryCastDuration():0; }
  bool Attacking() const { return attackAbility_>=0; }
  int AttackAbility() const { return attackAbility_; }
  bool OpeningAttack() const { return attackAbility_==7; }
  bool SignatureAttack() const { return attackAbility_==6; }
  bool TechniqueAttack() const { return attackAbility_==5; }
  bool HeavyAttack() const { return attackAbility_==1 || attackAbility_==5 || attackAbility_==6; }
  float AttackSeconds() const { return Attacking()?attackAge_:-1.f; }
  float AttackProgress() const { return Attacking()?attackAge_/combat::Duration(HeavyAttack()):0; }
  float Health() const { return health_; }
  int Experience() const;
  int Level() const;
  // Identity of the current single-character campaign, shared by all UI views.
  const char* Nickname() const { return nickname_.c_str(); }
  const char* ClassName() const { return class_==ClassId::Warrior?"Dawnwatch guard":ClassInfo().name; }
  int LevelStartExperience() const { return levelExperience[Level()-1]; }
  int NextLevelExperience() const { return levelExperience[std::min(Level(),levelCap-1)]; }
  float MaxHealth() const { return ClassInfo().health+20.f*(Level()-1)+BuildBonus(build_,&TalentInfo::health); }
  bool HasTalent(Talent talent) const { return rpg::HasTalent(build_,talent); }
  int TalentPoints() const;
  const char* TalentStatus(Talent talent) const;
  const char* BuildChangeStatus() const;
  const char* TalentResetStatus() const;
  unsigned BuildMask() const { return build_; }
  int Loadout(int slot) const { return slot>=0 && slot<int(loadouts_.size())?loadouts_[slot]:-1; }
  BuildStats PreviewBuild(unsigned mask) const;
  const char* LoadoutStatus(const BuildCommand& command) const;
  float StrikeBonus() const { return 3.f*(Level()-1)+BuildBonus(build_,&TalentInfo::strike); }
  float HeavyBonus() const { return 5.f*(Level()-1)+BuildBonus(build_,&TalentInfo::heavy); }
  float MendHealing() const { return ClassInfo().healing+BuildBonus(build_,&TalentInfo::mend); }
  float RecoveryWard() const { return ClassRecoveryBarrier(class_)+(ClassHealer(class_)?0:BuildBonus(build_,&TalentInfo::mend)*.5f+(HasTalent(Talent::Mercy) && health_<=MaxHealth()*.35f?10.f:0.f)+(HasTalent(Talent::Overflow)?12.f:0.f)); }
  float MendHealingNow() const { return MendHealing()+(ClassHealer(class_) && HasTalent(Talent::Mercy) && health_<=MaxHealth()*.35f?20.f:0.f); }
  float HeavyMultiplier(float health,float maximum) const {
    return HasTalent(Talent::Executioner) && maximum>0 && health>0 && health<=maximum*.5f?1.25f:1.f;
  }
  float HeavyDamageAgainst(float health,float maximum) const { return HeavyDamage()*HeavyMultiplier(health,maximum); }
  float AbilityCooldown(int index) const { return index>=5 && ValidSpell(index)?spells[index].cooldown:ValidSpell(index)?abilityCooldowns[index]-
    (index==3?BuildBonus(build_,&TalentInfo::mendCooldown):index==2?BuildBonus(build_,&TalentInfo::dashCooldown):
    index==1?BuildBonus(build_,&TalentInfo::heavyCooldown):BuildBonus(build_,&TalentInfo::basicCooldown)):0; }
  int DefeatedCount() const;
  bool HasWardArmor() const { return anchorQuest_==AnchorQuest::Complete; }
  bool WardArmorEquipped() const { return inventory_.Worn(GearSlot::Chest)==Item::Ward; }
  const Inventory& Items() const { return inventory_; }
  ItemInfo ItemDefinition(Item item) const { return DescribeClassItem(item,class_); }
  float DamageReduction() const;
  const std::vector<CombatFeedback>& Feedback() const { return feedback_; }
  float HurtFlash() const { return hurtFlash_; }
  bool DamagedThisUpdate() const { return damagedThisUpdate_; }
  float AttackFlash() const { return attackFlash_; }
  const auto& VisualEvents() const { return visualEvents_; }
  std::size_t VisualEventCount() const { return visualEventCount_; }
  int Target() const { return target_; }
  const Enemy* TargetActor() const;
  bool VisibleEnemyCast(const Enemy& enemy) const {
    return enemy.health>0 && enemy.windup>0 && enemy.state==EnemyState::Pursuing &&
      std::hypot(player_.x-enemy.position.x,player_.z-enemy.position.z)<=12.f &&
      HasLineOfSight(enemy.position,player_);
  }
  bool Sparring() const { return sparring_; }
  const Enemy& SparringOpponent() const { return trainer_; }
  float SparringSeconds() const { return sparringRemaining_; }
  int SparringDamageDealt() const { return sparringDealt_; }
  int SparringDamageTaken() const { return sparringTaken_; }
  float SparringReportTime() const { return sparringReportTime_; }
  const std::string& SparringResult() const { return sparringResult_; }
  int Gold() const { return gold_; }
  bool NearQuartermaster() const;
  bool CanTrade() const;
  bool TradeRequested() const { return tradeRequested_; }
  int ShopStock(int offer) const;
  const char* PurchaseStatus(int offer) const;
  int BuybackCount(Item item) const { return ValidItem(item)?buyback_[int(item)]:0; }
  const char* TradeStatus(const TradeCommand& command) const;
  int Potions() const { return inventory_.Count(Item::Potion); }
  bool HasRewardWeapon() const { return inventory_.Count(Item::Watchguard)>0; }
  bool RewardWeaponEquipped() const { return inventory_.Worn(GearSlot::Weapon)==Item::Watchguard; }
  float StrikeDamage() const { return DescribeItem(inventory_.Worn(GearSlot::Weapon)).strike+StrikeBonus()+ClassInfo().strike; }
  float HeavyDamage() const { return DescribeItem(inventory_.Worn(GearSlot::Weapon)).heavy+HeavyBonus()+ClassInfo().heavy; }
  float PotionCooldown() const { return potionCooldown_; }
  const char* AbilityStatus(int index) const;
  QuestId QuestDialogue() const { return questDialogue_; }
  QuestId CurrentQuest() const;
  Vec2 DialoguePosition() const { return QuestLocation(questDialogue_); }
  bool SurveyActive() const { return frontierActive_ && frontierCompleted_==8; }
  unsigned SurveyMask() const { return surveyMask_; }
  bool FrontierReady() const { return SurveyActive()?surveyMask_==7:DefeatedCount()==3; }
  QuestStage DialogueStage() const;
  QuestAction QuestPrimaryAction() const;
  const char* QuestConfirmStatus() const;
  void CloseQuestDialogue() { questDialogue_=QuestId::None; }
  Quest QuestState() const { return quest_; }
  AnchorQuest AnchorState() const { return anchorQuest_; }
  float ChannelProgress() const { return channelRemaining_>0?1-channelRemaining_/channelDuration:0; }
  bool Channeling() const { return channelRemaining_>0; }
  std::string Objective() const;
  std::string InteractionPrompt() const;
  Vec2 ObjectivePosition() const;
  float Cooldown(int index) const { return ValidSpell(index)?cooldowns_[index]:0; }
  const std::string& Message() const { return message_; }
  std::string Save() const;
  bool Load(const std::string& data);
  void Notify(const std::string& message) { message_ = message; }
  static constexpr Vec2 questGiver = WorldMap::questGiver;
  static constexpr std::array<float,4> abilityCooldowns{spells[0].cooldown,spells[1].cooldown,spells[2].cooldown,spells[3].cooldown};
  static constexpr float channelDuration = 3.f;
private:
  int frontierCompleted_=0,legacyLoot_=0;
  bool frontierActive_=false;
  unsigned surveyMask_=0;
  void PrepareFrontierEnemies(int rank);
  ClassId class_=ClassId::Warrior;
  std::string nickname_="Frontier guard";
  Appearance appearance_;
  void Move(Vec2& position, Vec2 displacement, float radius);
  void SelectTarget();
  enum class DashCheck { Ready, NoTarget, Returning, TooClose, TooFar, Blocked };
  DashCheck CheckDashTarget() const;
  const Enemy* DashTargetActor() const;
  void UseAbility(int index,float visualElapsed=0);
  void ResolveAttack(float elapsed);
  void UpdateCombat(float dt,bool advanceAttack);
  void CancelAttack() { attackAbility_=-1; attackAge_=attackDamage_=attackFlash_=0; attackTarget_=0; attackEmpowered_=false; }
  void LearnTalent(int index);
  void ResetTalents();
  void ChangeBuild(const BuildCommand& command);
  void Interact();
  bool CanTalkToKeeper() const;
  bool CanReadSurvey(Vec2 site) const;
  void ConfirmQuest(QuestAction action);
  bool HasLineOfSight(Vec2 from, Vec2 to) const;
  void UpdateEnemy(Enemy& enemy, float dt,float frameTail);
  void AdvanceEnemy(Enemy& enemy,float dt,float frameTail);
  void MoveEnemy(Enemy& enemy,Vec2 displacement);
  void SeparateEnemies(float dt);
  void Emit(FeedbackKind kind,Vec2 position,int amount = 0);
  void RestoreForLevel(int previousLevel);
  void Respawn();
  void DamagePlayer(float damage,float incomingFacing,float visualElapsed);
  void EmitVisual(CombatVisualKind kind,Vec2 position,float height,float facing,EntityId actor=0,float elapsed=0);
  void StartSparring();
  void EndSparring(const char* result);
  void Buy(int offer);
  int GoldSpent() const;
  int BuybackValue() const;
  void Trade(const TradeCommand& command);
  WorldMap map_;
  std::vector<Enemy> enemies_;
  Vec2 player_{0,-10};
  float height_ = 0, velocityY_ = 0, facing_ = 0, health_ = 100, attackFlash_ = 0;
  Vec2 movementVelocity_{};
  float airSpeedLimit_=4;
  float jumpBuffer_=0;
  float dashRemaining_=0,dashTravelLeft_=0;
  Vec2 retreatDirection_{};
  EntityId dashTarget_=0; // accepted target identity; selection may change during travel
  int attackAbility_=-1;
  EntityId attackTarget_=0;
  float attackAge_=0,attackDamage_=0;
  int classResource_=0;
  bool attackEmpowered_=false;
  float barrier_=0,recoveryCast_=0,evasion_=0;
  void CompleteRecovery();
  float movementDistance_ = 0;
  float groundMovementDistance_ = 0;
  float landingSpeed_ = 0, landingElapsed_ = 0;
  bool damagedThisUpdate_ = false;
  bool jumpedThisUpdate_ = false, dashedThisUpdate_ = false;
  std::array<float,8> cooldowns_{};
  int target_ = -1, gold_ = 0;
  int queuedAttack_=-1;
  EntityId queuedTarget_=0;
  float queueRemaining_=0;
  unsigned build_=0;
  std::array<int,3> loadouts_{{-1,-1,-1}};
  Inventory inventory_;
  std::array<int,shopOffers.size()> purchases_{};
  std::array<int,int(Item::Count)> buyback_{};
  bool tradeRequested_=false;
  Enemy trainer_;
  bool sparring_=false;
  float sparringRemaining_=0,sparringReportTime_=0;
  int sparringDealt_=0,sparringTaken_=0;
  std::string sparringResult_;
  float potionCooldown_ = 0;
  QuestId questDialogue_=QuestId::None; // reading state is transient and never saved
  Quest quest_ = Quest::Available;
  AnchorQuest anchorQuest_ = AnchorQuest::Locked;
  float channelRemaining_ = 0, hurtFlash_ = 0;
  std::vector<CombatFeedback> feedback_;
  std::array<CombatVisualEvent,16> visualEvents_{};
  std::size_t visualEventCount_ = 0;
  std::string message_ = "Approach the gold beacon and press E to accept the patrol.";
};
}
