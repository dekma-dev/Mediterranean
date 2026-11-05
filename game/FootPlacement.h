#pragma once
#include "game/Animation.h"
#include "game/WorldMap.h"
#include "game/GaitTiming.h"
#include "game/CharacterPose.h"

namespace animation {
// Heightfield presentation controller. Contacts are stored in world space and
// only change during a step; turning the body does not rotate a planted boot.
// No raycasts, simulation writes, allocations or render-time updates.
class FootPlacement {
public:
  void Reset(rpg::Vec2 position,float yaw) {
    previous_=position; heading_=yaw; pending_=0; ready_=true;
    for (int i=0;i<2;++i) {
      feet_[i]={}; feet_[i].point=Neutral(position,yaw,i); feet_[i].point.yaw=yaw;
      feet_[i].goal=feet_[i].point;
    }
    next_=0;firstStep_=true;
    support_=supportVelocity_=0;
    hip_=hipVelocity_=0;hasHip_=false;
  }
  void Update(float dt,rpg::Vec2 position,float yaw,bool grounded,bool alive,Sample& sample) {
    sample.feetActive=false;
    sample.pelvisStabilized=false;
    sample.support=0;
    if (!std::isfinite(dt) || dt<=0 || !std::isfinite(position.x) || !std::isfinite(position.z) || !std::isfinite(yaw)) return;
    dt=std::min(dt,.05f);
    if (!ready_ || !grounded || !alive || std::hypot(position.x-previous_.x,position.z-previous_.z)>.85f) {
      Reset(position,yaw);
      if (!grounded || !alive) { ready_=false; return; }
    }
    const rpg::Vec2 start=previous_,velocity{(position.x-start.x)/dt,(position.z-start.z)/dt};
    const float startYaw=heading_,turn=motion::AngleDelta(startYaw,yaw);
    // Fixed contact decisions avoid different left/right step ordering at
    // different frame rates. Interpolate the actor path at each 240 Hz tick;
    // Contact decisions stay fixed; swing poses below extrapolate the remainder
    // so high-refresh rendering does not staircase at this decision frequency.
    constexpr float interval=1.f/240;
    const int steps=int((pending_+dt+1e-7f)/interval);
    for (int step=0;step<steps;++step) {
      const float t=std::clamp((interval-pending_+step*interval)/dt,0.f,1.f);
      Tick(interval,{start.x+(position.x-start.x)*t,start.z+(position.z-start.z)*t},startYaw+turn*t,velocity,sample);
    }
    pending_=std::max(0.f,pending_+dt-steps*interval);
    previous_=position; heading_=yaw;
    sample.feetActive=true;
    const float c=std::cos(yaw),s=std::sin(yaw);
    std::array<Foot,2> visible{};
    for (int i=0;i<2;++i) {
      auto step=feet_[i];
      if(step.stepping&&pending_>0) AdvanceSwing(step,i,pending_,position,yaw,velocity,sample);
      const auto& p=step.point;visible[i]=p;
      const float dx=p.x-position.x,dz=p.z-position.z;
      sample.feet[i]={c*dx-s*dz,p.y-rpg::WorldMap::Height(position),s*dx+c*dz,motion::AngleDelta(yaw,p.yaw)};
      sample.feet[i].pitch=p.pitch;
    }
    auto support=support_,supportVelocity=supportVelocity_;
    motion::Spring(support,supportVelocity,std::clamp((visible[0].y-visible[1].y)/.10f,-1.f,1.f),20.f,pending_);
    sample.support=support;
    if(sample.authoredGait) {
      const auto form=sample.authoredEquipment==playerclips::Equipment::Staff?rpg::WeaponForm::AstralStaff:
        sample.authoredEquipment==playerclips::Equipment::Bow?rpg::WeaponForm::Handbow:rpg::WeaponForm::Blade;
      const float limit=BuildCharacterPose(0,0,false,sample,form).bodyLift;
      if(!hasHip_){hip_=limit;hipVelocity_=0;hasHip_=true;}
      else {
        motion::Spring(hip_,hipVelocity_,limit,18,dt);
        // Reach is a hard constraint; rising back from compression has mass.
        // A swing goal changing at a stop must not instantly lift the torso.
        if(hip_>limit){hip_=limit;hipVelocity_=std::min(0.f,hipVelocity_);}
      }
      sample.pelvisLimit=hip_;sample.pelvisStabilized=true;
    }
  }
  bool Planted(int side) const { return ready_ && !feet_[side].stepping; }
private:
  struct Step {
    Foot point,from,goal;
    rpg::Vec2 root{},velocity{};
    float age=0,duration=.16f,lift=.1f;
    bool stepping=false;
  };
  std::array<Step,2> feet_{};
  rpg::Vec2 previous_{};
  float heading_=0,pending_=0;
  float support_=0,supportVelocity_=0;
  float hip_=0,hipVelocity_=0;
  bool hasHip_=false;
  int next_=0;
  bool ready_=false;
  bool firstStep_=true;
  static Foot Neutral(rpg::Vec2 position,float yaw,int side) {
    const float offset=side==0?-.18f:.18f;
    const float x=position.x+std::cos(yaw)*offset,z=position.z-std::sin(yaw)*offset;
    return {x,rpg::WorldMap::Height({x,z})+.17f,z,yaw};
  }
  static void AdvanceSwing(Step& foot,int i,float dt,rpg::Vec2 position,float yaw,rpg::Vec2 velocity,const Sample& sample) {
    foot.age=std::min(foot.duration,foot.age+dt);
    auto desired=Neutral(position,yaw,i);
    const float speed=std::hypot(velocity.x,velocity.z);
    const float lead=.82f*TravelGait(speed).reach/std::max(speed,.7f);
    desired.x+=velocity.x*lead;
    desired.z+=velocity.z*lead;
    // Feed forward actor travel; otherwise a 24 Hz position filter lands the
    // foot almost 30 cm behind its intended contact at sprint speed.
    foot.goal.x=motion::Ease(foot.goal.x+velocity.x*dt,desired.x,24,dt);
    foot.goal.z=motion::Ease(foot.goal.z+velocity.z*dt,desired.z,24,dt);
    foot.velocity.x=motion::Ease(foot.velocity.x,velocity.x,18,dt);
    foot.velocity.z=motion::Ease(foot.velocity.z,velocity.z,18,dt);
    foot.goal.y=rpg::WorldMap::Height({foot.goal.x,foot.goal.z})+.17f;
    foot.goal.yaw=motion::FollowAngle(foot.goal.yaw,yaw,24,dt);
    const float t=foot.age/foot.duration;
    // Recover relative to the moving pelvis. World-space interpolation between
    // a fixed takeoff and a moving goal left the foot far behind the body.
    // Matching -actor velocity at both relative endpoints gives stationary
    // world-space takeoff/contact, with retraction before the landing.
    const float u=t*t*(3-2*t);
    const float tangent=foot.duration*(-(2*t*t*t-3*t*t+t)+.22f*std::sin(t*motion::tau)*std::sin(t*motion::tau*.5f));
    float height=foot.lift*16*t*t*(1-t)*(1-t);
    if(sample.authoredGait) {
      float amount=0;for(float w:sample.gaitWeights)amount+=Unit(w);
      // Sample the imported recovery shape inside the accepted step interval.
      height=height*(1-Unit(amount))+playerclips::SwingHeight(sample.gaitWeights,sample.runBlend,i,t);
    }
    foot.point={foot.from.x+(position.x-foot.root.x)*(1-u)+(foot.goal.x-foot.from.x)*u+foot.velocity.x*tangent,
      foot.from.y+(foot.goal.y-foot.from.y)*u+height,
      foot.from.z+(position.z-foot.root.z)*(1-u)+(foot.goal.z-foot.from.z)*u+foot.velocity.z*tangent,
      foot.from.yaw+motion::AngleDelta(foot.from.yaw,foot.goal.yaw)*u};
    // Toe clearance through recovery, followed by a restrained heel approach.
    // Flat, locked contacts remain unchanged at the two endpoints.
    foot.point.pitch=foot.from.pitch*(1-u)+.24f*std::sin(t*motion::tau)*std::sin(t*motion::tau*.5f);
    // The ankle must also clear the rotated sole's front and heel corners.
    const float floor=rpg::WorldMap::Height({foot.point.x,foot.point.z});
    foot.point.y=std::max(foot.point.y,floor+.17f*std::cos(foot.point.pitch)+.185f*std::abs(std::sin(foot.point.pitch)));
    if(foot.age>=foot.duration){foot.point=foot.goal;foot.stepping=false;}
  }
  static void RollSupport(Step& foot,float dt,rpg::Vec2 position,rpg::Vec2 velocity) {
    // The toe remains fixed in world space while the heel rises. Storing the
    // flat contact separately also lets a stopped or interrupted step settle
    // without either sliding the sole or snapping the ankle to its flat pose.
    const float forward=velocity.x*std::sin(foot.goal.yaw)+velocity.z*std::cos(foot.goal.yaw);
    const float behind=(position.x-foot.goal.x)*std::sin(foot.goal.yaw)+(position.z-foot.goal.z)*std::cos(foot.goal.yaw);
    const float target=.62f*SmoothStep((behind-.08f)/.34f)*Unit(forward/.8f);
    const float pitch=motion::Ease(foot.point.pitch,target,24,dt),s=std::sin(pitch),c=std::cos(pitch);
    const float dz=.185f*(1-c)+.17f*s;
    foot.point=foot.goal;foot.point.pitch=pitch;
    foot.point.x+=std::sin(foot.goal.yaw)*dz;foot.point.z+=std::cos(foot.goal.yaw)*dz;
    foot.point.y+=.185f*s-.17f*(1-c);
  }
  void Tick(float dt,rpg::Vec2 position,float yaw,rpg::Vec2 velocity,const Sample& sample) {
    const float speed=std::hypot(velocity.x,velocity.z);
    for(auto& foot:feet_)if(!foot.stepping)RollSupport(foot,dt,position,velocity);
    // Alternate ordinary steps. A supporting foot may release early during a
    // fast run/reversal to keep the leg reachable instead of sliding its contact.
    for (int order=0;order<2;++order) {
      const int i=(next_+order)%2;
      auto& foot=feet_[i];
      if (foot.stepping) continue;
      const auto neutral=Neutral(position,yaw,i);
      const float error=std::hypot(foot.goal.x-neutral.x,foot.goal.z-neutral.z);
      const float angle=std::abs(motion::AngleDelta(foot.point.yaw,yaw));
      const auto timing=TravelGait(speed);
      const float behind=speed>.1f?((neutral.x-foot.goal.x)*velocity.x+(neutral.z-foot.goal.z)*velocity.z)/speed:error;
      const float release=firstStep_&&i==next_?.065f:timing.reach;
      if ((behind>(speed>.1f?release:.065f) || (speed<=.1f&&error>.065f) || angle>.48f) &&
          (!feet_[1-i].stepping || behind>timing.reach+.015f || angle>.95f)) {
        foot.stepping=true; foot.from=foot.point; foot.age=0;
        foot.root={position.x-velocity.x*dt,position.z-velocity.z*dt};foot.velocity=velocity;
        foot.duration=speed>.1f?timing.swing:.28f;
        if(firstStep_&&speed>.1f)
          foot.duration=std::max(.12f,timing.period*(.5f-timing.stance*.5f)-.065f/speed);
        foot.lift=speed>.1f?.10f:.055f;
        foot.goal=neutral;
        // Land closer to the centre of mass; heel rise supplies the longer
        // rear part of support instead of overstriding and squatting at contact.
        const float lead=.82f*timing.reach/std::max(speed,.7f);
        foot.goal.x+=velocity.x*lead; foot.goal.z+=velocity.z*lead;
        next_=1-i;
        firstStep_=false;
        break;
      }
    }
    for (int i=0;i<2;++i) {
      auto& foot=feet_[i];
      if (!foot.stepping) continue;
      AdvanceSwing(foot,i,dt,position,yaw,velocity,sample);
    }
    // Shift weight toward the planted leg, using actual swing height rather
    // than an unrelated sine cycle. The spring avoids a snap at foot exchange.
    const float support=std::clamp((feet_[0].point.y-feet_[1].point.y)/.10f,-1.f,1.f);
    motion::Spring(support_,supportVelocity_,support,20.f,dt);
    if(speed<.05f&&!feet_[0].stepping&&!feet_[1].stepping) {
      bool settled=true;
      for(int i=0;i<2;++i) {
        const auto neutral=Neutral(position,yaw,i);
        settled&=std::hypot(feet_[i].point.x-neutral.x,feet_[i].point.z-neutral.z)<.065f&&
          std::abs(motion::AngleDelta(feet_[i].point.yaw,yaw))<.48f;
      }
      if(settled)firstStep_=true;
    }
  }
};
}
