#include "game/CastEffects.h"
#include "game/SpellFlow.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace castfx {
namespace {
using namespace DirectX;using C=rpg::ClassId;using Art=Renderer::EffectArt;using Medium=Renderer::SpellMedium;
constexpr float pi=3.14159265f,tau=pi*2;
XMFLOAT3 Mix(XMFLOAT3 a,XMFLOAT3 b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
XMFLOAT3 Local(const Attachments& a,XMFLOAT3 p,float x,float y,float z=0){const float s=std::sin(a.facing),c=std::cos(a.facing);return {p.x+c*x+s*z,p.y+y,p.z-s*x+c*z};}
XMFLOAT3 Forward(const Attachments& a){return {std::sin(a.facing),0,std::cos(a.facing)};}
bool Valid(const Attachments& a){
 if(!a.alive || !std::isfinite(a.facing))return false;
 for(auto p:{a.root,a.focus,a.left,a.right})if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
 if(a.blade)for(auto p:{a.edgeBase,a.edgeTip})if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
 return true;
}
void GroundSeal(Renderer& r,const Attachments& a,float radius,XMFLOAT4 tint){
 const auto at=[&](float x,float z){auto p=Local(a,a.root,x*radius,0,z*radius);
  p.y=(a.terrain?rpg::WorldMap::Height({p.x,p.z}):a.root.y)+.035f;return p;};
 r.DrawEffectSurface(Art::Seal,at(-1,-1),at(-1,1),at(1,1),at(1,-1),tint);
}
void Crystal(Renderer& r,const Attachments& a,XMFLOAT3 p,float radius,float height,float angle,float alpha){
 for(int face=0;face<6;++face){const float u=angle+face*tau/6,v=angle+(face+1)*tau/6;
  r.DrawEffectSurface(Art::Ice,Local(a,p,0,-height),Local(a,p,std::cos(u)*radius,-height*.15f,std::sin(u)*radius),
    Local(a,p,0,height),Local(a,p,std::cos(v)*radius,-height*.15f,std::sin(v)*radius),
    {face%2?.46f:.69f,.83f,1,alpha*(face%2?.7f:1.f)});
 }
}
void Steel(Renderer& r,int ability,float power,float age,const Attachments& a){
 if(!a.blade)return;
 const auto delta=XMLoadFloat3(&a.edgeTip)-XMLoadFloat3(&a.edgeBase);
 if(XMVectorGetX(XMVector3LengthSq(delta))<.01f)return;
 const bool strong=ability!=0 && ability!=12;
 // A travelling metallic glint lies on the equipped blade itself. It cannot
 // become a floating magic ball or survive unequipping the weapon.
 const float centre=.10f+.78f*power,half=strong?.22f:.13f;
 const auto start=Mix(a.edgeBase,a.edgeTip,std::max(0.f,centre-half));
 const auto end=Mix(a.edgeBase,a.edgeTip,std::min(1.f,centre+half));
 for(auto normal:{Forward(a),XMFLOAT3{0,1,0}})
  spellvfx::FilamentStream(r,[&](float u){return Mix(start,end,u);},normal,strong?.065f:.035f,.65f,age,
    {.80f,.89f,1,power*(strong?.72f:.34f)},6);
 if(strong){
  for(int n=0;n<3;++n){const float u=std::fmod(age*1.7f+n/3.f,1.f);const auto p=Mix(a.edgeBase,a.edgeTip,u);
   r.DrawEffectSprite(Art::Sparks,p,.022f,.05f,age,{1,.84f,.53f,power*std::sin(u*pi)*.45f});}
 }
}
void Bow(Renderer& r,int ability,float power,float age,const Attachments& a){
 if(ability==0 || ability==7 || ability==12)return;
 // Air is drawn into the actual release socket, from behind the shot. The
 // open streams leave the bow/string readable instead of painting a UI reticle.
 const bool focused=ability==9 || ability==11 || ability==14;
 const int count=ability==8?3:2;
 for(int n=0;n<count;++n){const float angle=n*tau/count+age*.45f;
  const auto path=[&](float u){const auto centre=Mix(a.right,a.focus,u);
   const float reach=std::sin(u*pi)*(.15f+power*.18f);
   return Local(a,centre,std::cos(angle)*reach,std::sin(angle)*reach,-(1-u)*(.18f+power*.22f));};
  spellvfx::FilamentStream(r,path,Forward(a),focused?.070f:.045f,1.2f,age,
    {.73f,.83f,.69f,power*(focused?.52f:.35f)},10);
 }
 // A concentrated point follows the physical nock/muzzle; no forward wake
 // or contact is shown until the simulation emits the accepted shot.
 r.DrawEffectSprite(Art::Sparks,a.focus,.025f+power*.035f,.025f+power*.035f,age,
   {.94f,.87f,.62f,power*.75f});
}
void Elemental(Renderer& r,int ability,float power,float age,const Attachments& a){
 const bool frost=ability==5 || ability==6 || ability==8 || ability==12;
 const bool grand=ability==6 || ability==8;
 const auto forward=Forward(a);
 const float size=.13f+power*.17f;
 r.DrawSpellVolume(frost?Medium::Frost:Medium::Arcane,a.focus,forward,
   {size,size,size*(frost?1.65f:1.1f)},age,{1,1,1,power*.64f},unsigned(ability));
 if(frost){
  // Closed crystal facets converge in three dimensions. There is no large
  // ice image that turns edge-on as the camera rotates around the caster.
  const int count=grand?5:3;
  for(int n=0;n<count;++n){const float angle=n*tau/count+age*.55f;
   const float radius=(.43f-power*.22f)*(grand?1.35f:1.f);
   const auto p=Local(a,a.focus,std::cos(angle)*radius,std::sin(angle)*radius*.80f,(n%2?1.f:-1.f)*radius*.45f);
   Crystal(r,a,p,.035f+power*.045f,.10f+power*.12f,age+n,power*.76f);
  }
  for(int side:{-1,1}){const auto hand=side<0?a.left:a.right;
   spellvfx::FilamentStream(r,[&](float u){auto p=Mix(hand,a.focus,u);return Local(a,p,side*std::sin(u*pi)*.16f,-std::sin(u*pi)*.12f);},
     forward,.09f,.9f,age,{.40f,.73f,1,power*.40f},12);
  }
  r.DrawEffectSprite(Art::Smoke,Local(a,a.focus,0,-.20f-power*.12f),.22f+power*.16f,.12f+power*.12f,age,
    {.45f,.65f,.78f,power*.18f},true);
 }else{
  // Three intersecting spirals contract around the focus while retaining
  // depth; bright knots travel towards the hand rather than orbiting forever.
  for(int n=0;n<3;++n){const float plane=n*tau/3;
   const auto path=[&](float u){const float angle=u*pi*1.6f+age*.8f+n;
    const float radial=(.38f-power*.12f)*(1-u*.70f);
    return Local(a,a.focus,std::cos(plane)*std::cos(angle)*radial,std::sin(angle)*radial,std::sin(plane)*std::cos(angle)*radial);};
   spellvfx::FilamentStream(r,path,{std::cos(a.facing+plane),0,-std::sin(a.facing+plane)},.075f,1.5f,age+n*.2f,
     {.64f,.38f,1,power*.68f},14);
   const float travel=std::fmod(age*.9f+n/3.f,1.f);
   r.DrawEffectSprite(Art::Arcane,path(travel),.045f,.075f,age,{.85f,.70f,1,power*std::sin(travel*pi)*.65f});
  }
 }
 if(grand)GroundSeal(r,a,.65f+power*.18f,{.40f,.72f,1,power*.25f});
}
void Prayer(Renderer& r,int ability,float power,float age,const Attachments& a){
 const bool heal=ability==3,grand=heal || ability==6;
 const auto focus=heal?Local(a,a.root,0,1.03f,.12f):a.focus;
 r.DrawSpellVolume(Medium::Holy,focus,{0,1,0},{.18f+power*.12f,.18f+power*.12f,.28f+power*.27f},age,
   {1,1,1,power*.57f},unsigned(ability));
 for(int side:{-1,1})for(int tier=0;tier<(grand?2:1);++tier){
  const auto hand=side<0?a.left:a.right;
  const auto from=grand?Local(a,a.root,side*(.48f+tier*.22f),.15f,-.15f-tier*.12f):hand;
  const auto path=[&](float u){const auto p=Mix(from,focus,u);const float bend=std::sin(u*pi);
   return Local(a,p,side*bend*(.19f+tier*.12f),bend*(heal?.26f:.18f),-bend*.12f);};
  spellvfx::FilamentStream(r,path,Forward(a),grand?.105f:.065f,grand?2.f:1.f,age+side*.2f+tier*.3f,
    {1,.83f,.46f,power*(tier?.36f:.53f)},12);
 }
 for(const auto hand:{a.left,a.right})spellvfx::FilamentStream(r,[&](float u){return Mix(hand,focus,u);},Forward(a),.055f,.8f,age,
   {1,.95f,.73f,power*.42f},8);
 if(grand){GroundSeal(r,a,.65f+power*.18f,{1,.78f,.30f,power*.26f});
  for(int n=0;n<5;++n){const float u=std::fmod(age*.8f+n*.2f,1.f),angle=n*2.39996f;
   const auto p=Local(a,a.root,std::cos(angle)*(.55f-u*.28f),.16f+u*1.35f,std::sin(angle)*(.55f-u*.28f));
   r.DrawEffectSprite(Art::Slash,p,.025f,.11f,angle,{1,.94f,.68f,power*std::sin(u*pi)*.6f});}
 }
}
}
bool ClassCharge(Renderer& r,C id,int ability,float progress,float age,const Attachments& a){
 if(!Valid(a) || !std::isfinite(progress) || !std::isfinite(age) || age<0 || progress<=0 || progress>=1)return false;
 const float power=combat::Smooth(progress);
 if(id==C::Warrior){Steel(r,ability,power,age,a);return true;}
 if(id==C::Scout){Bow(r,ability,power,age,a);return true;}
 if(id==C::Priest){Prayer(r,ability,power,age,a);return true;}
 if(id==C::Mage && (ability==5 || ability==6 || ability==8 || ability==12 || ability==10 || ability==13)){
  Elemental(r,ability,power,age,a);return true;}
 return false;
}
void PrayerCompletion(Renderer& r,float age,const Attachments& a){
 if(!Valid(a) || !std::isfinite(age) || age<=0 || age>=.8f)return;
 const float rise=combat::Smooth(age/.20f),fade=combat::Smooth(age/.025f)*(1-combat::Smooth(age/.8f));
 // Healing opens upwards from the champion, unlike Sanctuary's outward
 // wings at the victim. The attached fountain moves with its living owner.
 for(int n=0;n<6;++n){const float angle=n*tau/6;
  const auto path=[&](float u){const float radial=.20f+std::sin(u*pi)*(.48f+rise*.22f);
   return Local(a,a.root,std::cos(angle)*radial,.20f+u*(1.55f+rise*.55f),std::sin(angle)*radial);};
  spellvfx::FilamentStream(r,path,{std::cos(a.facing+angle),0,-std::sin(a.facing+angle)},.10f,2.3f,age+n*.14f,
   {1,.84f,.48f,fade*.50f},10);
 }
 r.DrawSpellVolume(Medium::Holy,Local(a,a.root,0,.9f+rise*.35f),{0,1,0},{.32f,.32f,.65f},age,{1,1,1,fade*.35f},31);
 GroundSeal(r,a,.70f+rise*.20f,{1,.82f,.40f,fade*.40f});
 for(int n=0;n<6;++n){const float angle=n*2.39996f;const auto p=Local(a,a.root,std::cos(angle)*.40f,.35f+age*(1.9f+n*.12f),std::sin(angle)*.4f);
  r.DrawEffectSprite(Art::Slash,p,.035f,.18f,age,{1,.96f,.75f,fade*.64f});}
}
bool Discharge(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age){
 const bool physical=e.school==C::Scout,holy=e.school==C::Priest;
 const bool cold=e.school==C::Mage && (e.ability==5 || e.ability==6 || e.ability==8 || e.ability==12);
 const bool arcane=e.school==C::Mage && (e.ability==10 || e.ability==13);
 if(!physical && !holy && !cold && !arcane)return false;
 if(e.kind!=rpg::CombatVisualKind::SpellCast || !std::isfinite(age) || age<0 || age>=.18f ||
   !std::isfinite(origin.x)||!std::isfinite(origin.y)||!std::isfinite(origin.z)||!std::isfinite(e.facing))return true;
 const auto a=Preview({origin.x,origin.y-1.1f,origin.z},origin,e.facing);
 const float open=combat::Smooth(age/.12f),fade=(1-combat::Smooth(age/.18f))*combat::Smooth(age/.006f);
 const XMFLOAT4 tint=physical?XMFLOAT4{.73f,.83f,.69f,fade*.42f}:holy?XMFLOAT4{1,.88f,.52f,fade*.56f}:
   cold?XMFLOAT4{.47f,.79f,1,fade*.56f}:XMFLOAT4{.70f,.42f,1,fade*.58f};
 // Retain the full gathered core at the accepted release, then compress it
 // as the actual projectile grows. No plate suddenly replaces the charge.
 if(!physical && age<.09f){const float collapse=1-combat::Smooth(age/.09f),size=.30f*(.2f+.8f*collapse);
  r.DrawSpellVolume(cold?Medium::Frost:holy?Medium::Holy:Medium::Arcane,origin,Forward(a),
    {size,size,size*(cold?1.65f:holy?1.8f:1.1f)},age,{1,1,1,collapse*.64f},unsigned(std::max(0,e.ability)));
 }
 for(int n=0;n<(physical?2:3);++n){const float angle=n*tau/(physical?2:3)+age;
  const auto path=[&](float u){const float turn=angle+u*(arcane?2.4f:.7f),radius=std::sin(u*pi)*(.12f+open*(physical?.20f:.40f));
   return Local(a,origin,std::cos(turn)*radius,std::sin(turn)*radius,-u*(.16f+open*.40f));};
  spellvfx::FilamentStream(r,path,Forward(a),physical?.065f:holy?.10f:.075f,.9f,age+n*.2f,tint,8);
  if(cold){const auto p=path(.5f);Crystal(r,a,p,.055f,.12f+open*.05f,angle,tint.w*.8f);}
 }
 if(!physical)r.DrawEffectSprite(Art::Sparks,origin,.22f,.22f,0,{1,.95f,.80f,(1-combat::Smooth(age/.075f))*fade*.6f});
 return true;
}
}
