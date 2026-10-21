#pragma once
#include "engine/Skinning.h"

namespace visual {
inline constexpr unsigned handBones=16,maxHands=maxBodies*2;
// Rigid finger transforms need no separate normal palette. 64 hands exactly
// fill one 64 KiB D3D11 constant buffer, separate from the body palette.
struct HandPalette { std::array<DirectX::XMFLOAT4X4,handBones> joints; };
const std::array<BoneSegment,handBones>& HandBindPose();
DirectX::XMMATRIX HandBoneFrame(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,unsigned finger,bool mirrored);
const BodyGeometry& CharacterHand(unsigned level=0,bool mirrored=false);
Vertex SkinHandVertex(const Vertex& vertex,const SkinInfluence& influence,const HandPalette& palette);
}
