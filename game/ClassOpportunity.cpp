#include "game/CastEffects.h"
#include "game/SpellFlow.h"
#include <algorithm>
#include <cmath>

namespace castfx {
namespace {
using namespace DirectX;
using C=rpg::ClassId;
using K=rpg::CombatVisualKind;
using Art=Renderer::EffectArt;
constexpr float pi=3.14159265f,tau=2*pi;
bool Finite(XMFLOAT3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
bool Valid(const Attachments& a){return a.alive && std::isfinite(a.facing) && Finite(a.root) && Finite(a.focus) &&
 Finite(a.left) && Finite(a.right) && (!a.blade || (Finite(a.edgeBase)&&Finite(a.edgeTip)));}
XMFLOAT3 Mix(XMFLOAT3 a,XMFLOAT3 b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
XMFLOAT3 Local(const Attachments& a,XMFLOAT3 p,float x,float y,float z=0){
 const float c=std::cos(a.facing),s=std::sin(a.facing);return {p.x+c*x+s*z,p.y+y,p.z-s*x+c*z};
}
template<class Path> void Flow(Renderer& r,const Attachments& a,const Path& path,float width,float time,XMFLOAT4 tint,int segments=14){
 spellvfx::FilamentStream(r,path,{std::sin(a.facing),0,std::cos(a.facing)},width,1.3f,time,tint,segments);
}
XMFLOAT4 Element(int n,float opacity){return n==0?XMFLOAT4{1,.48f,.16f,opacity}:n==1?XMFLOAT4{.38f,.81f,1,opacity}:XMFLOAT4{.78f,.52f,1,opacity};}
// Keep a narrow open silhouette around the shield. It is neither a damage
// footprint nor a replacement for the real weapon/hand sockets.
void Guard(Renderer& r,const Attachments& a,float time,float opacity){
 Flow(r,a,[&](float u){const float angle=-.7f+u*pi*1.45f;return Local(a,a.left,std::cos(angle)*.37f,std::sin(angle)*.43f,.08f);},
  .038f,time,{.72f,.84f,1,opacity},18);
}
void Blade(Renderer& r,const Attachments& a,float time,float opacity){
 if(!a.blade){Guard(r,a,time,opacity*.7f);return;}
 for(float side:{-1.f,1.f})Flow(r,a,[&](float u){
   const auto p=Mix(a.edgeBase,a.edgeTip,u);const float span=std::sin(u*pi);
   return Local(a,p,side*(.018f+.055f*span),.018f*std::sin(u*8-time*4)*span);
  },.024f,time*1.6f+side,{1,.78f,.42f,opacity},18);
 const float u=.12f+.76f*(.5f+.5f*std::sin(time*2.6f));
 r.DrawEffectSprite(Art::Sparks,Mix(a.edgeBase,a.edgeTip,u),.075f,.14f,0,{1,.91f,.69f,opacity*.7f});
}
void Pursuit(Renderer& r,const Attachments& a,float time,float opacity,float expand=0){
 for(int n=0;n<3;++n)Flow(r,a,[&](float u){
   const float angle=n*tau/3+u*2.2f-time*.65f,rad=(.14f+expand*.14f)*std::sin(u*pi);
   return Local(a,a.focus,std::cos(angle)*rad,std::sin(angle)*rad,-.52f+u*(.86f+expand*.35f));
  },.021f,time+n*.31f,{.65f,.86f,.76f,opacity},16);
 // The barbs travel with the bow release socket rather than fixed world axes.
 for(float side:{-1.f,1.f})r.DrawEffectStreak(Local(a,a.focus,side*.14f,.04f,-.30f),
  Local(a,a.focus,0,.04f,-.08f),.015f,{.9f,.96f,.82f,opacity*.8f});
}
void Benediction(Renderer& r,const Attachments& a,float time,float opacity,float rise=0){
 const auto chest=Local(a,a.root,0,1.12f,-.12f);
 for(float side:{-1.f,1.f}){
  const auto palm=side<0?a.left:a.right;
  Flow(r,a,[&](float u){
    const auto p=Mix(palm,chest,u);const float arch=std::sin(u*pi);
    return Local(a,p,side*arch*(.46f+rise*.25f),arch*(.56f+rise*.25f),-.12f*arch);
   },.042f,time+side,{1,.84f,.48f,opacity},20);
  for(int n=0;n<4;++n){const float u=std::fmod(time*.35f+n*.25f,1.f);
   const auto p=Local(a,chest,side*(.24f+.21f*std::sin(u*pi)),u*.85f+rise*.2f,-.12f);
   r.DrawEffectStreak(p,Local(a,p,0,.09f+.09f*u),.012f,{1,.94f,.75f,opacity*std::sin(u*pi)*.7f});}
 }
}
void Confluence(Renderer& r,const Attachments& a,unsigned mask,float time,float opacity,float gather){
 const auto chest=Local(a,a.root,0,1.18f,0),centre=Mix(chest,a.focus,gather);
 for(int n=0;n<3;++n)if(mask&(1u<<n)){
  const float angle=time*.65f+n*tau/3,reach=.62f*(1-.72f*gather);
  const auto tip=Local(a,centre,std::cos(angle)*reach,.18f*std::sin(angle),std::sin(angle)*reach);
  const auto tint=Element(n,opacity);
  Flow(r,a,[&](float u){const float arc=angle-(1-u)*1.8f,radius=reach*(.32f+.68f*u);
   return Local(a,centre,std::cos(arc)*radius,.18f*std::sin(arc)+.10f*std::sin(u*pi),std::sin(arc)*radius);
  },.032f,time+n*.4f,tint,18);
  if(n==0)r.DrawSpellVolume(Renderer::SpellMedium::Fire,tip,{0,1,0},{.095f,.17f,.095f},time,{1,1,1,opacity*.7f},31);
  else if(n==1){for(float side:{-1.f,1.f})r.DrawEffectSprite(Art::Ice,Local(a,tip,side*.035f,side*.035f),.075f,.22f,side*.25f,tint);}
  else {r.DrawEffectSprite(Art::Arcane,tip,.18f,.23f,angle,tint);
   r.DrawEffectStreak(Local(a,tip,0,-.14f),Local(a,tip,0,.14f),.015f,{.9f,.8f,1,opacity*.7f});}
 }
}
}
void Discipline(Renderer& r,const rpg::Simulation& s,const Attachments& a,float clock){
 if(!Valid(a) || !std::isfinite(clock) || clock<0 || s.Health()<=0 || !rpg::HasDiscipline(s.Class()))return;
 // Stateless evaluation: a repeated render, teleport or load cannot advance
 // an emitter or leave an orphaned aura at the previous world position.
 const float remaining=s.DisciplineSeconds(),duration=s.Class()==C::Mage?8.f:s.Class()==C::Priest?6.f:5.f;
 const float fade=combat::Smooth((duration-remaining)/.12f)*combat::Smooth(remaining/.18f);
 clock=std::fmod(clock,600.f);
 if(s.Class()==C::Warrior){
  if(s.GuardWindow()>0)Guard(r,a,clock,combat::Smooth(s.GuardWindow()/.12f)*.65f);
  if(remaining>0)Blade(r,a,clock,fade*.8f);
 }else if(s.Class()==C::Scout){if(remaining>0)Pursuit(r,a,clock,fade*.76f);}
 else if(s.Class()==C::Priest){if(remaining>0)Benediction(r,a,clock,fade*.70f);}
 else {
  const bool casting=s.ConfluenceCast() && s.AttackSeconds()<combat::StrokeStart(s.HeavyAttack());
  const unsigned mask=remaining>0 || casting?7:s.ElementMask();
  const float gather=casting?combat::Smooth(s.AttackSeconds()/combat::StrokeStart(s.HeavyAttack())):0;
  if(mask)Confluence(r,a,mask,clock,casting?.8f:remaining>0?fade*.72f:.42f,gather);
 }
}
void DisciplinePulse(Renderer& r,const rpg::CombatVisualEvent& e,float age,const Attachments* caster){
 if((e.kind!=K::DisciplineReady && e.kind!=K::DisciplineSpent) || !rpg::HasDiscipline(e.school) ||
  !std::isfinite(age) || age<=0 || age>=.65f || !std::isfinite(e.facing) ||
  !Finite({e.position.x,e.height,e.position.z}) || (e.actor==0 && caster && !Valid(*caster)))return;
 const bool spent=e.kind==K::DisciplineSpent;
 const float t=age/.65f,fade=combat::Smooth(age/.045f)*(1-combat::Smooth(t));
 const auto a=e.actor==0 && caster?*caster:Preview({e.position.x,e.height-1,e.position.z},{e.position.x,e.height,e.position.z},e.facing);
 if(e.school==C::Warrior){
  if(!spent){Guard(r,a,age*3,fade);Blade(r,a,age*3,fade*.8f);}
  else {const auto p=XMFLOAT3{e.position.x,e.height,e.position.z};
   for(float side:{-1.f,1.f})Flow(r,a,[&](float u){const float angle=-1.1f+u*2.2f;
    return Local(a,p,std::sin(angle)*(.30f+t*.55f),side*std::cos(angle)*(.24f+t*.45f),-.08f+u*.16f);
   },.065f,age*3,{1,.75f,.37f,fade},18);
   for(int n=0;n<9;++n){const float angle=n*tau/9;const auto tip=Local(a,p,std::cos(angle)*age*.9f,std::sin(angle)*age-age*age,.12f*age);
    r.DrawEffectStreak(tip,Local(a,tip,std::cos(angle)*.06f,std::sin(angle)*.06f),.012f,{1,.87f,.58f,fade});}
  }
 }else if(e.school==C::Scout)Pursuit(r,a,age*3,fade,spent?t:0);
 else if(e.school==C::Mage)Confluence(r,a,7,age*3,fade,spent?combat::Smooth(t):0);
 else if(!spent)Benediction(r,a,age*2,fade,t);
 else {
  const auto p=XMFLOAT3{e.position.x,e.height,e.position.z};
  for(float side:{-1.f,1.f})Flow(r,a,[&](float u){const float arch=std::sin(u*pi);
   return Local(a,p,side*arch*(.46f+t*.3f),1.15f*(1-u)-t*.25f,-.12f*arch);
  },.065f,age*3,{1,.87f,.52f,fade},20);
  r.DrawEffectStreak(Local(a,p,0,.95f-t*.2f),Local(a,p,0,-.35f),.035f,{1,.96f,.8f,fade*.75f});
 }
}
}
