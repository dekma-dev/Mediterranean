#include "engine/TexturePreparation.h"
#include "engine/SurfaceStorage.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

namespace resources {
using Clock=std::chrono::steady_clock;
using Microsoft::WRL::ComPtr;
bool TextureRequest::operator==(const TextureRequest& o)const {
  return kind==o.kind && slot==o.slot && layer==o.layer && resource==o.resource &&
    width==o.width && height==o.height && scale==o.scale;
}
std::vector<TextureRequest> TextureManifest() {
  std::vector<TextureRequest> requests;
  for(unsigned group=0;group<2;++group)for(unsigned map=0;map<2;++map)
    for(unsigned layer=0;layer<visual::SurfaceLayers(group);++layer)
      requests.push_back({TextureKind::Surface,group*2+map,layer,320+visual::SurfaceSource(group,layer)*2+map,
        visual::SurfaceSize(group),visual::SurfaceSize(group),false});
  for(unsigned i=0;i<7;++i) {
    const bool effect=i==6;
    const unsigned width=i==5?512:i==1?1536:1024,height=effect?512:i==5?256:i==1?864:i==3?768:1024;
    requests.push_back({effect?TextureKind::Effect:TextureKind::Interface,effect?0:i,0,effect?206:i==5?207:201+i,width,height,true});
  }
  return requests;
}
struct TextureBudget {
  mutable std::mutex mutex;
  std::condition_variable changed;
  std::atomic<bool> cancelled{false};
  TexturePreparationStats stats;
  std::size_t limit=0;
};
PreparedTexture::~PreparedTexture(){Reset();}
PreparedTexture::PreparedTexture(PreparedTexture&& o)noexcept
  :request_(o.request_),pixels_(std::move(o.pixels_)),budget_(std::move(o.budget_)){}
PreparedTexture& PreparedTexture::operator=(PreparedTexture&& o)noexcept {
  if(this!=&o){Reset();request_=o.request_;pixels_=std::move(o.pixels_);budget_=std::move(o.budget_);}
  return *this;
}
void PreparedTexture::Reset()noexcept {
  // Free actual storage BEFORE making its budget available to the worker.
  pixels_.reset();
  if(auto budget=std::move(budget_)) {
    {std::lock_guard<std::mutex> lock(budget->mutex);budget->stats.liveBytes-=request_.Bytes();--budget->stats.livePackets;}
    budget->changed.notify_all();
  }
}
namespace {
double Seconds(Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();}
struct Apartment {
  HRESULT result=CoInitializeEx(nullptr,COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);
  ~Apartment(){if(SUCCEEDED(result))CoUninitialize();}
};
// The factory and every WIC object are born and destroyed on the worker.
HRESULT Decode(IWICImagingFactory* factory,const TextureRequest& request,std::uint8_t* pixels,const std::atomic<bool>& cancelled) {
  if(cancelled.load())return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  const auto module=GetModuleHandleW(nullptr);
  const auto resource=FindResourceW(module,MAKEINTRESOURCEW(request.resource),RT_RCDATA);
  if(!resource)return HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
  const auto count=SizeofResource(module,resource);
  if(!count || count>16u*1024u*1024u)return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
  const auto loaded=LoadResource(module,resource);
  auto* bytes=loaded?static_cast<BYTE*>(LockResource(loaded)):nullptr;
  if(!bytes)return HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
  ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;
  ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
  HRESULT hr=factory->CreateStream(stream.GetAddressOf());if(FAILED(hr))return hr;
  hr=stream->InitializeFromMemory(bytes,count);if(FAILED(hr))return hr;
  hr=factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,decoder.GetAddressOf());if(FAILED(hr))return hr;
  hr=decoder->GetFrame(0,frame.GetAddressOf());if(FAILED(hr))return hr;
  UINT width=0,height=0;hr=frame->GetSize(&width,&height);if(FAILED(hr))return hr;
  // Reject oversized source images before WIC allocates a decoded frame.
  if(!width || !height || width>4096 || height>4096)return E_INVALIDARG;
  if(!request.scale && (width!=request.width || height!=request.height))return E_INVALIDARG;
  IWICBitmapSource* source=frame.Get();
  if(request.scale) {
    hr=factory->CreateBitmapScaler(scaler.GetAddressOf());if(FAILED(hr))return hr;
    hr=scaler->Initialize(frame.Get(),request.width,request.height,WICBitmapInterpolationModeFant);if(FAILED(hr))return hr;
    source=scaler.Get();
  }
  hr=factory->CreateFormatConverter(converter.GetAddressOf());if(FAILED(hr))return hr;
  hr=converter->Initialize(source,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);if(FAILED(hr))return hr;
  if(cancelled.load())return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  return converter->CopyPixels(nullptr,request.width*4,UINT(request.Bytes()),pixels);
}
}
struct TexturePreparation::Impl {
  std::shared_ptr<TextureBudget> budget=std::make_shared<TextureBudget>();
  std::vector<TextureRequest> requests;
  Decoder decoder;
  std::deque<PreparedTexture> ready;
  std::thread worker;
  DWORD owner=GetCurrentThreadId();
  bool done=false;
  HRESULT failure=S_OK;
  unsigned failedResource=0;
  void Fail(HRESULT hr,unsigned resource) {
    {std::lock_guard<std::mutex> lock(budget->mutex);failure=hr;failedResource=resource;done=true;}
    budget->changed.notify_all();
  }
  void Run()noexcept {
    unsigned resource=0;
    try {
      Apartment apartment;
      if(FAILED(apartment.result)){Fail(apartment.result,0);return;}
      ComPtr<IWICImagingFactory> factory;
      if(!decoder) {
        const auto hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.GetAddressOf()));
        if(FAILED(hr)){Fail(hr,0);return;}
      }
      {std::lock_guard<std::mutex> lock(budget->mutex);budget->stats.workerThread=GetCurrentThreadId();}
      for(const auto& request:requests) {
        resource=request.resource;
        PreparedTexture packet;packet.request_=request;
        {
          const auto start=Clock::now();std::unique_lock<std::mutex> lock(budget->mutex);
          budget->changed.wait(lock,[&]{return budget->cancelled.load() ||
            (budget->stats.livePackets<2 && request.Bytes()<=budget->limit-budget->stats.liveBytes);});
          budget->stats.producerWaitSeconds+=Seconds(start);
          if(budget->cancelled.load())break;
          budget->stats.liveBytes+=request.Bytes();++budget->stats.livePackets;
          budget->stats.peakBytes=std::max(budget->stats.peakBytes,budget->stats.liveBytes);
          budget->stats.peakPackets=std::max(budget->stats.peakPackets,budget->stats.livePackets);
          packet.budget_=budget;
        }
        packet.pixels_=std::make_unique<std::uint8_t[]>(request.Bytes());
        const auto start=Clock::now();
        const HRESULT hr=decoder?decoder(request,packet.pixels_.get(),budget->cancelled):Decode(factory.Get(),request,packet.pixels_.get(),budget->cancelled);
        {std::lock_guard<std::mutex> lock(budget->mutex);budget->stats.decodeSeconds+=Seconds(start);}
        if(budget->cancelled.load())break;
        if(FAILED(hr)){Fail(hr,resource);return;}
        {
          std::lock_guard<std::mutex> lock(budget->mutex);
          if(budget->cancelled.load())break;
          ready.push_back(std::move(packet));++budget->stats.prepared;
        }
        budget->changed.notify_all();
      }
      {std::lock_guard<std::mutex> lock(budget->mutex);done=true;}
      budget->changed.notify_all();
    }catch(const std::bad_alloc&){Fail(E_OUTOFMEMORY,resource);}
    catch(...){Fail(E_FAIL,resource);}
  }
};
TexturePreparation::TexturePreparation(std::vector<TextureRequest> requests,std::size_t byteBudget,Decoder decoder)
  :impl_(std::make_unique<Impl>()) {
  impl_->budget->limit=byteBudget;impl_->requests=std::move(requests);impl_->decoder=std::move(decoder);
  bool valid=byteBudget>0 && byteBudget<=64u*1024u*1024u && impl_->requests.size()<=64;
  for(const auto& request:impl_->requests) {
    if(!request.width || !request.height || request.width>4096 || request.height>4096 || request.resource>65535 || request.Bytes()>byteBudget) {
      impl_->Fail(E_INVALIDARG,request.resource);return;
    }
  }
  if(!valid){impl_->Fail(E_INVALIDARG,0);return;}
  try{impl_->worker=std::thread([this]{impl_->Run();});}
  catch(...){impl_->Fail(E_OUTOFMEMORY,0);}
}
TexturePreparation::~TexturePreparation(){Stop();}
void TexturePreparation::Stop() {
  Cancel();if(impl_->worker.joinable())impl_->worker.join();
  // No producers remain; queued reservations are released without holding the lock.
  impl_->ready.clear();
}
void TexturePreparation::Cancel()noexcept {impl_->budget->cancelled.store(true);impl_->budget->changed.notify_all();}
TexturePreparation::Result TexturePreparation::Take(PreparedTexture& output,std::chrono::milliseconds wait) {
  if(GetCurrentThreadId()!=impl_->owner)return Result::Failed;
  output.Reset();
  const auto start=Clock::now();std::unique_lock<std::mutex> lock(impl_->budget->mutex);
  impl_->budget->changed.wait_for(lock,wait,[&]{return impl_->budget->cancelled.load() || impl_->done || !impl_->ready.empty();});
  impl_->budget->stats.consumerWaitSeconds+=Seconds(start);
  if(impl_->budget->cancelled.load())return Result::Cancelled;
  if(FAILED(impl_->failure))return Result::Failed;
  if(!impl_->ready.empty()) {
    output=std::move(impl_->ready.front());impl_->ready.pop_front();++impl_->budget->stats.delivered;return Result::Ready;
  }
  return impl_->done?Result::Finished:Result::Pending;
}
TexturePreparationStats TexturePreparation::Stats()const {std::lock_guard<std::mutex> lock(impl_->budget->mutex);return impl_->budget->stats;}
HRESULT TexturePreparation::FailureCode()const {std::lock_guard<std::mutex> lock(impl_->budget->mutex);return impl_->failure;}
std::wstring TexturePreparation::Error()const {
  std::lock_guard<std::mutex> lock(impl_->budget->mutex);std::wostringstream out;
  out<<L"Texture preparation resource "<<impl_->failedResource<<L" failed (0x"<<std::hex<<static_cast<unsigned long>(impl_->failure)<<L").";
  return out.str();
}
}
