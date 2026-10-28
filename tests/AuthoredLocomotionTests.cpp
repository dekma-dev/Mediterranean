#include "game/AuthoredLocomotion.h"
#include "game/FootPlacement.h"
#include "game/CharacterPose.h"
#include "game/CharacterView.h"
#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <cstring>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
float Distance(XMFLOAT3 a,XMFLOAT3 b){return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b)));}
float Difference(const playerclips::Frame& a,const playerclips::Frame& b){
  float value=std::abs(a.hip-b.hip);
  for(unsigned i=0;i<2;++i)value=std::max({value,Distance(a.hands[i],b.hands[i]),Distance(a.feet[i],b.feet[i])});
  return std::max(value,Distance(a.torso,b.torso));
}
void LibraryChecks(){
  const auto bytes=playerclips::EmbeddedBytes();const auto clips=playerclips::Decode(bytes);
  Check(clips.size()==14&&bytes.size()<48*1024,"Four actions and ten movement tracks must fit the small motion budget");
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
    Check(walkPeak>.025f&&runPeak>walkPeak+.015f&&runPeak<=.145f,"Walk/run need different bounded recovery heights");
    Check(playerclips::SwingHeight(w,1,0,0)==0&&playerclips::SwingHeight(w,1,1,1)==0,"Contact endpoints must be flat");
  }
  const auto rejected=[](std::string data){try{playerclips::Decode(data);return false;}catch(const std::exception&){return true;}};
  Check(rejected(std::string(bytes.substr(0,bytes.size()-1))),"Truncated gait archive");
  auto corrupt=std::string(bytes);corrupt[8]=4;Check(rejected(corrupt),"Stale four-action resource must be rejected");
  std::size_t offset=12;for(unsigned i=0;i<4;++i)offset+=8+clips[i].frames.size()*sizeof(playerclips::Frame);
  corrupt=std::string(bytes);float bad=.5f;std::memcpy(corrupt.data()+offset+8,&bad,sizeof(bad));
  Check(rejected(corrupt),"Broken imported loop must be rejected");
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
void ContactsAndTransitions(){
  float worstSlip=0,worstJump=0;playerclips::Frame reference;
  for(int hz:{30,60,144,1000}){
    Walker walk;std::array<XMFLOAT3,2> previous{};bool planted[2]={};
    for(int i=0;i<hz*2;++i){
      walk.Update(1.f/hz,6,0,1);const auto pose=BuildCharacterPose(0,0,false,walk.state.pose);
      for(int side=0;side<2;++side){
        auto p=pose.legs[side].end;p.x+=walk.position.x;p.z+=walk.position.z;
        if(planted[side]&&walk.feet.Planted(side))worstSlip=std::max(worstSlip,Distance(p,previous[side]));
        Check(p.y>=.1699f&&p.y<=.316f,"Authored swing pierced floor or escaped reach budget");
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
    for(int side=0;side<2;++side)worstJump=std::max(worstJump,Distance(previous.arms[side].end,pose.arms[side].end));
    previous=pose;
  }
  std::cout<<"Authored gait: maximum planted slip "<<worstSlip<<" m, hand change per ms "<<worstJump<<" m\n";
  Check(worstSlip<.002f,"Authored pelvis displaced world-space foot anchors");
  Check(worstJump<.012f,"Start/stop/reversal caused a visible hand snap");
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
}
}
int main(){try{LibraryChecks();ContactsAndTransitions();Captures();std::cout<<"PASS: imported gait loops, walk/run distinction, contacts, transition continuity, gameplay isolation and native comparison\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
