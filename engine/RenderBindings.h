#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>

// Owns VS/PS resource, sampler and OM bindings on an immediate context.
// These bindings must not be changed behind its back. Clear() releases tracked
// references before resize/reconstruction; no frame-time Get* state queries.
class RenderBindings {
public:
  enum class Stage { Vertex, Pixel };
  struct Statistics {std::uint64_t requests=0,calls=0,skipped=0,hazardUnbinds=0,rejected=0;};
  HRESULT Attach(ID3D11DeviceContext* context);
  void Clear();
  void ResetStatistics(){statistics_={};}
  const Statistics& Stats() const{return statistics_;}
  HRESULT Resources(Stage stage,UINT first,UINT count,ID3D11ShaderResourceView* const* views);
  HRESULT Samplers(Stage stage,UINT first,UINT count,ID3D11SamplerState* const* states);
  HRESULT Targets(UINT count,ID3D11RenderTargetView* const* targets,ID3D11DepthStencilView* depth);
private:
  struct Range {
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    UINT mip=0,mips=1,slice=0,slices=1,aspects=7;
    UINT width=0,height=0,samples=0,quality=0;
  };
  struct Input {ID3D11ShaderResourceView* view=nullptr;Range range;};
  static bool Overlap(const Range& a,const Range& b);
  static HRESULT Describe(ID3D11ShaderResourceView* view,Range& range);
  static HRESULT Describe(ID3D11RenderTargetView* view,Range& range);
  static HRESULT Describe(ID3D11DepthStencilView* view,Range& range);
  bool Owns(ID3D11DeviceChild* child) const;
  void SetResources(unsigned stage,UINT first,UINT count,ID3D11ShaderResourceView* const* views);
  HRESULT Reject(HRESULT result){++statistics_.rejected;return result;}
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  std::array<std::array<Input,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>,2> inputs_{};
  std::array<std::array<ID3D11SamplerState*,D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT>,2> samplers_{};
  std::array<ID3D11RenderTargetView*,D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> targets_{};
  std::array<Range,D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> targetRanges_{};
  ID3D11DepthStencilView* depth_=nullptr;
  Range depthRange_;
  Statistics statistics_;
};
