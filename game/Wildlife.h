#pragma once
#include "game/WorldMap.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace ambient {
enum class Behaviour { Rest, Roam, Alert, Flee, Return };
struct Animal {
  rpg::Vec2 home{},position{},goal{};
  float yaw=0,speed=0,gait=0,travel=0,timer=2,alarm=0;
  unsigned choice=0;
  Behaviour behaviour=Behaviour::Rest;
};
// Transient presentation actors: no combat entities, loot, saves or global RNG.
class Wildlife {
public:
  Wildlife() { Reset(); }
  void Reset() {
    animals_={};remainder_=0;
    animals_[0].home=animals_[0].position=animals_[0].goal={-18,17};animals_[0].yaw=2.2f;
    animals_[1].home=animals_[1].position=animals_[1].goal={18,-18};animals_[1].yaw=-.8f;animals_[1].timer=3.7f;
  }
  const std::array<Animal,2>& Animals() const { return animals_; }
  void Update(float seconds,const rpg::WorldMap& map,rpg::Vec2 player,bool alive=true) {
    if(!std::isfinite(seconds)||seconds<=0||!std::isfinite(player.x)||!std::isfinite(player.z))return;
    remainder_+=std::min(double(seconds),.05);
    constexpr double step=1.0/120;
    while(remainder_+1e-9>=step) {
      for(unsigned i=0;i<animals_.size();++i) Tick(animals_[i],i,float(step),map,player,alive);
      remainder_-=step;
    }
  }
private:
  std::array<Animal,2> animals_{};
  double remainder_=0;
  static float Distance(rpg::Vec2 a,rpg::Vec2 b) {return std::hypot(a.x-b.x,a.z-b.z);}
  static void Destination(Animal& a,unsigned species,const rpg::WorldMap& map) {
    for(unsigned attempt=0;attempt<12;++attempt) {
      const float angle=float(++a.choice)*2.39996323f+species;
      const float radius=2+float(a.choice%5)*.8f;
      const rpg::Vec2 p{a.home.x+std::sin(angle)*radius,a.home.z+std::cos(angle)*radius};
      if(map.CanTravel(a.position,p,.65f)) {a.goal=p;a.behaviour=Behaviour::Roam;a.timer=12;return;}
    }
    a.behaviour=Behaviour::Rest;a.timer=2;
  }
  static void Tick(Animal& a,unsigned species,float dt,const rpg::WorldMap& map,rpg::Vec2 player,bool alive) {
    const float playerDistance=alive?Distance(a.position,player):1000;
    const bool visible=playerDistance<7 && map.CanTravel(a.position,player,.05f);
    a.timer=std::max(0.f,a.timer-dt);a.alarm=std::max(0.f,a.alarm-dt);
    if(visible && playerDistance<3.4f) {a.behaviour=Behaviour::Flee;a.alarm=2.5f;}
    else if(a.behaviour==Behaviour::Flee && a.alarm<=0) {a.behaviour=Behaviour::Return;a.goal=a.home;}
    else if(a.behaviour!=Behaviour::Flee && visible && playerDistance<6) {a.behaviour=Behaviour::Alert;a.timer=1.2f;}
    else if(a.behaviour==Behaviour::Alert && a.timer<=0) {a.behaviour=Behaviour::Return;a.goal=a.home;}
    if(a.behaviour==Behaviour::Rest && a.timer<=0) Destination(a,species,map);
    if((a.behaviour==Behaviour::Roam && (a.timer<=0||Distance(a.position,a.goal)<.4f)) ||
       (a.behaviour==Behaviour::Return && Distance(a.position,a.home)<.6f)) {
      a.behaviour=Behaviour::Rest;a.timer=2+float(a.choice%4);
    }
    float desiredSpeed=0,heading=a.yaw;
    if(a.behaviour==Behaviour::Alert) heading=std::atan2(player.x-a.position.x,player.z-a.position.z);
    if(a.behaviour==Behaviour::Roam||a.behaviour==Behaviour::Return||a.behaviour==Behaviour::Flee) {
      const auto target=a.behaviour==Behaviour::Flee?rpg::Vec2{2*a.position.x-player.x,2*a.position.z-player.z}:a.goal;
      heading=std::atan2(target.x-a.position.x,target.z-a.position.z);
      desiredSpeed=a.behaviour==Behaviour::Flee?(species==0?3.6f:2.7f):(species==0?1.05f:.75f);
      // Bounded steering probes choose a clear heading; no per-frame path allocation.
      bool found=false;
      for(float offset:{0.f,.45f,-.45f,.9f,-.9f,1.5f,-1.5f,2.2f,-2.2f,3.14159265f}) {
        const float candidate=heading+offset;
        const rpg::Vec2 ahead{a.position.x+std::sin(candidate)*1.2f,a.position.z+std::cos(candidate)*1.2f};
        if(map.CanTravel(a.position,ahead,.65f)) {heading=candidate;found=true;break;}
      }
      if(!found)desiredSpeed=0;
    }
    const float turn=std::remainder(heading-a.yaw,6.2831853f);
    a.yaw=std::remainder(a.yaw+std::clamp(turn,-3.f*dt,3.f*dt),6.2831853f);
    desiredSpeed*=std::max(0.f,std::cos(turn));
    a.speed+=(desiredSpeed-a.speed)*(1-std::exp(-5.f*dt));
    const auto next=map.MoveAndSlide(a.position,{std::sin(a.yaw)*a.speed*dt,std::cos(a.yaw)*a.speed*dt},.65f);
    const float moved=Distance(next,a.position);a.position=next;
    a.gait=std::fmod(a.gait+moved*(species==0?5.f:6.f),6.2831853f);
    const float stride=std::clamp(moved/dt/1.6f,0.f,1.f);
    a.travel+=(stride-a.travel)*(1-std::exp(-12.f*dt));
  }
};
}
