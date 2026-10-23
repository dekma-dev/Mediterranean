#include "engine/Renderer.h"
#include "game/WorldMap.h"
#include "SceneBytecode.generated.h"
#include "engine/Shadow.h"
#include "engine/InstanceTransform.h"
#include <cmath>
#include <d3d11sdklayers.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <wincodec.h>
#include <algorithm>
using namespace DirectX;
using Microsoft::WRL::ComPtr;
namespace {
using Vertex = visual::Vertex;
struct Constants {
 XMFLOAT4X4 world,wvp,normalWorld,shadowVP[visual::shadowLayers]; XMFLOAT4 color,eyeAndHud;
 XMFLOAT4 sun=visual::daylightSun;
 XMFLOAT4 atmosphere{0,.85f,.006f,.57f}; // cloud phase, exposure, haze density, cloud threshold
};
static_assert(sizeof(Constants)==384,"Keep the HLSL Scene layout and offscreen shader diagnostics in sync.");
static_assert(visual::shadowLayers==2,"The scene shader uses a whole-island and a detail shadow layer.");
}
bool Renderer::Check(HRESULT result, const wchar_t* operation) {
 if (SUCCEEDED(result)) return true;
 std::wostringstream s; s << operation << L" failed (0x" << std::hex << static_cast<unsigned long>(result) << L").";
 error_ = s.str(); OutputDebugStringW(error_.c_str()); return false;
}
bool Renderer::Initialize(HWND window, int width, int height) {
 DXGI_SWAP_CHAIN_DESC d{};
 d.BufferCount=2; d.BufferDesc.Width=width; d.BufferDesc.Height=height;
 d.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; d.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
 d.OutputWindow=window; d.SampleDesc.Count=1; d.Windowed=TRUE; d.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
 UINT flags=0;
#ifdef _DEBUG
 flags=D3D11_CREATE_DEVICE_DEBUG;
#endif
 const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0}; D3D_FEATURE_LEVEL actual{};
 auto create = [&](UINT f) { return D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,
  f,levels,1,D3D11_SDK_VERSION,&d,swapChain_.GetAddressOf(),device_.GetAddressOf(),&actual,context_.GetAddressOf()); };
 HRESULT hr=create(flags);
 if (hr==DXGI_ERROR_SDK_COMPONENT_MISSING && flags) {
  swapChain_.Reset(); device_.Reset(); context_.Reset(); hr=create(0);
 }
 if (!Check(hr,L"Create DirectX 11 device")) return false;
 ComPtr<IDXGIFactory> factory;
 if (SUCCEEDED(swapChain_->GetParent(IID_PPV_ARGS(factory.GetAddressOf()))))
  factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER);
 if (!CreateTargets(width,height) || !CreatePipeline() || !CreateUiArtwork()) return false;
 SetCamera({0,3,-6},{0,1,0});
 return true;
}
bool Renderer::CreateTargets(int width, int height) {
 ComPtr<ID3D11Texture2D> back;
 if (!Check(swapChain_->GetBuffer(0,IID_PPV_ARGS(back.GetAddressOf())),L"Get back buffer")) return false;
 if (!Check(device_->CreateRenderTargetView(back.Get(),nullptr,target_.GetAddressOf()),L"Create render target")) return false;
 D3D11_TEXTURE2D_DESC d{}; d.Width=width; d.Height=height; d.MipLevels=1; d.ArraySize=1;
 d.Format=DXGI_FORMAT_D24_UNORM_S8_UINT; d.SampleDesc.Count=1; d.BindFlags=D3D11_BIND_DEPTH_STENCIL;
 ComPtr<ID3D11Texture2D> texture;
 if (!Check(device_->CreateTexture2D(&d,nullptr,texture.GetAddressOf()),L"Create depth texture")) return false;
 if (!Check(device_->CreateDepthStencilView(texture.Get(),nullptr,depth_.GetAddressOf()),L"Create depth view")) return false;
 width_=width; height_=height; return true;
}
bool Renderer::Resize(int width, int height) {
 if (width<=0 || height<=0 || (width==width_ && height==height_)) return true;
 context_->OMSetRenderTargets(0,nullptr,nullptr); target_.Reset(); depth_.Reset();
 if (!Check(swapChain_->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0),L"Resize swap chain")) return false;
 return CreateTargets(width,height);
}
bool Renderer::CreatePipeline() {
 const auto *vs=&compiled_scene::VSMain,*ps=&compiled_scene::PSMain,*world=&compiled_scene::VSWorld,*shadow=&compiled_scene::VSShadow;
 const auto *body=&compiled_scene::VSBody,*bodyShadow=&compiled_scene::VSBodyShadow;
 const auto *hand=&compiled_scene::VSHand,*handShadow=&compiled_scene::VSHandShadow;
 const auto *sky=&compiled_scene::VSSky,*skyPs=&compiled_scene::PSSky,*effectVs=&compiled_scene::VSEffect,*effectPs=&compiled_scene::PSEffect;
 const auto& cloud=compiled_scene::PSCloudLight;
 if (!Check(device_->CreatePixelShader(cloud.GetBufferPointer(),cloud.GetBufferSize(),nullptr,cloudLightShader_.GetAddressOf()),L"Create cloud lighting shader")) return false;
 if (!Check(device_->CreateVertexShader(effectVs->GetBufferPointer(),effectVs->GetBufferSize(),nullptr,effectVertexShader_.GetAddressOf()),L"Create effect vertex shader") ||
   !Check(device_->CreatePixelShader(effectPs->GetBufferPointer(),effectPs->GetBufferSize(),nullptr,effectPixelShader_.GetAddressOf()),L"Create effect pixel shader")) return false;
 if (!Check(device_->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,vertexShader_.GetAddressOf()),L"Create HUD shader") ||
   !Check(device_->CreateVertexShader(body->GetBufferPointer(),body->GetBufferSize(),nullptr,bodyShader_.GetAddressOf()),L"Create skinning shader") ||
   !Check(device_->CreateVertexShader(bodyShadow->GetBufferPointer(),bodyShadow->GetBufferSize(),nullptr,bodyShadowShader_.GetAddressOf()),L"Create skinned shadow shader") ||
   !Check(device_->CreateVertexShader(hand->GetBufferPointer(),hand->GetBufferSize(),nullptr,handShader_.GetAddressOf()),L"Create hand shader") ||
   !Check(device_->CreateVertexShader(handShadow->GetBufferPointer(),handShadow->GetBufferSize(),nullptr,handShadowShader_.GetAddressOf()),L"Create hand shadow shader") ||
   !Check(device_->CreateVertexShader(world->GetBufferPointer(),world->GetBufferSize(),nullptr,worldShader_.GetAddressOf()),L"Create instance shader") ||
   !Check(device_->CreateVertexShader(shadow->GetBufferPointer(),shadow->GetBufferSize(),nullptr,shadowShader_.GetAddressOf()),L"Create shadow shader") ||
   !Check(device_->CreateVertexShader(sky->GetBufferPointer(),sky->GetBufferSize(),nullptr,skyShader_.GetAddressOf()),L"Create sky shader") ||
   !Check(device_->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,pixelShader_.GetAddressOf()),L"Create surface shader") ||
   !Check(device_->CreatePixelShader(skyPs->GetBufferPointer(),skyPs->GetBufferSize(),nullptr,skyPixel_.GetAddressOf()),L"Create sky pixel shader")) return false;
 std::vector<D3D11_INPUT_ELEMENT_DESC> elements={
  {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
  {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
  {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}};
 if (!Check(device_->CreateInputLayout(elements.data(),static_cast<UINT>(elements.size()),vs->GetBufferPointer(),vs->GetBufferSize(),layout_.GetAddressOf()),L"Create HUD layout")) return false;
 for (UINT i=0;i<4;++i) elements.push_back({"WORLD",i,DXGI_FORMAT_R32G32B32A32_FLOAT,1,i*16,D3D11_INPUT_PER_INSTANCE_DATA,1});
 for (UINT i=0;i<4;++i) elements.push_back({"INORMAL",i,DXGI_FORMAT_R32G32B32A32_FLOAT,1,64+i*16,D3D11_INPUT_PER_INSTANCE_DATA,1});
 elements.push_back({"PAINT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,128,D3D11_INPUT_PER_INSTANCE_DATA,1});
 elements.push_back({"SURFACE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,144,D3D11_INPUT_PER_INSTANCE_DATA,1});
 if (!Check(device_->CreateInputLayout(elements.data(),static_cast<UINT>(elements.size()),world->GetBufferPointer(),world->GetBufferSize(),worldLayout_.GetAddressOf()),L"Create instance layout")) return false;
 elements.push_back({"BLENDINDICES",0,DXGI_FORMAT_R32G32B32A32_UINT,2,0,D3D11_INPUT_PER_VERTEX_DATA,0});
 elements.push_back({"BLENDWEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,2,16,D3D11_INPUT_PER_VERTEX_DATA,0});
 if (!Check(device_->CreateInputLayout(elements.data(),static_cast<UINT>(elements.size()),body->GetBufferPointer(),body->GetBufferSize(),bodyLayout_.GetAddressOf()),L"Create skinned layout")) return false;
 static_assert(sizeof(Instance)==160,"Instance layout must match the shader.");
 static_assert(sizeof(visual::SkinInfluence)==32 && sizeof(visual::BodyPalette)==1376,"Keep skinning layouts synchronized with HLSL.");
 static_assert(sizeof(bodyPalettes_)<=65536,"Skin palettes must fit a D3D11 constant buffer.");
 static_assert(sizeof(visual::HandPalette)==1024 && sizeof(handPalettes_)==65536,"Hand palettes must match HLSL and the D3D11 constant limit.");
 std::vector<Vertex> vertices; std::vector<std::uint32_t> indices;
 std::vector<visual::SkinInfluence> influences;
 for (unsigned i=0;i<meshCount;++i) {
  auto mesh=visual::BuildMesh(static_cast<visual::Mesh>(i));
  auto& range=meshes_[i]; range.start=static_cast<UINT>(indices.size()); range.count=static_cast<UINT>(mesh.indices.size());
  for (auto index : mesh.indices) indices.push_back(index+static_cast<UINT>(vertices.size()));
  for (const auto& v : mesh.vertices) range.radius=std::max(range.radius,std::sqrt(v.position.x*v.position.x+v.position.y*v.position.y+v.position.z*v.position.z));
  vertices.insert(vertices.end(),mesh.vertices.begin(),mesh.vertices.end());
  if(visual::IsBodyMesh(static_cast<visual::Mesh>(i))) {
    const auto& weights=visual::CharacterBody(i-unsigned(visual::Mesh::Body)).influences;
    influences.insert(influences.end(),weights.begin(),weights.end());
  }
  else if(visual::IsHandMesh(static_cast<visual::Mesh>(i))) {
    const auto index=i-unsigned(visual::Mesh::Hand);
    const auto& weights=visual::CharacterHand(index/2,index%2!=0).influences;
    influences.insert(influences.end(),weights.begin(),weights.end());
  } else influences.resize(vertices.size());
 }
 D3D11_BUFFER_DESC b{}; b.ByteWidth=static_cast<UINT>(vertices.size()*sizeof(Vertex));
 b.Usage=D3D11_USAGE_IMMUTABLE; b.BindFlags=D3D11_BIND_VERTEX_BUFFER;
 D3D11_SUBRESOURCE_DATA data{}; data.pSysMem=vertices.data(); meshBytes_=b.ByteWidth;
 if (!Check(device_->CreateBuffer(&b,&data,vertices_.GetAddressOf()),L"Create reusable meshes")) return false;
 b.ByteWidth=static_cast<UINT>(influences.size()*sizeof(visual::SkinInfluence)); data.pSysMem=influences.data(); meshBytes_+=b.ByteWidth;
 if (!Check(device_->CreateBuffer(&b,&data,skinWeights_.GetAddressOf()),L"Create skin weights")) return false;
 b.ByteWidth=static_cast<UINT>(indices.size()*sizeof(std::uint32_t)); b.BindFlags=D3D11_BIND_INDEX_BUFFER;
 data.pSysMem=indices.data(); meshBytes_+=b.ByteWidth;
 if (!Check(device_->CreateBuffer(&b,&data,indices_.GetAddressOf()),L"Create mesh indices")) return false;
 b={}; b.ByteWidth=sizeof(Constants); b.Usage=D3D11_USAGE_DEFAULT; b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
 if (!Check(device_->CreateBuffer(&b,nullptr,constants_.GetAddressOf()),L"Create constants")) return false;
 b.ByteWidth=sizeof(XMFLOAT4);
 const XMFLOAT4 emptyField{};
 D3D11_SUBRESOURCE_DATA fieldData{}; fieldData.pSysMem=&emptyField;
 if (!Check(device_->CreateBuffer(&b,&fieldData,cloudLightConstants_.GetAddressOf()),L"Create cloud field constants")) return false;
 b.ByteWidth=sizeof(bodyPalettes_); b.Usage=D3D11_USAGE_DYNAMIC; b.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
 if (!Check(device_->CreateBuffer(&b,nullptr,skinConstants_.GetAddressOf()),L"Create skin palettes")) return false;
 b.ByteWidth=sizeof(handPalettes_);
 if (!Check(device_->CreateBuffer(&b,nullptr,handConstants_.GetAddressOf()),L"Create hand palettes")) return false;
 b.ByteWidth=16380*sizeof(Vertex); b.Usage=D3D11_USAGE_DYNAMIC; b.BindFlags=D3D11_BIND_VERTEX_BUFFER; b.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
 if (!Check(device_->CreateBuffer(&b,nullptr,hudVertices_.GetAddressOf()),L"Create HUD buffer")) return false;
 b.ByteWidth=maxInstances*sizeof(Instance);
 if (!Check(device_->CreateBuffer(&b,nullptr,instances_.GetAddressOf()),L"Create bounded instance buffer")) return false;
 instanceScratch_.reserve(maxInstances); hud_.reserve(16380); effects_.reserve(maxEffectVertices);
 D3D11_RASTERIZER_DESC r{}; r.FillMode=D3D11_FILL_SOLID; r.CullMode=D3D11_CULL_BACK; r.DepthClipEnable=TRUE;
 if (!Check(device_->CreateRasterizerState(&r,rasterizer_.GetAddressOf()),L"Create rasterizer")) return false;
 r.CullMode=D3D11_CULL_NONE;
 if (!Check(device_->CreateRasterizerState(&r,effectRasterizer_.GetAddressOf()),L"Create effect rasterizer")) return false;
 r.CullMode=D3D11_CULL_BACK;
 // Receiver-plane filtering handles slopes; a large raster bias erases boot/step
 // contact shadows. Keep only a small, bounded floating-point raster offset.
 r.DepthBias=16; r.SlopeScaledDepthBias=.15f; r.DepthBiasClamp=.0001f;
 if (!Check(device_->CreateRasterizerState(&r,shadowRasterizer_.GetAddressOf()),L"Create shadow rasterizer")) return false;
 D3D11_DEPTH_STENCIL_DESC z{}; z.DepthEnable=TRUE; z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL; z.DepthFunc=D3D11_COMPARISON_LESS;
 if (!Check(device_->CreateDepthStencilState(&z,depthOn_.GetAddressOf()),L"Create depth state")) return false;
 z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO; z.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
 if (!Check(device_->CreateDepthStencilState(&z,effectDepth_.GetAddressOf()),L"Create effect depth state")) return false;
 z.DepthEnable=FALSE; z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
 if (!Check(device_->CreateDepthStencilState(&z,depthOff_.GetAddressOf()),L"Create HUD depth state")) return false;
 shadowProjection_=visual::ShadowProjections({0,0,0});
 if (!gpuTimer_.Initialize(device_.Get())) { error_=L"Could not create GPU timing queries."; return false; }
 if (!font_.Initialize(device_.Get())) { error_=L"Could not rasterize the interface font."; return false; }
 D3D11_SAMPLER_DESC fontSampler{}; fontSampler.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
 fontSampler.AddressU=fontSampler.AddressV=fontSampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
 fontSampler.MaxLOD=D3D11_FLOAT32_MAX;
 if (!Check(device_->CreateSamplerState(&fontSampler,fontSampler_.GetAddressOf()),L"Create font sampler")) return false;
 D3D11_BLEND_DESC blend{}; auto& channel=blend.RenderTarget[0]; channel.BlendEnable=TRUE;
 channel.SrcBlend=D3D11_BLEND_SRC_ALPHA; channel.DestBlend=D3D11_BLEND_INV_SRC_ALPHA; channel.BlendOp=D3D11_BLEND_OP_ADD;
 channel.SrcBlendAlpha=D3D11_BLEND_ONE; channel.DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA; channel.BlendOpAlpha=D3D11_BLEND_OP_ADD;
 channel.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
 if (!Check(device_->CreateBlendState(&blend,hudBlend_.GetAddressOf()),L"Create interface blending")) return false;
 channel.DestBlend=D3D11_BLEND_ONE; channel.SrcBlendAlpha=D3D11_BLEND_ZERO; channel.DestBlendAlpha=D3D11_BLEND_ONE;
 if (!Check(device_->CreateBlendState(&blend,effectBlend_.GetAddressOf()),L"Create effect blending")) return false;
 return CreateMaterials();
}

bool Renderer::CreateMaterials() {
 constexpr UINT size=visual::materialSize,layers=static_cast<UINT>(visual::Material::Count);
 D3D11_TEXTURE2D_DESC texture{}; texture.Width=texture.Height=size; texture.MipLevels=0; texture.ArraySize=layers;
 texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM; texture.SampleDesc.Count=1; texture.Usage=D3D11_USAGE_DEFAULT;
 texture.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET; texture.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
 ComPtr<ID3D11Texture2D> materials;
 if (!Check(device_->CreateTexture2D(&texture,nullptr,materials.GetAddressOf()),L"Create procedural material array")) return false;
 materials->GetDesc(&texture);
 for (UINT layer=0;layer<layers;++layer) {
  const auto pixels=visual::BuildMaterialLayer(static_cast<visual::Material>(layer),size);
  context_->UpdateSubresource(materials.Get(),D3D11CalcSubresource(0,layer,texture.MipLevels),nullptr,pixels.data(),size*4,0);
 }
 if (!Check(device_->CreateShaderResourceView(materials.Get(),nullptr,materialView_.GetAddressOf()),L"Create material view")) return false;
 context_->GenerateMips(materialView_.Get());
 for (UINT dimension=size;dimension;dimension/=2) materialBytes_+=dimension*dimension*4*layers;
 texture={}; texture.Width=texture.Height=visual::shadowSize; texture.MipLevels=1;
 texture.ArraySize=visual::shadowLayers; texture.SampleDesc.Count=1;
 texture.Format=DXGI_FORMAT_R32_TYPELESS; texture.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
 ComPtr<ID3D11Texture2D> shadow;
 if (!Check(device_->CreateTexture2D(&texture,nullptr,shadow.GetAddressOf()),L"Create two-layer shadow map")) return false;
 D3D11_DEPTH_STENCIL_VIEW_DESC depth{}; depth.Format=DXGI_FORMAT_D32_FLOAT; depth.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
 depth.Texture2DArray.ArraySize=1;
 for (unsigned layer=0;layer<visual::shadowLayers;++layer) {
  depth.Texture2DArray.FirstArraySlice=layer;
  if (!Check(device_->CreateDepthStencilView(shadow.Get(),&depth,shadowDepth_[layer].GetAddressOf()),L"Create shadow layer depth view")) return false;
 }
 D3D11_SHADER_RESOURCE_VIEW_DESC resource{}; resource.Format=DXGI_FORMAT_R32_FLOAT;
 resource.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
 resource.Texture2DArray.MipLevels=1; resource.Texture2DArray.ArraySize=visual::shadowLayers;
 if (!Check(device_->CreateShaderResourceView(shadow.Get(),&resource,shadowView_.GetAddressOf()),L"Create shadow resource view")) return false;
 materialBytes_+=visual::shadowSize*visual::shadowSize*4*visual::shadowLayers;
 // A two-metre grid preserves the broad cloud footprint without repeating
 // twelve volume samples for every vertex of the island and its characters.
 texture={}; texture.Width=texture.Height=cloudLightSize; texture.MipLevels=1;
 texture.ArraySize=1; texture.SampleDesc.Count=1; texture.Format=DXGI_FORMAT_R32_FLOAT;
 texture.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 ComPtr<ID3D11Texture2D> cloudLight;
 if (!Check(device_->CreateTexture2D(&texture,nullptr,cloudLight.GetAddressOf()),L"Create cloud lighting field") ||
     !Check(device_->CreateRenderTargetView(cloudLight.Get(),nullptr,cloudLightTarget_.GetAddressOf()),L"Create cloud lighting target") ||
     !Check(device_->CreateShaderResourceView(cloudLight.Get(),nullptr,cloudLightView_.GetAddressOf()),L"Create cloud lighting view")) return false;
 materialBytes_+=cloudLightSize*cloudLightSize*sizeof(float);
 D3D11_SAMPLER_DESC sampler{}; sampler.Filter=D3D11_FILTER_ANISOTROPIC; sampler.MaxAnisotropy=4;
 sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
 sampler.MaxLOD=D3D11_FLOAT32_MAX;
 if (!Check(device_->CreateSamplerState(&sampler,surfaceSampler_.GetAddressOf()),L"Create surface sampler")) return false;
 sampler.Filter=D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT; sampler.ComparisonFunc=D3D11_COMPARISON_LESS_EQUAL;
 sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_BORDER;
 for (auto& channel : sampler.BorderColor) channel=1;
 return Check(device_->CreateSamplerState(&sampler,shadowSampler_.GetAddressOf()),L"Create shadow sampler");
}
void Renderer::BeginFrame() {
 drawCalls_=0; frameFailed_=false; portrait_=false; skyPending_=true;
 cloudLightUpdates_=0;
 context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
 triangles_=0;
 gpuTimer_.Begin(context_.Get());
 hud_.clear();
 effects_.clear(); effectVertices_=0;
 for (auto& batch : world_) batch.clear();
 bodyCount_=handCount_=0;
 bodyLodCounts_={}; bodyLodHistory_.BeginFrame();
 attachmentBody_=255; attachmentLodCounts_={};
 const float sky[]={.19f,.23f,.38f,1};
 context_->OMSetRenderTargets(1,target_.GetAddressOf(),depth_.Get());
 context_->ClearRenderTargetView(target_.Get(),sky);
 context_->ClearDepthStencilView(depth_.Get(),D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);
 D3D11_VIEWPORT v{0,0,static_cast<float>(width_),static_cast<float>(height_),0,1};
 context_->RSSetViewports(1,&v); context_->RSSetState(rasterizer_.Get());
 context_->VSSetConstantBuffers(0,1,constants_.GetAddressOf()); context_->PSSetConstantBuffers(0,1,constants_.GetAddressOf());
 // A launcher/roster can render a portrait before any world. Bind valid
 // resources even though portrait shaders bypass atmospheric lighting.
 context_->VSSetConstantBuffers(3,1,cloudLightConstants_.GetAddressOf());
 context_->VSSetShaderResources(8,1,cloudLightView_.GetAddressOf());
 context_->VSSetSamplers(2,1,fontSampler_.GetAddressOf());
}
void Renderer::DrawSky() {
 // SetCamera can arrive after BeginFrame (including capture camera overrides).
 // Opaque geometry fills depth first, rejecting hidden sky pixels before the
 // expensive cloud shader. Finish before effects/HUD or a portrait changes it.
 if (!skyPending_) return;
 skyPending_=false;
 context_->OMSetRenderTargets(1,target_.GetAddressOf(),depth_.Get());
 D3D11_VIEWPORT viewport{0,0,float(width_),float(height_),0,1};
 context_->RSSetViewports(1,&viewport); context_->RSSetState(rasterizer_.Get());
 context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
 Constants data{}; data.atmosphere.x=atmospherePhase_;
 XMStoreFloat4x4(&data.world,XMMatrixTranspose(XMMatrixInverse(nullptr,XMLoadFloat4x4(&viewProjection_))));
 data.eyeAndHud={eye_.x,eye_.y,eye_.z,0};
 context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
 context_->IASetInputLayout(nullptr); context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 context_->VSSetShader(skyShader_.Get(),nullptr,0); context_->PSSetShader(skyPixel_.Get(),nullptr,0);
 context_->OMSetDepthStencilState(effectDepth_.Get(),0);
 context_->Draw(3,0); ++drawCalls_; ++triangles_;
}
void Renderer::SetAtmosphereTime(double seconds) {
 if (portrait_ || !std::isfinite(seconds) || seconds<0) return;
 // A closed drift path avoids accumulated float error and a visible wrap seam.
 atmospherePhase_=float(std::fmod(seconds,1800.0)*(6.283185307179586/1800.0));
}
void Renderer::PrepareCloudLight() {
 const float extent=float(cloudLightSize); // Two metres per texel.
 const XMFLOAT4 field{cloudLightFieldEnabled_?extent:0.f,extent-1,0,0};
 context_->UpdateSubresource(cloudLightConstants_.Get(),0,nullptr,&field,0,0);
 context_->VSSetConstantBuffers(3,1,cloudLightConstants_.GetAddressOf());
 context_->PSSetConstantBuffers(3,1,cloudLightConstants_.GetAddressOf());
 if (cloudLightFieldEnabled_ && cloudLightPhase_!=atmospherePhase_) {
  // Unbind before making this SRV an output. The field depends only on phase,
  // not camera, viewport, shadow focus or actor pose. No readback or lag.
  ID3D11ShaderResourceView* empty=nullptr;
  context_->VSSetShaderResources(8,1,&empty);
  context_->OMSetRenderTargets(1,cloudLightTarget_.GetAddressOf(),nullptr);
  context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
  context_->OMSetDepthStencilState(depthOff_.Get(),0);
  context_->RSSetState(rasterizer_.Get());
  D3D11_VIEWPORT viewport{0,0,float(cloudLightSize),float(cloudLightSize),0,1};
  context_->RSSetViewports(1,&viewport);
  Constants data{}; data.atmosphere.x=atmospherePhase_;
  context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
  context_->IASetInputLayout(nullptr);
  context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context_->VSSetShader(skyShader_.Get(),nullptr,0);
  context_->PSSetShader(cloudLightShader_.Get(),nullptr,0);
  context_->Draw(3,0); ++drawCalls_; ++triangles_; ++cloudLightUpdates_;
  context_->OMSetRenderTargets(0,nullptr,nullptr);
  cloudLightPhase_=atmospherePhase_;
 }
 context_->VSSetShaderResources(8,1,cloudLightView_.GetAddressOf());
 context_->VSSetSamplers(2,1,fontSampler_.GetAddressOf());
}
void Renderer::SetCamera(const XMFLOAT3& eye,const XMFLOAT3& target) {
 eye_=eye;
 auto view=XMMatrixLookAtLH(XMLoadFloat3(&eye),XMLoadFloat3(&target),XMVectorSet(0,1,0,0));
 auto projection=XMMatrixPerspectiveFovLH(XM_PIDIV4,static_cast<float>(width_)/height_,.1f,160.f);
 XMStoreFloat4x4(&viewProjection_,view*projection);
 if (!portrait_) SetShadowFocus(target);
}
void Renderer::SetShadowFocus(const XMFLOAT3& focus) {
 if (portrait_ || !std::isfinite(focus.x) || !std::isfinite(focus.y) || !std::isfinite(focus.z)) return;
 shadowProjection_=visual::ShadowProjections(focus);
}
bool Renderer::Project(const XMFLOAT3& world,XMFLOAT2& screen) const {
 XMFLOAT4 clip;
 XMStoreFloat4(&clip,XMVector4Transform(XMVectorSet(world.x,world.y,world.z,1),XMLoadFloat4x4(&viewProjection_)));
 if (clip.w<=0 || clip.z<0 || clip.z>clip.w || clip.x<-clip.w || clip.x>clip.w || clip.y<-clip.w || clip.y>clip.w)
  return false;
 screen={(clip.x/clip.w+1)*.5f,(1-clip.y/clip.w)*.5f};
 return true;
}
void Renderer::Draw(const XMMATRIX& world,const XMMATRIX& vp,const XMFLOAT4& color,bool hud,UINT count,UINT start) {
 ++drawCalls_;
 triangles_+=count/3;
 Constants data{}; data.atmosphere.x=atmospherePhase_; XMStoreFloat4x4(&data.world,XMMatrixTranspose(world)); XMStoreFloat4x4(&data.wvp,XMMatrixTranspose(world*vp));
 const auto normal=XMMatrixTranspose(XMMatrixInverse(nullptr,world));
 XMStoreFloat4x4(&data.normalWorld,XMMatrixTranspose(normal));
 data.color=color; data.eyeAndHud={eye_.x,eye_.y,eye_.z,hud?1.f:0.f};
 context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
 context_->OMSetDepthStencilState(hud?depthOff_.Get():depthOn_.Get(),0); context_->Draw(count,start);
}
void Renderer::DrawMarkerBox(const XMFLOAT3& p,const XMFLOAT3& s,const XMFLOAT4& color,float yaw) {
 DrawMesh(visual::Mesh::Box,p,s,color,visual::Material::Plain,yaw,0,visual::ShadowMode::None);
}
void Renderer::Queue(visual::Mesh mesh,const XMMATRIX& transform,const XMFLOAT4& color,visual::Material material,visual::ShadowMode shadow,unsigned maxDetail) {
 // Private callers construct scaled orthonormal bases, never sheared matrices.
 Item item{};
 item.body=attachmentBody_; item.maxDetail=std::uint8_t(std::min(maxDetail,2u));
 XMStoreFloat4x4(&item.instance.world,transform);
 const auto prepared=visual::PrepareInstanceTransform(transform);
 XMStoreFloat4x4(&item.instance.normal,prepared.normal);
 item.instance.color=color; item.instance.surface={static_cast<float>(material),shadow==visual::ShadowMode::Cast?1.f:0.f,0,0};
 // A face is smaller than the PCF footprint: extrapolating each tiny triangle's
 // receiver plane across the curved nose produces triangular self-shadow marks.
 if(visual::IsHeadMesh(mesh)) item.instance.surface.w=-1;
 const auto& m=item.instance.world; item.center={m._41,m._42,m._43};
 item.radius=meshes_[static_cast<unsigned>(mesh)].radius*prepared.radiusScale;
 world_[static_cast<unsigned>(mesh)].push_back(item);
}
void Renderer::DrawMesh(visual::Mesh mesh,const XMFLOAT3& p,const XMFLOAT3& s,const XMFLOAT4& color,
 visual::Material material,float yaw,float pitch,visual::ShadowMode shadow,float roll,unsigned maxDetail) {
 if (s.x<=0 || s.y<=0 || s.z<=0) return;
 Queue(mesh,XMMatrixScaling(s.x,s.y,s.z)*XMMatrixRotationX(pitch)*XMMatrixRotationZ(roll)*XMMatrixRotationY(yaw)*
  XMMatrixTranslation(p.x,p.y,p.z),color,material,shadow,maxDetail);
}
void Renderer::DrawCloak(const XMFLOAT3& shoulder,float yaw,float pitch,float roll,float side,float back) {
 DrawMesh(visual::Mesh::Cloak,shoulder,{1,1,1},{1,1,1,1},visual::Material::Cloth,yaw,pitch,visual::ShadowMode::Cast,roll);
 auto& item=world_[static_cast<unsigned>(visual::Mesh::Cloak)].back();
 item.instance.surface.z=std::isfinite(side)?std::clamp(side,-.20f,.20f):0;
 // Positive w identifies cloth. Both shadow and color use the same deformation.
 item.instance.surface.w=.001f+(std::isfinite(back)?std::clamp(back,0.f,.35f):0);
 item.radius+=.41f;
}
unsigned Renderer::DrawBody(XMFLOAT3 position,float yaw,const visual::BodyPalette& palette,std::uint64_t actor) {
 if(bodyCount_>=visual::maxBodies) { error_=L"Scene exceeded the 32 skinned-body budget."; frameFailed_=true; return visual::maxBodies; }
 auto& gpu=bodyPalettes_[bodyCount_]; gpu.colors=palette.colors;
 bodyActors_[bodyCount_]=actor;
 for(unsigned bone=0;bone<visual::bodyBones;++bone) {
  XMStoreFloat4x4(&gpu.joints[bone],XMMatrixTranspose(XMLoadFloat4x4(&palette.joints[bone])));
  XMStoreFloat4x4(&gpu.normals[bone],XMMatrixTranspose(XMLoadFloat4x4(&palette.normals[bone])));
 }
 DrawMesh(visual::Mesh::Body,position,{1,1,1},{1,1,1,1},visual::Material::Cloth,yaw);
 auto& item=world_[unsigned(visual::Mesh::Body)].back(); item.instance.surface.z=float(bodyCount_++);
 // Covers every solved limb/torso pose, including fully raised arms. Bounds are
 // intentionally conservative; the simulation's collision body is unchanged.
 item.radius=3.f;
 item.body=255;
 return bodyCount_-1;
}
void Renderer::DrawHand(const XMMATRIX& transform,const visual::HandPalette& palette,bool mirrored,const XMFLOAT4& color,visual::Material material) {
 if(handCount_>=visual::maxHands) { error_=L"Scene exceeded the 64 skinned-hand budget."; frameFailed_=true; return; }
 auto& gpu=handPalettes_[handCount_];
 for(unsigned bone=0;bone<visual::handBones;++bone)
  XMStoreFloat4x4(&gpu.joints[bone],XMMatrixTranspose(XMLoadFloat4x4(&palette.joints[bone])));
 const auto mesh=mirrored?visual::Mesh::HandMirrored:visual::Mesh::Hand;
 Queue(mesh,transform,color,material,visual::ShadowMode::Cast);
 auto& item=world_[unsigned(mesh)].back(); item.instance.surface.z=float(handCount_++);
 // Finger triangles are much smaller than a PCF footprint. Avoid projecting
 // their receiver planes across neighboring fingers, while retaining shade.
 item.instance.surface.w=-2;
 // Covers opened and curled fingers; hand frames are rigid, in world metres.
 item.radius=.23f;
}
void Renderer::DrawSegment(const XMFLOAT3& from,const XMFLOAT3& to,float width,float depth,
 const XMFLOAT4& color,visual::Material material) {
 DrawMeshBetween(visual::Mesh::Limb,from,to,width,depth,color,material);
}
void Renderer::DrawMeshBetween(visual::Mesh mesh,const XMFLOAT3& from,const XMFLOAT3& to,float width,float depth,
 const XMFLOAT4& color,visual::Material material,XMFLOAT3 facing) {
 const auto delta=XMLoadFloat3(&to)-XMLoadFloat3(&from);
 const float length=XMVectorGetX(XMVector3Length(delta));
 if (length<.001f || width<=0 || depth<=0) return;
 const auto up=delta/length;
 auto right=XMVector3Cross(up,XMLoadFloat3(&facing));
 if (XMVectorGetX(XMVector3LengthSq(right))<.0001f)
   right=XMVector3Cross(up,std::abs(XMVectorGetX(up))<.9f?XMVectorSet(1,0,0,0):XMVectorSet(0,1,0,0));
 right=XMVector3Normalize(right);
 const auto forward=XMVector3Cross(right,up);
 const XMMATRIX basis(right,up,forward,XMVectorSet(0,0,0,1));
 Queue(mesh,XMMatrixScaling(width,length,depth)*basis*
  XMMatrixTranslation((from.x+to.x)*.5f,(from.y+to.y)*.5f,(from.z+to.z)*.5f),color,material);
}
void Renderer::DrawStaticScene(visual::StaticScene& scene,const std::function<void()>& emit) {
 if (!scene.ready_) {
  std::array<std::size_t,meshCount> starts{};
  for (unsigned mesh=0;mesh<meshCount;++mesh) starts[mesh]=world_[mesh].size();
  emit();
  for (unsigned mesh=0;mesh<meshCount;++mesh)
   scene.batches_[mesh].assign(world_[mesh].begin()+starts[mesh],world_[mesh].end());
  scene.ready_=true;
  return; // The initial draws are already queued, exactly once.
 }
 for (unsigned mesh=0;mesh<meshCount;++mesh) {
  const auto& batch=scene.batches_[mesh];
  world_[mesh].insert(world_[mesh].end(),batch.begin(),batch.end());
 }
}
bool Renderer::FlushWorld() {
 if (std::all_of(world_.begin(),world_.end(),[](const auto& batch) { return batch.empty(); })) {
  DrawSky(); return true;
 }
 // Choose only once the final camera is known: captures can override it after
 // queueing. A body's color and both shadow passes share the selected mesh.
 auto& bodies=world_[unsigned(visual::Mesh::Body)]; std::size_t kept=0;
 std::array<unsigned,visual::maxBodies> bodyLevels{};
 const auto& camera=viewProjection_;
 const float projectionScale=std::sqrt(camera._12*camera._12+camera._22*camera._22+camera._32*camera._32);
 for(const auto& item : bodies) {
  const auto p=item.center;
  const float depth=p.x*camera._14+(p.y+.9f)*camera._24+p.z*camera._34+camera._44;
  // Behind-eye casters still affect the shadow maps. Use radial distance there
  // instead of promoting every distant, offscreen body to maximum detail.
  const float distance=depth>.1f?depth:std::sqrt((p.x-eye_.x)*(p.x-eye_.x)+(p.y+.9f-eye_.y)*(p.y+.9f-eye_.y)+(p.z-eye_.z)*(p.z-eye_.z));
  const float pixels=1.8f*projectionScale*height_*.5f/std::max(.1f,distance);
  const auto level=portrait_ || !bodyLodEnabled_?0:bodyLodHistory_.Select(bodyActors_[unsigned(item.instance.surface.z)],pixels);
  bodyLevels[unsigned(item.instance.surface.z)]=level;
  if(!portrait_) ++bodyLodCounts_[level];
  if(level) world_[unsigned(visual::Mesh::Body)+level].push_back(item);
  else bodies[kept++]=item;
 }
 bodies.resize(kept);
 // Resolve attachments from the same owner decision, before any color/shadow
 // culling. Scenery has no owner, even when it reuses a character mesh shape.
 if(!portrait_ && attachmentLodEnabled_) for(unsigned mesh=0;mesh<meshCount;++mesh) {
  auto& batch=world_[mesh]; std::size_t retained=0;
  for(std::size_t i=0;i<batch.size();++i) {
   const auto& item=batch[i];
   const auto level=item.body<bodyCount_?bodyLevels[item.body]:0;
   if(level>item.maxDetail) { ++attachmentLodCounts_[1]; continue; }
   const auto replacement=visual::AttachmentMesh(visual::Mesh(mesh),level);
   if(unsigned(replacement)!=mesh) {
    // Keep bounds conservative even if a reduced ring samples a larger radius.
    auto reduced=item;
    reduced.radius*=std::max(1.f,meshes_[unsigned(replacement)].radius/std::max(.0001f,meshes_[mesh].radius));
    reduced.body=255; world_[unsigned(replacement)].push_back(reduced); ++attachmentLodCounts_[0];
   } else {
    if(retained!=i) batch[retained]=item;
    ++retained;
   }
  }
  batch.resize(retained);
 }
 struct Batch { unsigned mesh; UINT first,count; };
 constexpr unsigned colorPass=visual::shadowLayers,passCount=colorPass+1;
 // This mask order gives each pass at most two contiguous ranges per mesh.
 // A shared object occupies one upload slot, regardless of how many passes use it.
 constexpr unsigned maskOrder[]={1,3,7,5,4,6,2};
 static_assert(passCount==3,"Visibility ordering assumes two shadow passes and one color pass.");
 std::array<std::array<Batch,meshCount*2>,passCount> passes{};
 std::array<unsigned,passCount> batchCounts{};
 const std::array<visual::Frustum,passCount> frusta={visual::Frustum(shadowProjection_[0]),
   visual::Frustum(shadowProjection_[1]),visual::Frustum(viewProjection_)};
 instanceScratch_.clear();
 for (unsigned mesh=0;mesh<meshCount;++mesh) {
  std::array<UINT,8> counts{},offsets{};
  for (auto& item : world_[mesh]) {
   item.visibility=0;
   for (unsigned pass=portrait_?colorPass:0u;pass<passCount;++pass) {
    if (pass<colorPass && item.instance.surface.y==0) continue;
    if (frusta[pass].VisibleSphere(item.center,item.radius))
      item.visibility|=1u<<pass;
   }
   ++counts[item.visibility];
  }
  UINT end=static_cast<UINT>(instanceScratch_.size());
  for (unsigned mask : maskOrder) {
   const UINT start=end,count=counts[mask]; offsets[mask]=start;
   if (count>maxInstances-end) { error_=L"Scene exceeded the 4096 visible-instance budget."; return false; }
   end+=count;
   if (!count) continue;
   for (unsigned pass=0;pass<passCount;++pass) if (mask&(1u<<pass)) {
    auto& used=batchCounts[pass];
    if (used && passes[pass][used-1].mesh==mesh && passes[pass][used-1].first+passes[pass][used-1].count==start)
      passes[pass][used-1].count+=count;
    else passes[pass][used++]={mesh,start,count};
   }
  }
  instanceScratch_.resize(end);
  // Stable counting scatter preserves the old mask/mesh draw order in one scan.
  for (const auto& item : world_[mesh]) if (item.visibility)
    instanceScratch_[offsets[item.visibility]++]=item.instance;
 }
 // Shadow-only/fully culled scenes do not consume surface cloud lighting.
 if (!portrait_ && batchCounts[colorPass]) PrepareCloudLight();
 if (!instanceScratch_.empty()) {
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (!Check(context_->Map(instances_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),L"Upload scene instances")) return false;
  std::memcpy(mapped.pData,instanceScratch_.data(),instanceScratch_.size()*sizeof(Instance));
  context_->Unmap(instances_.Get(),0);
 }
 if(bodyCount_) {
  // A portrait can follow world draws immediately. DISCARD lets the driver
  // rename the palette instead of waiting for the world's bones to finish.
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if(!Check(context_->Map(skinConstants_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),L"Upload skin palettes")) return false;
  std::memcpy(mapped.pData,bodyPalettes_.data(),bodyCount_*sizeof(visual::BodyPalette));
  context_->Unmap(skinConstants_.Get(),0);
 }
 context_->VSSetConstantBuffers(1,1,skinConstants_.GetAddressOf());
 if(handCount_) {
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if(!Check(context_->Map(handConstants_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),L"Upload hand palettes")) return false;
  std::memcpy(mapped.pData,handPalettes_.data(),handCount_*sizeof(visual::HandPalette));
  context_->Unmap(handConstants_.Get(),0);
 }
 context_->VSSetConstantBuffers(2,1,handConstants_.GetAddressOf());
 ID3D11Buffer* buffers[]={vertices_.Get(),instances_.Get(),skinWeights_.Get()};
 const UINT strides[]={sizeof(Vertex),sizeof(Instance),sizeof(visual::SkinInfluence)},offsets[]={0,0,0};
 context_->IASetVertexBuffers(0,3,buffers,strides,offsets);
 context_->IASetIndexBuffer(indices_.Get(),DXGI_FORMAT_R32_UINT,0);
 context_->IASetInputLayout(worldLayout_.Get());
 context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 context_->OMSetDepthStencilState(depthOn_.Get(),0);
 context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
 ID3D11ShaderResourceView* nullView=nullptr; context_->PSSetShaderResources(1,1,&nullView);
 for (unsigned pass=portrait_?colorPass:0u;pass<passCount;++pass) {
  const bool shadowPass=pass<colorPass;
  if (shadowPass) {
   context_->OMSetRenderTargets(0,nullptr,shadowDepth_[pass].Get());
   context_->ClearDepthStencilView(shadowDepth_[pass].Get(),D3D11_CLEAR_DEPTH,1,0);
  }
  Constants data{}; data.atmosphere.x=atmospherePhase_; XMStoreFloat4x4(&data.wvp,XMMatrixTranspose(XMLoadFloat4x4(shadowPass?&shadowProjection_[pass]:&viewProjection_)));
  for (unsigned layer=0;layer<visual::shadowLayers;++layer)
   XMStoreFloat4x4(&data.shadowVP[layer],XMMatrixTranspose(XMLoadFloat4x4(&shadowProjection_[layer])));
  data.color={1,1,1,1}; data.eyeAndHud={eye_.x,eye_.y,eye_.z,portrait_?-1.f:0.f};
  context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
  D3D11_VIEWPORT viewport{0,0,shadowPass?float(visual::shadowSize):float(width_),shadowPass?float(visual::shadowSize):float(height_),0,1};
  if (portrait_) viewport=portraitViewport_;
  context_->RSSetViewports(1,&viewport);
  context_->RSSetState(shadowPass?shadowRasterizer_.Get():rasterizer_.Get());
  context_->IASetInputLayout(worldLayout_.Get());
  context_->VSSetShader(shadowPass?shadowShader_.Get():worldShader_.Get(),nullptr,0);
  context_->PSSetShader(shadowPass?nullptr:pixelShader_.Get(),nullptr,0);
  if (!shadowPass) {
   context_->OMSetRenderTargets(1,target_.GetAddressOf(),depth_.Get());
   ID3D11ShaderResourceView* views[]={materialView_.Get(),shadowView_.Get()};
   ID3D11SamplerState* samplers[]={surfaceSampler_.Get(),shadowSampler_.Get()};
   context_->PSSetShaderResources(0,2,views); context_->PSSetSamplers(0,2,samplers);
   context_->PSSetShaderResources(4,1,terrainView_.GetAddressOf());
   context_->PSSetSamplers(2,1,fontSampler_.GetAddressOf());
  }
  unsigned skinActive=0;
  for (unsigned b=0;b<batchCounts[pass];++b) {
   const auto& batch=passes[pass][b];
   const auto& mesh=meshes_[batch.mesh];
   const unsigned skin=visual::IsBodyMesh(static_cast<visual::Mesh>(batch.mesh))?1:visual::IsHandMesh(static_cast<visual::Mesh>(batch.mesh))?2:0;
   if(skin!=skinActive) {
    context_->IASetInputLayout(skin?bodyLayout_.Get():worldLayout_.Get());
    context_->VSSetShader(skin==2?(shadowPass?handShadowShader_.Get():handShader_.Get()):
      skin==1?(shadowPass?bodyShadowShader_.Get():bodyShader_.Get()):(shadowPass?shadowShader_.Get():worldShader_.Get()),nullptr,0);
    skinActive=skin;
   }
   context_->DrawIndexedInstanced(mesh.count,batch.count,mesh.start,0,batch.first);
   ++drawCalls_; triangles_+=mesh.count/3*batch.count;
  }
 }
 // Consume even a fully culled queue before changing camera/portrait state.
 for (auto& batch : world_) batch.clear();
 bodyCount_=handCount_=0;
 attachmentBody_=255;
 DrawSky();
 return true;
}
void Renderer::EffectQuad(XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 d,XMFLOAT4 color,float startAlpha,float endAlpha,float radial) {
 // Cosmetic saturation drops the newest quad, never grows an unbounded queue.
 if (portrait_ || color.w<=0 || effects_.size()+6+effectVertices_>maxEffectVertices) return;
 const XMFLOAT4 first{color.x,color.y,color.z,color.w*std::clamp(startAlpha,0.f,1.f)};
 const XMFLOAT4 last{color.x,color.y,color.z,color.w*std::clamp(endAlpha,0.f,1.f)};
 const Vertex va{a,{0,0,radial},first},vb{b,{0,1,radial},first},vc{c,{1,1,radial},last},vd{d,{1,0,radial},last};
 effects_.insert(effects_.end(),{va,vb,vc,va,vc,vd});
}
void Renderer::DrawEffectRibbon(XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 d,XMFLOAT4 color,float startAlpha,float endAlpha) {
 EffectQuad(a,b,c,d,color,startAlpha,endAlpha,0);
}
void Renderer::DrawEffectGlow(XMFLOAT3 center,float radius,XMFLOAT4 color) {
 EffectBillboard(center,radius,color,1);
}
void Renderer::DrawEffectRing(XMFLOAT3 center,float radius,XMFLOAT4 color) {
 EffectBillboard(center,radius,color,2);
}
void Renderer::EffectBillboard(XMFLOAT3 center,float radius,XMFLOAT4 color,float shape) {
 if (radius<=0 || color.w<=0) return;
 auto forward=XMLoadFloat3(&eye_)-XMLoadFloat3(&center);
 if (XMVectorGetX(XMVector3LengthSq(forward))<.00001f) return;
 forward=XMVector3Normalize(forward);
 const auto axis=std::abs(XMVectorGetY(forward))>.99f?XMVectorSet(1,0,0,0):XMVectorSet(0,1,0,0);
 const auto right=XMVector3Normalize(XMVector3Cross(axis,forward))*radius;
 const auto up=XMVector3Normalize(XMVector3Cross(forward,right))*radius;
 const auto origin=XMLoadFloat3(&center);
 XMFLOAT3 a,b,c,d;
 XMStoreFloat3(&a,origin-right-up); XMStoreFloat3(&b,origin-right+up);
 XMStoreFloat3(&c,origin+right+up); XMStoreFloat3(&d,origin+right-up);
 EffectQuad(a,b,c,d,color,1,1,shape);
}
void Renderer::DrawEffectStreak(XMFLOAT3 from,XMFLOAT3 to,float width,XMFLOAT4 color) {
 if (width<=0 || color.w<=0) return;
 const auto start=XMLoadFloat3(&from),finish=XMLoadFloat3(&to),direction=finish-start;
 auto side=XMVector3Cross(direction,XMLoadFloat3(&eye_)-(start+finish)*.5f);
 if (XMVectorGetX(XMVector3LengthSq(side))<1e-10f) { DrawEffectGlow(to,width,color); return; }
 side=XMVector3Normalize(side)*width;
 XMFLOAT3 a,b,c,d;
 XMStoreFloat3(&a,start-side); XMStoreFloat3(&b,start+side);
 XMStoreFloat3(&c,finish+side); XMStoreFloat3(&d,finish-side);
 EffectQuad(a,b,c,d,color,1,1,3);
}
bool Renderer::FlushEffects() {
 if (effects_.empty()) return true;
 const UINT stride=sizeof(Vertex),offset=0,count=static_cast<UINT>(effects_.size());
 D3D11_MAPPED_SUBRESOURCE mapped{};
 if (!Check(context_->Map(hudVertices_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),L"Upload combat effects")) return false;
 std::memcpy(mapped.pData,effects_.data(),count*sizeof(Vertex)); context_->Unmap(hudVertices_.Get(),0);
 Constants data{}; data.atmosphere.x=atmospherePhase_; XMStoreFloat4x4(&data.wvp,XMMatrixTranspose(XMLoadFloat4x4(&viewProjection_)));
 data.eyeAndHud={eye_.x,eye_.y,eye_.z,0}; context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
 context_->IASetVertexBuffers(0,1,hudVertices_.GetAddressOf(),&stride,&offset);
 context_->IASetInputLayout(layout_.Get()); context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 context_->VSSetShader(effectVertexShader_.Get(),nullptr,0); context_->PSSetShader(effectPixelShader_.Get(),nullptr,0);
 context_->RSSetState(effectRasterizer_.Get()); context_->OMSetDepthStencilState(effectDepth_.Get(),0);
 context_->OMSetBlendState(effectBlend_.Get(),nullptr,0xffffffff);
 context_->Draw(count,0); ++drawCalls_; triangles_+=count/3; effectVertices_+=count;
 context_->RSSetState(rasterizer_.Get()); context_->OMSetBlendState(nullptr,nullptr,0xffffffff);
 effects_.clear(); return true;
}
void Renderer::DrawHudTriangle(XMFLOAT2 a,XMFLOAT2 b,XMFLOAT2 c,const XMFLOAT4& color) {
 // A consistent screen-space winding keeps all authored UI polygons visible.
 if ((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)<0) std::swap(b,c);
 for (auto p : {a,b,c}) hud_.push_back({{p.x*2-1,1-p.y*2,.5f},{0,0,0},color});
}
void Renderer::DrawHudRect(float x,float y,float width,float height,const XMFLOAT4& color) {
 if (width<=0 || height<=0) return;
 DrawHudTriangle({x,y},{x+width,y},{x+width,y+height},color);
 DrawHudTriangle({x,y},{x+width,y+height},{x,y+height},color);
}
void Renderer::DrawHudGradient(float x,float y,float width,float height,XMFLOAT4 top,XMFLOAT4 bottom) {
 if(width<=0 || height<=0) return;
 const Vertex a{{x*2-1,1-y*2,.5f},{0,0,0},top},b{{(x+width)*2-1,1-y*2,.5f},{0,0,0},top};
 const Vertex c{{(x+width)*2-1,1-(y+height)*2,.5f},{0,0,0},bottom},d{{x*2-1,1-(y+height)*2,.5f},{0,0,0},bottom};
 hud_.insert(hud_.end(),{a,b,c,a,c,d});
}
void Renderer::DrawUiArt(UiArt art,float x,float y,float width,float height,XMFLOAT4 uv,XMFLOAT4 tint) {
 if (width<=0 || height<=0) return;
 const float texture=3.f+static_cast<float>(art);
 const Vertex a{{x*2-1,1-y*2,.5f},{uv.x,uv.y,texture},tint};
 const Vertex b{{(x+width)*2-1,1-y*2,.5f},{uv.z,uv.y,texture},tint};
 const Vertex c{{(x+width)*2-1,1-(y+height)*2,.5f},{uv.z,uv.w,texture},tint};
 const Vertex d{{x*2-1,1-(y+height)*2,.5f},{uv.x,uv.w,texture},tint};
 hud_.insert(hud_.end(),{a,b,c,a,c,d});
}
bool Renderer::CreateUiArtwork() {
 // Resource-backed PNGs travel with the executable. Decode once, bounded to
 // under 18 MiB including mipmaps; WIC objects/pixels are released immediately.
 const HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 if (FAILED(apartment) && apartment!=RPC_E_CHANGED_MODE) return Check(apartment,L"Initialize UI decoder apartment");
 struct Apartment {bool owned;~Apartment(){if(owned) CoUninitialize();}} scope{SUCCEEDED(apartment)};
 ComPtr<IWICImagingFactory> factory;
 if (!Check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.GetAddressOf())),L"Create UI image decoder")) return false;
 uiBytes_=0;
 for (unsigned i=0;i<uiViews_.size();++i) {
  const auto module=GetModuleHandleW(nullptr);
  const auto resource=FindResourceW(module,MAKEINTRESOURCEW(201+i),RT_RCDATA);
  const auto loaded=resource?LoadResource(module,resource):nullptr;
  auto* bytes=loaded?static_cast<BYTE*>(LockResource(loaded)):nullptr;
  if (!bytes) {error_=L"Embedded interface artwork is missing. Rebuild the complete executable.";return false;}
  ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;
  ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
  const UINT width=i==1?1536:1024,height=i==1?864:1024;
  if (!Check(factory->CreateStream(stream.GetAddressOf()),L"Create UI stream") ||
      !Check(stream->InitializeFromMemory(bytes,SizeofResource(module,resource)),L"Read UI resource") ||
      !Check(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,decoder.GetAddressOf()),L"Decode UI artwork") ||
      !Check(decoder->GetFrame(0,frame.GetAddressOf()),L"Read UI artwork frame") ||
      !Check(factory->CreateBitmapScaler(scaler.GetAddressOf()),L"Create UI scaler") ||
      !Check(scaler->Initialize(frame.Get(),width,height,WICBitmapInterpolationModeFant),L"Scale UI artwork") ||
      !Check(factory->CreateFormatConverter(converter.GetAddressOf()),L"Create UI converter") ||
      !Check(converter->Initialize(scaler.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom),L"Convert UI artwork")) return false;
  std::vector<BYTE> pixels(std::size_t(width)*height*4);
  if (!Check(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data()),L"Read UI pixels")) return false;
  D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.ArraySize=1;
  desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
  desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;desc.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
  ComPtr<ID3D11Texture2D> texture;
  if (!Check(device_->CreateTexture2D(&desc,nullptr,texture.GetAddressOf()),L"Create UI texture") ||
      !Check(device_->CreateShaderResourceView(texture.Get(),nullptr,uiViews_[i].ReleaseAndGetAddressOf()),L"Create UI texture view")) return false;
  context_->UpdateSubresource(texture.Get(),0,nullptr,pixels.data(),width*4,0);
  context_->GenerateMips(uiViews_[i].Get());
  for(UINT w=width,h=height;;w=std::max(1u,w/2),h=std::max(1u,h/2)) {
   uiBytes_+=std::size_t(w)*h*4;if(w==1 && h==1) break;
  }
 }
 return true;
}
bool Renderer::FlushHud() {
 if (!FlushWorld() || !FlushEffects()) return false;
 if (hud_.empty()) return true;
 const UINT stride=sizeof(Vertex),offset=0;
 context_->IASetVertexBuffers(0,1,hudVertices_.GetAddressOf(),&stride,&offset);
 context_->IASetInputLayout(layout_.Get());
 context_->VSSetShader(vertexShader_.Get(),nullptr,0); context_->PSSetShader(pixelShader_.Get(),nullptr,0);
 context_->OMSetBlendState(hudBlend_.Get(),nullptr,0xffffffff);
 auto* atlas=font_.View(); context_->PSSetShaderResources(2,1,&atlas);
 context_->PSSetShaderResources(3,1,mapView_.GetAddressOf());
 ID3D11ShaderResourceView* art[]={uiViews_[0].Get(),uiViews_[1].Get(),uiViews_[2].Get()};
 context_->PSSetShaderResources(5,3,art);
 context_->PSSetSamplers(2,1,fontSampler_.GetAddressOf());
 for (std::size_t start=0;start<hud_.size();start+=16380) {
  const UINT count=static_cast<UINT>(std::min<std::size_t>(16380,hud_.size()-start));
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (!Check(context_->Map(hudVertices_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),L"Upload HUD geometry")) return false;
  std::memcpy(mapped.pData,hud_.data()+start,count*sizeof(Vertex)); context_->Unmap(hudVertices_.Get(),0);
  Draw(XMMatrixIdentity(),XMMatrixIdentity(),{1,1,1,1},true,count);
 }
 hud_.clear(); return true;
}
bool Renderer::BeginPortrait(float x,float y,float width,float height,float closeup) {
 if (!FlushHud()) { frameFailed_=true; return false; }
 savedCamera_=viewProjection_; savedEye_=eye_; portrait_=true;
 portraitViewport_={x*width_,y*height_,width*width_,height*height_,0,1};
 closeup=std::clamp(closeup,0.f,1.f);
 eye_={0,1.1f+.58f*closeup,4.5f-3.15f*closeup};
 const auto view=XMMatrixLookAtLH(XMLoadFloat3(&eye_),XMVectorSet(0,1.05f+.63f*closeup,0,1),XMVectorSet(0,1,0,0));
 XMStoreFloat4x4(&viewProjection_,view*XMMatrixPerspectiveFovLH(.57f,portraitViewport_.Width/portraitViewport_.Height,.1f,20.f));
 context_->ClearDepthStencilView(depth_.Get(),D3D11_CLEAR_DEPTH,1,0);
 return true;
}
bool Renderer::EndPortrait() {
 const bool success=FlushWorld(); frameFailed_=frameFailed_ || !success;
 portrait_=false; viewProjection_=savedCamera_; eye_=savedEye_;
 D3D11_VIEWPORT viewport{0,0,float(width_),float(height_),0,1}; context_->RSSetViewports(1,&viewport);
 return success;
}
bool Renderer::EndFrame(bool vsync) {
 if (frameFailed_) return false;
 if (!FlushHud()) return false;
 lastDrawCalls_=drawCalls_;
 lastBodyLodCounts_=bodyLodCounts_;
 lastAttachmentLodCounts_=attachmentLodCounts_;
 gpuTimer_.End(context_.Get());
 return Check(swapChain_->Present(vsync?1:0,0),L"Present frame");
}

bool Renderer::Capture(const std::wstring& path) {
 if (!FlushHud()) return false;
 ComPtr<ID3D11Texture2D> back,staging;
 if (!Check(swapChain_->GetBuffer(0,IID_PPV_ARGS(back.GetAddressOf())),L"Get capture buffer")) return false;
 D3D11_TEXTURE2D_DESC desc{}; back->GetDesc(&desc);
 desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=0; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ; desc.MiscFlags=0;
 if (!Check(device_->CreateTexture2D(&desc,nullptr,staging.GetAddressOf()),L"Create capture buffer")) return false;
 context_->CopyResource(staging.Get(),back.Get());
 D3D11_MAPPED_SUBRESOURCE mapped{};
 if (!Check(context_->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped),L"Read capture buffer")) return false;
 std::vector<BYTE> pixels(static_cast<std::size_t>(width_)*height_*4);
 for (int row=0;row<height_;++row)
  std::memcpy(pixels.data()+static_cast<std::size_t>(row)*width_*4,
   static_cast<const BYTE*>(mapped.pData)+static_cast<std::size_t>(row)*mapped.RowPitch,width_*4);
 context_->Unmap(staging.Get(),0);
 for (std::size_t i=0;i<pixels.size();i+=4) std::swap(pixels[i],pixels[i+2]);
 const HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 if (FAILED(initialized) && initialized!=RPC_E_CHANGED_MODE) return Check(initialized,L"Initialize image encoder");
 const auto encode=[&]() {
  ComPtr<IWICImagingFactory> factory; ComPtr<IWICStream> stream;
  ComPtr<IWICBitmapEncoder> encoder; ComPtr<IWICBitmapFrameEncode> frame;
  ComPtr<IPropertyBag2> properties;
  if (!Check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.GetAddressOf())),L"Create image encoder")) return false;
  if (!Check(factory->CreateStream(stream.GetAddressOf()),L"Create image stream") ||
      !Check(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE),L"Open capture output")) return false;
  if (!Check(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.GetAddressOf()),L"Create PNG encoder") ||
      !Check(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache),L"Initialize PNG encoder") ||
      !Check(encoder->CreateNewFrame(frame.GetAddressOf(),properties.GetAddressOf()),L"Create PNG frame") ||
      !Check(frame->Initialize(properties.Get()),L"Initialize PNG frame") ||
      !Check(frame->SetSize(width_,height_),L"Set PNG dimensions")) return false;
  WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;
  if (!Check(frame->SetPixelFormat(&format),L"Set PNG format")) return false;
  if (format!=GUID_WICPixelFormat32bppBGRA) { error_=L"PNG encoder did not accept BGRA."; return false; }
  return Check(frame->WritePixels(height_,width_*4,static_cast<UINT>(pixels.size()),pixels.data()),L"Write PNG pixels") &&
   Check(frame->Commit(),L"Commit PNG frame") && Check(encoder->Commit(),L"Commit PNG");
 };
 const bool success=encode();
 if (SUCCEEDED(initialized)) CoUninitialize();
 return success;
}

bool Renderer::ValidateDebugMessages() {
#ifdef _DEBUG
 ComPtr<ID3D11InfoQueue> messages;
 if (SUCCEEDED(device_.As(&messages))) {
  for (UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i) {
   SIZE_T size=0; messages->GetMessage(i,nullptr,&size);
   std::vector<unsigned char> storage(size);
   auto* message=reinterpret_cast<D3D11_MESSAGE*>(storage.data());
   if (SUCCEEDED(messages->GetMessage(i,message,&size)) && message->Severity<=D3D11_MESSAGE_SEVERITY_WARNING) {
    const std::string description(message->pDescription);
    error_=L"DirectX validation: "+std::wstring(description.begin(),description.end()); return false;
   }
  }
  messages->ClearStoredMessages();
 }
#endif
 return true;
}

float Renderer::TextWidth(const std::string& text,float size) const {
 float line=0,width=0;
 for (unsigned char ch : text) {
  if (ch=='\n') { width=std::max(width,line); line=0; }
  else line+=font_.Character(ch).advance*size*.5f;
 }
 return std::max(width,line);
}
bool Renderer::SetTerrainSurface(const std::vector<std::uint8_t>& pixels,unsigned size) {
 if (size!=256 || pixels.size()!=static_cast<std::size_t>(size)*size*4) {
  error_=L"Invalid terrain surface dimensions."; frameFailed_=true; return false;
 }
 D3D11_TEXTURE2D_DESC desc{}; desc.Width=desc.Height=size; desc.ArraySize=1; desc.SampleDesc.Count=1;
 desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
 desc.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
 ComPtr<ID3D11Texture2D> texture;
 if (!Check(device_->CreateTexture2D(&desc,nullptr,texture.GetAddressOf()),L"Create terrain surface")) { frameFailed_=true; return false; }
 context_->UpdateSubresource(texture.Get(),0,nullptr,pixels.data(),size*4,0);
 if (!Check(device_->CreateShaderResourceView(texture.Get(),nullptr,terrainView_.ReleaseAndGetAddressOf()),L"Create terrain surface view")) { frameFailed_=true; return false; }
 context_->GenerateMips(terrainView_.Get()); terrainBytes_=0;
 for (auto dimension=size;dimension;dimension/=2) terrainBytes_+=dimension*dimension*4;
 return true;
}
bool Renderer::SetMapArtwork(const std::vector<std::uint8_t>& pixels,unsigned size) {
 if (pixels.size()!=static_cast<std::size_t>(size)*size*4 || size!=512) {
  error_=L"Invalid map artwork dimensions."; frameFailed_=true; return false;
 }
 D3D11_TEXTURE2D_DESC desc{}; desc.Width=desc.Height=size; desc.ArraySize=1; desc.SampleDesc.Count=1;
 desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
 desc.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
 ComPtr<ID3D11Texture2D> texture;
 if (!Check(device_->CreateTexture2D(&desc,nullptr,texture.GetAddressOf()),L"Create map artwork")) { frameFailed_=true; return false; }
 context_->UpdateSubresource(texture.Get(),0,nullptr,pixels.data(),size*4,0);
 if (!Check(device_->CreateShaderResourceView(texture.Get(),nullptr,mapView_.ReleaseAndGetAddressOf()),L"Create map view")) { frameFailed_=true; return false; }
 context_->GenerateMips(mapView_.Get()); mapBytes_=0;
 for (auto dimension=size;dimension;dimension/=2) mapBytes_+=dimension*dimension*4;
 return true;
}
void Renderer::DrawMapArtwork(float x,float y,float width,float height) {
 const XMFLOAT4 tint{1,1,1,1};
 const Vertex a{{x*2-1,1-y*2,.5f},{0,0,2},tint},b{{(x+width)*2-1,1-y*2,.5f},{1,0,2},tint};
 const Vertex c{{(x+width)*2-1,1-(y+height)*2,.5f},{1,1,2},tint},d{{x*2-1,1-(y+height)*2,.5f},{0,1,2},tint};
 hud_.insert(hud_.end(),{a,b,c,a,c,d});
}
void Renderer::DrawRoundMapArtwork(float x,float y,float width,float height) {
 const XMFLOAT4 tint{1,1,1,1};
 const auto vertex=[&](float dx,float dy) -> Vertex {
  // The circular survey keeps the same world scale on both axes.
  return {{(x+width*(.5f+dx*.5f))*2-1,1-(y+height*(.5f+dy*.5f))*2,.5f},
    {.5f+dx*.5f*rpg::WorldMap::surveyDepth/rpg::WorldMap::surveyWidth,.5f+dy*.5f,2},tint};
 };
 const auto center=vertex(0,0);
 for (int i=0;i<64;++i) {
  const float a=i*XM_2PI/64,b=(i+1)*XM_2PI/64;
  hud_.insert(hud_.end(),{center,vertex(std::cos(a),std::sin(a)),vertex(std::cos(b),std::sin(b))});
 }
}
void Renderer::DrawText(float x,float y,const std::string& text,const XMFLOAT4& color,float size) {
 const float start=x,scale=size*.5f,dx=32*scale/width_,dy=32*scale/height_;
 for (unsigned char ch : text) {
  if (ch=='\n') { y+=30*scale/height_; x=start; continue; }
  const auto& glyph=font_.Character(ch);
  if (ch!=' ') {
   const float u=glyph.u,v=glyph.v,du=32.f/512,dv=32.f/256;
   const Vertex a{{x*2-1,1-y*2,.5f},{u,v,1},color};
   const Vertex b{{(x+dx)*2-1,1-y*2,.5f},{u+du,v,1},color};
   const Vertex c{{(x+dx)*2-1,1-(y+dy)*2,.5f},{u+du,v+dv,1},color};
   const Vertex d{{x*2-1,1-(y+dy)*2,.5f},{u,v+dv,1},color};
   hud_.insert(hud_.end(),{a,b,c,a,c,d});
  }
  x+=glyph.advance*scale/width_;
 }
}
