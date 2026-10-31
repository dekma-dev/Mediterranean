#pragma once
#include "engine/Geometry.h"
#include <array>
#include <cmath>

namespace visual {
// Shared source for the visible sun, directional illumination and both shadow maps.
inline const DirectX::XMFLOAT4 daylightSun{-30,51,-24,.00465f}; // xyz light position, w angular radius
// A fixed whole-island map plus a fixed-size detail region. Only the latter's
// origin moves, in whole light-space texels; orbiting cannot rotate its grid.
inline std::array<DirectX::XMFLOAT4X4,shadowLayers> ShadowProjections(DirectX::XMFLOAT3 focus) {
  using namespace DirectX;
  const auto view=XMMatrixLookAtLH(XMVectorSet(daylightSun.x,daylightSun.y,daylightSun.z,1),XMVectorZero(),XMVectorSet(0,1,0,0));
  std::array<XMFLOAT4X4,shadowLayers> result;
  XMStoreFloat4x4(&result[0],view*XMMatrixOrthographicLH(360,360,-210,350));
  XMFLOAT3 center; XMStoreFloat3(&center,XMVector3TransformCoord(XMLoadFloat3(&focus),view));
  constexpr float span=20,texel=span/shadowSize;
  const float x=std::round(center.x/texel)*texel,y=std::round(center.y/texel)*texel;
  const float depth=std::round(center.z);
  XMStoreFloat4x4(&result[1],view*XMMatrixOrthographicOffCenterLH(x-span*.5f,x+span*.5f,y-span*.5f,y+span*.5f,depth-55,depth+55));
  return result;
}
}
