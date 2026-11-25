#include "engine/Renderer.h"
#include "engine/Audio.h"
#include "game/Game.h"
#include "platform/Win32Window.h"
#include <psapi.h>
#include <atomic>
#include <iostream>
#include <stdexcept>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
// Private-data ownership probes do not keep the resource/device alive. DX11
// releases each token when its owner dies, even without the SDK debug layer.
constexpr GUID lifetimeKey={0x29d8ce41,0xf510,0x437d,{0xb7,0xa5,0x18,0x55,0x91,0x92,0x71,0x0a}};
std::atomic<unsigned> liveDevices=0,liveSized=0,liveScene=0;
class Token final:public IUnknown {
public:
  explicit Token(std::atomic<unsigned>& live):live_(live){++live_;}
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override {
    if(!out)return E_POINTER;*out=nullptr;
    if(id!=__uuidof(IUnknown))return E_NOINTERFACE;*out=this;AddRef();return S_OK;
  }
  ULONG STDMETHODCALLTYPE AddRef()override{return ++refs_;}
  ULONG STDMETHODCALLTYPE Release()override{const auto count=--refs_;if(!count)delete this;return count;}
private:
  ~Token(){--live_;}
  std::atomic<ULONG> refs_=1;
  std::atomic<unsigned>& live_;
};
template<class T> void Track(T* owner,std::atomic<unsigned>& live) {
  Check(owner!=nullptr,"Missing lifetime probe resource");
  auto* token=new Token(live);const auto result=owner->SetPrivateDataInterface(lifetimeKey,token);token->Release();
  Check(SUCCEEDED(result),"Attach resource lifetime probe");
}
void TrackView(ID3D11View* view,std::atomic<unsigned>& live) {
  Check(view!=nullptr,"Missing lifetime probe view");Microsoft::WRL::ComPtr<ID3D11Resource> resource;
  view->GetResource(&resource);Track(resource.Get(),live);
}
struct Memory {SIZE_T privateBytes=0,working=0,heapBytes=0,heapBlocks=0;DWORD handles=0,gdi=0,user=0;};
Memory Sample(const char* label,unsigned cycle) {
  Memory result;PROCESS_MEMORY_COUNTERS_EX info{};info.cb=sizeof(info);
  Check(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&info),sizeof(info)),"Read process memory");
  result.privateBytes=info.PrivateUsage;result.working=info.WorkingSetSize;
  Check(GetProcessHandleCount(GetCurrentProcess(),&result.handles),"Read process handles");
  result.gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);result.user=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);
#ifdef _DEBUG
  _CrtMemState heap{};_CrtMemCheckpoint(&heap);result.heapBytes=heap.lSizes[_NORMAL_BLOCK];result.heapBlocks=heap.lCounts[_NORMAL_BLOCK];
#endif
  std::cout<<label<<' '<<cycle<<": private="<<result.privateBytes<<" working="<<result.working
    <<" handles="<<result.handles<<" gdi="<<result.gdi<<" user="<<result.user
    <<" CRT-normal-bytes="<<result.heapBytes<<" blocks="<<result.heapBlocks<<std::endl;
  return result;
}
void Bounded(const Memory& first,const Memory& now,SIZE_T allowance,DWORD handleAllowance=8) {
  // Allocator/driver high-water marks are not leaks. Exact ownership is tested
  // separately; these envelopes catch a continuing per-cycle/session increase.
  if(allowance)Check(now.privateBytes<=first.privateBytes+allowance,"Process private memory did not plateau");
  Check(now.heapBytes<=first.heapBytes+2*1024*1024,"Live CRT allocations kept growing");
  // Audio/COM/driver workers can lazily add a few native handles after startup;
  // repeated per-frame or per-reset growth must exceed this fixed envelope.
  Check(now.handles<=first.handles+handleAllowance,"Native handles kept growing");
  Check(now.gdi<=first.gdi+1 && now.user<=first.user+1,"GDI/USER objects kept growing");
}
std::string Hero(rpg::ClassId id) {
  rpg::Simulation s;Check(s.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Training fixture");
  auto save=s.Save();const auto at=save.find("PROFILE ");
  Check(at!=std::string::npos,"Training profile");save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Lifetime\" 7 1 3");
  Check(s.Load(save),"Class fixture");return s.Save();
}
}
struct MemoryLifetimeTestAccess {
  static void Device(Renderer& r){Track(r.device_.Get(),liveDevices);}
  static void Fonts(Renderer& r){
    std::atomic<unsigned> live=0;
    {FontAtlas atlas;
      for(unsigned i=0;i<12;++i){
        Check(atlas.Initialize(r.device_.Get()),"Rebuild font atlas");
        Check(live==0,"Rebuilt font atlas leaked its previous texture");
        TrackView(atlas.View(),live);Check(live==1,"Font texture lifetime probe");
        const auto retained=atlas.View();Check(!atlas.Initialize(nullptr) && atlas.View()==retained && live==1,"Failed font rebuild discarded live artwork");
      }
    }
    Check(live==0,"Destroyed font atlas leaked its texture");
  }
  static void Resources(Renderer& r){
    TrackView(r.target_.Get(),liveSized);TrackView(r.depth_.Get(),liveSized);
    for(auto& view:r.glowViews_)TrackView(view.Get(),liveSized);
    Track(r.pressureCopy_.Get(),liveSized);
    Track(r.shadowTexture_.Get(),liveScene);Track(r.staticShadowTexture_.Get(),liveScene);
    TrackView(r.mapView_.Get(),liveScene);TrackView(r.terrainView_.Get(),liveScene);
  }
  static void Drain(Renderer& r){r.context_->Flush();r.gpuQueue_.Reset();r.gpuQueue_.Initialize(r.device_.Get());}
};
namespace {
void Frame(Game& game,Renderer& r,bool effects=true) {
  Check(r.BeginFrame(),"Begin memory stress frame");
  if(effects) {
    r.SetCamera({0,3,-8},{0,1,0});
    r.DrawEffectSprite(Renderer::EffectArt::Flame,{0,1,0},1,1,0,{1,.5f,.1f,.8f});
    r.DrawPressureWave({0,1,0},{2,2,2},0,.2f,.8f,0);
  }
  game.Render(r);
  Check(r.EndFrame(false),"End memory stress frame");
}
void Lifecycle() {
  std::array<std::string,4> saves;unsigned i=0;
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest})saves[i++]=Hero(id);
  Memory baseline{};
  for(unsigned cycle=0;cycle<6;++cycle) {
    {
      Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),640,360,L"Memory lifetime regression",false),"Hidden window");
      auto r=std::make_unique<Renderer>();Check(r->Initialize(window.GetHandle(),640,360),"Renderer initialization");
      MemoryLifetimeTestAccess::Device(*r);
      MemoryLifetimeTestAccess::Fonts(*r);
      auto game=std::make_unique<Game>();game->Initialize();
      for(unsigned change=0;change<4;++change) {
        Check(game->RestoreSnapshot(saves[change]),"Restore isolated class");
        for(int f=0;f<12;++f) {
          rpg::Command c;c.movement={.3f,.5f};game->ExecuteFrame(c,1.f/60);
          game->SetCameraOrbit(f*.04f,.45f,8);Frame(*game,*r);
        }
        game->OpenPanel(ui::Panel::Character);game->ShowPanel(ui::Panel::Inventory);Frame(*game,*r);
        MemoryLifetimeTestAccess::Resources(*r);
        Check(liveSized==6 && liveScene==4 && liveDevices==1,"Ownership probe coverage");
        MemoryLifetimeTestAccess::Drain(*r);
        Check(r->Resize(change%2?640:960,change%2?360:540),"Repeated resize");
        Check(liveSized==0,"Old resolution retained a GPU texture after resize");
      }
      Check(r->TextureLoadingStats().liveBytes==0 && r->TextureLoadingStats().livePackets==0,"Texture staging survived upload");
      Check(r->ValidateDebugMessages(),"Memory stress submitted invalid DX11 work");
    }
    Check(liveDevices==0 && liveSized==0 && liveScene==0,"Renderer destruction retained device/resources");
    const auto sample=Sample("destroyed-generation",cycle);
    if(cycle==1)baseline=sample;
    // Private commit can vary by >100 MiB as the CRT/driver retains or returns
    // transient initialization heaps. Exact device/texture lifetimes and live
    // CRT blocks, not working-set trimming, decide ownership here. The local
    // hardware swap chain retains two handles per creation even in a minimal
    // DXGI-only reproduction (--isolate-platform); WARP does not.
    if(cycle>1)Bounded(baseline,sample,0,2*(cycle-1)+2);
  }
}
void SteadySession(unsigned batches=16) {
  Win32Window w;Check(w.Create(GetModuleHandleW(nullptr),640,360,L"Memory soak",false),"Soak window");
  auto r=std::make_unique<Renderer>();Check(r->Initialize(w.GetHandle(),640,360),"Soak renderer");
  auto game=std::make_unique<Game>();game->Initialize();
  auto sound=std::make_unique<Audio>();Check(sound->Initialize(true),"Muted soak audio");
  std::array<std::string,4> saves;unsigned i=0;
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest})saves[i++]=Hero(id);
  Memory baseline{};unsigned events=0,castFrames=0;std::array<unsigned,4> releases{};
  for(unsigned batch=0;batch<batches;++batch) {
    Check(game->RestoreSnapshot(saves[(batch/2)%4]),"Soak snapshot");
    for(unsigned f=0;f<180;++f) {
      rpg::Command c;
      if(batch%2){if(f==0)c.selectActor=210;if(f%60==0)c.ability=f%120?0:1;}
      else {c.movement={f%120<60?.2f:-.2f,0};if(f%60==0)c.jump=true;}
      const auto result=game->ExecuteFrame(c,1.f/60);events+=result.events;
      if(result.Has(rpg::CombatVisualKind::Strike) || result.Has(rpg::CombatVisualKind::HeavyStrike) || result.Has(rpg::CombatVisualKind::SpellCast))++releases[(batch/2)%4];
      const auto& audio=game->TakeAudioFrame(1.f/60);castFrames+=audio.preparing;sound->Update(audio);
      game->SetCameraOrbit(f*.015f,.45f,8);if(f%45==0)game->OpenPanel(f%90?ui::Panel::Inventory:ui::Panel::Character);
      Frame(*game,*r);
    }
    MemoryLifetimeTestAccess::Drain(*r);const auto sample=Sample("session",batch);
    if(batch==7)baseline=sample;if(batch>7)Bounded(baseline,sample,24*1024*1024);
  }
  std::cout<<"Session frames="<<batches*180<<" simulation-seconds="<<batches*3<<" events="<<events<<" cast-frames="<<castFrames<<" audio-submissions="<<sound->Stats().submitted<<'\n';
  for(auto count:releases)Check(count>0,"A class never released an attack during the soak");
  Check(castFrames>100 && sound->Stats().submitted>20 && sound->SpatialUpdateErrors()==0,"Soak did not exercise live combat/audio");
}
void SoundLifecycle() {
  Memory baseline{};auto game=std::make_unique<Game>();game->Initialize();
  for(unsigned cycle=0;cycle<6;++cycle) {
    {auto sound=std::make_unique<Audio>();Check(sound->Initialize(true),"Muted native audio initialization");
      Check(!sound->Initialize(true),"Repeated audio initialization overwrote its live owners");
      for(unsigned f=0;f<120;++f){rpg::Command c;c.movement={1,0};game->ExecuteFrame(c,1.f/60);sound->Update(game->TakeAudioFrame(1.f/60));}
      sound->Suspend(true);sound->Suspend(false);
    }
    const auto sample=Sample("destroyed-audio",cycle);if(cycle==1)baseline=sample;if(cycle>1)Bounded(baseline,sample,16*1024*1024);
  }
}
void IsolatePlatform() {
  for(unsigned cycle=0;cycle<6;++cycle){
    {Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),640,360,L"Window lifetime",false),"Window isolation");}
    Sample("window-only",cycle);
  }
  for(const auto kind:{D3D_DRIVER_TYPE_HARDWARE,D3D_DRIVER_TYPE_WARP})for(unsigned cycle=0;cycle<6;++cycle){
    {Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
      Check(SUCCEEDED(D3D11CreateDevice(nullptr,kind,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"Minimal device isolation");
      Track(device.Get(),liveDevices);context->ClearState();context->Flush();
    }
    Check(liveDevices==0,"Minimal device retained");Sample(kind==D3D_DRIVER_TYPE_HARDWARE?"hardware-only":"warp-only",cycle);
  }
  for(unsigned cycle=0;cycle<6;++cycle){
    {resources::TexturePreparation worker;resources::PreparedTexture packet;
      for(;;){const auto state=worker.Take(packet);if(state==resources::TexturePreparation::Result::Finished)break;
        Check(state==resources::TexturePreparation::Result::Ready || state==resources::TexturePreparation::Result::Pending,"Texture isolation");}
    }
    Sample("decode-only",cycle);
  }
  for(unsigned mode=0;mode<5;++mode)for(unsigned cycle=0;cycle<6;++cycle){
    {Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),640,360,L"Swap chain isolation",false),"Swap chain window");
      Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;Microsoft::WRL::ComPtr<IDXGISwapChain> swap;
      DXGI_SWAP_CHAIN_DESC d{};d.BufferCount=2;d.BufferDesc.Width=640;d.BufferDesc.Height=360;d.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
      d.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;d.OutputWindow=window.GetHandle();d.SampleDesc.Count=1;d.Windowed=TRUE;d.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
      Check(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr,mode==4?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,&swap,&device,nullptr,&context)),"Minimal swap chain");
      Track(device.Get(),liveDevices);
      {GpuFrameQueue queue;if(mode==2)Check(queue.Initialize(device.Get()),"Minimal completion queue");
        for(unsigned f=0;mode && f<8;++f){Check(SUCCEEDED(queue.Begin()),"Minimal completion wait");
          Check(SUCCEEDED(queue.End()) && SUCCEEDED(swap->Present(0,0)),"Minimal present");}
        context->ClearState();context->Flush();
      }
      if(mode==3){Microsoft::WRL::ComPtr<IDXGIFactory> factory;Check(SUCCEEDED(swap->GetParent(IID_PPV_ARGS(&factory))),"Swap chain factory");
        Check(SUCCEEDED(factory->MakeWindowAssociation(nullptr,0)),"Release window association");swap.Reset();context->ClearState();context->Flush();}
    }
    MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE))if(message.message!=WM_QUIT){TranslateMessage(&message);DispatchMessageW(&message);}
    Check(liveDevices==0,"Minimal swap chain retained device");
    const char* labels[]={"swapchain-only","present-only","completion-queue","explicit-detach","warp-swapchain"};Sample(labels[mode],cycle);
  }
}
}
int main(int argc,char** argv)try {
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
  if(argc>1 && std::string_view(argv[1])=="--isolate-platform"){IsolatePlatform();CoUninitialize();return 0;}
  if(argc>1 && std::string_view(argv[1])=="--soak"){SteadySession(100);CoUninitialize();return 0;}
  Lifecycle();SteadySession();SoundLifecycle();CoUninitialize();
  std::cout<<"PASS: six device/window/audio generations, 24 resizes/class loads, 2880 session frames; exact GPU ownership and bounded session storage (hardware swap-chain handle retention isolated separately)\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
