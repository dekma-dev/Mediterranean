#include "engine/Renderer.h"
#include "SceneBytecode.generated.h"
#include <algorithm>
#include <cmath>
using namespace DirectX;
namespace {struct GlowConstants {XMFLOAT4 texel,screen,region;};}

bool Renderer::CreateSpellGlow(){
 if(!glowVertex_){
  const auto& v=compiled_scene::VSGlow;
  if(!Check(device_->CreateVertexShader(v.data,v.size,nullptr,glowVertex_.GetAddressOf()),L"Create spell glare vertex shader"))return false;
  const compiled_scene::Bytecode* shaders[]={&compiled_scene::PSGlowSource,&compiled_scene::PSGlowHorizontal,&compiled_scene::PSGlowVertical,&compiled_scene::PSGlowComposite};
  ID3D11PixelShader** destinations[]={glowSource_.GetAddressOf(),glowHorizontal_.GetAddressOf(),glowVertical_.GetAddressOf(),glowComposite_.GetAddressOf()};
  for(int n=0;n<4;++n)if(!Check(device_->CreatePixelShader(shaders[n]->data,shaders[n]->size,nullptr,destinations[n]),L"Create spell glare pixel shader"))return false;
  D3D11_BUFFER_DESC b{};b.ByteWidth=sizeof(GlowConstants);b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(!Check(device_->CreateBuffer(&b,nullptr,glowConstants_.GetAddressOf()),L"Create spell glare constants"))return false;
 }
 if(!glowWidth_){
  const float scale=std::max({2.f,width_/960.f,height_/540.f});
  const unsigned w=std::clamp(unsigned(std::ceil(width_/scale)),1u,960u),h=std::clamp(unsigned(std::ceil(height_/scale)),1u,540u);
  D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;
  d.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
  for(unsigned n=0;n<3;++n){Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
   if(!Check(device_->CreateTexture2D(&d,nullptr,&texture),L"Create bounded spell glare texture") ||
      !Check(device_->CreateRenderTargetView(texture.Get(),nullptr,glowTargets_[n].ReleaseAndGetAddressOf()),L"Create spell glare target") ||
      !Check(device_->CreateShaderResourceView(texture.Get(),nullptr,glowViews_[n].ReleaseAndGetAddressOf()),L"Create spell glare view"))return false;
  }glowWidth_=w;glowHeight_=h;
 }
 const D3D11_VIEWPORT view=portrait_?portraitViewport_:D3D11_VIEWPORT{0,0,float(width_),float(height_),0,1};
 const GlowConstants c{{1.f/glowWidth_,1.f/glowHeight_,float(glowWidth_),float(glowHeight_)},
  {float(width_)/glowWidth_,float(height_)/glowHeight_,0,0},
  {view.TopLeftX/width_,view.TopLeftY/height_,(view.TopLeftX+view.Width)/width_,(view.TopLeftY+view.Height)/height_}};
 context_->UpdateSubresource(glowConstants_.Get(),0,nullptr,&c,0,0);context_->PSSetConstantBuffers(6,1,glowConstants_.GetAddressOf());
 return true;
}
void Renderer::EmitSpellGlow(UINT vertices){
 D3D11_VIEWPORT view=portrait_?portraitViewport_:D3D11_VIEWPORT{0,0,float(width_),float(height_),0,1};
 auto reduced=view;reduced.TopLeftX*=float(glowWidth_)/width_;reduced.Width*=float(glowWidth_)/width_;
 reduced.TopLeftY*=float(glowHeight_)/height_;reduced.Height*=float(glowHeight_)/height_;
 context_->RSSetViewports(1,&reduced);context_->OMSetRenderTargets(1,glowTargets_[0].GetAddressOf(),nullptr);
 context_->OMSetDepthStencilState(depthOff_.Get(),0);context_->OMSetBlendState(effectBlend_.Get(),nullptr,0xffffffff);
 context_->PSSetShader(glowSource_.Get(),nullptr,0);context_->Draw(vertices,0);++drawCalls_;++spellGlowPasses_;triangles_+=vertices/3;
 context_->RSSetViewports(1,&view);context_->OMSetRenderTargets(1,target_.GetAddressOf(),effectDepthView_.Get());
}
void Renderer::CompositeSpellGlow(){
 const D3D11_VIEWPORT reduced{0,0,float(glowWidth_),float(glowHeight_),0,1},full{0,0,float(width_),float(height_),0,1};
 ID3D11ShaderResourceView* empty=nullptr;
 context_->IASetInputLayout(nullptr);context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 context_->VSSetShader(glowVertex_.Get(),nullptr,0);context_->RSSetState(effectRasterizer_.Get());
 context_->OMSetDepthStencilState(depthOff_.Get(),0);context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
 context_->RSSetViewports(1,&reduced);
 for(unsigned pass=0;pass<2;++pass){
  context_->PSSetShaderResources(16,1,&empty);context_->OMSetRenderTargets(1,glowTargets_[pass+1].GetAddressOf(),nullptr);
  context_->PSSetShaderResources(16,1,glowViews_[pass].GetAddressOf());
  context_->PSSetShader(pass?glowVertical_.Get():glowHorizontal_.Get(),nullptr,0);context_->Draw(3,0);++drawCalls_;++spellGlowPasses_;++triangles_;
 }
 context_->PSSetShaderResources(16,1,&empty);context_->OMSetRenderTargets(1,target_.GetAddressOf(),nullptr);
 context_->PSSetShaderResources(16,1,glowViews_[2].GetAddressOf());context_->RSSetViewports(1,&full);
 context_->OMSetBlendState(effectBlend_.Get(),nullptr,0xffffffff);context_->PSSetShader(glowComposite_.Get(),nullptr,0);
 context_->Draw(3,0);++drawCalls_;++spellGlowPasses_;++triangles_;
 context_->PSSetShaderResources(16,1,&empty);context_->OMSetBlendState(nullptr,nullptr,0xffffffff);context_->RSSetState(rasterizer_.Get());
 if(portrait_)context_->RSSetViewports(1,&portraitViewport_);
}
