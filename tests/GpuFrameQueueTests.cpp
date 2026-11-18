#include "engine/GpuFrameQueue.h"
#include <iostream>
#include <cstdlib>
#include <type_traits>

void Check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
  static_assert(!std::is_copy_constructible_v<GpuFrameQueue>);
  Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
  Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"Create queue-test device");
  D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=512;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
  desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> target,staging;Microsoft::WRL::ComPtr<ID3D11RenderTargetView> view;
  Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target))&&SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&view)),"Create queue-test target");
  desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)),"Create queue-test readback");
  GpuFrameQueue queue;Check(queue.Initialize(device.Get())&&queue.Available(),"Native GPU completion events unavailable");
  for(int frame=0;frame<180;++frame){
    Check(SUCCEEDED(queue.Begin()),"A queued frame must finish without polling");
    const float color[]={float(frame),.25f,.5f,1};context->ClearRenderTargetView(view.Get(),color);
    if(frame==179)context->CopyResource(staging.Get(),target.Get());
    Check(SUCCEEDED(queue.End()),"GPU frame event submission failed");
  }
  const double waited=queue.WaitMilliseconds();queue.Reset();
  D3D11_MAPPED_SUBRESOURCE mapped{};Check(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)),"Read drained GPU frame");
  const auto* pixel=static_cast<const float*>(mapped.pData);
  Check(pixel[0]==179&&pixel[1]==.25f&&pixel[2]==.5f&&pixel[3]==1,"Queue drain lost or reordered submitted work");context->Unmap(staging.Get(),0);
  Check(queue.Initialize(device.Get()),"Queue must support a fresh device lifecycle");
  Check(SUCCEEDED(queue.End())&&SUCCEEDED(queue.End()),"Initial two in-flight slots");
  Check(queue.End()==E_UNEXPECTED,"Pending events must not be overwritten by excess producer frames");
  Check(SUCCEEDED(queue.Begin())&&SUCCEEDED(queue.End()),"Queue must recover after producer backpressure");
  queue.Reset();Check(!queue.Initialize(nullptr)&&!queue.Available(),"Missing device must leave the queue unavailable");
  Check(SUCCEEDED(queue.Begin())&&SUCCEEDED(queue.End()),"Unavailable queue must be a no-op fallback");
  std::cout<<"PASS: 180 native GPU frames, event waits, ordered completion, drain/reinitialize, bounded producer and unavailable fallback; event wait ms "<<waited<<'\n';
}
