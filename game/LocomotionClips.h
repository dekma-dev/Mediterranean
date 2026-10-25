#pragma once
#include "game/Motion.h"
#include <array>

namespace animation {
// Small code-authored additive clips in metres/radians. They shape the body;
// grounded foot targets and hand/weapon IK are solved after these channels.
struct MotionChannels { float hip=0,pitch=0,roll=0,yaw=0,reach=0; };
inline MotionChannels operator+(MotionChannels a,MotionChannels b) { return {a.hip+b.hip,a.pitch+b.pitch,a.roll+b.roll,a.yaw+b.yaw,a.reach+b.reach}; }
inline MotionChannels operator-(MotionChannels a,MotionChannels b) { return {a.hip-b.hip,a.pitch-b.pitch,a.roll-b.roll,a.yaw-b.yaw,a.reach-b.reach}; }
inline MotionChannels operator*(MotionChannels a,float t) { return {a.hip*t,a.pitch*t,a.roll*t,a.yaw*t,a.reach*t}; }
enum class MotionClip : unsigned { Forward,Backward,Left,Right,TurnLeft,TurnRight,Start,Stop,Rise,Fall,Land,Count };
struct ClipDefinition { const char* name; float duration; bool loop; std::array<MotionChannels,5> keys; };
inline const std::array<ClipDefinition,unsigned(MotionClip::Count)> locomotionClips={{
  {"Forward stride",1,true,{{{0,.025f,0,-.025f,0},{-.012f,.035f,0,0,.015f},{0,.025f,0,.025f,0},{-.012f,.035f,0,0,.015f},{0,.025f,0,-.025f,0}}}},
  {"Backward step",1,true,{{{0,-.018f,0,.018f,.01f},{-.009f,-.012f,0,0,.02f},{0,-.018f,0,-.018f,.01f},{-.009f,-.012f,0,0,.02f},{0,-.018f,0,.018f,.01f}}}},
  {"Left side step",1,true,{{{0,.008f,-.018f,.02f,0},{-.012f,.015f,-.03f,0,.01f},{0,.008f,-.018f,-.02f,0},{-.007f,.015f,-.025f,0,.01f},{0,.008f,-.018f,.02f,0}}}},
  {"Right side step",1,true,{{{0,.008f,.018f,-.02f,0},{-.012f,.015f,.03f,0,.01f},{0,.008f,.018f,.02f,0},{-.007f,.015f,.025f,0,.01f},{0,.008f,.018f,-.02f,0}}}},
  {"Left turn",1,true,{{{0,0,-.01f,-.045f,0},{-.006f,0,-.015f,-.035f,.01f},{0,0,-.01f,-.045f,0},{-.006f,0,-.015f,-.035f,.01f},{0,0,-.01f,-.045f,0}}}},
  {"Right turn",1,true,{{{0,0,.01f,.045f,0},{-.006f,0,.015f,.035f,.01f},{0,0,.01f,.045f,0},{-.006f,0,.015f,.035f,.01f},{0,0,.01f,.045f,0}}}},
  {"Start push",.28f,false,{{{},{-.018f,.045f,0,0,-.02f},{-.008f,.035f,0,0,-.01f},{0,.012f,0,0,0},{}}}},
  {"Stop catch",.32f,false,{{{},{-.02f,-.03f,0,0,.025f},{-.012f,-.02f,0,0,.015f},{-.003f,-.008f,0,0,.005f},{}}}},
  {"Jump rise",1,false,{{{},{-.008f,.025f,0,0,.015f},{0,.015f,0,0,.025f},{0,.005f,0,0,.01f},{}}}},
  {"Jump descent",1,false,{{{},{-.006f,-.01f,0,0,.012f},{-.012f,-.02f,0,0,.025f},{-.006f,-.01f,0,0,.012f},{}}}},
  {"Landing settle",.42f,false,{{{},{-.008f,.015f,0,0,.015f},{-.004f,.008f,0,0,.008f},{0,0,0,0,0},{}}}}
}};
inline MotionChannels SampleClip(MotionClip clip,float seconds) {
  const auto index=unsigned(clip);
  if(index>=locomotionClips.size() || !std::isfinite(seconds)) return {};
  const auto& c=locomotionClips[index];
  float t=c.loop?seconds/c.duration-std::floor(seconds/c.duration):std::clamp(seconds/c.duration,0.f,1.f);
  t*=4; const unsigned i=std::min(3u,unsigned(t)); t-=i;
  const auto& a=c.keys[i]; const auto& b=c.keys[i+1];
  const auto left=i?c.keys[i-1]:c.loop?c.keys[3]:a;
  const auto right=i<3?c.keys[i+2]:c.loop?c.keys[1]:b;
  // Cubic Hermite preserves tangents across key/loop boundaries. Non-looping
  // clips have zero endpoint velocity, avoiding new start/recovery snaps.
  const auto m0=(!c.loop && i==0)?MotionChannels{}:(b-left)*.5f;
  const auto m1=(!c.loop && i==3)?MotionChannels{}:(right-a)*.5f;
  const float t2=t*t,t3=t2*t;
  return a*(2*t3-3*t2+1)+m0*(t3-2*t2+t)+b*(-2*t3+3*t2)+m1*(t3-t2);
}
struct LocomotionController {
  MotionChannels pose;
  std::array<float,6> weights{};
  float phase=0,startAge=1,stopAge=1,landingAge=1,previousSpeed=0;
  float reverseAge=1,reverseForward=0,reverseRight=0;
  float startForward=1,startRight=0,stopForward=1,stopRight=0,lastForward=1,lastRight=0;
  bool airborne=false;
  void Reset() { *this={}; }
  void Update(float dt,float travel,float forward,float right,float turnRate,bool grounded,float vertical,float height,bool alive,float landingElapsed=0) {
    if(!std::isfinite(dt) || dt<=0) return;
    if(!alive) { Reset(); return; }
    dt=std::min(dt,.05f);
    travel=std::isfinite(travel)?std::max(0.f,travel):0;
    forward=std::isfinite(forward)?std::clamp(forward,-1.f,1.f):0;
    right=std::isfinite(right)?std::clamp(right,-1.f,1.f):0;
    turnRate=std::isfinite(turnRate)?turnRate:0;
    vertical=std::isfinite(vertical)?vertical:0; height=std::isfinite(height)?height:0;
    const float speed=grounded?travel/dt:0;
    startAge=std::min(1.f,startAge+dt); stopAge=std::min(1.f,stopAge+dt); landingAge=std::min(1.f,landingAge+dt);
    reverseAge=std::min(1.f,reverseAge+dt);
    if(grounded && !airborne && speed>.5f && previousSpeed>.5f && reverseAge>=.32f &&
       forward*lastForward+right*lastRight<-.25f) {
      reverseAge=0;reverseForward=lastForward;reverseRight=lastRight;
    }
    if(grounded && !airborne) {
      // Rapid taps must not discard/restart a nonzero one-shot. Let existing
      // pushes and catches settle while the responsive directional blend changes.
      if(speed>.15f && previousSpeed<=.15f && startAge>=.28f) {startAge=0;startForward=forward;startRight=right;}
      if(speed<=.15f && previousSpeed>.15f && stopAge>=.32f) {stopAge=0;stopForward=lastForward;stopRight=lastRight;}
    }
    // Let a current push/catch finish through takeoff; cancelling it here
    // would discard its hip offset on the first airborne frame.
    if(grounded && airborne) landingAge=std::isfinite(landingElapsed)?std::clamp(landingElapsed,0.f,dt):0;
    if(grounded && speed>.15f) {lastForward=forward;lastRight=right;}
    airborne=!grounded; previousSpeed=speed;
    // Pivot steps need a cadence even without translation. Blend out their
    // contribution during travel so strafing does not double the foot cycle.
    const float pivot=grounded?std::min(std::abs(turnRate),3.f)*dt*.18f*(1-std::clamp(speed/2.f,0.f,1.f)):0;
    phase=std::remainder(phase+travel/1.57f+pivot,1.f);
    const float moving=std::clamp(speed/2.f,0.f,1.f);
    std::array<float,6> target={std::max(0.f,forward),std::max(0.f,-forward),std::max(0.f,-right),std::max(0.f,right),0,0};
    const float sum=target[0]+target[1]+target[2]+target[3];
    for(unsigned i=0;i<4;++i) target[i]=sum>.0001f?target[i]/sum*moving:0;
    const float turn=grounded?std::clamp(turnRate/3.f,-1.f,1.f)*(1-moving*.7f):0;
    target[4]=std::max(0.f,-turn); target[5]=std::max(0.f,turn);
    pose={};
    for(unsigned i=0;i<6;++i) {
      weights[i]=motion::Ease(weights[i],target[i],14,dt);
      pose=pose+SampleClip(MotionClip(i),phase)*weights[i];
    }
    const auto directed=[](MotionChannels clip,float forward,float right) {
      const float pitch=clip.pitch;clip.pitch*=forward;clip.roll-=pitch*right;
      return clip;
    };
    pose=pose+directed(SampleClip(MotionClip::Start,startAge),startForward,startRight)+
      directed(SampleClip(MotionClip::Stop,stopAge),stopForward,stopRight);
    // A rapid reversal catches weight along the old direction while the
    // directional gait blends toward the new one. It never moves the actor.
    pose=pose+directed(SampleClip(MotionClip::Stop,reverseAge),reverseForward,reverseRight)*.65f;
    if(!grounded) {
      const float air=std::clamp(height/.25f,0.f,1.f);
      const float rise=std::clamp(vertical/motion::jumpSpeed,-1.f,1.f);
      pose=pose+SampleClip(rise>=0?MotionClip::Rise:MotionClip::Fall,rise>=0?1-rise:-rise)*air;
    }
    // A repeat jump must not discard the nonzero settle clip; it finishes
    // through takeoff just like the start/stop one-shots.
    pose=pose+SampleClip(MotionClip::Land,landingAge);
  }
};
}
