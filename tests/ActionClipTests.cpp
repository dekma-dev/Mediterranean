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
    std::abs(a.right.z)+std::abs(a.pitch)+std::abs(a.yaw)+std::abs(a.roll)+std::abs(a.hip)+std::abs(a.open)+std::abs(a.aim);
}
Sample Motion(ClassId id,ActionClip clip,float t,int layer=0) {
  Sample result; result.action=SampleAction(id,clip,t);
  if(clip!=ActionClip::Restore && !RangedClass(id)) { result.attackSeconds=t; result.heavyAttack=clip==ActionClip::Heavy || clip==ActionClip::Technique || clip==ActionClip::Signature; }
  if(clip==ActionClip::Restore && id==ClassId::Warrior) result.cast=Gesture(t,.18f,.72f);
  if(layer==1) { result.air=.8f; result.rise=.5f; result.bank=.4f; }
  if(layer==2) { result.landing=.65f; result.lean=.5f; }
  if(layer==3) { result.dash=.75f; result.dashBrake=.4f; result.dashPhase=.4f; }
  return result;
}
void ClipsAndRig() {
  for(bool heavy:{false,true}) {
    Sample strike;strike.attackSeconds=heavy?.24f:.14f;strike.heavyAttack=heavy;
    const auto sword=BuildCharacterPose(0,0,false,strike,rpg::WeaponForm::Blade);
    const auto mace=BuildCharacterPose(0,0,false,strike,rpg::WeaponForm::Mace);
    const auto cleaver=BuildCharacterPose(0,0,false,strike,rpg::WeaponForm::Cleaver);
    Check(Distance(sword.arms[1].end,mace.arms[1].end)>.005f &&
      Distance(sword.arms[1].end,cleaver.arms[1].end)>.005f,
      "Mace and cleaver must have distinct solved attack paths, not just different meshes");
    Check(Distance(sword.weaponDirection,mace.weaponDirection)>.005f &&
      Distance(sword.weaponDirection,cleaver.weaponDirection)>.005f,
      "Melee weapon orientation must follow its own authored strike");
  }
  float largestHandStep=0,largestTipStep=0;
  std::string handCase,bladeCase;
  std::set<std::string> names;
  for(int id=0;id<int(ClassId::Count);++id) {
    names.insert(classActionClips[id].name);
    for(auto clip:{ActionClip::Basic,ActionClip::Heavy,ActionClip::Restore,ActionClip::Technique,ActionClip::Signature,ActionClip::Opening}) {
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
          const auto pose=BuildCharacterPose(0,.35f,shield,Motion(type,clip,t,layer),rpg::WeaponFor(type).form);
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
  // Check velocity continuity at authored anticipation/release/recovery keys.
  // A small sample displacement alone can miss a sharp change of velocity.
  constexpr float epsilon=.0001f;
  for(int id=0;id<int(ClassId::Count);++id) for(auto clip:{ActionClip::Technique,ActionClip::Signature,ActionClip::Opening}) {
    const auto type=ClassId(id);const bool heavy=clip!=ActionClip::Opening;
    const float release=RangedClass(type)?combat::StrokeStart(heavy):combat::Contact(heavy);
    for(float knot:{release*(heavy?.65f:.40f),release,release+.09f}) {
      const auto middle=SampleAction(type,clip,knot);
      auto incoming=(middle-SampleAction(type,clip,knot-epsilon))*(1/epsilon);
      auto outgoing=(SampleAction(type,clip,knot+epsilon)-middle)*(1/epsilon);
      // Normalized aim/openness saturate by design; test the spatial channels.
      incoming.open=incoming.aim=outgoing.open=outgoing.aim=0;
      if(Size(incoming-outgoing)>=.20f) std::cerr<<"Velocity key: "<<id<<'/'<<int(clip)<<'/'<<knot<<" delta "<<Size(incoming-outgoing)<<'\n';
      Check(Size(incoming-outgoing)<.20f,"Class action velocity broke at an authored key");
    }
  }
  for(float height:{0.f,.5f,2.f,5.f}) {
    const XMFLOAT3 palm{2,1.4f+height,-3};const rpg::Vec2 target{4,7};
    Check(Distance(CombatEffects::SpellTracePoint(palm,target,0),palm)<.00001f,"Spell trace must start at the solved palm");
    Check(Distance(CombatEffects::SpellTracePoint(palm,target,1),{4,1.25f,7})<.00001f,"Airborne cast must arrive on the grounded target");
    Check(Distance(CombatEffects::SpellTracePoint(palm,target,std::numeric_limits<float>::quiet_NaN()),palm)<.00001f,
      "Invalid trace age must not poison effect geometry");
  }
  std::array<ActionChannels,int(ClassId::Count)> cancellationReference{};
  for(int hz:{30,60,144,1000}) for(int id=0;id<int(ClassId::Count);++id) {
    ActionController c;float time=0;
    while(time<.21f-1e-6f) {const float dt=std::min(1.f/hz,.21f-time);time+=dt;c.Update(dt,ClassId(id),time,true,2,true);}
    const auto accepted=c.pose;
    Check(Size(accepted-SampleAction(ClassId(id),ActionClip::Heavy,.21f))<.0001f,"Action phase drifted from the accepted attack clock");
    time=0;
    while(time<.08f-1e-6f) {const float dt=std::min(1.f/hz,.08f-time);time+=dt;c.Update(dt,ClassId(id),-1,false,2,true);}
    if(hz==30) cancellationReference[id]=c.pose;
    else Check(Size(c.pose-cancellationReference[id])<.001f,"Interrupted release differs with refresh rate");
    Check(Size(c.pose)<Size(accepted)+.5f,"Cancelled action gained excessive momentum");
    while(time<.16f-1e-6f) {const float dt=std::min(1.f/hz,.16f-time);time+=dt;c.Update(dt,ClassId(id),-1,false,2,true);}
    Check(Size(c.pose)<.0001f,"Cancelled clip did not return to locomotion");
    c.Update(.01f,ClassId(id),.2f,true,2,true);
    c.Update(.01f,ClassId(id),-1,false,2,false);Check(Size(c.pose)==0,"Death retained an action pose");
  }
  ActionController c;c.Update(.01f,ClassId::Mage,.21f,true,2,true);const auto before=c.pose,beforeVelocity=c.velocity;
  c.Update(.001f,ClassId::Mage,-1,false,0,true);
  Check(Size((c.pose-before)*1000-beforeVelocity)<.25f,"A replacement action lost outgoing gesture velocity");
  c.Reset();Check(Size(c.pose)==0,"Load/reset must clear the action layer");
  c.Update(.01f,ClassId::Engineer,.21f,true,2,true);
  const auto held=c.pose,previousVelocity=c.velocity;
  c.Update(.0001f,ClassId::Engineer,-1,false,2,true);
  Check(Size((c.pose-held)*10000-previousVelocity)<.025f,"Cancellation must retain outgoing action velocity");
  for(int id=0;id<int(ClassId::Count);++id) for(float stop:{.08f,.16f,.24f,.35f,.50f}) {
    ActionController action;const auto type=ClassId(id);
    action.Update(.01f,type,stop,true,2,true);
    auto last=action.pose;const auto outgoing=action.velocity;
    action.Update(.0001f,type,-1,false,2,true);
    Check(Size((action.pose-last)*10000-outgoing)<.04f,"Interrupted class gesture lost momentum");
    for(int tick=0;tick<180;++tick) {
      last=action.pose;action.Update(.001f,type,-1,false,2,true);
      Check(std::isfinite(Size(action.pose)) && Size(action.pose-last)<.04f,"Inertial recovery snapped or diverged");
      Sample sample;sample.action=action.pose;
      const auto rig=BuildCharacterPose(0,0,false,sample,rpg::WeaponFor(type).form);
      for(const auto& arm:rig.arms) Check(std::abs(Distance(arm.root,arm.joint)-CharacterPose::upperArm)<.00001f &&
        std::abs(Distance(arm.joint,arm.end)-CharacterPose::forearm)<.00001f,"Inertial recovery stretched the arm");
    }
    Check(Size(action.pose)<.0001f && Size(action.velocity)<.0001f,"Inertial recovery retained residual motion");
  }
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
    const float releaseEnd=combat::StrokeStart(true)/ClassAttackRate(ClassId(id),true)+.03f;
    while(time<releaseEnd-1e-6f) {
      const float dt=std::min(1.f/hz,releaseEnd-time);game.ExecuteCommand(cmd,dt);plain.Update(dt,cmd);time+=dt;cmd={};
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
  ActionController recovery;recovery.Update(.01f,ClassId::Mage,.16f,true,2,true);
  for(int frame=0;frame<5;++frame) {
    if(frame) recovery.Update(.04f,ClassId::Mage,-1,false,2,true);
    Sample sample;sample.action=recovery.pose;
    renderer.BeginFrame();renderer.SetCamera({2.2f,1.7f,3.5f},{0,1.0f,0});
    DrawCharacter(renderer,{0,0},0,0,{.3f,.4f,.65f,1},0,0,CharacterOutfit::From(rpg::Inventory{},ClassId::Mage),sample);
    Check(renderer.Capture((root/("cancel-recovery-"+std::to_string(frame)+".png")).wstring()) && renderer.EndFrame(false),"Inertial action recovery capture");
  }
  for(float side:{-1.f,1.f}) {
    animation::State impact;impact.ApplyImpact(0,side);impact.Update(.05f,0,0,0,true,0);
    renderer.BeginFrame();renderer.SetCamera({2.2f,1.7f,3.5f},{0,1.0f,0});
    DrawCharacter(renderer,{0,0},0,0,{.36f,.45f,.60f,1},0,0,CharacterOutfit{},impact.pose);
    Check(renderer.Capture((root/(side<0?"left-impact.png":"right-impact.png")).wstring()) && renderer.EndFrame(false),"Directional impact capture");
  }
  for(int id=0;id<int(ClassId::Count);++id) for(int phase=0;phase<8;++phase) {
    const auto type=ClassId(id);const auto clip=phase==6?ActionClip::Opening:phase==5?ActionClip::Signature:phase==4?ActionClip::Technique:phase==3?ActionClip::Restore:ActionClip::Heavy;
    const float release=RangedClass(type)?combat::StrokeStart(clip!=ActionClip::Opening):combat::Contact(clip!=ActionClip::Opening);
    const float t=phase==0?release*.65f:phase==1?release:phase==2?release+.09f:phase>=4?release:.18f;
    auto sample=Motion(type,clip,t);sample.face.focus=.5f;
    const auto& color=rpg::DescribeClass(type).color;
    renderer.BeginFrame();renderer.SetCamera({2.2f,1.7f,3.5f},{0,1.0f,0});
    auto outfit=CharacterOutfit::From(rpg::Inventory{},type);
    if(phase==7) outfit.helm=true;
    DrawCharacter(renderer,{0,0},0,0,{color[0],color[1],color[2],1},0,0,outfit,sample,1);
    Check(renderer.Capture((root/(std::to_string(id)+"-"+std::to_string(phase)+".png")).wstring()) && renderer.EndFrame(false),"Class gesture GPU capture");
  }
  Check(renderer.ValidateDebugMessages(),"Class action D3D debug errors");
}
}
int main() {
  for(unsigned attack=0;attack<3;++attack) {
    Check(Size(SampleBossGesture(attack,0))==0 && Size(SampleBossGesture(attack,combat::Duration(true)))==0,
      "Boss gesture must return to neutral at its endpoints");
    ActionChannels previous;
    for(int i=0;i<760;++i) {
      const auto sample=SampleBossGesture(attack,i*.001f);
      Check(Size(sample-previous)<.012f,"Boss gesture must remain continuous across contact");
      Sample dynamics;dynamics.attackSeconds=i*.001f;dynamics.heavyAttack=true;dynamics.action=sample;
      const auto pose=BuildCharacterPose(0,0,false,dynamics);
      Check(pose.WeaponPoint(.96f).y>.05f,"Boss gesture drove its weapon through the floor");
      previous=sample;
    }
  }
  Check(Size(SampleBossGesture(0,.15f)-SampleBossGesture(1,.15f))>.1f &&
    Size(SampleBossGesture(1,.15f)-SampleBossGesture(2,.15f))>.1f,"Boss preparations must have distinct silhouettes");
  ClipsAndRig();Controller();Integration();Captures();
  std::cout<<"PASS: class actions, fixed-limb/wrist continuity, timed release, cancellation, save isolation and 88 GPU views\n";
}
