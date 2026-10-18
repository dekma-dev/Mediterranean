#include "game/Game.h"
#include "game/CharacterPose.h"
#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using namespace DirectX;
void Require(bool ok,const char* message) { if (!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
XMFLOAT3 World(XMFLOAT3 p,rpg::Vec2 root,float yaw) {
  return {root.x+std::cos(yaw)*p.x+std::sin(yaw)*p.z,p.y,root.z-std::sin(yaw)*p.x+std::cos(yaw)*p.z};
}
void Geometry(const CharacterPose& pose) {
  for (const auto& leg : pose.legs) {
    Require(std::isfinite(leg.end.x) && std::isfinite(leg.end.y) && std::isfinite(leg.end.z),"Finite ankle targets");
    Require(leg.end.y>=.1699f,"Boots must not penetrate the flat floor");
    Require(std::abs(Distance(leg.root,leg.joint)-CharacterPose::thigh)<.0001f &&
      std::abs(Distance(leg.joint,leg.end)-CharacterPose::shin)<.0001f,"Foot placement must preserve limb lengths");
  }
  Require(pose.WeaponPoint(.96f).y>.03f,"Pelvis compensation must keep the resting sword above ground");
}
animation::Sample Walk(int hz,float speed,bool curved=false) {
  animation::FootPlacement feet; feet.Reset({0,0},0); animation::Sample sample;
  std::array<XMFLOAT3,2> previous{};
  bool planted[2]={false,false}; float previousYaw[2]={};
  float worstSlip=0; int contacts=0,lifts=0;
  for (int frame=1;frame<=hz*2;++frame) {
    const float t=float(frame)/hz,yaw=curved?t*.8f:0;
    const rpg::Vec2 root=curved?rpg::Vec2{speed/.8f*(1-std::cos(yaw)),speed/.8f*std::sin(yaw)}:rpg::Vec2{0,t*speed};
    feet.Update(1.f/hz,root,yaw,true,true,sample);
    const auto pose=BuildCharacterPose(0,0,false,sample); Geometry(pose);
    for (int i=0;i<2;++i) {
      const auto actual=World(pose.legs[i].end,root,yaw);
      if (planted[i] && feet.Planted(i)) {
        worstSlip=std::max(worstSlip,Distance(actual,previous[i])); ++contacts;
        Require(std::abs(motion::AngleDelta(previousYaw[i],yaw+pose.footYaw[i]))<.0001f,"Planted boots must not rotate with the body");
      }
      if (actual.y>.19f) ++lifts;
      previous[i]=actual; previousYaw[i]=yaw+pose.footYaw[i]; planted[i]=feet.Planted(i);
    }
  }
  std::cout<<"Support slip / "<<hz<<" Hz / speed "<<speed<<" / curved "<<curved<<": "<<worstSlip<<"; contacts "<<contacts<<"; lifts "<<lifts<<'\n';
  Require(contacts>hz/3 && lifts>hz/3,"Walking needs both grounded support and lifted recovery phases");
  Require(worstSlip<.002f,"World-space support feet must remain planted, including solved geometry");
  return sample;
}
}
int main() {
  for (float speed : {1.f,4.4f,7.f}) {
    const auto reference=Walk(30,speed);
    for (int hz : {60,144}) {
      const auto actual=Walk(hz,speed);
      for (int i=0;i<2;++i) {
        const float error=std::hypot(reference.feet[i].z-actual.feet[i].z,reference.feet[i].y-actual.feet[i].y);
        std::cout<<"Equal-time ankle difference / "<<hz<<" Hz / side "<<i<<": "<<error<<'\n';
        Require(error<.065f,
          "Equal-time walking poses must remain close across 30/60/144 Hz");
      }
    }
    Walk(60,speed,true);
  }
  animation::FootPlacement feet; animation::Sample sample;
  feet.Reset({0,0},0);
  for (int i=1;i<=120;++i) feet.Update(1.f/60,{0,float(i)*4.4f/60},0,true,true,sample);
  for (int i=0;i<120;++i) feet.Update(1.f/60,{0,8.8f},0,true,true,sample);
  for (int i=0;i<2;++i) Require(feet.Planted(i) && std::abs(sample.feet[i].z)<.066f,"Stopping must settle into a quiet stance without ongoing steps");
  const auto still=sample;
  for (int i=0;i<120;++i) feet.Update(1.f/60,{0,8.8f},0,true,true,sample);
  for (int i=0;i<2;++i) Require(sample.feet[i].x==still.feet[i].x && sample.feet[i].z==still.feet[i].z &&
    sample.feet[i].y==still.feet[i].y,"Idle must not keep shuffling after settling");
  for (int i=1;i<=120;++i) {
    feet.Update(1.f/60,{0,8.8f},float(i)*motion::tau/120,true,true,sample);
    Geometry(BuildCharacterPose(0,0,false,sample));
  }
  feet.Update(.02f,{0,8.8f},0,false,true,sample); Require(!sample.feetActive,"Jumping must release world contacts");
  feet.Update(.02f,{0,8.8f},0,true,true,sample);
  Require(sample.feetActive && feet.Planted(0) && feet.Planted(1) && std::abs(sample.feet[0].z)<.001f,"Landing must establish fresh local contacts");
  feet.Update(.02f,{10,-20},1,true,true,sample);
  Require(std::abs(sample.feet[0].x+.18f)<.001f && std::abs(sample.feet[0].z)<.001f,"Teleport must discard the old support anchors");
  feet.Update(.02f,{10,-20},1,true,false,sample); Require(!sample.feetActive,"Death must release stepping state");
  for (float bad : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) {
    feet.Update(bad,{10,-20},1,true,true,sample); Require(!sample.feetActive,"Invalid time must not publish a new foot pose");
  }
  sample.feetActive=true; sample.feet[0]={100,-20,100,std::numeric_limits<float>::infinity()};
  sample.feet[1]={std::numeric_limits<float>::quiet_NaN(),100,-100,0}; Geometry(BuildCharacterPose(0,0,false,sample));

  Game game; game.Initialize(); rpg::Simulation reference;
  for (int i=0;i<150;++i) {
    rpg::Command command; command.movement={i<80?1.f:-1.f,0}; command.jump=i==30; command.ability=i==100?2:-1;
    game.ExecuteCommand(command,1.f/60); reference.Update(1.f/60,command);
    Require(game.State().Save()==reference.Save(),"Foot placement cannot change movement, combat or persistent state");
    Require(game.PlayerAnimation().feetActive==(game.State().Height()==0 && game.State().Health()>0),"Player contact activation must follow actual ground state");
    Geometry(BuildCharacterPose(game.PlayerPose().attack,game.PlayerPose().gait,false,game.PlayerAnimation()));
  }
  const auto saved=game.State().Save(); Require(game.RestoreSnapshot(saved),"Foot-placement save fixture");
  Require(!game.PlayerAnimation().feetActive,"Reload must clear transient foot samples");
  game.ExecuteCommand({},.01f); Require(game.PlayerAnimation().feetActive,"Grounded reload must recreate local contacts");
  Require(sizeof(animation::FootPlacement)<=256,"Foot controller must remain a small fixed allocation per actor");

  // Repeated reversals with uneven updates must remain finite/reachable and stop.
  feet.Reset({0,0},0); rpg::Vec2 zigzag{};
  for (int frame=0;frame<300;++frame) {
    const float intervals[]={.05f,.007f,.013f,.021f},dt=intervals[frame%4];
    zigzag.z+=(frame/20%2?-7.f:7.f)*dt;
    feet.Update(dt,zigzag,frame/20%2?motion::tau*.5f:0,true,true,sample);
    Geometry(BuildCharacterPose(.5f,1,true,sample));
  }

  // Close-up sequence with ground/shadow contact, using the live foot controller.
  Require(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
  Win32Window window; Require(window.Create(GetModuleHandle(nullptr),960,720,L"Foot placement checks",false),"Hidden test window");
  Renderer renderer; Require(renderer.Initialize(window.GetHandle(),960,720),"Foot-placement renderer");
  feet.Reset({0,0},0); sample={};
  rpg::Vec2 root{}; float yaw=0;
  const wchar_t* names[]={L"walk-start",L"walk-support",L"walk-recovery",L"run",L"stop",L"turn",L"landing",L"equipment"};
  for (int view=0;view<8;++view) {
    for (int frame=0;frame<(view==0?3:view<3?5:30);++frame) {
      if (view<4) root.z+=(view==3?7.f:4.4f)/60;
      if (view==5) yaw+=1.5f/30;
      feet.Update(1.f/60,root,yaw,true,true,sample);
    }
    sample.landing=view==6?.8f:0;
    renderer.BeginFrame(); renderer.SetCamera({root.x+2.6f,1.4f,root.z+3.3f},{root.x,.9f,root.z});
    renderer.DrawMesh(visual::Mesh::Box,{root.x,-.07f,root.z},{8,.14f,8},{.40f,.43f,.39f,1},visual::Material::Stone);
    CharacterOutfit outfit; outfit.armor=outfit.helm=outfit.shield=view==7;
    DrawCharacter(renderer,root,0,yaw,{.22f,.37f,.54f,1},0,0,outfit,sample);
    const auto path=L"feet-"+std::wstring(names[view])+L".png";
    Require(renderer.Capture(path) && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Foot-placement inspection capture");
  }
  game.Initialize();
  const auto liveCapture=[&](const wchar_t* path) {
    const auto durable=game.State().Save(); const auto before=game.PlayerAnimation();
    renderer.BeginFrame(); game.Render(renderer,false);
    Require(renderer.Capture(path) && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Live foot-placement capture");
    renderer.BeginFrame(); game.Render(renderer,false); Require(renderer.EndFrame(false),"Repeated live foot render");
    Require(game.State().Save()==durable,"Rendering feet must not change simulation");
    for (int i=0;i<2;++i) Require(game.PlayerAnimation().feet[i].x==before.feet[i].x &&
      game.PlayerAnimation().feet[i].z==before.feet[i].z && game.PlayerAnimation().feet[i].y==before.feet[i].y,
      "Rendering must not advance a foot step");
  };
  rpg::Command walk; walk.movement={0,1};
  for (int i=0;i<32;++i) game.ExecuteCommand(walk,1.f/60);
  game.SetCameraOrbit(2.2f,.28f,4.8f); liveCapture(L"feet-live-walk.png");
  walk.movement={1,0};
  for (int i=0;i<6;++i) game.ExecuteCommand(walk,1.f/60);
  liveCapture(L"feet-live-turn.png");
  const std::string approach="MEDITERRANEAN 3\n-4 3.8 100 0 1 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(game.RestoreSnapshot(approach),"Enemy approach fixture");
  for (int i=0;i<30;++i) game.ExecuteCommand({},1.f/60);
  Require(game.EnemyAnimation(0).feetActive && game.State().Enemies()[0].movementDistance>0,"Approaching enemies must use planted-foot stepping");
  game.SetCameraOrbit(1.1f,.28f,5); liveCapture(L"feet-live-enemy.png");
  rpg::Command attack; attack.nextTarget=true; attack.ability=1;
  for (int i=0;i<30;++i) game.ExecuteCommand({},1.f/60);
  game.ExecuteCommand(attack,.01f);
  for (int i=0;i<7;++i) game.ExecuteCommand({},.02f);
  Require(game.Effects().TrailCount()>0,"Combat stance capture must contain an accepted strike");
  liveCapture(L"feet-live-combat.png");
  std::cout<<"PASS: planted support, fixed joints, turning, stop/jump/landing/reset, authoritative-state isolation and twelve foot-placement captures. Controller bytes: "<<sizeof(animation::FootPlacement)<<'\n';
  CoUninitialize();
}
