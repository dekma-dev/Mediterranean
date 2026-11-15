#pragma once
#include "engine/Renderer.h"
#include "game/WorldMap.h"
#include <cmath>

namespace presentation {
inline Renderer::AttackSurface AttackSurface(rpg::Vec2 center,float extent) {
  Renderer::AttackSurface surface;
  if(!std::isfinite(center.x+center.z+extent) || extent<=0 || extent>9.16f)return surface;
  surface.step=rpg::terrain::cell;
  surface.origin={rpg::terrain::left+std::floor((center.x-extent-rpg::terrain::left)/surface.step)*surface.step,
    rpg::terrain::front+std::floor((center.z-extent-rpg::terrain::front)/surface.step)*surface.step};
  surface.columns=unsigned(std::ceil((center.x+extent-surface.origin.x)/surface.step))+1;
  surface.rows=unsigned(std::ceil((center.z+extent-surface.origin.y)/surface.step))+1;
  if(surface.columns>11 || surface.rows>11){surface.columns=surface.rows=0;return surface;}
  for(unsigned z=0;z<surface.rows;++z)for(unsigned x=0;x<surface.columns;++x)
    surface.heights[z*surface.columns+x]=rpg::WorldMap::Height({surface.origin.x+x*surface.step,surface.origin.y+z*surface.step});
  return surface;
}
}
