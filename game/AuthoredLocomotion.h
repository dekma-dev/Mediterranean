#pragma once
#include "game/Animation.h"

namespace animation {
// Retarget imported gait targets; simulation owns translation and contact times.
// The existing directional springs retain their velocity across input changes.
inline void ApplyAuthoredLocomotion(State& state,float dt) {
  auto& sample=state.pose;
  const auto& c=state.locomotion;
  const float run=SmoothStep((c.previousSpeed-3.f)/3.f);
  motion::Spring(state.runBlend,state.runVelocity,run,18.f,dt);
  sample.runBlend=Unit(state.runBlend);
  sample.authoredGait=true;
  std::copy_n(c.weights.begin(),4,sample.gaitWeights.begin());
  sample.authoredMotion=playerclips::Locomotion(sample.gaitWeights,sample.runBlend,c.phase);
  sample.motion=c.pose-c.gaitPose-c.transitionPose;
  const auto transition=[&](playerclips::Clip clip,float age,float duration,float forward,float right) {
    if(age<=0 || age>=duration)return;
    const float t=age/duration;
    const auto value=playerclips::Sample(clip,t),a=playerclips::Sample(clip,0),b=playerclips::Sample(clip,1);
    // Only the interior deviation is additive. Authored endpoints carry gait
    // and idle poses already supplied by the directional blend above.
    const float envelope=16*t*t*(1-t)*(1-t)*.6f;
    auto& out=sample.authoredMotion;
    out.hip+=(value.hip-a.hip*(1-t)-b.hip*t)*envelope;
    const float pitch=(value.torso.x-a.torso.x*(1-t)-b.torso.x*t)*envelope;
    out.torso.x+=pitch*forward;out.torso.z-=pitch*right;
    for(unsigned i=0;i<2;++i) {
      const auto v=value.hands[i],start=a.hands[i],end=b.hands[i];
      const float x=(v.x-start.x*(1-t)-end.x*t)*envelope;
      const float z=(v.z-start.z*(1-t)-end.z*t)*envelope;
      out.hands[i].x+=x*forward+z*right;
      out.hands[i].y+=(v.y-start.y*(1-t)-end.y*t)*envelope;
      out.hands[i].z+=z*forward-x*right;
    }
  };
  transition(playerclips::Clip::Start,c.startAge,.28f,c.startForward,c.startRight);
  transition(playerclips::Clip::Stop,c.stopAge,.32f,c.stopForward,c.stopRight);
}
}
