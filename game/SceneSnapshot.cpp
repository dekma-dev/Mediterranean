#include "game/SceneSnapshot.h"
#include "game/AttackWarning.h"
#include "game/WorldInteraction.h"

namespace presentation {
ActorFrame::ActorFrame(const rpg::Enemy& e) noexcept:
  position(e.position),home(e.home),id(e.id),kind(e.kind),state(e.state),rank(e.rank),
  health(e.health),hitFlash(e.hitFlash),movementDistance(e.movementDistance),facing(e.facing),
  windup(e.windup),stunned(e.stunned),rooted(e.rooted),snared(e.snared),exposed(e.exposed),contactHeight(e.ContactHeight()),
  bossAttack(e.bossAttack),looted(e.looted),
  windupDuration_(e.WindupDuration()),beast_(e.Beast()),boss_(e.Boss()) {}

WarningBatch CaptureWarnings(const rpg::Simulation& s) {
  const auto selected=rpg::ReadAttackWarnings<WarningBatch::capacity>(s);
  WarningBatch result;result.count=selected.count;
  for(unsigned i=0;i<result.count;++i) {
    const auto& source=selected.rows[i];const auto& e=*source.enemy;
    const bool quake=e.Boss() && e.bossAttack==0;
    result.rows[i]={ActorFrame(e),e.WarningReach(),e.HalfAngle(),
      quake?rpg::Enemy::shockInner:0,quake?rpg::Enemy::shockOuter:0,e.Interruptible(),source.threatens};
  }
  return result;
}

const ActorFrame* SceneSnapshot::FindActor(rpg::EntityId id) const {
  if(!id)return nullptr;
  if(sparring_ && trainer_.id==id)return &trainer_;
  for(const auto& e:dummies_)if(e.id==id && ShowsDummy(e))return &e;
  for(const auto& e:enemies_)if(e.id==id)return &e;
  return nullptr;
}

PlayerFrame CapturePlayer(const rpg::Simulation& s) {
  PlayerFrame p;
  p.position=s.Player();p.velocity=s.MovementVelocity();p.height=s.Height();p.worldHeight=s.WorldHeight();
  p.verticalSpeed=s.VerticalSpeed();p.facing=s.Facing();p.health=s.Health();
  p.movementDistance=s.MovementDistance();p.groundMovementDistance=s.GroundMovementDistance();
  p.landingSpeed=s.LandingSpeed();p.landingElapsed=s.LandingElapsed();p.dashPhase=s.DashPhase();
  p.attackFlash=s.AttackFlash();p.hurtFlash=s.HurtFlash();p.attackSeconds=s.AttackSeconds();
  p.attackRate=s.AttackRate();p.recoveryProgress=s.RecoveryProgress();p.attackAbility=s.AttackAbility();
  p.classId=s.Class();p.grounded=s.Grounded();p.dashing=s.Dashing();p.dashed=s.DashedThisUpdate();
  p.attacking=s.Attacking();p.heavy=s.HeavyAttack();p.technique=s.TechniqueAttack();
  p.signature=s.SignatureAttack();p.opening=s.OpeningAttack();p.recovery=s.CastingRecovery();
  p.retreat=s.RetreatMobility();p.outfit=CharacterOutfit::From(s.Items(),s.Class(),s.Look());
  p.barrier=s.Barrier();p.evasionSeconds=s.EvasionSeconds();p.frostSeconds=s.FrostFieldSeconds();p.frostPosition=s.FrostFieldPosition();
  p.disciplineSeconds=s.DisciplineSeconds();p.guardWindow=s.GuardWindow();p.elementMask=s.ElementMask();p.confluenceCast=s.ConfluenceCast();
  return p;
}

void SceneSnapshot::Capture(const rpg::Simulation& s,std::uint64_t sequence) {
  const auto enemies=s.Combatants();
  // Reuse storage on ordinary ticks. Roster growth pays for compact values only,
  // never for the enemies' paths, perception history or the immutable world.
  if(enemies.size()>enemies_.capacity()){enemies_.reserve(enemies.size());++storageGrowths_;}
  if(s.Feedback().size()>feedback_.capacity()){feedback_.reserve(s.Feedback().size());++storageGrowths_;}
  enemies_.resize(enemies.size());
  for(std::size_t i=0;i<enemies.size();++i)enemies_[i]=ActorFrame(enemies[i]);
  trainer_=ActorFrame(s.SparringOpponent());
  for(std::size_t i=0;i<dummies_.size();++i)dummies_[i]=ActorFrame(s.Dummies()[i]);
  if(const auto* target=s.TargetActor())target_=ActorFrame(*target);else target_.reset();
  warnings_=CaptureWarnings(s);
  player_=CapturePlayer(s);
  world_.courtActive=s.CourtActive();world_.courtSeals=s.CourtSeals();
  world_.captain={};world_.captainProtected=false;
  for(const auto& enemy:s.Enemies())if(enemy.id==105) {
    world_.captain=ActorFrame(enemy);world_.captainProtected=s.CourtProtected(enemy);break;
  }
  for(int i=0;i<3;++i)world_.suppliesRecovered[i]=ui::SupplyRecovered(s,i);
  world_.supplySite=s.SupplyChannelSite();world_.supplyProgress=s.SupplyChannelProgress();
  world_.ritualActive=s.RitualActive();world_.ritualStage=s.RitualStage();world_.ritualCharge=s.RitualCharge();
  world_.anchorStable=s.AnchorState()>=rpg::AnchorQuest::Return;world_.channeling=s.Channeling();world_.channelProgress=s.ChannelProgress();
  feedback_.assign(s.Feedback().begin(),s.Feedback().end());
  dialogue_=s.QuestDialogue();dialogueStage_=s.DialogueStage();dialoguePosition_=s.DialoguePosition();
  sparring_=s.Sparring();replay_=s.ReplayActive();friendlyTarget_=s.FriendlyTarget();
  sequence_=sequence;++revision_;
}
}
