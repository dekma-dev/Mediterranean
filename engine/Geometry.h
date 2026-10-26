#pragma once
#include <DirectXMath.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace visual {
enum class ShadowMode { Cast, None };
inline constexpr unsigned materialSize=128,shadowSize=1024,shadowLayers=2;
enum class Mesh : unsigned { Box, Roof, Sphere, Rock, Trunk, Foliage, Torso, Limb, Blade, Crystal, Terrain, Cliff, Head, Boot, Hair, Grass, Barrel,Arch,Canopy,Masonry,Cloak,Body,BodyMedium,BodyFar,Feature,Palm,Finger,HeadMedium,HeadFar,HairLow,BootLow,SphereLow,TorsoLow,LimbLow,Hand,HandMirrored,HandMedium,HandMirroredMedium,HandFar,HandMirroredFar,CoatPanel,Helm,HelmLow,Pauldron,PauldronLow,GuardCuirass,GuardCuirassLow,GuardPlate,GuardPlateLow,GuardCloak,WeaponMace,WeaponHandbow,WeaponScepter,WeaponGroveStaff,WeaponAstralStaff,WeaponAshStaff,WeaponFocus,WeaponBaton,WeaponPistol,WeaponCleaver,Ridge,GuardShoulderLeft,GuardShoulderLeftLow,GuardShoulderRight,GuardShoulderRightLow,GuardFauld,GuardFauldLow,Count };
inline bool IsBodyMesh(Mesh mesh) { return mesh>=Mesh::Body && mesh<=Mesh::BodyFar; }
inline bool IsHandMesh(Mesh mesh) { return mesh>=Mesh::Hand && mesh<=Mesh::HandMirroredFar; }
inline bool IsHeadMesh(Mesh mesh) { return mesh==Mesh::Head || mesh==Mesh::HeadMedium || mesh==Mesh::HeadFar; }
inline Mesh AttachmentMesh(Mesh mesh,unsigned level) {
  if(!level) return mesh;
  switch(mesh) {
    case Mesh::Head: return level==1?Mesh::HeadMedium:Mesh::HeadFar;
    case Mesh::Hair: return Mesh::HairLow;
    case Mesh::Helm: return Mesh::HelmLow;
    case Mesh::Pauldron: return Mesh::PauldronLow;
    case Mesh::GuardCuirass: return Mesh::GuardCuirassLow;
    case Mesh::GuardPlate: return Mesh::GuardPlateLow;
    case Mesh::GuardShoulderLeft: return Mesh::GuardShoulderLeftLow;
    case Mesh::GuardShoulderRight: return Mesh::GuardShoulderRightLow;
    case Mesh::GuardFauld: return Mesh::GuardFauldLow;
    case Mesh::Boot: return Mesh::BootLow;
    case Mesh::Sphere: return Mesh::SphereLow;
    case Mesh::Torso: return Mesh::TorsoLow;
    case Mesh::Limb: return Mesh::LimbLow;
    case Mesh::Hand: case Mesh::HandMirrored: return Mesh(unsigned(mesh)+std::min(level,2u)*2);
    default: return mesh;
  }
}
enum class Material : unsigned { Plain, Ground, Stone, Wood, Plaster, Roof, Cloth, Metal, Leaves, Skin, Glow, Leather, Hair, Count };
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
