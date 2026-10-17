#pragma once
#include <array>
#include <string>
#include <vector>
#include "game/WorldMap.h"
#include "game/Inventory.h"
#include "game/Trade.h"

namespace rpg {
enum class EnemyState { Idle, Pursuing, Returning, Dead };
enum class EnemyKind { Raider, Captain,Trainer };
struct Enemy {
  Vec2 position, home;
  float health = 80, attackCooldown = 0, hitFlash = 0;
  EntityId id = 0;
  EnemyState state = EnemyState::Idle;
  bool looted = false;
  float repathTimer = 0;
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
  bool usePotion = false, equipWeapon = false;
  int potionSource=-1;
  bool respawn = false;
  InventoryCommand inventory;
  int purchase=-1;
  TradeCommand trade;
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
  float Facing() const { return facing_; }
  float Health() const { return health_; }
  int Experience() const;
  int Level() const;
  int LevelStartExperience() const { return Level()==1?0:Level()==2?100:250; }
  int NextLevelExperience() const { return Level()==1?100:250; }
  float MaxHealth() const { return 100.f+20.f*(Level()-1); }
  int DefeatedCount() const;
  bool HasWardArmor() const { return anchorQuest_==AnchorQuest::Complete; }
  bool WardArmorEquipped() const { return inventory_.Worn(GearSlot::Chest)==Item::Ward; }
  const Inventory& Items() const { return inventory_; }
  float DamageReduction() const;
  const std::vector<CombatFeedback>& Feedback() const { return feedback_; }
  float HurtFlash() const { return hurtFlash_; }
  float AttackFlash() const { return attackFlash_; }
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
  float StrikeDamage() const { return DescribeItem(inventory_.Worn(GearSlot::Weapon)).strike+3.f*(Level()-1); }
  float HeavyDamage() const { return DescribeItem(inventory_.Worn(GearSlot::Weapon)).heavy+5.f*(Level()-1); }
  float PotionCooldown() const { return potionCooldown_; }
  const char* AbilityStatus(int index) const;
  Quest QuestState() const { return quest_; }
  AnchorQuest AnchorState() const { return anchorQuest_; }
  float ChannelProgress() const { return channelRemaining_>0?1-channelRemaining_/channelDuration:0; }
  bool Channeling() const { return channelRemaining_>0; }
  std::string Objective() const;
  std::string InteractionPrompt() const;
  Vec2 ObjectivePosition() const;
  float Cooldown(int index) const { return cooldowns_[index]; }
  const std::string& Message() const { return message_; }
  std::string Save() const;
  bool Load(const std::string& data);
  void Notify(const std::string& message) { message_ = message; }
  static constexpr Vec2 questGiver = WorldMap::questGiver;
  static constexpr std::array<float,4> abilityCooldowns{.9f,4.f,5.f,10.f};
  static constexpr float channelDuration = 3.f;
private:
  void Move(Vec2& position, Vec2 displacement, float radius);
  void SelectTarget();
  void UseAbility(int index);
  void Interact();
  bool HasLineOfSight(Vec2 from, Vec2 to) const;
  void UpdateEnemy(Enemy& enemy, float dt);
  void SeparateEnemies();
  void Emit(FeedbackKind kind,Vec2 position,int amount = 0);
  void RestoreForLevel(int previousLevel);
  void Respawn();
  void DamagePlayer(float damage);
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
  std::array<float,4> cooldowns_{};
  int target_ = -1, gold_ = 0;
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
  Quest quest_ = Quest::Available;
  AnchorQuest anchorQuest_ = AnchorQuest::Locked;
  float channelRemaining_ = 0, hurtFlash_ = 0;
  std::vector<CombatFeedback> feedback_;
  std::string message_ = "Approach the gold beacon and press E to accept the patrol.";
};
}
