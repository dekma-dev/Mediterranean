#include "engine/Renderer.h"
#include "engine/ShaderInterface.h"
#include "SceneBytecode.generated.h"
#include <algorithm>
#include <cmath>
using namespace DirectX;

void Renderer::DrawPressureRibbon(XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 d,float from,float to,float age,XMFLOAT4 color){
 const auto finite=[](XMFLOAT3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
 if((portrait_ && !portraitEffects_) || cloudSprites_.size()>=96 || EffectVertices()+6>maxEffectVertices ||
   !finite(a)||!finite(b)||!finite(c)||!finite(d)||!std::isfinite(from)||!std::isfinite(to)||!std::isfinite(age)||
   !std::isfinite(color.x)||!std::isfinite(color.y)||!std::isfinite(color.z)||!std::isfinite(color.w)||color.w<=0)return;
 const auto center=(XMLoadFloat3(&a)+XMLoadFloat3(&b)+XMLoadFloat3(&c)+XMLoadFloat3(&d))*.25f;
 const float depth=XMVectorGetW(XMVector4Transform(XMVectorSetW(center,1),XMLoadFloat4x4(&viewProjection_)));
 if(depth<=.1f)return;
 const float u=from-age*1.8f,v=to-age*1.8f;
 const visual::Vertex va{a,{u,0,6},color},vb{b,{u,1,6},color},vc{c,{v,1,6},color},vd{d,{v,0,6},color};
 cloudSprites_.push_back({{va,vb,vc,va,vc,vd},depth});
}

void Renderer::DrawPressureWave(XMFLOAT3 center,XMFLOAT3 radii,float facing,float age,float strength,unsigned style){
 const auto finite=[](XMFLOAT3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
 if(!pressureEnabled_ || pressureClosed_ || portrait_ || pressurePasses_ || pressureCount_==pressureWaves_.size() ||
   !finite(center)||!finite(radii)||!std::isfinite(facing)||!std::isfinite(age)||!std::isfinite(strength)||
   age<0 || age>1 || strength<=0 || std::min({radii.x,radii.y,radii.z})<.04f ||
   std::max({radii.x,radii.y,radii.z})>4.5f)return;
 // Avoid camera-enclosing lenses and unbounded near-plane projection.
 const float radius=std::max({radii.x,radii.y,radii.z});
 const float distance=std::sqrt((eye_.x-center.x)*(eye_.x-center.x)+(eye_.y-center.y)*(eye_.y-center.y)+(eye_.z-center.z)*(eye_.z-center.z));
 if(distance<radius+.25f)return;
 unsigned outside=63;
 for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){
  XMFLOAT4 c;XMStoreFloat4(&c,XMVector4Transform(XMVectorSet(center.x+x*radius,center.y+y*radius,center.z+z*radius,1),XMLoadFloat4x4(&viewProjection_)));
  unsigned mask=0;mask|=c.x<-c.w?1u:0u;mask|=c.x>c.w?2u:0u;mask|=c.y<-c.w?4u:0u;mask|=c.y>c.w?8u:0u;
  mask|=c.z<0?16u:0u;mask|=c.z>c.w?32u:0u;outside&=mask;
 }
 if(outside)return;
 pressureWaves_[pressureCount_++]={{center.x,center.y,center.z,std::clamp(strength,0.f,1.f)},
   {radii.x,radii.y,radii.z,age},{std::sin(facing),std::cos(facing),float(std::min(style,2u)),0}};
 pressureSubmitted_=pressureCount_;
}
bool Renderer::CompositePressure(){
 if(!pressureCount_)return true;
 const unsigned count=pressureCount_;pressureCount_=0;
 if(!pressureEnabled_ || portrait_ || pressureClosed_ || pressurePasses_ || std::size_t(width_)*height_>8388608)return true;
 // Conservative projected world boxes; one shared scissor limits fill and
 // copying. The 16 px border contains the maximum refracted sample offset.
 float left=float(width_),top=float(height_),right=0,bottom=0;
 for(unsigned n=0;n<count;++n){const auto& w=pressureWaves_[n];const float radius=std::max({w.radiiAge.x,w.radiiAge.y,w.radiiAge.z});
  bool nearPlane=false;float l=float(width_),t=float(height_),r=0,b=0;
  for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1}){
   XMFLOAT4 clip;XMStoreFloat4(&clip,XMVector4Transform(XMVectorSet(w.centerStrength.x+x*radius,w.centerStrength.y+y*radius,w.centerStrength.z+z*radius,1),XMLoadFloat4x4(&viewProjection_)));
   if(clip.w<=.1f){nearPlane=true;continue;}
   const float px=(clip.x/clip.w*.5f+.5f)*width_,py=(.5f-clip.y/clip.w*.5f)*height_;
   l=std::min(l,px);r=std::max(r,px);t=std::min(t,py);b=std::max(b,py);
  }
  if(nearPlane){l=t=0;r=float(width_);b=float(height_);}
  left=std::min(left,l);right=std::max(right,r);top=std::min(top,t);bottom=std::max(bottom,b);
 }
 if(right<=0 || bottom<=0 || left>=width_ || top>=height_)return true;
 const D3D11_RECT rect{LONG(std::clamp(std::floor(left)-16,0.f,float(width_))),LONG(std::clamp(std::floor(top)-16,0.f,float(height_))),
   LONG(std::clamp(std::ceil(right)+16,0.f,float(width_))),LONG(std::clamp(std::ceil(bottom)+16,0.f,float(height_)))};
 if(rect.left>=rect.right || rect.top>=rect.bottom)return true;
 using Data=shader::Pressure;
 if(!pressureVertex_){
  if(!Check(device_->CreateVertexShader(compiled_scene::VSGlow.data,compiled_scene::VSGlow.size,nullptr,pressureVertex_.GetAddressOf()),L"Create pressure vertex shader") ||
    !Check(device_->CreatePixelShader(compiled_scene::PSPressure.data,compiled_scene::PSPressure.size,nullptr,pressurePixel_.GetAddressOf()),L"Create pressure pixel shader"))return false;
  D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=sizeof(Data);buffer.Usage=D3D11_USAGE_DEFAULT;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(!Check(device_->CreateBuffer(&buffer,nullptr,pressureConstants_.GetAddressOf()),L"Create pressure constants"))return false;
  D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;raster.ScissorEnable=TRUE;
  if(!Check(device_->CreateRasterizerState(&raster,pressureRasterizer_.GetAddressOf()),L"Create pressure scissor"))return false;
 }
 Microsoft::WRL::ComPtr<ID3D11Resource> scene;target_->GetResource(scene.GetAddressOf());
 if(!pressureCopy_){
  Microsoft::WRL::ComPtr<ID3D11Texture2D> source;if(!Check(scene.As(&source),L"Read pressure scene texture"))return false;
  D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=desc.MiscFlags=0;
  if(!Check(device_->CreateTexture2D(&desc,nullptr,pressureCopy_.GetAddressOf()),L"Create pressure scene copy") ||
    !Check(device_->CreateShaderResourceView(pressureCopy_.Get(),nullptr,pressureView_.GetAddressOf()),L"Create pressure scene view"))return false;
 }
 ID3D11ShaderResourceView* empty=nullptr;BindResources(RenderBindings::Stage::Pixel,shader::texture::pressure,1,&empty);
 if(!BindTargets(0,nullptr,nullptr))return false;
 const D3D11_BOX box{UINT(rect.left),UINT(rect.top),0,UINT(rect.right),UINT(rect.bottom),1};
 context_->CopySubresourceRegion(pressureCopy_.Get(),0,rect.left,rect.top,0,scene.Get(),0,&box);
 Data data{};XMStoreFloat4x4(&data.inverseVP,XMMatrixTranspose(XMMatrixInverse(nullptr,XMLoadFloat4x4(&viewProjection_))));
 XMStoreFloat4x4(&data.vp,XMMatrixTranspose(XMLoadFloat4x4(&viewProjection_)));
 data.eye={eye_.x,eye_.y,eye_.z,float(count)};data.screen={1.f/width_,1.f/height_,std::min(8.f,6.f*height_/1080.f),0};
 data.region={(rect.left+.5f)/width_,(rect.top+.5f)/height_,(rect.right-.5f)/width_,(rect.bottom-.5f)/height_};data.waves=pressureWaves_;
 context_->UpdateSubresource(pressureConstants_.Get(),0,nullptr,&data,0,0);
 context_->PSSetConstantBuffers(shader::slot::pressure,1,pressureConstants_.GetAddressOf());
 if(!BindTargets(1,target_.GetAddressOf(),nullptr))return false;context_->OMSetDepthStencilState(depthOff_.Get(),0);context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
 BindResources(RenderBindings::Stage::Pixel,shader::texture::depth,1,effectDepthResource_.GetAddressOf());BindResources(RenderBindings::Stage::Pixel,shader::texture::pressure,1,pressureView_.GetAddressOf());
 BindSamplers(RenderBindings::Stage::Pixel,shader::sampler::font,1,fontSampler_.GetAddressOf());
 const D3D11_VIEWPORT view{0,0,float(width_),float(height_),0,1};context_->RSSetViewports(1,&view);
 context_->RSSetScissorRects(1,&rect);context_->RSSetState(pressureRasterizer_.Get());
 context_->IASetInputLayout(nullptr);context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 context_->VSSetShader(pressureVertex_.Get(),nullptr,0);context_->PSSetShader(pressurePixel_.Get(),nullptr,0);if(!FrameActive())return false;context_->Draw(3,0);
 ++drawCalls_;++triangles_;++pressurePasses_;
 BindResources(RenderBindings::Stage::Pixel,shader::texture::pressure,1,&empty);BindResources(RenderBindings::Stage::Pixel,shader::texture::depth,1,&empty);
 context_->RSSetState(rasterizer_.Get());if(!BindTargets(1,target_.GetAddressOf(),depth_.Get()))return false;
 return true;
}
