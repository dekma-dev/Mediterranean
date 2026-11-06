#include "engine/Renderer.h"
#include "game/SpellVfx.h"
#include "game/ClassEffects.h"
#include "game/CombatEffects.h"
#include "game/Game.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <limits>
#include <chrono>
#include <algorithm>
namespace {
using namespace DirectX;using namespace rpg;using Art=Renderer::EffectArt;using K=CombatVisualKind;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Pixels=std::vector<unsigned char>;
Pixels Capture(Renderer& r,const std::filesystem::path& path){
 Check(r.Capture(path.wstring())&&r.EndFrame(false),"Native VFX capture");
 Microsoft::WRL::ComPtr<IWICImagingFactory> f;Microsoft::WRL::ComPtr<IWICBitmapDecoder> d;
 Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;Microsoft::WRL::ComPtr<IWICFormatConverter> c;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC factory");
 Check(SUCCEEDED(f->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d))&&
  SUCCEEDED(d->GetFrame(0,&frame))&&SUCCEEDED(f->CreateFormatConverter(&c))&&
  SUCCEEDED(c->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Read capture");
 Pixels p(std::size_t(r.Width())*r.Height()*4);Check(SUCCEEDED(c->CopyPixels(nullptr,r.Width()*4,UINT(p.size()),p.data())),"Read pixels");return p;
}
double Difference(const Pixels& a,const Pixels& b){Check(a.size()==b.size(),"Capture dimensions");double sum=0;for(std::size_t i=0;i<a.size();++i)sum+=std::abs(int(a[i])-int(b[i]));return sum/a.size();}
void Rendering(Renderer& r,const std::filesystem::path& out){
 Check(r.EffectTextureBytes()>2*1024*1024 && r.EffectTextureBytes()<3*1024*1024,"VFX atlas allocation budget");
 auto begin=[&]{r.BeginFrame();r.SetCamera({0,1,-5},{0,1,0});};
 for(int i=0;i<8;++i){begin();r.DrawEffectSprite(Art(i),{0,1,0},.9f,.9f,0,{1,1,1,1},i<=2);Capture(r,out/("material-"+std::to_string(i)+".png"));}
 Pixels sorted;
 for(bool reverse:{false,true}){
  begin();for(int n=0;n<2;++n){const int id=reverse?1-n:n;
   r.DrawEffectSprite(Art::Smoke,{0,1,id?.7f:0},.8f,.8f,0,id?XMFLOAT4{.2f,.3f,1,.75f}:XMFLOAT4{1,.3f,.2f,.75f},true);}
  auto p=Capture(r,out/(reverse?"cloud-order-reversed.png":"cloud-order.png"));
  if(!reverse)sorted=p;else Check(Difference(sorted,p)<.001,"Cloud compositing depends on emission order instead of depth");
 }
 const auto wall=[&]{begin();r.DrawMarkerBox({0,1,1},{4,4,.12f},{.2f,.2f,.2f,1});};
 wall();const auto base=Capture(r,out/"wall-base.png");
 wall();r.DrawEffectSprite(Art::Smoke,{0,1,.2f},.8f,.8f,0,{1,1,1,.8f},true);const auto visible=Capture(r,out/"wall-front.png");
 wall();r.DrawEffectSprite(Art::Smoke,{0,1,.90f},.8f,.8f,0,{1,1,1,.8f},true);const auto soft=Capture(r,out/"wall-soft.png");
 wall();r.DrawEffectSprite(Art::Smoke,{0,1,1.3f},.8f,.8f,0,{1,1,1,.8f},true);const auto hidden=Capture(r,out/"wall-hidden.png");
 const double strong=Difference(base,visible),weak=Difference(base,soft);
 Check(strong>.05 && weak>0 && weak<strong*.65,"Cloud intersection does not soften before opaque geometry");
 Check(Difference(base,hidden)<.001,"Cloud is visible through an opaque wall");
 begin();for(int i=0;i<1500;++i){r.DrawEffectSprite(Art::Smoke,{0,1,0},.2f,.2f,0,{1,1,1,.2f},true);r.DrawEffectSprite(Art::Flame,{0,1,0},.2f,.2f,0,{1,1,1,.2f});}
 Check(r.EffectVertices()==Renderer::maxEffectVertices,"Mixed alpha/additive workload escaped or failed to fill fixed budget");
 Check(r.EndFrame(false),"Saturated effect frame");
 begin();Check(r.EffectVertices()==0,"Frame retained old particle geometry");
 r.DrawEffectSprite(Art::Smoke,{0,1,0},std::numeric_limits<float>::quiet_NaN(),1,0,{1,1,1,1});
 Check(r.EffectVertices()==0,"Invalid sprite entered vertex buffer");
 Check(r.BeginPortrait(0,0,180,200),"Portrait start");r.DrawEffectSprite(Art::Flame,{0,1,0},1,1,0,{1,1,1,1});
 Check(r.EffectVertices()==0 && r.EndPortrait() && r.EndFrame(false),"Particles leaked into portrait");
 for(auto size:{std::pair<int,int>{1280,720},{960,600}}){Check(r.Resize(size.first,size.second),"Depth-resource resize");begin();r.DrawEffectSprite(Art::Flame,{0,1,0},.4f,.7f,0,{1,1,1,1});Check(r.EndFrame(false),"Resized effect frame");}
 Check(r.ValidateDebugMessages(),"VFX depth/blend/SRV state hazard");
}
void Lifetimes(Renderer& r,const std::filesystem::path& out){
 {
  CombatVisualEvent e;e.kind=K::SpellCast;e.school=ClassId::Mage;e.ability=13;e.heavy=true;e.flight=.4f;
  e.destination={0,0};e.destinationHeight=1.2f;const XMFLOAT3 origin{0,1.2f,-3};
  Pixels reference;
  for(int hz:{30,60,144}) {
   CombatEffects effects;effects.Emit(e,&origin);float time=0;
   while(time<.21f-1e-6f){const float dt=std::min(1.f/hz,.21f-time);effects.Advance(dt);time+=dt;}
   r.BeginFrame();r.SetCamera({2,2.1f,-4},{0,1,-1});effects.Render(r);
   Check(r.EffectVertices()>200 && r.EffectVertices()<900,"Prism flight lost its volume or exceeded the bounded trail budget");
   const auto pixels=Capture(r,out/("prism-flight-"+std::to_string(hz)+".png"));
   if(hz==30)reference=pixels;else Check(Difference(reference,pixels)<.03,"Prism flight depends on frame rate");
  }
  auto orb=e;orb.ability=10;
  r.BeginFrame();r.SetCamera({2,2.1f,-4},{0,1,-1});spellvfx::Flight(r,orb,origin,.21f);
  Check(Difference(reference,Capture(r,out/"arcane-orb-comparison.png"))>.02,"Prism still shares the generic arcane orb silhouette");
  const auto endpoint=spellvfx::ProjectilePosition(e,origin,1);
  Check(std::abs(endpoint.x-e.destination.x)<.0001f && std::abs(endpoint.z-e.destination.z)<.0001f,"Prism changed the accepted destination");
 }
 {CombatVisualEvent e;e.kind=K::SpellCast;e.school=ClassId::Mage;e.ability=11;e.heavy=true;e.flight=.75f;e.destination={0,0};
  CombatEffects fx;const XMFLOAT3 origin{0,1,-3};fx.Emit(e,&origin);for(int i=0;i<13;++i)fx.Advance(.05f);
  r.BeginFrame();r.SetCamera({1.5f,2.3f,-4},{0,1,0});fx.Render(r);
  Check(fx.BurstCount()==1 && r.EffectVertices()>0,"Slow projectile expired before its accepted flight ended");Capture(r,out/"slow-projectile.png");
  for(int i=0;i<7;++i)fx.Advance(.05f);Check(fx.BurstCount()==0,"Slow projectile never expired");}
 for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage})for(int ability:{0,1,5,12,13,14}){
  CombatVisualEvent e{K::HeavyHit,{0,0},1.2f,0,100};e.school=id;e.ability=ability;e.heavy=ability!=0 && ability!=12;e.chain=ability==14?3:0;
  Pixels equal;
  for(int hz:{30,60,144}){
   CombatEffects fx;fx.Emit(e);float time=0;while(time<.16f-1e-6f){const float dt=std::min(1.f/hz,.16f-time);fx.Advance(dt);time+=dt;}
   r.BeginFrame();r.SetCamera({1.5f,2.3f,-4},{0,1,0});fx.Render(r);Check(r.EffectVertices()<1800,"One impact exceeds its geometry budget");
   auto p=Capture(r,out/("contact-"+std::to_string(int(id))+"-"+std::to_string(ability)+"-"+std::to_string(hz)+".png"));
   if(hz==30)equal=p;else Check(Difference(equal,p)<.03,"Material layer depends on frame rate");
   while(time<1.01f){fx.Advance(.01f);time+=.01f;}
   r.BeginFrame();fx.Render(r);Check(fx.BurstCount()==0 && r.EffectVertices()==0 && r.EndFrame(false),"Contact residue survived lifetime");
  }
  CombatEffects fx;fx.Emit(e);for(int i=0;i<11;++i)fx.Advance(.05f);
  r.BeginFrame();r.SetCamera({1.5f,2.3f,-4},{0,1,0});fx.Render(r);
  if(id==ClassId::Mage || e.heavy || id==ClassId::Scout)Check(r.EffectVertices()>0,"Confirmed impact lost its delayed residue");
  Capture(r,out/("residue-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png"));
  fx.Reset();Check(fx.BurstCount()==0,"Reset retained VFX");
 }
}
std::string Fixture(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Native battle fixture");
 auto save=s.Save();const auto profile=save.find("PROFILE ");save.replace(profile,save.find('\n',profile)-profile,"PROFILE "+std::to_string(int(id))+" \"Tester\" 4 1 3");
 Check(s.Load(save),"Veteran class fixture");
 auto& target=const_cast<Enemy&>(s.Enemies()[0]);target.health=target.MaxHealth();target.looted=false;target.state=EnemyState::Idle;
 return s.Save();
}
void Workload(Renderer& r,const std::filesystem::path& out){
 Check(r.Resize(1920,1080),"Workload resolution");
 Game game;game.Initialize();Check(game.RestoreSnapshot(Fixture(ClassId::Mage)),"Workload fixture");
 game.SetCameraOrbit(1.1f,.32f,6.5f);
 std::ofstream report(out/"workload.txt");report<<r.AdapterDiagnostics()<<"\n1920x1080 hidden native window, vsync off.\n"
  <<"Same frozen world, 60 warm-up + 180 measured frames per phase.\n"
  <<"CPU includes submission/Present; GPU uses asynchronous originating-frame timestamps. Not interactive FPS.\n";
 const auto distribution=[&](const char* label,std::vector<double>& samples){
  report<<label<<" samples "<<samples.size();
  if(!samples.empty()){std::sort(samples.begin(),samples.end());report<<" p50 "<<samples[samples.size()/2]<<" p95 "<<samples[std::min(samples.size()-1,samples.size()*95/100)]<<" ms";}
  report<<'\n';
 };
 for(bool contacts:{false,true}){
  std::vector<double> cpu,gpu;unsigned peakVertices=0,peakDraws=0;
  const auto firstMeasured=r.GpuFrame()+61;
  for(int frame=0;frame<240;++frame){
   const auto start=std::chrono::steady_clock::now();r.BeginFrame();game.Render(r);
   if(contacts)for(int n=0;n<6;++n){
    CombatVisualEvent e;e.kind=K::HeavyHit;e.school=n%3==0?ClassId::Warrior:n%3==1?ClassId::Mage:ClassId::Scout;
    e.position={-5.f+(n%3)*1.1f,6.5f+(n/3)*1.3f};e.height=WorldMap::Height(e.position)+1.1f;
    e.actor=200+n;e.ability=n%2?14:1;e.heavy=true;e.chain=3;
    classfx::Impact(r,e,.025f+float((frame+n*7)%36)/60.f);
   }
   Check(r.EffectVertices()<=Renderer::maxEffectVertices,"World workload exceeded VFX budget");
   Check(r.EndFrame(false),"Native workload frame");
   if(frame>=60)cpu.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
   AppendGpuObservations(gpu,r.GpuReady(),r.GpuReadyCount(),firstMeasured);
   peakVertices=std::max(peakVertices,r.EffectVertices());peakDraws=std::max(peakDraws,r.LastDrawCalls());
  }
  report<<(contacts?"Six simultaneous mixed-class contacts\n":"World without contacts\n");
  distribution("CPU",cpu);distribution("GPU",gpu);report<<"peak effect vertices "<<peakVertices<<" peak draw calls "<<peakDraws<<'\n';
 }
 Check(r.ValidateDebugMessages(),"Workload DirectX warnings");
}
void World(Renderer& r,const std::filesystem::path& out){
 Check(r.Resize(1920,1080),"World review resolution");
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout})for(int ability:{1,5,13,14}){
  Game game;game.Initialize();Check(game.RestoreSnapshot(Fixture(id)),"World review load");
  game.SetCameraOrbit(1.1f,.32f,6.5f);Command c;c.selectActor=100;c.ability=ability;game.ExecuteCommand(c,.001f);
  Check(game.State().Attacking(),"World review attack rejected");
  const std::string stem=std::to_string(int(id))+"-"+std::to_string(ability);
  const auto tick=[&](float time){while(time>1e-6f){const float dt=std::min(time,1.f/120);game.ExecuteCommand({},dt);time-=dt;}};
  tick(game.State().Spell(ability).contact*.25f);
  r.BeginFrame();game.Render(r);Capture(r,out/("world-prepare-"+stem+".png"));
  bool hit=false;for(int n=0;n<600 && !hit;++n){game.ExecuteCommand({},1.f/120);for(std::size_t v=0;v<game.State().VisualEventCount();++v)hit|=game.State().VisualEvents()[v].kind==K::Hit || game.State().VisualEvents()[v].kind==K::HeavyHit;}
  Check(hit,"World attack never contacted");tick(.12f);
  const auto saved=game.State().Save();r.BeginFrame();game.Render(r);Capture(r,out/("world-contact-"+stem+".png"));
  Check(game.State().Save()==saved,"VFX rendering changed gameplay");tick(.38f);
  r.BeginFrame();game.Render(r);Capture(r,out/("world-residue-"+stem+".png"));
  Check(game.RestoreSnapshot(saved) && game.Effects().BurstCount()==0,"Load retained textured residue");
 }
}
}
int main(int argc,char** argv){try{
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");
 const auto out=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("vfx-material-review");std::filesystem::create_directories(out);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"VFX material review",false),"Native window");
 Renderer r;Check(r.Initialize(window.GetHandle(),960,600),"Native renderer");
 if(argc>2 && std::string(argv[2])=="--workload"){Workload(r,out);std::cout<<"PASS: bounded native VFX workload; see workload.txt.\n";return 0;}
 Rendering(r,out);Lifetimes(r,out);World(r,out);Check(r.ValidateDebugMessages(),"Native VFX warnings");
 std::cout<<"PASS: original atlas, alpha ordering, soft depth, fixed budgets, lifecycle, equal-time contacts and actual world attacks. Atlas bytes: "<<r.EffectTextureBytes()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
