#pragma once
#include <DirectXMath.h>
#include <cstdint>
#include <vector>

namespace visual {
inline constexpr unsigned materialSize=128,shadowSize=1024;
enum class Mesh : unsigned { Box, Roof, Sphere, Rock, Trunk, Foliage, Torso, Limb, Blade, Crystal, Terrain, Cliff, Count };
enum class Material : unsigned { Plain, Ground, Stone, Wood, Plaster, Roof, Cloth, Metal, Leaves, Skin, Glow, Count };
struct Vertex {
  DirectX::XMFLOAT3 position, normal;
  DirectX::XMFLOAT4 color{1,1,1,1};
};
struct Geometry { std::vector<Vertex> vertices; std::vector<std::uint32_t> indices; };
Geometry BuildMesh(Mesh mesh);
bool VisibleSphere(const DirectX::XMFLOAT3& center,float radius,const DirectX::XMFLOAT4X4& viewProjection);
// 128-square RGBA layers, generated once. Mips are built by DirectX.
std::vector<std::uint8_t> BuildMaterialLayer(Material material,unsigned size);
}
