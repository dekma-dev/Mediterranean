#pragma once
#include "game/CharacterPose.h"
#include <algorithm>
#include <cmath>

// Rigid plate articulation over the final IK pose. The shoulder hinge follows
// the upper arm only partially: a pauldron must not turn over with the hand.
inline DirectX::XMMATRIX GuardianShoulderTransform(const CharacterPose& pose,unsigned arm) {
  using namespace DirectX;
  arm=std::min(arm,1u);
  const float side=arm==0?1.f:-1.f;
  const auto torso=XMMatrixRotationX(pose.upperPitch)*XMMatrixRotationZ(pose.upperRoll)*
    XMMatrixRotationY(pose.upperYaw);
  auto direction=XMVector3TransformNormal(XMLoadFloat3(&pose.arms[arm].joint)-
    XMLoadFloat3(&pose.arms[arm].root),XMMatrixTranspose(torso));
  const float length=XMVectorGetX(XMVector3Length(direction));
  direction=length>.0001f?direction/length:XMVectorSet(0,-1,0,0);
  const float outward=std::max(0.f,side*XMVectorGetX(direction));
  const float elevation=std::atan2(outward,-XMVectorGetY(direction));
  const float amount=std::clamp((elevation-.18f)/1.39f,0.f,1.f);
  const float roll=side*.72f*amount*amount*(3-2*amount);
  const float pitch=-std::clamp(XMVectorGetZ(direction)*.38f,-.30f,.30f);
  // Export origin is above/outside the anatomical shoulder socket.
  return XMMatrixTranslation(side*.035f,.05f,0)*XMMatrixRotationX(pitch)*XMMatrixRotationZ(roll)*
    XMMatrixTranslation(side*.305f,1.43f-.95f,0)*torso*
    XMMatrixTranslation(pose.bodyShift,.95f+pose.bodyLift,0);
}
