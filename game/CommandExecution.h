#pragma once
#include "game/Simulation.h"
#include <memory>

namespace rpg {
// Owned event snapshot; valid after the next Simulation::Update. The remaining
// actor state is consumed synchronously before another step, never cross-thread.
struct StepEvents {
  std::array<CombatVisualEvent,Simulation::visualEventCapacity> values{};
  std::size_t count=0;
  unsigned dropped=0;
};
struct StepReceipt {
  std::uint64_t sequence=0;
  float seconds=0;
  Command command;
  bool wasDead=false,wasSparring=false,wasReplay=false;
  Vec2 previousVelocity{},previousPosition{};
  QuestId priorDialogue=QuestId::None;
  QuestStage priorStage{};
  StepEvents events;
};
struct ExecutionStats {
  std::uint64_t steps=0,emptySteps=0,events=0,droppedEvents=0;
  double simulationSeconds=0,presentationSeconds=0,admittedSeconds=0;
};
struct ReplayResult {
  bool available=false,valid=false,matched=false,sealed=false;
  std::size_t steps=0,mismatch=0;
};
class CommandExecution {
public:
  CommandExecution()=default;
  CommandExecution(const CommandExecution&)=delete;
  CommandExecution& operator=(const CommandExecution&)=delete;
  CommandExecution(CommandExecution&&)=default;
  CommandExecution& operator=(CommandExecution&&)=default;
  StepReceipt Advance(Simulation& state,float seconds,const Command& command);
  const ExecutionStats& Stats()const{return stats_;}
  void RecordPresentation(double seconds){stats_.presentationSeconds+=seconds;}
  // Opt-in diagnostic window: checkpoint includes transient combat/navigation
  // state that a gameplay save deliberately omits. Never enabled in normal play.
  void BeginTrace(const Simulation& state,std::size_t capacity=90);
  void InvalidateTrace();
  ReplayResult VerifyTrace()const;
private:
  struct Trace;
  std::shared_ptr<Trace> trace_;
  ExecutionStats stats_;
  std::uint64_t sequence_=0;
};
}
