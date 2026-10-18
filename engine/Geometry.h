#pragma once
#include <DirectXMath.h>
#include <array>
#include <cstdint>
#include <vector>

namespace visual {
enum class ShadowMode { Cast, None };
inline constexpr unsigned materialSize=128,shadowSize=1024,shadowLayers=2;
enum class Mesh : unsigned { Box, Roof, Sphere, Rock, Trunk, Foliage, Torso, Limb, Blade, Crystal, Terrain, Cliff, Head, Boot, Hair, Grass, Barrel,Arch,Canopy,Masonry,Count };
enum class Material : unsigned { Plain, Ground, Stone, Wood, Plaster, Roof, Cloth, Metal, Leaves, Skin, Glow, Count };
struct Vertex {
  DirectX::XMFLOAT3 position, normal;
  DirectX::XMFLOAT4 color{1,1,1,1};
};
struct Geometry { std::vector<Vertex> vertices; std::vector<std::uint32_t> indices; };
Geometry BuildMesh(Mesh mesh);
class Frustum {
public:
  explicit Frustum(const DirectX::XMFLOAT4X4& viewProjection);
  bool VisibleSphere(const DirectX::XMFLOAT3& center,float radius) const;
private:
  std::array<DirectX::XMFLOAT4,6> planes_;
  std::array<float,6> lengths_;
};
bool VisibleSphere(const DirectX::XMFLOAT3& center,float radius,const DirectX::XMFLOAT4X4& viewProjection);
// 128-square packed layers: R albedo value, GB signed height slopes (128=zero,
// 32 codes per slope unit), A roughness. Opaque output; mips average the slopes.
std::vector<std::uint8_t> BuildMaterialLayer(Material material,unsigned size);
}
