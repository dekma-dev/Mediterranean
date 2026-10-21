#pragma once
#include "game/Motion.h"
#include <cstdint>

// Presentation-only attention and expression. Never turns the collision body,
// weapon or attack cone, and never reads the wall clock or consumes gameplay RNG.
namespace facial {
inline float Unit(float x) { return std::isfinite(x)?std::clamp(x,0.f,1.f):0; }
inline float Smooth(float x) { x=Unit(x); return x*x*(3-2*x); }
struct Pose {
  float yaw=0,pitch=0,eyeYaw=0,eyePitch=0;
  float blink=0,focus=0,pain=0,smile=0;
};
struct Aim { float yaw=0,pitch=0,eyeYaw=0,eyePitch=0; };
inline Aim LookAt(float dx,float dy,float dz,float facing) {
  if(!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz) || !std::isfinite(facing)) return {};
  const float range=std::hypot(dx,dz);
  if(range<.35f || range>14) return {};
  const float angle=std::remainder(std::atan2(dx,dz)-facing,motion::tau);
  // Fade before the rear seam: a target crossing behind cannot flip the head.
  const float weight=(1-Smooth((std::abs(angle)-1.15f)/.65f))*(1-Smooth((range-10)/4));
  const float pitch=std::clamp(-std::atan2(dy,range),-.35f,.35f);
  const float head=std::clamp(angle,-.62f,.62f);
  return {head*weight,pitch*.7f*weight,std::clamp(angle-head,-.22f,.22f)*weight,pitch*.3f*weight};
}
inline float Blink(double clock,std::uint32_t seed=0) {
  if(!std::isfinite(clock)) return 0;
  const double period=4.1+.13*(seed%7);
  double age=std::fmod(clock+(seed%97)*.731,period);
  if(age<0) age+=period;
  age-=1.3;
  if(age<0 || age>.19) return 0;
  return age<.065?Smooth(float(age/.065)):1-Smooth(float((age-.065)/.125));
}
inline Pose Idle(double clock,std::uint32_t seed=0) { Pose p; p.blink=Blink(clock,seed); return p; }
struct State {
  Pose pose;
  double clock=0;
  std::uint32_t seed=0;
  float yawVelocity=0,pitchVelocity=0,eyeYawVelocity=0,eyePitchVelocity=0;
  void Reset(std::uint32_t id=0) { *this=State{}; seed=id; pose=Idle(0,id); }
  void Update(float dt,Aim aim,float focus,float pain,float smile,bool alive=true) {
    if(!std::isfinite(dt) || dt<=0) return;
    if(!alive) { const auto id=seed; Reset(id); return; }
    dt=std::min(dt,.05f);
    // Shared period avoids float-clock drift without a blink discontinuity.
    clock=std::fmod(clock+dt,4.1+.13*(seed%7));
    const auto sane=[](float x,float limit) { return std::isfinite(x)?std::clamp(x,-limit,limit):0.f; };
    motion::Spring(pose.yaw,yawVelocity,sane(aim.yaw,.62f),12.f,dt);
    motion::Spring(pose.pitch,pitchVelocity,sane(aim.pitch,.245f),12.f,dt);
    motion::Spring(pose.eyeYaw,eyeYawVelocity,sane(aim.eyeYaw,.22f),24.f,dt);
    motion::Spring(pose.eyePitch,eyePitchVelocity,sane(aim.eyePitch,.105f),24.f,dt);
    pose.focus=motion::Ease(pose.focus,Unit(focus),12.f,dt);
    pose.smile=motion::Ease(pose.smile,Unit(smile),8.f,dt);
    pose.pain=Unit(pain); // Already aged from authoritative contact events.
    pose.blink=Blink(clock,seed);
  }
};
}
