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
