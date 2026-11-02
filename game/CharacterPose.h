#pragma once
#include <DirectXMath.h>
#include <array>
#include "game/Animation.h"
#include "game/ClassEquipment.h"

// Local visual joints, independent of combat resolution. CharacterRig converts
// the final IK-corrected pose into the shared body/attachment skinning palette.
struct LimbPose { DirectX::XMFLOAT3 root,joint,end; };
struct CharacterPose {
  float bodyLift=0,upperPitch=0,upperYaw=0,upperRoll=0,bodyShift=0;
  float headYaw=0,headPitch=0;
  float handCast=0;
  float staffGripWeight=0;
  float bowDraw=0,bowArrow=1;
  std::array<LimbPose,2> arms,legs; // Left, right in character space.
  std::array<float,2> footYaw{};
  std::array<float,2> footPitch{};
  DirectX::XMFLOAT3 weaponDirection;
  // Authored clips target the handle, whereas the arm solver targets the wrist.
  DirectX::XMFLOAT3 weaponGrip{};
  bool hasGripSocket=false;
  // Match BodyBindPose: knee (.17,.53,0), ankle (.18,.17,0).
  // The former .40 m shin stretched the 36 cm bind bone and forced a crouch.
  static constexpr float upperArm=.28f,forearm=.27f,thigh=.40f,shin=.36013886f;
  DirectX::XMFLOAT3 UpperPoint(DirectX::XMFLOAT3 point) const;
  DirectX::XMFLOAT3 UpperDirection(DirectX::XMFLOAT3 direction) const;
  DirectX::XMMATRIX HeadTransform() const;
  DirectX::XMFLOAT3 HeadPoint(DirectX::XMFLOAT3 point) const;
  DirectX::XMFLOAT3 WeaponPoint(float distance) const;
};
CharacterPose BuildCharacterPose(float swing,float gait,bool shield,animation::Sample dynamics = {},rpg::WeaponForm weapon=rpg::WeaponForm::Blade);
