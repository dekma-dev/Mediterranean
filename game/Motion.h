#pragma once
#include <algorithm>
#include <cmath>

// Small, allocation-free motion filters. Time is in seconds and distance in
// world units. Call from Update, never Render, so extra renders cannot advance it.
namespace motion {
constexpr float tau=6.283185307f;
inline float Ease(float value,float target,float response,float dt) {
  return value+(target-value)*-std::expm1(-response*dt);
}
inline float AngleDelta(float from,float to) { return std::remainder(to-from,tau); }
inline float FollowAngle(float value,float target,float response,float dt) {
  return std::remainder(value+AngleDelta(value,target)*-std::expm1(-response*dt),tau);
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
struct Actor {
  float facing=0,phase=0,gait=0,attack=0;
  void Reset(float heading) { facing=heading; phase=gait=attack=0; }
  void Update(float heading,float travel,bool grounded,bool alive,float strike,float dt,bool committed=false) {
    if (!std::isfinite(dt) || dt<=0) return;
    facing=committed?heading:Turn(facing,heading,18.f,dt);
    if (!alive) { gait=attack=0; return; }
    // Integrate the moving sine target over this entire travel interval. Filtering
    // only its end sample made the stride longer and phase earlier at low FPS.
    constexpr float response=24.f;
    if (grounded && travel>.00001f) {
      const float previous=phase,omega=travel*4.f/dt;
      phase=std::remainder(phase+travel*4.f,tau);
      const float a=response*response/(response*response+omega*omega);
      const float b=response*omega/(response*response+omega*omega);
      const auto wave=[&](float angle) { return a*std::sin(angle)-b*std::cos(angle); };
      gait=wave(phase)+(gait-wave(previous))*std::exp(-response*dt);
    } else gait=Ease(gait,0.f,response,dt);
    attack=Ease(attack,std::clamp(strike,0.f,1.f),strike>attack?55.f:22.f,dt);
  }
};
struct CameraBoom {
  float distance=10,recoveryDelay=0;
  void Reset(float value) { distance=value; recoveryDelay=0; }
  void Update(float desired,float clearance,float dt) {
    // Retract immediately. Require 100 ms of clearance before moving back out,
    // otherwise alternating samples at a post/tree edge pump the boom in and out.
    distance=std::min(distance,clearance);
    if (clearance<desired-.01f && clearance<=distance+.05f) { recoveryDelay=.1f; return; }
    const float outwardTime=std::max(0.f,dt-recoveryDelay);
    recoveryDelay=std::max(0.f,recoveryDelay-dt);
    distance=std::min(clearance,Ease(distance,desired,9.f,desired<=distance?dt:outwardTime));
  }
};
}
