#pragma once
#include "game/Population.h"
#include "engine/Renderer.h"
namespace populationview {
inline authored::Asset Asset(rpg::population::Prop p) {
  using P=rpg::population::Prop;using A=authored::Asset;
  switch(p) {
  case P::Barrel:return A::Barrel;case P::Crate:return A::Crate;case P::Chest:return A::Chest;
  case P::Cart:return A::Handcart;case P::Well:return A::Well;case P::Bench:return A::Bench;
  case P::Lantern:return A::Lantern;case P::Brazier:return A::Brazier;case P::Anvil:return A::Anvil;
  case P::Logs:return A::LogPile;case P::Sacks:return A::SackStack;case P::Sign:return A::Signpost;
  case P::Fence:return A::Fence;case P::Stump:return A::Stump;default:return A::Gravestone;
  }
}
inline void Scenery(Renderer& r) {
  for(const auto& p:rpg::population::Props()) {
    const auto elevation=r.Elevate(rpg::WorldMap::Height(p.position));
    r.DrawAuthoredAsset(Asset(p.kind),{p.position.x,0,p.position.z},{p.scale,p.scale,p.scale},p.yaw);
  }
}
inline void Embers(Renderer& r,rpg::Vec2 player,float time) {
  for(const auto& d:rpg::population::districts) {
    const rpg::Vec2 p{d.yard.x+4,d.yard.z+5};
    if(std::hypot(p.x-player.x,p.z-player.z)>32)continue;
    const float y=rpg::WorldMap::Height(p)+.7f;
    for(int i=0;i<4;++i) {
      const float age=std::fmod(time*.5f+i*.25f,1.f),x=p.x+std::sin(time+i*2)*.12f,z=p.z+std::cos(time*.7f+i)*.10f;
      r.DrawEffectRibbon({x-.018f,y+age,z},{x+.018f,y+age,z},{x+.012f,y+age+.06f,z},{x-.012f,y+age+.06f,z},
        {.85f,.35f,.08f,(1-age)*.6f});
    }
  }
}
}
