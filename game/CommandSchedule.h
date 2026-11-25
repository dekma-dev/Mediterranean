#pragma once
#include "game/Simulation.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rpg {
// Bounded semi-fixed integration: consume this frame's admitted time completely,
// with no accumulator debt or extra input latency. Only the first step receives
// edge actions. Continuous controls remain present in all following steps.
class CommandSchedule {
public:
  static constexpr float maximumFrame=.05f,maximumStep=1.f/120;
  static constexpr unsigned maximumSteps=6;
  struct Plan {
    Command first,held;
    unsigned count=0;
    float seconds=0;
    const Command& At(unsigned index) const {return index==0?first:held;}
  };
  struct Statistics {
    std::uint64_t frames=0,subdivided=0,steps=0,cancelledSteps=0,invalid=0,cappedFrames=0;
    unsigned peakSteps=0;
    double admittedSeconds=0,cancelledSeconds=0,cappedSeconds=0;
  };
  Plan Begin(float seconds,const Command& command) {
    Plan plan;
    if(!std::isfinite(seconds) || seconds<=0){++stats_.invalid;return plan;}
    if(seconds>maximumFrame){++stats_.cappedFrames;stats_.cappedSeconds+=double(seconds)-maximumFrame;seconds=maximumFrame;}
    plan.count=std::clamp(unsigned(std::ceil(double(seconds)/maximumStep-1e-6)),1u,maximumSteps);
    plan.seconds=seconds/plan.count;plan.first=command;
    // An allow-list makes new command fields one-shot by default. In particular,
    // ability/queue insertion, jump, selection, trade and quest acceptance must
    // never be replayed by a long render frame.
    plan.held.movement=command.movement;plan.held.sprint=command.sprint;
    plan.held.steerFacing=command.steerFacing;plan.held.facingYaw=command.facingYaw;
    plan.held.haltMovement=command.haltMovement;plan.held.queueAttack=command.queueAttack;
    ++stats_.frames;stats_.subdivided+=plan.count>1;stats_.admittedSeconds+=seconds;
    stats_.peakSteps=std::max(stats_.peakSteps,plan.count);
    return plan;
  }
  void Complete(const Plan& plan,unsigned executed) {
    executed=std::min(executed,plan.count);stats_.steps+=executed;
    stats_.cancelledSteps+=plan.count-executed;
    stats_.cancelledSeconds+=double(plan.count-executed)*plan.seconds;
  }
  const Statistics& Stats() const {return stats_;}
private:
  Statistics stats_;
};
}
