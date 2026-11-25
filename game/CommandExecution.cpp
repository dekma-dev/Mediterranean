#include "game/CommandExecution.h"
#include <chrono>
#include <cstring>
#include <type_traits>

namespace rpg {
namespace {
using Clock=std::chrono::steady_clock;
// Same-build diagnostic fingerprint, not a portable serialization format.
// Hash fields explicitly: never object padding, pointers or container storage.
struct Fingerprint {
  std::uint64_t value=14695981039346656037ull;
  void Bytes(const void* data,std::size_t count){const auto* p=static_cast<const unsigned char*>(data);while(count--){value^=*p++;value*=1099511628211ull;}}
  template<class T> void Add(const T& v){static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>);Bytes(&v,sizeof(v));}
  void Add(Vec2 v){Add(v.x);Add(v.z);}
  void Add(const std::string& v){Add(v.size());Bytes(v.data(),v.size());}
};
std::uint64_t Observe(const Simulation& state) {
  Fingerprint f;f.Add(state.Save());f.Add(state.Message());
  f.Add(state.Player());f.Add(state.MovementVelocity());f.Add(state.Facing());f.Add(state.Height());f.Add(state.VerticalSpeed());
  f.Add(state.Health());f.Add(state.Grounded());f.Add(state.MovementDistance());f.Add(state.GroundMovementDistance());
  f.Add(state.LandingSpeed());f.Add(state.LandingElapsed());f.Add(state.JumpedThisUpdate());f.Add(state.DashedThisUpdate());
  f.Add(state.Dashing());f.Add(state.DashPhase());f.Add(state.AttackSeconds());f.Add(state.AttackAbility());
  f.Add(state.CastingRecovery());f.Add(state.RecoveryProgress());f.Add(state.DisciplineSeconds());f.Add(state.GuardWindow());
  f.Add(state.ElementMask());f.Add(state.PotionCooldown());f.Add(state.Target());f.Add(state.QuestDialogue());f.Add(state.DialogueStage());
  for(int i=0;i<int(spells.size());++i)f.Add(state.Cooldown(i));
  const auto actor=[&](const Enemy& e){
    f.Add(e.id);f.Add(e.position);f.Add(e.home);f.Add(e.health);f.Add(e.state);f.Add(e.attackCooldown);f.Add(e.windup);f.Add(e.facing);
    f.Add(e.movementSpeed);f.Add(e.movementDistance);f.Add(e.hitFlash);f.Add(e.damagedThisUpdate);f.Add(e.bossAttack);f.Add(e.nextBossAttack);
    f.Add(e.snared);f.Add(e.rooted);f.Add(e.stunned);f.Add(e.exposed);f.Add(e.respawn);f.Add(e.roamTime);f.Add(e.assistance);
    f.Add(e.patrolEnd);f.Add(e.patrolWait);f.Add(e.patrolReady);f.Add(e.patrolReturning);f.Add(e.hasSeenPlayer);f.Add(e.lastSeenPlayer);
    f.Add(e.lostSightTime);f.Add(e.searchFacing);f.Add(e.repathTimer);f.Add(e.waypoint);f.Add(e.path.size());for(auto p:e.path)f.Add(p);
  };
  for(const auto& e:state.Combatants())actor(e);actor(state.SparringOpponent());for(const auto& e:state.Dummies())actor(e);
  f.Add(state.Feedback().size());for(const auto& e:state.Feedback()){f.Add(e.position);f.Add(e.kind);f.Add(e.amount);f.Add(e.age);f.Add(e.sequence);}
  f.Add(state.VisualEventCount());f.Add(state.VisualEventsDropped());
  for(std::size_t i=0;i<state.VisualEventCount();++i){const auto& e=state.VisualEvents()[i];
    f.Add(e.kind);f.Add(e.position);f.Add(e.height);f.Add(e.facing);f.Add(e.actor);f.Add(e.elapsed);f.Add(e.destination);f.Add(e.school);
    f.Add(e.ability);f.Add(e.flight);f.Add(e.source);f.Add(e.sourceHeight);f.Add(e.heavy);f.Add(e.empowered);f.Add(e.technique);
    f.Add(e.signature);f.Add(e.opening);f.Add(e.chain);f.Add(e.destinationHeight);f.Add(e.discipline);f.Add(e.primary);f.Add(e.evaded);f.Add(e.target);
  }
  return f.value;
}
}
struct CommandExecution::Trace {
  struct Entry {Command command;float seconds;std::uint64_t fingerprint;};
  Simulation checkpoint;
  std::vector<Entry> entries;
  std::size_t capacity=0;
  bool valid=true;
  explicit Trace(const Simulation& state):checkpoint(state){}
};
StepReceipt CommandExecution::Advance(Simulation& state,float seconds,const Command& command) {
  StepReceipt result;result.sequence=++sequence_;result.seconds=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0;
  result.command=command;result.wasDead=state.Health()<=0;result.wasSparring=state.Sparring();result.wasReplay=state.ReplayActive();
  result.previousVelocity=state.MovementVelocity();result.previousPosition=state.Player();
  result.priorDialogue=state.QuestDialogue();result.priorStage=state.DialogueStage();
  const auto start=Clock::now();state.Update(result.seconds,command);
  stats_.simulationSeconds+=std::chrono::duration<double>(Clock::now()-start).count();
  result.events.count=state.VisualEventCount();result.events.dropped=state.VisualEventsDropped();
  std::copy_n(state.VisualEvents().begin(),result.events.count,result.events.values.begin());
  if(result.seconds>0){++stats_.steps;stats_.admittedSeconds+=result.seconds;}else ++stats_.emptySteps;
  stats_.events+=result.events.count;stats_.droppedEvents+=result.events.dropped;
  if(trace_ && trace_->valid && trace_->entries.size()<trace_->capacity)
    trace_->entries.push_back({command,result.seconds,Observe(state)});
  return result;
}
void CommandExecution::BeginTrace(const Simulation& state,std::size_t capacity) {
  trace_=std::make_shared<Trace>(state);trace_->capacity=std::clamp<std::size_t>(capacity,1,256);trace_->entries.reserve(trace_->capacity);
}
void CommandExecution::InvalidateTrace(){if(trace_ && trace_->entries.size()<trace_->capacity)trace_->valid=false;}
ReplayResult CommandExecution::VerifyTrace()const {
  ReplayResult result;if(!trace_)return result;
  result.available=true;result.valid=trace_->valid;result.steps=trace_->entries.size();result.sealed=result.steps==trace_->capacity;
  if(!result.valid)return result;
  auto replay=std::make_unique<Simulation>(trace_->checkpoint);
  for(std::size_t i=0;i<trace_->entries.size();++i){const auto& entry=trace_->entries[i];replay->Update(entry.seconds,entry.command);
    if(Observe(*replay)!=entry.fingerprint){result.mismatch=i+1;return result;}}
  result.matched=true;return result;
}
}
