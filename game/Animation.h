#pragma once
#include "game/Motion.h"
#include "game/AttackMotion.h"
#include "game/LocomotionClips.h"
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
  float dash=0,dashBrake=0,dashImpact=0,dashPhase=0,dashLoad=0;
  float cast=0;
  float attackSeconds=-1;
  bool heavyAttack=false;
  float takeoff=0,bank=0;
  float support=0,clothSide=0,clothBack=0;
  std::array<Foot,2> departureFeet{};
  float departure=0,departureSupport=0;
  MotionChannels motion;
};
inline Sample Idle(float time,float offset=0) {
  const float seconds=std::isfinite(time)?std::remainder(time,motion::tau*2/1.8f):0;
  const float phase=seconds*1.8f+(std::isfinite(offset)?std::remainder(offset,motion::tau*2):0);
  Sample result; result.breath=std::sin(phase); result.sway=std::sin(phase*.5f);
  // Half-integer harmonics stay continuous across the shared 4-pi clock wrap.
  result.clothSide=std::sin(phase*.5f)*.008f; result.clothBack=.025f;
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
inline float DashBrake(float phase) {
  return SmoothStep((phase-.18f)/.48f)*(1-.65f*SmoothStep((phase-.65f)/.35f));
}
struct State {
  Sample pose;
  LocomotionController locomotion;
  float phase=0,airBlend=0,riseBlend=0,travelBlend=0;
  float landingAge=2,landingStrength=0,hitAge=2,hitCarry=0;
  float restWeight=1;
  float castAge=2,leanVelocity=0,riseVelocity=0,travelVelocity=0;
  float bankVelocity=0;
  float clothSideVelocity=0,clothBackVelocity=0;
  float flightAge=1,departureLanding=0;
  bool wasAirborne=false,hasDeparture=false;
  float dashImpactAge=2,dashCarry=0,brakeCarry=0;
  bool dashCollision=false;
  void Reset(float offset=0) { *this=State{}; phase=std::isfinite(offset)?std::remainder(offset,motion::tau):0; }
  float RecoilAt(float age) const {
    return age>1?0:std::max(hitCarry*std::exp(-24*age),Pulse(age,.055f));
  }
  void RefreshRecoil() {
    pose.hit=RecoilAt(hitAge);
    const float rest=restWeight*(1-pose.hit);
    pose.breath=std::sin(phase)*rest; pose.sway=std::sin(phase*.5f)*rest;
  }
  // Apply after Update, in contact order. Elapsed is the time between impact
  // and this frame's end. Sample the old reaction at contact, not at either
  // frame boundary, so two hits in one frame preserve the same recoil as two
  // separate updates. The event stream remains owned by the simulation.
  void ApplyImpact(float elapsed) {
    if (!std::isfinite(elapsed) || elapsed<0) return;
    hitCarry=RecoilAt(std::max(0.f,hitAge-elapsed)); hitAge=elapsed;
    RefreshRecoil();
  }
  void CancelDash() {
    pose.dash=pose.dashBrake=pose.dashImpact=pose.dashPhase=pose.dashLoad=0;
    dashImpactAge=2; dashCarry=brakeCarry=0; dashCollision=false;
  }
  void UpdateDash(float dt,bool active,float dashPhase,bool impact,bool alive,bool finished=false,float endElapsed=0) {
    if (!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,.05f);
    if (!alive) { CancelDash(); return; }
    if (active) { dashImpactAge=2; dashCarry=brakeCarry=0; dashCollision=false; }
    else if (impact || finished) {
      dashImpactAge=std::isfinite(endElapsed)?std::clamp(endElapsed,0.f,dt):0;
      dashCarry=impact?pose.dash:0; brakeCarry=impact?pose.dashBrake:.35f; dashCollision=impact;
    } else dashImpactAge=std::min(2.f,dashImpactAge+dt);
    // A normal stop settles out of its catch step; collision retains the prior
    // forward load and adds recoil. Neither recovery delays gameplay commands.
    pose.dashPhase=active?Unit(dashPhase):1;
    pose.dash=active?motion::DashSpeed(pose.dashPhase):dashCarry*(1-SmoothStep(dashImpactAge/.18f));
    pose.dashBrake=active?DashBrake(pose.dashPhase):brakeCarry*(1-SmoothStep(dashImpactAge/.16f));
    pose.dashImpact=dashCollision?Gesture(dashImpactAge,.045f,.23f):0;
    pose.dashLoad=active?Gesture(pose.dashPhase,.10f,.32f):0;
  }
  void Update(float dt,float height,float velocity,float travel,bool alive,float strike,
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
    if (!onGround && !wasAirborne) {
      flightAge=0; hasDeparture=pose.feetActive;
      departureLanding=pose.landing;
      pose.departureFeet=pose.feet; pose.departureSupport=pose.support;
    }
    flightAge=std::min(1.f,flightAge+dt);
    wasAirborne=!onGround;
    if (onGround) { hasDeparture=false; departureLanding=0; }
    // Carry the departing foot arrangement briefly into flight. Both contacts
    // are released immediately; this only blends the airborne joint targets.
    pose.departure=hasDeparture?1-SmoothStep(flightAge/.12f):0;
    landingAge=std::min(2.f,landingAge+dt); hitAge=std::min(2.f,hitAge+dt);
    if (onGround && std::isfinite(landingSpeed) && landingSpeed>0) {
      landingAge=std::isfinite(landingElapsed)?std::clamp(landingElapsed,0.f,dt):0;
      landingStrength=Unit(landingSpeed/motion::jumpSpeed);
    }
    // Discard the old impact event, but release its visible compression over
    // the same short interval as departing feet instead of teleporting upright.
    if (!onGround) { landingAge=2; landingStrength=0; }
    phase=std::remainder(phase+dt*1.8f,motion::tau*2);
    airBlend=motion::Ease(airBlend,onGround?0.f:1.f,18.f,dt);
    motion::Spring(riseBlend,riseVelocity,Signed(velocity/motion::jumpSpeed),24.f,dt);
    motion::Spring(travelBlend,travelVelocity,onGround?Unit(travel/dt/7.f):0.f,20.f,dt);
    const float leanTarget=onGround?Signed(forwardAcceleration/60.f):0;
    motion::Spring(pose.lean,leanVelocity,leanTarget,24.f,dt);
    pose.landing=onGround?landingStrength*Gesture(landingAge,.09f,.42f):
      departureLanding*(1-SmoothStep(flightAge/.12f));
    // Extend the legs before touching down, regardless of filtered air state.
    pose.air=airBlend*SmoothStep(height/.25f); pose.rise=riseBlend; pose.travel=travelBlend;
    // Keep the push leg extended during the first part of ascent, then tuck.
    // This follows actual flight velocity; it cannot retrigger while falling.
    pose.takeoff=onGround?0:SmoothStep((velocity/motion::jumpSpeed-.55f)/.45f);
    // A bounded, damped cloth response to locomotion and flight. The shoulder
    // seam stays pinned; only the lower sheet responds, with no solver allocations.
    const float flow=Unit(travel/dt/7.f);
    motion::Spring(pose.clothBack,clothBackVelocity,
      std::clamp(.025f+flow*.24f+Signed(velocity/motion::jumpSpeed)*.05f,.005f,.32f),14.f,dt);
    motion::Spring(pose.clothSide,clothSideVelocity,
      -Signed(pose.bank)*.12f+std::sin(phase*3)*flow*.025f+std::sin(phase*.5f)*.008f,12.f,dt);
    restWeight=(1-travelBlend)*(1-Unit(strike))*(1-pose.air);
    RefreshRecoil();
  }
};
}
