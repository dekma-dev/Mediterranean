#include "engine/RenderBindings.h"
#include <algorithm>
#include <utility>

namespace {
constexpr UINT colorAspect=1,depthAspect=2,stencilAspect=4;
bool StageValid(RenderBindings::Stage stage){return stage==RenderBindings::Stage::Vertex || stage==RenderBindings::Stage::Pixel;}
HRESULT Texture(ID3D11Resource* resource,D3D11_TEXTURE2D_DESC& desc) {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  if(FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture))))return E_NOTIMPL;
  texture->GetDesc(&desc);return S_OK;
}
UINT ReadAspects(DXGI_FORMAT resource,DXGI_FORMAT view) {
  if(resource==DXGI_FORMAT_R24G8_TYPELESS || resource==DXGI_FORMAT_R32G8X24_TYPELESS)
    return view==DXGI_FORMAT_X24_TYPELESS_G8_UINT || view==DXGI_FORMAT_X32_TYPELESS_G8X24_UINT?stencilAspect:depthAspect;
  if(resource==DXGI_FORMAT_R32_TYPELESS || resource==DXGI_FORMAT_R16_TYPELESS)return depthAspect;
  return colorAspect;
}
}
HRESULT RenderBindings::Attach(ID3D11DeviceContext* context) {
  if(!context || context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return E_INVALIDARG;
  Clear();context_=context;context_->GetDevice(device_.ReleaseAndGetAddressOf());Clear();statistics_={};return S_OK;
}
bool RenderBindings::Owns(ID3D11DeviceChild* child) const {
  if(!child)return true;
  Microsoft::WRL::ComPtr<ID3D11Device> owner;child->GetDevice(&owner);return owner==device_;
}
void RenderBindings::SetResources(unsigned stage,UINT first,UINT count,ID3D11ShaderResourceView* const* views) {
  if(stage==0)context_->VSSetShaderResources(first,count,views);else context_->PSSetShaderResources(first,count,views);
  ++statistics_.calls;
}
void RenderBindings::Clear() {
  if(context_) {
    std::array<ID3D11ShaderResourceView*,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> empty{};
    SetResources(0,0,UINT(empty.size()),empty.data());SetResources(1,0,UINT(empty.size()),empty.data());
    std::array<ID3D11SamplerState*,D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> samplers{};
    context_->VSSetSamplers(0,UINT(samplers.size()),samplers.data());context_->PSSetSamplers(0,UINT(samplers.size()),samplers.data());
    context_->OMSetRenderTargets(0,nullptr,nullptr);statistics_.calls+=3;
  }
  inputs_={};samplers_={};targets_={};targetRanges_={};depth_=nullptr;depthRange_={};
}
bool RenderBindings::Overlap(const Range& a,const Range& b) {
  return a.resource && a.resource==b.resource && (a.aspects&b.aspects) &&
    a.mip<b.mip+b.mips && b.mip<a.mip+a.mips && a.slice<b.slice+b.slices && b.slice<a.slice+a.slices;
}
HRESULT RenderBindings::Describe(ID3D11ShaderResourceView* view,Range& r) {
  if(!view)return S_OK;
  view->GetResource(&r.resource);D3D11_SHADER_RESOURCE_VIEW_DESC v{};view->GetDesc(&v);
  if(v.ViewDimension==D3D11_SRV_DIMENSION_BUFFER || v.ViewDimension==D3D11_SRV_DIMENSION_BUFFEREX)return S_OK;
  D3D11_TEXTURE2D_DESC t{};if(FAILED(Texture(r.resource.Get(),t)))return E_NOTIMPL;
  switch(v.ViewDimension) {
    case D3D11_SRV_DIMENSION_TEXTURE2D:r.mip=v.Texture2D.MostDetailedMip;r.mips=v.Texture2D.MipLevels;break;
    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:r.mip=v.Texture2DArray.MostDetailedMip;r.mips=v.Texture2DArray.MipLevels;r.slice=v.Texture2DArray.FirstArraySlice;r.slices=v.Texture2DArray.ArraySize;break;
    case D3D11_SRV_DIMENSION_TEXTURE2DMS:break;
    case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:r.slice=v.Texture2DMSArray.FirstArraySlice;r.slices=v.Texture2DMSArray.ArraySize;break;
    case D3D11_SRV_DIMENSION_TEXTURECUBE:r.mip=v.TextureCube.MostDetailedMip;r.mips=v.TextureCube.MipLevels;r.slices=6;break;
    case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:r.mip=v.TextureCubeArray.MostDetailedMip;r.mips=v.TextureCubeArray.MipLevels;r.slice=v.TextureCubeArray.First2DArrayFace;r.slices=v.TextureCubeArray.NumCubes*6;break;
    default:return E_NOTIMPL;
  }
  r.mips=std::min(r.mips,t.MipLevels-r.mip);r.aspects=ReadAspects(t.Format,v.Format);return S_OK;
}
HRESULT RenderBindings::Describe(ID3D11RenderTargetView* view,Range& r) {
  if(!view)return S_OK;
  view->GetResource(&r.resource);D3D11_RENDER_TARGET_VIEW_DESC v{};view->GetDesc(&v);
  D3D11_TEXTURE2D_DESC t{};if(FAILED(Texture(r.resource.Get(),t)))return E_NOTIMPL;
  switch(v.ViewDimension) {
    case D3D11_RTV_DIMENSION_TEXTURE2D:r.mip=v.Texture2D.MipSlice;break;
    case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:r.mip=v.Texture2DArray.MipSlice;r.slice=v.Texture2DArray.FirstArraySlice;r.slices=v.Texture2DArray.ArraySize;break;
    case D3D11_RTV_DIMENSION_TEXTURE2DMS:break;
    case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:r.slice=v.Texture2DMSArray.FirstArraySlice;r.slices=v.Texture2DMSArray.ArraySize;break;
    default:return E_NOTIMPL;
  }
  r.width=std::max(1u,t.Width>>r.mip);r.height=std::max(1u,t.Height>>r.mip);r.samples=t.SampleDesc.Count;r.quality=t.SampleDesc.Quality;return S_OK;
}
HRESULT RenderBindings::Describe(ID3D11DepthStencilView* view,Range& r) {
  if(!view)return S_OK;
  view->GetResource(&r.resource);D3D11_DEPTH_STENCIL_VIEW_DESC v{};view->GetDesc(&v);
  D3D11_TEXTURE2D_DESC t{};if(FAILED(Texture(r.resource.Get(),t)))return E_NOTIMPL;
  switch(v.ViewDimension) {
    case D3D11_DSV_DIMENSION_TEXTURE2D:r.mip=v.Texture2D.MipSlice;break;
    case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:r.mip=v.Texture2DArray.MipSlice;r.slice=v.Texture2DArray.FirstArraySlice;r.slices=v.Texture2DArray.ArraySize;break;
    case D3D11_DSV_DIMENSION_TEXTURE2DMS:break;
    case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:r.slice=v.Texture2DMSArray.FirstArraySlice;r.slices=v.Texture2DMSArray.ArraySize;break;
    default:return E_NOTIMPL;
  }
  const bool stencil=v.Format==DXGI_FORMAT_D24_UNORM_S8_UINT || v.Format==DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
  r.aspects=(v.Flags&D3D11_DSV_READ_ONLY_DEPTH?0:depthAspect)|(stencil && !(v.Flags&D3D11_DSV_READ_ONLY_STENCIL)?stencilAspect:0);
  r.width=std::max(1u,t.Width>>r.mip);r.height=std::max(1u,t.Height>>r.mip);r.samples=t.SampleDesc.Count;r.quality=t.SampleDesc.Quality;return S_OK;
}
HRESULT RenderBindings::Resources(Stage stage,UINT first,UINT count,ID3D11ShaderResourceView* const* views) {
  ++statistics_.requests;
  if(!context_ || !StageValid(stage) || first>inputs_[0].size() || count>inputs_[0].size()-first)return Reject(E_INVALIDARG);
  auto& inputs=inputs_[unsigned(stage)];
  UINT begin=count,end=0;
  for(UINT i=0;i<count;++i)if(inputs[first+i].view!=(views?views[i]:nullptr)){begin=std::min(begin,i);end=i+1;}
  if(begin==count){++statistics_.skipped;return S_OK;}
  std::array<Range,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> proposed{};
  for(UINT i=begin;i<end;++i) {
    auto* view=views?views[i]:nullptr;if(view==inputs[first+i].view)continue;
    if(!Owns(view))return Reject(E_INVALIDARG);
    const auto hr=Describe(view,proposed[i]);if(FAILED(hr))return Reject(hr);
    if(Overlap(proposed[i],depthRange_))return Reject(DXGI_ERROR_INVALID_CALL);
    for(const auto& target:targetRanges_)if(Overlap(proposed[i],target))return Reject(DXGI_ERROR_INVALID_CALL);
  }
  std::array<ID3D11ShaderResourceView*,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> actual{};
  for(UINT i=begin;i<end;++i) {
    auto* view=views?views[i]:nullptr;
    if(view!=inputs[first+i].view)inputs[first+i]={view,std::move(proposed[i])};
    actual[i]=view;
  }
  SetResources(unsigned(stage),first+begin,end-begin,actual.data()+begin);return S_OK;
}
HRESULT RenderBindings::Samplers(Stage stage,UINT first,UINT count,ID3D11SamplerState* const* states) {
  ++statistics_.requests;
  if(!context_ || !StageValid(stage) || first>samplers_[0].size() || count>samplers_[0].size()-first)return Reject(E_INVALIDARG);
  auto& current=samplers_[unsigned(stage)];UINT begin=count,end=0;
  for(UINT i=0;i<count;++i)if(current[first+i]!=(states?states[i]:nullptr)){
    if(!Owns(states?states[i]:nullptr))return Reject(E_INVALIDARG);
    begin=std::min(begin,i);end=i+1;
  }
  if(begin==count){++statistics_.skipped;return S_OK;}
  for(UINT i=begin;i<end;++i)current[first+i]=states?states[i]:nullptr;
  if(stage==Stage::Vertex)context_->VSSetSamplers(first+begin,end-begin,current.data()+first+begin);
  else context_->PSSetSamplers(first+begin,end-begin,current.data()+first+begin);
  ++statistics_.calls;return S_OK;
}
HRESULT RenderBindings::Targets(UINT count,ID3D11RenderTargetView* const* targets,ID3D11DepthStencilView* depth) {
  ++statistics_.requests;
  if(!context_ || count>targets_.size() || (count && !targets))return Reject(E_INVALIDARG);
  std::array<ID3D11RenderTargetView*,D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> next{};
  for(UINT i=0;i<count;++i)next[i]=targets[i];
  if(next==targets_ && depth==depth_){++statistics_.skipped;return S_OK;}
  if(!Owns(depth))return Reject(E_INVALIDARG);
  for(UINT i=0;i<count;++i)if(!Owns(next[i]))return Reject(E_INVALIDARG);
  std::array<Range,D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> ranges{};Range depthRange;
  auto hr=Describe(depth,depthRange);if(FAILED(hr))return Reject(hr);
  auto depthConflict=depthRange;depthConflict.aspects=7;
  for(UINT i=0;i<count;++i){hr=Describe(next[i],ranges[i]);if(FAILED(hr))return Reject(hr);}
  const Range* shape=depth?&depthRange:nullptr;
  for(UINT i=0;i<count;++i)if(next[i]) {
    if(shape && (shape->width!=ranges[i].width || shape->height!=ranges[i].height || shape->slices!=ranges[i].slices || shape->samples!=ranges[i].samples || shape->quality!=ranges[i].quality))return Reject(E_INVALIDARG);
    if(!shape)shape=&ranges[i];
    if(Overlap(ranges[i],depthConflict))return Reject(E_INVALIDARG);
    for(UINT j=0;j<i;++j)if(Overlap(ranges[i],ranges[j]))return Reject(E_INVALIDARG);
  }
  for(unsigned stage=0;stage<2;++stage) {
    auto& inputs=inputs_[stage];UINT begin=UINT(inputs.size()),end=0;
    std::array<ID3D11ShaderResourceView*,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> actual{};
    for(UINT i=0;i<inputs.size();++i) {
      bool conflict=Overlap(inputs[i].range,depthRange);
      for(UINT t=0;t<count && !conflict;++t)conflict=Overlap(inputs[i].range,ranges[t]);
      if(conflict){inputs[i]={};begin=std::min(begin,i);end=i+1;++statistics_.hazardUnbinds;}
      actual[i]=inputs[i].view;
    }
    if(begin<end)SetResources(stage,begin,end-begin,actual.data()+begin);
  }
  context_->OMSetRenderTargets(count,targets,depth);++statistics_.calls;
  targets_=next;targetRanges_=std::move(ranges);depth_=depth;depthRange_=std::move(depthRange);return S_OK;
}
