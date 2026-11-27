#pragma once
#include "engine/Geometry.h"
#include "engine/BodyLod.h"
#include "engine/SkinPaletteLayout.h"
#include <cstddef>
#include <type_traits>

namespace visual {
inline constexpr unsigned sourceBodyBones=10,bodyBones=15,bodyRegions=7,maxBodies=32;
inline constexpr unsigned neckBone=14;
inline constexpr unsigned paletteBones=32; // Authored quadrupeds have independent anatomical joints.
// Original indices: pelvis, chest, left/right thigh+shin and upper arm+forearm.
// Native extension: spine (10/11), clavicles (12/13), continuous neck (14).
struct BoneSegment { DirectX::XMFLOAT3 from,to; float radiusFrom,radiusTo; };
struct SkinInfluence { std::array<std::uint32_t,4> bones{}; DirectX::XMFLOAT4 weights{}; };
struct BodyGeometry { Geometry mesh; std::vector<SkinInfluence> influences; };
struct DualQuaternion { DirectX::XMFLOAT4 rotation{},translation{}; };
struct BodyPalette {
  std::array<DirectX::XMFLOAT4X4,paletteBones> joints{},normals{};
  // RGB tint, W material index. Chest, sleeve, forearm, thigh, shin, neck, undershorts.
  std::array<DirectX::XMFLOAT4,bodyRegions> colors;
  std::array<DualQuaternion,bodyBones> volume{};
  DirectX::XMFLOAT4 deformation{}; // X: preserve volume; Y: fitted warrior under-layer.
};
inline constexpr unsigned bodyPaletteRows=RPG_SKIN_ROWS;
static_assert(sizeof(BodyPalette)==bodyPaletteRows*sizeof(DirectX::XMFLOAT4));
static_assert(std::is_standard_layout_v<BodyPalette> && offsetof(BodyPalette,joints)==0);
static_assert(sizeof(DirectX::XMFLOAT4X4)==RPG_SKIN_JOINT_ROWS*sizeof(DirectX::XMFLOAT4));
static_assert(offsetof(BodyPalette,normals)==RPG_SKIN_NORMAL_ROW*sizeof(DirectX::XMFLOAT4));
static_assert(offsetof(BodyPalette,colors)==RPG_SKIN_COLOR_ROW*sizeof(DirectX::XMFLOAT4));
static_assert(offsetof(BodyPalette,volume)==RPG_SKIN_VOLUME_ROW*sizeof(DirectX::XMFLOAT4));
static_assert(sizeof(DualQuaternion)==RPG_SKIN_VOLUME_ROWS*sizeof(DirectX::XMFLOAT4));
static_assert(offsetof(BodyPalette,deformation)==RPG_SKIN_DEFORMATION_ROW*sizeof(DirectX::XMFLOAT4));
// Optional human-body deformation. Matrix residuals retain the small bind-length
// corrections; imported/scaled creatures keep their existing matrix skinning.
void PreserveBodyVolume(BodyPalette& palette);
// Distribute original torso weights over the native spine and shoulder joints.
SkinInfluence ArticulateTorso(DirectX::XMFLOAT3 position,const SkinInfluence& source,bool rigidChest=false,bool articulateNeck=false);
// Validate externally supplied preview geometry before immutable GPU upload.
void ValidateBodyGeometry(const BodyGeometry& body);
const std::array<BoneSegment,bodyBones>& BodyBindPose();
const BodyGeometry& CharacterBody(unsigned level=0);
DirectX::XMMATRIX BoneFrame(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,DirectX::XMFLOAT3 forward={0,0,1});
// CPU reference for bounds/regressions; rendering uses the same four-weight GPU rule.
Vertex SkinVertex(const Vertex& vertex,const SkinInfluence& influence,const BodyPalette& palette);
}
