#pragma once
#include "engine/Geometry.h"
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
}
