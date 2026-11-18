#include "game/ImpactPressure.h"
#include "game/SpellVfx.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
namespace impactfx {
Pressure PressureAt(const rpg::CombatVisualEvent& e,float age){
 using C=rpg::ClassId;using K=rpg::CombatVisualKind;
 Pressure p;
 if(!e.primary || e.evaded || (e.kind!=K::Hit && e.kind!=K::HeavyHit) ||
   !std::isfinite(age)||age<0 || !std::isfinite(e.position.x)||!std::isfinite(e.position.z)||
   !std::isfinite(e.height)||!std::isfinite(e.facing))return p;
 const bool heavy=e.kind==K::HeavyHit || e.heavy || e.signature || e.chain>=2;
 float life=.66f,reach=heavy?3.25f:1.65f;DirectX::XMFLOAT3 aspect{1,.78f,1};
 if(e.school==C::Warrior){life=.42f;reach=heavy?2.8f:1.3f;aspect={1,.42f,.58f};p.tint={.62f,.59f,.49f,1};}
 else if(e.school==C::Scout){life=.32f;reach=heavy?1.9f:1.1f;aspect={.40f,.42f,1};p.tint={.53f,.52f,.42f,1};}
 else if(e.school==C::Mage){
  const auto kind=spellvfx::MakeResidue(e).kind;
  p.style=kind==spellvfx::ResidueKind::Ember || kind==spellvfx::ResidueKind::Thermal?1u:2u;
  p.tint=p.style==1?DirectX::XMFLOAT4{.66f,.40f,.22f,1}:kind==spellvfx::ResidueKind::Frost?
    DirectX::XMFLOAT4{.49f,.72f,.84f,1}:DirectX::XMFLOAT4{.59f,.47f,.76f,1};
 }else if(e.school==C::Priest){life=.75f;reach=heavy?3.1f:1.8f;p.style=2;p.tint={.83f,.75f,.50f,1};aspect.y=.66f;}
 else return p;
 if(age>=life)return {};
 const float t=age/life,fade=std::min(1.f,age/.028f)*std::pow(1-t,1.4f);
 const float radius=.16f+reach*(1-std::pow(1-t,2.4f));
 p.center={e.position.x,e.height,e.position.z};p.radii={radius*aspect.x,radius*aspect.y,radius*aspect.z};
 p.strength=fade*(heavy?1.f:.55f);p.age=age;p.facing=e.facing;
 const float altitude=e.height-rpg::WorldMap::Height(e.position),groundAge=age-.045f-std::max(0.f,altitude)*.025f;
 if(heavy && groundAge>0 && altitude<3.f){
  p.groundRadius=.18f+reach*(1-std::exp(-groundAge*7));
  p.groundOpacity=std::min(1.f,groundAge/.06f)*std::pow(1-t,1.1f)*.62f;
 }
 return p;
}
void DrawPressure(Renderer& r,const Pressure& p,bool ground){
 if(p.strength<=0)return;
 r.DrawPressureWave(p.center,p.radii,p.facing,p.age,p.strength,p.style);
 if(!ground || p.groundRadius<=0 || p.groundOpacity<=0 || r.EffectVertices()+240>Renderer::maxEffectVertices)return;
 // A low rolling front follows actual terrain. Broken highlights avoid a
 // solid targeting circle; this moving cosmetic never defines damage range.
 constexpr int pieces=20;constexpr float tau=6.2831853f;
 const float width=.12f+p.age*.60f;
 const auto section=[&](float angle,float radius,float rise){
  const float x=p.center.x+std::sin(angle)*radius,z=p.center.z+std::cos(angle)*radius;
  return DirectX::XMFLOAT3{x,rpg::WorldMap::Height({x,z})+rise,z};
 };
 for(int n=0;n<pieces;++n){const float a=n*tau/pieces,b=(n+1)*tau/pieces;
  const float roll=.13f+p.age*.24f,opacity=p.groundOpacity*(.64f+.36f*std::sin(a*3+p.facing)*std::sin(a*3+p.facing));
  auto color=p.tint;color.w=opacity;
  const auto outerA=section(a,p.groundRadius+width*.5f,.025f),outerB=section(b,p.groundRadius+width*.5f,.025f);
  const auto innerA=section(a,p.groundRadius-width,roll),innerB=section(b,p.groundRadius-width,roll);
  r.DrawPressureRibbon(outerA,innerA,innerB,outerB,a*p.groundRadius,b*p.groundRadius,p.age,color);
  if(n%3!=1){const float thin=.025f+p.age*.04f;color.w=opacity*(p.style?.64f:.28f);
   r.DrawFilamentRibbon(section(a,p.groundRadius,.055f),section(a,p.groundRadius-thin,.065f),
     section(b,p.groundRadius-thin,.065f),section(b,p.groundRadius,.055f),a*p.groundRadius,b*p.groundRadius,p.age,color,.5f,.5f);}
 }
}
}
