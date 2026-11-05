#include "game/CastEffects.h"
#include "game/ClassEffects.h"
#include "game/HandPose.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace castfx {
namespace {
using namespace DirectX;using C=rpg::ClassId;using Art=Renderer::EffectArt;
constexpr float pi=3.14159265f,tau=2*pi;
float Unit(float x){return std::clamp(x,0.f,1.f);}
XMFLOAT3 Add(XMFLOAT3 a,XMFLOAT3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
XMFLOAT3 Mix(XMFLOAT3 a,XMFLOAT3 b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
XMFLOAT3 Local(const Attachments& a,XMFLOAT3 origin,float x,float y,float z=0){
 const float s=std::sin(a.facing),c=std::cos(a.facing);return Add(origin,{c*x+s*z,y,-s*x+c*z});
}
bool Valid(const Attachments& a){
 for(const auto p:{a.root,a.focus,a.left,a.right})if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
 return a.alive && std::isfinite(a.facing);
}
void Panel(Renderer& r,const Attachments& a,Art art,XMFLOAT3 p,float width,float height,float tilt,XMFLOAT4 tint){
 const auto at=[&](float x,float y){return Local(a,p,(x*std::cos(tilt)-y*std::sin(tilt)),x*std::sin(tilt)+y*std::cos(tilt));};
 r.DrawEffectSurface(art,at(-width,-height),at(-width,height),at(width,height),at(width,-height),tint);
}
void Seal(Renderer& r,const Attachments& a,float radius,float phase,XMFLOAT4 tint){
 // A ground projection follows terrain, never an airborne root's altitude.
 const auto at=[&](float x,float z){const float c=std::cos(phase),s=std::sin(phase);
  const rpg::Vec2 p{a.root.x+(c*x-s*z)*radius,a.root.z+(s*x+c*z)*radius};return XMFLOAT3{p.x,rpg::WorldMap::Height(p)+.045f,p.z};};
 r.DrawEffectSurface(Art::Seal,at(-1,-1),at(-1,1),at(1,1),at(1,-1),tint);
}
void Stream(Renderer& r,const Attachments& a,XMFLOAT3 from,XMFLOAT3 to,float bend,float phase,float opacity,XMFLOAT4 color,int segments=12){
 const auto at=[&](float t){auto p=Mix(from,to,t);return Local(a,p,std::sin(t*pi)*std::sin(t*7+phase)*bend,std::sin(t*pi)*bend);};
 for(int n=0;n<segments;++n){const float u=float(n)/segments,v=float(n+1)/segments;
  auto c=color;c.w*=opacity*std::sin((u+v)*pi*.5f);r.DrawEffectStreak(at(u),at(v),.012f+.012f*opacity,c);}
}
void Orbit(Renderer& r,const Attachments& a,float radius,float height,float phase,float span,float width,XMFLOAT4 color,int segments=20){
 const auto at=[&](float u,float edge){const float angle=phase+span*u;
  return Local(a,a.root,std::cos(angle)*radius,height+std::sin(angle)*.08f+edge,std::sin(angle)*radius);};
 for(int n=0;n<segments;++n){const float u=float(n)/segments,v=float(n+1)/segments;
  r.DrawEffectRibbon(at(u,-width),at(u,width),at(v,width),at(v,-width),color,std::sin(u*pi),std::sin(v*pi));}
}
XMFLOAT4 RecoveryColor(C id,float alpha){return id==C::Mage?XMFLOAT4{.40f,.76f,1,alpha}:classfx::Color(id,alpha);}
void RecoveryCharge(Renderer& r,const rpg::Simulation& state,const Attachments& a,float progress){
 const auto id=state.Class();const float power=combat::Smooth(progress),alpha=power*.72f;
 auto c=RecoveryColor(id,alpha);const auto chest=Local(a,a.root,0,1.03f,.20f);
 if(id==C::Engineer){
  for(int n=0;n<5;++n){const float phase=std::fmod(progress*3+n*.2f,1.f),angle=n*2.4f;
   const auto spark=Local(a,a.right,std::cos(angle)*(.12f+.15f*phase),-.18f*phase,std::sin(angle)*.15f);
   r.DrawEffectSprite(Art::Sparks,spark,.045f,.08f,angle,{1,.68f,.28f,alpha*std::sin(phase*pi)});}
  Stream(r,a,a.right,a.left,.04f,progress*10,1,c);return;
 }
 Seal(r,a,.50f+power*.25f,-progress*.45f,{c.x,c.y,c.z,alpha*.48f});
 Stream(r,a,a.left,chest,.16f,progress*4,1,c);Stream(r,a,a.right,chest,-.16f,-progress*4,1,c);
 if(id==C::Druid){
  for(int n=0;n<6;++n){const float u=std::fmod(progress*1.3f+n/6.f,1.f),angle=n*tau/6+progress;
   const auto p=Local(a,a.root,std::sin(angle)*(.6f-u*.35f),.2f+u*1.1f,std::cos(angle)*(.6f-u*.35f));
   Panel(r,a,Art::Slash,p,.07f,.16f,angle,{.3f,.8f,.48f,alpha*std::sin(u*pi)});}
 }else if(id==C::Bard){
  for(int n=0;n<3;++n){const float beat=std::fmod(progress*2+n/3.f,1.f);
   Orbit(r,a,.25f+beat*.45f,.35f+beat*.8f,progress*3+n,tau*.65f,.015f,{.42f,.78f,1,alpha*std::sin(beat*pi)},12);}
 }else{
  for(const auto p:{a.left,a.right})Panel(r,a,Art::Seal,p,.13f+power*.05f,.13f+power*.05f,progress,{1,.85f,.48f,alpha*.7f});
  for(int n=0;n<5;++n){const float u=std::fmod(progress*1.3f+n*.2f,1.f),angle=n*tau/5;
   r.DrawEffectSprite(Art::Sparks,Local(a,a.root,std::cos(angle)*.5f,.20f+u*1.35f,std::sin(angle)*.5f),.055f,.13f,0,{1,.88f,.54f,alpha*std::sin(u*pi)});}
 }
}
}
Attachments Preview(XMFLOAT3 root,XMFLOAT3 focus,float facing){
 Attachments a;a.root=root;a.focus=focus;a.facing=facing;a.left=Local(a,root,-.32f,1.15f,.15f);a.right=Local(a,root,.32f,1.15f,.15f);return a;
}
Attachments FromPose(const CharacterPose& pose,XMFLOAT3 root,float facing,rpg::WeaponForm form,bool shield,bool weapon,bool alive){
 Attachments a;a.root=root;a.facing=facing;a.alive=alive;
 const auto world=[&](XMFLOAT3 p){return Local(a,root,p.x,p.y,p.z);};
 a.left=world(BuildHandPose(pose,0,weapon,shield,pose.handCast).Point({0,.045f,.025f}));
 a.right=world(BuildHandPose(pose,1,weapon,shield,pose.handCast).Point({0,.045f,.025f}));
 a.focus=world(rpg::PhysicalLauncher(form)?WeaponReleasePoint(pose,form):SpellReleasePoint(pose,shield));return a;
}
void Prepare(Renderer& r,const rpg::Simulation& state,const Attachments& a){
 if(!Valid(a) || state.Health()<=0)return;
 const bool recovery=state.CastingRecovery();
 if(!recovery && (!state.Attacking() || state.ClassInfo().range<=3))return;
 const float progress=recovery?state.RecoveryProgress():state.AttackSeconds()/combat::StrokeStart(state.HeavyAttack());
 if(!std::isfinite(progress) || progress<=0 || progress>=1)return;
 if(recovery){RecoveryCharge(r,state,a,progress);return;}
 const auto id=state.Class();const int ability=state.AttackAbility();const float power=combat::Smooth(progress);
 const float age=state.AttackSeconds()/state.AttackRate();
 if(id==C::Scout || id==C::Engineer){
  if(!state.HeavyAttack())return;
  const float span=.18f-.12f*power;const XMFLOAT4 c{.72f,.86f,.73f,power*.68f};
  for(float side:{-1.f,1.f}){
   r.DrawEffectStreak(Local(a,a.focus,side*span,-.075f),Local(a,a.focus,side*span,.075f),.009f,c);
   r.DrawEffectStreak(Local(a,a.focus,side*span,.075f),Local(a,a.focus,side*(span-.04f),.075f),.009f,c);
   r.DrawEffectStreak(Local(a,a.focus,side*span,-.075f),Local(a,a.focus,side*(span-.04f),-.075f),.009f,c);
  }
  if(id==C::Scout && (ability==9 || ability==11 || ability==14))for(int n=0;n<4;++n){
   const float u=std::fmod(age*1.8f+n*.25f,1.f),side=n%2?1.f:-1.f;
   const auto p=Local(a,a.focus,side*(1-u)*.35f,(n/2?1.f:-1.f)*(1-u)*.16f,-(1-u)*.45f);
   r.DrawEffectStreak(p,Mix(p,a.focus,.24f),.010f,{.67f,.82f,.73f,power*std::sin(u*pi)*.55f});}
  return;
 }
 if(id==C::Priest){
  const float radius=.20f+power*.20f;
  Panel(r,a,Art::Seal,a.focus,radius,radius,-age*.5f,{1,.84f,.4f,power*.65f});
  for(int n=0;n<6;++n){const float angle=n*tau/6+age*.4f;const auto p=Local(a,a.focus,std::cos(angle)*radius*1.4f,std::sin(angle)*radius*1.4f);
   Stream(r,a,p,a.focus,.045f,age*3,power,{1,.94f,.72f,.7f},6);}
  Stream(r,a,a.right,a.focus,.16f,age*3,power,{1,.83f,.46f,.65f});
  if(state.HeavyAttack())Seal(r,a,.70f+power*.2f,0,{1,.83f,.4f,power*.26f});return;
 }
 const bool mage=id==C::Mage,frost=mage && (ability==5 || ability==6 || ability==8 || ability==12),arcane=mage && (ability==10 || ability==13),thermal=mage && ability==14;
 const bool large=mage && (ability==8 || ability==9 || ability==11 || thermal);
 const auto color=frost?XMFLOAT4{.42f,.78f,1,.75f}:arcane?XMFLOAT4{.73f,.48f,1,.72f}:classfx::Color(id,.75f);
 const auto cool=XMFLOAT4{.42f,.78f,1,.75f};const float scale=large?1.35f:1.f;
 // Converging material fragments come from the solved other hand. The focus
 // remains the exact socket used by Game for projectile release.
 Stream(r,a,a.right,a.focus,.13f,age*3,power*.65f,color);
 for(int n=0;n<(thermal?8:6);++n){
  const float u=std::fmod(age*1.2f+n/(thermal?8.f:6.f),1.f),angle=n*2.4f+age*.9f;
  const float radius=(.38f-.23f*u)*scale;
  const auto p=Local(a,a.focus,std::cos(angle)*radius,std::sin(angle)*radius,-.12f*(1-u));
  const bool ice=frost || (thermal && n%2);auto tint=ice?cool:color;tint.w*=power*std::sin(u*pi);
  if(ice)r.DrawEffectSprite(Art::Ice,p,.035f*scale,.12f*scale,-angle,tint);
  else if(mage && !arcane)r.DrawEffectSprite(Art::Flame,p,.07f*scale,.12f*scale,-angle*.12f,{1,.84f,.55f,tint.w*.85f},true);
  else r.DrawEffectSprite(Art::Arcane,p,.065f*scale,.065f*scale,angle,tint);
 }
 const float radius=(mage?.14f+.22f*power:.09f+.13f*power)*scale;
 if(frost){
  Panel(r,a,Art::Ice,a.focus,radius*.45f,radius*1.15f,age*.25f,{.60f,.85f,1,power*.9f});
  for(int n=0;n<6;++n){const float angle=n*tau/6+age*.45f;
   r.DrawEffectStreak(Local(a,a.focus,std::cos(angle)*radius*.55f,std::sin(angle)*radius*.55f),
    Local(a,a.focus,std::cos(angle)*radius*1.25f,std::sin(angle)*radius*1.25f),.010f,{.55f,.84f,1,power*.68f});}
 }
 else if(mage && !arcane){r.DrawEffectSprite(Art::Flame,a.focus,radius*.8f,radius*1.2f,.1f*std::sin(age*2),{1,.85f,.57f,power*.85f},true);
  if(thermal)Panel(r,a,Art::Ice,a.focus,radius*.32f,radius*1.25f,-age*.5f,{.56f,.84f,1,power*.8f});}
 else Panel(r,a,Art::Seal,a.focus,radius,radius,-age*.65f,{color.x,color.y,color.z,power*.8f});
 if(large){
  const auto footColor=thermal?cool:color;
  Seal(r,a,.62f+power*.33f,age*.15f,{footColor.x,footColor.y,footColor.z,power*.32f});
  for(int band=0;band<2;++band)Orbit(r,a,.50f+power*.18f,.24f+band*.45f,age*(band?-1.f:1.f)+band*pi,pi*1.15f,.019f,
    {band && thermal?cool.x:color.x,band && thermal?cool.y:color.y,band && thermal?cool.z:color.z,power*.36f},18);
 }
}
void Discipline(Renderer& r,const rpg::Simulation& s,const Attachments& a,float clock){
 if(!Valid(a) || !rpg::HasDiscipline(s.Class()))return;
 const float remaining=s.DisciplineSeconds(),duration=s.Class()==C::Mage?8.f:s.Class()==C::Priest?6.f:5.f;
 const float fade=combat::Smooth((duration-remaining)/.12f)*combat::Smooth(remaining/.18f);
 if(s.Class()==C::Warrior){
  if(s.GuardWindow()>0)Panel(r,a,Art::Slash,a.left,.18f,.36f,-.2f,{.8f,.85f,1,combat::Smooth(s.GuardWindow()/.12f)*.48f});
  if(remaining>0)for(float side:{-1.f,1.f}){
   const auto p=Local(a,a.right,side*.10f,.10f,0);
   Panel(r,a,Art::Slash,p,.065f,.26f,side*.35f,{1,.73f,.38f,fade*(.48f+.12f*std::sin(clock*5))});}
 }else if(s.Class()==C::Priest){
  if(remaining<=0)return;
  for(const auto p:{a.left,a.right}){Panel(r,a,Art::Seal,p,.19f,.19f,clock*.3f,{1,.85f,.44f,fade*.65f});
   r.DrawEffectStreak(Local(a,p,0,-.15f),Local(a,p,0,.27f),.025f,{1,.94f,.72f,fade*.6f});}
 }else if(s.Class()==C::Scout){
  if(remaining<=0)return;
  for(int n=0;n<3;++n){const auto p=Local(a,a.focus,(n-1)*.12f,.02f*std::sin(clock*4+n),-.10f);
   const auto end=Local(a,p,0,.16f,.10f);
   r.DrawEffectStreak(p,end,.014f,{.65f,.91f,.70f,fade*.75f});
   r.DrawEffectStreak(Local(a,p,-.035f,.07f),end,.009f,{.85f,.95f,.74f,fade*.55f});}
 }else{
  const bool casting=s.ConfluenceCast() && s.AttackSeconds()<combat::StrokeStart(s.HeavyAttack());
  const unsigned mask=remaining>0 || casting?7:s.ElementMask();
  const XMFLOAT4 colors[]={{1,.40f,.10f,1},{.40f,.78f,1,1},{.75f,.46f,1,1}};
  const auto center=casting?a.focus:Local(a,a.root,0,1.18f,0);
  for(int n=0;n<3;++n)if(mask&(1u<<n)){
   const float angle=clock*.75f+n*tau/3,rr=casting?.32f:.57f;
   const auto p=Local(a,center,std::cos(angle)*rr,.15f*std::sin(angle),std::sin(angle)*rr);
   auto color=colors[n];color.w=casting?.75f:remaining>0?fade*.70f:.38f;
   r.DrawEffectSprite(n==0?Art::Flame:n==1?Art::Ice:Art::Arcane,p,.065f,.13f,-angle,color,n==0);
   if(remaining>0 || casting)Stream(r,a,p,center,.06f,clock+n,.55f,color,6);
  }
 }
}
void DisciplinePulse(Renderer& r,const rpg::CombatVisualEvent& e,float age,const Attachments* caster){
 if(age<=0 || age>=.65f || !std::isfinite(age))return;
 if(e.actor==0 && caster && !Valid(*caster))return;
 const float t=age/.65f,fade=combat::Smooth(age/.045f)*(1-combat::Smooth(t));
 auto a=caster && e.actor==0?*caster:Preview({e.position.x,e.height-1.f,e.position.z},{e.position.x,e.height,e.position.z},e.facing);
 const auto p=e.actor?XMFLOAT3{e.position.x,e.height,e.position.z}:e.school==C::Warrior?a.right:e.school==C::Scout?a.focus:Local(a,a.root,0,1.1f,0);
 if(e.school==C::Priest){
  for(int n=0;n<6;++n){const float angle=n*tau/6;const auto tip=Local(a,p,std::cos(angle)*(.25f+t*.45f),std::sin(angle)*(.25f+t*.65f));
   r.DrawEffectStreak(p,tip,.028f,{1,.90f,.57f,fade*.8f});}
  Panel(r,a,Art::Seal,p,.28f+t*.30f,.28f+t*.30f,0,{1,.85f,.4f,fade*.45f});
 }else if(e.school==C::Mage){
  const XMFLOAT4 colors[]={{1,.45f,.17f,fade*.7f},{.45f,.80f,1,fade*.7f},{.74f,.43f,1,fade*.7f}};
  for(int n=0;n<3;++n){const float angle=n*tau/3+age*1.5f;const auto tip=Local(a,p,std::cos(angle)*(.25f+t*.38f),std::sin(angle)*(.25f+t*.38f));
   Panel(r,a,Art::Seal,tip,.11f,.11f,angle,colors[n]);Stream(r,a,p,tip,.08f,age*4,.8f,colors[n],8);}
 }else if(e.school==C::Warrior){
  for(float side:{-1.f,1.f})Panel(r,a,Art::Slash,p,.12f,.30f+t*.25f,side*.65f,{1,.77f,.43f,fade*.75f});
  for(int n=0;n<8;++n){const float angle=n*tau/8;const auto tip=Local(a,p,std::cos(angle)*age,.08f+std::sin(angle)*age-age*age);
   r.DrawEffectSprite(Art::Sparks,tip,.035f,.09f,angle,{1,.77f,.42f,fade*.8f});}
 }else if(e.school==C::Scout){
  for(int n=0;n<5;++n){const float x=(n-2)*(.07f+t*.10f);const auto base=Local(a,p,x,-.15f+t*.20f,-t*.15f);
   r.DrawEffectStreak(base,Local(a,base,x*.3f,.28f,.14f),.019f,{.66f,.9f,.72f,fade*.72f});}
  r.DrawEffectSprite(Art::Smoke,p,.22f+t*.25f,.12f+t*.12f,0,{.58f,.68f,.57f,fade*.13f},true);
 }
}
void Complete(Renderer& r,const rpg::CombatVisualEvent& e,float age,const Attachments& a){
 if(!Valid(a) || e.kind!=rpg::CombatVisualKind::Mend || !std::isfinite(age) || age<=0 || age>=.8f)return;
 const float t=age/.8f,fade=combat::Smooth(age/.055f)*(1-combat::Smooth(t));
 auto c=RecoveryColor(e.school,fade*.72f);const auto chest=Local(a,a.root,0,1.0f,.1f);
 if(e.school==C::Warrior || e.school==C::Engineer){
  // Protection: overlapping metal-like facets at the guard hand, not healing.
  for(int n=-1;n<=1;++n)Panel(r,a,Art::Slash,Local(a,a.left,n*.09f,-.05f,0),.13f,.28f,n*.25f,{.78f,.86f,.93f,fade*.7f});
  for(int n=0;n<7;++n){const float angle=n*tau/7;
   const auto p=Local(a,a.left,std::cos(angle)*age*.8f,.05f+std::sin(angle)*age-age*age,age*.15f);
   r.DrawEffectSprite(Art::Sparks,p,.04f,.10f,angle,{1,.68f,.28f,fade*.8f});}
  r.DrawEffectSprite(Art::Smoke,Local(a,chest,0,age*.14f,.1f),.30f+t*.25f,.17f+t*.14f,0,{.55f,.55f,.50f,fade*.15f},true);return;
 }
 if(e.school==C::Scout){
  for(int n=0;n<3;++n)Orbit(r,a,.38f+t*.15f,.35f+n*.32f,age*3+n,pi*1.45f,.04f,{.69f,.82f,.67f,fade*.52f},16);
  for(int n=0;n<4;++n)r.DrawEffectSprite(Art::Smoke,Local(a,a.root,std::sin(n*1.6f)*.35f,.15f+t*.28f,std::cos(n*1.6f)*.35f),.22f,.14f,float(n),{.58f,.67f,.60f,fade*.18f},true);return;
 }
 if(e.school==C::Mage){
  for(int n=0;n<6;++n){const float angle=n*tau/6,rr=.46f+.10f*std::sin(t*pi);const auto p=Local(a,a.root,std::sin(angle)*rr,.6f+n%2*.35f+age*.3f,std::cos(angle)*rr);
   auto panel=a;panel.facing+=angle;Panel(r,panel,Art::Ice,p,.11f,.35f,std::sin(angle)*.2f,{.50f,.80f,1,fade*.88f});}
  Seal(r,a,.80f+t*.16f,-age*.2f,{.44f,.72f,1,fade*.50f});
  for(int n=0;n<3;++n)r.DrawEffectSprite(Art::Smoke,Local(a,a.root,std::sin(n*2.1f)*.35f,.20f+age*.3f,std::cos(n*2.1f)*.35f),.42f,.24f,float(n),{.57f,.73f,.86f,fade*.28f},true);return;
 }
 if(e.school==C::Priest || e.school==C::Paladin){
  const float span=e.school==C::Priest?.92f:.48f;
  Seal(r,a,.70f+t*.15f,0,{1,.80f,.32f,fade*.56f});
  for(float side:{-1.f,1.f})for(int n=0;n<4;++n){const auto base=Local(a,chest,side*.18f,.02f+n*.10f,0);
   const auto tip=Local(a,chest,side*(span+n*.11f),.22f+n*.13f+age*.16f,-.10f);
   r.DrawEffectStreak(base,tip,.025f,{1,.84f,.48f,fade*.62f});
   Panel(r,a,Art::Slash,Mix(base,tip,.65f),.10f,.27f,side*.65f,{1,.85f,.52f,fade*.55f});}
  for(int n=0;n<5;++n)r.DrawEffectSprite(Art::Sparks,Local(a,a.root,std::cos(n*2.4f)*.42f,.20f+t*1.45f,std::sin(n*2.4f)*.42f),.055f,.18f,0,{1,.90f,.61f,fade*.78f});return;
 }
 if(e.school==C::Druid){
  for(int n=0;n<3;++n){const auto from=Local(a,a.root,std::sin(n*tau/3)*.50f,.08f,std::cos(n*tau/3)*.50f);Stream(r,a,from,chest,.22f,age*3+n,1,c);
   for(int j=0;j<3;++j){const float u=std::fmod(t+j/3.f,1.f);Panel(r,a,Art::Slash,Mix(from,chest,u),.065f,.17f,n+u,{.37f,.85f,.45f,fade*std::sin(u*pi)});}}
  Seal(r,a,.65f,age*.18f,{.35f,.75f,.47f,fade*.38f});return;
 }
 if(e.school==C::Bard){
  for(int n=0;n<3;++n){const float beat=Unit((age-n*.11f)/.47f);if(beat<=0 || beat>=1)continue;
   const float alpha=std::sin(beat*pi)*fade;Orbit(r,a,.28f+beat*.7f,.18f+beat*.85f,n+age*2,pi*1.6f,.022f,{.44f,.78f,1,alpha*.8f},20);
   Panel(r,a,Art::Seal,Local(a,a.root,(n-1)*.32f,.7f+beat*.7f,0),.10f,.16f,age,{.60f,.82f,1,alpha*.7f});}return;
 }
 // Soul/pact flows contract into the torso; mind defence has opposite lenses.
 if(e.school==C::Psionic){
  for(float side:{-1.f,1.f})Panel(r,a,Art::Arcane,Local(a,chest,side*(.65f-t*.22f),.1f,0),.19f,.38f,side*age,{.71f,.48f,1,fade*.85f});
  Stream(r,a,a.left,a.right,.14f,age*3,1,c);
 }else for(int n=0;n<6;++n){const float angle=n*tau/6+age;const auto from=Local(a,chest,std::cos(angle)*(.85f-t*.40f),std::sin(angle)*.25f,std::sin(angle)*(.85f-t*.4f));
  Stream(r,a,from,chest,.08f,age*5+n,1,c,8);
  r.DrawEffectSprite(e.school==C::Demonologist?Art::Slash:Art::Arcane,Mix(from,chest,t),.10f,.16f,angle,{c.x,c.y,c.z,fade*.7f});}
}
}
