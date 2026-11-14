#pragma once
#include <array>
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

namespace weapongrip {
// Measured from the authored oval leather grip, including its winding.
// Values are hand-local: X follows the shaft; Y/Z surround the handle.
inline constexpr float swordAcross=.020f,swordDepth=.016f;
inline constexpr float swordPalmInset=.011f,swordAlong=.035f;
// Fit fixed-length phalanges independently around the handle surface.
inline constexpr std::array<std::array<float,3>,4> swordCurl{{
  {1.352949f,2.282950f,3.251560f},
  {1.269954f,2.361816f,3.372013f},
  {1.334107f,2.286154f,3.280571f},
  {1.070422f,1.834302f,2.811027f}
}};
// Ranged grips retain each hand's anatomical handedness. Their palm frame is
// independent of the weapon's decorative roll/arrow plane.
inline constexpr float rangedPalmInset=.004f;
inline constexpr DirectX::XMFLOAT3 rangedSocket{0,0,-rangedPalmInset};
inline constexpr DirectX::XMFLOAT3 bowStringContact{.022f,.060f,-.008f};
inline constexpr std::array<std::array<float,3>,4> staffCurl{{
  {1.082656f,1.906200f,2.794935f},{1.193760f,2.011741f,2.966175f},
  {1.117232f,1.920132f,2.823824f},{.957350f,1.523236f,2.351457f}
}};
inline constexpr std::array<std::array<float,3>,4> bowCurl{{
  {.923839f,1.915887f,2.921370f},{.999138f,1.966497f,2.964237f},
  {.964071f,1.888275f,2.940420f},{.768799f,1.536777f,2.644050f}
}};
inline DirectX::XMMATRIX RangedPalm(DirectX::FXMVECTOR up,DirectX::FXMVECTOR right,bool bow,unsigned side) {
  using namespace DirectX;
  const auto forward=XMVector3Cross(right,up);
  const auto x=side?-up:up,y=bow?forward:right;
  return {x,y,XMVector3Cross(x,y),XMVectorSet(0,0,0,1)};
}
inline DirectX::XMVECTOR WristOffset(const DirectX::XMMATRIX& palm,bool stringHand=false) {
  using namespace DirectX;
  const auto contact=stringHand?XMLoadFloat3(&bowStringContact):XMLoadFloat3(&rangedSocket);
  return XMVector3TransformNormal(XMVectorSet(0,-.055f,-.043f,0)-contact,palm);
}
inline std::array<float,3> StringHook(unsigned finger,const float* lengths) {
  // Three fingers meet one string line: index above the arrow, middle/ring
  // below. Solve the last two fixed phalanges after a relaxed MCP joint.
  const float first=.10f+.02f*std::min(finger,2u);
  const float y=.024f+lengths[0]*std::cos(first),z=-.045f+lengths[0]*std::sin(first);
  const float dy=.053f-y,dz=-.008f-z,d=std::hypot(dy,dz),a=lengths[1],b=lengths[2];
  const float second=std::atan2(dz,dy)-std::acos(std::clamp((d*d+a*a-b*b)/(2*d*a),-1.f,1.f));
  return {first,second,second+std::acos(std::clamp((d*d-a*a-b*b)/(2*a*b),-1.f,1.f))};
}
}
