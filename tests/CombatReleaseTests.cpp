#include "game/Game.h"
#include "game/ClassEffects.h"
#include "game/SpellVfx.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace {
using namespace rpg;using namespace DirectX;using K=CombatVisualKind;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Pixels=std::vector<unsigned char>;
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring())&&r.EndFrame(false),"Release capture");
 Microsoft::WRL::ComPtr<IWICImagingFactory> f;Microsoft::WRL::ComPtr<IWICBitmapDecoder> d;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> c;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC");
 Check(SUCCEEDED(f->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d))&&
  SUCCEEDED(d->GetFrame(0,&frame))&&SUCCEEDED(f->CreateFormatConverter(&c))&&SUCCEEDED(c->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Read capture");
 Pixels p(std::size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(c->CopyPixels(nullptr,r.Width()*4,UINT(p.size()),p.data())),"Pixels");return p;
}
double Difference(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Dimensions");double sum=0;for(std::size_t i=0;i<a.size();++i)sum+=std::abs(int(a[i])-int(b[i]));return sum/a.size();}
void Tick(CombatEffects& fx,float time,int hz){while(time>1e-6f){const float dt=std::min(time,1.f/hz);fx.Advance(dt);time-=dt;}}
Simulation Hero(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Hero");
 auto save=s.Save();const auto p=save.find("PROFILE ");save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 4 1 3");
 Check(s.Load(save),"Class");auto& e=const_cast<Enemy&>(s.Enemies()[0]);e.health=e.MaxHealth();e.looted=false;e.state=EnemyState::Idle;return s;
}
void TargetHeights(){
 for(auto kind:{EnemyKind::Raider,EnemyKind::Wolf,EnemyKind::Boar}){
  auto s=Hero(ClassId::Mage);auto& victim=const_cast<Enemy&>(s.Enemies()[0]);victim.kind=kind;victim.health=500;victim.attackCooldown=100;
  victim.position={-4,9.5f};victim.home=victim.position;const float expected=WorldMap::Height(victim.position)+victim.ContactHeight();
  Command c;c.selectActor=victim.id;c.ability=0;s.Update(.001f,c);
  bool release=false,hit=false;
  for(int n=0;n<600 && !hit;++n){s.Update(1.f/120,{});for(std::size_t i=0;i<s.VisualEventCount();++i){const auto& e=s.VisualEvents()[i];
    if(e.kind==K::SpellCast){release=true;Check(e.destinationHeight==victim.ContactHeight(),"Release did not snapshot target height");
     const auto end=CombatEffects::SpellTracePoint({-4,3,5.2f},e.destination,1,.28f,e.destinationHeight);
     Check(std::abs(end.y-expected)<.0001f,"Projectile endpoint missed target surface");}
    if(e.kind==K::Hit){hit=true;Check(std::abs(e.height-expected)<.0001f,"Hit does not match target surface");}
   }}
  Check(release && hit,"Height regression did not exercise an actual spell/contact");
 }
}
void Native(Renderer& r,const std::filesystem::path& out){
 const auto begin=[&]{r.BeginFrame();r.SetCamera({6,6,-8},{0,.8f,1});
  r.DrawMesh(visual::Mesh::Terrain,{0,0,0},{1,1,1},{.23f,.25f,.22f,1},visual::Material::Stone);};
 for(K kind:{K::EnemyQuake,K::EnemyTempest,K::EnemyExecution,K::EnemyLance}){
  CombatVisualEvent e;e.kind=kind;e.position={0,0};e.facing=.3f;e.actor=200;
  for(float age:{.025f,.12f,.30f,.56f}){
   Pixels first;
   for(int hz:{30,60,144}){CombatEffects fx;fx.Emit(e);Tick(fx,age,hz);begin();fx.Render(r);
    Check(r.EffectVertices()<=1800,"One hostile release over budget");
    const auto image=Capture(r,out/("enemy-"+std::to_string(int(kind))+"-"+std::to_string(int(age*1000))+"-"+std::to_string(hz)+".png"));
    if(hz==30)first=image;else Check(Difference(first,image)<.03,"Release depends on refresh rate");
    Tick(fx,1.1f,hz);begin();fx.Render(r);Check(fx.BurstCount()==0 && r.EffectVertices()==0 && r.EndFrame(false),"Release failed to expire");
   }
  }
 }
 for(auto id:{ClassId::Mage,ClassId::Scout})for(int ability:{0,1,5,12,13,14})for(float age:{.02f,.10f,.22f,.29f}){
  CombatVisualEvent e;e.kind=K::SpellCast;e.school=id;e.ability=ability;e.heavy=ability!=0 && ability!=12;
  e.position={-2,0};e.destination={2,2};e.flight=.24f;e.facing=.78f;const XMFLOAT3 origin{-2,1.35f,0};
  CombatEffects fx;fx.Emit(e,&origin);Tick(fx,age,60);begin();fx.Render(r);Check(r.EffectVertices()<1200,"Projectile over budget");
  Capture(r,out/("flight-"+std::to_string(int(id))+"-"+std::to_string(ability)+"-"+std::to_string(int(age*1000))+".png"));
 }
 for(auto id:{ClassId::Mage,ClassId::Scout}){
  CombatVisualEvent e;e.kind=K::SpellCast;e.school=id;e.ability=14;e.flight=.24f;e.destination={2,2};
  const XMFLOAT3 origin{-2,1.35f,0};Pixels first;
  for(int hz:{30,60,144}){CombatEffects fx;fx.Emit(e,&origin);Tick(fx,.13f,hz);begin();fx.Render(r);
   auto p=Capture(r,out/("flight-equal-"+std::to_string(int(id))+"-"+std::to_string(hz)+".png"));
   if(hz==30)first=p;else Check(Difference(first,p)<.03,"Projectile depends on refresh rate");}
  begin();for(float age:{-1.f,std::numeric_limits<float>::quiet_NaN(),.45f})classfx::Projectile(r,e,origin,age);
  Check(r.EffectVertices()==0 && r.EndFrame(false),"Invalid or expired projectile renders");
 }
 Pixels crowd;
 for(bool reverse:{false,true}){
  CombatEffects fx;for(int n=0;n<8;++n){const int id=reverse?7-n:n;CombatVisualEvent e;e.kind=K::EnemyTempest;e.actor=200+id;e.position={float(id%3)*.4f,float(id/3)*.4f};fx.Emit(e);}
  Tick(fx,.06f,60);begin();fx.Render(r);Check(r.EffectVertices()>=2800 && r.EffectVertices()<=3300,"Eight releases escaped crowd budget");
  const auto p=Capture(r,out/(reverse?"crowd-reversed.png":"crowd.png"));if(!reverse)crowd=p;else Check(crowd==p,"Release order changed compositing");
 }
}
void World(Renderer& r,const std::filesystem::path& out){
 Check(r.Resize(1600,900),"World size");
 for(unsigned mode:{0u,1u,2u}){
  Game game;game.Initialize();Check(game.RestoreSnapshot(Hero(ClassId::Warrior).Save()),"Boss fixture");
  auto& boss=const_cast<Enemy&>(game.State().Enemies()[0]);boss.kind=EnemyKind::Captain;boss.rank=1;boss.health=boss.MaxHealth();boss.state=EnemyState::Pursuing;
  boss.windup=.01f;boss.bossAttack=mode;boss.facing=3.14159265f;game.SetCameraOrbit(.4f,.85f,11);
  game.ExecuteCommand({},.01f);bool released=false;for(std::size_t i=0;i<game.State().VisualEventCount();++i){auto k=game.State().VisualEvents()[i].kind;released|=k==K::EnemyQuake || k==K::EnemyTempest || k==K::EnemyExecution;}
  Check(released,"Boss did not actually release");
  for(int n=0;n<12;++n)game.ExecuteCommand({},.01f);
  const auto saved=game.State().Save();r.BeginFrame();game.Render(r);const auto a=Capture(r,out/("world-boss-release-"+std::to_string(mode)+".png"));
  r.BeginFrame();game.Render(r);const auto b=Capture(r,out/("world-boss-repeat-"+std::to_string(mode)+".png"));
  Check(a==b && saved==game.State().Save(),"Rendering mutates release/gameplay");
  Check(game.RestoreSnapshot(Hero(ClassId::Warrior).Save()) && game.Effects().BurstCount()==0,"Load retained release");
 }
 for(auto id:{ClassId::Mage,ClassId::Scout})for(int ability:{0,5,13,14}){
  Game game;game.Initialize();Check(game.RestoreSnapshot(Hero(id).Save()),"Player release fixture");
  auto& enemy=const_cast<Enemy&>(game.State().Enemies()[0]);enemy.position={-4,10.5f};enemy.home=enemy.position;enemy.attackCooldown=100;enemy.health=500;
  game.SetCameraOrbit(1.1f,.45f,8);Command cast;cast.selectActor=100;cast.ability=ability;game.ExecuteCommand(cast,.001f);
  bool released=false;for(int n=0;n<600 && !released;++n){game.ExecuteCommand({},1.f/120);for(std::size_t i=0;i<game.State().VisualEventCount();++i)released|=game.State().VisualEvents()[i].kind==K::SpellCast;}
  Check(released,"Actual player release missing");game.ExecuteCommand({},.025f);
  r.BeginFrame();game.Render(r);Capture(r,out/("world-player-release-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png"));
  for(int n=0;n<100;++n)game.ExecuteCommand({},.01f);
  Check(game.RestoreSnapshot(Hero(id).Save()),"Cancel fixture");cast={};cast.selectActor=100;cast.ability=1;game.ExecuteCommand(cast,.001f);
  cast={};cast.haltMovement=true;game.ExecuteCommand(cast,.001f);Check(game.Effects().BurstCount()==0,"Cancelled preparation emitted release");
 }
}
}
int main(int argc,char** argv){try{
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");TargetHeights();
 const auto out=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("combat-release-review");std::filesystem::create_directories(out);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Combat release checks",false),"Window");
 Renderer r;Check(r.Initialize(window.GetHandle(),960,600),"Renderer");Native(r,out);World(r,out);
 Check(r.ValidateDebugMessages(),"DirectX release warnings");std::cout<<"PASS: native release sequence, equal-time geometry, bounded lifetime, actual boss contacts and render/load isolation.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
