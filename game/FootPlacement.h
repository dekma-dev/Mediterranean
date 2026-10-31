#pragma once
#include "game/Animation.h"
#include "game/WorldMap.h"

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
    }
    next_=0;
    support_=supportVelocity_=0;
  }
  void Update(float dt,rpg::Vec2 position,float yaw,bool grounded,bool alive,Sample& sample) {
    sample.feetActive=false;
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
    }
    auto support=support_,supportVelocity=supportVelocity_;
    motion::Spring(support,supportVelocity,std::clamp((visible[0].y-visible[1].y)/.10f,-1.f,1.f),20.f,pending_);
    sample.support=support;
  }
  bool Planted(int side) const { return ready_ && !feet_[side].stepping; }
private:
  struct Step {
    Foot point,from,goal;
    float age=0,duration=.16f,lift=.1f;
    bool stepping=false;
  };
  std::array<Step,2> feet_{};
  rpg::Vec2 previous_{};
  float heading_=0,pending_=0;
  float support_=0,supportVelocity_=0;
  int next_=0;
  bool ready_=false;
  static Foot Neutral(rpg::Vec2 position,float yaw,int side) {
    const float offset=side==0?-.18f:.18f;
    const float x=position.x+std::cos(yaw)*offset,z=position.z-std::sin(yaw)*offset;
    return {x,rpg::WorldMap::Height({x,z})+.17f,z,yaw};
  }
  static void AdvanceSwing(Step& foot,int i,float dt,rpg::Vec2 position,float yaw,rpg::Vec2 velocity,const Sample& sample) {
    foot.age=std::min(foot.duration,foot.age+dt);
    auto desired=Neutral(position,yaw,i);
    desired.x+=velocity.x*(foot.duration-foot.age+.035f);
    desired.z+=velocity.z*(foot.duration-foot.age+.035f);
    foot.goal.x=motion::Ease(foot.goal.x,desired.x,24,dt);
    foot.goal.z=motion::Ease(foot.goal.z,desired.z,24,dt);
    foot.goal.y=rpg::WorldMap::Height({foot.goal.x,foot.goal.z})+.17f;
    foot.goal.yaw=motion::FollowAngle(foot.goal.yaw,yaw,24,dt);
    const float t=foot.age/foot.duration,u=SmoothStep(t);
    float height=foot.lift*16*t*t*(1-t)*(1-t);
    if(sample.authoredGait) {
      float amount=0;for(float w:sample.gaitWeights)amount+=Unit(w);
      // Sample the imported recovery shape inside the accepted step interval.
      height=height*(1-Unit(amount))+playerclips::SwingHeight(sample.gaitWeights,sample.runBlend,i,t);
    }
    foot.point={foot.from.x+(foot.goal.x-foot.from.x)*u,foot.from.y+(foot.goal.y-foot.from.y)*u+height,
      foot.from.z+(foot.goal.z-foot.from.z)*u,foot.from.yaw+motion::AngleDelta(foot.from.yaw,foot.goal.yaw)*u};
    if(foot.age>=foot.duration){foot.point=foot.goal;foot.stepping=false;}
  }
  void Tick(float dt,rpg::Vec2 position,float yaw,rpg::Vec2 velocity,const Sample& sample) {
    const float speed=std::hypot(velocity.x,velocity.z);
    // Alternate ordinary steps. A supporting foot may release early during a
    // fast run/reversal to keep the leg reachable instead of sliding its contact.
    for (int order=0;order<2;++order) {
      const int i=(next_+order)%2;
      auto& foot=feet_[i];
      if (foot.stepping) continue;
      const auto neutral=Neutral(position,yaw,i);
      const float error=std::hypot(foot.point.x-neutral.x,foot.point.z-neutral.z);
      const float angle=std::abs(motion::AngleDelta(foot.point.yaw,yaw));
      if ((error>(speed>.1f?.16f:.065f) || angle>.48f) &&
          (!feet_[1-i].stepping || error>.36f || angle>.95f)) {
        foot.stepping=true; foot.from=foot.point; foot.age=0;
        foot.duration=std::clamp(.50f/std::max(2.8f,speed),.075f,.18f);
        foot.lift=speed>.1f?.10f:.055f;
        foot.goal=neutral;
        foot.goal.x+=velocity.x*(foot.duration+.035f); foot.goal.z+=velocity.z*(foot.duration+.035f);
        next_=1-i;
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
  }
};
}
