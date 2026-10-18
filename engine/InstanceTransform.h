#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

namespace visual {
// For scale * orthonormal rotation * translation only (no shear).
// Normals use xyz with w=0; the unused translation column need not be inverted.
struct InstanceTransform {
  DirectX::XMMATRIX normal;
  float radiusScale;
};
inline InstanceTransform PrepareInstanceTransform(const DirectX::XMMATRIX& world) {
  using namespace DirectX;
  const auto x=XMVector3LengthSq(world.r[0]);
  const auto y=XMVector3LengthSq(world.r[1]);
  const auto z=XMVector3LengthSq(world.r[2]);
  return {XMMATRIX(world.r[0]/x,world.r[1]/y,world.r[2]/z,XMVectorSet(0,0,0,1)),
    std::sqrt(std::max({XMVectorGetX(x),XMVectorGetX(y),XMVectorGetX(z)}))};
}
}
