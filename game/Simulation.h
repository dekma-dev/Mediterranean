#pragma once
#include <array>
#include <string>
#include <vector>
#include "game/WorldMap.h"
#include "game/Inventory.h"
#include "game/Trade.h"
#include "game/Talents.h"
#include "game/Spells.h"
#include "game/Quests.h"
#include "game/Motion.h"
#include "game/AttackMotion.h"

namespace rpg {
enum class EnemyState { Idle, Pursuing, Returning, Dead };
enum class EnemyKind { Raider, Captain,Trainer };
struct Enemy {
  Vec2 position, home;
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
  float MaxHealth() const { return kind==EnemyKind::Trainer?180.f:kind==EnemyKind::Captain?120.f:80.f; }
  float WindupDuration() const { return kind==EnemyKind::Trainer?.9f:kind==EnemyKind::Captain?1.f:.7f; }
  float AttackInterval() const { return kind==EnemyKind::Captain?2.4f:1.4f; }
  float Damage() const { return kind==EnemyKind::Captain?16.f:8.f; }
  const char* Name() const { return kind==EnemyKind::Trainer?"SPARRING GUARD":kind==EnemyKind::Captain?"RAIDER CAPTAIN":"RAIDER"; }
  static constexpr float attackReach = 2.4f, attackHalfAngle = 1.f;
};
struct Command {
  Vec2 movement;
  bool sprint = false, jump = false, nextTarget = false, interact = false, restart = false;
  int ability = -1;
  int learnTalent = -1;
  bool resetTalents = false;
  BuildCommand build;
  QuestAction questAction=QuestAction::None;
  bool usePotion = false, equipWeapon = false;
  int potionSource=-1;
  bool respawn = false;
  bool haltMovement = false; // Explicit UI control capture, not ordinary key release.
  InventoryCommand inventory;
  int purchase=-1;
  TradeCommand trade;
  bool steerFacing=false;
  float facingYaw=0;
};
enum class Quest { Available, Active, Return, Complete };
enum class AnchorQuest { Locked, Available, Active, Return, Complete };
enum class FeedbackKind { Damage, Hurt, Heal, Experience, LevelUp, Miss, Interrupt };
struct CombatFeedback {
  Vec2 position;
  FeedbackKind kind;
  int amount = 0;
  float age = 0;
};

// One-update presentation events. Never serialized or used to resolve combat.
enum class CombatVisualKind { Strike, HeavyStrike, EnemySwing, Hit, HeavyHit, Hurt, Interrupt, Mend, Potion, DashStart, DashStop, DashImpact };
struct CombatVisualEvent {
  CombatVisualKind kind = CombatVisualKind::Hit;
  Vec2 position;
  float height = 0, facing = 0;
  EntityId actor = 0; // zero is the player; enemies retain their stable IDs
  float elapsed = 0; // time from emission to the end of this update
};

class Simulation {
public:
  Simulation();
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
  float DashPhase() const { return Dashing()?1-dashRemaining_/motion::dashDuration:0; }
  float Facing() const { return facing_; }
  float MovementDistance() const { return movementDistance_; } // excludes dash/respawn
  Vec2 MovementVelocity() const { return movementVelocity_; }
  float GroundMovementDistance() const { return groundMovementDistance_; }
  void StopMovement() { movementVelocity_={}; movementDistance_=groundMovementDistance_=0; jumpBuffer_=0; dashRemaining_=dashTravelLeft_=0; dashTarget_=0; CancelAttack(); }
  bool Attacking() const { return attackAbility_>=0; }
  bool HeavyAttack() const { return attackAbility_==1; }
  float AttackSeconds() const { return Attacking()?attackAge_:-1.f; }
  float AttackProgress() const { return Attacking()?attackAge_/combat::Duration(HeavyAttack()):0; }
  float Health() const { return health_; }
  int Experience() const;
  int Level() const;
  int LevelStartExperience() const { return Level()==1?0:Level()==2?100:250; }
  int NextLevelExperience() const { return Level()==1?100:250; }
  float MaxHealth() const { return 100.f+20.f*(Level()-1)+TalentBonus(build_,&TalentInfo::health); }
  bool HasTalent(Talent talent) const { return rpg::HasTalent(build_,talent); }
  int TalentPoints() const;
  const char* TalentStatus(Talent talent) const;
  const char* BuildChangeStatus() const;
  const char* TalentResetStatus() const;
  unsigned BuildMask() const { return build_; }
  int Loadout(int slot) const { return slot>=0 && slot<int(loadouts_.size())?loadouts_[slot]:-1; }
  BuildStats PreviewBuild(unsigned mask) const;
  const char* LoadoutStatus(const BuildCommand& command) const;
  float StrikeBonus() const { return 3.f*(Level()-1)+TalentBonus(build_,&TalentInfo::strike); }
  float HeavyBonus() const { return 5.f*(Level()-1)+TalentBonus(build_,&TalentInfo::heavy); }
  float MendHealing() const { return 35.f+TalentBonus(build_,&TalentInfo::mend); }
  float MendHealingNow() const { return MendHealing()+(HasTalent(Talent::Mercy) && health_<=MaxHealth()*.35f?20.f:0.f); }
  float HeavyMultiplier(float health,float maximum) const {
    return HasTalent(Talent::Executioner) && maximum>0 && health>0 && health<=maximum*.5f?1.25f:1.f;
  }
  float HeavyDamageAgainst(float health,float maximum) const { return HeavyDamage()*HeavyMultiplier(health,maximum); }
  float AbilityCooldown(int index) const { return ValidSpell(index)?abilityCooldowns[index]-
    (index==3?TalentBonus(build_,&TalentInfo::mendCooldown):index==2?TalentBonus(build_,&TalentInfo::dashCooldown):0):0; }
  int DefeatedCount() const;
  bool HasWardArmor() const { return anchorQuest_==AnchorQuest::Complete; }
  bool WardArmorEquipped() const { return inventory_.Worn(GearSlot::Chest)==Item::Ward; }
  const Inventory& Items() const { return inventory_; }
  float DamageReduction() const;
  const std::vector<CombatFeedback>& Feedback() const { return feedback_; }
  float HurtFlash() const { return hurtFlash_; }
  bool DamagedThisUpdate() const { return damagedThisUpdate_; }
  float AttackFlash() const { return attackFlash_; }
  const auto& VisualEvents() const { return visualEvents_; }
  std::size_t VisualEventCount() const { return visualEventCount_; }
  int Target() const { return target_; }
  const Enemy* TargetActor() const;
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
  float StrikeDamage() const { return DescribeItem(inventory_.Worn(GearSlot::Weapon)).strike+StrikeBonus(); }
  float HeavyDamage() const { return DescribeItem(inventory_.Worn(GearSlot::Weapon)).heavy+HeavyBonus(); }
  float PotionCooldown() const { return potionCooldown_; }
  const char* AbilityStatus(int index) const;
  QuestId QuestDialogue() const { return questDialogue_; }
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
  void Move(Vec2& position, Vec2 displacement, float radius);
  void SelectTarget();
  enum class DashCheck { Ready, NoTarget, Returning, TooClose, TooFar, Blocked };
  DashCheck CheckDashTarget() const;
  const Enemy* DashTargetActor() const;
  void UseAbility(int index,float visualElapsed=0);
  void ResolveAttack(float elapsed);
  void UpdateCombat(float dt,bool advanceAttack);
  void CancelAttack() { attackAbility_=-1; attackAge_=attackDamage_=attackFlash_=0; attackTarget_=0; }
  void LearnTalent(int index);
  void ResetTalents();
  void ChangeBuild(const BuildCommand& command);
  void Interact();
  bool CanTalkToKeeper() const;
  void ConfirmQuest(QuestAction action);
  bool HasLineOfSight(Vec2 from, Vec2 to) const;
  void UpdateEnemy(Enemy& enemy, float dt,float frameTail);
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
  EntityId dashTarget_=0; // accepted target identity; selection may change during travel
  int attackAbility_=-1;
  EntityId attackTarget_=0;
  float attackAge_=0,attackDamage_=0;
  float movementDistance_ = 0;
  float groundMovementDistance_ = 0;
  float landingSpeed_ = 0, landingElapsed_ = 0;
  bool damagedThisUpdate_ = false;
  bool jumpedThisUpdate_ = false, dashedThisUpdate_ = false;
  std::array<float,4> cooldowns_{};
  int target_ = -1, gold_ = 0;
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
