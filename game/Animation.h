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
  float lean=0;
  float dash=0,dashBrake=0,dashImpact=0;
  float cast=0;
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
// Zero endpoint velocity at anticipation, peak and recovery.
inline float Gesture(float age,float peak,float duration) {
  if (!std::isfinite(age) || age<0 || age>=duration) return 0;
  return age<peak?SmoothStep(age/peak):1-SmoothStep((age-peak)/(duration-peak));
}
struct State {
  Sample pose;
  float phase=0,airBlend=0,riseBlend=0,travelBlend=0;
  float landingAge=2,landingStrength=0,hitAge=2,hitCarry=0;
  float castAge=2,leanVelocity=0,riseVelocity=0,travelVelocity=0;
  float dashImpactAge=2,dashCarry=0;
  void Reset(float offset=0) { *this=State{}; phase=std::isfinite(offset)?std::remainder(offset,motion::tau):0; }
  void CancelDash() { pose.dash=pose.dashBrake=pose.dashImpact=0; dashImpactAge=2; dashCarry=0; }
  void UpdateDash(float dt,bool active,float dashPhase,bool impact,bool alive) {
    if (!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,.05f);
    if (!alive) { CancelDash(); return; }
    if (impact) { dashImpactAge=0; dashCarry=pose.dash; }
    else dashImpactAge=std::min(2.f,dashImpactAge+dt);
    // Retain the forward load at collision, then recoil instead of snapping upright.
    pose.dash=active?motion::DashSpeed(dashPhase):dashCarry*(1-SmoothStep(dashImpactAge/.18f));
    pose.dashBrake=active?Gesture(dashPhase,.68f,1):0;
    pose.dashImpact=Gesture(dashImpactAge,.045f,.23f);
  }
  void Update(float dt,float height,float velocity,float travel,bool alive,bool damaged,float strike,
              float landingSpeed=0,float landingElapsed=0,float forwardAcceleration=0,bool castStarted=false) {
    if (!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,.05f);
    if (!alive) { Reset(); return; }
    castAge=castStarted?0:std::min(2.f,castAge+dt);
    pose.cast=Gesture(castAge,.18f,.72f);
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
    motion::Spring(riseBlend,riseVelocity,Signed(velocity/motion::jumpSpeed),24.f,dt);
    motion::Spring(travelBlend,travelVelocity,onGround?Unit(travel/dt/7.f):0.f,20.f,dt);
    const float leanTarget=onGround?Signed(forwardAcceleration/60.f):0;
    motion::Spring(pose.lean,leanVelocity,leanTarget,24.f,dt);
    pose.hit=hitAge>1?0:std::max(hitCarry*std::exp(-24*hitAge),Pulse(hitAge,.055f));
    pose.landing=landingStrength*Gesture(landingAge,.09f,.42f);
    // Extend the legs before touching down, regardless of filtered air state.
    pose.air=airBlend*SmoothStep(height/.25f); pose.rise=riseBlend; pose.travel=travelBlend;
    const float rest=(1-travelBlend)*(1-Unit(strike))*(1-pose.hit)*(1-pose.air);
    pose.breath=std::sin(phase)*rest; pose.sway=std::sin(phase*.5f)*rest;
  }
};
}
