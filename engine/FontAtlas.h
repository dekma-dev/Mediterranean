#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
class FontAtlas {
public:
  struct Glyph { float u=0,v=0,advance=0; };
  bool Initialize(ID3D11Device* device);
  const Glyph& Character(unsigned c) const {
    if(c==0x2013 || c==0x2014) c='-';
    if(c==0x2019) c='\'';
    return glyphs_[c>=32 && c<127?c-32:c>=0x400 && c<0x460?95+c-0x400:'?'-32];
  }
  ID3D11ShaderResourceView* View() const { return view_.Get(); }
private:
  std::array<Glyph,191> glyphs_{};
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_;
};
