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
    phaseReady_=false;shownPhase_=0;landingOffset_=0;landingAge_=1;
  }
  void Update(float dt,rpg::Vec2 position,float yaw,bool grounded,bool alive,Sample& sample) {
    sample.feetActive=false;
    sample.pelvisStabilized=false;
    sample.support=0;
    if (!std::isfinite(dt) || dt<=0 || !std::isfinite(position.x) || !std::isfinite(position.z) || !std::isfinite(yaw)) return;
    dt=std::min(dt,.05f);
    landingAge_=std::min(1.f,landingAge_+dt);
    if(!alive){Reset(position,yaw);ready_=false;return;}
    if(!grounded){
      // Release support immediately, but retain the departing gait phase while
      // the airborne layer blends in. Resetting it changed the arms by almost
      // half a cycle on the very first airborne frame.
      if(sample.authoredGait&&phaseReady_){phase_=shownPhase_;pending_=0;Retime(sample,phase_,landingOffset_,SmoothStep(landingAge_/.18f));}
      previous_=position;heading_=yaw;ready_=false;return;
    }
    if(!ready_){
      const bool retained=phaseReady_;const float phase=phase_;
      Reset(position,yaw);
      phaseReady_=retained;phase_=shownPhase_=phase;
      if(retained&&sample.authoredGait){
        // Resume on a contact, not in the middle of two recovering legs. The
        // upper body crossfades between the two phases instead of reversing
        // or snapping its arms when the feet establish the landing support.
        const float contact=std::ceil(phase*2-1e-6f)*.5f;
        landingOffset_=std::max(0.f,contact-phase);landingAge_=0;
        phase_=shownPhase_=contact-std::floor(contact);
      }
    } else if(std::hypot(position.x-previous_.x,position.z-previous_.z)>.85f)Reset(position,yaw);
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
    std::array<float,2> mobility{};
    std::array<float,2> reachWeight{};
    for(int i=0;i<2;++i){
      auto step=feet_[i];
      if(step.stepping&&pending_>0)AdvanceSwing(step,i,pending_,position,yaw,velocity,sample);
      visible[i]=step.point;
      const float recovery=step.age/std::max(.001f,step.duration);
      mobility[i]=step.stepping?SmoothStep(recovery/.4f)*(1-SmoothStep(recovery)):0;
      reachWeight[i]=!sample.authoredGait||!step.stepping?1.f:SmoothStep((recovery-.88f)/.12f);
    }
    if(sample.authoredGait){
      // The lateral recovery path must pass beside the supporting leg. At
      // full strafe speed, independent endpoint interpolation sent the legs
      // through each other. Move only recovering feet; support stays anchored.
      const float gap=c*(visible[1].x-visible[0].x)-s*(visible[1].z-visible[0].z);
      const float overlap=std::max(0.f,.14f-gap);
      const float correction=overlap*overlap/(overlap+.025f);
      const float free=mobility[0]+mobility[1];
      if(free>.000001f)for(int i=0;i<2;++i)if(mobility[i]>0){
        const float shift=(i?1.f:-1.f)*correction*mobility[i]/free;
        visible[i].x+=c*shift;visible[i].z-=s*shift;
        const float pitch=visible[i].pitch;
        visible[i].y=std::max(visible[i].y,rpg::WorldMap::Height({visible[i].x,visible[i].z})+
          .17f*std::cos(pitch)+.185f*std::abs(std::sin(pitch)));
      }
    }
    for (int i=0;i<2;++i) {
      const auto& p=visible[i];
      const float dx=p.x-position.x,dz=p.z-position.z;
      sample.feet[i]={c*dx-s*dz,p.y-rpg::WorldMap::Height(position),s*dx+c*dz,motion::AngleDelta(yaw,p.yaw)};
      sample.feet[i].pitch=p.pitch;
      sample.feetReachWeight[i]=reachWeight[i];
    }
    auto support=support_,supportVelocity=supportVelocity_;
    motion::Spring(support,supportVelocity,std::clamp((visible[0].y-visible[1].y)/.10f,-1.f,1.f),20.f,pending_);
    sample.support=support;
    if(sample.authoredGait) {
      // The authored controller advances one clock for both legs and the
      // torso. Recovering phase from only the left contact made the clock
      // accelerate through stance and even run backwards after landing.
      const float speed=std::hypot(velocity.x,velocity.z);
      if(phaseReady_){
        const float next=phase_+(speed>.1f?pending_/TravelGait(speed).period:0);
        shownPhase_+=std::max(0.f,std::remainder(next-shownPhase_,1.f));
        shownPhase_-=std::floor(shownPhase_);Retime(sample,shownPhase_,landingOffset_,SmoothStep(landingAge_/.18f));
      }
      const auto form=sample.authoredEquipment==playerclips::Equipment::Staff?rpg::WeaponForm::AstralStaff:
        sample.authoredEquipment==playerclips::Equipment::Bow?rpg::WeaponForm::Handbow:rpg::WeaponForm::Blade;
      const auto solved=BuildCharacterPose(0,0,false,sample,form);
      // Filter only the correction imposed by reach. Filtering the entire
      // authored pelvis curve erased its rise during the running flight phase.
      const float limit=solved.bodyLift-solved.unconstrainedLift;
      if(!hasHip_){hip_=limit;hipVelocity_=0;hasHip_=true;}
      else {
        motion::Spring(hip_,hipVelocity_,limit,35,dt);
        // Reach is a hard constraint; rising back from compression has mass.
        // A swing goal changing at a stop must not instantly lift the torso.
        if(hip_>limit){hip_=limit;hipVelocity_=std::min(0.f,hipVelocity_);}
      }
      sample.pelvisLimit=solved.unconstrainedLift+hip_;sample.pelvisStabilized=true;
    }
  }
  bool Planted(int side) const { return ready_ && !feet_[side].stepping; }
private:
  static void Retime(Sample& sample,float phase,float offset=0,float landingBlend=1){
    const auto old=playerclips::Locomotion(sample.gaitWeights,sample.runBlend,sample.authoredPhase,sample.authoredEquipment);
    auto corrected=playerclips::Locomotion(sample.gaitWeights,sample.runBlend,phase,sample.authoredEquipment);
    if(landingBlend<1)corrected=playerclips::Blend(
      playerclips::Locomotion(sample.gaitWeights,sample.runBlend,phase-offset,sample.authoredEquipment),corrected,landingBlend);
    // Keep the additive start/stop deviation; replace only the looping layer.
    corrected.hip+=sample.authoredMotion.hip-old.hip;
    corrected.torso.x+=sample.authoredMotion.torso.x-old.torso.x;
    corrected.torso.z+=sample.authoredMotion.torso.z-old.torso.z;
    for(unsigned i=0;i<2;++i){
      corrected.hands[i].x+=sample.authoredMotion.hands[i].x-old.hands[i].x;
      corrected.hands[i].y+=sample.authoredMotion.hands[i].y-old.hands[i].y;
      corrected.hands[i].z+=sample.authoredMotion.hands[i].z-old.hands[i].z;
    }
    sample.authoredPhase=phase;sample.authoredMotion=corrected;
  }
  struct Step {
    Foot point,from,goal;
    rpg::Vec2 root{},velocity{};
    float age=0,duration=.16f,lift=.1f;
    float sourcePhase=0;
    bool stepping=false;
  };
  std::array<Step,2> feet_{};
  rpg::Vec2 previous_{};
  float heading_=0,pending_=0;
  float support_=0,supportVelocity_=0;
  float hip_=0,hipVelocity_=0;
  bool hasHip_=false;
  float phase_=0,shownPhase_=0;
  float landingOffset_=0,landingAge_=1;
  bool phaseReady_=false;
  int next_=0;
  bool ready_=false;
  bool firstStep_=true;
  static Foot Neutral(rpg::Vec2 position,float yaw,int side,float width=.18f) {
    const float offset=side==0?-width:width;
    const float x=position.x+std::cos(yaw)*offset,z=position.z-std::sin(yaw)*offset;
    return {x,rpg::WorldMap::Height({x,z})+.17f,z,yaw};
  }
  static float ContactLead(const Sample& sample) {
    // Fast forward travel lands close to the centre of mass. Extending the
    // recovery to the walking lead held the pelvis below its authored height.
    return .58f-.16f*Unit(sample.runBlend)*Unit(sample.gaitWeights[0]);
  }
  static float AuthoredDuty(const Sample& sample) {
    const auto& tracks=playerclips::Collection();
    const float walk=tracks[unsigned(playerclips::Clip::WalkForward)].stance;
    const float run=tracks[unsigned(playerclips::Clip::RunForward)].stance;
    return walk+(run-walk)*Unit(sample.runBlend);
  }
  void AdvanceSwing(Step& foot,int i,float dt,rpg::Vec2 position,float yaw,rpg::Vec2 velocity,const Sample& sample) {
    foot.age=std::min(foot.duration,foot.age+dt);
    const float width=.18f-.055f*Unit(sample.runBlend)*Unit(sample.gaitWeights[0]);
    auto desired=Neutral(position,yaw,i,width);
    // Carry the landing prediction through abrupt input changes. A raw zero
    // velocity removed the entire lead inside one render extrapolation and
    // could move a recovering ankle several centimetres in a millisecond.
    foot.velocity.x=motion::Ease(foot.velocity.x,velocity.x,18,dt);
    foot.velocity.z=motion::Ease(foot.velocity.z,velocity.z,18,dt);
    const float speed=std::hypot(foot.velocity.x,foot.velocity.z);
    const auto timing=TravelGait(speed);
    const float lead=sample.authoredGait?timing.period*AuthoredDuty(sample)*.35f:
      ContactLead(sample)*timing.reach/std::max(speed,.7f);
    desired.x+=foot.velocity.x*lead;
    desired.z+=foot.velocity.z*lead;
    auto goalVelocity=velocity;
    const auto& other=feet_[1-i];
    const float supportWeight=other.stepping?1-SmoothStep(other.age/.12f):1;
    if(sample.authoredGait&&supportWeight>0){
      const auto& support=other.stepping?other.from:other.point;
      const float c=std::cos(yaw),s=std::sin(yaw),side=i?1.f:-1.f;
      const float separation=side*(c*(desired.x-support.x)-s*(desired.z-support.z));
      if(separation<.14f){
        const float shift=side*(.14f-separation)*supportWeight;
        desired.x+=c*shift;desired.z-=s*shift;
        // This bound belongs to a stationary support anchor. Feeding actor
        // velocity into it would push the landing goal through the other foot.
        const float across=c*goalVelocity.x-s*goalVelocity.z;
        goalVelocity.x-=c*across*supportWeight;goalVelocity.z+=s*across*supportWeight;
      }
    }
    // Feed forward actor travel; otherwise a 24 Hz position filter lands the
    // foot almost 30 cm behind its intended contact at sprint speed.
    foot.goal.x=motion::Ease(foot.goal.x+goalVelocity.x*dt,desired.x,24,dt);
    foot.goal.z=motion::Ease(foot.goal.z+goalVelocity.z*dt,desired.z,24,dt);
    foot.goal.y=rpg::WorldMap::Height({foot.goal.x,foot.goal.z})+.17f;
    foot.goal.yaw=motion::FollowAngle(foot.goal.yaw,yaw,24,dt);
    const float t=foot.age/foot.duration;
    // Recover relative to the moving pelvis. World-space interpolation between
    // a fixed takeoff and a moving goal left the foot far behind the body.
    // Matching -actor velocity at both relative endpoints gives stationary
    // world-space takeoff/contact, with retraction before the landing.
    const float u=sample.authoredGait?playerclips::SwingProgress(sample.gaitWeights,sample.runBlend,i,t):t*t*(3-2*t);
    // Endpoint velocity compensation belongs near release and touchdown.
    // Applying a stride-long tangent throughout recovery swept the ankle far
    // behind its reach sphere and drove the pelvis into a permanent crouch.
    const float tangent=foot.duration*t*(1-t)*
      (-std::exp(-t/.075f)+std::exp(-(1-t)/.075f));
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
    foot.point.pitch=sample.authoredGait?playerclips::SwingPitch(sample.gaitWeights,sample.runBlend,i,t,foot.from.pitch):
      foot.from.pitch*(1-u)+.24f*std::sin(t*motion::tau)*std::sin(t*motion::tau*.5f);
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
    const bool recovering=(feet_[0].stepping&&feet_[0].sourcePhase>=0)||(feet_[1].stepping&&feet_[1].sourcePhase>=0);
    if(sample.authoredGait&&(speed>.1f||recovering)) {
      TickAuthored(dt,position,yaw,velocity,sample,speed);return;
    }
    for(auto& foot:feet_)if(!foot.stepping)RollSupport(foot,dt,position,velocity);
    // Alternate ordinary steps. A supporting foot may release early during a
    // fast run/reversal to keep the leg reachable instead of sliding its contact.
    for (int order=0;order<2;++order) {
      const int i=(next_+order)%2;
      auto& foot=feet_[i];
      if (foot.stepping) continue;
      const float width=.18f-.055f*Unit(sample.runBlend)*Unit(sample.gaitWeights[0]);
      const auto neutral=Neutral(position,yaw,i,width);
      const float error=std::hypot(foot.goal.x-neutral.x,foot.goal.z-neutral.z);
      const float angle=std::abs(motion::AngleDelta(foot.point.yaw,yaw));
      const auto timing=TravelGait(speed);
      const float behind=speed>.1f?((neutral.x-foot.goal.x)*velocity.x+(neutral.z-foot.goal.z)*velocity.z)/speed:error;
      const float release=firstStep_&&i==next_?.065f:timing.reach;
      if ((behind>(speed>.1f?release:.065f) || (speed<=.1f&&error>.065f) || angle>.48f) &&
          (!feet_[1-i].stepping || behind>timing.reach+.015f || angle>.95f)) {
        foot.stepping=true; foot.from=foot.point; foot.age=0;
        foot.sourcePhase=-1;
        foot.root={position.x-velocity.x*dt,position.z-velocity.z*dt};foot.velocity=velocity;
        foot.duration=speed>.1f?timing.swing:.28f;
        if(firstStep_&&speed>.1f)
          foot.duration=std::max(.12f,timing.period*(.5f-timing.stance*.5f)-.065f/speed);
        foot.lift=speed>.1f?.10f:.055f;
        foot.goal=neutral;
        // Land closer to the centre of mass; heel rise supplies the longer
        // rear part of support instead of overstriding and squatting at contact.
        const float lead=ContactLead(sample)*timing.reach/std::max(speed,.7f);
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
  void TickAuthored(float dt,rpg::Vec2 position,float yaw,rpg::Vec2 velocity,const Sample& sample,float speed) {
    auto timing=TravelGait(speed);const float duty=AuthoredDuty(sample);
    if(speed<=.1f)for(const auto& foot:feet_)if(foot.stepping&&foot.sourcePhase>=0){
      timing.period=foot.duration/std::max(.001f,1-foot.sourcePhase);break;
    }
    if(!phaseReady_){phase_=0;phaseReady_=true;}
    const float previousPhase=phase_;
    phase_+=dt/timing.period;phase_-=std::floor(phase_);
    for(int i=0;i<2;++i) {
      auto& foot=feet_[i];const float q=phase_+i*.5f-std::floor(phase_+i*.5f);
      const float previousQ=previousPhase+i*.5f-std::floor(previousPhase+i*.5f);
      const bool completing=foot.stepping&&foot.sourcePhase>=0&&q<previousQ;
      if(foot.stepping&&foot.sourcePhase<0){
        foot.from=foot.point;foot.root=position;foot.velocity=velocity;foot.sourcePhase=q;
        foot.duration=std::max(.04f,(1-q)*timing.period);foot.age=0;
      }
      if(completing) {
          // Compare consecutive clock samples, not the original release phase.
          // A release just after zero can be skipped by the next wrapped tick;
          // resetting its progress then teleported the foot to its old origin.
          // Finish at the live landing anchor; subsequent support never moves
          // that anchor. Both legs use the same cycle rather than each waiting
          // for an unrelated reach threshold to fire.
          AdvanceSwing(foot,i,std::max(dt,foot.duration-foot.age),position,yaw,velocity,sample);
          foot.stepping=false;foot.point=foot.goal;
      }
      const float width=.18f-.055f*Unit(sample.runBlend)*Unit(sample.gaitWeights[0]);
      const auto neutral=Neutral(position,yaw,i,width);
      const float supportReach=std::hypot(foot.goal.x-neutral.x,foot.goal.z-neutral.z);
      // An abrupt start or reversal can exhaust the support leg before the
      // blended walk/run duty catches up. Release that foot into the current
      // cycle rather than dragging its planted anchor past anatomical reach.
      const float releaseReach=.48f+.12f*Unit(sample.runBlend)*Unit(sample.gaitWeights[0]);
      if(!foot.stepping&&((q<duty&&supportReach<releaseReach)||speed<=.1f)){
        RollSupport(foot,dt,position,velocity);
        continue;
      }
      if(!foot.stepping) {
        foot.stepping=true;foot.from=foot.point;foot.root=position;foot.velocity=velocity;
        foot.sourcePhase=q;foot.age=0;foot.duration=std::max(.04f,(1-q)*timing.period);foot.lift=.1f;
        foot.goal=neutral;
        foot.goal.x+=velocity.x*timing.period*duty*.35f;
        foot.goal.z+=velocity.z*timing.period*duty*.35f;
      }
      const float progress=Unit((q-foot.sourcePhase)/std::max(.001f,1-foot.sourcePhase));
      // AdvanceSwing adds dt. Permit the pre-increment age to be negative so
      // a newly released foot starts at its exact contact instead of skipping
      // the first 240 Hz interval (visible as a jump on high-refresh frames).
      foot.age=progress*foot.duration-dt;
      AdvanceSwing(foot,i,dt,position,yaw,velocity,sample);
    }
    const float support=std::clamp((feet_[0].point.y-feet_[1].point.y)/.10f,-1.f,1.f);
    motion::Spring(support_,supportVelocity_,support,20.f,dt);
    firstStep_=false;
  }
};
}
