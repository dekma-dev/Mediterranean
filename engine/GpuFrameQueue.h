#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <array>

// Bound CPU render-ahead without polling GPU queries or letting Map/Present
// spend a core waiting in the driver. Two frames retain CPU/GPU overlap.
class GpuFrameQueue {
public:
  GpuFrameQueue()=default;
  GpuFrameQueue(const GpuFrameQueue&)=delete;
  GpuFrameQueue& operator=(const GpuFrameQueue&)=delete;
  ~GpuFrameQueue(){Reset();}
  bool Initialize(ID3D11Device* device){
    Reset();
    if(!device||FAILED(device->QueryInterface(IID_PPV_ARGS(device_.GetAddressOf()))))return false;
    for(auto& event:events_)if(!(event=CreateEventW(nullptr,FALSE,FALSE,nullptr))){Reset();return false;}
    return true;
  }
  HRESULT Begin(){
    if(!device_||!pending_[cursor_])return S_OK;
    LARGE_INTEGER start{},end{},frequency{};QueryPerformanceCounter(&start);
    // DXGI also signals completion on device removal. Keep the event alive
    // until then; closing a pending event on a timeout could race the driver.
    const DWORD result=WaitForSingleObject(events_[cursor_],INFINITE);
    QueryPerformanceCounter(&end);QueryPerformanceFrequency(&frequency);
    waitMilliseconds_+=double(end.QuadPart-start.QuadPart)*1000/double(frequency.QuadPart);
    if(result!=WAIT_OBJECT_0)return HRESULT_FROM_WIN32(result==WAIT_FAILED?GetLastError():ERROR_TIMEOUT);
    pending_[cursor_]=false;return S_OK;
  }
  HRESULT End(){
    if(!device_)return S_OK;
    if(pending_[cursor_])return E_UNEXPECTED;
    const HRESULT result=device_->EnqueueSetEvent(events_[cursor_]);
    if(SUCCEEDED(result)){pending_[cursor_]=true;cursor_=(cursor_+1)%events_.size();}
    return result;
  }
  bool Available()const{return device_!=nullptr;}
  double WaitMilliseconds()const{return waitMilliseconds_;}
  void Reset(){
    for(size_t i=0;i<events_.size();++i)if(events_[i]){
      if(pending_[i])WaitForSingleObject(events_[i],INFINITE);
      CloseHandle(events_[i]);events_[i]=nullptr;pending_[i]=false;
    }
    device_.Reset();cursor_=0;waitMilliseconds_=0;
  }
private:
  Microsoft::WRL::ComPtr<IDXGIDevice2> device_;
  std::array<HANDLE,2> events_{};
  std::array<bool,2> pending_{};
  size_t cursor_=0;
  double waitMilliseconds_=0;
};
