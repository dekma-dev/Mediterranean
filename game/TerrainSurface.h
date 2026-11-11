#pragma once
#include "game/WorldMap.h"
#include <cstdint>
#include <vector>

inline constexpr unsigned terrainSurfaceSize=256;
// RGBA data: dirt, paving, ambient shelter, woodland litter. Extent matches atlas.
std::vector<std::uint8_t> BuildTerrainSurface(const rpg::WorldMap& map);
