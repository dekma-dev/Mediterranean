#pragma once
#include <algorithm>
#include <cmath>

// Small, allocation-free motion filters. Time is in seconds and distance in
// world units. Call from Update, never Render, so extra renders cannot advance it.
namespace motion {
constexpr float tau=6.283185307f;
// A deliberate RPG jump: 1.08 m apex, 0.60 s total flight on level ground.
constexpr float jumpSpeed=7.2f,gravity=24.f;
constexpr float groundAcceleration=16.f,groundBraking=24.f;
// Camera-facing movement has a planted side step and a shorter backward stride.
inline float DirectionalPace(float forward) {
  forward=std::clamp(forward,-1.f,1.f);
  return forward>=0?.85f+.15f*forward:.85f+.20f*forward;
}
// A fast push-off followed by longer braking, integrated analytically.
// Peak speed is 2 * distance / duration; no frame can skip collision sweeps.
constexpr float dashDuration=.34f,dashDistance=3.5f,dashPeak=.22f;
constexpr float dashStandOff=1.5f,dashRange=dashDistance+dashStandOff;
inline float DashSpeed(float phase) {
  const float t=std::clamp(phase,0.f,1.f);
  const float u=t<dashPeak?t/dashPeak:(t-dashPeak)/(1-dashPeak);
  const float smooth=u*u*(3-2*u);
  return t<dashPeak?smooth:1-smooth;
}
inline float DashTravel(float phase) {
  const float t=std::clamp(phase,0.f,1.f);
  if (t<dashPeak) {
    const float u=t/dashPeak;
    return dashPeak*u*u*u*(2-u);
  }
  const float u=(t-dashPeak)/(1-dashPeak);
  return dashPeak+2*(1-dashPeak)*(u-u*u*u+.5f*u*u*u*u);
}
// Signed velocity response with its exact displacement integral. Unlike Travel,
// direction changes preserve momentum and a zero target provides braking.
inline float IntegrateVelocity(float& velocity,float target,float response,float dt) {
  const float change=-std::expm1(-response*dt);
  const float distance=target*dt+(velocity-target)*change/response;
  velocity+=(target-velocity)*change;
  return distance;
}
inline float Ease(float value,float target,float response,float dt) {
  return value+(target-value)*-std::expm1(-response*dt);
}
inline float AngleDelta(float from,float to) { return std::remainder(to-from,tau); }
inline float FollowAngle(float value,float target,float response,float dt) {
  return std::remainder(value+AngleDelta(value,target)*-std::expm1(-response*dt),tau);
}
// Exact critically damped response to a fixed target. Keeping velocity across
// target changes avoids the instantaneous start/stop of a first-order blend.
inline void Spring(float& value,float& velocity,float target,float response,float dt) {
  const float offset=value-target,impulse=velocity+response*offset;
  const float decay=std::exp(-response*dt);
  value=target+(offset+impulse*dt)*decay;
  velocity=(velocity-response*impulse*dt)*decay;
}
inline float Turn(float value,float target,float radiansPerSecond,float dt) {
  return std::remainder(value+std::clamp(AngleDelta(value,target),-radiansPerSecond*dt,radiansPerSecond*dt),tau);
}
// Integrate the exponential speed ramp analytically, avoiding a different
// starting distance at 30/60/144 Hz. No input means an immediate physical stop.
inline float Travel(float& speed,float target,float response,float dt) {
  if (target<=0) { speed=0; return 0; }
  const float change=-std::expm1(-response*dt);
  const float distance=target*dt+(speed-target)*change/response;
  speed+=(target-speed)*change;
  return std::max(0.f,distance);
}
// Presentation-only displacement history, in world space. Projection must use
// the current heading: differentiating local velocity would invent acceleration
// just because an actor rotated. Teleports/death must not produce a body kick.
struct MovementSample {
  float x=0,z=0,vx=0,vz=0;
  float forward=0,right=0,acceleration=0,lateralAcceleration=0;
  void Reset(float px,float pz) { *this={}; x=px; z=pz; }
  void Update(float px,float pz,float heading,float dt,bool alive) {
    if (!std::isfinite(dt) || dt<=0) return;
    const float dx=px-x,dz=pz-z,distance=std::hypot(dx,dz);
    if (!alive || distance>.85f) { Reset(px,pz); return; }
    const float nextX=dx/dt,nextZ=dz/dt,c=std::cos(heading),s=std::sin(heading);
    forward=distance>1e-6f?(s*dx+c*dz)/distance:0;
    right=distance>1e-6f?(c*dx-s*dz)/distance:0;
    acceleration=((nextX-vx)*s+(nextZ-vz)*c)/dt;
    lateralAcceleration=((nextX-vx)*c-(nextZ-vz)*s)/dt;
    vx=nextX; vz=nextZ; x=px; z=pz;
  }
};
struct Actor {
  float facing=0,phase=0,gait=0,attack=0;
  float turnVelocity=0,gaitVelocity=0,attackVelocity=0;
  void Reset(float heading) { facing=heading; phase=gait=attack=turnVelocity=gaitVelocity=attackVelocity=0; }
  void Update(float heading,float travel,bool grounded,bool alive,float strike,float dt,bool committed=false,float turnResponse=22.f) {
    if (!std::isfinite(dt) || dt<=0) return;
    if (committed) { facing=heading; turnVelocity=0; }
    else {
      Spring(facing,turnVelocity,facing+AngleDelta(facing,heading),turnResponse,dt);
      facing=std::remainder(facing,tau);
    }
    if (!alive) { gait=attack=gaitVelocity=attackVelocity=0; return; }
    // Integrate the moving sine target over this entire travel interval. Filtering
    // only its end sample made the stride longer and phase earlier at low FPS.
    // Subtract the sinusoidal particular solution, advance the homogeneous
    // spring, then restore the wave. Its derivative also remains continuous.
    constexpr float response=36.f;
    if (grounded && travel>.00001f) {
      const float previous=phase,omega=travel*4.f/dt;
      phase=std::remainder(phase+travel*4.f,tau);
      const float sum=response*response+omega*omega;
      const float a=response*response*(response*response-omega*omega)/(sum*sum);
      const float b=-2*response*response*response*omega/(sum*sum);
      const auto wave=[&](float angle) { return a*std::sin(angle)+b*std::cos(angle); };
      const auto slope=[&](float angle) { return omega*(a*std::cos(angle)-b*std::sin(angle)); };
      gait-=wave(previous); gaitVelocity-=slope(previous);
      Spring(gait,gaitVelocity,0,response,dt);
      gait+=wave(phase); gaitVelocity+=slope(phase);
    } else Spring(gait,gaitVelocity,0,response,dt);
    Spring(attack,attackVelocity,std::clamp(strike,0.f,1.f),72.f,dt);
  }
};
struct CameraBoom {
  float distance=10,recoveryDelay=0,velocity=0;
  void Reset(float value) { distance=value; recoveryDelay=velocity=0; }
  void Update(float desired,float clearance,float dt) {
    if (!std::isfinite(dt) || dt<=0 || !std::isfinite(desired) || !std::isfinite(clearance)) return;
    // Retract immediately. Require 100 ms of clearance before moving back out,
    // otherwise alternating samples at a post/tree edge pump the boom in and out.
    if (distance>clearance) { distance=clearance; velocity=0; }
    if (clearance<desired-.01f && clearance<=distance+.05f) { recoveryDelay=.1f; velocity=0; return; }
    const float outwardTime=std::max(0.f,dt-recoveryDelay);
    recoveryDelay=std::max(0.f,recoveryDelay-dt);
    // Preserve velocity across wheel reversals; collision remains authoritative.
    Spring(distance,velocity,desired,14.f,desired<=distance?dt:outwardTime);
    if (distance>clearance) { distance=clearance; velocity=0; }
  }
};
}
