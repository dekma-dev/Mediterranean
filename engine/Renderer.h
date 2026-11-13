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
#include "engine/TransientLights.h"
#include <array>
#include <functional>
class Renderer {
public:
  // Expanded highlands share one bounded upload across color and shadow passes.
  static constexpr unsigned maxWorldInstances=8192;
  // Optional immutable preview mesh replaces all three body LODs for this
  // renderer only. Read synchronously; no borrowed pointer survives this call.
  bool Initialize(HWND window, int width, int height,const visual::BodyGeometry* previewBody=nullptr);
  bool Resize(int width, int height);
  void BeginFrame();
  bool EndFrame(bool vsync = true);
  double GpuMilliseconds() const { return gpuTimer_.Milliseconds(); }
  std::uint64_t GpuSamples() const { return gpuTimer_.Samples(); }
  std::uint64_t GpuFrame() const { return gpuTimer_.Frame(); }
  const std::array<GpuObservation,4>& GpuReady() const { return gpuTimer_.Ready(); }
  std::size_t GpuReadyCount() const { return gpuTimer_.ReadyCount(); }
  std::string AdapterDiagnostics() const;
  UINT Triangles() const { return triangles_; }
  std::size_t MeshBytes() const { return meshBytes_; }
  std::size_t MaterialBytes() const { return materialBytes_+terrainBytes_; }
  void SetCamera(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target);
  void SetShadowFocus(const DirectX::XMFLOAT3& focus);
  void SetAtmosphereTime(double seconds);
  void AddCombatLight(DirectX::XMFLOAT3 position,float radius,DirectX::XMFLOAT3 color,float power){combatLights_.Add(position,radius,color,power,eye_);}
  unsigned CombatLightCount() const {return combatLights_.Count();}
  void SetCombatLightingEnabled(bool value){combatLightingEnabled_=value;}
  // Compare the shared field with the original per-vertex integration.
  void SetCloudLightFieldEnabled(bool enabled) { cloudLightFieldEnabled_=enabled; }
  unsigned CloudLightUpdates() const { return cloudLightUpdates_; }
  bool Project(const DirectX::XMFLOAT3& world, DirectX::XMFLOAT2& screen,float* depth=nullptr) const;
  void DrawMarkerBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color, float yaw = 0);
  void DrawMesh(visual::Mesh mesh,const DirectX::XMFLOAT3& position,const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color,visual::Material material,float yaw = 0,float pitch = 0,
    visual::ShadowMode shadow = visual::ShadowMode::Cast,float roll = 0,unsigned maxDetail=2);
  void DrawCloak(const DirectX::XMFLOAT3& shoulder,float yaw,float pitch,float roll,float side,float back,bool guard=false,
                 const DirectX::XMFLOAT4& tint={1,1,1,1},const DirectX::XMFLOAT4& profile={},const DirectX::XMFLOAT3& ripple={});
  void DrawBow(visual::Mesh mesh,const DirectX::XMMATRIX& transform,float draw,float arrow);
  void DrawCoatPanel(visual::Mesh mesh,DirectX::XMFLOAT3 hem,DirectX::XMFLOAT3 waist,DirectX::XMFLOAT3 knee,
    float width,float depth,DirectX::XMFLOAT3 facing,const DirectX::XMFLOAT4& paint={1,1,1,1});
  void DrawHead(const DirectX::XMMATRIX& transform,const DirectX::XMFLOAT4& color,float blink,float expression,
    visual::Mesh mesh=visual::Mesh::Head);
  void DrawMeshTransformed(visual::Mesh mesh,const DirectX::XMMATRIX& transform,
    const DirectX::XMFLOAT4& color,visual::Material material,visual::ShadowMode shadow=visual::ShadowMode::Cast,unsigned maxDetail=2) {
    Queue(mesh,transform,color,material,shadow,maxDetail);
  }
  unsigned DrawBody(DirectX::XMFLOAT3 position,float yaw,const visual::BodyPalette& palette,std::uint64_t actor=0);
  void DrawGarment(visual::Mesh mesh,unsigned body,const DirectX::XMMATRIX& transform,
    const DirectX::XMFLOAT4& paint,float side,float back,float hem);
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
  class ElevationScope {
    Renderer& renderer_;float previous_;
  public:
    ElevationScope(Renderer& r,float h):renderer_(r),previous_(r.elevation_) {r.elevation_+=h;}
    ElevationScope(const ElevationScope&)=delete;
    ~ElevationScope(){renderer_.elevation_=previous_;}
  };
  [[nodiscard]] ElevationScope Elevate(float h) {return {*this,h};}
  void SetAttachmentLodEnabled(bool enabled) { attachmentLodEnabled_=enabled; }
  // detail=3 chooses a camera-dependent LOD; 0..2 force a review level.
  void DrawAuthoredAsset(authored::Asset asset,const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& scale={1,1,1},float yaw=0,unsigned detail=3,float pitch=0,float roll=0);
  void DrawAnimatedAsset(authored::Asset asset,const DirectX::XMFLOAT3& position,float yaw,
    float time,float gait=0,float attack=0,float travel=0,unsigned detail=3,
    const DirectX::XMFLOAT4& tint={1,1,1,1},authored::Gesture gesture={});
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
  // Continuous material coordinates across joined quads; phase is the accepted
  // effect age, never a renderer clock. Shares the bounded additive batch.
  void DrawFlameRibbon(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 c,DirectX::XMFLOAT3 d,
    float from,float to,float phase,DirectX::XMFLOAT4 color,float startAlpha=1,float endAlpha=1);
  void DrawEffectGlow(DirectX::XMFLOAT3 center,float radius,DirectX::XMFLOAT4 color);
  void DrawGroundMark(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 c,DirectX::XMFLOAT3 d,DirectX::XMFLOAT4 color);
  void DrawEffectRing(DirectX::XMFLOAT3 center,float radius,DirectX::XMFLOAT4 color);
  void DrawEffectStreak(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,float width,DirectX::XMFLOAT4 color);
  enum class EffectArt { Flame, Smoke, Dust, Ice, Arcane, Slash, Sparks, Seal };
  enum class SpellMedium { Fire, Frost, Arcane, Holy };
  // Oriented participating media, evaluated along the viewing ray rather than
  // painted onto a camera-facing image. The proxy shares the effect batch.
  void DrawSpellVolume(SpellMedium medium,DirectX::XMFLOAT3 center,DirectX::XMFLOAT3 direction,
    DirectX::XMFLOAT3 radii,float phase,DirectX::XMFLOAT4 tint,unsigned seed=0);
  unsigned SpellVolumeCount() const { return spellVolumeCount_; }
  enum class SpellField { Frost, Ember, Holy, Arcane };
  // A terrain-conforming 4x4 patch; height samples are world Y, row-major Z/X.
  void DrawSpellField(SpellField kind,DirectX::XMFLOAT2 center,float radius,float age,
    DirectX::XMFLOAT4 tint,const std::array<float,25>& heights,unsigned seed=0);
  unsigned SpellFieldCount() const { return spellFieldCount_; }
  void SetSpellGlowEnabled(bool enabled) { spellGlowEnabled_=enabled; }
  unsigned SpellGlowPasses() const { return spellGlowPasses_; }
  std::size_t SpellGlowBytes() const { return std::size_t(glowWidth_)*glowHeight_*8*3; }
  // Half extents in metres. Cloud sprites use sorted alpha; luminous sprites
  // share the additive batch. All share the existing per-frame vertex ceiling.
  void DrawEffectSprite(EffectArt art,DirectX::XMFLOAT3 center,float width,float height,float rotation,
    DirectX::XMFLOAT4 color,bool cloud=false);
  void DrawEffectSurface(EffectArt art,DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 c,
    DirectX::XMFLOAT3 d,DirectX::XMFLOAT4 color);
  std::size_t EffectTextureBytes() const {return effectTextureBytes_;}
  unsigned EffectVertices() const { return effectVertices_+static_cast<unsigned>(effects_.size()+groundMarks_.size()+cloudSprites_.size()*6); }
  UINT LastDrawCalls() const { return lastDrawCalls_; }
  bool Capture(const std::wstring& path);
  bool ValidateDebugMessages();
  int Width() const { return width_; }
  int Height() const { return height_; }
  bool BeginPortrait(float x,float y,float width,float height,float closeup=0,bool effects=false);
  bool EndPortrait();
  UINT DrawCalls() const { return drawCalls_ + static_cast<UINT>((hud_.size()+16379)/16380) + UINT(!effects_.empty())+UINT(!cloudSprites_.empty())+UINT(!groundMarks_.empty()); }
  void DrawHudRect(float x, float y, float width, float height, const DirectX::XMFLOAT4& color);
  void DrawHudGradient(float x,float y,float width,float height,DirectX::XMFLOAT4 top,DirectX::XMFLOAT4 bottom);
  void DrawHudTriangle(DirectX::XMFLOAT2 a,DirectX::XMFLOAT2 b,DirectX::XMFLOAT2 c,const DirectX::XMFLOAT4& color);
  enum class UiArt { Frame, Hall, Icons, CombatSpells, ClassSpells };
  // Artwork joins the existing HUD batch; no texture swaps or per-panel upload.
  void DrawUiArt(UiArt art,float x,float y,float width,float height,
    DirectX::XMFLOAT4 uv={0,0,1,1},DirectX::XMFLOAT4 tint={1,1,1,1});
  float TextWidth(const std::string& text,float size,bool literal=false) const;
  bool HasMapArtwork() const { return mapView_!=nullptr; }
  bool HasTerrainSurface() const { return terrainView_!=nullptr; }
  bool SetTerrainSurface(const std::vector<std::uint8_t>& pixels,unsigned size);
  bool SetMapArtwork(const std::vector<std::uint8_t>& pixels,unsigned size);
  void DrawMapArtwork(float x,float y,float width,float height);
  void DrawRoundMapArtwork(float x,float y,float width,float height);
  std::size_t InterfaceBytes() const { return 512*512+mapBytes_+uiBytes_; }
  void DrawText(float x, float y, const std::string& text, const DirectX::XMFLOAT4& color, float size = 2,bool literal=false);
  const std::wstring& Error() const { return error_; }
private:
  bool Check(HRESULT result, const wchar_t* operation);
  bool CreateTargets(int width, int height);
  bool CreatePipeline(const visual::BodyGeometry* previewBody);
  visual::TransientLights combatLights_;
  bool combatLightingEnabled_=true;
  Microsoft::WRL::ComPtr<ID3D11Buffer> combatLightConstants_;
  bool FlushHud();
  bool CreateMaterials();
  bool CreateSurfaceMaterials();
  bool CreateUiArtwork();
  void DrawSky();
  void PrepareCloudLight();
  bool FlushWorld();
  bool FlushEffects();
  bool CreateSpellGlow();
  void EmitSpellGlow(UINT vertices);
  void CompositeSpellGlow();
  bool spellGlowEnabled_=true;
  unsigned spellGlowPasses_=0,glowWidth_=0,glowHeight_=0;
  std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>,3> glowTargets_;
  std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>,3> glowViews_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> glowConstants_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> glowVertex_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> glowSource_,glowHorizontal_,glowVertical_,glowComposite_;
  void EffectQuad(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 c,DirectX::XMFLOAT3 d,
    DirectX::XMFLOAT4 color,float startAlpha,float endAlpha,float radial);
  void EffectBillboard(DirectX::XMFLOAT3 center,float radius,DirectX::XMFLOAT4 color,float shape);
  void Queue(visual::Mesh mesh,const DirectX::XMMATRIX& world,const DirectX::XMFLOAT4& color,visual::Material material,
    visual::ShadowMode shadow = visual::ShadowMode::Cast,unsigned maxDetail=2);
  static constexpr unsigned meshCount = authored::totalMeshes;
  static constexpr unsigned maxInstances = maxWorldInstances;
  using Instance=visual::SceneInstance;
  using Item=visual::SceneItem;
  struct MeshRange { UINT start = 0,count = 0; float radius = 0; };
  std::array<MeshRange,meshCount> meshes_{};
  std::array<std::vector<Item>,meshCount> world_;
  std::vector<Instance> instanceScratch_;
  // Bounded staging storage lives on the heap. Keeping both palettes inline
  // consumed over 100 KiB of each caller's stack before nested UI/pose work.
  std::vector<visual::BodyPalette> bodyPalettes_=std::vector<visual::BodyPalette>(visual::maxBodies);
  unsigned bodyCount_=0;
  float previewBodyRadius_=0,elevation_=0;
  std::vector<visual::HandPalette> handPalettes_=std::vector<visual::HandPalette>(visual::maxHands);
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
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skinPaletteView_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> bodyShader_,bodyShadowShader_;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> bodyLayout_;
  std::vector<visual::Vertex> hud_;
  std::vector<visual::Vertex> effects_;
  std::vector<visual::Vertex> groundMarks_;
  struct CloudSprite {std::array<visual::Vertex,6> vertices;float depth;};
  std::vector<CloudSprite> cloudSprites_;
  struct SpellVolume {
    DirectX::XMFLOAT4 centerKind,rightRadius,upRadius,axisRadius,clock,tint;
  };
  static constexpr unsigned maxSpellVolumes=24;
  std::array<SpellVolume,maxSpellVolumes> spellVolumes_{};
  unsigned spellVolumeCount_=0;
  float spellVolumeCoverage_=0;
  Microsoft::WRL::ComPtr<ID3D11Buffer> spellVolumeConstants_;
  struct SpellFieldData {DirectX::XMFLOAT4 clock,tint;};
  std::array<SpellFieldData,12> spellFields_{};
  unsigned spellFieldCount_=0;
  Microsoft::WRL::ComPtr<ID3D11Buffer> spellFieldConstants_;
  std::vector<visual::Vertex> cloudVertices_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> effectAtlas_;
  std::size_t effectTextureBytes_=0;
  unsigned effectVertices_=0;
  FontAtlas font_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mapView_;
  std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>,5> uiViews_;
  std::size_t uiBytes_=0;
  std::size_t mapBytes_ = 0;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> fontSampler_;
  Microsoft::WRL::ComPtr<ID3D11BlendState> hudBlend_;
  Microsoft::WRL::ComPtr<ID3D11BlendState> effectBlend_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> effectRasterizer_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> effectDepth_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> effectVertexShader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> effectPixelShader_;
  bool portrait_ = false,portraitEffects_ = false,frameFailed_ = false,skyPending_ = false;
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
  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> effectDepthView_;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> effectDepthResource_;
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
  std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>,2> surfaceViews_;
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
