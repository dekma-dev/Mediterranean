#include "game/TerrainSurface.h"
#include <algorithm>
#include <cmath>

std::vector<std::uint8_t> BuildTerrainSurface(const rpg::WorldMap& map) {
  std::vector<std::uint8_t> pixels(terrainSurfaceSize*terrainSurfaceSize*4);
  for (unsigned row=0;row<terrainSurfaceSize;++row) for (unsigned column=0;column<terrainSurfaceSize;++column) {
    const rpg::Vec2 p{((column+.5f)/terrainSurfaceSize-.5f)*rpg::WorldMap::surveyWidth,
      (.5f-(row+.5f)/terrainSurfaceSize)*rpg::WorldMap::surveyDepth};
    const auto surface=map.SurfaceAt(p);
    const auto offset=(row*terrainSurfaceSize+column)*4;
    pixels[offset+1]=static_cast<std::uint8_t>(std::lround(surface.paving*255));
    pixels[offset]=static_cast<std::uint8_t>(std::min(std::lround(surface.dirt*255),255L-pixels[offset+1]));
    pixels[offset+2]=0; pixels[offset+3]=255;
  }
  return pixels;
}
