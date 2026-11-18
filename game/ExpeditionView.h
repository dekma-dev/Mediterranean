#pragma once
#include "game/Simulation.h"
#include "game/WorldInteraction.h"
#include "engine/Renderer.h"

namespace expeditionview {
inline constexpr rpg::Vec2 camps[]={{-12,53},{-79,-66},{79,-87}};
// Dress the accepted authored shelters inside their existing solid footprints.
// No replacement buildings, textures or new invisible collision are introduced.
inline void CampDetails(Renderer& r) {
  using M=visual::Mesh;using S=visual::Material;
  for(int camp=0;camp<3;++camp) {
    const auto p=camps[camp];const auto elevation=r.Elevate(rpg::WorldMap::Height(p));
    const DirectX::XMFLOAT4 iron{.16f,.17f,.16f,1};
    // Wall-mounted charcoal basket above head height, inside the camera apron.
    const float x=p.x+1.65f,z=p.z-2.26f;
    r.DrawAuthoredAsset(authored::Asset::Brazier,{x,2.10f,z},{.65f,.73f,.65f});
    for(int leg=0;leg<3;++leg) {
      const float a=leg*2.0943951f;
      r.DrawSegment({x+std::sin(a)*.16f,2.4f,z+std::cos(a)*.16f},
        {x,2.12f,p.z-2.09f},.035f,.035f,iron,S::Metal);
    }
    // A faded narrow standard with separated, uneven hems instead of a flat
    // bright rectangle. The middle slit and banding remain visible at game view.
    const float front=p.z-2.34f;
    r.DrawSegment({p.x-.52f,3.1f,front},{p.x+.52f,3.1f,front},.055f,.055f,iron,S::Metal);
    for(int strip=0;strip<8;++strip) {
      const float sx=p.x-.42f+strip*.12f,height=1.05f+.055f*((strip*3)%4);
      const DirectX::XMFLOAT4 cloth=camp==0?DirectX::XMFLOAT4{.24f,.27f,.28f,1}:
        camp==1?DirectX::XMFLOAT4{.25f,.28f,.22f,1}:DirectX::XMFLOAT4{.31f,.19f,.15f,1};
      r.DrawMesh(M::Box,{sx,3.07f-height*.5f,front-.035f*std::sin(strip*1.8f)},
        {.116f,height,.035f},cloth,S::Cloth);
      r.DrawMesh(M::Box,{sx,2.48f,front-.065f},{.116f,.085f,.018f},{.40f,.35f,.24f,1},S::Cloth);
    }
  }
}
inline void Draw(Renderer& r,const rpg::Simulation& s,float time) {
  for(int i=0;i<3;++i) {
    const auto p=rpg::WorldMap::expeditionSites[6+i];
    if(std::hypot(p.x-s.Player().x,p.z-s.Player().z)>65)continue;
    const float ground=rpg::WorldMap::Height(p);
    if(!ui::SupplyRecovered(s,i))
      r.DrawAuthoredAsset(authored::Asset::SackStack,{p.x+.42f,ground,p.z+.16f},{.65f,.65f,.65f});
    if(s.SupplyChannelSite()!=i)continue;
    const float progress=std::clamp(s.SupplyChannelProgress(),0.f,1.f);
    const auto at=[&](float angle,float radius)->DirectX::XMFLOAT3 {
      const rpg::Vec2 q{p.x+std::sin(angle)*radius,p.z+std::cos(angle)*radius};
      return {q.x,rpg::WorldMap::Height(q)+.065f,q.z};
    };
    // Terrain-following interaction progress, separate from combat warnings.
    for(int n=0;n<32;++n) {
      const float a=n*6.2831853f/32,b=(n+1)*6.2831853f/32;
      r.DrawEffectRibbon(at(a,.83f),at(a,.88f),at(b,.88f),at(b,.83f),{.44f,.35f,.20f,.23f});
      const float end=std::min(b,progress*6.2831853f);
      if(end>a)r.DrawEffectRibbon(at(a,.83f),at(a,.88f),at(end,.88f),at(end,.83f),{.85f,.65f,.32f,.75f});
    }
  }
  for(const auto p:camps) {
    if(std::hypot(p.x-s.Player().x,p.z-s.Player().z)>38) continue;
    const float x=p.x+1.65f,z=p.z-2.26f,y=rpg::WorldMap::Height(p)+2.60f;
    for(int spark=0;spark<5;++spark) {
      const float phase=std::fmod(time*.6f+spark*.193f,1.f),a=spark*2.4f;
      const DirectX::XMFLOAT3 pos{x+std::sin(a+phase)*.13f,y+phase*.55f,z+std::cos(a)*.12f};
      r.DrawEffectStreak(pos,{pos.x+.015f,pos.y+.05f,pos.z},.013f,{.78f,.34f,.08f,(1-phase)*.35f});
    }
  }
}
}
