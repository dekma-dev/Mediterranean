#include "engine/RenderBindings.h"
#include <d3d11sdklayers.h>
#include <iostream>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;
using Stage=RenderBindings::Stage;
namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Native {
  ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
  ComPtr<ID3D11InfoQueue> debug;RenderBindings bindings;
  Native(){
    HRESULT hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_DEBUG,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context);
    if(hr==DXGI_ERROR_SDK_COMPONENT_MISSING)hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context);
    Check(SUCCEEDED(hr),"Native device");device.As(&debug);Check(SUCCEEDED(bindings.Attach(context.Get())),"Attach binding owner");
  }
  ~Native(){bindings.Clear();context->ClearState();context->Flush();}
  void Clean(){
    if(!debug)return;
    for(UINT64 i=0;i<debug->GetNumStoredMessagesAllowedByRetrievalFilter();++i){
      SIZE_T bytes=0;debug->GetMessage(i,nullptr,&bytes);std::vector<unsigned char> storage(bytes);
      auto* message=reinterpret_cast<D3D11_MESSAGE*>(storage.data());
      Check(SUCCEEDED(debug->GetMessage(i,message,&bytes)),"Read native validation message");
      if(message->Severity<=D3D11_MESSAGE_SEVERITY_WARNING)throw std::runtime_error(message->pDescription);
    }
  }
  ComPtr<ID3D11ShaderResourceView> Input(Stage stage,UINT slot){
    ComPtr<ID3D11ShaderResourceView> view;
    if(stage==Stage::Vertex)context->VSGetShaderResources(slot,1,&view);else context->PSGetShaderResources(slot,1,&view);return view;
  }
};
struct Color {
  ComPtr<ID3D11Texture2D> texture;
  Color(ID3D11Device* device,UINT width=64,UINT slices=2){
    D3D11_TEXTURE2D_DESC t{};t.Width=t.Height=width;t.MipLevels=3;t.ArraySize=slices;t.Format=DXGI_FORMAT_R8G8B8A8_UNORM;t.SampleDesc.Count=1;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    Check(SUCCEEDED(device->CreateTexture2D(&t,nullptr,&texture)),"Create alias texture");
  }
  ComPtr<ID3D11ShaderResourceView> Read(ID3D11Device* device,UINT mip,UINT slice,UINT mips=1,UINT slices=1){
    D3D11_SHADER_RESOURCE_VIEW_DESC d{};d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;d.Texture2DArray={mip,mips,slice,slices};
    ComPtr<ID3D11ShaderResourceView> view;Check(SUCCEEDED(device->CreateShaderResourceView(texture.Get(),&d,&view)),"Create input alias");return view;
  }
  ComPtr<ID3D11RenderTargetView> Write(ID3D11Device* device,UINT mip,UINT slice){
    D3D11_RENDER_TARGET_VIEW_DESC d{};d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2DARRAY;d.Texture2DArray={mip,slice,1};
    ComPtr<ID3D11RenderTargetView> view;Check(SUCCEEDED(device->CreateRenderTargetView(texture.Get(),&d,&view)),"Create output alias");return view;
  }
};
void AliasesAndTransactions(Native& n){
  n.bindings.Clear();n.bindings.ResetStatistics();Color texture(n.device.Get()),other(n.device.Get(),128);
  auto a=texture.Read(n.device.Get(),0,0),alias=texture.Read(n.device.Get(),0,0),mip=texture.Read(n.device.Get(),1,0),slice=texture.Read(n.device.Get(),0,1),all=texture.Read(n.device.Get(),0,0,UINT(-1),2);
  auto target=texture.Write(n.device.Get(),0,0),otherMip=texture.Write(n.device.Get(),1,0),badSize=other.Write(n.device.Get(),0,0);
  Check(SUCCEEDED(n.bindings.Resources(Stage::Vertex,2,1,a.GetAddressOf())) && SUCCEEDED(n.bindings.Resources(Stage::Pixel,3,1,alias.GetAddressOf())),"Bind same resource through different views/stages");
  Check(SUCCEEDED(n.bindings.Resources(Stage::Pixel,4,1,mip.GetAddressOf())) && SUCCEEDED(n.bindings.Resources(Stage::Pixel,5,1,slice.GetAddressOf())),"Bind disjoint subresources");
  Check(SUCCEEDED(n.bindings.Resources(Stage::Pixel,127,1,all.GetAddressOf())),"Last valid SRV slot");
  Check(SUCCEEDED(n.bindings.Targets(1,target.GetAddressOf(),nullptr)),"Transition read resource to output");
  Check(!n.Input(Stage::Vertex,2) && !n.Input(Stage::Pixel,3) && !n.Input(Stage::Pixel,127),"All overlapping aliases must be explicitly unbound");
  Check(n.Input(Stage::Pixel,4)==mip && n.Input(Stage::Pixel,5)==slice,"Disjoint mip/slice was needlessly unbound");
  Check(n.bindings.Stats().hazardUnbinds==3,"Alias transition accounting");
  const auto calls=n.bindings.Stats().calls;
  for(int i=0;i<20;++i){Check(SUCCEEDED(n.bindings.Targets(1,target.GetAddressOf(),nullptr)),"Cached target");Check(SUCCEEDED(n.bindings.Resources(Stage::Pixel,4,1,mip.GetAddressOf())),"Cached input");}
  Check(n.bindings.Stats().calls==calls && n.bindings.Stats().skipped>=40,"Redundant bindings reached the driver");
  ID3D11ShaderResourceView* batch[]={slice.Get(),a.Get()};
  Check(n.bindings.Resources(Stage::Pixel,8,2,batch)==DXGI_ERROR_INVALID_CALL && !n.Input(Stage::Pixel,8) && !n.Input(Stage::Pixel,9),"Failed input batch changed part of the state");
  ID3D11RenderTargetView* duplicates[]={target.Get(),target.Get()};
  Check(FAILED(n.bindings.Targets(2,duplicates,nullptr)),"Duplicate output subresource accepted");
  ID3D11RenderTargetView* mismatched[]={target.Get(),badSize.Get()};
  Check(FAILED(n.bindings.Targets(2,mismatched,nullptr)),"Mismatched output dimensions accepted");
  ComPtr<ID3D11RenderTargetView> actual;n.context->OMGetRenderTargets(1,&actual,nullptr);Check(actual==target,"Failed target batch changed current output");
  Check(SUCCEEDED(n.bindings.Targets(1,otherMip.GetAddressOf(),nullptr)) && !n.Input(Stage::Pixel,4) && n.Input(Stage::Pixel,5)==slice,"Second transition retained stale alias state");
  Check(SUCCEEDED(n.bindings.Resources(Stage::Pixel,3,1,a.GetAddressOf())),"Former output could not become input");
  Check(n.bindings.Resources(Stage::Pixel,128,1,nullptr)==E_INVALIDARG && n.bindings.Resources(Stage(9),0,1,nullptr)==E_INVALIDARG,"Invalid input ranges/stage");
  n.Clean();
}
void DepthAspects(Native& n){
  n.bindings.Clear();D3D11_TEXTURE2D_DESC d{};d.Width=d.Height=64;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R24G8_TYPELESS;d.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
  ComPtr<ID3D11Texture2D> texture;Check(SUCCEEDED(n.device->CreateTexture2D(&d,nullptr,&texture)),"Depth resource");
  ComPtr<ID3D11DepthStencilView> depthWrite,depthRead,allRead;
  D3D11_DEPTH_STENCIL_VIEW_DESC dv{};dv.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dv.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;
  Check(SUCCEEDED(n.device->CreateDepthStencilView(texture.Get(),&dv,&depthWrite)),"Writable depth");dv.Flags=D3D11_DSV_READ_ONLY_DEPTH;
  Check(SUCCEEDED(n.device->CreateDepthStencilView(texture.Get(),&dv,&depthRead)),"Read-only depth with writable stencil");dv.Flags|=D3D11_DSV_READ_ONLY_STENCIL;
  Check(SUCCEEDED(n.device->CreateDepthStencilView(texture.Get(),&dv,&allRead)),"Read-only depth/stencil");
  D3D11_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;sv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
  ComPtr<ID3D11ShaderResourceView> depth,stencil;
  Check(SUCCEEDED(n.device->CreateShaderResourceView(texture.Get(),&sv,&depth)),"Depth input");sv.Format=DXGI_FORMAT_X24_TYPELESS_G8_UINT;
  Check(SUCCEEDED(n.device->CreateShaderResourceView(texture.Get(),&sv,&stencil)),"Stencil input");
  Check(SUCCEEDED(n.bindings.Targets(0,nullptr,depthRead.Get())) && SUCCEEDED(n.bindings.Resources(Stage::Pixel,15,1,depth.GetAddressOf())),"Depth-read DSV must allow depth sampling");
  Check(FAILED(n.bindings.Resources(Stage::Pixel,16,1,stencil.GetAddressOf())) && !n.Input(Stage::Pixel,16),"Writable stencil was sampled");
  Check(SUCCEEDED(n.bindings.Targets(0,nullptr,allRead.Get())) && SUCCEEDED(n.bindings.Resources(Stage::Pixel,16,1,stencil.GetAddressOf())),"Read-only stencil was blocked");
  Check(SUCCEEDED(n.bindings.Targets(0,nullptr,depthRead.Get())) && n.Input(Stage::Pixel,15)==depth && !n.Input(Stage::Pixel,16),"Stencil-only hazard did not preserve depth input");
  Check(SUCCEEDED(n.bindings.Targets(0,nullptr,depthWrite.Get())) && !n.Input(Stage::Pixel,15),"Writable depth transition retained sampling");n.Clean();
}
void SamplersAndResize(Native& n){
  n.bindings.Clear();D3D11_SAMPLER_DESC d{};d.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;d.AddressU=d.AddressV=d.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;d.MaxLOD=D3D11_FLOAT32_MAX;d.ComparisonFunc=D3D11_COMPARISON_NEVER;
  ComPtr<ID3D11SamplerState> sampler;Check(SUCCEEDED(n.device->CreateSamplerState(&d,&sampler)),"Sampler state");
  Check(SUCCEEDED(n.bindings.Samplers(Stage::Vertex,15,1,sampler.GetAddressOf())),"Sampler last valid slot");const auto calls=n.bindings.Stats().calls;
  Check(SUCCEEDED(n.bindings.Samplers(Stage::Vertex,15,1,sampler.GetAddressOf())) && n.bindings.Stats().calls==calls,"Repeated sampler call not elided");
  Check(n.bindings.Samplers(Stage::Pixel,16,1,nullptr)==E_INVALIDARG,"Sampler overflow");
  n.bindings.Clear();ComPtr<ID3D11SamplerState> actual;n.context->VSGetSamplers(15,1,&actual);Check(!actual,"Clear retained sampler");
  const auto window=CreateWindowExW(0,L"STATIC",L"Binding resize test",WS_OVERLAPPEDWINDOW,0,0,128,128,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);Check(window!=nullptr,"Hidden resize window");
  ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<IDXGIFactory> factory;
  Check(SUCCEEDED(n.device.As(&dxgi)) && SUCCEEDED(dxgi->GetAdapter(&adapter)) && SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(&factory))),"Swap chain factory");
  DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=2;desc.BufferDesc.Width=desc.BufferDesc.Height=64;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=window;desc.SampleDesc.Count=1;desc.Windowed=TRUE;
  ComPtr<IDXGISwapChain> chain;Check(SUCCEEDED(factory->CreateSwapChain(n.device.Get(),&desc,&chain)),"Binding test swap chain");
  ComPtr<ID3D11Texture2D> back;ComPtr<ID3D11RenderTargetView> target;
  Check(SUCCEEDED(chain->GetBuffer(0,IID_PPV_ARGS(&back))) && SUCCEEDED(n.device->CreateRenderTargetView(back.Get(),nullptr,&target)),"Swap target");
  Check(SUCCEEDED(n.bindings.Targets(1,target.GetAddressOf(),nullptr)),"Bind swap target");target.Reset();back.Reset();n.bindings.Clear();
  Check(SUCCEEDED(chain->ResizeBuffers(0,96,80,DXGI_FORMAT_UNKNOWN,0)),"Binding owner retained back-buffer reference across resize");
  chain.Reset();DestroyWindow(window);n.Clean();
  Check(SUCCEEDED(n.bindings.Attach(n.context.Get())) && !n.Input(Stage::Pixel,15),"Reattach retained stale cache");
}
void DeviceOwnership(Native& n){
  n.bindings.Clear();Native foreign;
  Color localColor(n.device.Get()),foreignColor(foreign.device.Get());
  auto localView=localColor.Read(n.device.Get(),0,0),foreignView=foreignColor.Read(foreign.device.Get(),0,0);
  auto foreignTarget=foreignColor.Write(foreign.device.Get(),0,0);
  Check(SUCCEEDED(n.bindings.Resources(Stage::Pixel,0,1,localView.GetAddressOf())),"Local device input");
  ID3D11ShaderResourceView* mixed[]={nullptr,foreignView.Get()};
  Check(n.bindings.Resources(Stage::Pixel,0,2,mixed)==E_INVALIDARG && n.Input(Stage::Pixel,0)==localView && !n.Input(Stage::Pixel,1),"Foreign input batch partially changed native state");
  Check(n.bindings.Targets(1,foreignTarget.GetAddressOf(),nullptr)==E_INVALIDARG && n.Input(Stage::Pixel,0)==localView,"Foreign output changed local bindings");

  D3D11_TEXTURE2D_DESC td{};td.Width=td.Height=64;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_D32_FLOAT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;
  ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11DepthStencilView> depth;
  Check(SUCCEEDED(foreign.device->CreateTexture2D(&td,nullptr,&texture)) && SUCCEEDED(foreign.device->CreateDepthStencilView(texture.Get(),nullptr,&depth)),"Foreign depth fixture");
  Check(n.bindings.Targets(0,nullptr,depth.Get())==E_INVALIDARG,"Foreign depth output accepted");
  ComPtr<ID3D11RenderTargetView> actualTarget;ComPtr<ID3D11DepthStencilView> actualDepth;
  n.context->OMGetRenderTargets(1,&actualTarget,&actualDepth);Check(!actualTarget && !actualDepth,"Foreign output reached native context");

  D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;sd.ComparisonFunc=D3D11_COMPARISON_NEVER;
  ComPtr<ID3D11SamplerState> localSampler,foreignSampler;
  Check(SUCCEEDED(n.device->CreateSamplerState(&sd,&localSampler)) && SUCCEEDED(foreign.device->CreateSamplerState(&sd,&foreignSampler)),"Device sampler fixtures");
  Check(SUCCEEDED(n.bindings.Samplers(Stage::Pixel,0,1,localSampler.GetAddressOf())),"Local device sampler");
  ID3D11SamplerState* mixedSamplers[]={nullptr,foreignSampler.Get()};
  Check(n.bindings.Samplers(Stage::Pixel,0,2,mixedSamplers)==E_INVALIDARG,"Foreign sampler accepted");
  ComPtr<ID3D11SamplerState> actualSampler;n.context->PSGetSamplers(0,1,&actualSampler);Check(actualSampler==localSampler,"Foreign sampler batch partially changed state");
  ComPtr<ID3D11DeviceContext> deferred;Check(SUCCEEDED(n.device->CreateDeferredContext(0,&deferred)),"Deferred context fixture");
  Check(n.bindings.Attach(deferred.Get())==E_INVALIDARG && n.Input(Stage::Pixel,0)==localView,"Invalid attachment discarded existing owner");

  Check(SUCCEEDED(n.bindings.Attach(foreign.context.Get())) && !n.Input(Stage::Pixel,0),"Owner replacement retained previous context bindings");
  Check(SUCCEEDED(n.bindings.Resources(Stage::Pixel,0,1,foreignView.GetAddressOf())),"Owner replacement rejected new device");
  Check(n.bindings.Resources(Stage::Pixel,0,1,localView.GetAddressOf())==E_INVALIDARG && foreign.Input(Stage::Pixel,0)==foreignView,"Stale device accepted after reconstruction");
  Check(SUCCEEDED(n.bindings.Attach(n.context.Get())) && !foreign.Input(Stage::Pixel,0),"Reattachment did not clear replaced context");
  n.Clean();foreign.Clean();
}
}
int main()try {
  Native native;AliasesAndTransactions(native);DepthAspects(native);SamplersAndResize(native);DeviceOwnership(native);
  std::cout<<"PASS: native alias/mip/slice transitions, atomic rejection, depth/stencil aspects, redundant-call suppression, real swap-chain resize and device ownership; debug layer "<<(native.debug?"active":"unavailable")<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
