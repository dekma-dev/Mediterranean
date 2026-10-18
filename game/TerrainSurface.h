#pragma once
#include "game/WorldMap.h"
#include <cstdint>
#include <vector>

inline constexpr unsigned terrainSurfaceSize=256;
// RGBA: dirt weight, paving weight, reserved, opaque. Extent matches the atlas.
std::vector<std::uint8_t> BuildTerrainSurface(const rpg::WorldMap& map);
