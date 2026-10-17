#include "engine/Renderer.h"
#include <d3dcompiler.h>
#include <cstring>
#include <sstream>
#include <vector>
using namespace DirectX;
using Microsoft::WRL::ComPtr;
namespace {
struct Vertex { XMFLOAT3 position, normal; };
struct Constants { XMFLOAT4X4 world, wvp; XMFLOAT4 color, eyeAndHud; };
static_assert(sizeof(Constants)%16==0,"D3D constant buffers require 16-byte alignment.");
constexpr char shader[] = R"(
cbuffer Scene : register(b0) { matrix world; matrix wvp; float4 color; float4 eyeAndHud; };
struct VIn { float3 p : POSITION; float3 n : NORMAL; };
struct VOut { float4 p : SV_POSITION; float3 n : NORMAL; float3 wp : TEXCOORD0; };
VOut VSMain(VIn i) {
 VOut o; o.p = mul(float4(i.p,1),wvp); o.wp = mul(float4(i.p,1),world).xyz;
 o.n = normalize(mul(i.n,(float3x3)world)); return o;
}
float4 PSMain(VOut i) : SV_TARGET {
 if (eyeAndHud.w > 0.5) return color;
 float light = .42 + .58 * saturate(dot(normalize(i.n),normalize(float3(-.4,.8,-.3))));
 float fog = saturate((distance(i.wp,eyeAndHud.xyz)-24)/65);
 return float4(lerp(color.rgb*light,float3(.19,.23,.38),fog),1);
}
)";
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
 return CreateTargets(width,height) && CreatePipeline();
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
 ComPtr<ID3DBlob> vs,ps,errors;
 auto compile = [&](const char* entry,const char* profile,ComPtr<ID3DBlob>& output) {
  errors.Reset(); HRESULT hr=D3DCompile(shader,std::strlen(shader),"BuiltInScene",nullptr,nullptr,entry,profile,
   D3DCOMPILE_ENABLE_STRICTNESS,0,output.GetAddressOf(),errors.GetAddressOf());
  if (errors) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
  return Check(hr,L"Compile scene shader");
 };
 if (!compile("VSMain","vs_5_0",vs) || !compile("PSMain","ps_5_0",ps)) return false;
 if (!Check(device_->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,vertexShader_.GetAddressOf()),L"Create vertex shader")) return false;
 if (!Check(device_->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,pixelShader_.GetAddressOf()),L"Create pixel shader")) return false;
 const D3D11_INPUT_ELEMENT_DESC elements[]={
  {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
  {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
 if (!Check(device_->CreateInputLayout(elements,2,vs->GetBufferPointer(),vs->GetBufferSize(),layout_.GetAddressOf()),L"Create input layout")) return false;
 std::vector<Vertex> mesh;
 auto face = [&](XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 d,XMFLOAT3 n) {
  for (const auto& p : {a,b,c,a,c,d}) mesh.push_back({p,n});
 };
 face({-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f},{.5f,-.5f,-.5f},{0,0,-1});
 face({.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f},{-.5f,-.5f,.5f},{0,0,1});
 face({-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f},{-.5f,-.5f,-.5f},{-1,0,0});
 face({.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f},{.5f,-.5f,.5f},{1,0,0});
 face({-.5f,.5f,-.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f},{0,1,0});
 face({-.5f,-.5f,.5f},{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f},{0,-1,0});
 D3D11_BUFFER_DESC b{}; b.ByteWidth=static_cast<UINT>(mesh.size()*sizeof(Vertex));
 b.Usage=D3D11_USAGE_IMMUTABLE; b.BindFlags=D3D11_BIND_VERTEX_BUFFER;
 D3D11_SUBRESOURCE_DATA data{}; data.pSysMem=mesh.data();
 if (!Check(device_->CreateBuffer(&b,&data,vertices_.GetAddressOf()),L"Create box mesh")) return false;
 b.ByteWidth=sizeof(Constants); b.Usage=D3D11_USAGE_DEFAULT; b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
 if (!Check(device_->CreateBuffer(&b,nullptr,constants_.GetAddressOf()),L"Create constants")) return false;
 D3D11_RASTERIZER_DESC r{}; r.FillMode=D3D11_FILL_SOLID; r.CullMode=D3D11_CULL_BACK; r.DepthClipEnable=TRUE;
 if (!Check(device_->CreateRasterizerState(&r,rasterizer_.GetAddressOf()),L"Create rasterizer")) return false;
 D3D11_DEPTH_STENCIL_DESC z{}; z.DepthEnable=TRUE; z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL; z.DepthFunc=D3D11_COMPARISON_LESS;
 if (!Check(device_->CreateDepthStencilState(&z,depthOn_.GetAddressOf()),L"Create depth state")) return false;
 z.DepthEnable=FALSE; z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
 return Check(device_->CreateDepthStencilState(&z,depthOff_.GetAddressOf()),L"Create HUD depth state");
}
void Renderer::BeginFrame() {
 const float sky[]={.19f,.23f,.38f,1};
 context_->OMSetRenderTargets(1,target_.GetAddressOf(),depth_.Get());
 context_->ClearRenderTargetView(target_.Get(),sky);
 context_->ClearDepthStencilView(depth_.Get(),D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);
 D3D11_VIEWPORT v{0,0,static_cast<float>(width_),static_cast<float>(height_),0,1};
 context_->RSSetViewports(1,&v); context_->RSSetState(rasterizer_.Get());
 const UINT stride=sizeof(Vertex), offset=0;
 context_->IASetVertexBuffers(0,1,vertices_.GetAddressOf(),&stride,&offset);
 context_->IASetInputLayout(layout_.Get()); context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 context_->VSSetShader(vertexShader_.Get(),nullptr,0); context_->PSSetShader(pixelShader_.Get(),nullptr,0);
 context_->VSSetConstantBuffers(0,1,constants_.GetAddressOf()); context_->PSSetConstantBuffers(0,1,constants_.GetAddressOf());
}
void Renderer::SetCamera(const XMFLOAT3& eye,const XMFLOAT3& target) {
 eye_=eye;
 auto view=XMMatrixLookAtLH(XMLoadFloat3(&eye),XMLoadFloat3(&target),XMVectorSet(0,1,0,0));
 auto projection=XMMatrixPerspectiveFovLH(XM_PIDIV4,static_cast<float>(width_)/height_,.1f,160.f);
 XMStoreFloat4x4(&viewProjection_,view*projection);
}
void Renderer::Draw(const XMMATRIX& world,const XMMATRIX& vp,const XMFLOAT4& color,bool hud) {
 Constants data{}; XMStoreFloat4x4(&data.world,XMMatrixTranspose(world)); XMStoreFloat4x4(&data.wvp,XMMatrixTranspose(world*vp));
 data.color=color; data.eyeAndHud={eye_.x,eye_.y,eye_.z,hud?1.f:0.f};
 context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
 context_->OMSetDepthStencilState(hud?depthOff_.Get():depthOn_.Get(),0); context_->Draw(hud?6:36,0);
}
void Renderer::DrawBox(const XMFLOAT3& p,const XMFLOAT3& s,const XMFLOAT4& color,float yaw) {
 Draw(XMMatrixScaling(s.x,s.y,s.z)*XMMatrixRotationY(yaw)*XMMatrixTranslation(p.x,p.y,p.z),XMLoadFloat4x4(&viewProjection_),color,false);
}
void Renderer::DrawHudRect(float x,float y,float width,float height,const XMFLOAT4& color) {
 if (width<=0 || height<=0) return;
 Draw(XMMatrixScaling(width*2,height*2,.01f)*XMMatrixTranslation(x*2+width-1,1-y*2-height,.5f),XMMatrixIdentity(),color,true);
}
bool Renderer::EndFrame() { return Check(swapChain_->Present(1,0),L"Present frame"); }

void Renderer::DrawText(float x,float y,const std::string& text,const XMFLOAT4& color,float size) {
 // Embedded 5x7 bitmap font: no external assets or font downloads.
 static constexpr unsigned char glyphs[][5]={
  {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
  {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e},
  {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
  {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
  {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,1},
  {0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},{0x7f,2,0x0c,2,0x7f},
  {0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},{0x7f,9,9,9,6},
  {0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
  {1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
  {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
 };
 const float px=size/width_, py=size/height_, start=x;
 for (char ch : text) {
  if (ch=='\n') { y+=9*py; x=start; continue; }
  if (ch>='a' && ch<='z') ch=static_cast<char>(ch-'a'+'A');
  const int index=ch>='0' && ch<='9'?ch-'0':ch>='A' && ch<='Z'?ch-'A'+10:-1;
  if (index>=0) {
   for (int row=0;row<7;++row) {
    int column=0;
    while (column<5) {
     if (!(glyphs[index][column] & (1<<row))) { ++column; continue; }
     const int begin=column;
     while (column<5 && (glyphs[index][column] & (1<<row))) ++column;
     DrawHudRect(x+begin*px,y+row*py,(column-begin)*px,py,color);
    }
   }
  } else if (ch=='.' || ch==':') {
   DrawHudRect(x+2*px,y+6*py,px,py,color);
   if (ch==':') DrawHudRect(x+2*px,y+2*py,px,py,color);
  } else if (ch=='-' || ch=='/') DrawHudRect(x,y+3*py,4*px,py,color);
  x+=6*px;
 }
}
