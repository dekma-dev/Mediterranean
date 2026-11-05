#include "game/CastEffects.h"
#include "game/Game.h"
#include "game/ClassEffects.h"
#include "game/HandPose.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <limits>

namespace {
using namespace rpg;using namespace DirectX;using K=CombatVisualKind;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Pixels=std::vector<unsigned char>;
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring())&&r.EndFrame(false),"Cast capture");
 Microsoft::WRL::ComPtr<IWICImagingFactory> f;Microsoft::WRL::ComPtr<IWICBitmapDecoder> d;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> c;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC");
 Check(SUCCEEDED(f->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d))&&
  SUCCEEDED(d->GetFrame(0,&frame))&&SUCCEEDED(f->CreateFormatConverter(&c))&&SUCCEEDED(c->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Decode");
 Pixels p(std::size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(c->CopyPixels(nullptr,r.Width()*4,UINT(p.size()),p.data())),"Pixels");return p;
}
double Difference(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Dimensions");double sum=0;for(std::size_t i=0;i<a.size();++i)sum+=std::abs(int(a[i])-int(b[i]));return sum/a.size();}
Simulation Hero(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Hero");
 auto save=s.Save();const auto p=save.find("PROFILE ");save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 7 1 3");
 Check(s.Load(save),"Class");return s;
}
template<class F>void Advance(float time,int hz,F step){while(time>1e-6f){const float dt=std::min(time,1.f/hz);step(dt);time-=dt;}}
void Timing(){
 for(auto id:{ClassId::Paladin,ClassId::Priest,ClassId::Druid,ClassId::Bard,ClassId::Engineer})for(int hz:{30,60,144}){
  auto s=Hero(id);Command c;c.ability=3;s.Update(.001f,c);Check(s.CastingRecovery(),"Cast did not start");
  const float health=s.Health(),duration=s.RecoveryCastDuration();
  Advance(duration-.013f,hz,[&](float dt){s.Update(dt,{});});Check(s.Health()==health,"Recovery happened before completion");
  const float remaining=(1-s.RecoveryProgress())*duration;s.Update(.031f,{});
  Check(!s.CastingRecovery(),"Recovery did not finish");bool found=false;
  for(std::size_t i=0;i<s.VisualEventCount();++i)if(s.VisualEvents()[i].kind==K::Mend){found=true;Check(std::abs(s.VisualEvents()[i].elapsed-(.031f-remaining))<.00003f,"Completion lost its sub-frame age");}
  Check(found,"Completion did not emit effect");
  for(int mode=0;mode<3;++mode){auto cancelled=Hero(id);cancelled.Update(.001f,c);cancelled.Update(.08f,{});
   Command stop;if(mode==0)stop.movement={1,0};else if(mode==1)stop.jump=true;else stop.haltMovement=true;
   cancelled.Update(.01f,stop);Check(!cancelled.CastingRecovery() && cancelled.Health()==health,"Interrupted recovery changed health");
   for(std::size_t i=0;i<cancelled.VisualEventCount();++i)Check(cancelled.VisualEvents()[i].kind!=K::Mend,"Cancelled recovery emitted a false completion");}
 }
}
void AttachmentChecks(Renderer& r,const std::filesystem::path& out){
 const auto pose=BuildCharacterPose(0,0,false);
 for(bool weapon:{false,true})for(bool shield:{false,true}){
  const auto p=BuildCharacterPose(.2f,.4f,shield);
  const auto a=castfx::FromPose(p,{},0,WeaponFor(ClassId::Warrior).form,shield,weapon);
  for(unsigned side=0;side<2;++side){const auto expected=BuildHandPose(p,side,weapon,shield,p.handCast).Point({0,.045f,.025f});
   const auto actual=side?a.right:a.left;
   Check(std::abs(expected.x-actual.x)+std::abs(expected.y-actual.y)+std::abs(expected.z-actual.z)<.00001f,"Effect hand ignores equipment grip");}
 }
 for(int id=0;id<int(ClassId::Count);++id){
  const auto school=ClassId(id);auto a=castfx::FromPose(pose,{0,0,0},.4f,WeaponFor(school).form,false);
  CombatVisualEvent e;e.kind=K::Mend;e.school=school;e.position={-20,20};CombatEffects fx;fx.Emit(e);Advance(.22f,60,[&](float dt){fx.Advance(dt);});
  const auto begin=[&](XMFLOAT3 offset){r.BeginFrame();r.SetCamera({offset.x+2.7f,offset.y+2.2f,offset.z-4},{offset.x,offset.y+1,offset.z});};
  begin({});fx.Render(r,&a);Check(r.EffectVertices()>0 && r.EffectVertices()<900,"Attached recovery absent/over budget");
  const auto original=Capture(r,out/("attached-"+std::to_string(id)+".png"));
  auto moved=castfx::FromPose(pose,{2,0,1},.4f,WeaponFor(school).form,false);begin({2,0,1});fx.Render(r,&moved);
  const auto translated=Capture(r,out/("translated-"+std::to_string(id)+".png"));
  Check(Difference(original,translated)<.04,"Recovery remained at its old emission point");
  a.alive=false;begin({});fx.Render(r,&a);Check(r.EffectVertices()==0 && r.EndFrame(false),"Dead actor retained recovery");
  a.alive=true;a.focus.x=std::numeric_limits<float>::quiet_NaN();begin({});fx.Render(r,&a);Check(r.EffectVertices()==0 && r.EndFrame(false),"Invalid socket submitted geometry");
  fx.Reset();Check(!fx.HasRecovery(),"Reset retained attached pulse");
 }
 CombatEffects potion;CombatVisualEvent e;e.kind=K::Potion;potion.Emit(e);potion.Advance(.05f);
 auto dead=castfx::Preview({},{0,1,0},0);dead.alive=false;r.BeginFrame();potion.Render(r,&dead);Check(r.EffectVertices()==0 && r.EndFrame(false),"Dead actor retained potion aura");
}
void Preparations(Renderer& r,const std::filesystem::path& out){
 for(auto id:{ClassId::Mage,ClassId::Scout})for(int ability:{1,5,8,9,13,14}){
  Pixels reference;
  for(int hz:{30,60,144}){
   auto s=Hero(id);auto& target=const_cast<Enemy&>(s.Enemies()[0]);target.position=target.home={-4,7};target.state=EnemyState::Idle;target.health=target.MaxHealth();target.attackCooldown=100;target.looted=false;
   Command c;c.selectActor=100;c.ability=ability;s.Update(.001f,c);Check(s.Attacking(),"Preparation fixture rejected");
   const float time=combat::StrokeStart(s.HeavyAttack())/s.AttackRate()*.70f;Advance(time,hz,[&](float dt){s.Update(dt,{});});
   const auto a=castfx::Preview({0,0,0},{-.2f,1.3f,.3f},.45f);r.BeginFrame();r.SetCamera({2.7f,2.4f,-4},{0,.9f,0});
   castfx::Prepare(r,s,a);Check(r.EffectVertices()>0 && r.EffectVertices()<512,"Preparation budget");
   const auto image=Capture(r,out/("prepare-"+std::to_string(int(id))+"-"+std::to_string(ability)+"-"+std::to_string(hz)+".png"));
   if(hz==30)reference=image;else Check(Difference(reference,image)<.04,"Preparation depends on refresh rate");
   s.StopMovement();r.BeginFrame();castfx::Prepare(r,s,a);Check(r.EffectVertices()==0 && r.EndFrame(false),"Cancelled preparation retained charge");
  }
 }
}
void World(Renderer& r,const std::filesystem::path& out){
 Check(r.Resize(1600,900),"World resolution");
 const auto tick=[](Game& g,float seconds){Advance(seconds,120,[&](float dt){g.ExecuteCommand({},dt);});};
 for(int id=0;id<int(ClassId::Count);++id){
  Game g;g.Initialize();Check(g.RestoreSnapshot(Hero(ClassId(id)).Save()),"World recovery fixture");g.SetCameraOrbit(.55f,.40f,6.5f);
  Command cast;cast.ability=3;g.ExecuteCommand(cast,.001f);
  if(g.State().CastingRecovery()){tick(g,g.State().RecoveryCastDuration()*.65f);r.BeginFrame();g.Render(r);Capture(r,out/("world-charge-"+std::to_string(id)+".png"));
   while(g.State().CastingRecovery())g.ExecuteCommand({},.01f);}
  tick(g,.18f);const auto saved=g.State().Save();r.BeginFrame();g.Render(r);const auto image=Capture(r,out/("world-complete-"+std::to_string(id)+".png"));
  r.BeginFrame();g.Render(r);const auto repeat=Capture(r,out/("world-repeat-"+std::to_string(id)+".png"));Check(image==repeat && saved==g.State().Save(),"Draw changed simulation/timing");
  Command walk;walk.movement={1,0};for(int n=0;n<15;++n)g.ExecuteCommand(walk,.01f);
  r.BeginFrame();g.Render(r);Capture(r,out/("world-moving-"+std::to_string(id)+".png"));
  Check(g.RestoreSnapshot(Hero(ClassId(id)).Save()) && !g.Effects().HasRecovery(),"Load replayed recovery");
 }
 for(auto id:{ClassId::Mage,ClassId::Scout})for(int ability:{9,14}){
  Game g;g.Initialize();Check(g.RestoreSnapshot(Hero(id).Save()),"Strong cast fixture");auto& e=const_cast<Enemy&>(g.State().Enemies()[0]);e.position=e.home={-4,7};e.state=EnemyState::Idle;e.health=e.MaxHealth();e.attackCooldown=100;e.looted=false;
  Command c;c.selectActor=100;c.ability=ability;g.ExecuteCommand(c,.001f);Check(g.State().Attacking(),"Strong cast rejected");
  tick(g,combat::StrokeStart(g.State().HeavyAttack())/g.State().AttackRate()*.82f);
  for(float yaw:{.55f,2.4f}){g.SetCameraOrbit(yaw,.40f,6.5f);r.BeginFrame();g.Render(r);Capture(r,out/("world-prepare-"+std::to_string(int(id))+"-"+std::to_string(ability)+"-"+std::to_string(int(yaw*10))+".png"));}
  g.SuspendInput();Check(!g.State().Attacking() && !g.Effects().HasRecovery(),"Focus loss left a preparation");
 }
}
}
int main(){try{
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");Timing();
 const auto out=std::filesystem::current_path()/"casting-effects-review";std::filesystem::create_directories(out);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Casting and attached recovery",false),"Window");
 Renderer r;Check(r.Initialize(window.GetHandle(),960,600),"Renderer");AttachmentChecks(r,out);Preparations(r,out);World(r,out);
 Check(r.ValidateDebugMessages(),"DirectX validation");std::cout<<"PASS: eleven attached recoveries, cast clocks/cancellation, frame-rate equality, actual pose/world captures and reset isolation.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
