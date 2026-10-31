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
    // Bake broad terrain shelter into the unused blue channel. Only ambient
    // light is attenuated; the sun still uses the real shadow maps. There is
    // no per-frame heightfield sampling or additional GPU texture allocation.
    float shelter=0;const float floor=map.Height(p);
    for(int direction=0;direction<8;++direction) {
      const float angle=direction*6.2831853f/8;
      float horizon=0;
      for(float distance:{6.f,14.f}) {
        const float rise=map.Height({p.x+std::cos(angle)*distance,p.z+std::sin(angle)*distance})-floor;
        horizon=std::max(horizon,std::max(0.f,rise/distance));
      }
      shelter+=horizon;
    }
    pixels[offset+2]=static_cast<std::uint8_t>(std::lround(std::min(.35f,shelter*.1f)*255));
    pixels[offset+3]=255;
  }
  return pixels;
}
