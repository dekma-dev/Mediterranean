#pragma once
#include <algorithm>
#include <cmath>

namespace animation {
struct CloakSample {
  float midSide=0,midBack=0,hemSide=0,hemBack=0,flutter=0,phase=0,twist=0;
  bool active=false;
};
// Two damped modes transmit the shoulder load toward the free hem. This is
// a small presentation model, not a general cloth/contact simulation.
class CloakMotion {
  struct Mode {float position=0,velocity=0;};
  Mode midSide_,midBack_,hemSide_,hemBack_,flow_,twist_;
  float phase_=0;
  static void Advance(Mode& m,float target,float frequency,float damping,float dt) {
    const float a=frequency*damping,w=frequency*std::sqrt(1-damping*damping);
    const float x=m.position-target,c=std::cos(w*dt),s=std::sin(w*dt),e=std::exp(-a*dt);
    const float v=m.velocity;
    m.position=target+e*(x*c+(v+a*x)*s/w);
    m.velocity=e*(v*c-(a*v+frequency*frequency*x)*s/w);
  }
public:
  CloakSample Update(float dt,float speed,float vertical,float bank) {
    if(!std::isfinite(dt)||dt<=0)return {};
    dt=std::min(dt,.05f);
    speed=std::isfinite(speed)?std::clamp(speed,0.f,9.f):0;
    vertical=std::isfinite(vertical)?std::clamp(vertical,-10.f,10.f):0;
    bank=std::isfinite(bank)?std::clamp(bank,-1.f,1.f):0;
    const int steps=std::max(1,int(std::ceil(dt*240)));const float h=dt/steps;
    for(int i=0;i<steps;++i) {
      Advance(flow_,speed/7,11,.82f,h);
      const float load=std::clamp(.025f+flow_.position*.24f+vertical*.008f,.005f,.34f);
      const float side=-bank*.13f;
      Advance(midBack_,load*.20f,13,.67f,h);
      Advance(hemBack_,load*.48f+midBack_.position*2.0f,8,.48f,h);
      Advance(midSide_,side*.34f,11,.65f,h);
      Advance(hemSide_,side*.3f+midSide_.position*1.9f,7,.48f,h);
      Advance(twist_,-bank*.035f,8,.58f,h);
      phase_+=h*(3.2f+std::clamp(flow_.position,0.f,1.f)*7.f);
      if(phase_>6.2831853f)phase_-=6.2831853f;
    }
    return {std::clamp(midSide_.position,-.10f,.10f),std::clamp(midBack_.position,0.f,.16f),
      std::clamp(hemSide_.position,-.20f,.20f),std::clamp(hemBack_.position,0.f,.39f),
      .002f+.036f*std::clamp(flow_.position,0.f,1.f),phase_,std::clamp(twist_.position,-.055f,.055f),true};
  }
};
}
