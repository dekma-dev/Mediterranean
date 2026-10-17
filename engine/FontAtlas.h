#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
class FontAtlas {
public:
  struct Glyph { float u=0,v=0,advance=0; };
  bool Initialize(ID3D11Device* device);
  const Glyph& Character(unsigned char c) const { return glyphs_[c>=32 && c<127?c-32:'?'-32]; }
  ID3D11ShaderResourceView* View() const { return view_.Get(); }
private:
  std::array<Glyph,95> glyphs_{};
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_;
};
