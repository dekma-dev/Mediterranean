#pragma once
#include "engine/Geometry.h"
#include "engine/Skinning.h"
#include <array>
#include <string_view>
namespace authored {
enum class Asset : unsigned { Guardian, Mage, Orc, Revenant, Skeleton, Wolf, Boar, Oak, Pine, DeadTree, Cottage, Inn, Tower, Count };
inline constexpr unsigned assetCount=unsigned(Asset::Count),lodCount=3,meshCount=assetCount*lodCount;
inline constexpr unsigned firstMesh=unsigned(visual::Mesh::Count),totalMeshes=firstMesh+meshCount;
inline constexpr std::size_t archiveBudget=32*1024*1024;
struct Bounds { DirectX::XMFLOAT3 minimum,maximum; };
struct Mesh { visual::Geometry geometry; Bounds bounds; };
using Library=std::array<Mesh,meshCount>;
Library Decode(std::string_view bytes);
const Library& Collection();
std::string_view EmbeddedBytes();
const char* Name(Asset asset);
constexpr unsigned Slot(Asset asset,unsigned lod=0) {return firstMesh+unsigned(asset)*lodCount+lod;}
unsigned SelectLod(float projectedHeight);
constexpr bool Animated(unsigned mesh) {return mesh>=firstMesh && mesh<Slot(Asset::Oak);}
visual::SkinInfluence Influence(Asset asset,DirectX::XMFLOAT3 position);
// Additive presentation channels, in radians/metres, independent of game rules.
struct Gesture {
 float pitch=0,yaw=0,roll=0,hip=0;
 float leftLift=0,rightLift=0,leftOpen=0,rightOpen=0;
};
visual::BodyPalette Pose(Asset asset,float time,float gait=0,float attack=0,float travel=0,Gesture gesture={});
}
