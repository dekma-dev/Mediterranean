#include "engine/TexturePreparation.h"
#include "engine/Renderer.h"
#include "engine/Engine.h"
#include "platform/Win32Window.h"
#include "platform/MessagePump.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

// Exercise the production loading pump with a hidden window and no foreground
// changes. Public visible startup uses this exact callback.
struct ExecutionTestAccess {
  static bool InterruptLoading(Engine& engine,bool explicitQuit) {
    engine.window_=std::make_unique<Win32Window>();
    if(!engine.window_->Create(GetModuleHandleW(nullptr),640,360,L"Loading exit",false))return false;
    engine.renderer_=std::make_unique<Renderer>();unsigned visits=0;
    return engine.renderer_->Initialize(engine.window_->GetHandle(),640,360,nullptr,[&]{
      if(++visits==20) {
        if(explicitQuit)PostThreadMessageW(GetCurrentThreadId(),WM_QUIT,31,0);
        else {
          PostMessageW(engine.window_->GetHandle(),WM_CLOSE,0,0);
          for(int n=0;n<600;++n)PostThreadMessageW(GetCurrentThreadId(),WM_APP+17,0,0);
        }
      }
      return engine.PumpLoadingMessages();
    });
  }
};
namespace {
using resources::PreparedTexture;
using resources::TexturePreparation;
using resources::TextureRequest;
using Result=TexturePreparation::Result;
using Clock=std::chrono::steady_clock;
void Check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
TextureRequest Small(unsigned id){return {resources::TextureKind::Interface,id,0,id,16,16,true};}
template<class Predicate> void Eventually(Predicate predicate,const char* message) {
  const auto deadline=Clock::now()+std::chrono::seconds(5);
  while(!predicate()){Check(Clock::now()<deadline,message);Sleep(1);}
}
Result Next(TexturePreparation& preparation,PreparedTexture& packet) {
  Result result=Result::Pending;
  Eventually([&]{result=preparation.Take(packet);return result!=Result::Pending;},"Texture delivery timed out");return result;
}
void BudgetAndLifetime() {
  std::atomic<unsigned> calls{0};
  PreparedTexture survivor;
  {
    TexturePreparation preparation({Small(1),Small(2),Small(3),Small(4)},2048,
      [&](const TextureRequest& request,std::uint8_t* pixels,const std::atomic<bool>&){++calls;std::memset(pixels,int(request.resource),request.Bytes());return S_OK;});
    Eventually([&]{return preparation.Stats().prepared==2;},"Two bounded packets did not prepare");
    auto stats=preparation.Stats();Check(stats.liveBytes==2048 && stats.livePackets==2 && calls==2,"Budget reservation did not block producer");
    PreparedTexture first,second,empty;
    Check(Next(preparation,first)==Result::Ready && first.Request().resource==1 && first.Pixels()[1023]==1,"First FIFO packet");
    Check(Next(preparation,second)==Result::Ready && second.Request().resource==2,"Second FIFO packet");
    Check(preparation.Stats().liveBytes==2048 && calls==2,"Dequeue released consumer-held budget");
    Check(preparation.Take(empty,std::chrono::milliseconds(0))==Result::Pending,"Producer exceeded two retained packets");
    first.Reset();
    Check(Next(preparation,first)==Result::Ready && first.Request().resource==3,"Release did not wake blocked producer");
    second=std::move(first); // Releases the replaced reservation before taking ownership.
    Check(!first.Pixels() && second.Pixels()[0]==3,"Move assignment lost packet ownership");
    Check(Next(preparation,survivor)==Result::Ready && survivor.Request().resource==4,"Final FIFO packet");
    second.Reset();
    Check(Next(preparation,empty)==Result::Finished,"Successful stream never finished");
    const auto done=preparation.Stats();
    Check(done.peakBytes==2048 && done.peakPackets==2 && done.prepared==4 && done.delivered==4 && done.liveBytes==1024,"Final budget accounting");
    Check(done.workerThread && done.workerThread!=GetCurrentThreadId(),"Preparation ran on consumer thread");
  }
  Check(survivor.Pixels()[511]==4,"Packet lifetime depends on destroyed producer");survivor.Reset();
  // Byte bound and packet-count bound are independent.
  for(const auto limit:{std::size_t(1024),std::size_t(65536)}) {
    TexturePreparation preparation({Small(1),Small(2),Small(3)},limit,
      [](const TextureRequest&,std::uint8_t*,const std::atomic<bool>&){return S_OK;});
    Eventually([&]{return preparation.Stats().prepared==(limit==1024?1u:2u);},"Independent preparation bound");
    Check(preparation.Stats().peakBytes==(limit==1024?1024u:2048u),"Byte or packet limit exceeded");
    preparation.Cancel();PreparedTexture packet;Check(Next(preparation,packet)==Result::Cancelled,"Cancelled queue delivered pixels");
  }
}
void CancellationAndErrors() {
  const auto start=Clock::now();
  for(unsigned cycle=0;cycle<30;++cycle) {
    TexturePreparation preparation({Small(1),Small(2),Small(3)},2048,
      [](const TextureRequest&,std::uint8_t*,const std::atomic<bool>&){return S_OK;});
    Eventually([&]{return preparation.Stats().prepared==2;},"Cancellation backpressure setup");
    // Destruction must wake and join a worker parked on its byte/count budget.
  }
  Check(Clock::now()-start<std::chrono::seconds(10),"Bounded queue destruction stalled");
  std::atomic<bool> started{false},exited{false};
  {
    TexturePreparation preparation({Small(1)},1024,[&](const TextureRequest&,std::uint8_t*,const std::atomic<bool>& cancelled){
      started=true;while(!cancelled.load())Sleep(1);exited=true;return E_ABORT;
    });
    Eventually([&]{return started.load();},"In-flight cancellation setup");
    std::thread canceller([&]{preparation.Cancel();});canceller.join();
    PreparedTexture packet;Check(Next(preparation,packet)==Result::Cancelled && !packet.Pixels(),"Late result escaped cancellation");
  }
  Check(exited,"Worker was detached during destruction");
  PreparedTexture packet;
  for(unsigned mode=0;mode<2;++mode) {
    TexturePreparation failure({Small(27)},1024,[=](const TextureRequest&,std::uint8_t*,const std::atomic<bool>&)->HRESULT{
      if(mode)throw std::runtime_error("Injected decoder exception");return E_ACCESSDENIED;
    });
    Check(Next(failure,packet)==Result::Failed && failure.FailureCode()==(mode?E_FAIL:E_ACCESSDENIED),"Decoder failure was hidden");
    Check(failure.Error().find(L"27")!=std::wstring::npos && !packet.Pixels(),"Failure lost resource identity or published partial pixels");
  }
  for(auto invalid:{TextureRequest{},TextureRequest{resources::TextureKind::Surface,0,0,1,4097,1,false},Small(1)}) {
    TexturePreparation failure({invalid},512);
    Check(Next(failure,packet)==Result::Failed && failure.FailureCode()==E_INVALIDARG && !failure.Stats().workerThread,"Invalid manifest launched a worker");
  }
  TexturePreparation empty({},1024);Check(Next(empty,packet)==Result::Finished,"Empty preparation batch");
  TexturePreparation missing({{resources::TextureKind::Interface,0,0,65535,16,16,true}},1024);
  Check(Next(missing,packet)==Result::Failed && missing.FailureCode()==HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND),"Missing embedded resource was hidden");
  TexturePreparation badDimensions({{resources::TextureKind::Surface,0,0,320,16,16,false}},1024);
  Check(Next(badDimensions,packet)==Result::Failed && badDimensions.FailureCode()==E_INVALIDARG,"Surface manifest dimensions were ignored");
  TexturePreparation notImage({{resources::TextureKind::Interface,0,0,301,16,16,true}},1024);
  Check(Next(notImage,packet)==Result::Failed,"Non-image resource accepted");
  TexturePreparation foreign({},1024);Result foreignResult=Result::Ready;
  std::thread wrongConsumer([&]{PreparedTexture other;foreignResult=foreign.Take(other);});wrongConsumer.join();
  Check(foreignResult==Result::Failed && Next(foreign,packet)==Result::Finished,"Foreign consumer disturbed owner stream");
}
// Independent synchronous reference: the old main-thread decode/scaling path.
std::vector<std::uint8_t> Reference(const TextureRequest& request) {
  using Microsoft::WRL::ComPtr;
  const auto module=GetModuleHandleW(nullptr);const auto resource=FindResourceW(module,MAKEINTRESOURCEW(request.resource),RT_RCDATA);
  auto* bytes=static_cast<BYTE*>(LockResource(LoadResource(module,resource)));
  ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.GetAddressOf()))),"Reference factory");
  Check(SUCCEEDED(factory->CreateStream(stream.GetAddressOf())) && SUCCEEDED(stream->InitializeFromMemory(bytes,SizeofResource(module,resource))),"Reference resource stream");
  Check(SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,decoder.GetAddressOf())) && SUCCEEDED(decoder->GetFrame(0,frame.GetAddressOf())),"Reference decode");
  IWICBitmapSource* source=frame.Get();
  if(request.scale) {
    Check(SUCCEEDED(factory->CreateBitmapScaler(scaler.GetAddressOf())) && SUCCEEDED(scaler->Initialize(frame.Get(),request.width,request.height,WICBitmapInterpolationModeFant)),"Reference scaler");source=scaler.Get();
  }
  Check(SUCCEEDED(factory->CreateFormatConverter(converter.GetAddressOf())) && SUCCEEDED(converter->Initialize(source,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Reference conversion");
  std::vector<std::uint8_t> pixels(request.Bytes());
  Check(SUCCEEDED(converter->CopyPixels(nullptr,request.width*4,UINT(pixels.size()),pixels.data())),"Reference pixels");return pixels;
}
void NativePixels() {
  const auto apartment=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Check(SUCCEEDED(apartment),"Reference STA");
  struct ApartmentEnd {~ApartmentEnd(){CoUninitialize();}} apartmentEnd;
  const auto manifest=resources::TextureManifest();Check(manifest.size()==19,"Production manifest count");
  TexturePreparation preparation;PreparedTexture packet;std::size_t bytes=0;
  for(const auto& request:manifest) {
    Check(Next(preparation,packet)==Result::Ready && packet.Request()==request,"Production texture ordering");
    const auto reference=Reference(request);
    Check(!std::memcmp(reference.data(),packet.Pixels(),reference.size()),"Worker pixels differ from synchronous original");bytes+=reference.size();
  }
  packet.Reset();Check(Next(preparation,packet)==Result::Finished,"Production stream completion");
  const auto stats=preparation.Stats();
  Check(stats.liveBytes==0 && stats.livePackets==0 && stats.peakBytes<=TexturePreparation::defaultBudget && stats.peakPackets<=2,"Production staging leak/budget");
  std::cout<<"Pixel parity: "<<manifest.size()<<" textures, "<<bytes<<" bytes; peak "<<stats.peakBytes
    <<" bytes / "<<stats.peakPackets<<" packets; decode "<<stats.decodeSeconds<<" s\n";
}
void NativeCancellation() {
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),640,360,L"Texture cancellation",false),"Cancellation window");
  Renderer renderer;unsigned visits=0;
  Check(!renderer.Initialize(window.GetHandle(),640,360,nullptr,[&]{return ++visits<20;}),"Renderer ignored loading cancellation");
  Check(visits==20 && renderer.FailureCode()==HRESULT_FROM_WIN32(ERROR_CANCELLED) && !renderer.BeginFrame(),"Cancelled renderer became renderable");
  Check(renderer.TextureLoadingStats().liveBytes==0 && renderer.TextureLoadingStats().livePackets==0,"Cancelled renderer retained staging reservations");
  Renderer replacement;Check(replacement.Initialize(window.GetHandle(),640,360),"Replacement after interrupted initialization");
  const auto stats=replacement.TextureLoadingStats();
  Check(stats.prepared==19 && stats.delivered==19 && stats.liveBytes==0 && stats.workerThread!=GetCurrentThreadId(),"Native renderer bypassed resource worker");
  Check(replacement.BeginFrame() && replacement.EndFrame(false) && replacement.ValidateDebugMessages(),"Replacement frame failed");
  std::cout<<replacement.AdapterDiagnostics();
}
void NativeClose() {
  for(const bool explicitQuit:{false,true}) {
    platform::PumpMessages(2048,1);
    {
      Engine engine;
      Check(!ExecutionTestAccess::InterruptLoading(engine,explicitQuit),"Native loading close was ignored");
      Check(engine.LoadingCancelled() && engine.ExitCode()==(explicitQuit?31:0),"Native loading exit lost cancellation/quit code");
    }
    platform::PumpMessages(2048,1);
  }
}
}
int main()try {
  BudgetAndLifetime();CancellationAndErrors();NativePixels();NativeCancellation();NativeClose();
  std::cout<<"PASS: bounded CPU texture preparation, cancellation, ownership, errors, pixel parity and native upload\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
