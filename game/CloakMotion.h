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
  Mode midSide_,midBack_,hemSide_,hemBack_,flow_,twist_,turn_;
  float phase_=0,previousBodyYaw_=0;
  bool bodyYawReady_=false;
  static void Advance(Mode& m,float target,float frequency,float damping,float dt) {
    const float a=frequency*damping,w=frequency*std::sqrt(1-damping*damping);
    const float x=m.position-target,c=std::cos(w*dt),s=std::sin(w*dt),e=std::exp(-a*dt);
    const float v=m.velocity;
    m.position=target+e*(x*c+(v+a*x)*s/w);
    m.velocity=e*(v*c-(a*v+frequency*frequency*x)*s/w);
  }
public:
  CloakSample Update(float dt,float speed,float vertical,float bank,float acceleration=0,
                     float bodyYaw=0,float rootYawRate=0) {
    if(!std::isfinite(dt)||dt<=0)return {};
    dt=std::min(dt,.05f);
    speed=std::isfinite(speed)?std::clamp(speed,0.f,9.f):0;
    vertical=std::isfinite(vertical)?std::clamp(vertical,-10.f,10.f):0;
    bank=std::isfinite(bank)?std::clamp(bank,-1.f,1.f):0;
    acceleration=std::isfinite(acceleration)?std::clamp(acceleration,-15.f,15.f):0;
    bodyYaw=std::isfinite(bodyYaw)?bodyYaw:0;
    const float bodyRate=bodyYawReady_?std::remainder(bodyYaw-previousBodyYaw_,6.2831853f)/dt:0;
    previousBodyYaw_=bodyYaw;bodyYawReady_=true;
    rootYawRate=std::isfinite(rootYawRate)?rootYawRate:0;
    const float turnRate=std::clamp(bodyRate+rootYawRate,-8.f,8.f);
    const int steps=std::max(1,int(std::ceil(dt*240)));const float h=dt/steps;
    for(int i=0;i<steps;++i) {
      Advance(flow_,speed/7,11,.82f,h);
      Advance(turn_,turnRate,18,.82f,h);
      // Drag follows speed; acceleration supplies a separate inertial load.
      // The filtered flow velocity also gives legacy callers a bounded start/
      // brake response without differentiating noisy frame input directly.
      const float load=std::clamp(.025f+flow_.position*.24f+vertical*.006f+
        flow_.velocity*.018f+acceleration*.003f,.005f,.34f);
      // Angular momentum comes from the moving shoulders as well as facing.
      // A standing cut therefore loads the cloth without inventing foot speed.
      const float side=-bank*.13f-turn_.position*.035f;
      Advance(midBack_,load*.20f,13,.67f,h);
      Advance(hemBack_,load*.48f+midBack_.position*2.0f,8,.48f,h);
      Advance(midSide_,side*.34f,11,.65f,h);
      Advance(hemSide_,side*.3f+midSide_.position*1.9f,7,.48f,h);
      Advance(twist_,-bank*.035f-turn_.position*.012f,8,.58f,h);
      phase_+=h*(2.4f+std::clamp(flow_.position,0.f,1.f)*5.2f);
      if(phase_>6.2831853f)phase_-=6.2831853f;
    }
    return {std::clamp(midSide_.position,-.10f,.10f),std::clamp(midBack_.position,0.f,.16f),
      std::clamp(hemSide_.position,-.20f,.20f),std::clamp(hemBack_.position,0.f,.39f),
      std::clamp(.001f+.018f*flow_.position+.0015f*std::abs(flow_.velocity),.001f,.04f),
      phase_,std::clamp(twist_.position,-.055f,.055f),true};
  }
};
}
