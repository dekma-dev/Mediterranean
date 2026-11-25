#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace execution {
// A presentation deadline, not a simulation clock. Missed frames are dropped;
// input wakes the native wait but cannot admit an extra render.
class FramePacing {
public:
  static constexpr unsigned gameplayHz=60,selectionHz=30;
  explicit FramePacing(std::int64_t frequency):frequency_(frequency){}
  void Reset(){ready_=false;}
  double Remaining(std::int64_t now,unsigned hz){
    if(frequency_<=0 || now<0 || !hz){Reset();return 0;}
    if(!ready_ || hz!=hz_ || now<last_){
      ready_=true;hz_=hz;next_=double(now);last_=now;
    }
    last_=now;
    return std::max(0.0,(next_-double(now))/double(frequency_));
  }
  void Admit(std::int64_t now){
    if(!ready_)return;
    const double period=double(frequency_)/hz_;
    // Keep phase through normal VSync/timer jitter; never replay a missed slot.
    next_=double(now)-next_>=period?double(now)+period:next_+period;
    last_=now;++frames_;
  }
  std::uint64_t Frames()const{return frames_;}
private:
  std::int64_t frequency_=0,last_=0;
  double next_=0;
  unsigned hz_=0;
  bool ready_=false;
  std::uint64_t frames_=0;
};
}
