#pragma once
#include "game/Motion.h"
#include <array>

namespace ui {
// Event-owned HUD history. Rendering never consumes or replays simulation events.
struct DamageDirection {
  static constexpr float duration=.85f;
  struct Pulse { float bearing=0,age=duration; };
  std::array<Pulse,6> pulses{};
  void Reset() { *this={}; }
  void Advance(float dt,bool alive=true) {
    if(!alive || !std::isfinite(dt) || dt<0 || dt>.25f) {Reset();return;}
    for(auto& pulse:pulses) pulse.age=std::min(duration,pulse.age+dt);
  }
  void Hit(float incomingFacing,float elapsed=0) {
    if(!std::isfinite(incomingFacing) || !std::isfinite(elapsed) || elapsed<0 || elapsed>=duration) return;
    const float bearing=std::remainder(incomingFacing+motion::tau*.5f,motion::tau);
    auto* slot=&pulses[0];
    for(auto& pulse:pulses) {
      if(pulse.age<duration && std::abs(motion::AngleDelta(pulse.bearing,bearing))<.3f) {
        // A younger contact wins even if events arrive in a different order.
        if(elapsed<pulse.age) pulse={bearing,elapsed};
        return;
      }
      if(pulse.age>slot->age) slot=&pulse;
    }
    if(elapsed<=slot->age) *slot={bearing,elapsed};
  }
  static float Opacity(const Pulse& pulse) {
    const float t=std::clamp(pulse.age/duration,0.f,1.f);
    return (1-t)*(1-t);
  }
  static float ScreenAngle(const Pulse& pulse,float cameraYaw) {
    return std::isfinite(cameraYaw)?std::remainder(pulse.bearing-cameraYaw,motion::tau):0;
  }
};
}
