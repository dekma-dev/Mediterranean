#pragma once
#include "game/Simulation.h"
#include "engine/Renderer.h"

namespace courtview {
// The same sites drive collision-free ground plates, atlas markers and quests.
inline void Plates(Renderer& r) {
  for(const auto p:rpg::WorldMap::courtSeals) {
    const float h=rpg::WorldMap::Height(p);
    r.DrawMesh(visual::Mesh::Rock,{p.x,h-.07f,p.z},{1.45f,.18f,1.45f},{.31f,.32f,.29f,1},visual::Material::Stone);
    for(int spoke=0;spoke<6;++spoke) {
      const float a=spoke*1.04719755f;
      r.DrawSegment({p.x+std::cos(a)*.25f,h+.05f,p.z+std::sin(a)*.25f},
        {p.x+std::cos(a)*.65f,h+.05f,p.z+std::sin(a)*.65f},.035f,.025f,
        {.38f,.29f,.17f,1},visual::Material::Metal);
    }
  }
}
inline void Draw(Renderer& r,const rpg::Simulation& s,float time) {
  if(!s.CourtActive()) return;
  const auto& captain=s.Enemies().back();
  for(int seal=0;seal<3;++seal) {
    const auto p=rpg::WorldMap::courtSeals[seal];
    if(std::hypot(p.x-s.Player().x,p.z-s.Player().z)>65) continue;
    const bool broken=(s.CourtSeals()&(1u<<seal))!=0;
    const DirectX::XMFLOAT4 tint=broken?DirectX::XMFLOAT4{.24f,.40f,.28f,.23f}:
      DirectX::XMFLOAT4{.70f,.28f,.12f,.38f+.07f*std::sin(time*2+seal)};
    const auto at=[&](float angle,float radius) -> DirectX::XMFLOAT3 {
      const rpg::Vec2 v{p.x+std::cos(angle)*radius,p.z+std::sin(angle)*radius};
      return {v.x,rpg::WorldMap::Height(v)+.08f,v.z};
    };
    for(int i=0;i<32;++i) {
      const float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;
      r.DrawEffectRibbon(at(a,.70f),at(a,.77f),at(b,.77f),at(b,.70f),tint);
    }
    if(!broken && captain.health>0) {
      const DirectX::XMFLOAT3 start{p.x,rpg::WorldMap::Height(p)+.2f,p.z};
      const DirectX::XMFLOAT3 end{captain.position.x,rpg::WorldMap::Height(captain.position)+1.2f,captain.position.z};
      // A low arch identifies the source without a solid screen-filling beam.
      DirectX::XMFLOAT3 previous=start;
      for(int i=1;i<=8;++i) {
        const float t=i/8.f;
        const DirectX::XMFLOAT3 next{start.x+(end.x-start.x)*t,start.y+(end.y-start.y)*t+std::sin(t*3.14159265f)*2,start.z+(end.z-start.z)*t};
        r.DrawEffectStreak(previous,next,.025f,{.55f,.20f,.09f,.17f});previous=next;
      }
    }
  }
  if(s.CourtProtected(captain) && captain.health>0)
    r.DrawEffectRing({captain.position.x,rpg::WorldMap::Height(captain.position)+1.1f,captain.position.z},.85f,{.7f,.29f,.1f,.4f});
}
}
