#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ui {
// Presentation only: actual health always updates immediately. The trailing
// segment briefly retains damage, then decays without frame-rate dependence.
struct HealthTrail {
  float value=0,trail=0,hold=0,maximum=0;
  std::uint32_t actor=0;
  bool initialized=false;
  void Reset() { *this={}; }
  void Update(float health,float maxHealth,std::uint32_t id,float dt) {
    const float next=std::isfinite(health) && std::isfinite(maxHealth) && maxHealth>0?
      std::clamp(health/maxHealth,0.f,1.f):0;
    if(!initialized || actor!=id || maximum!=maxHealth || next<=0 ||
      !std::isfinite(dt) || dt<0 || dt>.25f) {
      value=trail=next;hold=0;actor=id;maximum=maxHealth;initialized=true;return;
    }
    if(next<value) {trail=std::max(trail,value);hold=.20f;}
    else if(next>value) {trail=next;hold=0;}
    value=next;
    const float elapsed=std::max(0.f,dt-hold);
    hold=std::max(0.f,hold-dt);
    trail=value+(trail-value)*std::exp(-elapsed*7.f);
    if(trail-value<.001f) trail=value;
  }
};
}
