#pragma once
#include "engine/Geometry.h"
#include "engine/BodyLod.h"

namespace visual {
inline constexpr unsigned bodyBones=10,bodyRegions=6,maxBodies=32;
inline constexpr unsigned paletteBones=32; // Authored quadrupeds have independent anatomical joints.
// Pelvis, chest, left/right thigh+shin, left/right upper arm+forearm.
struct BoneSegment { DirectX::XMFLOAT3 from,to; float radiusFrom,radiusTo; };
struct SkinInfluence { std::array<std::uint32_t,4> bones{}; DirectX::XMFLOAT4 weights{}; };
struct BodyGeometry { Geometry mesh; std::vector<SkinInfluence> influences; };
struct BodyPalette {
  std::array<DirectX::XMFLOAT4X4,paletteBones> joints{},normals{};
  // RGB tint, W material index. Regions: chest, sleeve, forearm, thigh, shin, neck.
  std::array<DirectX::XMFLOAT4,bodyRegions> colors;
};
// Validate externally supplied preview geometry before immutable GPU upload.
void ValidateBodyGeometry(const BodyGeometry& body);
const std::array<BoneSegment,bodyBones>& BodyBindPose();
const BodyGeometry& CharacterBody(unsigned level=0);
DirectX::XMMATRIX BoneFrame(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,DirectX::XMFLOAT3 forward={0,0,1});
// CPU reference for bounds/regressions; rendering uses the same four-weight GPU rule.
Vertex SkinVertex(const Vertex& vertex,const SkinInfluence& influence,const BodyPalette& palette);
}
