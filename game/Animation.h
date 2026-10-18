#pragma once
#include "game/Motion.h"
#include <array>

// Presentation-only channels; no collision, combat timing or durable save data.
namespace animation {
inline float Unit(float value) { return std::isfinite(value)?std::clamp(value,0.f,1.f):0; }
inline float Signed(float value) { return std::isfinite(value)?std::clamp(value,-1.f,1.f):0; }
inline float SmoothStep(float value) { const float t=Unit(value); return t*t*(3-2*t); }
struct Foot { float x=0,y=.17f,z=0,yaw=0; };
struct Sample {
  float breath=0,sway=0,air=0,rise=0,landing=0,hit=0,travel=0;
  std::array<Foot,2> feet{};
  bool feetActive=false;
};
inline Sample Idle(float time,float offset=0) {
  const float seconds=std::isfinite(time)?std::remainder(time,motion::tau*2/1.8f):0;
  const float phase=seconds*1.8f+(std::isfinite(offset)?std::remainder(offset,motion::tau*2):0);
  Sample result; result.breath=std::sin(phase); result.sway=std::sin(phase*.5f);
  return result;
}
inline float Pulse(float age,float peak) {
  if (age<0 || age>1) return 0;
  const float t=age/peak;
  return t*std::exp(1-t);
}
struct State {
  Sample pose;
  float phase=0,airBlend=0,riseBlend=0,travelBlend=0;
  float landingAge=2,landingStrength=0,hitAge=2,hitCarry=0;
  void Reset(float offset=0) { *this=State{}; phase=std::isfinite(offset)?std::remainder(offset,motion::tau):0; }
  void Update(float dt,float height,float velocity,float travel,bool alive,bool damaged,float strike,
              float landingSpeed=0,float landingElapsed=0) {
    if (!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,.05f);
    if (!alive) { Reset(); return; }
    height=std::isfinite(height)?std::max(0.f,height):0;
    velocity=std::isfinite(velocity)?std::clamp(velocity,-20.f,20.f):0;
    travel=std::isfinite(travel)?std::max(0.f,travel):0;
    const bool onGround=height<=0 && velocity<=0;
    landingAge=std::min(2.f,landingAge+dt); hitAge=std::min(2.f,hitAge+dt);
    if (onGround && std::isfinite(landingSpeed) && landingSpeed>0) {
      landingAge=std::isfinite(landingElapsed)?std::clamp(landingElapsed,0.f,dt):0;
      landingStrength=Unit(landingSpeed/motion::jumpSpeed);
    }
    // Consume actual impact events: a fading tint is neither a timestamp nor
    // evidence of fresh damage. Preserve recoil when a second impact arrives.
    if (damaged) { hitAge=0; hitCarry=pose.hit; }
    // A new takeoff cannot inherit the last landing's crouch.
    if (!onGround) { landingAge=2; landingStrength=0; }
    phase=std::remainder(phase+dt*1.8f,motion::tau*2);
    airBlend=motion::Ease(airBlend,onGround?0.f:1.f,18.f,dt);
    riseBlend=motion::Ease(riseBlend,Signed(velocity/motion::jumpSpeed),16.f,dt);
    travelBlend=motion::Ease(travelBlend,onGround?Unit(travel/dt/7.f):0.f,12.f,dt);
    pose.hit=hitAge>1?0:std::max(hitCarry*std::exp(-24*hitAge),Pulse(hitAge,.055f));
    pose.landing=landingStrength*Pulse(landingAge,.075f);
    // Extend the legs before touching down, regardless of filtered air state.
    pose.air=airBlend*SmoothStep(height/.25f); pose.rise=riseBlend; pose.travel=travelBlend;
    const float rest=(1-travelBlend)*(1-Unit(strike))*(1-pose.hit)*(1-pose.air);
    pose.breath=std::sin(phase)*rest; pose.sway=std::sin(phase*.5f)*rest;
  }
};
}
