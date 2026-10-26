#pragma once
#include "game/AuthoredActorMotion.h"

// Transient presentation only. Saved dead enemies start settled, never replay.
struct EnemyDefeat {
 static constexpr float duration=1.05f;
 float age=-1,attack=0,gait=0,travel=0;
 authored::Gesture held{};
 void Reset(bool alive) { *this={};age=alive?-1.f:duration; }
 void Update(float dt,bool alive,float weapon,float phase,const animation::Sample& pose) {
   if(alive) {age=-1;attack=weapon;gait=phase;travel=pose.travel;held=AuthoredActorGesture(pose);return;}
   if(age<0) {age=0;attack=weapon;gait=phase;travel=pose.travel;held=AuthoredActorGesture(pose);}
   if(std::isfinite(dt)&&dt>0)age=std::min(duration,age+dt);
 }
 float Remaining() const {return 1-animation::SmoothStep(std::max(0.f,age)/duration);}
 authored::Gesture Gesture() const {
   const float keep=Remaining();auto result=held;
   result.pitch*=keep;result.yaw*=keep;result.roll*=keep;result.hip*=keep;
   result.leftLift*=keep;result.rightLift*=keep;result.leftOpen*=keep;result.rightOpen*=keep;
   result.collapse=1-keep;return result;
 }
};
