#include "engine/Engine.h"
#include "engine/Renderer.h"
#include "engine/Audio.h"
#include "game/Game.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

// Native tests enter the same failure handler used by Tick. Synthetic HRESULTs
// never reset the OS driver, edit TDR settings or touch the player's saves.
struct GraphicsLifecycleTestAccess {
  static Renderer& Render(Engine& e){return *e.renderer_;}
  static Game& Session(Engine& e){return *e.game_;}
  static Input& Controls(Engine& e){return e.input_;}
  static bool Recover(Engine& e){return e.TryRecoverGraphics();}
  static void Fail(Renderer& r,HRESULT code){r.Check(code,L"Injected native lifecycle regression");}
  static unsigned Recoveries(const Engine& e){return e.graphicsRecoveries_;}
  static bool ClockReset(const Engine& e){return e.resetFrameClock_;}
};
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string Bytes(const std::filesystem::path& path) {
  std::ifstream file(path,std::ios::binary);Check(bool(file),"Read recovery capture");
  return {std::istreambuf_iterator<char>(file),{}};
}
void Policy() {
  for(HRESULT hr:{DXGI_ERROR_DEVICE_RESET,DXGI_ERROR_DEVICE_REMOVED,DXGI_ERROR_DEVICE_HUNG,DXGI_ERROR_DRIVER_INTERNAL_ERROR})
    Check(graphics::Recoverable(hr,S_OK),"Lost device was not recoverable");
  for(HRESULT hr:{S_OK,E_FAIL,E_INVALIDARG,E_OUTOFMEMORY,DXGI_ERROR_INVALID_CALL})
    Check(!graphics::Recoverable(hr,S_OK),"Non-device error was retried");
  Check(graphics::Recoverable(E_FAIL,DXGI_ERROR_DEVICE_RESET),"Removal reason lost behind generic error");
  Check(!graphics::Recoverable(DXGI_ERROR_DEVICE_REMOVED,DXGI_ERROR_INVALID_CALL),"Invalid GPU commands must not be retried");
  graphics::RecoveryBudget budget;
  Check(budget.Request(100) && budget.Request(1000) && !budget.Request(2000),"Unbounded recovery storm");
  Check(!budget.Request(60099) && budget.Request(60100) && !budget.Request(60101),"Rolling recovery interval");
  Check(budget.Request(61000) && !budget.Request(61001),"Independent recovery attempt expiry");
}
void Protocol() {
  {Renderer r;Check(!r.BeginFrame() && !r.EndFrame() && !r.Resize(640,360),"Uninitialized renderer accepted work");}
  Win32Window w;Check(w.Create(GetModuleHandleW(nullptr),640,360,L"Graphics lifecycle",false),"Create native test window");
  for(int scenario=0;scenario<4;++scenario) {
    Renderer r;Check(r.Initialize(w.GetHandle(),640,360),"Initialize native protocol renderer");
    if(scenario==0) {
      Check(r.Resize(0,0) && r.Width()==640 && r.BeginFrame(),"Minimize no-op");
      Check(!r.BeginFrame(),"Nested frame discarded in-flight work");
    } else if(scenario==1) {
      Check(r.BeginFrame(),"Start resize protocol frame");Check(!r.Resize(800,600),"Resize accepted during a frame");
    } else if(scenario==2) {
      Check(r.BeginFrame() && r.BeginPortrait(.1f,.1f,.4f,.7f),"Begin native portrait");
      Check(!r.EndFrame(),"Present accepted an unclosed portrait");
    } else {
      Check(!r.Resize(20000,360) && r.Width()==640,"Oversize resize accepted or changed dimensions");
    }
    const auto error=r.Error();const auto draws=r.LastDrawCalls();
    Check(!r.EndFrame() && !r.BeginFrame() && error==r.Error() && draws==r.LastDrawCalls(),"Failed renderer resumed or overwrote its first error");
    Check(r.ValidateDebugMessages(),"Protocol guard submitted invalid D3D commands");
  }
  Renderer r;Check(r.Initialize(w.GetHandle(),640,360),"Initialize discarded portrait renderer");
  Check(r.BeginFrame() && r.BeginPortrait(.1f,.1f,.4f,.7f),"Begin discarded portrait");
  Check(!r.SetTerrainSurface({},256),"Malformed upload must reject the frame");
  const auto cause=r.Error();
  Check(!r.EndPortrait() && !r.EndFrame(false) && r.FailureCode()==S_OK && r.Error()==cause,
    "Unwinding a discarded portrait poisoned the device or replaced its cause");
  Check(r.BeginFrame(),"Retry after discarded portrait");r.DrawHudRect(.2f,.2f,.4f,.4f,{.2f,.4f,.6f,1});
  Check(r.EndFrame(false) && r.ValidateDebugMessages(),"Discarded portrait leaked GPU state");
}
void NativeRecovery() {
  Engine engine;Check(engine.Initialize(GetModuleHandleW(nullptr),960,540,false,false),"Initialize hidden engine");
  auto& game=GraphicsLifecycleTestAccess::Session(engine);
  rpg::Command walk;walk.movement={.2f,.5f};game.ExecuteCommand(walk,.1f);
  game.SetCameraOrbit(.4f,.25f,6.f);game.OpenPanel(ui::Panel::Character);
  const auto save=game.State().Save();const auto yaw=game.CameraYaw(),pitch=game.CameraPitch(),distance=game.CameraRange();
  const auto directory=std::filesystem::current_path()/"graphics-lifecycle-review";
  std::filesystem::create_directories(directory);
  const auto frame=[&](const wchar_t* name) {
    auto& r=GraphicsLifecycleTestAccess::Render(engine);
    Check(r.BeginFrame(),"Begin recovery frame");game.Render(r);
    Check(r.Capture((directory/name).wstring()),"Capture recovery frame");
    Check(r.EndFrame(false) && r.ValidateDebugMessages(),"Submit recovery frame / D3D validation");
    Check(r.BindingStats().requests>0 && r.BindingStats().skipped>0 && r.BindingStats().rejected==0,"Native resource bindings were bypassed or rejected");
    std::cout<<"Native frame bindings: requests "<<r.BindingStats().requests<<", calls "<<r.BindingStats().calls
      <<", skipped "<<r.BindingStats().skipped<<", alias unbinds "<<r.BindingStats().hazardUnbinds<<'\n';
    Check(r.HasTerrainSurface() && r.HasMapArtwork(),"Lazy scene resources missing");
    Check(game.State().Save()==save,"Rendering/recovery changed simulation state");
  };
  frame(L"before.png");
  for(int generation=0;generation<2;++generation) {
    auto& r=GraphicsLifecycleTestAccess::Render(engine);
    Check(r.Resize(1200,700) && r.BeginFrame(),"Resize after a completed frame");game.Render(r);
    Check(r.EndFrame(false),"Resized frame");
    Input::Frame held;held.focused=true;held.down['1']=true;held.down[VK_RBUTTON]=true;
    GraphicsLifecycleTestAccess::Controls(engine).Update(held);
    // One failure while idle, one in the middle of an unfinished GPU frame.
    if(generation==1){Check(r.BeginFrame(),"Mid-frame failure setup");game.Render(r);}
    GraphicsLifecycleTestAccess::Fail(r,generation?DXGI_ERROR_DEVICE_REMOVED:DXGI_ERROR_DEVICE_RESET);
    const auto first=r.Error();Check(!r.BeginFrame() && !r.EndFrame() && r.Error()==first,"Failure lost during frame cancellation");
    Check(GraphicsLifecycleTestAccess::Recover(engine),"Native device reconstruction failed");
    Check(&GraphicsLifecycleTestAccess::Session(engine)==&game && game.State().Save()==save,"Recovery replaced the game session");
    Check(game.CameraYaw()==yaw && game.CameraPitch()==pitch && game.CameraRange()==distance,"Recovery changed camera");
    Check(GraphicsLifecycleTestAccess::ClockReset(engine) && !GraphicsLifecycleTestAccess::Controls(engine).Held('1'),"Clock/input not resynchronized");
    auto& restored=GraphicsLifecycleTestAccess::Render(engine);
    const auto prepared=restored.TextureLoadingStats();
    Check(prepared.prepared==19 && prepared.delivered==19 && prepared.liveBytes==0 && prepared.livePackets==0 &&
      prepared.peakBytes<=resources::TexturePreparation::defaultBudget && prepared.workerThread!=GetCurrentThreadId(),
      "Reconstruction bypassed CPU preparation or retained staging packets");
    Check(!restored.HasTerrainSurface() && !restored.HasMapArtwork(),"Old-device views survived replacement");
    frame(generation?L"restored-2.png":L"restored-1.png");
    Check(Bytes(directory/"before.png")==Bytes(directory/(generation?"restored-2.png":"restored-1.png")),"Restored frame differs from original session");
  }
  Check(GraphicsLifecycleTestAccess::Recoveries(engine)==2,"Recovery counter");
  GraphicsLifecycleTestAccess::Fail(GraphicsLifecycleTestAccess::Render(engine),DXGI_ERROR_DEVICE_RESET);
  Check(!GraphicsLifecycleTestAccess::Recover(engine) && engine.Error().find(L"limit")!=std::wstring::npos,"Native recovery storm not stopped");
}
}
int main()try{Policy();Protocol();NativeRecovery();std::cout<<"PASS: DX11 frame/resize guards, bounded reconstruction, unchanged session/camera and identical restored frames\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
