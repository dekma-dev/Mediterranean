#include "game/ClassEffects.h"
#include "game/CombatEffects.h"
#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "engine/Renderer.h"
#include "engine/Audio.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cmath>

namespace {
using namespace rpg;
using namespace DirectX;
using K=CombatVisualKind;
void Check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
using Pixels=std::vector<unsigned char>;
Pixels Capture(Renderer& r,const std::filesystem::path& path) {
  Check(r.Capture(path.wstring()) && r.EndFrame(false),"Class effect capture");
  Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
  Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
  Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
  Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))),"WIC factory");
  Check(SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"WIC decode");
  Pixels pixels(std::size_t(r.Width())*r.Height()*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,UINT(r.Width()*4),UINT(pixels.size()),pixels.data())),"WIC pixels");
  return pixels;
}
double Difference(const Pixels& a,const Pixels& b) {
  Check(a.size()==b.size(),"Image dimensions");double sum=0;
  for(std::size_t i=0;i<a.size();++i) sum+=std::abs(int(a[i])-int(b[i]));
  return sum/a.size();
}
std::size_t VisiblePixels(const Pixels& a,const Pixels& b) {
  Check(a.size()==b.size(),"Image dimensions");std::size_t count=0;
  for(std::size_t i=0;i<a.size();i+=4) {
    int contrast=0;for(int c=0;c<3;++c) contrast=std::max(contrast,std::abs(int(a[i+c])-int(b[i+c])));
    count+=contrast>12;
  }
  return count;
}
void Tick(Simulation& s,float seconds) {
  while(seconds>1e-6f) {const float dt=std::min(seconds,.01f);s.Update(dt,{});seconds-=dt;}
}
Simulation Training(ClassId id) {
  Simulation s;
  Check(s.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Training fixture");
  auto save=s.Save();const auto p=save.find("PROFILE ");
  save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 0 0 0");
  Check(s.Load(save),"Training class");Command equip;
  equip.inventory={InventoryAction::Equip,s.Items().Find(Item::Starter),int(GearSlot::Weapon),Item::Starter};s.Update(.001f,equip);
  Command train;train.interact=true;s.Update(.001f,train);Check(s.Sparring(),"Training start");
  return s;
}
void Stage(Renderer& r,ClassId id,bool release,bool restore=false) {
  Simulation hero;Check(hero.CreateCharacter(id,"Tester"),"Stage class");
  r.BeginFrame();r.SetCamera({0,2.8f,-6.5f},{0,1,0});
  r.DrawMesh(visual::Mesh::Box,{0,-.08f,0},{9,.12f,5},{.24f,.24f,.23f,1},visual::Material::Stone);
  animation::Sample pose;
  pose.action=animation::SampleAction(id,restore?animation::ActionClip::Restore:animation::ActionClip::Heavy,restore?.26f:release?combat::StrokeStart(true):.39f);
  if(!restore && hero.ClassInfo().range<=3) {pose.attackSeconds=release?combat::StrokeStart(true):.39f;pose.heavyAttack=true;}
  const auto& c=hero.ClassInfo().color;
  DrawCharacter(r,{-1.25f,0},0,1.5707963f,{c[0],c[1],c[2],1},0,0,CharacterOutfit::From(hero.Items(),id),pose,1);
  DrawCharacter(r,{1.25f,0},0,-1.5707963f,{.43f,.29f,.24f,1},0,0,{}, {},2);
}
void Captures(Renderer& r,const std::filesystem::path& root) {
  unsigned maxVertices=0;
  for(int i=0;i<int(ClassId::Count);++i) {
    const auto id=ClassId(i);const auto stem=std::to_string(i);
    CombatVisualEvent e{K::HeavyHit,{1.25f,0},1.2f,1.5707963f,100};
    e.school=id;e.heavy=true;e.source={-1.25f,0};
    Stage(r,id,false);const auto base=Capture(r,root/(stem+"-base.png"));
    Pixels impact;
    for(bool empowered:{false,true}) {
      e.empowered=empowered;CombatEffects effects;effects.Emit(e);effects.Advance(.05f);effects.Advance(.05f);
      Stage(r,id,false);effects.Render(r);
      maxVertices=std::max(maxVertices,r.EffectVertices());
      Check(r.EffectVertices()>0 && r.EffectVertices()<1600,"One class impact exceeded its vertex budget");
      const auto frame=Capture(r,root/(stem+(empowered?"-empowered.png":"-impact.png")));
      // Small physical contacts should have crisp local contrast; averaging
      // brightness over the entire sky would reward oversized spell glows.
      Check(VisiblePixels(base,frame)>60,"Class impact is not visible beside the target");
      if(empowered) Check(VisiblePixels(impact,frame)>60,"Resource payoff has no distinct impact layer");else impact=frame;
    }
    // Every emitter must end; a late render cannot manufacture more particles.
    CombatEffects ended;ended.Emit(e);for(int n=0;n<20;++n) ended.Advance(.05f);
    Stage(r,id,false);ended.Render(r);Check(r.EffectVertices()==0,"Class impact outlived its pool lifetime");
    Check(r.EndFrame(false),"Expired effect frame");
    if(DescribeClass(id).range<=3) continue;
    e.kind=K::SpellCast;e.position=e.source;e.height=0;e.destination={1.25f,0};e.flight=.09f/ClassAttackRate(id,true);
    const XMFLOAT3 origin{-1.0f,1.35f,-.12f};
    Pixels first;
    for(int hz:{30,60,144,1000}) {
      CombatEffects effects;effects.Emit(e,&origin);float age=0;
      while(age<e.flight*.65f-1e-7f) {const float dt=std::min(1.f/hz,e.flight*.65f-age);effects.Advance(dt);age+=dt;}
      Stage(r,id,true);effects.Render(r);
      Check(r.EffectVertices()>0 && r.EffectVertices()<1000,"Class projectile missing or oversized");
      const auto frame=Capture(r,root/(stem+"-flight-"+std::to_string(hz)+".png"));
      if(hz==30) first=frame;else Check(Difference(first,frame)<.03,"Class projectile depends on refresh rate");
    }
  }
  std::cout<<"Maximum single impact vertices: "<<maxVertices<<"; fixed pool bytes: "<<sizeof(CombatEffects)<<'\n';
}
void Recoveries(Renderer& r,const std::filesystem::path& root) {
  for(int i=0;i<int(ClassId::Count);++i) {
    const auto id=ClassId(i);auto s=Training(id);const_cast<rpg::Enemy&>(s.SparringOpponent()).attackCooldown=100;const_cast<rpg::Enemy&>(s.SparringOpponent()).windup=0;Command heal;heal.ability=3;s.Update(.001f,heal);
    if(s.CastingRecovery()) {
      // Keep the fixture stationary and actors distant during cast validation.
      for(auto& e:const_cast<std::vector<rpg::Enemy>&>(s.Enemies())) {e.position=e.home={25,25};e.windup=0;e.state=rpg::EnemyState::Idle;}
      while(s.CastingRecovery()) s.Update(.01f,{});
    }
    Check(s.VisualEventCount()==1 && s.VisualEvents()[0].kind==K::Mend && s.VisualEvents()[0].school==id,"Recovery event identity");
    sound::Events audio;audio.Update(s);
    Check(audio.count==1 && audio.cues[0]==sound::ClassRecoveryCue(id),"Recovery sound identity");
    auto event=s.VisualEvents()[0];event.position={-1.25f,0};
    Stage(r,id,false,true);const auto base=Capture(r,root/(std::to_string(i)+"-recovery-base.png"));
    Pixels first;
    for(int hz:{30,60,144,1000}) {
      CombatEffects effects;effects.Emit(event);float age=0;
      while(age<.26f-1e-7f) {const float dt=std::min(1.f/hz,.26f-age);effects.Advance(dt);age+=dt;}
      Stage(r,id,false,true);effects.Render(r);
      Check(r.EffectVertices()>0 && r.EffectVertices()<900,"Recovery vertex budget");
      const auto frame=Capture(r,root/(std::to_string(i)+"-recovery-"+std::to_string(hz)+".png"));
      Check(VisiblePixels(base,frame)>30,"Recovery is not visible");
      if(hz==30) first=frame;else Check(Difference(first,frame)<.03,"Recovery depends on refresh rate");
      for(int n=0;n<20;++n) effects.Advance(.05f);
      Stage(r,id,false,true);effects.Render(r);Check(r.EffectVertices()==0 && r.EndFrame(false),"Recovery residue never expired");
    }
  }
}
void LiveStates(Renderer& r,const std::filesystem::path& root) {
  const auto begin=[&](const Simulation& s) {
    const auto p=s.Player();r.BeginFrame();r.SetCamera({p.x-3,3,p.z-4},{p.x,1,p.z+1});
  };
  for(auto id:{ClassId::Paladin,ClassId::Priest,ClassId::Scout,ClassId::Druid,ClassId::Psionic,ClassId::Engineer}) {
    auto s=Training(id);
    begin(s);classfx::States(r,s,2);Check(r.EffectVertices()==0 && r.EndFrame(false),"Inactive states emit idle spam");
    for(int i=0;i<s.CombatStyleInfo().capacity;++i) {Command c;c.ability=0;s.Update(.001f,c);Tick(s,1);}
    Command heavy;heavy.ability=1;s.Update(.001f,heavy);Tick(s,s.Spell(1).contact+.001f);
    const auto before=s.Save();begin(s);classfx::States(r,s,2);
    Check(r.EffectVertices()>0 && r.EffectVertices()<700,"Real class payoff lacks bounded live-state geometry");
    const auto p=s.Player();const auto target=s.SparringOpponent().position;
    DrawCharacter(r,p,0,0,{.4f,.4f,.5f,1},0,0,CharacterOutfit::From(s.Items(),id));
    DrawCharacter(r,target,0,3.14f,{.5f,.3f,.2f,1},0,0);
    Capture(r,root/(std::to_string(int(id))+"-status.png"));
    Check(s.Save()==before,"Drawing a class status changed gameplay");
    // Temporal controls expire; barriers are a capacity, not a visual timer.
    if(id!=ClassId::Paladin && id!=ClassId::Priest) {
      Tick(s,5.1f);begin(s);classfx::States(r,s,3);
      Check(r.EffectVertices()==0 && r.EndFrame(false),"Expired status left a cosmetic aura");
    }
    Check(s.Load(before),"Reload state");begin(s);classfx::States(r,s,2);
    Check(r.EffectVertices()==0 && r.EndFrame(false),"Load replayed a transient ward/control state");
  }
  for(auto id:{ClassId::Mage,ClassId::Scout,ClassId::Engineer,ClassId::Priest,ClassId::Necromancer,ClassId::Psionic,ClassId::Bard,ClassId::Druid}) {
    auto s=Training(id);Command heavy;heavy.ability=1;s.Update(.01f,heavy);Tick(s,.06f);
    const auto p=s.Player();const XMFLOAT3 palm{p.x,s.Height()+1.4f,p.z};
    begin(s);classfx::Prepare(r,s,palm);
    Check(r.EffectVertices()>0 && r.EndFrame(false),"Accepted preparation has no hand/weapon cue");
    s.StopMovement();begin(s);classfx::Prepare(r,s,palm);
    Check(r.EffectVertices()==0 && r.EndFrame(false),"Cancelled preparation left a false charge");
    Tick(s,3);s.Update(.001f,heavy);Tick(s,combat::StrokeStart(true)/s.AttackRate()+.002f);
    begin(s);classfx::Prepare(r,s,palm);
    Check(r.EffectVertices()==0 && r.EndFrame(false),"Released projectile retained a preparation orb");
  }
}
void SchoolHues(Renderer& r,const std::filesystem::path& root) {
  const auto begin=[&] {
    r.BeginFrame();r.SetCamera({0,1,-4},{0,1,0});
    r.DrawMesh(visual::Mesh::Box,{0,1,0},{3,3,.2f},{.04f,.04f,.04f,1},visual::Material::Plain);
  };
  begin();const auto base=Capture(r,root/"hue-baseline.png");
  for(auto id:{ClassId::Mage,ClassId::Druid,ClassId::Psionic}) {
    begin();r.DrawEffectGlow({0,1,-1},.35f,classfx::Color(id,.65f));
    const auto frame=Capture(r,root/("hue-"+std::to_string(int(id))+".png"));
    double energy[3]{};
    for(std::size_t n=0;n<frame.size();n+=4) for(int c=0;c<3;++c) energy[c]+=std::max(0,int(frame[n+c])-int(base[n+c]));
    if(id==ClassId::Mage) Check(energy[0]>energy[2]*2,"Fire core lost its warm hue");
    if(id==ClassId::Druid) Check(energy[1]>energy[0]*1.6,"Nature core washed out to white");
    if(id==ClassId::Psionic) Check(energy[2]>energy[1]*1.6,"Mind core lost its violet hue");
  }
}
}
int main() {
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
  const auto root=std::filesystem::current_path()/"class-effects-check";std::filesystem::create_directories(root);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,640,L"Class effects checks",false),"Hidden window");
  Renderer r;Check(r.Initialize(window.GetHandle(),960,640),"D3D renderer");
  Captures(r,root);Recoveries(r,root);LiveStates(r,root);SchoolHues(r,root);
  Check(sizeof(CombatEffects)<=16384,"Class effects exceeded the fixed CPU budget");
  Check(r.ValidateDebugMessages(),"D3D validation errors");
  std::cout<<"PASS: 11 class impacts, empowered layers, timed projectiles, live states, cancellation and GPU captures\n";
}
