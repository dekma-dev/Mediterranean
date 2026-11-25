#pragma once
#include "game/Simulation.h"
#include "game/CharacterView.h"
#include <optional>
#include <type_traits>

namespace presentation {
// Owned visual state, deliberately excluding navigation, AI history and save data.
// Derived combat properties are evaluated by Simulation's Enemy, never duplicated.
struct ActorFrame {
  rpg::Vec2 position{},home{};
  rpg::EntityId id=0;
  rpg::EnemyKind kind=rpg::EnemyKind::Raider;
  rpg::EnemyState state=rpg::EnemyState::Idle;
  int rank=0;
  float health=0,hitFlash=0,movementDistance=0,facing=0,windup=0,stunned=0;
  float rooted=0,snared=0,exposed=0,contactHeight=0;
  unsigned bossAttack=0;
  bool looted=false;
  ActorFrame()=default;
  ActorFrame(const rpg::Enemy& enemy) noexcept;
  bool Beast() const {return beast_;}
  bool Boss() const {return boss_;}
  float WindupDuration() const {return windupDuration_;}
private:
  float windupDuration_=0;
  bool beast_=false,boss_=false;
};
static_assert(std::is_trivially_copyable_v<ActorFrame>);
static_assert(sizeof(ActorFrame)<=96,"Actor snapshots must not acquire AI/path storage");

struct WarningFrame {
  ActorFrame actor;
  float reach=0,halfAngle=0,inner=0,outer=0;
  bool interruptible=false,threatens=false;
};
struct WarningBatch {
  static constexpr std::size_t capacity=8;
  std::array<WarningFrame,capacity> rows{};
  unsigned count=0;
};
WarningBatch CaptureWarnings(const rpg::Simulation&);

struct PlayerFrame {
  rpg::Vec2 position{},velocity{};
  float height=0,worldHeight=0,verticalSpeed=0,facing=0,health=0;
  float movementDistance=0,groundMovementDistance=0,landingSpeed=0,landingElapsed=0;
  float dashPhase=0,attackFlash=0,hurtFlash=0,attackSeconds=-1,attackRate=1,recoveryProgress=0;
  int attackAbility=-1;
  rpg::ClassId classId=rpg::ClassId::Warrior;
  bool grounded=true,dashing=false,dashed=false,jumped=false,attacking=false,heavy=false;
  bool technique=false,signature=false,opening=false,recovery=false,retreat=false;
  float barrier=0,evasionSeconds=0,frostSeconds=0,disciplineSeconds=0,guardWindow=0;
  rpg::Vec2 frostPosition{};
  unsigned elementMask=0;
  bool confluenceCast=false;
  CharacterOutfit outfit{};
};
PlayerFrame CapturePlayer(const rpg::Simulation&);

// Only evaluated presentation inputs. Quest state machines and reward ledgers
// stay in Simulation; renderers cannot infer or commit objective completion.
struct WorldFrame {
  bool courtActive=false,captainProtected=false,ritualActive=false,anchorStable=false,channeling=false;
  unsigned courtSeals=0;
  ActorFrame captain;
  std::array<bool,3> suppliesRecovered{};
  int supplySite=-1,ritualStage=0;
  float supplyProgress=0,ritualCharge=0,channelProgress=0;
};

// One publication per admitted step; refreshes also cover out-of-step lifecycle
// changes. A consumer owns a copy if it needs to keep a frame across publication.
// This is not a concurrent mailbox and does not enable a separate render thread.
class SceneSnapshot {
public:
  void Capture(const rpg::Simulation&,std::uint64_t sequence);
  const PlayerFrame& Player() const {return player_;}
  const std::vector<ActorFrame>& Combatants() const {return enemies_;}
  const ActorFrame& Trainer() const {return trainer_;}
  const std::array<ActorFrame,3>& Dummies() const {return dummies_;}
  bool ShowsDummy(const ActorFrame& dummy) const {return !(sparring_ && dummy.id==210);}
  const ActorFrame* Target() const {return target_?&*target_:nullptr;}
  const WarningBatch& Warnings() const {return warnings_;}
  const WorldFrame& World() const {return world_;}
  const auto& Feedback() const {return feedback_;}
  const ActorFrame* FindActor(rpg::EntityId) const;
  rpg::QuestId Dialogue() const {return dialogue_;}
  rpg::QuestStage DialogueStage() const {return dialogueStage_;}
  rpg::Vec2 DialoguePosition() const {return dialoguePosition_;}
  bool Sparring() const {return sparring_;}
  bool Replay() const {return replay_;}
  int FriendlyTarget() const {return friendlyTarget_;}
  std::uint64_t Sequence() const {return sequence_;}
  std::uint64_t Revision() const {return revision_;}
  std::uint64_t StorageGrowths() const {return storageGrowths_;}
  std::size_t Bytes() const {return sizeof(*this)+enemies_.capacity()*sizeof(ActorFrame)+feedback_.capacity()*sizeof(rpg::CombatFeedback);}
private:
  PlayerFrame player_;
  std::vector<ActorFrame> enemies_;
  ActorFrame trainer_;
  std::array<ActorFrame,3> dummies_{};
  std::optional<ActorFrame> target_;
  WarningBatch warnings_;
  WorldFrame world_;
  std::vector<rpg::CombatFeedback> feedback_;
  rpg::QuestId dialogue_=rpg::QuestId::None;
  rpg::QuestStage dialogueStage_{};
  rpg::Vec2 dialoguePosition_{};
  bool sparring_=false,replay_=false;
  int friendlyTarget_=-1;
  std::uint64_t sequence_=0,revision_=0,storageGrowths_=0;
};
}
