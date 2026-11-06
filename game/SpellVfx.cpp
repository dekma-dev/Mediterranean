#include "game/SpellVfx.h"
#include "game/CombatEffects.h"
#include "game/ClassEffects.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
namespace spellvfx {
namespace {
using namespace DirectX;
using C=rpg::ClassId;using K=rpg::CombatVisualKind;using Art=Renderer::EffectArt;
constexpr float pi=3.14159265f,tau=2*pi;
float Unit(float x){return std::clamp(x,0.f,1.f);}
XMFLOAT3 Add(XMFLOAT3 a,XMFLOAT3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
float Rand(unsigned seed){seed^=seed>>16;seed*=0x7feb352du;seed^=seed>>15;seed*=0x846ca68bu;seed^=seed>>16;return float(seed&65535)/65535;}
enum class Element {Fire,Frost,Arcane,Thermal};
Element School(int ability){return ability==14?Element::Thermal:ability==10 || ability==13?Element::Arcane:
 ability==5 || ability==6 || ability==8 || ability==12?Element::Frost:Element::Fire;}
// Soft attack / slow decay rather than a one-frame billboard at full opacity.
float Envelope(float age,float life,float rise=.025f){return Unit(age/rise)*std::pow(Unit(1-age/life),1.35f);}
XMFLOAT3 Ground(rpg::Vec2 p,float lift=.12f){return {p.x,rpg::WorldMap::Height(p)+lift,p.z};}
void Stamp(Renderer& r,rpg::Vec2 p,float radius,float spin,XMFLOAT4 color){
 const auto corner=[&](float x,float z){return Ground({p.x+(std::cos(spin)*x-std::sin(spin)*z)*radius,p.z+(std::sin(spin)*x+std::cos(spin)*z)*radius},.045f);};
 r.DrawEffectSurface(Art::Seal,corner(-1,-1),corner(-1,1),corner(1,1),corner(1,-1),color);
}
void Dust(Renderer& r,XMFLOAT3 p,float age,float size,unsigned seed,bool stone=false){
 const float opacity=Envelope(age,.72f,.045f);
 for(unsigned n=0;n<3;++n){
  const float a=Rand(seed+n*13)*tau,speed=.4f+Rand(seed+n*27)*.6f;
  const auto at=Add(p,{std::cos(a)*age*speed,age*.18f,std::sin(a)*age*speed});
  const float radius=(.10f+age*.48f)*size;
  r.DrawEffectSprite(stone?Art::Smoke:Art::Dust,at,radius,radius*.72f,a+age*.2f,
    stone?XMFLOAT4{.38f,.35f,.31f,opacity*.30f}:XMFLOAT4{.64f,.55f,.42f,opacity*.34f},true);
 }
}
float Roll(Renderer& r,XMFLOAT3 a,XMFLOAT3 b){
 XMFLOAT2 from,to;if(!r.Project(a,from)||!r.Project(b,to))return 0;
 return std::atan2(from.x-to.x,from.y-to.y);
}
// A release plane faces along the accepted trajectory, not the camera. It
// remains attached to the sampled launch socket even when selection changes.
void Release(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age){
 if(age>=.18f)return;
 const float fade=Envelope(age,.14f,.008f);
 const float sx=std::sin(e.facing),cx=std::cos(e.facing);
 const auto local=[&](float x,float y,float z=0.f){return Add(origin,{cx*x+sx*z,y,-sx*x+cx*z});};
 const bool physical=e.school==C::Scout;
 const auto element=School(e.ability);const bool holy=e.school==C::Priest,cold=!holy && element==Element::Frost,arcane=!holy && element==Element::Arcane;
 const XMFLOAT4 tint=holy?XMFLOAT4{1,.88f,.51f,fade*.7f}:physical?XMFLOAT4{.70f,.83f,.72f,fade*.35f}:cold?XMFLOAT4{.45f,.78f,1,fade*.7f}:
  arcane?XMFLOAT4{.68f,.38f,1,fade*.7f}:XMFLOAT4{1,.47f,.12f,fade*.65f};
 const float radius=(physical?.09f:.25f)+age*(physical?1.f:3.4f);
 r.DrawEffectSurface(physical?Art::Slash:cold?Art::Ice:Art::Seal,local(-radius,-radius),local(-radius,radius),local(radius,radius),local(radius,-radius),tint);
 if(!physical)for(int n=0;n<5;++n){
  const float a=n*tau/5+e.ability*.4f;const auto p=local(std::cos(a)*age*1.3f,std::sin(a)*age*1.3f,-age*.65f);
  r.DrawEffectStreak(p,local(std::cos(a)*age*.75f,std::sin(a)*age*.75f,-age*.3f),.012f,tint);
 }
 if(!physical){
  // Brief material ignition at the real socket; the larger translucent body
  // survives the bright core so a release is readable at a normal camera zoom.
  const float body=Envelope(age,.18f,.015f),core=Envelope(age,.085f,.004f),size=e.heavy?.82f:.58f;
  for(int n=0;n<3;++n){const float a=n*tau/3+age*4;
   const auto p=local(std::cos(a)*age*2.2f,std::sin(a)*age*2.2f,-age*.4f);
   r.DrawEffectSprite(holy?Art::Slash:cold?Art::Ice:arcane?Art::Arcane:Art::Flame,p,size*.42f,size*.68f,a*.15f,
    holy?XMFLOAT4{1,.89f,.56f,body*.47f}:cold?XMFLOAT4{.46f,.79f,1,body*.62f}:arcane?XMFLOAT4{.70f,.43f,1,body*.52f}:XMFLOAT4{1,.80f,.43f,body*.7f},!holy&&!cold&&!arcane);
  }
  r.DrawEffectSprite(Art::Sparks,origin,size,size,0,{1,.94f,.78f,core*.85f});
 }
}
void MageContact(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 p,float age,float size){
 const auto element=School(e.ability);const bool frost=element==Element::Frost,arcane=element==Element::Arcane,mixed=element==Element::Thermal;
 const float fade=Envelope(age,ContactLifetime(e)),burst=Envelope(age,.20f,.008f);
 if(arcane){
  for(int n=0;n<2;++n){const float radius=(.15f+age*(n?.75f:1.4f))*size;
   r.DrawEffectSprite(Art::Arcane,p,radius,radius,age*(n?-2.f:2.8f)+n*pi,{.85f,.72f,1,fade*(n?.28f:.65f)});}
  r.DrawEffectSprite(Art::Seal,p,(.25f+age*.8f)*size,(.25f+age*.8f)*size,-age*.7f,{.8f,.58f,1,fade*.48f});
  if(e.ability==13) {
   // The mobile prism fractures into three offset planes. The central gap
   // keeps the struck actor visible; this is distinct from the interrupt orb.
   for(int n=0;n<3;++n) {
    const float angle=e.facing+n*tau/3+age*.65f,opening=combat::Smooth(age/.22f);
    const XMFLOAT3 side{std::cos(angle),0,-std::sin(angle)};
    const auto at=[&](float x,float y){return Add(p,{side.x*(x+opening*.40f)*size,y*size,side.z*(x+opening*.40f)*size});};
    const auto a=at(-.16f,-.35f),b=at(-.22f,.35f),c=at(.16f,.62f),d=at(.22f,-.22f);
    r.DrawEffectSurface(Art::Arcane,a,b,c,d,{.51f,.37f,1,fade*.38f});
    r.DrawEffectStreak(a,b,.012f,{.78f,.88f,1,fade*.72f});
    r.DrawEffectStreak(b,c,.012f,{.89f,.76f,1,fade*.78f});
   }
  }
 }else{
  const unsigned seed=e.actor*37u+unsigned(std::max(e.ability,0))*71u;
  for(unsigned n=0;n<6;++n){
   const float a=n*tau/6+Rand(seed+n)*.35f,speed=.55f+Rand(seed+n*17)*.75f;
   const bool ice=frost || (mixed && n%2==0);
   auto at=Add(p,{std::cos(a)*age*speed*size,ice?std::sin(a)*age*speed*size-1.3f*age*age:age*(.35f+speed*.3f),std::sin(a)*age*.38f*size});
   if(ice){
    const float shard=(.14f+Rand(seed+n*3)*.12f)*size;
    r.DrawEffectSprite(Art::Ice,at,shard*.33f,shard*(1+age*.25f),a+age*.6f,{.6f,.85f,1,fade*.78f});
   }else{
    const float radius=(.20f+age*.43f)*size;
    r.DrawEffectSprite(Art::Flame,at,radius*.62f,radius, std::sin(a+age*3)*.22f,{1,.86f,.63f,fade*.68f},true);
   }
  }
  const float residue=Envelope(age-.09f,ContactLifetime(e)-.09f,.09f);
  for(int n=0;n<2;++n){
   const auto smoke=Add(p,{(n?1.f:-1.f)*age*.30f,age*.52f,age*.10f});
   const float radius=(.12f+age*.55f)*size;
   r.DrawEffectSprite(Art::Smoke,smoke,radius,radius*.75f,n*2.f+age*.12f,
     frost?XMFLOAT4{.64f,.78f,.88f,residue*.24f}:XMFLOAT4{.30f,.26f,.23f,residue*.24f},true);
  }
 }
 r.DrawEffectSprite(frost?Art::Ice:Art::Sparks,p,.38f*size,.38f*size,.7f,
   frost?XMFLOAT4{.52f,.85f,1,burst*.9f}:XMFLOAT4{1,.85f,.62f,burst*.8f});
 if(e.heavy || e.chain>=2){
  const auto c=arcane?XMFLOAT4{.75f,.42f,1,fade*.25f}:frost || mixed?XMFLOAT4{.42f,.80f,1,fade*.23f}:XMFLOAT4{1,.62f,.23f,fade*.20f};
  Stamp(r,e.position,(.45f+age*.42f)*size,e.facing+age*.18f,c);
 }
}
void GroundWave(Renderer& r,rpg::Vec2 centre,float radius,float age,XMFLOAT4 color){
 const float width=.055f+.045f*combat::Smooth(age/.3f);
 for(int n=0;n<32;++n){
  const auto at=[&](float t,float edge){const float a=t*tau/32;return Ground({centre.x+std::sin(a)*(radius+edge),centre.z+std::cos(a)*(radius+edge)},.065f);};
  r.DrawEffectRibbon(at(float(n),-width),at(float(n),width),at(float(n+1),width),at(float(n+1),-width),color);
 }
}
void MajorContact(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 p,float age){
 if(!e.primary)return;
 const auto floor=Ground(e.position);const float bloom=Envelope(age,.86f,.028f);
 if(e.school==C::Mage && (e.ability==8 || e.ability==9 || e.ability==11)){
  const bool frost=e.ability==8,storm=e.ability==11;
  const float radius=(storm?4.5f:3.f)*combat::Smooth(age/(frost?.48f:.38f));
  GroundWave(r,e.position,std::max(.1f,radius),age,frost?XMFLOAT4{.43f,.80f,1,bloom*.55f}:XMFLOAT4{1,.48f,.10f,bloom*.52f});
  const int count=storm?10:8;
  for(int n=0;n<count;++n){
   const float a=n*tau/count+e.facing,delay=(n%3)*.025f,t=age-delay;
   if(t<=0)continue;
   const float fade=Envelope(t,.78f,.055f),spread=frost?1.05f+combat::Smooth(t/.38f)*1.65f:storm?(.7f+(n%3)*.85f):.45f+t*1.8f;
   const auto base=Ground({e.position.x+std::cos(a)*spread,e.position.z+std::sin(a)*spread});
   if(frost){
    const float h=(.50f+(n%3)*.25f)*combat::Smooth(t/.12f);
    const auto top=Add(base,{std::cos(a)*.2f,h,std::sin(a)*.2f});
    r.DrawEffectSprite(Art::Ice,Add(base,{0,h*.45f,0}),.16f,h*.66f,a*.12f,{.51f,.80f,1,fade*.82f});
    r.DrawEffectStreak(base,top,.024f,{.82f,.96f,1,fade*.65f});
   }else{
    const float height=(storm?1.1f:1.35f)+(n%3)*.35f;
    const auto at=Add(base,{std::sin(t*3+a)*.12f,height*.48f+t*.3f,0});
    r.DrawEffectSprite(Art::Flame,at,.30f+t*.26f,height*.64f,.12f*std::sin(a+t*2),{1,.79f,.43f,fade*.76f},true);
    r.DrawEffectSprite(Art::Smoke,Add(at,{0,.35f,0}),.35f+t*.4f,.4f+t*.3f,a,{.28f,.24f,.20f,fade*.22f},true);
   }
  }
  if(!frost){
   r.DrawEffectSprite(Art::Sparks,Add(floor,{0,.55f,0}),1.0f,1.1f,0,{1,.88f,.65f,Envelope(age,.24f,.01f)*.8f});
   Dust(r,floor,age,2.7f,e.actor,true);
  }
 }else if(e.school==C::Mage && e.ability==14){
  // Thermal rupture meets hot and cold material in two opposite swept lobes.
  for(int side:{-1,1})for(int n=0;n<6;++n){const float angle=e.facing+side*(n*.30f+age*2.2f),radius=.35f+age*2.1f;
   const auto at=Add(p,{std::cos(angle)*radius,std::sin(angle)*radius*.65f,side*.20f});
   const float size=(.24f+age*.20f)*(1-n*.065f);
   r.DrawEffectSprite(side<0?Art::Ice:Art::Flame,at,side<0?size*.4f:size*.8f,size*1.6f,angle,
    side<0?XMFLOAT4{.52f,.83f,1,bloom*.75f}:XMFLOAT4{1,.79f,.47f,bloom*.75f},side>0);}
  r.DrawEffectSprite(Art::Smoke,Add(p,{0,age*.7f,0}),.6f+age,.5f+age*.6f,age*.3f,{.64f,.72f,.77f,bloom*.26f},true);
 }else if(e.school==C::Priest && e.ability==6){
  GroundWave(r,e.position,.25f+1.5f*combat::Smooth(age/.50f),age,{1,.86f,.48f,bloom*.55f});
  const float cs=std::cos(e.facing),sn=std::sin(e.facing);
  for(int side:{-1,1})for(int n=0;n<5;++n){
   const float opening=combat::Smooth(age/.16f),x=side*(.40f+n*.22f)*opening;
   const auto base=Add(p,{cs*x,.30f+n*.11f,-sn*x});
   const auto tip=Add(base,{cs*side*(.32f+n*.07f),.55f-n*.06f,-sn*side*(.32f+n*.07f)});
   r.DrawEffectStreak(base,tip,.065f,{1,.91f,.62f,bloom*.7f});
   r.DrawEffectSprite(Art::Slash,base,.10f,.42f,side*(.55f+n*.10f),{1,.89f,.48f,bloom*.45f});
  }
  for(int n=-1;n<=1;++n){const auto base=Add(floor,{cs*n*.5f,0,-sn*n*.5f});
   r.DrawEffectStreak(Add(base,{0,3.5f,0}),Add(base,{0,.3f,0}),n?.08f:.14f,{1,.96f,.78f,bloom*(n?.4f:.65f)});}
 }else if(e.school==C::Priest && e.ability==5){
  const XMFLOAT3 end{e.source.x,e.sourceHeight+1.15f,e.source.z};
  for(int side:{-1,1})for(int n=0;n<12;++n){
   const auto at=[&](float u){const float wave=std::sin(u*pi);return XMFLOAT3{p.x+(end.x-p.x)*u+std::cos(e.facing)*side*.28f*wave,p.y+(end.y-p.y)*u+.40f*wave,p.z+(end.z-p.z)*u-std::sin(e.facing)*side*.28f*wave};};
   const float u=n/12.f,v=(n+1)/12.f,front=combat::Smooth(age/.38f);
   if(u>front)continue;
   const auto a=at(u),b=at(std::min(v,front));
   r.DrawEffectStreak(a,b,.085f,{1,.67f,.19f,bloom*.32f});
   r.DrawEffectStreak(a,b,.032f,{1,.95f,.74f,bloom*.78f});
   if(n%3==0)r.DrawEffectSprite(Art::Sparks,a,.11f,.11f,age*2,{1,.89f,.52f,bloom*.32f});
  }
 }
}
}
float ContactLifetime(const rpg::CombatVisualEvent& e){return e.school==C::Priest?.88f:e.school==C::Mage?(e.heavy || e.chain>=2?.88f:.70f):.66f;}
XMFLOAT3 ProjectilePosition(const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float progress){
 const float t=std::isfinite(progress)?Unit(progress):0;
 if(e.school==C::Mage && e.ability==9){
  const XMFLOAT3 target{e.destination.x,rpg::WorldMap::Height(e.destination)+e.destinationHeight,e.destination.z};
  const float remaining=1-t*t;
  return Add(target,{-std::sin(e.facing)*2.2f*remaining,5.4f*remaining,-std::cos(e.facing)*2.2f*remaining});
 }
 return CombatEffects::SpellTracePoint(origin,e.destination,t,e.school==C::Mage?.28f:0.f,e.destinationHeight);
}
void Contact(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 if(!std::isfinite(age)||age<0 || age>=ContactLifetime(e))return;
 const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.36f,e.height,e.position.z-std::cos(e.facing)*.36f};
 MajorContact(r,e,p,age);
 const float power=(e.heavy?1.15f:.72f)*(e.empowered?1.15f:1)*(e.chain>=2?1.12f:1);
 if(e.school==C::Mage){
  MageContact(r,e,p,age,power*(e.heavy?1.8f:1.4f));
  const float blast=Envelope(age,.24f,.012f);const auto element=School(e.ability);
  const XMFLOAT4 color=element==Element::Frost?XMFLOAT4{.48f,.80f,1,blast*.68f}:element==Element::Arcane?XMFLOAT4{.72f,.49f,1,blast*.6f}:XMFLOAT4{1,.64f,.28f,blast*.58f};
  for(int n=0;n<8;++n){const float angle=n*tau/8,reach=(.25f+age*4.5f)*power;
   const auto tip=Add(p,{std::cos(angle)*reach,std::sin(angle)*reach,std::sin(angle*2+e.facing)*reach*.55f});
   r.DrawEffectStreak(p,tip,.035f*power,color);}
  if(e.heavy){
   if(e.primary){
    // A textured pressure skirt expands through volume rather than laying
    // another bright circle on the floor. Gaps retain the target silhouette.
    const float pressure=Envelope(age,.42f,.016f);
    const auto color=element==Element::Frost?XMFLOAT4{.49f,.76f,.9f,pressure*.30f}:
      element==Element::Arcane?XMFLOAT4{.66f,.40f,.94f,pressure*.32f}:XMFLOAT4{1,.63f,.29f,pressure*.34f};
    for(int n=0;n<12;++n){
     const auto at=[&](float u,bool top){const float a=(n+u)*tau/12+e.facing;
      const float radius=(.20f+age*4.8f)*power*(top?.48f:1.f);
      const float y=top?.28f+age*.7f:-.24f-age*.7f;
      return Add(p,{std::cos(a)*radius,y,std::sin(a)*radius});};
     r.DrawEffectSurface(element==Element::Arcane?Art::Arcane:Art::Smoke,at(.08f,false),at(.08f,true),at(.92f,true),at(.92f,false),color);
    }
   }
   const float bloom=Envelope(age,.64f,.045f);
   for(unsigned n=0;n<4;++n){
    const float a=n*tau/4+e.facing,spread=(.16f+age*1.25f)*power;
    auto plume=Add(p,{std::cos(a)*spread,age*.75f,std::sin(a)*spread});
    const float radius=(.24f+age*.50f)*power;
    if(element==Element::Frost)
     r.DrawEffectSprite(Art::Smoke,plume,radius*1.35f,radius,.3f*n,{.55f,.73f,.88f,bloom*.23f},true);
    else if(element==Element::Arcane)
     r.DrawEffectSprite(Art::Arcane,plume,radius,radius,age*2+n,{.69f,.40f,1,bloom*.33f});
    else
     r.DrawEffectSprite(Art::Flame,plume,radius*.75f,radius*1.5f,a*.17f,{1,.73f,.40f,bloom*.58f},true);
   }
  }
  r.DrawEffectSprite(Art::Sparks,p,.75f*power,.75f*power,0,{1,.92f,.77f,blast*.65f});return;
 }
 if(e.school==C::Priest){
  const float fade=Envelope(age,.88f,.025f),flash=Envelope(age,.22f,.008f),size=power*(e.signature?1.25f:1.f);
  const float sx=std::sin(e.facing),cx=std::cos(e.facing);
  const auto plane=[&](float x,float y){return Add(p,{cx*x,y,-sx*x});};
  r.DrawEffectSurface(Art::Slash,plane(-.24f*size,-.70f*size),plane(-.24f*size,1.25f*size),plane(.24f*size,1.25f*size),plane(.24f*size,-.70f*size),{1,.90f,.57f,fade*.75f});
  r.DrawEffectSprite(Art::Seal,p,(.35f+age*.8f)*size,(.35f+age*.8f)*size,-age*.35f,{1,.83f,.39f,fade*.58f});
  for(int n=0;n<8;++n){const float a=n*tau/8,reach=(.30f+age*1.6f)*size;
   r.DrawEffectStreak(plane(std::cos(a)*.14f,std::sin(a)*.14f),plane(std::cos(a)*reach,std::sin(a)*reach),.025f,{1,.94f,.72f,fade*.6f});}
  Stamp(r,e.position,.8f*size,0,{1,.82f,.35f,fade*.32f});
  if(e.heavy)for(int n=-1;n<=1;++n){
   // Three descending curtains end at the accepted target, with a warm gold
   // outer veil and a narrow ivory core. No full-screen exposure flash.
   const float x=n*.34f*size,descent=Unit(age/.16f);
   const float lower=(1-descent)*1.8f*size,upper=(2.2f-age*.45f)*size;
   const float alpha=Envelope(age,.62f,.025f)*(n?.32f:.52f);
   r.DrawEffectSurface(Art::Slash,plane(x-.10f*size,lower),plane(x-.10f*size,upper),
    plane(x+.10f*size,upper),plane(x+.10f*size,lower),{1,.91f,.64f,alpha});
  }
  if(e.heavy && e.primary) {
   // Open gold/ivory vanes around the contact form a volume from side views,
   // where the original single-plane descending curtains become edge-on.
   const float opening=combat::Smooth(age/.20f),glow=Envelope(age,.72f,.025f);
   for(int n=0;n<6;++n) {
    const float a=e.facing+n*tau/6,radial=(.26f+opening*.66f)*size;
    const float y=-.18f+age*.62f,span=(.30f+opening*.36f)*size;
    const XMFLOAT3 base=Add(p,{std::sin(a)*radial,y,std::cos(a)*radial});
    const XMFLOAT3 up=Add(base,{std::sin(a)*.24f*size,span,std::cos(a)*.24f*size});
    const XMFLOAT3 edge{std::cos(a)*.12f*size,0,-std::sin(a)*.12f*size};
    r.DrawEffectSurface(Art::Slash,Add(base,{-edge.x,0,-edge.z}),Add(up,{-edge.x*.28f,0,-edge.z*.28f}),
      Add(up,{edge.x*.28f,0,edge.z*.28f}),Add(base,edge),{1,.86f,.43f,glow*.44f});
    r.DrawEffectStreak(base,up,.014f*size,{1,.97f,.79f,glow*.65f});
   }
  }
  r.DrawEffectSprite(Art::Sparks,p,.65f*size,.65f*size,0,{1,.96f,.83f,flash*.8f});return;
 }
 const float flash=Envelope(age,.18f,.008f),fade=Envelope(age,.32f);
 if(e.school==C::Warrior || e.school==C::Demonologist){
  const auto color=e.school==C::Warrior?XMFLOAT4{.82f,.90f,1,fade*.78f}:XMFLOAT4{1,.24f,.25f,fade*.78f};
  const int cuts=e.ability==14 || e.signature?2:1;
  for(int n=0;n<cuts;++n){
   const float tilt=(n?-.65f:.65f)+(e.ability==12?pi:0),cs=std::cos(e.facing),sn=std::sin(e.facing);
   const auto surface=[&](float x,float y){const float right=(x*std::cos(tilt)-y*std::sin(tilt))*power;
    const float up=(x*std::sin(tilt)+y*std::cos(tilt))*power;
    return Add(p,{cs*right,up,-sn*right});};
   r.DrawEffectSurface(Art::Slash,surface(-.62f,-.42f),surface(-.62f,.42f),surface(.62f,.42f),surface(.62f,-.42f),color);
  }
  r.DrawEffectSprite(Art::Sparks,p,.38f*power,.38f*power,e.facing,{1,.75f,.42f,flash*.9f});
  // A brief blade flash yields to gravity-driven metal fragments, not a ball
  // of light glued to the victim for the whole recovery.
  for(unsigned n=0;n<(e.heavy?7u:4u);++n){const float a=Rand(e.actor+n*43u)*tau,speed=.8f+Rand(n*23u+e.actor)*1.8f;
   const auto at=[&](float t){return Add(p,{std::cos(a)*speed*t,std::sin(a)*speed*t-3.6f*t*t,std::sin(e.facing)*t*.4f});};
   r.DrawEffectStreak(at(std::max(0.f,age-.025f)),at(age),.009f,{1,.68f,.29f,Envelope(age,.42f,.008f)*.8f});}
  if(e.heavy)Dust(r,Ground(e.position,.18f),age,power,e.actor,true);
 }else if(e.school==C::Scout || e.school==C::Engineer){
  r.DrawEffectSprite(Art::Sparks,p,.19f*power,.28f*power,e.facing,{.86f,.76f,.48f,flash*.75f});
  Dust(r,p,age,power*.70f,e.actor+unsigned(e.ability));
  if(e.ability==9 || e.ability==11 || e.ability==14)
   r.DrawEffectSprite(Art::Slash,p,.42f,.16f,-.4f,{.65f,.76f,.72f,fade*.32f});
 }else{
  // Other schools keep their existing shapes and add an authored surface layer.
  const bool holy=e.school==C::Priest || e.school==C::Paladin;
  auto c=classfx::Color(e.school,fade*.46f);
  r.DrawEffectSprite(holy?Art::Seal:Art::Arcane,p,(.25f+age*.7f)*power,(.25f+age*.7f)*power,age*.8f,c);
 }
}
float ArrowProgress(int ability,int ordinal,float age,float flight){
 if(!std::isfinite(age) || !std::isfinite(flight) || flight<=0 || ordinal<0)return -1;
 const float delay=std::min(flight*.45f,ability==12?ordinal*.063f:ability==14?ordinal*.035f:0.f);
 return age<delay?-1:Unit((age-delay)/(flight-delay));
}
void Flight(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age){
 if((e.school!=C::Mage && e.school!=C::Scout && e.school!=C::Priest) || !std::isfinite(age) || !std::isfinite(e.flight) || e.flight<=0 || age<0 || age>=e.flight+.16f)return;
 Release(r,e,origin,age);
 const float t=Unit(age/e.flight),after=std::max(0.f,age-e.flight),fade=1-combat::Smooth(after/.16f);
 const auto point=[&](float u){return ProjectilePosition(e,origin,u);};
 const auto head=point(t);const float power=e.heavy?1.25f:1.f;
 const float length=std::max(.01f,std::sqrt(std::pow(e.destination.x-origin.x,2.f)+std::pow(e.destination.z-origin.z,2.f)+std::pow(rpg::WorldMap::Height(e.destination)+e.destinationHeight-origin.y,2.f)));
 const float sx=std::sin(e.facing),cx=std::cos(e.facing);
 if(e.school==C::Scout){
  const float arrowFade=1-combat::Smooth(after/.025f);if(arrowFade<=0)return;
  const int count=e.ability==8?5:e.ability==12?2:e.ability==14?3:1;
  for(int n=0;n<count;++n){
   const float u=ArrowProgress(e.ability,n,age,e.flight);if(u<0)continue;
   const float spread=(n-(count-1)*.5f)*(e.ability==8?.38f:.24f)*std::sin(u*pi);
   const XMFLOAT3 offset{cx*spread,.03f*std::abs(n-(count-1)*.5f)*std::sin(u*pi),-sx*spread};
   const auto tip=Add(point(u),offset),tail=Add(point(std::max(0.f,u-.45f/length)),offset);
   r.DrawEffectStreak(tail,tip,.012f,{.86f,.78f,.57f,arrowFade});
   const auto fin=Add(point(std::max(0.f,u-.35f/length)),offset);
   for(float side:{-1.f,1.f})r.DrawEffectStreak(Add(tail,{cx*side*.045f,.02f,-sx*side*.045f}),fin,.012f,{.55f,.74f,.63f,arrowFade*.8f});
   if(e.heavy || count>1)r.DrawEffectStreak(Add(point(std::max(0.f,u-.8f/length)),offset),tip,.022f,{.63f,.82f,.73f,arrowFade*.22f});
  }
  if(e.ability==13)r.DrawEffectSprite(Art::Sparks,head,.075f,.075f,age*5,{.65f,.75f,.28f,arrowFade*.5f});
  return;
 }
 if(e.school==C::Mage && e.ability==9){
  const float glow=combat::Smooth(t/.18f)*(1-combat::Smooth(after/.035f));
  const auto trailing=point(std::max(0.f,t-.14f));
  const float roll=Roll(r,trailing,head);
  r.DrawEffectSprite(Art::Flame,head,.48f,.76f,roll,{1,.79f,.45f,glow*.95f},true);
  r.DrawEffectSprite(Art::Sparks,head,.26f,.35f,roll,{1,.95f,.74f,glow*.7f});
  for(int n=1;n<=6;++n){const float u=t-n*.035f;if(u<0)continue;const float tailFade=glow*(1-n/7.f);
   r.DrawEffectSprite(Art::Flame,point(u),.45f+n*.025f,.56f,roll,{1,.60f,.20f,tailFade*.62f},true);
   r.DrawEffectSprite(Art::Smoke,point(u),.44f,.48f,n*.7f,{.24f,.21f,.19f,tailFade*.22f},true);}
  return;
 }
 if(e.school==C::Priest){
  const float front=1-combat::Smooth(after/.035f),tail=std::max(0.f,t-1.65f*power/length);
  r.DrawEffectStreak(point(tail),head,.075f*power,{1,.84f,.40f,fade*.55f});
  r.DrawEffectStreak(point(tail),head,.022f*power,{1,.97f,.82f,fade*.85f});
  r.DrawEffectSprite(Art::Seal,head,.32f*power,.32f*power,age*2,{1,.88f,.48f,front*.7f});
  if(e.heavy){
   // A tapering reliquary lance has volume from side and overhead cameras.
   // Its point stays at the accepted projectile head throughout the flight.
   const auto previous=point(std::max(0.f,t-.3f));
   const XMFLOAT3 axis{head.x-previous.x,head.y-previous.y,head.z-previous.z};
   const float norm=std::sqrt(axis.x*axis.x+axis.y*axis.y+axis.z*axis.z);
   const XMFLOAT3 direction=norm>.0001f?XMFLOAT3{axis.x/norm,axis.y/norm,axis.z/norm}:XMFLOAT3{sx,0,cx};
   const auto back=Add(head,{-direction.x*.95f*power,-direction.y*.95f*power,-direction.z*.95f*power});
   for(int n=0;n<4;++n){const float a=n*pi*.5f;
    const XMFLOAT3 wing{cx*std::cos(a)*.25f*power,std::sin(a)*.25f*power,-sx*std::cos(a)*.25f*power};
    r.DrawEffectSurface(Art::Slash,back,Add(back,wing),head,Add(back,{-wing.x,-wing.y,-wing.z}),{1,.91f,.60f,front*.46f});
   }
  }
  for(int side:{-1,1})for(int n=0;n<6;++n){const float u=float(n)/6,v=float(n+1)/6;
   const auto swept=[&](float a){const float back=a*.75f*power,wing=side*std::sin(a*pi)*.32f*power;
    return Add(head,{cx*wing-sx*back,.08f*std::sin(a*pi),-sx*wing-cx*back});};
   r.DrawEffectStreak(swept(u),swept(v),(.035f+.045f*(1-u))*power,{1,.82f,.32f,front*(1-u)*.55f});}
  for(int n=0;n<4;++n){const float a=n*pi*.5f,side=std::cos(a)*.40f*power;const auto tip=Add(head,{cx*side,std::sin(a)*.40f*power,-sx*side});r.DrawEffectStreak(head,tip,.025f,{1,.96f,.78f,front*.7f});}
  return;
 }
 const auto element=School(e.ability);const bool frost=element==Element::Frost,arcane=element==Element::Arcane,mixed=element==Element::Thermal;
 if(e.ability==13) {
  // A refracting core with three braided light trails, sampled directly from
  // accepted flight time. No extra hits, homing, target changes or new textures.
  const float visible=1-combat::Smooth(after/.12f),tail=std::max(0.f,t-2.8f/length);
  for(int strand=0;strand<3;++strand) {
   const auto at=[&](float u){const float phase=u*length*5-age*4+strand*tau/3;
    const float width=.14f*std::sin(Unit((u-tail)/std::max(.0001f,t-tail))*pi);
    return Add(point(u),{cx*std::sin(phase)*width,std::cos(phase)*width,-sx*std::sin(phase)*width});};
   for(int n=0;n<12 && t>tail+.00001f;++n) {
    const float u=tail+(t-tail)*n/12,v=tail+(t-tail)*(n+1)/12;
    const auto a=at(u),b=at(v);const float taper=(n+1)/12.f;
    r.DrawEffectStreak(a,b,.047f,{.50f,.26f,1,visible*taper*.28f});
    r.DrawEffectStreak(a,b,.011f,{.82f,.83f,1,visible*taper*.82f});
   }
  }
  const float core=1-combat::Smooth(after/.025f);
  for(int n=0;n<3;++n) {
   const float angle=n*tau/3+age*3;
   const XMFLOAT3 side{cx*std::cos(angle)*.23f,std::sin(angle)*.23f,-sx*std::cos(angle)*.23f};
   const auto front=Add(head,{sx*.26f,0,cx*.26f}),back=Add(head,{-sx*.32f,0,-cx*.32f});
   const auto a=Add(head,side),b=Add(head,{-side.x,-side.y,-side.z});
   r.DrawEffectSurface(Art::Arcane,back,a,front,b,{.69f,.53f,1,core*.48f});
   r.DrawEffectStreak(back,a,.013f,{.83f,.89f,1,core*.8f});
   r.DrawEffectStreak(a,front,.013f,{.92f,.83f,1,core*.9f});
  }
  r.DrawEffectSprite(Art::Sparks,head,.22f,.22f,age*2,{.86f,.86f,1,core*.7f});
  return;
 }
 const auto tint=[&](bool ice,float alpha)->XMFLOAT4{return ice?XMFLOAT4{.40f,.76f,1,alpha}:arcane?XMFLOAT4{.70f,.38f,1,alpha}:XMFLOAT4{1,.43f,.09f,alpha};};
 // Fixed world length: a nearby projectile is no longer a tiny dot while a
 // distant shot stretches into a several-metre stripe. Advect and drain its
 // wake after arrival; the head stops at the authoritative contact.
 const float span=std::min(1.f,(frost?.85f:arcane?1.05f:1.35f)*power/length);
 const float tail=std::max(0.f,t-span*(1-combat::Smooth(after/.16f)));
 for(int strand=0;strand<(mixed?2:1);++strand){
  const bool ice=frost || (mixed && strand==1);
  const auto at=[&](float u){auto p=point(u);const float phase=u*length*7-age*5+strand*pi;
   const float radius=std::sin(Unit((u-tail)/std::max(.0001f,t-tail))*pi)*(mixed?.10f:arcane?.075f:.035f);
   return Add(p,{cx*std::sin(phase)*radius,std::cos(phase)*radius,-sx*std::sin(phase)*radius});};
  if(t>tail+.00001f)for(int n=0;n<14;++n){
   const float a=tail+(t-tail)*n/14,b=tail+(t-tail)*(n+1)/14;
   const float taper=(n+.5f)/14;
   r.DrawEffectStreak(at(a),at(b),(.008f+taper*.028f)*power,tint(ice,fade*taper*.64f));
  }
  for(int n=1;n<=7;++n){
   const float u=t-span*n/8;if(u<0)continue;
   const float opacity=fade*(1-float(n)/9);auto p=point(u);p.y+=after*.38f;
   const float size=(.23f+after*.17f)*power*(1-float(n)/10);
   if(ice)r.DrawEffectSprite(Art::Ice,p,size*.4f,size,age*2+n*.8f,{.55f,.80f,1,opacity*.48f});
   else r.DrawEffectSprite(arcane?Art::Arcane:Art::Flame,p,size,size*1.35f,n*.55f+age*.8f,
    arcane?XMFLOAT4{.73f,.45f,1,opacity*.38f}:XMFLOAT4{1,.83f,.54f,opacity*.65f},!arcane);
  }
 }
 const float headFade=1-combat::Smooth(after/.025f);if(headFade<=0)return;
 const float roll=Roll(r,point(t-.10f/length),head);
 if(frost){
  const int count=e.ability==12?3:1;
  for(int n=0;n<count;++n){const float offset=(n-(count-1)*.5f)*.09f*std::sin(t*pi);
   r.DrawEffectSprite(Art::Ice,Add(head,{cx*offset,0,-sx*offset}),.10f*power,.39f*power,roll,{.67f,.88f,1,headFade*.95f});}
  for(int n=0;n<4;++n){const float a=n*pi*.5f+age*3;const auto at=Add(head,{cx*std::sin(a)*.19f*power,std::cos(a)*.19f*power,-sx*std::sin(a)*.19f*power});
   r.DrawEffectSprite(Art::Ice,at,.045f*power,.18f*power,roll+a*.12f,{.55f,.80f,1,headFade*.65f});}
 }else if(arcane){
  r.DrawEffectSprite(Art::Arcane,head,.47f*power,.47f*power,age*5,{.86f,.66f,1,headFade*.85f});
  for(int n=0;n<2;++n)r.DrawEffectSprite(Art::Seal,head,(.55f+n*.14f)*power,.17f*power,(n?1.f:-1.f)*age*3,{.67f,.42f,1,headFade*.4f});
 }else {r.DrawEffectSprite(Art::Flame,head,.39f*power,.56f*power,roll,{1,.85f,.60f,headFade*.95f},true);
  for(int n=0;n<3;++n){const float a=n*tau/3+age*7;const auto at=Add(head,{cx*std::sin(a)*.19f*power,std::cos(a)*.19f*power,-sx*std::sin(a)*.19f*power});
   r.DrawEffectSprite(Art::Flame,at,.22f*power,.33f*power,roll+a*.12f,{1,.71f,.32f,headFade*.66f},true);}
  if(mixed)r.DrawEffectSprite(Art::Ice,head,.075f*power,.23f*power,roll+.35f,{.55f,.82f,1,headFade*.85f});}
 r.DrawEffectGlow(head,.035f*power,{.9f,.92f,1,headFade*.6f});
}
void EnemyContact(Renderer& r,const rpg::CombatVisualEvent& e,float age,bool fullDetail){
 if(!std::isfinite(age)||age<0 || age>=.72f)return;
 const bool quake=e.kind==K::EnemyQuake,tempest=e.kind==K::EnemyTempest;
 const bool lance=e.kind==K::EnemyLance,execution=e.kind==K::EnemyExecution;
 if(!quake && !tempest && !lance && !execution && e.kind!=K::BeastCharge)return;
 const float flash=1-combat::Smooth(age/.12f),body=Envelope(age,.46f,.012f),residue=Envelope(age-.04f,.68f,.055f);
 const auto point=[&](float angle,float radius,float up){return Ground({e.position.x+std::sin(angle)*radius,e.position.z+std::cos(angle)*radius},up);};
 const auto ring=[&](float radius,float up,float width,XMFLOAT4 color){
  const int segments=fullDetail?48:16;
  for(int n=0;n<segments;++n){const float a=n*tau/segments,b=(n+1)*tau/segments;
   r.DrawEffectRibbon(point(a,radius-width,up),point(a,radius+width,up),point(b,radius+width,up),point(b,radius-width,up),color);}
 };
 if(quake){
  // The full damaged annulus flashes at contact; delayed dust is cosmetic.
  // Keep the safe inner flank clear instead of filling the whole disc.
  ring(rpg::Enemy::shockInner,.065f,.018f,{1,.48f,.15f,flash*.8f});
  ring(rpg::Enemy::shockOuter,.065f,.028f,{1,.48f,.15f,flash*.9f});
  const int count=fullDetail?18:6,branches=fullDetail?4:3;
  for(int n=0;n<count;++n){
   const float a=n*tau/count,noise=Rand(e.actor+unsigned(n)*31);
   auto previous=point(a,rpg::Enemy::shockInner+.08f,.075f);
   for(int j=1;j<=branches;++j){const float radius=rpg::Enemy::shockInner+.08f+(rpg::Enemy::shockOuter-rpg::Enemy::shockInner-.16f)*j/branches;
    const auto next=point(a+std::sin(j*3.2f+n)*.035f,radius,.075f);
    r.DrawEffectStreak(previous,next,.065f,{1,.27f,.035f,body*(.7f+noise*.3f)});
    if(fullDetail)r.DrawEffectStreak(previous,next,.013f,{1,.75f,.35f,body*.85f});previous=next;}
   const float radius=rpg::Enemy::shockInner+.35f+noise*(rpg::Enemy::shockOuter-rpg::Enemy::shockInner-.7f);
   const auto p=point(a,radius,.12f+age*(.4f+noise*.6f));
   r.DrawEffectSprite(Art::Dust,p,.33f+age*.65f,.24f+age*.48f,a,{.55f,.46f,.36f,residue*.82f},true);
   if(n%2==0){
    const auto shard=point(a,radius,.10f+age*2.3f-3.2f*age*age);
    r.DrawEffectSprite(Art::Sparks,shard,.08f,.16f,a+age,{1,.61f,.25f,body*.65f});
   }
  }
 }else if(tempest){
  constexpr float radius=3.8f;
  ring(radius,.07f,.035f,{.65f,.42f,1,flash*.75f});
  // Broken rising bands form volume, leaving clear windows onto the actor.
  const int bands=fullDetail?3:2,segments=fullDetail?28:10,count=fullDetail?10:4;
  for(int band=0;band<bands;++band)for(int n=0;n<segments;++n){
   const auto at=[&](float u,float side){const float a=e.facing+band*tau/bands+u*tau*.78f-age*6;
    const float rr=(radius-.30f)*(.94f-u*.21f);const float y=.18f+u*2.2f+age*.65f+side;
    return point(a,rr,y);};
   const float a=float(n)/segments,b=float(n+1)/segments,taper=std::sin((a+b)*pi*.5f);
   r.DrawEffectRibbon(at(a,-.095f),at(a,.095f),at(b,.095f),at(b,-.095f),{.48f,.28f,1,body*.76f*taper});
   r.DrawEffectStreak(at(a,0),at(b,0),.019f,{.76f,.68f,1,body*.9f*taper});
  }
  for(int n=0;n<count;++n){const float a=n*tau/count-age*2;const auto p=point(a,radius-.4f,.2f+age*.6f);
   r.DrawEffectSprite(Art::Smoke,p,.22f+age*.45f,.18f+age*.35f,a,{.39f,.34f,.47f,residue*.36f},true);
   r.DrawEffectSprite(Art::Slash,point(a,radius-.55f,.6f+age),.25f,.55f,-a,{.64f,.51f,1,body*.5f});}
 }else if(lance || execution){
  const float range=execution?8.f:7.f,half=execution?.22f:.16f;
  const float sx=std::sin(e.facing),cx=std::cos(e.facing);
  const auto at=[&](float u,float side,float up){const float d=.20f+(range-.20f)*u;
   const float lateral=side*std::tan(half)*d*.78f;
   return Ground({e.position.x+sx*d+cx*lateral,e.position.z+cx*d-sx*lateral},up);};
  // Split, tapering strands agree with the committed cone and remain visible
  // from a side camera. No large terminal orb pretending the hit is delayed.
  const int segments=fullDetail?20:8,count=fullDetail?8:4;
  for(int strand=0;strand<(execution && fullDetail?4:2);++strand)for(int n=0;n<segments;++n){
   const auto bolt=[&](float u){const float wave=std::sin(u*31+strand*2.4f+age*13);
    return at(u,(strand%2?1.f:-1.f)*(.18f+std::sin(u*pi)*.55f),.90f+wave*.12f*std::sin(u*pi));};
   const float a=float(n)/segments,b=float(n+1)/segments;
   r.DrawEffectStreak(bolt(a),bolt(b),execution?.062f:.035f,{.59f,.32f,1,body*(1-a*.5f)});
   if(strand==0)r.DrawEffectStreak(bolt(a),bolt(b),.014f,{.88f,.79f,1,std::max(flash,body*.6f)*.9f});
  }
  for(int n=0;n<count;++n){const float u=(n+.5f)/count;const auto p=at(u,0,.90f+age*.35f);
   r.DrawEffectSprite(Art::Arcane,p,.16f+age*.17f,.35f+age*.2f,e.facing+age*1.3f,{.62f,.40f,1,body*.66f});
   r.DrawEffectSprite(Art::Smoke,at(u,0,.22f+age*.3f),.16f+age*.32f,.13f+age*.22f,n*.7f,{.40f,.34f,.49f,residue*.32f},true);
  }
  r.DrawEffectSprite(Art::Seal,point(e.facing,.25f,1.05f),.25f+age*.4f,.25f+age*.4f,-age*3,{.72f,.50f,1,body*.55f});
 }else if(age<.48f){
  for(int n=0;n<4;++n){const float a=e.facing+(n-1.5f)*.35f;
   r.DrawEffectSprite(Art::Dust,point(a,.7f+age,.18f+age*.4f),.16f+age*.68f,.12f+age*.44f,a,{.64f,.52f,.37f,Envelope(age,.48f)*.44f},true);}
 }
}
}
