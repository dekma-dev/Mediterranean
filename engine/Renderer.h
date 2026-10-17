#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include "engine/GpuTimer.h"
#include "engine/Geometry.h"
#include "engine/FontAtlas.h"
#include <array>
class Renderer {
public:
  bool Initialize(HWND window, int width, int height);
  bool Resize(int width, int height);
  void BeginFrame();
  bool EndFrame(bool vsync = true);
  double GpuMilliseconds() const { return gpuTimer_.Milliseconds(); }
  std::uint64_t GpuSamples() const { return gpuTimer_.Samples(); }
  UINT Triangles() const { return triangles_; }
  std::size_t MeshBytes() const { return meshBytes_; }
  std::size_t MaterialBytes() const { return materialBytes_; }
  void SetCamera(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target);
  bool Project(const DirectX::XMFLOAT3& world, DirectX::XMFLOAT2& screen) const;
  void DrawBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color, float yaw = 0);
  void DrawMesh(visual::Mesh mesh,const DirectX::XMFLOAT3& position,const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color,visual::Material material,float yaw = 0,float pitch = 0);
  void DrawSegment(const DirectX::XMFLOAT3& from,const DirectX::XMFLOAT3& to,float width,float depth,
    const DirectX::XMFLOAT4& color,visual::Material material);
  UINT LastDrawCalls() const { return lastDrawCalls_; }
  bool Capture(const std::wstring& path);
  bool ValidateDebugMessages();
  int Width() const { return width_; }
  int Height() const { return height_; }
  bool BeginPortrait(float x,float y,float width,float height);
  bool EndPortrait();
  UINT DrawCalls() const { return drawCalls_ + static_cast<UINT>((hud_.size()+16379)/16380); }
  void DrawHudRect(float x, float y, float width, float height, const DirectX::XMFLOAT4& color);
  void DrawHudTriangle(DirectX::XMFLOAT2 a,DirectX::XMFLOAT2 b,DirectX::XMFLOAT2 c,const DirectX::XMFLOAT4& color);
  float TextWidth(const std::string& text,float size) const;
  bool HasMapArtwork() const { return mapView_!=nullptr; }
  bool SetMapArtwork(const std::vector<std::uint8_t>& pixels,unsigned size);
  void DrawMapArtwork(float x,float y,float width,float height);
  void DrawRoundMapArtwork(float x,float y,float width,float height);
  std::size_t InterfaceBytes() const { return 512*256+mapBytes_; }
  void DrawText(float x, float y, const std::string& text, const DirectX::XMFLOAT4& color, float size = 2);
  const std::wstring& Error() const { return error_; }
private:
  bool Check(HRESULT result, const wchar_t* operation);
  bool CreateTargets(int width, int height);
  bool CreatePipeline();
  bool FlushHud();
  bool CreateMaterials();
  bool FlushWorld();
  void Queue(visual::Mesh mesh,const DirectX::XMMATRIX& world,const DirectX::XMFLOAT4& color,visual::Material material);
  static constexpr unsigned meshCount = static_cast<unsigned>(visual::Mesh::Count);
  static constexpr unsigned maxInstances = 4096;
  struct Instance { DirectX::XMFLOAT4X4 world,normal; DirectX::XMFLOAT4 color,surface; };
  struct Item { Instance instance; DirectX::XMFLOAT3 center; float radius; };
  struct MeshRange { UINT start = 0,count = 0; float radius = 0; };
  std::array<MeshRange,meshCount> meshes_{};
  std::array<std::vector<Item>,meshCount> world_;
  std::vector<Instance> instanceScratch_;
  std::vector<visual::Vertex> hud_;
  FontAtlas font_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mapView_;
  std::size_t mapBytes_ = 0;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> fontSampler_;
  Microsoft::WRL::ComPtr<ID3D11BlendState> hudBlend_;
  bool portrait_ = false,frameFailed_ = false;
  D3D11_VIEWPORT portraitViewport_{};
  DirectX::XMFLOAT4X4 savedCamera_{};
  DirectX::XMFLOAT3 savedEye_{};
  void Draw(const DirectX::XMMATRIX& world, const DirectX::XMMATRIX& vp,
    const DirectX::XMFLOAT4& color, bool hud, UINT count = 36, UINT start = 0);
  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_, constants_, hudVertices_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> indices_,instances_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> worldShader_,skyShader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> skyPixel_;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> worldLayout_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> materialView_,shadowView_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDepth_;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> surfaceSampler_,shadowSampler_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthOn_, depthOff_;
  DirectX::XMFLOAT4X4 viewProjection_{};
  DirectX::XMFLOAT4X4 shadowProjection_{};
  DirectX::XMFLOAT3 eye_{};
  int width_ = 1, height_ = 1;
  UINT drawCalls_ = 0;
  UINT triangles_ = 0;
  UINT lastDrawCalls_ = 0;
  std::size_t meshBytes_ = 0, materialBytes_ = 0;
  GpuTimer gpuTimer_;
  std::wstring error_;
};
