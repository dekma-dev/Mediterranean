#pragma once
#include "engine/Skeleton.h"
#include <string_view>
#include <array>
#include <cstdint>

namespace assets {
// Offline GLB 2.0 subset. Never follows paths or opens resources.
// One skin, TRS joints, optional identity mesh node and embedded accessors.
// Matrix nodes, sparse/strided accessors and extensions reject.
struct SkinVertex {
  DirectX::XMFLOAT3 position{},normal{};
  std::array<std::uint16_t,4> joints{};
  std::array<float,4> weights{};
};
struct SkinMesh { std::vector<SkinVertex> vertices; std::vector<std::uint16_t> indices; };
struct DeformedVertex { DirectX::XMFLOAT3 position{},normal{}; };
// Offline correctness reference, not a per-frame rendering path.
std::vector<DeformedVertex> DeformMesh(const SkinMesh& mesh,const animation::Pose& pose);
struct NamedClip { std::string name; animation::Clip clip; };
struct RigAsset { animation::Skeleton skeleton; std::vector<NamedClip> clips; SkinMesh mesh; };
// Also accepts bounded LINEAR translation/rotation/scale animation channels.
RigAsset ImportGlbAsset(std::string_view bytes);
// Static-rig convenience: refuses animation/meshes rather than discarding them.
animation::Skeleton ImportGlbRig(std::string_view bytes);
}
