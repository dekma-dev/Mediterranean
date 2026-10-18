#pragma once
#include "engine/Geometry.h"
#include <array>
#include <cmath>

namespace visual {
// A fixed whole-island map plus a fixed-size detail region. Only the latter's
// origin moves, in whole light-space texels; orbiting cannot rotate its grid.
inline std::array<DirectX::XMFLOAT4X4,shadowLayers> ShadowProjections(DirectX::XMFLOAT3 focus) {
  using namespace DirectX;
  const auto view=XMMatrixLookAtLH(XMVectorSet(-30,51,-24,1),XMVectorZero(),XMVectorSet(0,1,0,0));
  std::array<XMFLOAT4X4,shadowLayers> result;
  XMStoreFloat4x4(&result[0],view*XMMatrixOrthographicLH(62,62,1,110));
  XMFLOAT3 center; XMStoreFloat3(&center,XMVector3TransformCoord(XMLoadFloat3(&focus),view));
  constexpr float span=20,texel=span/shadowSize;
  const float x=std::round(center.x/texel)*texel,y=std::round(center.y/texel)*texel;
  XMStoreFloat4x4(&result[1],view*XMMatrixOrthographicOffCenterLH(x-span*.5f,x+span*.5f,y-span*.5f,y+span*.5f,1,110));
  return result;
}
}
