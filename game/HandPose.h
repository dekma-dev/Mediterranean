#pragma once
#include "game/CharacterPose.h"
#include "engine/HandSkinning.h"

// Small visual articulation layered over the solved arm. Grip anchors stay at
// the established weapon/shield sockets, so combat trails retain their path.
struct FingerPose {
  std::array<DirectX::XMFLOAT3,4> joints{}; // Hand-local; three fixed phalanges.
  float width=0;
};
struct HandPose {
  DirectX::XMFLOAT4X4 frame{};
  std::array<FingerPose,5> fingers{}; // Index, middle, ring, little, thumb.
  float curl=0;
  bool mirrored=false;
  DirectX::XMFLOAT3 Point(DirectX::XMFLOAT3 local) const;
};
DirectX::XMMATRIX WeaponFrame(const CharacterPose& pose);
HandPose BuildHandPose(const CharacterPose& pose,unsigned side,bool weapon,bool shield,float cast);
// Spell release uses the same solved palm as drawing; an occupied offhand
// emits beyond its shield face instead of hiding the trace inside equipment.
DirectX::XMFLOAT3 SpellReleasePoint(const CharacterPose& pose,bool shield);
DirectX::XMFLOAT3 WeaponReleasePoint(const CharacterPose& pose,rpg::WeaponForm form);
visual::HandPalette BuildHandRig(const HandPose& pose);
