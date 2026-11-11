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
    float woodland=0,contact=0;
    for(const auto& o:map.Obstacles()) {
      if(o.kind==rpg::PropKind::Tree) {
        const float radius=o.canopyRadius+1.7f,dx=p.x-o.center.x,dz=p.z-o.center.z;
        if(std::abs(dx)>radius || std::abs(dz)>radius)continue;
        const float w=std::max(0.f,1-(dx*dx+dz*dz)/(radius*radius));
        woodland=std::min(1.f,woodland+w*w*.65f);
      } else if(o.kind==rpg::PropKind::Building || o.kind==rpg::PropKind::Dressing || o.kind==rpg::PropKind::Ruin) {
        const float x=std::max(0.f,std::abs(p.x-o.center.x)-o.halfX),z=std::max(0.f,std::abs(p.z-o.center.z)-o.halfZ);
        contact=std::max(contact,std::max(0.f,1-(x*x+z*z)/3.24f)*.16f);
      }
    }
    pixels[offset+2]=static_cast<std::uint8_t>(std::lround(std::min(.35f,shelter*.1f+contact)*255));
    // Alpha is data, not transparency: bounded litter/understory coverage.
    pixels[offset+3]=static_cast<std::uint8_t>(std::lround(woodland*255));
  }
  return pixels;
}
