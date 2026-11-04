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
void MageContact(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 p,float age,float size){
 const auto element=School(e.ability);const bool frost=element==Element::Frost,arcane=element==Element::Arcane,mixed=element==Element::Thermal;
 const float fade=Envelope(age,ContactLifetime(e)),burst=Envelope(age,.20f,.008f);
 if(arcane){
  for(int n=0;n<2;++n){const float radius=(.15f+age*(n?.75f:1.4f))*size;
   r.DrawEffectSprite(Art::Arcane,p,radius,radius,age*(n?-2.f:2.8f)+n*pi,{.85f,.72f,1,fade*(n?.28f:.65f)});}
  r.DrawEffectSprite(Art::Seal,p,(.25f+age*.8f)*size,(.25f+age*.8f)*size,-age*.7f,{.8f,.58f,1,fade*.48f});
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
}
float ContactLifetime(const rpg::CombatVisualEvent& e){return e.school==C::Mage?(e.heavy || e.chain>=2?.88f:.70f):.66f;}
void Contact(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 if(!std::isfinite(age)||age<0 || age>=ContactLifetime(e))return;
 const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.36f,e.height,e.position.z-std::cos(e.facing)*.36f};
 const float power=(e.heavy?1.15f:.72f)*(e.empowered?1.15f:1)*(e.chain>=2?1.12f:1);
 if(e.school==C::Mage){MageContact(r,e,p,age,power);return;}
 const float flash=Envelope(age,.18f,.008f),fade=Envelope(age,.32f);
 if(e.school==C::Warrior || e.school==C::Demonologist){
  const auto color=e.school==C::Warrior?XMFLOAT4{.82f,.90f,1,fade*.78f}:XMFLOAT4{1,.24f,.25f,fade*.78f};
  const int cuts=e.ability==14 || e.signature?2:1;
  for(int n=0;n<cuts;++n)r.DrawEffectSprite(Art::Slash,p,.56f*power,.46f*power,(n?-.65f:.65f)+(e.ability==12?pi:0),color);
  r.DrawEffectSprite(Art::Sparks,p,.38f*power,.38f*power,e.facing,{1,.75f,.42f,flash*.9f});
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
void Flight(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age){
 if(!std::isfinite(age)||age<0 || age>=e.flight+.06f)return;
 const float t=Unit(age/std::max(.01f,e.flight)),fade=Unit((e.flight+.06f-age)/.06f);
 const auto point=[&](float u){return CombatEffects::SpellTracePoint(origin,e.destination,Unit(u),e.school==C::Mage?.28f:0.f);};
 const auto head=point(t);const float power=e.heavy?1.25f:1.f;
 if(e.school==C::Mage){
  const auto element=School(e.ability);
  if(element==Element::Frost){
   const float roll=Roll(r,point(t-.09f),head);
   const int count=e.ability==12?3:1;
   for(int n=0;n<count;++n)r.DrawEffectSprite(Art::Ice,Add(head,{std::cos(e.facing)*(n-(count-1)*.5f)*.07f,0,-std::sin(e.facing)*(n-(count-1)*.5f)*.07f}),.055f*power,.23f*power,roll,{.62f,.85f,1,fade*.85f});
  }else if(element==Element::Arcane){
   r.DrawEffectSprite(Art::Arcane,head,.18f*power,.18f*power,age*4,{.9f,.65f,1,fade*.85f});
  }else{
   for(int n=0;n<4;++n){
    const float u=std::max(0.f,t-n*.045f);auto at=point(u);at.y+=n*.023f;
    const bool cold=element==Element::Thermal && n%2;
    r.DrawEffectSprite(cold?Art::Ice:Art::Flame,at,(.12f-n*.017f)*power,(.19f-n*.025f)*power,
      cold?Roll(r,point(t-.05f),head):std::sin(age*7+n)*.25f,
      cold?XMFLOAT4{.58f,.83f,1,fade*.65f}:XMFLOAT4{1,.86f,.63f,fade*(.88f-n*.15f)},!cold);
   }
  }
 }else if(e.school==C::Scout && e.heavy){
  r.DrawEffectSprite(Art::Slash,point(t-.04f),.07f,.20f,Roll(r,point(t-.1f),head),{.60f,.75f,.67f,fade*.16f});
 }
}
void Preparation(Renderer& r,const rpg::Simulation& state,XMFLOAT3 p,float progress){
 if(state.Class()!=C::Mage || state.CastingRecovery() || progress<=0 || progress>=1)return;
 const auto school=School(state.AttackAbility());const float power=progress*progress;
 if(school==Element::Frost)r.DrawEffectSprite(Art::Ice,p,.055f+power*.05f,.12f+power*.14f,progress*.35f,{.65f,.86f,1,power*.75f});
 else if(school==Element::Arcane)r.DrawEffectSprite(Art::Arcane,p,.10f+power*.20f,.10f+power*.20f,-progress*1.7f,{.9f,.75f,1,power*.65f});
 else{
  r.DrawEffectSprite(Art::Flame,Add(p,{0,.06f,0}),.09f+power*.11f,.13f+power*.20f,0,{1,.86f,.63f,power*.85f},true);
  if(school==Element::Thermal)r.DrawEffectSprite(Art::Ice,Add(p,{.10f,0,0}),.055f,.19f,progress*.5f,{.58f,.85f,1,power*.8f});
 }
}
void EnemyContact(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 if(!std::isfinite(age)||age<0 || age>=.48f)return;
 const bool quake=e.kind==K::EnemyQuake || e.kind==K::EnemyTempest,charge=e.kind==K::BeastCharge;
 if(quake || charge){
  const int count=quake?8:4;
  for(int n=0;n<count;++n){
   const float angle=quake?n*tau/count:e.facing+(n-1.5f)*.35f;
   const float radius=quake?(e.kind==K::EnemyQuake?rpg::Enemy::shockOuter:3.8f):.7f+age;
   const auto p=Ground({e.position.x+std::sin(angle)*radius,e.position.z+std::cos(angle)*radius},.18f+age*.4f);
   const float size=.16f+age*.68f;
   r.DrawEffectSprite(Art::Dust,p,size,size*.65f,angle,{.64f,.52f,.37f,Envelope(age,.48f)*.44f},true);
  }
 }else if(e.kind==K::EnemyLance || e.kind==K::EnemyExecution){
  const float range=e.kind==K::EnemyExecution?8.f:7.f;
  const auto p=Ground({e.position.x+std::sin(e.facing)*range,e.position.z+std::cos(e.facing)*range},1.05f);
  const float size=.18f+age*.6f;r.DrawEffectSprite(Art::Arcane,p,size,size,age*2,{.75f,.48f,1,Envelope(age,.48f)*.6f});
 }
}
}
