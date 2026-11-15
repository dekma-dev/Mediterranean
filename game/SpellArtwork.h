#pragma once
#include "game/Classes.h"
#include <DirectXMath.h>
namespace ui {
struct SpellArtworkCell {
  bool combat=false;
  int tile=-1;
  bool priest=false;
  DirectX::XMFLOAT4 Uv() const {
    if(priest)return {(tile%4)/4.f+2.f/512,(tile/4)/2.f+2.f/256,
      (tile%4+1)/4.f-2.f/512,(tile/4+1)/2.f-2.f/256};
    const float rows=combat?6.f:8.f;
    // Inset two decoded pixels to avoid neighboring cells under bilinear filtering.
    return {(tile%8)/8.f+2.f/1024,(tile/8)/rows+2.f/(combat?768:1024),
      (tile%8+1)/8.f-2.f/1024,(tile/8+1)/rows-2.f/(combat?768:1024)};
  }
};
inline SpellArtworkCell SpellArtwork(rpg::ClassId id,int ability) {
  if(!rpg::ValidClass(id) || ability<0 || ability>=int(rpg::spells.size())) return {};
  if(id==rpg::ClassId::Priest && ability>=8)return {false,ability-8,true};
  if(rpg::AdvancedClass(id) && id!=rpg::ClassId::Priest) return {true,(id==rpg::ClassId::Warrior?0:id==rpg::ClassId::Scout?15:30)+ability};
  if(ability>=8) return {};
  constexpr int rows[]{-1,0,-1,1,2,-1,3,4,5,6,7};
  return {false,rows[int(id)]*8+ability};
}
}
