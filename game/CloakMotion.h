#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace animation {
struct CloakSample {
  float midSide=0,midBack=0,hemSide=0,hemBack=0,flutter=0,phase=0,twist=0;
  bool active=false;
};
// A pinned, inextensible centre strip drives the rendered cape's middle and
// hem. The shoulder band is stiff; the remaining links retain their own
// velocity under drag, acceleration and turning. This is local presentation
// cloth, not a world collision or self-collision solver.
class CloakMotion {
  struct Point {float x=0,y=0,z=0;};
  static constexpr int links=6;
  static constexpr float length=.88f/links,step=1.f/240;
  std::array<Point,links+1> points_{},previous_{};
  float pending_=0,phase_=0,previousBodyYaw_=0,turn_=0,twist_=0,flutter_=0;
  bool ready_=false,bodyYawReady_=false;
  static float Norm(Point p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
  void ResetStrip(){for(int i=0;i<=links;++i)points_[i]=previous_[i]={0,-i*length,0};ready_=true;}
  void Integrate(float h,float speed,float vertical,float bank,float acceleration,float turnRate){
    turn_+=(turnRate-turn_)*(1-std::exp(-18*h));
    const float twistTarget=std::clamp(-.018f*turn_-.035f*bank,-.055f,.055f);
    twist_+=(twistTarget-twist_)*(1-std::exp(-9*h));
    phase_=std::fmod(phase_+h*(2.2f+speed*.75f),6.2831853f);
    const float damping=std::exp(-6.8f*h);
    const float drag=std::clamp(.11f*speed*speed+.32f*acceleration+.025f*vertical*vertical,0.f,7.f);
    const float side=-turn_*1.2f-bank*1.4f;
    for(int i=2;i<=links;++i){
      const auto p=points_[i],old=previous_[i];const float q=float(i)/links;
      previous_[i]=p;
      points_[i]={p.x+(p.x-old.x)*damping*(h/step)+side*(.35f+.65f*q)*h*h,
        p.y+(p.y-old.y)*damping*(h/step)-12.f*h*h,
        p.z+(p.z-old.z)*damping*(h/step)-drag*(.3f+.7f*q)*h*h};
    }
    for(int iteration=0;iteration<12;++iteration){
      points_[0]={};points_[1]={0,-length,0};
      for(int i=2;i<=links;++i){
        auto& a=points_[i-1];auto& b=points_[i];Point d{b.x-a.x,b.y-a.y,b.z-a.z};
        const float n=std::max(Norm(d),.00001f),error=(n-length)/n;
        const float wa=i==2?0.f:.5f,wb=1-wa;
        a.x+=d.x*error*wa;a.y+=d.y*error*wa;a.z+=d.z*error*wa;
        b.x-=d.x*error*wb;b.y-=d.y*error*wb;b.z-=d.z*error*wb;
        const float q=float(i)/links;
        b.x=std::clamp(b.x,-.20f*q*q,.20f*q*q);
        b.z=std::clamp(b.z,-.39f*q*q,0.f);
      }
    }
    const auto& hem=points_[links];const auto& old=previous_[links];
    const float velocity=Norm({hem.x-old.x,hem.y-old.y,hem.z-old.z})/h;
    const float target=.001f+.0035f*speed+.003f*std::abs(vertical)+.015f*std::min(velocity,1.f);
    flutter_+=(std::clamp(target,.001f,.04f)-flutter_)*(1-std::exp(-8*h));
  }
public:
  CloakSample Update(float dt,float speed,float vertical,float bank,float acceleration=0,
                     float bodyYaw=0,float rootYawRate=0){
    if(!std::isfinite(dt)||dt<=0)return {};
    dt=std::min(dt,.05f);if(!ready_)ResetStrip();
    speed=std::isfinite(speed)?std::clamp(speed,0.f,9.f):0;
    vertical=std::isfinite(vertical)?std::clamp(vertical,-10.f,10.f):0;
    bank=std::isfinite(bank)?std::clamp(bank,-1.f,1.f):0;
    acceleration=std::isfinite(acceleration)?std::clamp(acceleration,-15.f,15.f):0;
    bodyYaw=std::isfinite(bodyYaw)?bodyYaw:0;
    const float bodyRate=bodyYawReady_?std::remainder(bodyYaw-previousBodyYaw_,6.2831853f)/dt:0;
    previousBodyYaw_=bodyYaw;bodyYawReady_=true;
    rootYawRate=std::isfinite(rootYawRate)?rootYawRate:0;
    const float turnRate=std::clamp(bodyRate+rootYawRate,-8.f,8.f);
    pending_+=dt;
    while(pending_+1e-7f>=step){Integrate(step,speed,vertical,bank,acceleration,turnRate);pending_=std::max(0.f,pending_-step);}
    // Extrapolate only the unsimulated fraction in a disposable copy. Simulation
    // history remains on the fixed clock, independent of the rendering rate.
    auto visible=*this;if(pending_>1e-7f)visible.Integrate(pending_,speed,vertical,bank,acceleration,turnRate);
    const auto& mid=visible.points_[3];const auto& hem=visible.points_[links];
    return {mid.x,std::clamp(-mid.z,0.f,.16f),hem.x,std::clamp(-hem.z,0.f,.39f),
      std::clamp(visible.flutter_,.001f,.04f),visible.phase_,visible.twist_,true};
  }
  float MaximumStretch() const {
    float result=0;for(int i=1;i<=links;++i){const auto a=points_[i-1],b=points_[i];result=std::max(result,std::abs(Norm({b.x-a.x,b.y-a.y,b.z-a.z})-length));}return result;
  }
};
}
