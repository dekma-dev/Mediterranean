#pragma once
#include "game/Population.h"
#include <cmath>

namespace rpg::grove {
// Placement metadata for the frozen collection exports (oak / pine / dead).
// Native regression checks these envelopes against all runtime source bounds.
struct Profile {int species;float halfX,halfZ,height;};
inline Profile Tree(const Obstacle& o) {
  const float x=o.center.x,z=o.center.z;const bool mist=x<-65 && z<-60;
  const int variety=int(std::abs(x*7+z*11));
  int species=(mist && variety%3==0) || (z>55 && x>45)?2:x>0 || mist || variety%3?1:0;
  if(o.introducedVersion==22) {
    int district=0;float nearest=10000;
    for(int d=0;d<population::districtCount;++d) {
      const auto p=population::districts[d].habitat;const float distance=std::hypot(x-p.x,z-p.z);
      if(distance<nearest){nearest=distance;district=d;}
    }
    species=district==3?2:district==0 || district==4?0:1;
  }
  constexpr Profile profiles[]={{0,1.913f,1.969f,3.355f},{1,1.109f,1.121f,3.767f},{2,1.327f,1.379f,3.070f}};
  return profiles[species];
}
struct Scale {float width,height;};
inline Scale Size(const Obstacle& o) {
  const auto p=Tree(o);const float radial=std::hypot(p.halfX,p.halfZ);
  const float width=std::min((o.canopyRadius-.1f)/radial,(o.CameraHeight()-.1f)/p.height);
  return {width,std::min(width*1.10f,(o.CameraHeight()-.1f)/p.height)};
}
inline void FitCamera(Obstacle& o) {
  const auto p=Tree(o);const auto size=Size(o);
  o.canopyRadius=std::hypot(p.halfX,p.halfZ)*size.width+.1f;
  o.height=p.height*size.height+.1f-.5f;
}
}
