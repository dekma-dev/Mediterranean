#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using K=rpg::CombatVisualKind;
using namespace DirectX;
using Microsoft::WRL::ComPtr;
void Require(bool ok,const char* message) { if (!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); } }
const std::string duel="MEDITERRANEAN 3\n-4 5.2 100 0 1 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 0 0 1 0.5 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
bool Has(const rpg::Simulation& game,K kind) {
  for (std::size_t i=0;i<game.VisualEventCount();++i) if (game.VisualEvents()[i].kind==kind) return true;
  return false;
}
struct Image { UINT width=0,height=0; std::vector<unsigned char> pixels; };
Image Capture(Renderer& renderer,const wchar_t* path,bool present=true) {
  if (!renderer.Capture(path)) { std::wcerr<<renderer.Error()<<L'\n'; Require(false,"Effect capture failed"); }
  if (present) Require(renderer.EndFrame(false),"Effect frame failed");
  if (!renderer.ValidateDebugMessages()) { std::wcerr<<renderer.Error()<<L'\n'; Require(false,"DirectX effect validation failed"); }
  ComPtr<IWICImagingFactory> factory; ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame; ComPtr<IWICFormatConverter> converter;
  Require(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))),"WIC factory failed");
  Require(SUCCEEDED(factory->CreateDecoderFromFilename(path,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"WIC effect read failed");
  Image image; Require(SUCCEEDED(converter->GetSize(&image.width,&image.height)),"Image dimensions failed");
  image.pixels.resize(size_t(image.width)*image.height*4);
  Require(SUCCEEDED(converter->CopyPixels(nullptr,image.width*4,UINT(image.pixels.size()),image.pixels.data())),"Effect pixels failed");
  return image;
}
int Difference(const Image& a,const Image& b) {
  Require(a.width==b.width && a.height==b.height,"Effect image dimensions changed");
  int difference=0;
  for (std::size_t i=0;i<a.pixels.size();++i) difference=std::max(difference,std::abs(int(a.pixels[i])-int(b.pixels[i])));
  return difference;
}
double MeanDifference(const Image& a,const Image& b) {
  Require(a.width==b.width && a.height==b.height,"Effect image dimensions changed");
  double sum=0;
  for (std::size_t i=0;i<a.pixels.size();++i) sum+=std::abs(int(a.pixels[i])-int(b.pixels[i]));
  return sum/a.pixels.size();
}
}
int main() {
  Game game; rpg::Simulation reference; rpg::Command strike; strike.nextTarget=true; strike.ability=0;
  Require(game.RestoreSnapshot(duel) && reference.Load(duel),"Duel fixture rejected");
  game.ExecuteCommand(strike,.01f); reference.Update(.01f,strike);
  Require(Has(game.State(),K::Strike) && Has(game.State(),K::Hit),"Accepted strike needs one swing and impact event");
  Require(game.State().VisualEventCount()==2 && game.Effects().BurstCount()==1 && game.Effects().TrailCount()==1,"Strike effects must spawn once");
  Require(game.State().Save()==reference.Save(),"Effects must not change combat or saves");
  rpg::Command rejected; rejected.ability=0; game.ExecuteCommand(rejected,.01f);
  Require(game.State().VisualEventCount()==0 && game.Effects().BurstCount()==1,"Cooldown input must not duplicate impacts");
  rpg::Command heavy; heavy.ability=1; game.ExecuteCommand(heavy,.01f);
  Require(Has(game.State(),K::HeavyStrike) && Has(game.State(),K::HeavyHit) && Has(game.State(),K::Interrupt),"Heavy strike must distinguish impact and interrupt");
  Require(!Has(game.State(),K::EnemySwing),"An interrupted windup must not emit an enemy swing");
  for (int i=0;i<10;++i) game.ExecuteCommand({},.05f);
  Require(game.Effects().BurstCount()==0 && game.Effects().TrailCount()==0,"Expired effects must disappear completely");
  Require(game.RestoreSnapshot(duel),"Reload duel");
  game.ExecuteCommand(strike,.01f); const auto snapshot=game.State().Save();
  Require(game.RestoreSnapshot(snapshot) && game.Effects().BurstCount()==0 && game.Effects().TrailCount()==0 && game.State().VisualEventCount()==0,
    "Load must not replay transient combat effects");
  game.Initialize(); game.ExecuteCommand(strike,.01f);
  Require(game.State().VisualEventCount()==0 && game.Effects().TrailCount()==0,"Out-of-range attacks must not create a trail");
  Require(game.RestoreSnapshot(duel),"Reload unarmed fixture");
  rpg::Command unequip; unequip.inventory={rpg::InventoryAction::Unequip,int(rpg::GearSlot::Weapon),-1,rpg::Item::Starter};
  game.ExecuteCommand(unequip,.01f); game.ExecuteCommand(strike,.01f);
  Require(game.Effects().TrailCount()==0 && game.Effects().BurstCount()==1,"Unarmed attacks must not display a sword trail");
  auto fatal=duel; fatal.replace(fatal.find("5.2 100"),7,"5.2 1"); fatal.replace(fatal.find("1 0.5 3.14159"),13,"1 0.01 3.14159");
  Require(game.RestoreSnapshot(fatal),"Fatal fixture rejected"); game.ExecuteCommand({},.05f);
  Require(game.State().Health()==0 && game.Effects().TrailCount()==0 && game.Effects().BurstCount()==1,"Defeat must cancel trails while retaining the actual impact");
  for (int i=0;i<10;++i) game.ExecuteCommand({},.05f);
  Require(game.Effects().BurstCount()==0,"Fatal impact must expire while defeated");
  rpg::Command respawn; respawn.respawn=true; game.ExecuteCommand(respawn,.01f);
  Require(game.Effects().BurstCount()==0 && game.Effects().TrailCount()==0,"Respawn must clear effects");
  Require(game.RestoreSnapshot(duel),"Reload restart fixture"); game.ExecuteCommand(strike,.01f);
  rpg::Command restart; restart.restart=true; game.ExecuteCommand(restart,.01f);
  Require(game.Effects().BurstCount()==0 && game.Effects().TrailCount()==0 && game.State().VisualEventCount()==0,"Restart must discard old combat effects");
  Require(game.RestoreSnapshot(duel),"Reload miss fixture"); rpg::Command dash; dash.ability=2;
  game.ExecuteCommand(dash,.01f);
  bool miss=false;
  for (int i=0;i<12;++i) {
    game.ExecuteCommand({},.05f);
    if (Has(game.State(),K::EnemySwing)) { Require(!Has(game.State(),K::Hurt),"A missed swing must not create a hit flash"); miss=true; }
  }
  Require(miss,"Miss fixture must resolve its committed swing");
  for (float rate : {30.f,60.f,144.f}) {
    CombatEffects effects; effects.Emit({K::Hit,{0,0},1,0}); effects.Emit({K::Strike,{0,0},0,0});
    effects.TrackWeapon(0,{0,0,0},{0,1,0});
    for (int i=0;i<int(rate*.5f)+1;++i) effects.Advance(1/rate);
    Require(effects.BurstCount()==0 && effects.TrailCount()==0,"Effect lifetime must be bounded at different update rates");
  }
  CombatEffects pool;
  for (unsigned i=0;i<1000;++i) { pool.Emit({K::Hit,{0,0},1,0}); pool.Emit({K::Strike,{0,0},0,0,100+i}); }
  Require(pool.BurstCount()==CombatEffects::maxBursts && pool.TrailCount()==CombatEffects::maxTrails,"Effects must retain fixed pool bounds under overload");
  for (float dt : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) pool.Advance(dt);
  Require(pool.BurstCount()==CombatEffects::maxBursts,"Invalid time must not advance effects");
  pool.Reset(); Require(pool.BurstCount()==0 && pool.TrailCount()==0,"Reset must clear every pool slot");
  Require(sizeof(CombatEffects)<=16384,"Fixed combat effect state must fit the 16 KiB CPU budget");

  Require(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM failed");
  Win32Window window; Require(window.Create(GetModuleHandle(nullptr),960,720,L"combat effects checks",false),"Hidden effect window failed");
  Renderer renderer;
  if (!renderer.Initialize(window.GetHandle(),960,720)) { std::wcerr<<renderer.Error()<<L'\n'; Require(false,"Effect renderer initialization failed"); }
  const auto scene=[&]() {
    renderer.BeginFrame(); renderer.SetCamera({0,1,-4},{0,1,0});
    renderer.DrawMesh(visual::Mesh::Box,{0,1,0},{1.6f,1.6f,.3f},{.12f,.12f,.12f,1},visual::Material::Plain);
  };
  scene(); const auto base=Capture(renderer,L"effects-depth-reference.png"); const auto baseDraws=renderer.LastDrawCalls();
  scene(); renderer.DrawEffectGlow({0,1,1},.3f,{1,.7f,.2f,1}); const auto hidden=Capture(renderer,L"effects-occluded.png");
  Require(Difference(base,hidden)==0,"Opaque scenery must occlude effects completely");
  scene(); renderer.DrawEffectGlow({0,1,-1},.3f,{1,.7f,.2f,1}); const auto visible=Capture(renderer,L"effects-visible.png");
  Require(Difference(base,visible)>80 && renderer.LastDrawCalls()==baseDraws+1,"Visible effects need one additional batched draw");
  scene(); renderer.DrawEffectRing({0,1,1},.4f,{.1f,.6f,1,1}); const auto hiddenRing=Capture(renderer,L"effects-ring-occluded.png");
  Require(Difference(base,hiddenRing)==0,"Shock rings must obey opaque depth just like sparks");
  scene(); renderer.DrawEffectRing({0,1,-1},.4f,{.1f,.6f,1,1}); const auto ring=Capture(renderer,L"effects-ring-visible.png");
  const auto ringCenter=(size_t(ring.height/2)*ring.width+ring.width/2)*4;
  Require(Difference(base,ring)>40 && std::equal(base.pixels.begin()+ringCenter,base.pixels.begin()+ringCenter+4,ring.pixels.begin()+ringCenter),
    "A shock ring needs a visible rim and a transparent center");
  const auto overlap=[&](bool reverse,const wchar_t* path) {
    scene();
    for (int i=0;i<2;++i) { const int n=reverse?1-i:i; renderer.DrawEffectGlow({0,1,-.6f-float(n)*.5f},.3f,n?XMFLOAT4{0,0,.3f,1}:XMFLOAT4{.3f,0,0,1}); }
    return Capture(renderer,path);
  };
  const auto orderA=overlap(false,L"effects-overlap-a.png"),orderB=overlap(true,L"effects-overlap-b.png");
  Require(Difference(orderA,orderB)<=1,"Additive effects must not write depth or depend on submission order");
  scene(); renderer.DrawEffectGlow({0,1,-1},.3f,{1,1,1,1}); renderer.DrawHudRect(.4f,.4f,.2f,.2f,{0,0,0,1});
  const auto hud=Capture(renderer,L"effects-under-hud.png");
  const auto center=(size_t(hud.height/2)*hud.width+hud.width/2)*4;
  Require(hud.pixels[center]==0 && hud.pixels[center+1]==0 && hud.pixels[center+2]==0,"HUD must cover world effects");
  scene(); renderer.DrawEffectGlow({0,1,-1},.3f,{1,.7f,.2f,1}); const auto before=Capture(renderer,L"effects-before-portrait.png",false);
  Require(renderer.BeginPortrait(.7f,.1f,.25f,.6f),"Effect portrait failed");
  renderer.DrawMesh(visual::Mesh::Sphere,{0,1,0},{.7f,1,.7f},{.4f,.5f,.7f,1},visual::Material::Metal);
  renderer.DrawEffectGlow({0,1,0},2,{1,0,0,1}); // world-only API must reject portrait effects
  Require(renderer.EndPortrait(),"Effect portrait restore failed");
  const auto after=Capture(renderer,L"effects-after-portrait.png");
  Require(renderer.EffectVertices()==6,"Portrait must not replay or submit combat effects");
  for (UINT y=0;y<after.height;++y) for (UINT x=0;x<after.width;++x) {
    if (x>=672 && x<912 && y>=72 && y<504) continue;
    const auto p=(size_t(y)*after.width+x)*4;
    Require(std::equal(before.pixels.begin()+p,before.pixels.begin()+p+4,after.pixels.begin()+p),"Preview must preserve world effects outside its viewport");
  }
  scene(); const auto restored=Capture(renderer,L"effects-restored.png");
  Require(Difference(base,restored)==0,"Effects must not leak pipeline state or vertices into the next frame");
  scene(); for (int i=0;i<2000;++i) renderer.DrawEffectGlow({0,1,-1},.01f,{.01f,.01f,.01f,.01f});
  Require(renderer.EffectVertices()==Renderer::maxEffectVertices && renderer.EndFrame(false),"Effect upload must stay bounded and render overload safely");
  pool.Emit({K::Strike,{0,0},0,0}); pool.TrackWeapon(0,{0,.5f,0},{0,1.3f,0}); pool.Advance(.02f);
  pool.TrackWeapon(0,{.1f,.6f,0},{.1f,1.4f,0}); scene(); pool.Render(renderer);
  Require(renderer.EffectVertices()>0 && renderer.EndFrame(false),"A moving blade must leave a short trail");
  pool.Advance(.02f); pool.TrackWeapon(0,{4,.5f,0},{4,1.3f,0}); scene(); pool.Render(renderer);
  Require(renderer.EffectVertices()==0 && renderer.EndFrame(false),"Dash/teleport discontinuities must not stretch blade ribbons");
  Image referenceRate;
  for (int rate : {30,60,144}) {
    CombatEffects consistent;
    consistent.Emit({K::HeavyStrike,{0,0},0,0}); consistent.Emit({K::HeavyHit,{0,.8f},1.2f,0});
    float elapsed=0;
    while (elapsed<.12f) { const float dt=std::min(1.f/rate,.12f-elapsed); consistent.Advance(dt); elapsed+=dt; }
    scene(); consistent.Render(renderer);
    const auto path=L"effects-rate-"+std::to_wstring(rate)+L".png";
    const auto frame=Capture(renderer,path.c_str());
    if (rate==30) referenceRate=frame;
    else Require(MeanDifference(referenceRate,frame)<.05,"Crescents, shock rings and sparks must agree at equal elapsed time across update rates");
  }
  CombatEffects saturated;
  for (unsigned i=0;i<CombatEffects::maxTrails;++i) saturated.Emit({K::HeavyStrike,{0,0},0,0,100+i});
  for (unsigned i=0;i<CombatEffects::maxBursts;++i) saturated.Emit({K::HeavyHit,{0,0},1,0});
  saturated.Advance(.05f); saturated.Advance(.05f); scene(); saturated.Render(renderer);
  Require(renderer.EffectVertices()==Renderer::maxEffectVertices && renderer.EndFrame(false),
    "Overlapping heavy effects must saturate at the existing upload limit without growing or failing");

  // Real gameplay captures exercise event -> blade samples -> render, including
  // repeat rendering without advancing state and the live Escape menu.
  for (int ability=0;ability<2;++ability) {
    Require(game.RestoreSnapshot(duel),"Attack capture fixture rejected");
    rpg::Command command; command.nextTarget=true; command.ability=ability; game.ExecuteCommand(command,.01f);
    game.SetCameraOrbit(1.1f,.28f,4.8f);
    const wchar_t* names[2][3]={{L"effects-strike-impact.png",L"effects-strike-trail.png",L"effects-strike-fade.png"},
      {L"effects-heavy-impact.png",L"effects-heavy-trail.png",L"effects-heavy-fade.png"}};
    for (int phase=0;phase<3;++phase) {
      if (phase) for (int i=0;i<4;++i) game.ExecuteCommand({},.02f);
      const auto durable=game.State().Save();
      const auto bursts=game.Effects().BurstCount(),trails=game.Effects().TrailCount();
      renderer.BeginFrame(); game.Render(renderer,false); Capture(renderer,names[ability][phase]);
      const auto vertices=renderer.EffectVertices();
      Require(vertices>0 && vertices<=Renderer::maxEffectVertices,"Attack capture needs a bounded visible effect submission");
      renderer.BeginFrame(); game.Render(renderer,false); Require(renderer.EndFrame(false),"Repeated attack render failed");
      Require(game.State().Save()==durable && game.Effects().BurstCount()==bursts && game.Effects().TrailCount()==trails && renderer.EffectVertices()==vertices,
        "Rendering must never advance or duplicate effects");
    }
  }
  // Equal-time attack inspections from gameplay distance and opposite sides.
  Require(game.RestoreSnapshot(duel),"Cinematic attack fixture rejected");
  heavy.nextTarget=true; game.ExecuteCommand(heavy,.01f);
  for (int i=0;i<7;++i) game.ExecuteCommand({},.02f);
  const float headings[]={0,-1.1f,3.14159265f};
  const wchar_t* views[]={L"effects-heavy-gameplay.png",L"effects-heavy-opposite.png",L"effects-heavy-front.png"};
  for (int i=0;i<3;++i) {
    game.SetCameraOrbit(headings[i],.40f,i==0?8.f:5.5f);
    renderer.BeginFrame(); game.Render(renderer,false); Capture(renderer,views[i]);
  }
  for (int i=0;i<9;++i) game.ExecuteCommand({},.02f);
  renderer.BeginFrame(); game.Render(renderer,false); Capture(renderer,L"effects-heavy-afterglow.png");
  game.OpenPanel(ui::Panel::Character); game.ShowPanel(ui::Panel::Inventory);
  renderer.BeginFrame(); game.Render(renderer); Capture(renderer,L"effects-heavy-equipment.png");
  Require(game.RestoreSnapshot(duel),"Enemy impact fixture rejected");
  for (int i=0;i<11;++i) game.ExecuteCommand({},.05f);
  game.SetCameraOrbit(1.1f,.28f,4.8f); renderer.BeginFrame(); game.Render(renderer,false);
  Capture(renderer,L"effects-enemy-hit.png");
  Require(game.RestoreSnapshot(duel),"Enemy miss capture fixture rejected"); game.ExecuteCommand(dash,.01f);
  for (int i=0;i<12 && !Has(game.State(),K::EnemySwing);++i) game.ExecuteCommand({},.05f);
  game.ExecuteCommand({},.03f); game.SetCameraOrbit(1.1f,.28f,4.8f);
  renderer.BeginFrame(); game.Render(renderer,false); Capture(renderer,L"effects-enemy-miss.png");
  Require(game.RestoreSnapshot(duel),"Menu fixture rejected"); game.OpenPanel(ui::Panel::Menu);
  for (int i=0;i<11;++i) game.Update(.05,Input{},0);
  Require(game.State().Health()<100 && game.Effects().BurstCount()>0,"The Escape menu must not freeze real enemy impact effects");
  renderer.BeginFrame(); game.Render(renderer); Capture(renderer,L"effects-live-menu.png");
  Require(renderer.ValidateDebugMessages(),"Combat effect checks emitted DirectX warnings");
  std::cout<<"PASS: confirmed combat events, miss/interrupt/death/load lifetimes, bounded pools/uploads, depth/blend/HUD/portrait isolation and 28 diagnostic captures. Fixed effect state bytes: "<<sizeof(CombatEffects)<<'\n';
  CoUninitialize();
}
