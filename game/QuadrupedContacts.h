#pragma once
#include "engine/QuadrupedRig.h"
#include "engine/WolfAnimation.h"
#include "game/Motion.h"
#include "game/CreatureContacts.h"

namespace presentation {
class QuadrupedContacts {
 struct Foot {
   DirectX::XMFLOAT3 anchor{},from{};
   float weight=0,step=-1;
   bool planted=false;
 };
 std::array<Foot,4> feet_{};
 bool ready_=false;
 rpg::Vec2 previousPosition_{};
public:
 std::array<CreatureContact,4> events{};
 unsigned count=0;
 void Reset() {*this={};}
 static bool Stance(authored::Asset asset,unsigned leg,float phase,float travel) {
   float offset=0,stance=.6f;
   if(asset==authored::Asset::Wolf) {
     const auto blend=authored::wolf::Blend(travel);
     const auto profile=[&](unsigned clip) {
       const float walk[]={0,.5f,.25f,.75f},trot[]={0,.5f,.5f,0},gallop[]={0,.12f,.53f,.65f};
       return std::pair<float,float>{clip==1?walk[leg]:clip==2?trot[leg]:gallop[leg],clip==1?.68f:clip==2?.54f:.34f};
     };
     auto a=profile(blend.first),b=profile(blend.second);
     float difference=std::remainder(b.first-a.first,1.f);
     offset=a.first+difference*blend.blend;stance=a.second+(b.second-a.second)*blend.blend;
   } else {
     // The boar's sinusoidal leg swing reaches forward at a quarter cycle.
     // Plant during its backward sweep, not while the hoof travels forward.
     offset=leg==0 || leg==3?.75f:.25f;stance=.5f;
   }
   const float cycle=phase/motion::tau+offset;
   return cycle-std::floor(cycle)<stance;
 }
 template<class Height> void Update(float dt,authored::Asset asset,rpg::EntityId id,rpg::Vec2 position,float yaw,
    float phase,float travel,float speed,const visual::BodyPalette& base,authored::Gesture& gesture,Height height) {
   count=0;if(!std::isfinite(dt)||dt<=0)return;
   const float c=std::cos(yaw),s=std::sin(yaw),ground=height(position);
   const bool wolf=asset==authored::Asset::Wolf;
   const float sole=wolf?.065f:.08f;
   const bool moving=speed>.10f;
   const bool jump=ready_ && std::hypot(position.x-previousPosition_.x,position.z-previousPosition_.z)>.85f;
   if(jump)Reset();
   std::array<DirectX::XMFLOAT3,4> points{};
   for(unsigned i=0;i<4;++i) {
     const auto paw=authored::quadruped::Paw(base,asset,i);
     points[i]={position.x+c*paw.x+s*paw.z,ground+paw.y,position.z-s*paw.x+c*paw.z};
   }
   if(!ready_) {
     for(unsigned i=0;i<4;++i) {
       auto& f=feet_[i];f.anchor=points[i];
       f.anchor.y=height({f.anchor.x,f.anchor.z})+sole;
       f.planted=!moving || Stance(asset,i,phase,travel);f.weight=0;
     }
     ready_=true;previousPosition_=position;
   }
   bool adjusting=false;for(const auto& f:feet_)adjusting|=f.step>=0;
   for(unsigned i=0;i<4;++i) {
     auto& f=feet_[i];const auto raw=points[i];
     bool planted=!moving || Stance(asset,i,phase,travel);
     const float error=std::hypot(raw.x-f.anchor.x,raw.z-f.anchor.z);
     if(!moving && !adjusting && error>.09f) {
       f.from=f.anchor;f.step=0;adjusting=true;
     }
     if(f.step>=0) {
       f.step=std::min(.16f,f.step+dt);const float t=f.step/.16f,u=t*t*(3-2*t);
       f.anchor={f.from.x+(raw.x-f.from.x)*u,0,f.from.z+(raw.z-f.from.z)*u};
       f.anchor.y=height({f.anchor.x,f.anchor.z})+sole+.045f*std::sin(3.14159265f*t);
       planted=true;
       if(f.step>=.16f)f.step=-1;
     } else if(planted && (!f.planted || error>(wolf?.23f:.14f))) {
       f.anchor=raw;f.anchor.y=height({raw.x,raw.z})+sole;
       // No event for spawn, idle settle, or a contact correction forced by a
       // long turn. Sound/dust only follows a new moving stance.
       if(!f.planted && moving)events[count++]={id,{f.anchor.x,f.anchor.y-sole,f.anchor.z},yaw,
         std::clamp(speed/(wolf?3.f:4.f),.15f,1.f),!wolf};
     }
     f.planted=planted;
     f.weight=motion::Ease(f.weight,planted?1.f:0.f,planted?38.f:30.f,dt);
     // During swing preserve the authored lift, adding only terrain height
     // residual beyond the body's fitted support plane.
     const float plane=ground+gesture.supportLift;
     const float lx=c*(raw.x-position.x)-s*(raw.z-position.z),lz=s*(raw.x-position.x)+c*(raw.z-position.z);
     const float floor=height({raw.x,raw.z});
     const float predicted=plane+std::tan(gesture.supportRoll)*lx-std::tan(gesture.supportPitch)*lz;
     float correction=std::clamp(floor-predicted,-.18f,.18f);
     if(!wolf && moving && !planted) {
       float cycle=phase/motion::tau+(i==0 || i==3?.75f:.25f);cycle-=std::floor(cycle);
       correction+=.055f*std::sin((cycle-.5f)*motion::tau)*travel;
     }
     DirectX::XMFLOAT3 target{raw.x+(f.anchor.x-raw.x)*f.weight,
       raw.y+correction+(f.anchor.y-raw.y-correction)*f.weight,
       raw.z+(f.anchor.z-raw.z)*f.weight};
     const float dx=target.x-position.x,dz=target.z-position.z;
     gesture.paws[i]={{c*dx-s*dz,target.y-ground,s*dx+c*dz},1};
   }
   previousPosition_=position;
 }
 bool Planted(unsigned leg) const {return ready_ && feet_.at(leg).planted;}
};
}
