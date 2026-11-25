#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace execution {
// One owner for wall time admitted to the variable-step game. No catch-up debt:
// normal frames retain the existing 50 ms ceiling; pauses are accounted apart.
class FrameClock {
public:
  static constexpr double maximumStep=.05;
  struct Step { double seconds=0; bool resynchronized=false; };
  struct Statistics {
    std::uint64_t frames=0,resynchronizations=0,clampedFrames=0,invalidSamples=0;
    double wallSeconds=0,gameSeconds=0,excludedSeconds=0,clampedSeconds=0;
  };
  explicit FrameClock(std::int64_t frequency):frequency_(frequency){}
  Step Sample(std::int64_t tick,bool active,bool interrupted=false) {
    if(frequency_<=0 || tick<0){++statistics_.invalidSamples;ready_=false;active_=false;return {};}
    if(!ready_){last_=tick;ready_=true;active_=active;++statistics_.resynchronizations;return {0,true};}
    if(tick<last_){++statistics_.invalidSamples;active_=false;return {};}
    const double elapsed=double(tick-last_)/double(frequency_);last_=tick;
    statistics_.wallSeconds+=elapsed;
    if(!active || !active_ || interrupted){
      statistics_.excludedSeconds+=elapsed;
      const bool reset=active && (!active_ || interrupted);
      active_=active;if(reset)++statistics_.resynchronizations;
      return {0,reset};
    }
    if(elapsed==0)return {};
    const double admitted=std::min(elapsed,maximumStep);
    ++statistics_.frames;statistics_.gameSeconds+=admitted;
    if(elapsed>maximumStep){++statistics_.clampedFrames;statistics_.clampedSeconds+=elapsed-admitted;}
    return {admitted,false};
  }
  const Statistics& Stats()const{return statistics_;}
private:
  std::int64_t frequency_=0,last_=0;
  bool ready_=false,active_=false;
  Statistics statistics_;
};
}
