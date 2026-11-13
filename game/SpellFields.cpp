#include "game/SpellVfx.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
namespace spellvfx {
namespace {
using namespace DirectX;constexpr float pi=3.14159265f,tau=2*pi;
XMFLOAT3 Ground(rpg::Vec2 p,float lift=0){return {p.x,rpg::WorldMap::Height(p)+lift,p.z};}
XMFLOAT3 Add(XMFLOAT3 a,XMFLOAT3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
}
void GroundField(Renderer& r,rpg::Vec2 p,float radius,float age,ResidueKind kind,float opacity,unsigned seed){
 if(!std::isfinite(p.x+p.z+radius+age+opacity) || radius<=0 || radius>6 || age<0 || opacity<=0 || kind==ResidueKind::None)return;
 const auto medium=kind==ResidueKind::Frost?Renderer::SpellField::Frost:kind==ResidueKind::Holy?Renderer::SpellField::Holy:
   kind==ResidueKind::Arcane?Renderer::SpellField::Arcane:Renderer::SpellField::Ember;
 std::array<float,25> heights{};
 for(int z=0;z<5;++z)for(int x=0;x<5;++x)heights[z*5+x]=rpg::WorldMap::Height({p.x+(x*.5f-1)*radius,p.z+(z*.5f-1)*radius});
 r.DrawSpellField(medium,{p.x,p.z},radius,age,{1,1,1,opacity},heights,seed);
}
void Aftermath(Renderer& r,const Residue& e){
 const float life=ResidueLifetime(e.kind);
 if(!std::isfinite(e.age+e.position.x+e.position.z+e.size+e.facing) || e.age<0 || e.age>=life || e.size<=0 ||
    r.EffectVertices()+180>Renderer::maxEffectVertices)return;
 const float age=e.age,size=std::clamp(e.size,.3f,2.f),fade=combat::Smooth(age/.15f)*(1-combat::Smooth(age/life));
 GroundField(r,e.position,size,age,e.kind,fade*(e.kind==ResidueKind::Holy?.90f:.76f),e.seed);
 const auto point=[&](float a,float radius,float y){return Ground({e.position.x+std::cos(a)*radius*size,e.position.z+std::sin(a)*radius*size},y);};
 if(e.kind==ResidueKind::Holy || e.kind==ResidueKind::Arcane){
  const bool holy=e.kind==ResidueKind::Holy;
  for(int n=0;n<6;++n){const float phase=std::fmod(age*.55f+n/6.f,1.f),angle=n*tau/6+e.facing;
   const auto a=point(angle,.60f,.08f+phase*1.15f),b=point(angle+.045f,.60f,.23f+phase*1.15f);
   const auto c=point(angle+.10f,.58f,.40f+phase*1.15f);const float light=fade*std::sin(phase*pi);
   const XMFLOAT4 color=holy?XMFLOAT4{1,.86f,.46f,light*.52f}:XMFLOAT4{.65f,.47f,1,light*.50f};
   r.DrawEffectStreak(a,b,.020f,color);r.DrawEffectStreak(b,c,.009f,color);
  }
  return;
 }
 const bool frost=e.kind==ResidueKind::Frost,mixed=e.kind==ResidueKind::Thermal;
 for(int n=0;n<4;++n){const float angle=n*2.39996f+e.seed*.013f,phase=std::fmod(age*.45f+n*.25f,1.f);
  const auto base=point(angle,.45f+phase*.20f,.06f);
  if(frost || (mixed && n%2==0)){
   const float h=(.17f+n*.04f)*(1-combat::Smooth(age/life));
   const auto tip=Add(base,{.06f,h,.035f});
   // Two crossed spatial facets; afterglow stays within the old 180-vertex
   // allowance, independent of the larger active-field sculpture.
   for(int face=0;face<2;++face){const float a=angle+face*pi*.5f;
    r.DrawEffectSurface(Renderer::EffectArt::Ice,Add(base,{-std::cos(a)*.07f,0,-std::sin(a)*.07f}),tip,
      Add(base,{std::cos(a)*.07f,0,std::sin(a)*.07f}),Add(base,{0,-.015f,0}),{.35f,.73f,1,fade*.62f});}
  }else{
   const auto ember=Add(base,{std::sin(angle)*phase*.17f,.10f+phase*.6f,std::cos(angle)*phase*.17f});
   r.DrawEffectStreak(Add(ember,{0,-.055f,0}),ember,.012f,{1,.48f,.075f,fade*std::sin(phase*pi)*.85f});
  }
  if(n<2)r.DrawEffectSprite(Renderer::EffectArt::Smoke,Add(base,{0,.12f+age*.20f,0}),.23f+age*.15f,.15f+age*.13f,
    angle+age*.12f,frost?XMFLOAT4{.34f,.60f,.77f,fade*.25f}:XMFLOAT4{.20f,.16f,.12f,fade*.28f},true);
 }
}
void FrostField(Renderer& r,rpg::Vec2 p,float age,float remaining){
 if(!std::isfinite(age+remaining+p.x+p.z) || age<0 || remaining<=0)return;
 const float fade=combat::Smooth(age/.22f)*combat::Smooth(remaining/.48f),radius=rpg::Simulation::frostFieldRadius;
 GroundField(r,p,radius,age,ResidueKind::Frost,fade*.88f,17);
 // One continuous silhouette follows the actual three-metre gameplay edge.
 for(int n=0;n<48;++n){const float a=n*tau/48,b=(n+1)*tau/48;
  const auto at=[&](float angle,float rad){return Ground({p.x+std::cos(angle)*rad,p.z+std::sin(angle)*rad},.065f);};
  r.DrawEffectRibbon(at(a,radius-.035f),at(a,radius),at(b,radius),at(b,radius-.035f),{.30f,.67f,1,fade*.46f});}
 for(int n=0;n<10;++n){const float angle=n*2.39996f,spread=1.10f+(n%3)*.58f;
  const auto base=Ground({p.x+std::cos(angle)*spread,p.z+std::sin(angle)*spread},.035f);
  const float growth=combat::Smooth((age-.04f*(n%3))/.28f)*combat::Smooth(remaining/.55f);
  const float h=(.42f+(n%3)*.22f)*growth,w=.09f+float(n%2)*.04f;
  const auto tip=Add(base,{std::cos(angle)*h*.32f,h,std::sin(angle)*h*.32f});
  for(int face=0;face<4;++face){const float a=angle+face*pi*.5f,b=a+pi*.5f;
   const auto x=Add(base,{std::cos(a)*w,h*.20f,std::sin(a)*w}),y=Add(base,{std::cos(b)*w,h*.20f,std::sin(b)*w});
   r.DrawEffectSurface(Renderer::EffectArt::Ice,base,x,tip,y,{.28f,.64f,.96f,fade*.70f});
   if(face==0)r.DrawEffectStreak(x,tip,.012f,{.70f,.94f,1,fade*.75f});}
 }
 // Low, advecting wisps stay inside the field. Deterministic phases are driven
 // by the spell's own age, never a frame counter or the global scene clock.
 for(int n=0;n<6;++n){const float phase=std::fmod(age*.28f+n/6.f,1.f),a=n*tau/6+phase*.65f;
  auto at=Ground({p.x+std::cos(a)*(1.1f+phase*.8f),p.z+std::sin(a)*(1.1f+phase*.8f)},.16f+phase*.20f);
  r.DrawEffectSprite(Renderer::EffectArt::Smoke,at,.55f+phase*.35f,.18f+phase*.17f,a,{.34f,.60f,.78f,fade*std::sin(phase*pi)*.33f},true);
  const auto tip=Add(at,{-.10f*std::sin(a),.07f,.10f*std::cos(a)});
  r.DrawEffectStreak(at,tip,.012f,{.68f,.91f,1,fade*std::sin(phase*pi)*.62f});
 }
}
}
