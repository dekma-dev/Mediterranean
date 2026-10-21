#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include "engine/GpuTimer.h"
#include "engine/Geometry.h"
#include "engine/Skinning.h"
#include "engine/HandSkinning.h"
#include "engine/FontAtlas.h"
#include "engine/StaticScene.h"
#include <array>
#include <functional>
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
  std::size_t MaterialBytes() const { return materialBytes_+terrainBytes_; }
  void SetCamera(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target);
  void SetShadowFocus(const DirectX::XMFLOAT3& focus);
  void SetAtmosphereTime(double seconds);
  // Compare the shared field with the original per-vertex integration.
  void SetCloudLightFieldEnabled(bool enabled) { cloudLightFieldEnabled_=enabled; }
  unsigned CloudLightUpdates() const { return cloudLightUpdates_; }
  bool Project(const DirectX::XMFLOAT3& world, DirectX::XMFLOAT2& screen) const;
  void DrawMarkerBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color, float yaw = 0);
  void DrawMesh(visual::Mesh mesh,const DirectX::XMFLOAT3& position,const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color,visual::Material material,float yaw = 0,float pitch = 0,
    visual::ShadowMode shadow = visual::ShadowMode::Cast,float roll = 0,unsigned maxDetail=2);
  void DrawCloak(const DirectX::XMFLOAT3& shoulder,float yaw,float pitch,float roll,float side,float back);
  void DrawMeshTransformed(visual::Mesh mesh,const DirectX::XMMATRIX& transform,
    const DirectX::XMFLOAT4& color,visual::Material material,visual::ShadowMode shadow=visual::ShadowMode::Cast,unsigned maxDetail=2) {
    Queue(mesh,transform,color,material,shadow,maxDetail);
  }
  unsigned DrawBody(DirectX::XMFLOAT3 position,float yaw,const visual::BodyPalette& palette,std::uint64_t actor=0);
  void DrawHand(const DirectX::XMMATRIX& transform,const visual::HandPalette& palette,bool mirrored,
    const DirectX::XMFLOAT4& color,visual::Material material);
  // Only mesh draws belong inside this scope; flushes/portraits begin outside.
  class AttachmentScope {
    friend class Renderer;
    Renderer& renderer_; std::uint8_t previous_;
    AttachmentScope(Renderer& renderer,unsigned body):renderer_(renderer),previous_(renderer.attachmentBody_) {
      renderer_.attachmentBody_=body<renderer.bodyCount_?std::uint8_t(body):255;
    }
  public:
    AttachmentScope(const AttachmentScope&)=delete;
    ~AttachmentScope() { renderer_.attachmentBody_=previous_; }
  };
  [[nodiscard]] AttachmentScope AttachToBody(unsigned body) { return {*this,body}; }
  void SetAttachmentLodEnabled(bool enabled) { attachmentLodEnabled_=enabled; }
  // Remapped/omitted queued world attachments, excluding portraits.
  const std::array<unsigned,2>& AttachmentLodCounts() const { return attachmentLodCounts_; }
  const std::array<unsigned,2>& LastAttachmentLodCounts() const { return lastAttachmentLodCounts_; }
  // Diagnostics can compare against full detail without changing game state.
  void SetBodyLodEnabled(bool enabled) { bodyLodEnabled_=enabled; bodyLodHistory_.Reset(); }
  const std::array<unsigned,visual::bodyLodCount>& BodyLodCounts() const { return bodyLodCounts_; }
  const std::array<unsigned,visual::bodyLodCount>& LastBodyLodCounts() const { return lastBodyLodCounts_; }
  void DrawSegment(const DirectX::XMFLOAT3& from,const DirectX::XMFLOAT3& to,float width,float depth,
    const DirectX::XMFLOAT4& color,visual::Material material);
  void DrawMeshBetween(visual::Mesh mesh,const DirectX::XMFLOAT3& from,const DirectX::XMFLOAT3& to,float width,float depth,
    const DirectX::XMFLOAT4& color,visual::Material material,DirectX::XMFLOAT3 facing={0,0,1});
  // Emit only immutable mesh draws here: no effects, portraits, HUD or flushes.
  // Invalidate the scene when its authored geometry changes.
  void DrawStaticScene(visual::StaticScene& scene,const std::function<void()>& emit);
  // Texture-free, additive world effects: depth tested, no depth writes/shadows.
  static constexpr unsigned maxEffectVertices=6144;
  void DrawEffectRibbon(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 c,DirectX::XMFLOAT3 d,
    DirectX::XMFLOAT4 color,float startAlpha=1,float endAlpha=1);
  void DrawEffectGlow(DirectX::XMFLOAT3 center,float radius,DirectX::XMFLOAT4 color);
  void DrawEffectRing(DirectX::XMFLOAT3 center,float radius,DirectX::XMFLOAT4 color);
  void DrawEffectStreak(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,float width,DirectX::XMFLOAT4 color);
  unsigned EffectVertices() const { return effectVertices_+static_cast<unsigned>(effects_.size()); }
  UINT LastDrawCalls() const { return lastDrawCalls_; }
  bool Capture(const std::wstring& path);
  bool ValidateDebugMessages();
  int Width() const { return width_; }
  int Height() const { return height_; }
  bool BeginPortrait(float x,float y,float width,float height);
  bool EndPortrait();
  UINT DrawCalls() const { return drawCalls_ + static_cast<UINT>((hud_.size()+16379)/16380) + UINT(!effects_.empty()); }
  void DrawHudRect(float x, float y, float width, float height, const DirectX::XMFLOAT4& color);
  void DrawHudGradient(float x,float y,float width,float height,DirectX::XMFLOAT4 top,DirectX::XMFLOAT4 bottom);
  void DrawHudTriangle(DirectX::XMFLOAT2 a,DirectX::XMFLOAT2 b,DirectX::XMFLOAT2 c,const DirectX::XMFLOAT4& color);
  enum class UiArt { Frame, Hall, Icons };
  // Artwork joins the existing HUD batch; no texture swaps or per-panel upload.
  void DrawUiArt(UiArt art,float x,float y,float width,float height,
    DirectX::XMFLOAT4 uv={0,0,1,1},DirectX::XMFLOAT4 tint={1,1,1,1});
  float TextWidth(const std::string& text,float size) const;
  bool HasMapArtwork() const { return mapView_!=nullptr; }
  bool HasTerrainSurface() const { return terrainView_!=nullptr; }
  bool SetTerrainSurface(const std::vector<std::uint8_t>& pixels,unsigned size);
  bool SetMapArtwork(const std::vector<std::uint8_t>& pixels,unsigned size);
  void DrawMapArtwork(float x,float y,float width,float height);
  void DrawRoundMapArtwork(float x,float y,float width,float height);
  std::size_t InterfaceBytes() const { return 512*256+mapBytes_+uiBytes_; }
  void DrawText(float x, float y, const std::string& text, const DirectX::XMFLOAT4& color, float size = 2);
  const std::wstring& Error() const { return error_; }
private:
  bool Check(HRESULT result, const wchar_t* operation);
  bool CreateTargets(int width, int height);
  bool CreatePipeline();
  bool FlushHud();
  bool CreateMaterials();
  bool CreateUiArtwork();
  void DrawSky();
  void PrepareCloudLight();
  bool FlushWorld();
  bool FlushEffects();
  void EffectQuad(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 c,DirectX::XMFLOAT3 d,
    DirectX::XMFLOAT4 color,float startAlpha,float endAlpha,float radial);
  void EffectBillboard(DirectX::XMFLOAT3 center,float radius,DirectX::XMFLOAT4 color,float shape);
  void Queue(visual::Mesh mesh,const DirectX::XMMATRIX& world,const DirectX::XMFLOAT4& color,visual::Material material,
    visual::ShadowMode shadow = visual::ShadowMode::Cast,unsigned maxDetail=2);
  static constexpr unsigned meshCount = static_cast<unsigned>(visual::Mesh::Count);
  static constexpr unsigned maxInstances = 4096;
  using Instance=visual::SceneInstance;
  using Item=visual::SceneItem;
  struct MeshRange { UINT start = 0,count = 0; float radius = 0; };
  std::array<MeshRange,meshCount> meshes_{};
  std::array<std::vector<Item>,meshCount> world_;
  std::vector<Instance> instanceScratch_;
  std::array<visual::BodyPalette,visual::maxBodies> bodyPalettes_{};
  unsigned bodyCount_=0;
  std::array<visual::HandPalette,visual::maxHands> handPalettes_{};
  unsigned handCount_=0;
  Microsoft::WRL::ComPtr<ID3D11Buffer> handConstants_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> handShader_,handShadowShader_;
  std::array<std::uint64_t,visual::maxBodies> bodyActors_{};
  visual::BodyLodHistory bodyLodHistory_;
  std::array<unsigned,visual::bodyLodCount> bodyLodCounts_{},lastBodyLodCounts_{};
  bool bodyLodEnabled_=true;
  bool attachmentLodEnabled_=true;
  std::uint8_t attachmentBody_=255;
  std::array<unsigned,2> attachmentLodCounts_{},lastAttachmentLodCounts_{};
  Microsoft::WRL::ComPtr<ID3D11Buffer> skinWeights_,skinConstants_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> bodyShader_,bodyShadowShader_;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> bodyLayout_;
  std::vector<visual::Vertex> hud_;
  std::vector<visual::Vertex> effects_;
  unsigned effectVertices_=0;
  FontAtlas font_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mapView_;
  std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>,3> uiViews_;
  std::size_t uiBytes_=0;
  std::size_t mapBytes_ = 0;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> fontSampler_;
  Microsoft::WRL::ComPtr<ID3D11BlendState> hudBlend_;
  Microsoft::WRL::ComPtr<ID3D11BlendState> effectBlend_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> effectRasterizer_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> effectDepth_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> effectVertexShader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> effectPixelShader_;
  bool portrait_ = false,frameFailed_ = false,skyPending_ = false;
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
  Microsoft::WRL::ComPtr<ID3D11VertexShader> worldShader_,shadowShader_,skyShader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> skyPixel_;
  static constexpr unsigned cloudLightSize=64;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> cloudLightShader_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> cloudLightConstants_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cloudLightView_;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cloudLightTarget_;
  bool cloudLightFieldEnabled_=true;
  float cloudLightPhase_=-1;
  unsigned cloudLightUpdates_=0;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> worldLayout_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> materialView_,shadowView_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainView_;
  std::array<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>,visual::shadowLayers> shadowDepth_;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> surfaceSampler_,shadowSampler_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthOn_, depthOff_;
  DirectX::XMFLOAT4X4 viewProjection_{};
  std::array<DirectX::XMFLOAT4X4,visual::shadowLayers> shadowProjection_{};
  DirectX::XMFLOAT3 eye_{};
  float atmospherePhase_=0;
  int width_ = 1, height_ = 1;
  UINT drawCalls_ = 0;
  UINT triangles_ = 0;
  UINT lastDrawCalls_ = 0;
  std::size_t meshBytes_ = 0, materialBytes_ = 0;
  std::size_t terrainBytes_ = 0;
  GpuTimer gpuTimer_;
  std::wstring error_;
};
