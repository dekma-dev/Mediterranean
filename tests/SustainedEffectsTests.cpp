#include "game/SustainedEffects.h"
#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace {
using namespace rpg;using namespace DirectX;
constexpr float pi=3.14159265f;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
using Pixels=std::vector<unsigned char>;
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring())&&r.EndFrame(false),"Native sustained capture");
 Microsoft::WRL::ComPtr<IWICImagingFactory> f;Microsoft::WRL::ComPtr<IWICBitmapDecoder> d;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> c;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC");
 Check(SUCCEEDED(f->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d))&&
  SUCCEEDED(d->GetFrame(0,&frame))&&SUCCEEDED(f->CreateFormatConverter(&c))&&SUCCEEDED(c->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Decode capture");
 Pixels p(std::size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(c->CopyPixels(nullptr,r.Width()*4,UINT(p.size()),p.data())),"Capture pixels");return p;
}
double Difference(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Pixel dimensions");double sum=0;for(std::size_t i=0;i<a.size();++i)sum+=std::abs(int(a[i])-int(b[i]));return sum/a.size();}
Simulation Hero(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Legacy fixture");
 auto save=s.Save();const auto p=save.find("PROFILE ");save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 4 1 3");
 Check(s.Load(save),"Class fixture");auto& e=const_cast<Enemy&>(s.Enemies()[0]);e.health=e.MaxHealth();e.looted=false;e.state=EnemyState::Idle;return s;
}
Enemy& First(Simulation& s){return const_cast<Enemy&>(s.Enemies()[0]);}
void Advance(SustainedEffects& fx,const Simulation& s,float time,int hz=60){while(time>1e-6f){const float dt=std::min(time,1.f/hz);fx.Update(s,dt);time-=dt;}}
void Lifecycle(){
 auto s=Hero(ClassId::Mage);auto& e=First(s);e.rooted=2;SustainedEffects fx;
 fx.Update(s,1.f/60);Check(fx.Weight(100,0)>0 && fx.Weight(100,0)<.2f,"Control must fade in");
 Advance(fx,s,.2f);Check(fx.Weight(100,0)>.99f,"Control never becomes visible");
 auto& enemies=const_cast<std::vector<Enemy>&>(s.Enemies());std::reverse(enemies.begin(),enemies.end());fx.Update(s,.01f);
 Check(fx.Weight(100,0)>.99f,"Roster reorder reset status");
 auto& moved=*std::find_if(enemies.begin(),enemies.end(),[](const Enemy& a){return a.id==100;});
 moved.position.x+=2;fx.Update(s,.01f);Check(fx.Weight(100,0)<.1f,"Teleport retained old envelope");
 Advance(fx,s,.2f);moved.home.x+=1;fx.Update(s,.01f);Check(fx.Weight(100,0)<.1f,"Reused identity retained envelope");
 moved.health=0;fx.Update(s,.01f);Check(fx.ActorCount()==0,"Dead actor retained control aura");
 moved.health=100;Advance(fx,s,.2f);moved.rooted=0;fx.Update(s,.02f);
 Check(fx.Weight(100,0)>0 && fx.Weight(100,0)<1,"Control must fade out");Advance(fx,s,.22f);
 Check(fx.ActorCount()==0,"Expired control retained slot");
 moved.rooted=2;Advance(fx,s,.05f);const float weight=fx.Weight(100,0);
 for(float dt:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN()})fx.Update(s,dt);
 Check(fx.Weight(100,0)==weight,"Invalid time advanced state");fx.Reset();Check(fx.ActorCount()==0,"Reset retained actor slots");
 Check(sizeof(SustainedEffects)<4096,"Status storage escaped fixed budget");
 auto& beasts=const_cast<std::vector<Enemy>&>(s.Beasts());
 for(int n=0;n<14;++n){auto& b=beasts[n];b.health=b.MaxHealth();b.rooted=2;b.position={s.Player().x+1.f+n*.15f,s.Player().z+2};b.home=b.position;b.state=EnemyState::Idle;}
 Command target;target.selectActor=beasts[13].id;s.Update(.001f,target);Check(s.TargetActor() && s.TargetActor()->id==beasts[13].id,"Crowd target fixture");
 Advance(fx,s,.2f);Check(fx.ActorCount()==SustainedEffects::maxActors && fx.Weight(beasts[13].id,0)>.9f,"Crowd omitted selected status");
 beasts[13].state=EnemyState::Returning;fx.Update(s,.01f);Check(fx.Weight(beasts[13].id,0)==0,"Returning actor retained status");
 auto& dummy=const_cast<Enemy&>(s.Dummies()[0]);dummy.rooted=2;dummy.position={s.Player().x,s.Player().z+1};
 target.selectActor=dummy.id;s.Update(.001f,target);Advance(fx,s,.2f);Check(fx.Weight(dummy.id,0)>.9f,"Training dummy omitted control effect");
}
void Native(Renderer& r,const std::filesystem::path& out){
 const auto begin=[&](const Simulation& s){const auto p=s.Player();r.BeginFrame();r.SetCamera({p.x+3,3,p.z-4},{p.x,1,p.z+1.5f});
  r.DrawMesh(visual::Mesh::Terrain,{0,0,0},{1,1,1},{.23f,.25f,.22f,1},visual::Material::Stone);};
 for(auto id:{ClassId::Mage,ClassId::Scout,ClassId::Warrior}){
  auto s=Hero(id);First(s).rooted=2;Pixels baseline;
  for(int hz:{30,60,144}){SustainedEffects fx;Advance(fx,s,.09f,hz);begin(s);fx.Render(r,s,1,.3f);
   const auto p=Capture(r,out/("root-rise-"+std::to_string(int(id))+"-"+std::to_string(hz)+".png"));
   if(hz==30)baseline=p;else Check(Difference(baseline,p)<.03,"Control rise depends on refresh rate");
   First(s).rooted=0;Advance(fx,s,.07f,hz);begin(s);fx.Render(r,s,1,.3f);Capture(r,out/("root-fade-"+std::to_string(int(id))+"-"+std::to_string(hz)+".png"));
   Advance(fx,s,.3f,hz);begin(s);fx.Render(r,s,1,.3f);Check(r.EffectVertices()==0 && r.EndFrame(false),"Expired root renders");First(s).rooted=2;
  }
 }
 auto s=Hero(ClassId::Mage);auto& e=First(s);e.rooted=e.stunned=e.exposed=2;SustainedEffects fx;Advance(fx,s,.2f);
 const auto saved=s.Save();begin(s);fx.Render(r,s,1,0);const auto a=Capture(r,out/"statuses.png");
 begin(s);fx.Render(r,s,1,0);const auto b=Capture(r,out/"statuses-repeat.png");Check(a==b && saved==s.Save(),"Rendering advanced or changed status/gameplay");
 e.kind=EnemyKind::Boar;fx.Reset();Advance(fx,s,.2f);begin(s);fx.Render(r,s,1,0);Capture(r,out/"beast-statuses.png");
 // Borders are derived from the real enemy shapes and survive cosmetic saturation.
 auto& roster=const_cast<std::vector<Enemy>&>(s.Enemies());
 roster.resize(8);for(int n=0;n<8;++n){auto& foe=roster[n];foe.id=100+n;foe.kind=EnemyKind::Captain;foe.rank=1;
  foe.position={-4.f+(n%3)*.22f,7.f+(n/3)*.22f};foe.home=foe.position;foe.health=foe.MaxHealth();
  foe.state=EnemyState::Pursuing;foe.windup=.7f;foe.facing=pi;foe.bossAttack=0;}
 std::vector<presentation::Actor> actors;presentation::Bind(actors,s.Combatants(),s.SparringOpponent());
 begin(s);SustainedEffects::Warnings(r,s,&actors);const unsigned warningVertices=r.EffectVertices();
 Check(warningVertices>=3168 && warningVertices<=3600,"Eight quake borders missing or over budget");
 const auto original=Capture(r,out/"crowd-warnings.png");std::reverse(roster.begin(),roster.end());
 presentation::Bind(actors,s.Combatants(),s.SparringOpponent());
 begin(s);SustainedEffects::Warnings(r,s,&actors);const auto reordered=Capture(r,out/"crowd-warnings-reordered.png");
 Check(original==reordered,"Warning priority depends on roster order");
 begin(s);SustainedEffects::Warnings(r,s,&actors);for(int n=0;n<2000;++n)r.DrawEffectGlow({-4,1,7},.05f,{1,1,1,.01f});
 Check(r.EffectVertices()==Renderer::maxEffectVertices && r.EndFrame(false),"Cosmetics escaped shared warning budget");
 for(auto& foe:roster)foe.windup=0;
 begin(s);SustainedEffects::Warnings(r,s);Check(r.EffectVertices()==0 && r.EndFrame(false),"Interrupted cast left danger contour");
 Check(r.ValidateDebugMessages(),"Sustained DirectX warnings");
}
void World(Renderer& r,const std::filesystem::path& out){
 Check(r.Resize(1920,1080),"World resolution");
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}){
  auto fixture=Hero(id);Game game;game.Initialize();Check(game.RestoreSnapshot(fixture.Save()),"World fixture");
  auto& s=const_cast<Simulation&>(game.State());auto& e=First(s);e.attackCooldown=100;
  game.SetCameraOrbit(1.1f,.45f,7.f);Command cast;cast.ability=3;game.ExecuteCommand(cast,.01f);
  for(int n=0;n<150;++n)game.ExecuteCommand({},.01f);
  r.BeginFrame();game.Render(r);Capture(r,out/("world-defence-"+std::to_string(int(id))+".png"));
  e.rooted=e.stunned=e.exposed=2;for(int n=0;n<20;++n)game.ExecuteCommand({},.01f);
  r.BeginFrame();game.Render(r);Capture(r,out/("world-controls-"+std::to_string(int(id))+".png"));
  Check(game.RestoreSnapshot(fixture.Save()) && game.Sustained().ActorCount()==0,"Load retained control cache");
  auto& foe=First(const_cast<Simulation&>(game.State()));foe.attackCooldown=100;
  if(id==ClassId::Mage){cast={};cast.selectActor=100;cast.ability=8;game.ExecuteCommand(cast,.001f);
   for(int n=0;n<150;++n)game.ExecuteCommand({},.01f);Check(game.State().FrostFieldSeconds()>0,"Real frost field missing");
   game.SetCameraOrbit(.3f,.7f,10);r.BeginFrame();game.Render(r);Capture(r,out/"world-frost-field.png");
   const auto before=game.State().Save();r.BeginFrame();game.Render(r);Check(r.EndFrame(false) && game.State().Save()==before,"Field render mutated simulation");
  }
 }
 for(unsigned mode:{0u,1u,2u}){Game game;game.Initialize();Check(game.RestoreSnapshot(Hero(ClassId::Warrior).Save()),"Boss fixture");
  auto& boss=First(const_cast<Simulation&>(game.State()));boss.kind=EnemyKind::Captain;boss.rank=1;boss.health=boss.MaxHealth();boss.state=EnemyState::Pursuing;
  boss.windup=.8f;boss.bossAttack=mode;boss.facing=3.14159265f;game.ExecuteCommand({},.001f);game.SetCameraOrbit(.4f,.85f,11);
  r.BeginFrame();game.Render(r);Capture(r,out/("world-boss-"+std::to_string(mode)+".png"));
 }
}
}
int main(){try{
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");Lifecycle();
 const auto out=std::filesystem::current_path()/"sustained-effects-review";std::filesystem::create_directories(out);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Sustained effects checks",false),"Native window");
 Renderer r;Check(r.Initialize(window.GetHandle(),960,600),"Renderer");Native(r,out);World(r,out);
 Check(r.ValidateDebugMessages(),"World DirectX warnings");
 std::cout<<"PASS: status lifecycle, identities, priority, dummy controls, native equal-time capture, real fields and boss warnings.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
