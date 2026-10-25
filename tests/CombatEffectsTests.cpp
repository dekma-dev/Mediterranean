#include "game/Game.h"
#include "tests/DashFixture.h"
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
  // Safe outpost fixture keeps enemy damage out of recovery acceptance checks.
  rpg::Simulation fresh;
  {
    Game burst; burst.Initialize(); rpg::Simulation plain; rpg::Command dash; dash.ability=2; dash.nextTarget=true;
    Require(burst.RestoreSnapshot(test::dashFixture) && plain.Load(test::dashFixture),"Target dash effect fixtures");
    const auto origin=burst.State().Player();
    burst.ExecuteCommand(dash,.03f); plain.Update(.03f,dash);
    Require(Has(burst.State(),K::DashStart) && burst.Effects().DashSampleCount()>=4 && burst.Effects().DashSampleCount()<=6 &&
      burst.Effects().BurstCount()==1,"Accepted dash must seed a wake and one launch pulse");
    const auto& launch=burst.State().VisualEvents()[0];
    Require(launch.position.x==origin.x && launch.position.z==origin.z && launch.elapsed==.03f,"Launch effect must stay at the true starting position");
    Require(burst.State().Save()==plain.Save(),"Dash cosmetics must not affect simulation or durable state");
    dash.nextTarget=false; burst.ExecuteCommand(dash,.01f); plain.Update(.01f,dash);
    Require(!Has(burst.State(),K::DashStart) && burst.Effects().BurstCount()==1,"Cooldown rejection must not restart the wake");
    for (int i=0;i<70;++i) { burst.ExecuteCommand({},.02f); plain.Update(.02f,{}); }
    Require(burst.State().Save()==plain.Save() && burst.Effects().DashSampleCount()==0 && burst.Effects().BurstCount()==0,
      "Dash residue must expire and preserve renderer-free movement results");
    burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Target dash reset fixture"); dash.nextTarget=true; burst.ExecuteCommand(dash,.05f); burst.SuspendInput();
    Require(burst.Effects().DashSampleCount()==0 && burst.Effects().BurstCount()==0 && burst.PlayerAnimation().dash==0,
      "Focus suspension must immediately discard dash motion and cosmetics");
    burst.ExecuteCommand({},.05f);
    Require(!burst.State().Dashing() && !Has(burst.State(),K::DashStop),"Cancelled dash must not invent a finishing burst");
    burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Target dash reset fixture"); dash.nextTarget=true; burst.ExecuteCommand(dash,.05f);
    rpg::Command capture; capture.haltMovement=true; burst.ExecuteCommand(capture,.01f);
    Require(burst.Effects().DashSampleCount()==0 && burst.Effects().BurstCount()==0,"Modal capture must clear dash effects");
    burst.Initialize(); Require(burst.RestoreSnapshot(test::dashFixture),"Target dash reset fixture"); dash.nextTarget=true; burst.ExecuteCommand(dash,.05f);
    Require(burst.RestoreSnapshot(burst.State().Save()) && burst.Effects().DashSampleCount()==0 &&
      burst.Effects().BurstCount()==0 && !burst.State().Dashing(),"Loading must not replay an unfinished dash");
    // Injured champion: another spell cannot override the physical burst.
    std::string hurt=test::dashFixture; hurt.replace(hurt.find(" 100 "),5," 40 ");
    Require(burst.RestoreSnapshot(hurt),"Dash casting fixture");
    burst.ExecuteCommand(dash,.05f); rpg::Command mend; mend.ability=3; burst.ExecuteCommand(mend,.01f);
    Require(burst.State().Health()==40 && burst.State().Cooldown(3)==0 && !Has(burst.State(),K::Mend),
      "An active dash must reject another spell without consuming its cooldown");
    for (int i=0;i<7;++i) burst.ExecuteCommand({},.05f);
    burst.ExecuteCommand(mend,.01f);
    Require(burst.State().Health()==40 && burst.State().Barrier()>0 && Has(burst.State(),K::Mend),"Normal casting must resume after the stop");
  }
  for (int rate : {30,60,144,1000}) {
    CombatEffects wake; wake.Emit({K::DashStart,{0,0},0,0});
    float elapsed=0;
    while (elapsed<.3f-1e-7f) {
      const float dt=std::min(1.f/rate,.3f-elapsed); elapsed+=dt; wake.Advance(dt);
      wake.TrackDash({0,0,elapsed*5},true,dt,1);
      Require(wake.DashSampleCount()<=CombatEffects::trailSamples,"Wake history must remain bounded at high update rates");
    }
    Require(wake.DashSampleCount()>=23 && wake.DashSampleCount()<=26 &&
      wake.DashHistorySeconds()>.18f && wake.DashHistorySeconds()<.201f,
      "Wake density and retained time must agree across update rates");
    wake.TrackDash({0,0,8},true,.01f,1);
    Require(wake.DashSampleCount()==0,"A discontinuity must never stretch the wake across the map");
  }
  auto injured=fresh.Save();
  const auto healthPosition=injured.find(" 100 ");
  Require(healthPosition!=std::string::npos,"Recovery fixture health field missing");
  injured.replace(healthPosition,5," 20 ");
  for (bool potion : {false,true}) {
    Game recovery; rpg::Simulation recoveryReference;
    Require(recovery.RestoreSnapshot(injured) && recoveryReference.Load(injured),"Injured recovery fixture rejected");
    rpg::Command command; command.usePotion=potion; command.ability=potion?-1:3;
    recovery.ExecuteCommand(command,.01f); recoveryReference.Update(.01f,command);
    Require(Has(recovery.State(),potion?K::Potion:K::Mend) && recovery.State().VisualEventCount()==1,
      "Accepted recovery must emit exactly its own visual event");
    Require(recovery.State().Health()==(potion?60.f:20.f) && recovery.State().Save()==recoveryReference.Save(),
      "Recovery cosmetics must preserve healing, inventory and save behavior");
    recovery.ExecuteCommand(command,.01f);
    Require(recovery.State().VisualEventCount()==0 && recovery.Effects().BurstCount()==1,"Recovery cooldown must not duplicate effects");
    for (int i=0;i<17;++i) recovery.ExecuteCommand({},.05f);
    Require(recovery.Effects().BurstCount()==0,"Recovery residue must expire");
    recovery.Initialize(); recovery.ExecuteCommand(command,.01f);
    Require(recovery.State().VisualEventCount()==(potion?0:1) && recovery.Effects().BurstCount()==(potion?0:1),"Only healing potions reject at full health; guard still activates");
  }
  Game game; rpg::Simulation reference; rpg::Command strike; strike.nextTarget=true; strike.ability=0;
  Require(game.RestoreSnapshot(duel) && reference.Load(duel),"Duel fixture rejected");
  game.ExecuteCommand(strike,.01f); reference.Update(.01f,strike);
  Require(game.State().VisualEventCount()==0,"Acceptance must not emit premature swing or impact effects");
  bool sawSwing=false;
  for(int i=0;i<18;++i) {
    game.ExecuteCommand({},.01f); reference.Update(.01f,{});
    sawSwing|=Has(game.State(),K::Strike);
    if(i<17) Require(!Has(game.State(),K::Hit),"The blade must reach contact before an impact appears");
  }
  Require(sawSwing && Has(game.State(),K::Hit),"The swing must precede the contact effect");
  Require(game.State().VisualEventCount()==1 && game.Effects().BurstCount()==1 && game.Effects().TrailCount()==1,"Strike effects must spawn once");
  Require(game.State().Save()==reference.Save(),"Effects must not change combat or saves");
  rpg::Command rejected; rejected.ability=0; game.ExecuteCommand(rejected,.01f);
  Require(game.State().VisualEventCount()==0 && game.Effects().BurstCount()==1,"Cooldown input must not duplicate impacts");
  Require(game.RestoreSnapshot(duel),"Independent heavy interrupt fixture");
  rpg::Command heavy; heavy.ability=1; heavy.nextTarget=true; game.ExecuteCommand(heavy,.01f);
  sawSwing=false;
  for(int i=0;i<30;++i) { game.ExecuteCommand({},.01f); sawSwing|=Has(game.State(),K::HeavyStrike); }
  heavy.nextTarget=false;
  Require(sawSwing && Has(game.State(),K::HeavyHit) && Has(game.State(),K::Interrupt),"Heavy strike must distinguish swing, impact and interrupt");
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
  for(int i=0;i<18;++i) game.ExecuteCommand({},.01f);
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
  Require(game.RestoreSnapshot(duel),"Reload miss fixture"); rpg::Command retreat; retreat.movement={0,-1};
  game.ExecuteCommand(retreat,.01f);
  bool miss=false;
  for (int i=0;i<12;++i) {
    game.ExecuteCommand(retreat,.05f);
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
  pool.Emit({K::Mend,{0,0},0,0});
  pool.Advance(.05f); pool.Advance(.05f);
  for (unsigned i=1;i<CombatEffects::maxBursts;++i) pool.Emit({K::Hit,{0,0},1,0});
  for (int i=0;i<10;++i) pool.Advance(.05f);
  pool.Emit({K::Hit,{0,0},1,0});
  Require(pool.BurstCount()==2,"Expired short impacts must be reused before a still-active longer recovery pulse");
  pool.Reset();
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
  for(auto kind:{rpg::EnemyKind::Captain,rpg::EnemyKind::Wraith,rpg::EnemyKind::Reaver}) {
    Game encounter;Require(encounter.RestoreSnapshot(duel),"Enemy warning capture fixture");
    auto& foe=const_cast<std::vector<rpg::Enemy>&>(encounter.State().Enemies())[0];
    foe.kind=kind;foe.rank=kind==rpg::EnemyKind::Captain?1:0;
    foe.health=foe.MaxHealth();foe.windup=foe.WindupDuration();
    rpg::Command select;select.nextTarget=true;encounter.ExecuteCommand(select,.01f);
    encounter.SetCameraOrbit(.5f,.65f,11);
    renderer.BeginFrame();encounter.Render(renderer);
    Capture(renderer,kind==rpg::EnemyKind::Captain?L"boss-warning.png":kind==rpg::EnemyKind::Wraith?L"lance-warning.png":L"cleave-warning.png");
  }
  const auto scene=[&]() {
    renderer.BeginFrame(); renderer.SetCamera({0,1,-4},{0,1,0});
    renderer.DrawMesh(visual::Mesh::Box,{0,1,0},{1.6f,1.6f,.3f},{.12f,.12f,.12f,1},visual::Material::Plain);
  };
  scene(); const auto base=Capture(renderer,L"effects-depth-reference.png");
  const auto baseDraws=renderer.LastDrawCalls()-renderer.CloudLightUpdates();
  for(auto kind:{K::EnemyLance,K::EnemyQuake}) {
    CombatEffects special;special.Emit({kind,{0,0},0,0,100});special.Advance(.05f);
    renderer.BeginFrame();renderer.SetCamera({8,9,-10},{0,0,1});
    special.Render(renderer);
    Require(renderer.EffectVertices()>0 && renderer.EffectVertices()<3000,"Enemy special effect exceeds its fixed geometry budget");
    Capture(renderer,kind==K::EnemyLance?L"enemy-lance.png":L"enemy-doomquake.png");
    for(int i=0;i<12;++i) special.Advance(.05f);
    scene();special.Render(renderer);
    Require(renderer.EffectVertices()==0 && renderer.EndFrame(false),"Enemy spell effect failed to expire");
  }
  scene(); renderer.DrawEffectGlow({0,1,1},.3f,{1,.7f,.2f,1}); const auto hidden=Capture(renderer,L"effects-occluded.png");
  Require(Difference(base,hidden)==0,"Opaque scenery must occlude effects completely");
  scene(); renderer.DrawEffectGlow({0,1,-1},.3f,{1,.7f,.2f,1}); const auto visible=Capture(renderer,L"effects-visible.png");
  Require(Difference(base,visible)>80 && renderer.LastDrawCalls()-renderer.CloudLightUpdates()==baseDraws+1,"Visible effects need one additional batched draw");
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
  // At high update rates the fixed history must retain duration, not merely
  // the latest 32 frames. Keep the synthetic crescent outside the camera.
  for (int rate : {60,144,1000}) {
    CombatEffects history; history.Emit({K::Strike,{100,100},0,0});
    history.TrackWeapon(0,{-1,.8f,-1},{-1,1.2f,-1});
    float elapsed=0;
    while (elapsed<.235f) {
      const float dt=std::min(1.f/rate,.235f-elapsed); history.Advance(dt); elapsed+=dt;
      const float x=-1+2*elapsed/.235f;
      history.TrackWeapon(0,{x,.8f,-1},{x,1.2f,-1});
    }
    history.Advance(.01f);
    scene(); history.Render(renderer);
    const auto path=L"effects-history-"+std::to_wstring(rate)+L".png";
    const auto frame=Capture(renderer,path.c_str());
    int first=int(frame.width),last=-1;
    for (UINT y=0;y<frame.height;++y) for (UINT x=0;x<frame.width;++x) {
      const auto offset=(size_t(y)*frame.width+x)*4;
      bool changed=false;
      for (int c=0;c<3;++c) changed=changed || std::abs(int(frame.pixels[offset+c])-int(base.pixels[offset+c]))>3;
      if (changed) { first=std::min(first,int(x)); last=std::max(last,int(x)); }
    }
    std::cout<<"Blade history width at "<<rate<<" Hz: "<<last-first<<'\n';
    Require(last-first>150,"High update rates must retain the visible blade trail history");
  }
  for (unsigned i=0;i<CombatEffects::maxTrails;++i) saturated.Emit({K::HeavyStrike,{0,0},0,0,100+i});
  for (unsigned i=0;i<CombatEffects::maxBursts;++i) saturated.Emit({K::HeavyHit,{0,0},1,0});
  saturated.Advance(.05f); saturated.Advance(.05f); scene(); saturated.Render(renderer);
  Require(renderer.EffectVertices()==Renderer::maxEffectVertices && renderer.EndFrame(false),
    "Overlapping heavy effects must saturate at the existing upload limit without growing or failing");

  for (bool potion : {false,true}) {
    Image recoveryRate;
    for (int rate : {30,60,144}) {
      CombatEffects recovery; recovery.Emit({potion?K::Potion:K::Mend,{0,-1},0,0});
      float elapsed=0;
      while (elapsed<.3f) { const float dt=std::min(1.f/rate,.3f-elapsed); recovery.Advance(dt); elapsed+=dt; }
      scene(); recovery.Render(renderer);
      Require(renderer.EffectVertices()>0 && renderer.EffectVertices()<=(potion?162u:306u),"Recovery geometry must stay within its fixed budget");
      const auto path=std::wstring(potion?L"effects-potion-rate-":L"effects-mend-rate-")+std::to_wstring(rate)+L".png";
      const auto frame=Capture(renderer,path.c_str());
      Require(Difference(base,frame)>40,"Recovery effect must be visible");
      if (rate==30) recoveryRate=frame;
      else Require(MeanDifference(recoveryRate,frame)<.05,"Recovery rendering must agree at equal elapsed time");
      for (int i=0;i<17;++i) recovery.Advance(.05f);
      scene(); recovery.Render(renderer);
      Require(renderer.EffectVertices()==0 && recovery.BurstCount()==0 && renderer.EndFrame(false),"Expired recovery must submit no geometry");
    }
    Require(game.RestoreSnapshot(injured),"Recovery capture fixture rejected");
    rpg::Command command; command.usePotion=potion; command.ability=potion?-1:3;
    game.ExecuteCommand(command,.01f);
    for (int i=0;i<6;++i) game.ExecuteCommand({},.05f);
    game.SetCameraOrbit(1.1f,.28f,4.8f);
    renderer.BeginFrame(); game.Render(renderer,false);
    Capture(renderer,potion?L"effects-potion-gameplay.png":L"effects-mend-gameplay.png");
    const auto saved=game.State().Save();
    Require(game.RestoreSnapshot(saved) && game.Effects().BurstCount()==0,"Reload must clear recovery residue");
  }

  // Real dash phases, including air motion and a swept collision.
  for (int pose=0;pose<7;++pose) {
    Game burst; burst.Initialize(); rpg::Command dash; dash.ability=2; dash.nextTarget=true;
    Require(burst.RestoreSnapshot(pose==4?test::dashWallFixture:test::dashFixture),"Target dash capture fixture");
    if (pose==5) { rpg::Command jump; jump.jump=true; burst.ExecuteCommand(jump,.05f); }
    const float times[]={.03f,.11f,.27f,.36f,.2f,.11f,.42f};
    float elapsed=0; bool impacted=false;
    while (elapsed<times[pose]-1e-7f) {
      const float dt=std::min(.01f,times[pose]-elapsed);
      burst.ExecuteCommand(elapsed==0?dash:rpg::Command{},dt); elapsed+=dt;
      if (Has(burst.State(),K::DashImpact)) {
        Require(!impacted,"A wall collision must emit only one stopping impact"); impacted=true;
        for (std::size_t i=0;i<burst.State().VisualEventCount();++i) {
          const auto& event=burst.State().VisualEvents()[i];
          if (event.kind==K::DashImpact) Require(event.position.z<3.5f && event.position.z==burst.State().Player().z &&
            event.position.x==burst.State().Player().x,"Wall effect must use the swept endpoint");
        }
      }
    }
    if (pose==4) Require(impacted && !burst.State().Dashing() && burst.State().CanStand(burst.State().Player()),
      "Wall capture must stop with a valid endpoint and a recoil effect");
    if (pose==5) Require(!burst.State().Grounded() && burst.State().Height()>0,"Air dash must keep ballistic height");
    burst.SetCameraOrbit(1.15f,.28f,4.8f);
    const wchar_t* names[]={L"effects-dash-launch.png",L"effects-dash-drive.png",L"effects-dash-brake.png",
      L"effects-dash-stop.png",L"effects-dash-wall.png",L"effects-dash-air.png",L"effects-dash-settle.png"};
    const auto saved=burst.State().Save(); const auto samples=burst.Effects().DashSampleCount();
    renderer.BeginFrame(); burst.Render(renderer,false); Capture(renderer,names[pose]);
    const auto vertices=renderer.EffectVertices();
    Require(vertices>0 && vertices<=Renderer::maxEffectVertices,"Dash must submit visible bounded effect geometry");
    renderer.BeginFrame(); burst.Render(renderer,false); Require(renderer.EndFrame(false),"Repeat dash render");
    Require(burst.State().Save()==saved && burst.Effects().DashSampleCount()==samples && renderer.EffectVertices()==vertices,
      "Rendering must not change dash movement, history or particle age");
    if (pose==1) {
      burst.SetCameraOrbit(1.5707963f,.10f,4.8f);
      renderer.BeginFrame(); burst.Render(renderer,false); Capture(renderer,L"effects-dash-side.png");
      burst.SetCameraOrbit(3.14159265f,.18f,4.8f);
      renderer.BeginFrame(); burst.Render(renderer,false); Capture(renderer,L"effects-dash-front.png");
    }
    if (pose==6) Require(!burst.State().Dashing() && burst.PlayerAnimation().dashBrake>0 &&
      burst.PlayerAnimation().dashBrake<.35f,"The settled capture must show recovery after physical movement has ended");
  }

  // Real gameplay captures exercise event -> blade samples -> render, including
  // repeat rendering without advancing state and the live Escape menu.
  for (int ability=0;ability<2;++ability) {
    Require(game.RestoreSnapshot(duel),"Attack capture fixture rejected");
    rpg::Command command; command.nextTarget=true; command.ability=ability; game.ExecuteCommand(command,.01f);
    for(int i=0;i<(ability?30:18);++i) game.ExecuteCommand({},.01f);
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
      if (ability==1 && phase==0) {
        renderer.BeginFrame(); game.Render(renderer); Capture(renderer,L"effects-combat-hud.png");
      }
    }
  }
  // Equal-time attack inspections from gameplay distance and opposite sides.
  Require(game.RestoreSnapshot(duel),"Cinematic attack fixture rejected");
  heavy.nextTarget=true; game.ExecuteCommand(heavy,.01f);
  for (int i=0;i<17;++i) game.ExecuteCommand({},.02f);
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
  Require(game.RestoreSnapshot(duel),"Enemy miss capture fixture rejected"); game.ExecuteCommand(retreat,.01f);
  for (int i=0;i<12 && !Has(game.State(),K::EnemySwing);++i) game.ExecuteCommand(retreat,.05f);
  game.ExecuteCommand({},.03f); game.SetCameraOrbit(1.1f,.28f,4.8f);
  renderer.BeginFrame(); game.Render(renderer,false); Capture(renderer,L"effects-enemy-miss.png");
  Require(game.RestoreSnapshot(duel),"Menu fixture rejected"); game.OpenPanel(ui::Panel::Menu);
  for (int i=0;i<11;++i) game.Update(.05,Input{},0);
  Require(game.State().Health()<100 && game.Effects().BurstCount()>0,"The Escape menu must not freeze real enemy impact effects");
  renderer.BeginFrame(); game.Render(renderer); Capture(renderer,L"effects-live-menu.png");
  Require(renderer.ValidateDebugMessages(),"Combat effect checks emitted DirectX warnings");
  std::cout<<"PASS: confirmed combat and recovery events, miss/interrupt/death/load lifetimes, bounded pools/uploads, depth/blend/HUD/portrait isolation and 48 diagnostic captures. Fixed effect state bytes: "<<sizeof(CombatEffects)<<'\n';
  CoUninitialize();
}
