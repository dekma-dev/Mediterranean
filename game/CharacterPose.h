#pragma once
#include <DirectXMath.h>
#include <array>

// Local visual joints, independent of combat resolution. No skinning buffers.
struct LimbPose { DirectX::XMFLOAT3 root,joint,end; };
struct CharacterPose {
  float bodyLift=0;
  std::array<LimbPose,2> arms,legs; // Left, right in character space.
  DirectX::XMFLOAT3 weaponDirection;
  static constexpr float upperArm=.28f,forearm=.27f,thigh=.40f,shin=.40f;
};
CharacterPose BuildCharacterPose(float swing,float gait,bool shield);
