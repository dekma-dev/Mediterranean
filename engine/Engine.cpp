#include "engine/Engine.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <psapi.h>
#include <vector>

#include "engine/Renderer.h"
#include "game/Game.h"
#include "game/CharacterView.h"
#include "platform/Win32Window.h"

Engine::Engine() = default;

Engine::~Engine() = default;

bool Engine::Initialize(HINSTANCE instance, int width, int height, bool visible) {
  window_ = std::make_unique<Win32Window>();
  if (!window_->Create(instance, width, height, L"RPG Native Prototype", visible)) {
    error_ = L"Could not create the application window.";
    return false;
  }

  renderer_ = std::make_unique<Renderer>();
  RECT client{}; GetClientRect(window_->GetHandle(),&client);
  width=client.right; height=client.bottom;
  if (!renderer_->Initialize(window_->GetHandle(), width, height)) {
    error_ = renderer_->Error();
    return false;
  }

  game_ = std::make_unique<Game>();
  game_->Initialize();
  game_->SetFullscreenState(window_->Fullscreen());
  SetWindowTextW(window_->GetHandle(), game_->WindowTitle().c_str());

  running_ = true;
  return true;
}

bool Engine::SmokeTest(const std::wstring& capturePath,const std::string& snapshot) {
  RECT before{},full{},restored{}; GetClientRect(window_->GetHandle(),&before);
  MONITORINFO monitor{sizeof(MONITORINFO)};
  if (!GetMonitorInfo(MonitorFromWindow(window_->GetHandle(),MONITOR_DEFAULTTONEAREST),&monitor) || !window_->SetFullscreen(true)) {
    error_=L"Hidden fullscreen transition failed."; return false;
  }
  GetClientRect(window_->GetHandle(),&full);
  if (full.right!=monitor.rcMonitor.right-monitor.rcMonitor.left || full.bottom!=monitor.rcMonitor.bottom-monitor.rcMonitor.top ||
    IsWindowVisible(window_->GetHandle()) || !window_->SetFullscreen(false)) {
    error_=L"Fullscreen size, visibility or restoration failed."; return false;
  }
  GetClientRect(window_->GetHandle(),&restored);
  if (restored.right!=before.right || restored.bottom!=before.bottom || IsWindowVisible(window_->GetHandle())) {
    error_=L"Windowed client size was not preserved."; return false;
  }
  if (!snapshot.empty() && !game_->RestoreSnapshot(snapshot)) {
    error_=L"Smoke-test snapshot failed save validation."; return false;
  }
  // Exercise real device/shader creation, resized targets, rendering and GPU readback.
  // This does not synthesize input or claim to replace interactive playtesting.
  for (const auto dimensions : {std::pair<int,int>{960,600},{1600,900},{1280,720}}) {
    if (!renderer_->Resize(dimensions.first,dimensions.second)) { error_=renderer_->Error(); return false; }
    game_->Update(1.0/60.0,input_,0);
    renderer_->BeginFrame(); game_->Render(*renderer_);
    const auto base=std::filesystem::path(capturePath);
    const auto output=dimensions.first==1280?base:base.parent_path() /
      (base.stem().wstring()+L"-"+std::to_wstring(dimensions.first)+L"x"+std::to_wstring(dimensions.second)+base.extension().wstring());
    if (!renderer_->Capture(output.wstring())) { error_=renderer_->Error(); return false; }
    if (!renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
  }
  // Inspect silhouette, back faces, roofs and tree crowns from two additional
  // real orbit-camera views. These renders deliberately omit the HUD.
  for (int view=0;view<2;++view) {
    game_->SetCameraOrbit(view==0?2.6f:3.14159265f,view==0?.85f:.2f,view==0?18.f:4.f);
    renderer_->BeginFrame(); game_->Render(*renderer_,false);
    const auto base=std::filesystem::path(capturePath);
    const auto output=base.parent_path()/(base.stem().wstring()+(view==0?L"-overview":L"-character")+base.extension().wstring());
    if (!renderer_->Capture(output.wstring()) || !renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
  }
  game_->SetCameraOrbit(0,.45f,10);
  for (const auto dimensions : {std::pair<int,int>{960,600},{1600,900},{1280,720}}) {
    if (!renderer_->Resize(dimensions.first,dimensions.second)) { error_=renderer_->Error(); return false; }
    const std::pair<ui::Panel,const wchar_t*> panels[]={
      {ui::Panel::Menu,L"menu"},{ui::Panel::Inventory,L"inventory"},{ui::Panel::Character,L"equipment"},
      {ui::Panel::Map,L"atlas"},{ui::Panel::Journal,L"journal"},{ui::Panel::Character,L"equipment-bag"}};
    for (const auto& panel : panels) {
      game_->OpenPanel(panel.first);
      if (std::wstring(panel.second)==L"equipment-bag") game_->ShowPanel(ui::Panel::Inventory);
      renderer_->BeginFrame(); game_->Render(*renderer_);
      const auto base=std::filesystem::path(capturePath);
      const auto suffix=dimensions.first==1280?L"":L"-"+std::to_wstring(dimensions.first)+L"x"+std::to_wstring(dimensions.second);
      const auto output=base.parent_path()/(base.stem().wstring()+L"-"+panel.second+suffix+base.extension().wstring());
      if (!renderer_->Capture(output.wstring()) || !renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
    }
  }
  game_->OpenPanel(ui::Panel::None);
  if (game_->State().HasWardArmor()) {
    const auto original=game_->State().Save();
    const auto capture=[&](const wchar_t* suffix,bool showInterface=true) {
      const auto base=std::filesystem::path(capturePath);
      const auto path=base.parent_path()/(base.stem().wstring()+suffix+base.extension().wstring());
      renderer_->BeginFrame(); game_->Render(*renderer_,showInterface);
      return renderer_->Capture(path.wstring()) && renderer_->EndFrame();
    };
    for (int i=0;i<rpg::Inventory::gearCount;++i) {
      rpg::Command c; c.inventory={rpg::InventoryAction::Unequip,i,-1,game_->State().Items().Worn(rpg::GearSlot(i))}; game_->ExecuteCommand(c);
    }
    game_->OpenPanel(ui::Panel::Character); game_->ShowPanel(ui::Panel::Inventory);
    if (!capture(L"-base-outfit")) { error_=renderer_->Error(); return false; }
    if (!game_->RestoreSnapshot(original)) return false;
    rpg::Command c; c.inventory={rpg::InventoryAction::Equip,game_->State().Items().Find(rpg::Item::Tunic),int(rpg::GearSlot::Chest),rpg::Item::Tunic}; game_->ExecuteCommand(c);
    game_->OpenPanel(ui::Panel::Character); game_->ShowPanel(ui::Panel::Inventory);
    if (!capture(L"-travel-outfit")) { error_=renderer_->Error(); return false; }
    if (!game_->RestoreSnapshot(original)) return false;
    const rpg::Vec2 destination{12,-3.8f};
    const auto path=game_->State().Map().FindPath(game_->State().Player(),destination);
    for (auto waypoint : path) for (int step=0;step<500;++step) {
      const auto p=game_->State().Player(); if (std::hypot(p.x-waypoint.x,p.z-waypoint.z)<.15f) break;
      c={}; c.sprint=true; c.movement={waypoint.x-p.x,waypoint.z-p.z}; game_->ExecuteCommand(c,.05f);
    }
    c={}; c.interact=true; game_->ExecuteCommand(c);
    if (!game_->State().Sparring()) { error_=L"Sparring render scenario could not start."; return false; }
    for (int step=0;step<8;++step) game_->ExecuteCommand({},.05f);
    game_->SetCameraOrbit(0,.45f,7);
    if (!capture(L"-sparring")) { error_=renderer_->Error(); return false; }
    game_->OpenPanel(ui::Panel::Character); game_->ShowPanel(ui::Panel::Inventory);
    if (!capture(L"-sparring-interface")) { error_=renderer_->Error(); return false; }
    if (!game_->RestoreSnapshot(original)) return false;
    const auto walkTo=[&](rpg::Vec2 destination) {
      for (auto waypoint : game_->State().Map().FindPath(game_->State().Player(),destination)) {
        for (int step=0;step<500;++step) {
          const auto p=game_->State().Player(); if (std::hypot(p.x-waypoint.x,p.z-waypoint.z)<.15f) break;
          rpg::Command move; move.sprint=true; move.movement={waypoint.x-p.x,waypoint.z-p.z}; game_->ExecuteCommand(move,.05f);
        }
      }
      const auto p=game_->State().Player(); return std::hypot(p.x-destination.x,p.z-destination.z)<.2f;
    };
    if (!walkTo({3,-10}) || !game_->State().CanTrade()) { error_=L"Quartermaster render scenario could not reach the shop."; return false; }
    game_->SetCameraOrbit(0,.45f,10);
    std::string equippedSnapshot;
    for (int stage=0;stage<3;++stage) {
      if (stage==0) { game_->OpenPanel(ui::Panel::Merchant); game_->ShowPanel(ui::Panel::Character); }
      else if (stage==1) {
        c={}; c.purchase=1; game_->ExecuteCommand(c); c.purchase=2; game_->ExecuteCommand(c);
        for (const auto item : {rpg::Item::Helm,rpg::Item::Shield}) {
          c={}; c.inventory={rpg::InventoryAction::Equip,game_->State().Items().Find(item),int(rpg::DescribeItem(item).slot),item}; game_->ExecuteCommand(c);
        }
        if (game_->State().DamageReduction()!=5) { error_=L"Purchased equipment render scenario failed."; return false; }
        game_->OpenPanel(ui::Panel::Character); game_->ShowPanel(ui::Panel::Inventory);
        equippedSnapshot=game_->State().Save();
      } else {
        c={}; c.inventory={rpg::InventoryAction::Unequip,int(rpg::GearSlot::Offhand),-1,rpg::Item::Shield}; game_->ExecuteCommand(c);
        for (auto item : {rpg::Item::Starter,rpg::Item::Tunic,rpg::Item::Shield,rpg::Item::Potion}) {
          c={}; c.trade={rpg::TradeAction::Sell,item,game_->State().Items().Find(item)}; game_->ExecuteCommand(c);
        }
        c={}; c.inventory={rpg::InventoryAction::Split,game_->State().Items().Find(rpg::Item::Potion),-1,rpg::Item::Potion,2}; game_->ExecuteCommand(c);
        if (game_->State().BuybackCount(rpg::Item::Shield)!=1 || game_->State().Potions()!=4) { error_=L"Buyback render scenario failed."; return false; }
        game_->OpenPanel(ui::Panel::Merchant); game_->ShowPanel(ui::Panel::Character);
        renderer_->BeginFrame(); game_->Render(*renderer_); if (!renderer_->EndFrame(false)) return false;
        const int cell=game_->State().Items().Find(rpg::Item::Potion);
        Input sample; Input::Frame frame; frame.focused=true; frame.client={1028+(cell%6)*40,274+(cell/6)*42};
        frame.screen=frame.client; sample.Update(frame);
        frame.down[VK_LBUTTON]=true; sample.Update(frame); game_->Update(.01,sample,0);
        frame.down[VK_LBUTTON]=false; frame.client={720,585}; frame.screen=frame.client; sample.Update(frame); game_->Update(.01,sample,0);
      }
      for (const auto dimensions : {std::pair<int,int>{960,600},{1600,900},{1280,720}}) {
        if (!renderer_->Resize(dimensions.first,dimensions.second)) { error_=renderer_->Error(); return false; }
        std::wstring suffix=stage==0?L"-quartermaster":stage==1?L"-purchased-equipment":L"-buyback";
        if (dimensions.first!=1280) suffix+=L"-"+std::to_wstring(dimensions.first)+L"x"+std::to_wstring(dimensions.second);
        if (!capture(suffix.c_str())) { error_=renderer_->Error(); return false; }
      }
    }
    if (!game_->RestoreSnapshot(equippedSnapshot)) return false;
    game_->OpenPanel(ui::Panel::Character); game_->ShowPanel(ui::Panel::Inventory);
    if (!walkTo({0,-8})) { error_=L"Equipped character inspection could not reach clear ground."; return false; }
    game_->SetCameraOrbit(game_->State().Facing()+3.14159265f,.2f,4);
    if (!capture(L"-watchman-character",false)) { error_=renderer_->Error(); return false; }
    if (!walkTo({12,-3.8f})) { error_=L"Equipped sparring scenario could not reach the guard."; return false; }
    c={}; c.interact=true; game_->ExecuteCommand(c);
    if (!game_->State().Sparring()) { error_=L"Equipped sparring render scenario could not start."; return false; }
    game_->SetCameraOrbit(0,.45f,7);
    if (!capture(L"-equipped-sparring")) { error_=renderer_->Error(); return false; }
    if (!game_->RestoreSnapshot(original)) return false;
  }
  if (game_->State().HasWardArmor()) {
    struct EnvironmentView { const wchar_t* name; DirectX::XMFLOAT3 eye,target; };
    const EnvironmentView views[]={
      {L"overview",{28,42,-36},{0,0,0}}, {L"canopy",{18,4,-7},{14,2.2f,-2}},
      {L"training",{16,8,-7},{12,0,-2}}, {L"dock",{-17,5,-7},{-12,0,0}},
      {L"village",{0,8,-25},{0,2.7f,-8}}, {L"stall",{-.6f,3.3f,-17},{3,1.4f,-12.8f}},
      {L"ruin",{5,4,8},{0,1.6f,15}}};
    for (const auto& view : views) {
      renderer_->BeginFrame(); game_->Render(*renderer_,false);
      renderer_->SetCamera(view.eye,view.target);
      const auto base=std::filesystem::path(capturePath);
      const auto path=base.parent_path()/(base.stem().wstring()+L"-environment-"+view.name+base.extension().wstring());
      if (!renderer_->Capture(path.wstring()) || !renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
    }
    const wchar_t* poses[]={L"front",L"profile",L"back",L"walk-left",L"walk-right",L"strike-start",L"strike-mid",L"strike-end"};
    for (int pose=0;pose<8;++pose) {
      renderer_->BeginFrame();
      if (!renderer_->BeginPortrait(0,0,1,1)) { error_=renderer_->Error(); return false; }
      renderer_->SetCamera({0,1.3f,3.8f},{0,1.3f,0});
      CharacterOutfit outfit; outfit.armor=pose>=2; outfit.helm=pose>=2; outfit.shield=pose>=2; outfit.rewardWeapon=pose>=2;
      const float yaw=pose==1?1.5707963f:pose==2?3.14159265f:pose>=3?.4f:0;
      DrawCharacter(*renderer_,{0,0},0,yaw,{.22f,.37f,.54f,1},pose>=5?float(7-pose)*.5f:0,pose==3?-1.f:pose==4?1.f:0,outfit);
      if (!renderer_->EndPortrait()) { error_=renderer_->Error(); return false; }
      renderer_->DrawText(.03f,.04f,"Procedural outfit / pose inspection",{.94f,.89f,.69f,1},2);
      const auto base=std::filesystem::path(capturePath);
      const auto path=base.parent_path()/(base.stem().wstring()+L"-pose-"+poses[pose]+base.extension().wstring());
      if (!renderer_->Capture(path.wstring()) || !renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
    }
  }
  if (game_->State().HasWardArmor()) {
    struct AnimationView { const wchar_t* name; const char* label; animation::Sample sample; };
    const AnimationView views[]={
      {L"idle-inhale","Idle / inhale",{1,.7f}}, {L"idle-exhale","Idle / exhale",{-1,-.7f}},
      {L"jump-rise","Jump / rising",{0,0,1,1}}, {L"jump-fall","Jump / falling",{0,0,1,-1}},
      {L"landing","Landing / knee compression",{0,0,0,0,1}}, {L"recoil","Hit / upper-body recoil",{0,0,0,0,0,1}}};
    const auto base=std::filesystem::path(capturePath);
    for (const auto& view : views) {
      renderer_->BeginFrame();
      if (!renderer_->BeginPortrait(0,0,1,1)) { error_=renderer_->Error(); return false; }
      renderer_->SetCamera({0,1.2f,4},{0,1.2f,0});
      CharacterOutfit outfit; outfit.armor=outfit.helm=outfit.shield=outfit.rewardWeapon=true;
      DrawCharacter(*renderer_,{0,0},0,.55f,{.22f,.37f,.54f,1},0,0,outfit,view.sample);
      if (!renderer_->EndPortrait()) { error_=renderer_->Error(); return false; }
      renderer_->DrawText(.03f,.04f,view.label,{.94f,.89f,.69f,1},2);
      const auto path=base.parent_path()/(base.stem().wstring()+L"-animation-"+view.name+base.extension().wstring());
      if (!renderer_->Capture(path.wstring()) || !renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
    }
    // Exercise the actual simulation-to-animation path, not just authored poses.
    const auto beforeJump=game_->State().Save();
    if (!game_->RestoreSnapshot(beforeJump)) return false;
    game_->SetCameraOrbit(3.14159265f,.2f,5);
    const wchar_t* stages[]={L"rise",L"apex",L"fall",L"landing"};
    int stage=0;
    for (int step=1;step<=15;++step) {
      rpg::Command command; command.jump=step==1; game_->ExecuteCommand(command,.05f);
      if (step!=3+stage*4) continue;
      renderer_->BeginFrame(); game_->Render(*renderer_,false);
      const auto path=base.parent_path()/(base.stem().wstring()+L"-jump-"+stages[stage++]+base.extension().wstring());
      if (!renderer_->Capture(path.wstring()) || !renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
    }
    if (!game_->RestoreSnapshot(beforeJump)) return false;
  }
  if (!renderer_->ValidateDebugMessages()) { error_=renderer_->Error(); return false; }
  return true;
}

bool Engine::Benchmark(const std::wstring& reportPath) {
  LARGE_INTEGER frequency{},start{},finish{}; QueryPerformanceFrequency(&frequency);
  std::vector<double> cpu,gpu;
  // Same stationary spawn camera, 90 warmup + 240 measured frames, no captures.
  for (int frame=0;frame<330;++frame) {
    QueryPerformanceCounter(&start);
    const auto samples=renderer_->GpuSamples();
    game_->Update(1.0/60.0,input_,0);
    renderer_->BeginFrame(); game_->Render(*renderer_);
    if (!renderer_->EndFrame(false)) { error_=renderer_->Error(); return false; }
    QueryPerformanceCounter(&finish);
    if (frame>=90) {
      cpu.push_back(1000.0*double(finish.QuadPart-start.QuadPart)/double(frequency.QuadPart));
      if (renderer_->GpuSamples()!=samples) gpu.push_back(renderer_->GpuMilliseconds());
    }
  }
  const auto percentile=[](std::vector<double> values,double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(),values.end()); return values[static_cast<std::size_t>((values.size()-1)*p)];
  };
  PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb=sizeof(memory);
  const bool memoryAvailable=GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory))!=FALSE;
  std::ofstream report{std::filesystem::path(reportPath)};
  report << std::fixed << std::setprecision(3)
    << "Scene: stationary spawn, 1280x720, hidden window, present interval 0\n"
    << "Warmup frames: 90\nMeasured frames: " << cpu.size() << "\nGPU samples: " << gpu.size()
    << "\nCPU frame median ms (includes submission/present): " << percentile(cpu,.5)
    << "\nCPU frame p95 ms: " << percentile(cpu,.95)
    << "\nGPU measured median ms: " << percentile(gpu,.5)
    << "\nGPU measured p95 ms: " << percentile(gpu,.95)
    << "\nDraw submissions including HUD: " << renderer_->DrawCalls()
    << "\nSubmitted triangles including HUD: " << renderer_->Triangles()
    << "\nImmutable mesh bytes: " << renderer_->MeshBytes()
    << "\nMaterial and shadow texture bytes: " << renderer_->MaterialBytes();
  report << "\nInterface font and map texture bytes: " << renderer_->InterfaceBytes();
  if (memoryAvailable) report << "\nProcess working set MiB: " << memory.WorkingSetSize/1048576.0
    << "\nProcess private bytes MiB: " << memory.PrivateUsage/1048576.0;
  report << "\nNot an interactive FPS guarantee. Driver, visibility and workload affect results.\n";
  if (!report) { error_=L"Could not write benchmark report."; return false; }
  return true;
}

bool Engine::BenchmarkSuite(const std::wstring& reportPath) {
  // Authored in-memory scenarios. No player save or desktop input is touched.
  const std::string combat="MEDITERRANEAN 3\n-4 5.2 100 0 1 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.5 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  const std::string veteran="MEDITERRANEAN 3\n3 -10 140 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
  const auto original=game_->State().Save();
  const char* names[]={"Stationary spawn HUD","Village walking and orbit","Repeated combat and orbit","Merchant, character and bag"};
  const auto percentile=[](std::vector<double> values,double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(),values.end()); return values[std::size_t((values.size()-1)*p)];
  };
  LARGE_INTEGER frequency{},start{},finish{}; QueryPerformanceFrequency(&frequency);
  std::ofstream report{std::filesystem::path(reportPath)};
  report<<std::fixed<<std::setprecision(3)
    <<"Hidden 1280x720 workload suite; present interval 0; fixed simulation step 1/60 s\n"
    <<"Each workload: 90 warmup + 600 measured frames. Setup/save loads excluded.\n"
    <<"Combat replays a 3-second window; GPU observations are asynchronous (latest ready sample per frame).\n"
    <<"Not interactive FPS, a long-session test or a GPU trace. No player saves written.\n";
  for (int scene=0;scene<4;++scene) {
    game_->Initialize(); game_->OpenPanel(ui::Panel::None); game_->SetCameraOrbit(0,.45f,10);
    if ((scene==2 && !game_->RestoreSnapshot(combat)) || (scene==3 && !game_->RestoreSnapshot(veteran))) {
      error_=L"Benchmark scenario failed save validation."; return false;
    }
    if (scene==3) {
      for (int offer : {1,2}) {
        rpg::Command c; c.purchase=offer; game_->ExecuteCommand(c);
        const auto item=rpg::shopOffers[offer].item;
        c={}; c.inventory={rpg::InventoryAction::Equip,game_->State().Items().Find(item),int(rpg::DescribeItem(item).slot),item}; game_->ExecuteCommand(c);
      }
      game_->OpenPanel(ui::Panel::Merchant); game_->ShowPanel(ui::Panel::Character);
    }
    std::vector<double> cpu,gpu; double distance=0; unsigned drawPeak=0,trianglePeak=0,windups=0,hits=0,effectPeak=0,effectFrames=0;
    const rpg::Vec2 route[]={{3,-10},{3,-6},{0,-6},{0,-10}}; unsigned waypoint=0;
    for (int frame=0;frame<690;++frame) {
      if (scene==2 && frame%180==0 && !game_->RestoreSnapshot(combat)) return false;
      rpg::Command command;
      const auto before=game_->State().Player(); const float health=game_->State().Health();
      if (scene==1) {
        if (std::hypot(before.x-route[waypoint].x,before.z-route[waypoint].z)<.15f) waypoint=(waypoint+1)%4;
        command.movement={route[waypoint].x-before.x,route[waypoint].z-before.z};
      }
      if (scene==2) { command.nextTarget=frame%180==0; if (frame%180==80) command.ability=0; }
      QueryPerformanceCounter(&start); const auto samples=renderer_->GpuSamples();
      game_->ExecuteCommand(command,1.f/60);
      if (scene==1 || scene==2) game_->SetCameraOrbit(frame*.012f,.4f+.18f*std::sin(frame*.021f),7.5f+1.5f*std::sin(frame*.017f));
      renderer_->BeginFrame(); game_->Render(*renderer_);
      if (!renderer_->EndFrame(false)) { error_=renderer_->Error(); return false; }
      QueryPerformanceCounter(&finish);
      if (frame<90) continue;
      cpu.push_back(1000.0*double(finish.QuadPart-start.QuadPart)/double(frequency.QuadPart));
      if (renderer_->GpuSamples()!=samples) gpu.push_back(renderer_->GpuMilliseconds());
      drawPeak=std::max(drawPeak,renderer_->DrawCalls()); trianglePeak=std::max(trianglePeak,renderer_->Triangles());
      effectPeak=std::max(effectPeak,renderer_->EffectVertices());
      if (renderer_->EffectVertices()>0) ++effectFrames;
      const auto after=game_->State().Player(); distance+=std::hypot(after.x-before.x,after.z-before.z);
      if (game_->State().Health()<health) ++hits;
      for (const auto& enemy : game_->State().Enemies()) if (enemy.windup>0) { ++windups; break; }
    }
    if ((scene==1 && distance<10) || (scene==2 && (windups==0 || hits==0 || effectFrames==0)) || (scene!=2 && effectFrames>0)) {
      error_=L"Benchmark workload did not exercise movement or combat."; return false;
    }
    report<<"\n"<<names[scene]<<"\nCPU samples: "<<cpu.size()<<"; GPU observations: "<<gpu.size()
      <<"\nCPU median / p95 / max ms: "<<percentile(cpu,.5)<<" / "<<percentile(cpu,.95)<<" / "<<percentile(cpu,1)
      <<"\nGPU median / p95 ms: "<<percentile(gpu,.5)<<" / "<<percentile(gpu,.95)
      <<"\nPeak submissions / triangles: "<<drawPeak<<" / "<<trianglePeak
      <<"\nCombat effect frames / peak vertices: "<<effectFrames<<" / "<<effectPeak
      <<"\nWalked distance / windup frames / received hits: "<<distance<<" / "<<windups<<" / "<<hits<<'\n';
  }
  report<<"\nImmutable mesh bytes: "<<renderer_->MeshBytes()<<"\nMaterial/shadow bytes: "<<renderer_->MaterialBytes()
    <<"\nInterface bytes: "<<renderer_->InterfaceBytes()<<'\n';
  PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb=sizeof(memory);
  if (GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory)))
    report<<"Working set / private MiB at end: "<<memory.WorkingSetSize/1048576.0<<" / "<<memory.PrivateUsage/1048576.0<<'\n';
  if (!game_->RestoreSnapshot(original)) return false;
  if (!report) { error_=L"Could not write workload report."; return false; }
  if (!renderer_->ValidateDebugMessages()) { error_=renderer_->Error(); return false; }
  return true;
}

int Engine::Run() {
  LARGE_INTEGER frequency = {};
  LARGE_INTEGER last = {};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&last);

  MSG msg = {};
  while (running_) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running_ = false;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (!running_) break;

    input_.Update(window_->GetHandle());
    if (IsIconic(window_->GetHandle()) || GetForegroundWindow() != window_->GetHandle()) {
      game_->CancelUiGesture();
      window_->ConsumeWheel();
      WaitMessage();
      QueryPerformanceCounter(&last);
      continue;
    }

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    const double deltaSeconds = static_cast<double>(now.QuadPart - last.QuadPart)
      / static_cast<double>(frequency.QuadPart);
    last = now;

    Tick(std::min(deltaSeconds, 0.05));
  }

  return exitCode_;
}

void Engine::Tick(double deltaSeconds) {
  if (!renderer_) return;

  RECT client{};
  GetClientRect(window_->GetHandle(), &client);
  if (renderer_->Width()!=client.right || renderer_->Height()!=client.bottom) game_->CancelUiGesture();
  if (!renderer_->Resize(client.right, client.bottom)) {
    MessageBoxW(window_->GetHandle(), renderer_->Error().c_str(), L"Renderer error", MB_OK | MB_ICONERROR);
    exitCode_ = 1;
    running_ = false;
    return;
  }
  game_->Update(deltaSeconds, input_, window_->ConsumeWheel());
  const bool fullscreenRequest=game_->TakeFullscreenRequest();
  if (input_.Pressed(VK_F11) || fullscreenRequest) {
    game_->CancelUiGesture();
    if (!window_->SetFullscreen(!window_->Fullscreen())) {
      error_=L"Could not change the game window display mode."; exitCode_=1; running_=false; return;
    }
    GetClientRect(window_->GetHandle(),&client);
    if (!renderer_->Resize(client.right,client.bottom)) { error_=renderer_->Error(); exitCode_=1; running_=false; return; }
  }
  game_->SetFullscreenState(window_->Fullscreen());
  if (game_->QuitRequested()) { running_=false; return; }

  renderer_->BeginFrame();
  game_->Render(*renderer_);
  if (!renderer_->EndFrame()) {
    MessageBoxW(window_->GetHandle(), renderer_->Error().c_str(), L"Renderer error", MB_OK | MB_ICONERROR);
    exitCode_ = 1;
    running_ = false;
  }
}
