#include "game/Game.h"
#include "game/CharacterView.h"
#include "game/HandPose.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <set>

namespace {
using namespace DirectX;
using namespace animation;
using rpg::ClassId;
void Check(bool ok,const char* text) { if(!ok) { std::cerr<<text<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
float Size(ActionChannels a) {
  return std::abs(a.left.x)+std::abs(a.left.y)+std::abs(a.left.z)+std::abs(a.right.x)+std::abs(a.right.y)+
    std::abs(a.right.z)+std::abs(a.pitch)+std::abs(a.yaw)+std::abs(a.roll)+std::abs(a.hip)+std::abs(a.open);
}
Sample Motion(ClassId id,ActionClip clip,float t,int layer=0) {
  Sample result; result.action=SampleAction(id,clip,t);
  if(clip!=ActionClip::Restore && !RangedClass(id)) { result.attackSeconds=t; result.heavyAttack=clip==ActionClip::Heavy; }
  if(clip==ActionClip::Restore && id==ClassId::Warrior) result.cast=Gesture(t,.18f,.72f);
  if(layer==1) { result.air=.8f; result.rise=.5f; result.bank=.4f; }
  if(layer==2) { result.landing=.65f; result.lean=.5f; }
  if(layer==3) { result.dash=.75f; result.dashBrake=.4f; result.dashPhase=.4f; }
  return result;
}
void ClipsAndRig() {
  float largestHandStep=0,largestTipStep=0;
  std::string handCase,bladeCase;
  std::set<std::string> names;
  for(int id=0;id<int(ClassId::Count);++id) {
    names.insert(classActionClips[id].name);
    for(auto clip:{ActionClip::Basic,ActionClip::Heavy,ActionClip::Restore}) {
      const auto type=ClassId(id);const float end=ActionDuration(clip);
      Check(Size(SampleAction(type,clip,0))==0 && Size(SampleAction(type,clip,end))==0,"Clip endpoints must return to the movement layer");
      Check(Size(SampleAction(type,clip,.00001f))/.00001f<.08f &&
        Size(SampleAction(type,clip,end-.00001f))/.00001f<.08f,"Clip endpoint velocity must approach zero");
      for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        Check(Size(SampleAction(type,clip,bad))==0,"Invalid action age must be inert");
      for(int layer=0;layer<4;++layer) for(bool shield:{false,true}) {
        std::array<XMFLOAT3,2> previous{};XMFLOAT3 previousTip{};
        for(int tick=0;tick<=int(end*1000)+2;++tick) {
          const float t=tick*.001f;
          const auto pose=BuildCharacterPose(0,.35f,shield,Motion(type,clip,t,layer));
          for(int side=0;side<2;++side) {
            const auto& arm=pose.arms[side];const auto& leg=pose.legs[side];
            Check(std::abs(Distance(arm.root,arm.joint)-CharacterPose::upperArm)<.00001f &&
              std::abs(Distance(arm.joint,arm.end)-CharacterPose::forearm)<.00001f,"Action clip stretched an arm");
            Check(std::abs(Distance(leg.root,leg.joint)-CharacterPose::thigh)<.00001f &&
              std::abs(Distance(leg.joint,leg.end)-CharacterPose::shin)<.00001f,"Action clip stretched a leg");
            const auto hand=BuildHandPose(pose,unsigned(side),true,shield,pose.handCast);
            Check(std::abs(XMVectorGetX(XMMatrixDeterminant(XMLoadFloat4x4(&hand.frame)))-1)<.00002f,"Action hand frame lost orientation/volume");
            const auto finger=hand.Point(hand.fingers[0].joints[3]);
            if(tick && Distance(previous[side],finger)>largestHandStep) {
              largestHandStep=Distance(previous[side],finger);
              handCase=std::to_string(id)+"/"+std::to_string(int(clip))+"/"+std::to_string(layer)+"/"+std::to_string(shield)+"/"+std::to_string(tick)+"/"+std::to_string(side);
            }
            previous[side]=finger;
          }
          const auto tip=pose.WeaponPoint(.96f);
          Check(tip.y>.05f,"Class gesture drove the held blade into the ground");
          if(tick && Distance(previousTip,tip)>largestTipStep) {
            largestTipStep=Distance(previousTip,tip);
            bladeCase=std::to_string(id)+"/"+std::to_string(int(clip))+"/"+std::to_string(layer)+"/"+std::to_string(shield)+"/"+std::to_string(tick);
          }
          previousTip=tip;
          const auto release=SpellReleasePoint(pose,shield);
          Check(Distance(release,pose.arms[0].end)<.31f,"Spell release detached from its palm/equipment socket");
        }
      }
    }
  }
  Check(names.size()==11,"Each class needs a named action motif");
  std::cout<<"Maximum 1 ms finger/blade displacement: "<<largestHandStep<<" / "<<largestTipStep<<" at "<<handCase<<" / "<<bladeCase<<'\n';
  Check(largestHandStep<.012f && largestTipStep<.055f,"Class clip or wrist orientation snapped between 1 ms samples");
  Check(Size(SampleAction(ClassId::Count,ActionClip::Heavy,.2f))==0,"Invalid class sampled outside the clip table");
}
void Controller() {
  for(float height:{0.f,.5f,2.f,5.f}) {
    const XMFLOAT3 palm{2,1.4f+height,-3};const rpg::Vec2 target{4,7};
    Check(Distance(CombatEffects::SpellTracePoint(palm,target,0),palm)<.00001f,"Spell trace must start at the solved palm");
    Check(Distance(CombatEffects::SpellTracePoint(palm,target,1),{4,1.25f,7})<.00001f,"Airborne cast must arrive on the grounded target");
    Check(Distance(CombatEffects::SpellTracePoint(palm,target,std::numeric_limits<float>::quiet_NaN()),palm)<.00001f,
      "Invalid trace age must not poison effect geometry");
  }
  for(int hz:{30,60,144,1000}) for(int id=0;id<int(ClassId::Count);++id) {
    ActionController c;float time=0;
    while(time<.21f-1e-6f) {const float dt=std::min(1.f/hz,.21f-time);time+=dt;c.Update(dt,ClassId(id),time,true,2,true);}
    const auto accepted=c.pose;
    Check(Size(accepted-SampleAction(ClassId(id),ActionClip::Heavy,.21f))<.0001f,"Action phase drifted from the accepted attack clock");
    time=0;
    while(time<.08f-1e-6f) {const float dt=std::min(1.f/hz,.08f-time);time+=dt;c.Update(dt,ClassId(id),-1,false,2,true);}
    Check(Size(c.pose-accepted*.5f)<.0001f,"Interrupted release differs with refresh rate");
    while(time<.16f-1e-6f) {const float dt=std::min(1.f/hz,.16f-time);time+=dt;c.Update(dt,ClassId(id),-1,false,2,true);}
    Check(Size(c.pose)<.0001f,"Cancelled clip did not return to locomotion");
    c.Update(.01f,ClassId(id),.2f,true,2,true);
    c.Update(.01f,ClassId(id),-1,false,2,false);Check(Size(c.pose)==0,"Death retained an action pose");
  }
  ActionController c;c.Update(.01f,ClassId::Mage,.21f,true,2,true);const auto before=c.pose;
  c.Update(.001f,ClassId::Mage,-1,false,0,true);
  Check(Size(c.pose-before)<.003f,"A replacement action snapped the previous gesture away");
  c.Reset();Check(Size(c.pose)==0,"Load/reset must clear the action layer");
}
std::string Fixture(ClassId id) {
  const std::string duel="MEDITERRANEAN 3\n-4 5.2 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 0 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  rpg::Simulation s;Check(s.Load(duel),"Action fixture migration");auto save=s.Save();
  const auto p=save.find("PROFILE ");save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 0 0 0");return save;
}
void Integration() {
  for(int id=0;id<int(ClassId::Count);++id) for(int hz:{30,60,144,1000}) {
    Game game;rpg::Simulation plain;const auto snapshot=Fixture(ClassId(id));
    Check(game.RestoreSnapshot(snapshot) && plain.Load(snapshot),"Game action fixture");
    rpg::Command cmd;cmd.ability=1;cmd.nextTarget=true;
    game.ExecuteCommand(cmd,.01f);plain.Update(.01f,cmd);cmd={};
    float time=0;int launches=0;
    while(time<.24f-1e-6f) {
      const float dt=std::min(1.f/hz,.24f-time);game.ExecuteCommand(cmd,dt);plain.Update(dt,cmd);time+=dt;cmd={};
      for(std::size_t i=0;i<game.State().VisualEventCount();++i)
        launches+=game.State().VisualEvents()[i].kind==rpg::CombatVisualKind::SpellCast;
    }
    Check(game.State().Save()==plain.Save(),"Presentation changed authoritative combat/save state");
    Check(launches==(RangedClass(ClassId(id))?1:0),"Class gesture duplicated/lost its accepted spell release");
    Check(Size(game.PlayerAnimation().action-SampleAction(ClassId(id),ActionClip::Heavy,game.State().AttackSeconds()))<.0001f,
      "Game did not select the correct class clip/time");
    game.SuspendInput();
    for(int n=0;n<20;++n) game.ExecuteCommand({},.01f);
    Check(!game.State().Attacking() && Size(game.PlayerAnimation().action)<.0001f,"Focus interruption retained/replayed an action");
    Check(game.RestoreSnapshot(snapshot) && Size(game.PlayerAnimation().action)==0,"Load retained action history");
    cmd={};cmd.ability=3;game.ExecuteCommand(cmd,.01f);
    for(int n=0;n<18;++n) game.ExecuteCommand({},.01f);
    const auto pose=BuildCharacterPose(0,0,false,game.PlayerAnimation());
    Check(pose.arms[0].end.y>1.20f,"Restoration did not lift the class's casting hand");
  }
}
void Captures() {
  const auto root=std::filesystem::current_path()/"action-clip-check";std::filesystem::create_directories(root);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Class action checks",false),"Hidden action window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,720),"Action renderer");
  for(int id=0;id<int(ClassId::Count);++id) for(int phase=0;phase<4;++phase) {
    const auto type=ClassId(id);const auto clip=phase==3?ActionClip::Restore:ActionClip::Heavy;
    const float release=RangedClass(type)?combat::StrokeStart(true):combat::Contact(true);
    const float t=phase==0?release*.65f:phase==1?release:phase==2?release+.09f:.18f;
    auto sample=Motion(type,clip,t);sample.face.focus=.5f;
    const auto& color=rpg::DescribeClass(type).color;
    renderer.BeginFrame();renderer.SetCamera({2.2f,1.7f,3.5f},{0,1.0f,0});
    DrawCharacter(renderer,{0,0},0,0,{color[0],color[1],color[2],1},0,0,{},sample,1);
    Check(renderer.Capture((root/(std::to_string(id)+"-"+std::to_string(phase)+".png")).wstring()) && renderer.EndFrame(false),"Class gesture GPU capture");
  }
  Check(renderer.ValidateDebugMessages(),"Class action D3D debug errors");
}
}
int main() {
  ClipsAndRig();Controller();Integration();Captures();
  std::cout<<"PASS: class actions, fixed-limb/wrist continuity, timed release, cancellation, save isolation and 44 GPU views\n";
}
