#pragma once
#include "engine/WolfAnimation.h"
#include "game/Simulation.h"
#include "game/AuthoredActorMotion.h"
#include "game/QuadrupedContacts.h"

namespace presentation {
// Presentation only: observes successful navigation, never moves the collider.
struct BeastLocomotion {
 float phase=0,travel=0,speed=0,jaw=0,jawVelocity=0;
 float pitchVelocity=0,rollVelocity=0,liftVelocity=0;
 double clock=0;
 float idleBlend=0,idleVelocity=0;
 QuadrupedContacts feet;
 authored::Gesture gesture{};
 void Reset(unsigned id=0) { *this={};clock=double(id%31)*.137; }

 template<class Height> static authored::Gesture Support(rpg::Vec2 p,float facing,bool wolf,Height height) {
   const float length=wolf?.55f:.46f,width=wolf?.20f:.27f,c=std::cos(facing),s=std::sin(facing);
   const auto at=[&](float x,float z){return height(rpg::Vec2{p.x+c*x+s*z,p.z-s*x+c*z});};
   const float fl=at(-width,length),fr=at(width,length),bl=at(-width,-length),br=at(width,-length);
   authored::Gesture result;
   result.supportPitch=std::clamp(-std::atan2((fl+fr-bl-br)*.5f,2*length),-.38f,.38f);
   result.supportRoll=std::clamp(std::atan2((fr+br-fl-bl)*.5f,2*width),-.30f,.30f);
   result.supportLift=std::clamp((fl+fr+bl+br)*.25f-height(p),-.18f,.18f);
   return result;
 }
 void Place(const rpg::Enemy& enemy) {
   const auto ground=Support(enemy.position,enemy.facing,enemy.kind==rpg::EnemyKind::Wolf,rpg::WorldMap::Height);
   gesture.supportPitch=ground.supportPitch;gesture.supportRoll=ground.supportRoll;gesture.supportLift=ground.supportLift;
 }
 void Update(float dt,const rpg::Enemy& enemy,const motion::MovementSample& movement,
             const animation::Sample& animation,float releaseAge,bool active,bool detailed=true) {
   feet.count=0;
   if(!std::isfinite(dt)||dt<=0||enemy.health<=0)return;
   dt=std::min(dt,.05f);clock+=dt;
   const bool wolf=enemy.kind==rpg::EnemyKind::Wolf;
   const float distance=std::isfinite(enemy.movementDistance)?std::clamp(enemy.movementDistance,0.f,.85f):0;
   const float targetSpeed=distance/dt;
   // Small integration steps keep the phase matched to the changing clip blend
   // at different frame rates. A stationary collider never advances the stride.
   for(float elapsed=0;elapsed<dt;) {
     const float h=std::min(1.f/240,dt-elapsed);
     const float middle=motion::Ease(speed,targetSpeed,14.f,h*.5f);
     const float blend=wolf?authored::wolf::TravelAtSpeed(middle):std::clamp(middle/3.2f,0.f,1.f);
     const float stride=wolf?authored::wolf::Blend(blend).stride:1.15f;
     phase=std::remainder(phase+targetSpeed*h*motion::tau/stride*(movement.forward<-.1f?-1.f:1.f),motion::tau);
     speed=motion::Ease(speed,targetSpeed,14.f,h);elapsed+=h;
   }
   travel=wolf?authored::wolf::TravelAtSpeed(speed):std::clamp(speed/3.2f,0.f,1.f);
   const float opening=active && wolf?(enemy.windup>0?combat::Smooth(1-enemy.windup/enemy.WindupDuration()):
     1-combat::Smooth(releaseAge/.09f)):0;
   motion::Spring(jaw,jawVelocity,opening,opening>jaw?32.f:48.f,dt);
   const auto ground=Support(enemy.position,enemy.facing,wolf,rpg::WorldMap::Height);
   motion::Spring(gesture.supportPitch,pitchVelocity,ground.supportPitch,18.f,dt);
   motion::Spring(gesture.supportRoll,rollVelocity,ground.supportRoll,18.f,dt);
   motion::Spring(gesture.supportLift,liftVelocity,ground.supportLift,18.f,dt);
   gesture.pitch=std::clamp(-::animation::ForwardRecoil(animation)*.13f+animation.lean*.06f,-.20f,.20f);
   gesture.yaw=animation.hitSide*.065f;
   gesture.roll=std::clamp(animation.bank*.08f-animation.hitSide*.10f,-.16f,.16f);
   gesture.hip=-animation.hit*.022f;
   gesture.jaw=std::clamp(jaw,0.f,1.f);
   // Deterministic, per-actor idle beats yield immediately to accepted combat.
   const bool quiet=enemy.state==rpg::EnemyState::Idle && enemy.windup<=0 && speed<.12f && enemy.stunned<=0;
   motion::Spring(idleBlend,idleVelocity,quiet?1.f:0.f,12.f,dt);
   // All frequencies are multiples of .05 rad/s, so wrapping this common
   // period cannot snap the head, ears or tail after a long idle.
   const float t=float(std::fmod(clock,double(motion::tau)*20)),sniff=std::pow(std::max(0.f,std::sin(t*.65f)),4.f)*idleBlend;
   gesture.headPitch=(wolf?.10f:.16f)*sniff;
   gesture.headYaw=std::sin(t*.7f)*(wolf?.16f:.08f)*idleBlend;
   gesture.ears=std::pow(std::max(0.f,std::sin(t*2.1f)),12.f)*.13f*idleBlend;
   gesture.tail=std::sin(t*1.7f)*.12f*idleBlend;
   const float load=active && enemy.windup>0?combat::Smooth(1-enemy.windup/enemy.WindupDuration()):0;
   gesture.pitch+=load*(wolf?-.035f:.075f);
   gesture.hip-=load*(wolf?.025f:.040f);
   if(!detailed) {feet.Reset();gesture.paws={};return;}
   gesture.paws={};
   const auto asset=wolf?authored::Asset::Wolf:authored::Asset::Boar;
   const float strike=active?(enemy.windup>0?load:1-combat::Smooth(releaseAge/.4f)):0;
   const auto base=authored::Pose(asset,float(clock),phase,strike,travel,gesture);
   feet.Update(dt,asset,enemy.id,enemy.position,enemy.facing,phase,travel,speed,base,gesture,rpg::WorldMap::Height);
 }
};
}
