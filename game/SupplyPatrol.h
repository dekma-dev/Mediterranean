#pragma once
#include "game/WorldMap.h"
#include <algorithm>
#include <cmath>

namespace rpg {
// Out-and-back patrols leave the stores unattended for observable intervals.
// The static route is checked against the same geometry used by actor movement.
inline Vec2 SupplyPatrolEnd(const WorldMap& map,Vec2 home,Vec2 cache) {
  const float heading=std::atan2(home.x-cache.x,home.z-cache.z);
  const auto storesDistance=[](Vec2 p) {
    float nearest=10000;
    for(int i=6;i<9;++i){const auto q=WorldMap::expeditionSites[i];nearest=std::min(nearest,std::hypot(p.x-q.x,p.z-q.z));}
    return nearest;
  };
  Vec2 best=home;float clearance=storesDistance(home);
  for(float length:{10.f,8.f,6.f}) for(float turn:{0.f,.35f,-.35f,.7f,-.7f,1.05f,-1.05f,1.4f,-1.4f,1.8f,-1.8f}) {
    const Vec2 p{home.x+std::sin(heading+turn)*length,home.z+std::cos(heading+turn)*length};
    const float distance=storesDistance(p);
    if(distance>clearance && map.CanTravel(home,p,.45f)) {best=p;clearance=distance;}
  }
  return best;
}
}
