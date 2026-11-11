#pragma once
#include "game/WorldMap.h"
#include "engine/Renderer.h"

namespace locationview {
inline authored::Asset DetailAsset(rpg::LandscapeKind kind) {
  using K=rpg::LandscapeKind;using A=authored::Asset;
  switch(kind) {
  case K::Crate:return A::Crate;case K::Cart:return A::Handcart;
  case K::Well:return A::Well;case K::Bench:return A::Bench;
  case K::Logs:return A::LogPile;case K::Sacks:return A::SackStack;
  case K::Fence:return A::Fence;case K::Lantern:return A::Lantern;
  case K::Stump:return A::Stump;default:return A::Barrel;
  }
}
inline void Details(Renderer& r,const rpg::WorldMap& map) {
  using K=rpg::LandscapeKind;using A=authored::Asset;
  for(const auto& detail:map.Details()) {
    const auto p=detail.position;const float scale=detail.scale;
    const auto elevation=r.Elevate(map.Height(p));
    if(detail.kind==K::Boulder) {
      // Three buried, overlapping strata occupy one shared physical footprint.
      // Low shelves blend into the bank instead of floating on its centre.
      for(int layer=0;layer<3;++layer) {
        const float width=1.36f-layer*.24f,depth=1.06f-layer*.13f;
        r.DrawMesh(visual::Mesh::Rock,{p.x,(-.04f+layer*.20f)*scale,p.z},
          {width*scale,.48f*scale,depth*scale},{.37f+layer*.02f,.37f+layer*.018f,.32f+layer*.013f,1},
          visual::Material::Stone,detail.yaw+layer*.15f);
      }
      continue;
    }
    r.DrawAuthoredAsset(DetailAsset(detail.kind),{p.x,0,p.z},{scale,scale,scale},detail.yaw);
  }
}
inline void Lanterns(Renderer& r,const rpg::WorldMap& map,rpg::Vec2 observer,float time) {
  if(!std::isfinite(time))return;
  struct Lamp {DirectX::XMFLOAT3 p;float distance;};
  std::array<Lamp,4> nearest{};unsigned count=0;
  const auto consider=[&](DirectX::XMFLOAT3 p) {
    const float distance=std::hypot(p.x-observer.x,p.z-observer.z);if(distance>=28)return;
    Lamp lamp{p,distance};
    if(count<nearest.size())nearest[count++]=lamp;
    else if(distance<nearest[count-1].distance)nearest[count-1]=lamp;
    else return;
    std::sort(nearest.begin(),nearest.begin()+count,[](const Lamp& a,const Lamp& b){return a.distance<b.distance;});
  };
  for(const auto& o:map.Obstacles())if(o.kind==rpg::PropKind::Building)
    consider({o.center.x+o.halfX*.55f,map.Height(o.center)+std::min(1.75f,o.height*.68f),o.center.z-o.halfZ-.13f});
  for(const auto& p:map.Details())if(p.kind==rpg::LandscapeKind::Lantern)
    consider({p.position.x,map.Height(p.position)+.28f*p.scale,p.position.z});
  for(unsigned i=0;i<count;++i) {
    const auto& lamp=nearest[i];const auto p=lamp.p;
    const float phase=std::fmod(time,120.f),flicker=.88f+.07f*std::sin(phase*5.1f+p.x)+.05f*std::sin(phase*8.3f+p.z);
    const float fade=1-rpg::terrain::Smooth((lamp.distance-18)/10);
    r.DrawEffectSprite(Renderer::EffectArt::Flame,p,.035f,.07f*flicker,0,{1,.58f,.18f,.55f*fade});
    r.DrawEffectGlow(p,.11f,{1,.57f,.16f,.045f*fade});
    // Low-power practical lights share the existing top-four budget. Nearby
    // spell sources outrank them; no new shadow maps, textures or per-frame heap.
    r.AddCombatLight(p,3.5f,{1,.52f,.17f},.55f*flicker*fade);
  }
}
}
