#include "game/AuthoredLocomotion.h"
#include "game/FootPlacement.h"
#include "game/CharacterPose.h"
#include "game/CharacterView.h"
#include "game/Game.h"
#include "game/HandPose.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <cstring>
#include <fstream>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
float Distance(XMFLOAT3 a,XMFLOAT3 b){return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b)));}
float Difference(const playerclips::Frame& a,const playerclips::Frame& b){
  float value=std::abs(a.hip-b.hip);
  for(unsigned i=0;i<2;++i)value=std::max({value,Distance(a.hands[i],b.hands[i]),Distance(a.feet[i],b.feet[i])});
  for(unsigned i=0;i<2;++i)value=std::max(value,Distance(a.elbows[i],b.elbows[i]));
  return std::max({value,Distance(a.torso,b.torso),Distance(a.pelvis,b.pelvis),std::hypot(a.shift.x-b.shift.x,a.shift.y-b.shift.y)});
}
void LibraryChecks(){
  const auto bytes=playerclips::EmbeddedBytes();const auto clips=playerclips::Decode(bytes);
  Check(clips.size()==57&&bytes.size()<2*1024*1024,"Fifty-one authored actions and six compatibility tracks must fit the motion budget");
  for(auto clip:{playerclips::Clip::RunForward,playerclips::Clip::RunBackward,playerclips::Clip::RunLeft,playerclips::Clip::RunRight,
      playerclips::Clip::SwordRun,playerclips::Clip::SwordRunBackward,playerclips::Clip::SwordRunLeft,playerclips::Clip::SwordRunRight,
      playerclips::Clip::StaffRun,playerclips::Clip::StaffRunBackward,playerclips::Clip::StaffRunLeft,playerclips::Clip::StaffRunRight,
      playerclips::Clip::BowRun,playerclips::Clip::BowRunBackward,playerclips::Clip::BowRunLeft,playerclips::Clip::BowRunRight}) {
    const auto& track=clips[unsigned(clip)];const auto native=animation::TravelGait(track.speed);
    Check(track.speed>5.f&&std::abs(track.duration-native.period)<1e-5f&&
      std::abs(track.duration*track.speed-native.stride)<1e-5f,"Authored run stride disagrees with native travel distance");
    // Local reach clamps used to bake a narrow pelvis trough at toe-off.
    // Check acceleration over the whole loop, including its seam, so a
    // continuous position curve cannot hide a sharp body/shoulder jolt.
    const auto count=track.frames.size()-1;const double dt=track.duration/count;
    double squaredAcceleration=0;
    for(std::size_t i=0;i<count;++i){
      const double a=(track.frames[(i+1)%count].hip-2*double(track.frames[i].hip)+track.frames[(i+count-1)%count].hip)/(dt*dt);
      squaredAcceleration+=a*a;
    }
    Check(std::sqrt(squaredAcceleration/count)<50,"Authored pelvis contains an abrupt support-release jolt");
  }
  for(unsigned i=4;i<12;++i){
    const auto clip=playerclips::Clip(i);
    Check(Difference(playerclips::Sample(clip,-.000001f),playerclips::Sample(clip,.000001f))<.0001f,"Gait loop seam");
    Check(Difference(playerclips::Sample(clip,.2f),playerclips::Sample(clip,.7f))>.025f,"Gait must articulate both halves of a stride");
    Check(Difference(playerclips::Sample(clip,.2f),playerclips::Sample(clip,4.2f))<.00001f,"Gait wrapping lost phase");
  }
  for(unsigned direction=0;direction<4;++direction){
    std::array<float,4>w{};w[direction]=1;
    float walkPeak=0,runPeak=0;
    for(int i=0;i<=100;++i)for(int side=0;side<2;++side){
      walkPeak=std::max(walkPeak,playerclips::SwingHeight(w,0,side,i/100.f));
      runPeak=std::max(runPeak,playerclips::SwingHeight(w,1,side,i/100.f));
    }
    Check(walkPeak>.025f&&walkPeak<.09f&&runPeak>walkPeak+.07f&&runPeak<=.23f,"Economical recovery must clear the ground without a high-stepping gait");
    Check(playerclips::SwingHeight(w,1,0,0)==0&&playerclips::SwingHeight(w,1,1,1)==0,"Contact endpoints must be flat");
  }
  const auto rejected=[](std::string data){try{playerclips::Decode(data);return false;}catch(const std::exception&){return true;}};
  Check(rejected(std::string(bytes.substr(0,bytes.size()-1))),"Truncated gait archive");
  auto corrupt=std::string(bytes);corrupt[8]=4;Check(rejected(corrupt),"Stale four-action resource must be rejected");
  std::size_t offset=12;for(unsigned i=0;i<4;++i)offset+=36+clips[i].frames.size()*sizeof(playerclips::Frame);
  corrupt=std::string(bytes);float bad=.5f;std::memcpy(corrupt.data()+offset+36,&bad,sizeof(bad));
  Check(rejected(corrupt),"Broken imported loop must be rejected");
  // Compatibility archives still load with zero new channels. Repacking the
  // current samples also exercises the variable on-disk stride independently
  // of the in-memory Frame layout.
  std::string legacy(bytes.substr(0,12));legacy[3]='3';legacy[4]=3;
  std::size_t pos=12;
  for(const auto& track:clips){
    legacy.append(bytes.substr(pos,36));pos+=36;
    for(std::size_t i=0;i<track.frames.size();++i){legacy.append(bytes.substr(pos,120));pos+=sizeof(playerclips::Frame);}
  }
  const auto compatibility=playerclips::Decode(legacy);
  Check(compatibility[8].frames[20].ankles[0].x==0&&compatibility[8].frames[20].knees[0].z==0,"Legacy clips must initialize extension channels");
  Check(Difference(compatibility[8].frames[20],clips[8].frames[20])==0,"PCL3 compatibility changed existing channels");
  corrupt=std::string(bytes);bad=2;
  std::memcpy(corrupt.data()+12+36+34*sizeof(float),&bad,sizeof(bad));
  Check(rejected(corrupt),"Invalid authored knee direction must be rejected");
  Check(clips[8].frames[20].articulated==1,"Blender spine and clavicle channels were discarded");
  Check(compatibility[8].frames[20].articulated==0,"Legacy archive enabled nonexistent spine channels");
  corrupt=std::string(bytes);bad=.5f;std::memcpy(corrupt.data()+12+36+52*sizeof(float),&bad,sizeof(bad));
  Check(rejected(corrupt),"Invalid skeletal extension flag accepted");
  corrupt=std::string(bytes);bad=.2f;std::memcpy(corrupt.data()+12+36+46*sizeof(float),&bad,sizeof(bad));
  Check(rejected(corrupt),"Unreachable clavicle offset accepted");
}
struct Walker {
  animation::State state;animation::FootPlacement feet;
  rpg::Vec2 position{};
  Walker(){feet.Reset(position,0);}
  void Update(float dt,float speed,float forward,float right,bool imported=true){
    position.x+=right*speed*dt;position.z+=forward*speed*dt;
    state.Update(dt,0,0,speed*dt,true,0);
    state.locomotion.Update(dt,speed*dt,forward,right,0,true,0,0,true);
    state.pose.motion=state.locomotion.pose;
    if(imported)animation::ApplyAuthoredLocomotion(state,dt);
    feet.Update(dt,position,0,true,true,state.pose);
  }
};
void FlightCarryBlendChecks(){
  // Changing the outgoing carry must retain half its effect halfway into
  // flight. A second fade before the flight blend reduced this to a quarter
  // and pulled the hands down through an unrelated neutral pose.
  const auto displacement=[](float air){
    animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;
    sample.gaitWeights[0]=1;sample.air=air;sample.flightPhase=.45f;
    sample.authoredMotion.hands[0]={-.035f,.10f,.08f};
    const auto a=BuildCharacterPose(0,0,false,sample);
    sample.authoredMotion.hands[0].z+=.02f;
    const auto b=BuildCharacterPose(0,0,false,sample);
    return Distance(a.arms[0].end,b.arms[0].end);
  };
  const float ground=displacement(0),half=displacement(.5f);
  Check(ground>.019f&&half>ground*.45f&&half<ground*.55f,"Flight attenuated the outgoing hand carry twice");
  Check(displacement(1)<1e-6f,"Full flight retained an outgoing carry offset");
  std::cout<<"Flight carry: midpoint retains "<<half/ground<<" of outgoing hand displacement\n";
  animation::State state;animation::FootPlacement feet;rpg::Vec2 position{};feet.Reset(position,0);
  CharacterPose previous;float fastest=0;bool wasFlying=false;
  for(int tick=0;tick<3000;++tick){
    constexpr float dt=.001f;const float t=tick*dt,flight=t-1.2f;
    const bool flying=flight>0&&flight<2*motion::jumpSpeed/motion::gravity;
    const float height=flying?motion::jumpSpeed*flight-.5f*motion::gravity*flight*flight:0;
    const float velocity=flying?motion::jumpSpeed-motion::gravity*flight:0;
    position.z+=3*dt;
    state.Update(dt,height,velocity,3*dt,true,0,wasFlying&&!flying?motion::jumpSpeed:0);
    state.locomotion.Update(dt,flying?0:3*dt,1,0,0,!flying,velocity,height,true);
    animation::ApplyAuthoredLocomotion(state,dt);feet.Update(dt,position,0,!flying,true,state.pose);
    const auto pose=BuildCharacterPose(0,0,false,state.pose);
    if(t>1.2f&&t<2.5f)for(unsigned side=0;side<2;++side)fastest=std::max(fastest,Distance(pose.arms[side].end,previous.arms[side].end));
    previous=pose;wasFlying=flying;
  }
  std::cout<<"Moving jump: maximum hand change per ms "<<fastest<<" m\n";
  Check(fastest<.006f,"Moving jump retracts arms abruptly before landing");
}
void NaturalGaitChecks(){
  FlightCarryBlendChecks();
  float compression=0,palmAgreement=1,footWidth=0,highestHip=-1;
  double footSum=0,handSum=0,product=0,hipSum=0;unsigned count=0;
  for(float speed:{1.4f,6.f}){
    Walker walk;
    for(int tick=0;tick<2400;++tick){
      walk.Update(.001f,speed,1,0);
      if(tick<1200)continue;
      const auto pose=BuildCharacterPose(0,0,false,walk.state.pose);
      for(unsigned side=0;side<2;++side){
        const auto hand=BuildHandPose(pose,side,false,false,0);
        const auto frame=XMLoadFloat4x4(&hand.frame);
        const auto forearm=XMVector3Normalize(XMLoadFloat3(&pose.arms[side].end)-XMLoadFloat3(&pose.arms[side].joint));
        palmAgreement=std::min(palmAgreement,XMVectorGetX(XMVector3Dot(frame.r[1],forearm)));
        Check(Distance(hand.Point({0,-.055f,-.043f}),pose.arms[side].end)<.00001f,"Free running wrist lost its forearm attachment");
      }
      if(speed>5){
        compression=std::max(compression,-pose.bodyLift);
        highestHip=std::max(highestHip,pose.bodyLift);hipSum+=pose.bodyLift;
        footWidth=std::max(footWidth,std::abs(pose.legs[1].end.x-pose.legs[0].end.x));
        const float foot=pose.legs[0].end.z,hand=pose.arms[0].end.z;
        footSum+=foot;handSum+=hand;product+=foot*hand;++count;
      }
    }
  }
  const double covariance=product/count-(footSum/count)*(handSum/count);
  std::cout<<"Natural gait: compression "<<compression<<", mean hip "<<hipSum/count<<", running width "<<footWidth<<", wrist alignment "<<palmAgreement<<", arm/leg covariance "<<covariance<<'\n';
  Check(palmAgreement>.99f,"Free hand reflected the backswing and folded at the wrist");
  // A loaded contact is lower than flight. Bound the mean separately so a
  // short compression can absorb contact without authorizing a constant squat.
  Check(compression<.12f&&hipSum/count>-.045,"Steady run collapsed into a persistent crouch");
  Check(highestHip>.005f&&highestHip+compression>.08f,"Pelvis smoothing erased the authored flight rise");
  Check(footWidth<.31f,"Forward running retained the wide standing stance");
  Check(covariance<-.002,"Running arm failed to oppose its same-side leg");
}
void AuthoredLegChecks(){
  const std::array<float,4> forward={1,0,0,0};
  float recoveryPitch=0,livePitch=0,liveHeight=0;
  for(int side=0;side<2;++side){
    Check(std::abs(playerclips::SwingPitch(forward,1,side,0,.4f)-.4f)<1e-6f,"Toe-off must retain its real orientation");
    Check(std::abs(playerclips::SwingPitch(forward,1,side,1,.4f))<1e-6f,"Touchdown must meet the flat world anchor");
    for(int k=0;k<=100;++k)recoveryPitch=std::max(recoveryPitch,playerclips::SwingPitch(forward,1,side,k/100.f,.4f));
  }
  Walker runner;
  for(int k=0;k<300;++k){
    runner.Update(1.f/120,6,1,0);
    if(k<120)continue;
    const auto pose=BuildCharacterPose(0,0,false,runner.state.pose);
    for(int side=0;side<2;++side){
      livePitch=std::max(livePitch,pose.footPitch[side]);liveHeight=std::max(liveHeight,pose.legs[side].end.y);
      Check(std::abs(Distance(pose.legs[side].root,pose.legs[side].joint)-CharacterPose::thigh)<1e-5f,"Authored knee plane stretched thigh");
      Check(std::abs(Distance(pose.legs[side].joint,pose.legs[side].end)-CharacterPose::shin)<1e-5f,"Authored knee plane stretched shin");
    }
  }
  Check(recoveryPitch>.65f&&livePitch>.65f,"Source ankle recovery was lost between Blender and native playback");
  Check(liveHeight>.32f&&liveHeight<.45f,"Running ankle recovery must remain compact instead of reaching half a metre");
  auto sample=runner.state.pose;sample.feetActive=true;
  sample.feet[0]={-.13f,.42f,.12f,0,0};sample.feet[1]={.13f,.20f,-.08f,0,0};
  sample.authoredMotion.knees[0]={0,0,1};
  const auto first=BuildCharacterPose(0,0,false,sample);
  sample.authoredMotion.knees[0]={.6f,0,.8f};
  const auto second=BuildCharacterPose(0,0,false,sample);
  Check(Distance(first.legs[0].joint,second.legs[0].joint)>.02f,"Native IK discarded exported knee direction");
  Check(Distance(first.legs[0].end,second.legs[0].end)<1e-6f,"Changing knee plane moved the contact anchor");
  animation::Sample reach;reach.authoredGait=true;reach.feetActive=true;
  reach.feet[0]={-.18f,.17f,-.55f,0,0};reach.feet[1]={.18f,.17f,0,0,0};
  const auto anchored=BuildCharacterPose(0,0,false,reach);
  reach.feetReachWeight[0]=0;
  const auto recovery=BuildCharacterPose(0,0,false,reach);
  Check(recovery.bodyLift>anchored.bodyLift+.15f,"Recovering foot pulled the whole pelvis into a squat");
  Check(Distance(recovery.legs[1].end,anchored.legs[1].end)<1e-5f,"Releasing swing reach displaced the support foot");
  Check(std::abs(Distance(recovery.legs[0].root,recovery.legs[0].joint)-CharacterPose::thigh)<1e-5f&&
    std::abs(Distance(recovery.legs[0].joint,recovery.legs[0].end)-CharacterPose::shin)<1e-5f,"Free leg reach correction stretched a bone");
  std::cout<<"Authored legs: recovery pitch "<<livePitch<<", ankle height "<<liveHeight<<'\n';
}
void CuttingGripChecks(){
  float worstAlignment=1,worstRollStep=0;
  for(float speed:{0.f,1.4f,6.f}) {
    animation::State state;XMFLOAT3 previous{};
    for(int tick=0;tick<2400;++tick) {
      constexpr float dt=.001f;
      state.Update(dt,0,0,speed*dt,true,0);
      state.locomotion.Update(dt,speed*dt,1,0,0,true,0,0,true);
      animation::ApplyAuthoredLocomotion(state,dt,playerclips::Equipment::Sword);
      const auto pose=BuildCharacterPose(0,0,false,state.pose,rpg::WeaponForm::Blade);
      const auto frame=WeaponFrame(pose);
      const auto forearm=XMVector3Normalize(XMLoadFloat3(&pose.arms[1].end)-XMLoadFloat3(&pose.arms[1].joint));
      if(tick>500) {
        worstAlignment=std::min(worstAlignment,XMVectorGetX(XMVector3Dot(frame.r[0],forearm)));
        worstRollStep=std::max(worstRollStep,Distance(previous,pose.weaponRight));
        Check(pose.weaponDirection.y<-.6f&&pose.weaponDirection.z<-.15f,
          "Travelling swordsman points the blade forward instead of carrying it down and clear");
        Check(Distance(BuildHandPose(pose,1,true,false,0).Point({0,-.055f,-.043f}),pose.arms[1].end)<1e-5f,
          "Cutting grip detached the wrist from the forearm");
      }
      previous=pose.weaponRight;
    }
  }
  Check(worstAlignment>.7f,"Carried sword forces the knuckles sideways across the forearm");
  Check(worstRollStep<.003f,"Carried sword rolls abruptly at a grip-frame pole");
  std::cout<<"Cutting grip: forearm alignment "<<worstAlignment<<", maximum roll-vector change per ms "<<worstRollStep<<'\n';
}
void ContinuousCycleChecks(){
  // Directional source support must survive retargeting. Forward running has
  // a longer push than sideways/backward travel; reusing its duty everywhere
  // keeps those feet planted beyond the authored release.
  for(unsigned direction=0;direction<4;++direction){
    Walker directional;const float forward=direction==0?1.f:direction==1?-1.f:0.f;
    const float right=direction==2?-1.f:direction==3?1.f:0.f;
    const float duty=playerclips::Collection()[unsigned(playerclips::Clip::RunForward)+direction].stance;
    for(int tick=0;tick<960;++tick){
      directional.Update(1.f/240,6,forward,right);
      if(tick<480)continue;
      for(int side=0;side<2;++side){
        const float p=directional.state.pose.authoredPhase+side*.5f;
        const float q=p-std::floor(p);
        if(directional.feet.Planted(side))Check(q<duty+.009f,"Directional support outlasted its authored release");
        if(q>.02f&&q<duty-.02f)Check(directional.feet.Planted(side),"Directional support released before accepting body weight");
      }
    }
  }
  for(int hz:{30,60,144})for(float speed:{1.4f,6.f}){
    Walker walker;float previous=0,minRate=10,maxRate=0;bool planted[2]={true,true};
    std::array<std::vector<float>,2> contacts;
    for(int tick=0;tick<hz*5;++tick){
      walker.Update(1.f/hz,speed,1,0);
      const float phase=walker.state.pose.authoredPhase;
      if(tick>hz*2){
        const float rate=std::remainder(phase-previous,1.f)*hz;
        minRate=std::min(minRate,rate);maxRate=std::max(maxRate,rate);
        for(int side=0;side<2;++side)if(walker.feet.Planted(side)&&!planted[side])contacts[side].push_back(phase);
      }
      previous=phase;for(int side=0;side<2;++side)planted[side]=walker.feet.Planted(side);
    }
    const float expected=1/animation::TravelGait(speed).period;
    Check(minRate>expected-.003f&&maxRate<expected+.003f,"Constant travel warped the authored cycle clock");
    for(int side=0;side<2;++side){
      Check(contacts[side].size()>=2,"Both legs must complete repeated support phases");
      for(float p:contacts[side])Check(std::abs(std::remainder(p-side*.5f,1.f))<expected/hz+.015f,"Opposite contact drifted away from the half-cycle");
    }
  }
  Walker walker;float previous=0;
  for(int tick=0;tick<480;++tick){
    const float dt=1.f/144,flight=tick*dt-1,span=2*motion::jumpSpeed/motion::gravity;
    const bool air=flight>0&&flight<span;
    const float height=air?motion::jumpSpeed*flight-.5f*motion::gravity*flight*flight:0;
    walker.position.z+=6*dt;
    walker.state.Update(dt,height,air?motion::jumpSpeed-motion::gravity*flight:0,6*dt,true,0);
    walker.state.locomotion.Update(dt,air?0:6*dt,1,0,0,!air,0,height,true);
    animation::ApplyAuthoredLocomotion(walker.state,dt);
    walker.feet.Update(dt,walker.position,0,!air,true,walker.state.pose);
    const float phase=walker.state.pose.authoredPhase;
    if(tick>30)Check(std::remainder(phase-previous,1.f)>-.0001f,"Takeoff or landing played the body cycle backwards");
    previous=phase;
  }
  std::cout<<"Continuous bilateral gait: uniform clock at 30/60/144 Hz; half-cycle contacts; no reverse phase through flight\n";
}
void RecoveryTimingChecks(){
  const std::array<float,4> forward={1,0,0,0};
  float deviation=0;
  for(int side=0;side<2;++side){
    Check(playerclips::SwingProgress(forward,1,side,0)==0&&playerclips::SwingProgress(forward,1,side,1)==1,"Source recovery must retain live release and contact anchors");
    for(int frame=1;frame<100;++frame){
      const float t=frame/100.f,p=playerclips::SwingProgress(forward,1,side,t);
      Check(std::isfinite(p)&&p>=-.12f&&p<=1.12f,"Source recovery progress escaped its bounded path");
      deviation=std::max(deviation,std::abs(p-t*t*(3-2*t)));
    }
  }
  Check(deviation>.06f,"Native recovery discarded the authored heel fold and extension timing");
  animation::Sample airborne;airborne.authoredGait=true;airborne.air=1;airborne.flightPhase=.5f;
  airborne.departureFeet[0].z=.225f;airborne.departureFeet[1].z=-.225f;
  const auto leftLead=BuildCharacterPose(0,0,false,airborne);
  airborne.departureFeet[0].z=-.225f;airborne.departureFeet[1].z=.225f;
  const auto rightLead=BuildCharacterPose(0,0,false,airborne);
  const float leftSeparation=leftLead.legs[0].end.z-rightLead.legs[0].end.z;
  const float rightSeparation=rightLead.legs[1].end.z-leftLead.legs[1].end.z;
  Check(leftSeparation>.10f&&leftSeparation<.18f&&rightSeparation>.10f&&rightSeparation<.18f,
    "Running jump must retain a compact leading leg without a cycling pose");
  for(int hz:{30,60,144}){
    Game game;game.Initialize();float previous=-1;bool seen=false;
    for(int frame=0;frame<hz;++frame){
      rpg::Command command;command.jump=frame==0;command.movement={0,1};game.ExecuteCommand(command,1.f/hz);
      if(!game.State().Grounded()){
        const float phase=game.PlayerAnimation().flightPhase;
        Check(phase>=0&&phase<=1&&phase>=previous,"Ballistic animation reversed or lost its flight phase");
        previous=phase;seen=true;
      }
    }
    Check(seen&&previous>.9f,"Flight animation did not progress from takeoff to pre-contact extension");
  }
}
void FlightClockChecks(){
  for(const auto equipment:{playerclips::Equipment::None,playerclips::Equipment::Sword,playerclips::Equipment::Staff,playerclips::Equipment::Bow})
    for(float phase:{.55f,.8f,.98f}){
      animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;sample.authoredEquipment=equipment;
      sample.air=1;sample.rise=-.7f;sample.flightPhase=phase;
      const auto first=BuildCharacterPose(0,0,false,sample);
      for(float clock:{.37f,1.8f,27.1f}){
        sample.authoredClock=clock;const auto next=BuildCharacterPose(0,0,false,sample);
        Check(std::abs(first.bodyLift-next.bodyLift)<1e-6f,"Jump pelvis depends on unrelated idle time");
        for(int side=0;side<2;++side)Check(Distance(first.legs[side].end,next.legs[side].end)<1e-6f&&
          Distance(first.arms[side].end,next.arms[side].end)<1e-6f,"Jump limbs sampled a free-running fall loop");
      }
    }
  animation::State state;
  state.Update(.01f,.8f,-motion::jumpSpeed,0,true,0);
  Check(state.pose.extendedFall==0,"Normal jump descent must retain its landing trajectory");
  state.Update(.01f,3.f,-motion::jumpSpeed*1.8f,0,true,0);
  Check(state.pose.extendedFall>.99f,"Deep fall did not enter the separate falling pose");
  std::cout<<"Flight poses: independent of idle clock; extended fall driven by descent velocity\n";
}
void LateralClearanceChecks(){
  float minimum=10,maximumStep=0;
  for(float speed:{1.4f,3.f,6.f})for(float right:{-1.f,1.f}){
    Walker walker;CharacterPose previous;
    for(int tick=0;tick<4000;++tick){
      walker.Update(.001f,speed,0,right);
      const auto pose=BuildCharacterPose(0,0,false,walker.state.pose);
      if(tick>1000){
        minimum=std::min(minimum,pose.legs[1].end.x-pose.legs[0].end.x);
        for(int side=0;side<2;++side)maximumStep=std::max(maximumStep,Distance(previous.legs[side].end,pose.legs[side].end));
      }
      previous=pose;
    }
  }
  std::cout<<"Lateral clearance "<<minimum<<" m; maximum ankle change per ms "<<maximumStep<<" m\n";
  Check(minimum>.09f,"Lateral recovery crossed the supporting leg");
  Check(maximumStep<.014f,"Lateral clearance snapped a foot at release/contact");
}
void ContactsAndTransitions(){
  float worstSlip=0,worstJump=0,worstFoot=0;playerclips::Frame reference;
  for(int hz:{30,60,144,1000}){
    Walker walk;std::array<XMFLOAT3,2> previous{};bool planted[2]={};
    for(int i=0;i<hz*2;++i){
      walk.Update(1.f/hz,6,0,1);const auto pose=BuildCharacterPose(0,0,false,walk.state.pose);
      for(int side=0;side<2;++side){
        auto p=pose.FootPoint(side,{0,-.17f,.185f});p.x+=walk.position.x;p.z+=walk.position.z;
        if(planted[side]&&walk.feet.Planted(side))worstSlip=std::max(worstSlip,Distance(p,previous[side]));
        Check(p.y>=-.0001f&&pose.legs[side].end.y<=.50f,"Authored swing pierced floor or escaped reach budget");
        Check(std::abs(Distance(pose.legs[side].root,pose.legs[side].joint)-CharacterPose::thigh)<.0001f,"Authored gait stretched thigh");
        Check(std::abs(Distance(pose.legs[side].joint,pose.legs[side].end)-CharacterPose::shin)<.0001f,"Authored gait stretched shin");
        previous[side]=p;planted[side]=walk.feet.Planted(side);
      }
    }
    if(hz!=30)Check(Difference(reference,walk.state.pose.authoredMotion)<.001f,"Equal-distance authored gait depends on refresh rate");
    reference=walk.state.pose.authoredMotion;
    for(int i=0;i<hz*2;++i)walk.Update(1.f/hz,0,0,0);
    Check(Difference(walk.state.pose.authoredMotion,{})<.0001f,"Stopped player kept locomotion motion");
  }
  Walker taps;CharacterPose previous=BuildCharacterPose(0,0,false,taps.state.pose);
  for(int i=0;i<3000;++i){
    const int direction=i/150%4;const float speed=i%150<110?6.f:0.f;
    taps.Update(.001f,speed,direction==0?1.f:direction==1?-1.f:0.f,direction==2?-1.f:direction==3?1.f:0.f);
    const auto pose=BuildCharacterPose(0,0,false,taps.state.pose);
    for(int side=0;side<2;++side) {
      const float delta=Distance(previous.arms[side].end,pose.arms[side].end);
      worstFoot=std::max(worstFoot,Distance(previous.legs[side].end,pose.legs[side].end));
      if(delta>.012f)std::cerr<<"Reversal frame "<<i<<" side "<<side<<" hand "<<delta<<" hip "<<previous.bodyLift<<" -> "<<pose.bodyLift<<" feet "<<taps.state.pose.feet[0].x<<','<<taps.state.pose.feet[0].y<<','<<taps.state.pose.feet[0].z<<" | "<<taps.state.pose.feet[1].x<<','<<taps.state.pose.feet[1].y<<','<<taps.state.pose.feet[1].z<<'\n';
      worstJump=std::max(worstJump,delta);
    }
    previous=pose;
  }
  std::cout<<"Authored gait: maximum planted slip "<<worstSlip<<" m, hand change per ms "<<worstJump<<" m, ankle change per ms "<<worstFoot<<" m\n";
  Check(worstSlip<.002f,"Authored pelvis displaced world-space foot anchors");
  Check(worstJump<.012f,"Start/stop/reversal caused a visible hand snap");
  Check(worstFoot<.025f,"A wrapped early-release step teleported its recovering foot");
  // Shared locomotion must coexist with every weapon form, not just the guard.
  for(unsigned id=0;id<unsigned(rpg::ClassId::Count);++id)for(bool heavy:{false,true}){
    Walker moving;for(int i=0;i<60;++i)moving.Update(.01f,5,1,0);
    for(int frame=0;frame<80;++frame){
      moving.Update(.01f,5,1,0);auto sample=moving.state.pose;
      sample.action=animation::SampleAction(rpg::ClassId(id),heavy?animation::ActionClip::Heavy:animation::ActionClip::Basic,frame*.01f);
      sample.attackSeconds=rpg::DescribeClass(rpg::ClassId(id)).range>3?-1:frame*.01f;sample.heavyAttack=heavy;
      const auto pose=BuildCharacterPose(0,0,id==0,sample,rpg::WeaponFor(rpg::ClassId(id)).form);
      Check(pose.WeaponPoint(.96f).y>.03f,"Moving class attack put weapon through ground");
      for(const auto& arm:pose.arms){
        Check(std::abs(Distance(arm.root,arm.joint)-CharacterPose::upperArm)<.0001f&&
          std::abs(Distance(arm.joint,arm.end)-CharacterPose::forearm)<.0001f,"Moving class attack stretched an arm");
      }
    }
  }
  Game game;game.Initialize();rpg::Simulation simulation;
  for(int i=0;i<240;++i){rpg::Command command;command.movement={i<120?1.f:-1.f,0};command.jump=i==55;
    game.ExecuteCommand(command,1.f/60);simulation.Update(1.f/60,command);
    Check(game.State().Save()==simulation.Save(),"Authored locomotion changed simulation/save state");
  }
  const auto save=game.State().Save();Check(game.RestoreSnapshot(save)&&!game.PlayerAnimation().authoredGait,"Loading retained transient authored pose");
}
void WeaponMotionChecks(){
  float worstGrip=0,worstStep=0,worstFinger=0;
  for(bool staff:{false,true})for(bool heavy:{false,true})for(int hz:{30,60,144,1000}) {
    animation::AuthoredWeaponController controller;CharacterPose previous;bool havePrevious=false;
    XMFLOAT3 first{};float excursion=0;
    for(int tick=0;tick<hz*2;++tick) {
      const float t=float(tick)/hz;
      controller.Update(1.f/hz,t<combat::Duration(heavy)?t:-1,heavy,staff,true);
      animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;
      sample.authoredReady=1; // Exercise the fully engaged two-hand combat grip.
      sample.authoredEquipment=staff?playerclips::Equipment::Staff:playerclips::Equipment::Sword;
      sample.authoredStrike=controller.frame;sample.authoredStrikeWeight=controller.weight;
      sample.authoredStrikePhase=controller.phase;sample.authoredStrikeHeavy=heavy;
      const auto pose=BuildCharacterPose(0,0,false,sample,staff?rpg::WeaponForm::AstralStaff:rpg::WeaponForm::Blade);
      const auto tip=pose.WeaponPoint(.96f);
      for(unsigned side=0;side<2;++side)
        Check(Distance(BuildHandPose(pose,side,true,false,pose.handCast).Point({0,-.055f,-.043f}),pose.arms[side].end)<.0001f,"Wrist detached during weapon attack/recovery");
      if(tick==0)first=tip;excursion=std::max(excursion,Distance(first,tip));
      Check(std::isfinite(tip.x)&&std::isfinite(tip.y)&&std::isfinite(tip.z),"Imported weapon pose is not finite");
      Check(tip.y>.025f,"Imported weapon tip pierced floor");
      if(staff) {
        const auto hand=BuildHandPose(pose,0,true,false,pose.handCast);
        worstGrip=std::max(worstGrip,Distance(hand.Point({0,0,0}),pose.WeaponPoint(.26f)));
        Check(hand.curl>.99f,"Supporting staff hand did not close on shaft");
      }
      if(havePrevious&&hz==1000) {
        const float step=Distance(previous.WeaponPoint(.96f),tip);
        worstStep=std::max(worstStep,step);
        for(unsigned side=0;side<2;++side) {
          const auto before=BuildHandPose(previous,side,true,false,previous.handCast),after=BuildHandPose(pose,side,true,false,pose.handCast);
          worstFinger=std::max(worstFinger,Distance(before.Point(before.fingers[0].joints[3]),after.Point(after.fingers[0].joints[3])));
        }
      }
      if(t>combat::Duration(heavy)+.01f&&t<combat::Duration(heavy)+.05f)Check(controller.weight>0,"Recovery was cut off by the gameplay timer");
      previous=pose;havePrevious=true;
    }
    Check(excursion>.4f,"Equipped sword/staff remained static through the attack");
    Check(controller.weight==0,"Authored recovery did not finish");
    // Cancelling before release must freeze, then fade, never reach contact.
    controller={};for(int k=0;k<5;++k)controller.Update(.01f,k*.01f,heavy,staff,true);
    const auto cancelled=controller.frame;
    for(int k=0;k<25;++k) {controller.Update(.01f,-1,heavy,staff,true);Check(Difference(controller.frame,cancelled)<.0001f,"Cancelled attack advanced into phantom contact");}
    Check(controller.weight==0,"Cancelled authored strike persisted");
  }
  std::cout<<"V2 staff grip error "<<worstGrip<<" m; 1 ms weapon tip change "<<worstStep<<" m; fingertip "<<worstFinger<<" m\n";
  Check(worstGrip<.003f,"Second hand detached from staff");
  Check(worstStep<.055f,"Authored recovery or weapon orientation snapped");
  Check(worstFinger<.02f,"Authored weapon wrist or fingers snapped");
  // Exercise the actual player adapter, including ranged staff releases. The
  // same command stream must leave combat and saves identical to Simulation.
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Scout,rpg::ClassId::Mage})for(bool heavy:{false,true})for(int hz:{30,60,144}) {
    rpg::Simulation seed;
    Check(seed.Load("MEDITERRANEAN 3\n-4 5.2 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n100 -4 7 80 0 0 0 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n"),"V2 combat fixture");
    auto snapshot=seed.Save();const auto p=snapshot.find("PROFILE ");
    snapshot.replace(p,snapshot.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Motion\" 0 0 0");
    Game game;rpg::Simulation reference;Check(game.RestoreSnapshot(snapshot)&&reference.Load(snapshot),"V2 player setup");
    float peak=0;bool recovery=false;
    const float duration=1.3f/rpg::ClassAttackRate(id,heavy)+.5f;
    for(int k=0;k<int(hz*duration);++k) {
      rpg::Command command;if(k==0){command.ability=heavy?1:0;command.nextTarget=true;}
      game.ExecuteCommand(command,1.f/hz);reference.Update(1.f/hz,command);
      const auto& pose=game.PlayerAnimation();peak=std::max(peak,pose.authoredStrikeWeight);
      recovery|=!game.State().Attacking()&&pose.authoredStrikeWeight>.02f;
      Check(game.State().Save()==reference.Save(),"V2 presentation changed combat/save state");
      Check(pose.authoredEquipment==(id==rpg::ClassId::Mage?playerclips::Equipment::Staff:id==rpg::ClassId::Scout?playerclips::Equipment::Bow:playerclips::Equipment::Sword),"Live equipment did not select its authored clips");
    }
    if(peak<=.95f||!recovery)std::cerr<<"Live v2 class "<<int(id)<<" heavy "<<heavy<<" Hz "<<hz<<" peak "<<peak<<" recovery "<<recovery<<" attack "<<game.State().AttackSeconds()<<'\n';
    Check(peak>.95f&&recovery,"Live player did not play its full authored attack/recovery");
    Check(game.PlayerAnimation().authoredStrikeWeight==0,"Live authored recovery did not settle");
    game.SuspendInput();Check(game.PlayerAnimation().authoredStrikeWeight==0,"Focus loss retained v2 attack");
  }
}
void LoadedArmRunningChecks(){
  // A carried weapon must have a genuine shoulder/elbow arc, without the
  // high-frequency wrist acceleration caused by the old world-locked grip.
  for(auto equipment:{playerclips::Equipment::Sword,playerclips::Equipment::Staff,playerclips::Equipment::Bow}){
    animation::State state;animation::FootPlacement feet;rpg::Vec2 position{};feet.Reset(position,0);
    const auto form=equipment==playerclips::Equipment::Sword?rpg::WeaponForm::Blade:
      equipment==playerclips::Equipment::Staff?rpg::WeaponForm::AstralStaff:rpg::WeaponForm::Handbow;
    constexpr float dt=1.f/120;XMFLOAT3 before{},previous{},firstRelative{};
    double accelerationSquared=0;unsigned samples=0;float excursion=0,minimumFlex=4;
    for(int tick=0;tick<720;++tick){
      position.z+=6*dt;state.Update(dt,0,0,6*dt,true,0);
      state.locomotion.Update(dt,6*dt,1,0,0,true,0,0,true);
      animation::ApplyAuthoredLocomotion(state,dt,equipment);feet.Update(dt,position,0,true,true,state.pose);
      const auto pose=BuildCharacterPose(0,0,false,state.pose,form);const auto& arm=pose.arms[1];
      const auto wrist=XMLoadFloat3(&arm.end),root=XMLoadFloat3(&arm.root),joint=XMLoadFloat3(&arm.joint);
      XMFLOAT3 relative;XMStoreFloat3(&relative,wrist-root);
      if(tick==240)firstRelative=relative;
      if(tick>240){
        excursion=std::max(excursion,Distance(relative,firstRelative));
        const auto upper=XMVector3Normalize(joint-root),lower=XMVector3Normalize(wrist-joint);
        minimumFlex=std::min(minimumFlex,std::acos(std::clamp(XMVectorGetX(XMVector3Dot(upper,lower)),-1.f,1.f)));
        // Isolate arm motion from the unchanged planted-leg pelvis response.
        const auto acceleration=(XMLoadFloat3(&relative)-2*XMLoadFloat3(&previous)+XMLoadFloat3(&before))/(dt*dt);
        accelerationSquared+=XMVectorGetX(XMVector3LengthSq(acceleration));++samples;
        Check(Distance(BuildHandPose(pose,1,true,false,0).Point({0,0,0}),pose.WeaponPoint(0))<.0001f,"Running weapon detached from palm");
      }
      before=previous;previous=relative;
    }
    const float rms=float(std::sqrt(accelerationSquared/samples));
    std::cout<<"Loaded run "<<unsigned(equipment)<<": shoulder-relative wrist acceleration RMS "<<rms<<" m/s2; relative arc "<<excursion<<" m; minimum elbow flex "<<minimumFlex<<" rad\n";
    Check(rms<30,"Loaded running wrist has abrupt acceleration instead of a continuous carrying arc");
    Check(excursion>.06f,"Loaded arm was frozen to hide its motion");
    Check(minimumFlex>.40f,"Loaded elbow locked near full extension during running");
  }
}
void GroundedMotionChecks(){
  std::filesystem::create_directories("grounded-review");
  std::ofstream csv("grounded-review/trajectories.csv");
  csv<<"class,heavy,ready,t,tip_x,tip_y,tip_z,hip_x,hip_y,hip_z,pelvis_yaw,chest_yaw,elbow_x,elbow_y,elbow_z\n";
  for(int cls=0;cls<3;++cls)for(bool heavy:{false,true})for(bool ready:{false,true}) {
    animation::AuthoredWeaponController controller;CharacterPose previous;
    float tipSpeed=0,handSpeed=0,elbowSpeed=0,tipAt=0,handAt=0,elbowAt=0,pelvisTurn=0;
    for(int tick=0;tick<1500;++tick) {
      const float t=tick*.001f;controller.Update(.001f,t<combat::Duration(heavy)?t:-1,heavy,cls==2,true,1,cls==1);
      animation::Sample s;s.authoredGait=true;s.authoredEquipment=cls==0?playerclips::Equipment::Sword:cls==1?playerclips::Equipment::Bow:playerclips::Equipment::Staff;
      s.authoredReady=ready?1:controller.ready;s.authoredEntryReady=ready?1:controller.entryReady;
      s.authoredStrike=controller.frame;s.authoredStrikeWeight=controller.weight;s.authoredStrikePhase=controller.phase;s.authoredStrikeHeavy=heavy;
      s.feetActive=true;s.feet[0].x=-.18f;s.feet[1].x=.18f;
      const auto pose=BuildCharacterPose(0,0,false,s,cls==0?rpg::WeaponForm::Blade:cls==1?rpg::WeaponForm::Handbow:rpg::WeaponForm::AstralStaff);
      const auto tip=pose.WeaponPoint(.96f),hip=pose.PelvisPoint({0,.93f,0});
      if(tick){
        const float speed=Distance(tip,previous.WeaponPoint(.96f))*1000;
        if(speed>tipSpeed){tipSpeed=speed;tipAt=t;}
        for(int side=0;side<2;++side) {
          float v=Distance(pose.arms[side].end,previous.arms[side].end)*1000;if(v>handSpeed){handSpeed=v;handAt=t;}
          v=Distance(pose.arms[side].joint,previous.arms[side].joint)*1000;if(v>elbowSpeed){elbowSpeed=v;elbowAt=t;}
        }
      }
      for(int side=0;side<2;++side) {
        Check(Distance(pose.legs[side].end,{side? .18f:-.18f,.17f,0})<.0001f,"Strike shifted a planted ankle");
        Check(std::abs(Distance(pose.arms[side].root,pose.arms[side].joint)-CharacterPose::upperArm)<.0001f,"Strike stretched upper arm");
      }
      pelvisTurn=std::max(pelvisTurn,std::abs(pose.pelvisRotation.y));
      if(tick%5==0)csv<<cls<<','<<heavy<<','<<ready<<','<<t<<','<<tip.x<<','<<tip.y<<','<<tip.z<<','<<hip.x<<','<<hip.y<<','<<hip.z<<','<<pose.pelvisRotation.y<<','<<pose.upperYaw<<','<<pose.arms[1].joint.x<<','<<pose.arms[1].joint.y<<','<<pose.arms[1].joint.z<<'\n';
      previous=pose;
    }
    std::cout<<"Grounded "<<cls<<" heavy "<<heavy<<" ready "<<ready<<": tip "<<tipSpeed<<" @ "<<tipAt<<"; hand "<<handSpeed<<" @ "<<handAt<<"; elbow "<<elbowSpeed<<" @ "<<elbowAt<<" m/s; pelvis turn "<<pelvisTurn<<'\n';
    Check(pelvisTurn>.015f,"Full-body strike lost authored pelvis rotation");
    Check(tipSpeed<22.f&&handSpeed<13.f&&elbowSpeed<13.f,"Retargeted strike has a wrist, elbow or weapon discontinuity");
  }
}
void BowMotionChecks(){
  float worstStep=0,worstNock=0;
  for(bool heavy:{false,true}) {
    animation::AuthoredWeaponController controller;CharacterPose previous;bool havePrevious=false,drawn=false,released=false;
    for(int tick=0;tick<2000;++tick) {
      const float t=tick*.001f;controller.Update(.001f,t<combat::Duration(heavy)?t:-1,heavy,false,true,1,true);
      animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;sample.authoredEquipment=playerclips::Equipment::Bow;
      sample.authoredStrike=controller.frame;sample.authoredStrikeWeight=controller.weight;sample.authoredStrikePhase=controller.phase;sample.authoredStrikeHeavy=heavy;
      const auto pose=BuildCharacterPose(0,0,true,sample,rpg::WeaponForm::Handbow);
      for(unsigned side=0;side<2;++side)
        Check(Distance(BuildHandPose(pose,side,true,false,0).Point({0,-.055f,-.043f}),pose.arms[side].end)<.0001f,"Archer wrist detached during draw/release");
      if(havePrevious)for(unsigned side=0;side<2;++side){const float step=Distance(previous.arms[side].end,pose.arms[side].end);worstStep=std::max(worstStep,step);
        if(step>.025f)std::cout<<"Bow jump heavy "<<heavy<<" t "<<t<<" side "<<side<<" phase "<<controller.phase<<" draw "<<previous.bowDraw<<" -> "<<pose.bowDraw<<" delta "<<step<<'\n';}
      if(pose.bowDraw>.95f) {
        XMFLOAT3 nock;XMStoreFloat3(&nock,XMVector3TransformCoord(XMVectorSet(0,.035f,-.07f-.40f*pose.bowDraw,1),WeaponFrame(pose)));
        worstNock=std::max(worstNock,Distance(nock,BuildHandPose(pose,0,true,false,0).Point({0,0,0})));drawn=true;
        Check(BuildHandPose(pose,0,true,false,0).curl>.75f,"Archer drawing hand stayed open");
        const auto contact=playerclips::Collection()[unsigned(heavy?playerclips::Clip::BowPowerRelease:playerclips::Clip::BowRelease)].contact;
        if(controller.phase<contact)Check(pose.bowArrow>.9f,"Drawing bow lost its nocked arrow");
      }
      const auto contact=playerclips::Collection()[unsigned(heavy?playerclips::Clip::BowPowerRelease:playerclips::Clip::BowRelease)].contact;
      if(controller.phase>contact) {Check(pose.bowArrow==0,"Released arrow respawned during recovery");released=true;}
      Check(pose.bowDraw>=0&&pose.bowDraw<=1&&pose.bowArrow>=0&&pose.bowArrow<=1,"Invalid bow draw/release channel");
      previous=pose;havePrevious=true;
    }
    Check(drawn&&released&&controller.weight==0,"Bow did not draw, release and settle");
  }
  std::cout<<"Bow nock error "<<worstNock<<" m; hand step per ms "<<worstStep<<" m\n";
  Check(worstNock<.01f,"Drawing hand detached from the string");
  Check(worstStep<.025f,"Archery draw/release snapped the wrist");
}
void WeaponCarryChecks(){
  using E=playerclips::Equipment;
  for(auto equipment:{E::Sword,E::Staff,E::Bow}) {
    const auto form=equipment==E::Staff?rpg::WeaponForm::AstralStaff:equipment==E::Bow?rpg::WeaponForm::Handbow:rpg::WeaponForm::Blade;
    animation::Sample sample;sample.authoredGait=true;sample.authoredEquipment=equipment;
    const auto relaxed=BuildCharacterPose(0,0,false,sample,form);
    sample.authoredReady=1;const auto guard=BuildCharacterPose(0,0,false,sample,form);
    if(equipment==E::Bow)Check(relaxed.bowArrow==0&&guard.bowArrow==0,"Relaxed bow carries an unsupported arrow");
    Check(Distance(relaxed.WeaponPoint(.8f),guard.WeaponPoint(.8f))>.16f,"Relaxed carry and combat guard are indistinguishable");
    if(equipment==E::Staff)Check(relaxed.staffGripWeight==0&&guard.staffGripWeight>.99f,"Staff support hand must release during relaxed carry");
    if(equipment==E::Sword) {
      const auto shieldPose=BuildCharacterPose(0,0,true,sample,form);
      Check(Distance(BuildHandPose(shieldPose,0,true,true,0).Point({0,-.055f,-.043f}),shieldPose.arms[0].end)<.0001f,"Shield grip separated the wrist from the arm");
    }
    for(unsigned direction=0;direction<4;++direction)for(float run:{0.f,1.f}) {
      sample={};sample.authoredGait=true;sample.authoredEquipment=equipment;sample.gaitWeights[direction]=1;
      CharacterPose previous;float excursion=0;XMFLOAT3 first{};
      for(int i=0;i<=1000;++i) {
        sample.authoredMotion=playerclips::Locomotion(sample.gaitWeights,run,i/1000.f,equipment);
        const auto pose=BuildCharacterPose(0,0,false,sample,form);
        for(unsigned side=0;side<2;++side) {
          const auto hand=BuildHandPose(pose,side,true,false,pose.handCast);
          Check(Distance(hand.Point({0,-.055f,-.043f}),pose.arms[side].end)<.0001f,"Wrist detached from forearm during equipped movement");
          Check(std::abs(Distance(pose.arms[side].joint,pose.arms[side].end)-CharacterPose::forearm)<.0001f,"Forearm stretched to reach weapon socket");
        }
        const auto tip=pose.WeaponPoint(equipment==E::Staff?-.72f:equipment==E::Bow?-.64f:.96f);
        Check(tip.y>.025f,"Carried weapon intersects the floor");
        if(i)Check(Distance(previous.WeaponPoint(.96f),pose.WeaponPoint(.96f))<.008f,"Directional weapon carry has a discontinuity");
        else first=pose.WeaponPoint(.96f);
        excursion=std::max(excursion,Distance(first,pose.WeaponPoint(.96f)));previous=pose;
      }
        if(excursion<=.035f)std::cerr<<"Carry excursion "<<excursion<<" m, equipment "<<unsigned(equipment)<<", direction "<<direction<<", run "<<run<<'\n';
        Check(excursion>.035f,"Carried weapon is frozen during locomotion");
    }
    animation::AuthoredWeaponController controller;
    for(int i=0;i<400;++i)controller.Update(.01f,i<40?i*.01f:-1,false,equipment==E::Staff,true,1,equipment==E::Bow);
    Check(controller.ready<.02f,"Weapon did not settle back to relaxed carry after combat");
    controller.Update(.01f,0,false,equipment==E::Staff,true,1,equipment==E::Bow);
    Check(controller.ready>0&&controller.ready<.15f,"Combat guard engages with a snap");
  }
}
void EquippedFlightChecks(){
  using E=playerclips::Equipment;
  float worstHand=0,worstTip=0,worstShield=0;
  for(auto equipment:{E::Sword,E::Staff,E::Bow})for(bool shield:{false,true}) {
    if(shield&&equipment!=E::Sword)continue;
    const auto form=equipment==E::Staff?rpg::WeaponForm::AstralStaff:equipment==E::Bow?rpg::WeaponForm::Handbow:rpg::WeaponForm::Blade;
    animation::State state;CharacterPose previous;XMFLOAT3 previousShield{};bool initialized=false;
    for(int tick=0;tick<2200;++tick) {
      const float flight=tick*.001f-.4f;const bool flying=flight>0&&flight<.6f;
      const float height=flying?5*flight*(.6f-flight):0,velocity=flying?3-10*flight:0;
      state.Update(.001f,height,velocity,0,true,0,tick==1000?3.f:0);
      animation::ApplyAuthoredLocomotion(state,.001f,equipment);
      const auto pose=BuildCharacterPose(0,0,shield,state.pose,form);
      if(equipment==E::Bow)Check(pose.bowArrow==0,"Jumping archer carries an unsupported arrow");
      for(unsigned side=0;side<2;++side) {
        Check(Distance(BuildHandPose(pose,side,true,shield,0).Point({0,-.055f,-.043f}),pose.arms[side].end)<.0001f,"Equipped flight detached the wrist");
        Check(std::abs(Distance(pose.arms[side].joint,pose.arms[side].end)-CharacterPose::forearm)<.0001f,"Equipped flight stretched the forearm");
        if(initialized){
          const float step=Distance(previous.arms[side].end,pose.arms[side].end);
          if(step>.006f)std::cout<<"Flight wrist step "<<step<<", equipment "<<unsigned(equipment)<<", side "<<side<<", tick "<<tick<<", air "<<state.pose.air<<", landing "<<state.pose.landing<<'\n';
          worstHand=std::max(worstHand,step);
        }
      }
      const float tip=equipment==E::Sword?.96f:equipment==E::Staff?-.72f:-.64f;
      Check(pose.WeaponPoint(tip).y+height>.04f,"Equipped flight or landing pierced floor");
      if(initialized) {
        const float step=Distance(previous.WeaponPoint(tip),pose.WeaponPoint(tip));
        if(step>.018f)std::cout<<"Flight tip step "<<step<<", equipment "<<unsigned(equipment)<<", tick "<<tick<<", air "<<state.pose.air<<", landing "<<state.pose.landing<<'\n';
        worstTip=std::max(worstTip,step);
      }
      if(shield) {
        const auto plate=ShieldFrame(pose);
        Check(std::abs(XMVectorGetX(XMMatrixDeterminant(plate))-1)<.0001f,"Shield frame sheared or reflected");
        XMFLOAT3 corner;XMStoreFloat3(&corner,XMVector3TransformCoord(XMVectorSet(.27f,.38f,.14f,1),plate));
        if(initialized)worstShield=std::max(worstShield,Distance(previousShield,corner));
        previousShield=corner;
      }
      previous=pose;initialized=true;
    }
  }
  std::cout<<"Equipped jump/landing per-ms hand "<<worstHand<<", tip "<<worstTip<<", shield "<<worstShield<<" m\n";
  Check(worstHand<.006f&&worstTip<.018f&&worstShield<.012f,"Equipped jump/landing snapped an attachment");
  // Rotate a solved forearm while keeping the torso fixed. The shield must
  // follow that exact rotation instead of retaining the old chest orientation.
  auto pose=BuildCharacterPose(0,0,true,{});const auto before=ShieldFrame(pose);
  const auto turn=XMMatrixRotationAxis(XMVector3Normalize(XMVectorSet(1,.3f,.2f,0)),.35f);
  XMStoreFloat3(&pose.arms[0].joint,XMLoadFloat3(&pose.arms[0].end)+XMVector3TransformNormal(XMLoadFloat3(&pose.arms[0].joint)-XMLoadFloat3(&pose.arms[0].end),turn));
  const auto after=ShieldFrame(pose);
  const auto neutral=BuildCharacterPose(0,0,true,{});
  const auto from=XMVector3Normalize(XMLoadFloat3(&neutral.arms[0].end)-XMLoadFloat3(&neutral.arms[0].joint));
  const auto target=XMVector3Normalize(XMLoadFloat3(&pose.arms[0].end)-XMLoadFloat3(&pose.arms[0].joint));
  Check(XMVectorGetX(XMVector3Length(XMVector3TransformNormal(from,after)-target))<.0001f,"Shield ignored the solved forearm rotation");
  Check(XMVectorGetX(XMVector3Length(before.r[2]-after.r[2]))>.05f,"Shield stayed rigid on the chest");
}
void Captures(){
  const auto root=std::filesystem::current_path()/"authored-locomotion-check";std::filesystem::create_directories(root);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Authored locomotion comparison",false),"Hidden comparison window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Comparison renderer");
  Walker before,after;auto outfit=CharacterOutfit::From(rpg::Inventory{},rpg::ClassId::Warrior);outfit.helm=false;outfit.shield=false;
  for(int frame=0;frame<90;++frame){
    const float speed=frame<18?2.f:frame<60?6.f:0.f;
    before.Update(1.f/30,speed,1,0,false);after.Update(1.f/30,speed,1,0);
    renderer.BeginFrame();renderer.SetCamera({0,1.7f,4.8f},{0,1.f,0});
    renderer.DrawMesh(visual::Mesh::Box,{0,-.1f,0},{8,.2f,8},{.24f,.23f,.21f,1},visual::Material::Stone);
    DrawCharacter(renderer,{-1,0},0,.45f,{.4f,.45f,.50f,1},0,0,outfit,before.state.pose,1);
    DrawCharacter(renderer,{1,0},0,.45f,{.4f,.45f,.50f,1},0,0,outfit,after.state.pose,2);
    renderer.DrawHudRect(.15f,.065f,.29f,.07f,{.035f,.03f,.025f,.92f});
    renderer.DrawHudRect(.61f,.065f,.29f,.07f,{.035f,.03f,.025f,.92f});
    renderer.DrawText(.18f,.08f,"Раньше",{1,.92f,.72f,1},2);
    renderer.DrawText(.64f,.08f,"Движения Blender",{1,.92f,.72f,1},2);
    Check(renderer.Capture((root/("compare-"+std::to_string(frame)+".png")).wstring())&&renderer.EndFrame(false),"Before/after movement capture");
  }
  for(unsigned direction=0;direction<4;++direction)for(unsigned pace=0;pace<2;++pace){
    Walker walk;for(int frame=0;frame<51;++frame)walk.Update(1.f/60,pace?6.f:2.f,
      direction==0?1.f:direction==1?-1.f:0.f,direction==2?-1.f:direction==3?1.f:0.f);
    renderer.BeginFrame();renderer.SetCamera({1.7f,1.7f,3.5f},{0,1.f,0});
    renderer.DrawMesh(visual::Mesh::Box,{0,-.1f,0},{8,.2f,8},{.24f,.23f,.21f,1},visual::Material::Stone);
    DrawCharacter(renderer,{0,0},0,0,{.4f,.45f,.50f,1},0,0,outfit,walk.state.pose,1);
    Check(renderer.Capture((root/("direction-"+std::to_string(direction)+"-"+std::to_string(pace)+".png")).wstring())&&renderer.EndFrame(false),"Directional native gait capture");
  }
  Check(renderer.ValidateDebugMessages(),"Authored locomotion GPU diagnostics");
  for(bool staff:{false,true})for(bool heavy:{false,true}) {
    animation::AuthoredWeaponController controller;
    auto kit=CharacterOutfit::From(rpg::Inventory{},staff?rpg::ClassId::Mage:rpg::ClassId::Warrior);
    kit.helm=false;kit.shield=false;
    for(int frame=0;frame<42;++frame) {
      const float t=frame/30.f;controller.Update(1.f/30,t<combat::Duration(heavy)?t:-1,heavy,staff,true);
      animation::Sample sample;sample.authoredGait=true;sample.authoredEquipment=staff?playerclips::Equipment::Staff:playerclips::Equipment::Sword;
      sample.authoredStrike=controller.frame;sample.authoredStrikeWeight=controller.weight;sample.authoredStrikePhase=controller.phase;sample.authoredStrikeHeavy=heavy;
      renderer.BeginFrame();renderer.SetCamera({1.9f,1.5f,3.8f},{0,.95f,0});
      renderer.DrawMesh(visual::Mesh::Box,{0,-.1f,0},{8,.2f,8},{.24f,.23f,.21f,1},visual::Material::Stone);
      DrawCharacter(renderer,{0,0},0,0,{.4f,.45f,.50f,1},0,0,kit,sample,1);
      Check(renderer.Capture((root/(std::string(staff?"staff-":"sword-")+(heavy?"heavy-":"basic-")+std::to_string(frame)+".png")).wstring())&&renderer.EndFrame(false),"V2 weapon capture");
    }
  }
}
}
int main(int argc,char** argv){try{
  if(argc==2&&std::string_view(argv[1])=="--grounded-only") {LibraryChecks();GroundedMotionChecks();return 0;}
  if(argc==2&&std::string_view(argv[1])=="--weapons-only") {WeaponMotionChecks();std::cout<<"PASS: v2 weapon motion\n";return 0;}
  if(argc==2&&std::string_view(argv[1])=="--capture-only") {Captures();return 0;}
  if(argc==2&&std::string_view(argv[1])=="--flight-only") {LibraryChecks();EquippedFlightChecks();return 0;}
  if(argc==2&&std::string_view(argv[1])=="--bow-only") {LibraryChecks();BowMotionChecks();WeaponCarryChecks();EquippedFlightChecks();std::cout<<"PASS: archery draw, release, carry and flight\n";return 0;}
  LibraryChecks();GroundedMotionChecks();NaturalGaitChecks();AuthoredLegChecks();CuttingGripChecks();ContinuousCycleChecks();LateralClearanceChecks();RecoveryTimingChecks();FlightClockChecks();ContactsAndTransitions();WeaponMotionChecks();LoadedArmRunningChecks();BowMotionChecks();WeaponCarryChecks();EquippedFlightChecks();
  if(!(argc==2&&std::string_view(argv[1])=="--motion-only"))Captures();
  std::cout<<"PASS: imported gait loops, directional weapon carry, equipped jumps, combat guards, contacts, transition continuity, gameplay isolation and native comparison\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
