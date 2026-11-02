#pragma once
#include "engine/Geometry.h"
#include "engine/Skinning.h"
#include <array>
#include <string_view>
namespace classart {
enum class Part:unsigned { WarriorChest,WarriorVeteranChest,WarriorShoulder,WarriorHelm,WarriorShield,WarriorSword,WarriorVeteranSword,
 ScoutChest,ScoutVeteranChest,ScoutShoulder,ScoutHood,ScoutShield,ScoutBow,ScoutVeteranBow,
 MageChest,MageVeteranChest,MageShoulder,MageHood,MageShield,MageStaff,MageVeteranStaff,
 WarriorSkirt,ScoutSkirt,MageSkirt,WarriorBelt,ScoutBelt,MageBelt,Count };
using Library=std::array<std::array<visual::Geometry,2>,unsigned(Part::Count)>;
using GarmentWeights=std::array<std::array<std::vector<visual::SkinInfluence>,2>,3>;
Library Decode(std::string_view bytes);
const Library& Collection();
GarmentWeights DecodeGarmentWeights(std::string_view bytes,const Library& meshes);
std::string_view GarmentWeightBytes();
const std::vector<visual::SkinInfluence>& GarmentInfluences(visual::Mesh mesh);
visual::Mesh Mesh(Part part,unsigned lod=0);
}
