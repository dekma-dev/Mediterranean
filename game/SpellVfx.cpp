#include "game/SpellVfx.h"
#include "game/SpellTrajectory.h"
#include "game/SpellFlow.h"
#include "game/CastEffects.h"
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
// All contact geometry is expressed in the accepted attack frame. Rotation of
// the camera does not flatten a burst, and changing target cannot steer it.
struct ContactFrame {
 XMFLOAT3 origin;float sn,cs;
 ContactFrame(XMFLOAT3 p,float facing):origin(p),sn(std::sin(facing)),cs(std::cos(facing)){}
 XMFLOAT3 At(float right,float up,float forward)const {
  return Add(origin,{cs*right+sn*forward,up,-sn*right+cs*forward});
 }
};
template<class Path> void ContactRibbon(Renderer& r,const Path& path,float width,XMFLOAT4 color,
 XMFLOAT3 normal={0,1,0},int segments=12){
 const auto edge=[&](float u){const auto a=path(std::max(0.f,u-.005f)),b=path(std::min(1.f,u+.005f));
  auto side=XMVector3Cross(XMLoadFloat3(&normal),XMLoadFloat3(&b)-XMLoadFloat3(&a));
  if(XMVectorGetX(XMVector3LengthSq(side))<1e-8f)side=XMVectorSet(1,0,0,0);
  XMFLOAT3 result;XMStoreFloat3(&result,XMVector3Normalize(side)*(width*(1-u*.72f)));return result;};
 const auto opacity=[](float u){return combat::Smooth(u/.10f)*combat::Smooth((1-u)/.20f);};
 auto a=path(0.f),x=edge(0.f);
 for(int n=0;n<segments;++n){
  const float u=float(n)/segments,v=float(n+1)/segments;
  const auto b=path(v),y=edge(v);
  r.DrawEffectRibbon(Add(a,{-x.x,-x.y,-x.z}),Add(a,x),Add(b,y),Add(b,{-y.x,-y.y,-y.z}),color,opacity(u),opacity(v));
  a=b;x=y;
 }
}
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
// A release plane faces along the accepted trajectory, not the camera. It
// remains attached to the sampled launch socket even when selection changes.
void Release(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age){
 if(age>=.18f)return;
 if(castfx::Discharge(r,e,origin,age))return;
 const float fade=Envelope(age,.14f,.008f);
 const float sx=std::sin(e.facing),cx=std::cos(e.facing);
 const auto local=[&](float x,float y,float z=0.f){return Add(origin,{cx*x+sx*z,y,-sx*x+cx*z});};
 const bool physical=e.school==C::Scout;
 const auto element=School(e.ability);const bool holy=e.school==C::Priest,cold=!holy && element==Element::Frost,arcane=!holy && element==Element::Arcane;
 const XMFLOAT4 tint=holy?XMFLOAT4{1,.88f,.51f,fade*.7f}:physical?XMFLOAT4{.70f,.83f,.72f,fade*.35f}:cold?XMFLOAT4{.45f,.78f,1,fade*.7f}:
  arcane?XMFLOAT4{.68f,.38f,1,fade*.7f}:XMFLOAT4{1,.47f,.12f,fade*.65f};
 const float radius=(physical?.09f:.25f)+age*(physical?1.f:3.4f);
 const bool fire=!physical&&!holy&&!cold&&!arcane;
 if(!fire)r.DrawEffectSurface(physical?Art::Slash:cold?Art::Ice:Art::Seal,local(-radius,-radius),local(-radius,radius),local(radius,radius),local(radius,-radius),tint);
 else for(int petal=0;petal<3;++petal){
  const float angle=petal*tau/3,open=combat::Smooth(age/.10f);
  FlameStream(r,[&](float u){const float turn=angle+u*1.7f,rr=std::sin(u*pi)*(.12f+open*.38f);
   return local(std::cos(turn)*rr,std::sin(turn)*rr,-u*(.12f+open*.36f));
  },{sx,0,cx},.14f,.75f,age+petal*.23f,{1,1,1,fade*.75f},6);
 }
 if(!physical)for(int n=0;n<5;++n){
  const float a=n*tau/5+e.ability*.4f;const auto p=local(std::cos(a)*age*1.3f,std::sin(a)*age*1.3f,-age*.65f);
  r.DrawEffectStreak(p,local(std::cos(a)*age*.75f,std::sin(a)*age*.75f,-age*.3f),.012f,tint);
 }
 if(!physical){
  // Brief material ignition at the real socket; the larger translucent body
  // survives the bright core so a release is readable at a normal camera zoom.
  const float body=Envelope(age,.18f,.015f),core=Envelope(age,.085f,.004f),size=e.heavy?.82f:.58f;
  if(!holy && !cold && !arcane)r.DrawSpellVolume(Renderer::SpellMedium::Fire,local(0,0,age*.4f),{sx,0,cx},
    {size*.34f,size*.34f,size*.50f},age,{1,1,1,body*.85f},e.actor+unsigned(std::max(0,e.ability)));
  for(int n=0;n<3;++n){const float a=n*tau/3+age*4;
   if(!holy && !cold && !arcane)continue;
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
 const ContactFrame frame(p,e.facing);
 if(arcane){
  r.DrawSpellVolume(Renderer::SpellMedium::Arcane,p,{frame.sn,0,frame.cs},{.50f*size,.58f*size,.35f*size},age,
    {1,1,1,fade*.55f},e.actor);
  // Split the arriving sphere into folding glass and curved pressure paths.
  // The empty middle is intentional: the hit actor must remain legible.
  const float open=combat::Smooth(age/.23f),fold=combat::Smooth((age-.16f)/.45f);
  for(int n=0;n<3;++n){
   const float angle=n*tau/3+age*.8f,cs=std::cos(angle),sn=std::sin(angle);
   const auto at=[&](float x,float y,float z){return frame.At((cs*x-sn*z)*size,y*size,(sn*x+cs*z)*size);};
   const float radius=.12f+open*.52f,w=.12f+(1-fold)*.16f;
   const auto a=at(radius-w,-.40f+fold*.25f,-.10f),b=at(radius-w*.6f,.45f,-.14f-fold*.20f);
   const auto c=at(radius+w*.5f,.64f-fold*.20f,.12f),d=at(radius+w,-.28f,.14f+fold*.20f);
   r.DrawEffectSurface(Art::Arcane,a,b,c,d,{.47f,.30f,1,fade*.35f});
   r.DrawEffectStreak(a,b,.014f*size,{.68f,.86f,1,fade*.72f});
   r.DrawEffectStreak(b,c,.019f*size,{.92f,.77f,1,fade*.8f});
   if(e.primary)ContactRibbon(r,[&](float u){
    const float t=angle+u*2.0f,radius=(.12f+open*.62f)*(1-u*.35f);
    return frame.At(std::cos(t)*radius*size,(u-.5f)*1.15f*size,std::sin(t)*radius*size);
   },.055f*size,{.62f,.46f,1,Envelope(age,.55f,.018f)*.65f});
  }
  r.DrawEffectSprite(Art::Arcane,p,.38f*size,.48f*size,age*.7f,{.63f,.49f,1,burst*.52f});
  if(e.ability==13)r.DrawEffectSprite(Art::Seal,p,(.22f+age*.9f)*size,(.22f+age*.9f)*size,-age*.7f,{.73f,.65f,1,fade*.28f});
 }else{
  const unsigned seed=e.actor*37u+unsigned(std::max(e.ability,0))*71u;
  const unsigned count=frost?10:6;
  for(unsigned n=0;n<count;++n){
   const float a=n*tau/count+Rand(seed+n)*.35f,speed=.8f+Rand(seed+n*17)*1.25f;
   const bool ice=frost || (mixed && n%2==0);
   if(ice){
    const auto shardAt=[&](float t){const float spread=.10f+speed*t*1.25f;
     return frame.At(std::cos(a)*spread*size,std::sin(a)*spread*size-1.8f*t*t,
      -t*(.4f+Rand(seed+n*11)*2.8f)*size);};
    const auto centre=shardAt(age);const ContactFrame shard(centre,e.facing+a+age*(n%2?2.f:-2.f));
    const float length=(.17f+Rand(seed+n*3)*.18f)*size,w=length*.30f;
    // Two crossing fractured surfaces retain thickness from side/rear views.
    for(int side=0;side<2;++side){const auto at=[&](float x,float y){return side?shard.At(0,y,x):shard.At(x,y,0);};
     r.DrawEffectSurface(Art::Ice,at(-w,-length*.5f),at(-w*.35f,length),at(w*.30f,length*.75f),at(w,-length*.6f),{.37f,.69f,1,fade*.62f});}
    r.DrawEffectStreak(shardAt(std::max(0.f,age-.045f)),centre,.012f*size,{.74f,.94f,1,fade*.7f});
   }else{
    const auto at=frame.At(std::cos(a)*age*speed*size,age*(.6f+speed*.35f),-age*(.35f+std::sin(a)*.25f)*size);
    const auto tip=frame.At(std::cos(a)*(.25f+age*speed)*size,age*(.6f+speed*.35f)+.40f,-age*(.35f+std::sin(a)*.25f)*size-.10f);
    const float radius=(.24f+age*.48f)*size;
    if(n%2==0){
     auto centre=at;XMFLOAT3 axis{tip.x-at.x,tip.y-at.y,tip.z-at.z},radii{radius*.82f,radius*.82f,radius*1.35f};
     float opacity=fade*.86f;
     if(n==0 && !mixed){
      // Morph the arriving heart into the first plume instead of spending an
      // additional crowd-volume slot or popping a new plume in at its expiry.
      const float spread=combat::Smooth(age/.11f),depth=(.32f-.26f*spread)*size;
      const float open=combat::Smooth((age-.045f)/.15f);
      const auto mix=[](XMFLOAT3 a,XMFLOAT3 b,float t){return XMFLOAT3{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};};
      centre=mix(frame.At(0,0,-depth),at,open);axis=mix({frame.sn,0,frame.cs},axis,open);
      radii=mix({(.25f+.53f*spread)*size,(.25f+.40f*spread)*size,depth},radii,open);
      opacity=Envelope(age,.20f,.005f)*.90f*(1-open)+opacity*open;
     }
     r.DrawSpellVolume(Renderer::SpellMedium::Fire,centre,axis,radii,age+n*.13f,{1,1,1,opacity},seed+n);
    }
   }
  }
  if(!frost && e.primary){
   // A rapid, curled detonation opens towards the incoming projectile. The
   // moving crest dies before the slower flame/smoke; it is not a static star.
   const float expand=1-std::pow(1-Unit(age/.34f),3.f),crest=Envelope(age,.40f,.012f);
   for(int lobe=0;lobe<3;++lobe)FlameStream(r,[&](float u){
    const float angle=lobe*tau/3+u*1.40f+age*1.6f,radius=(.12f+expand*.78f*u)*size;
    return frame.At(std::cos(angle)*radius,std::sin(angle)*radius+age*.22f,-(.08f+u*.35f)*expand*size);
   },{frame.sn,0,frame.cs},.26f*size,1.4f*size,age+lobe*.23f,{1,1,1,crest*.78f},8);
   // Detached ballistic embers retain their own trajectories after the flash.
   for(unsigned n=0;n<9;++n){const float a=Rand(seed+n*29)*tau,speed=1.2f+Rand(seed+n*53)*2.8f;
    const auto at=[&](float t){return frame.At(std::cos(a)*t*speed*size,
     std::sin(a)*t*speed*size+t*.8f-2.1f*t*t,-t*(.3f+Rand(seed+n*11))*size);};
    r.DrawEffectStreak(at(std::max(0.f,age-.024f)),at(age),.009f*size,{1,.65f,.16f,Envelope(age,.56f,.012f)*.75f});
   }
  }
  const float residue=Envelope(age-.09f,ContactLifetime(e)-.09f,.09f);
  for(int n=0;n<2;++n){
   const auto smoke=frame.At((n?1.f:-1.f)*age*.30f,age*.52f,-age*.10f);
   const float radius=(.12f+age*.55f)*size;
   r.DrawEffectSprite(Art::Smoke,smoke,radius,radius*.75f,n*2.f+age*.12f,
     frost?XMFLOAT4{.64f,.78f,.88f,residue*.24f}:XMFLOAT4{.30f,.26f,.23f,residue*.24f},true);
  }
 }
 r.DrawEffectSprite(frost?Art::Ice:Art::Sparks,p,.38f*size,.38f*size,.7f,
   frost?XMFLOAT4{.52f,.85f,1,burst*.9f}:arcane?XMFLOAT4{.72f,.80f,1,burst*.75f}:XMFLOAT4{1,.85f,.62f,burst*.8f});
 if(e.primary && (e.heavy || e.chain>=2)){
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
void PressureCrown(Renderer& r,rpg::Vec2 centre,float radius,float age,XMFLOAT4 color){
 // A travelling, torn sheet rather than a static circular stamp. The front
 // conforms to terrain; the raised rim rolls over before collapsing.
 const float life=Envelope(age,.46f,.018f),height=.12f+.78f*std::sin(combat::Smooth(age/.46f)*pi);
 color.w*=life;
 const auto at=[&](float a,bool top){const float torn=.64f+.23f*std::sin(a*7+age*9)+.13f*std::sin(a*13-age*5);
  const float distance=radius+(top?-.10f:.12f);
  return Ground({centre.x+std::sin(a)*distance,centre.z+std::cos(a)*distance},.065f+(top?height*torn:0));};
 for(int n=0;n<32;++n){const float a=n*tau/32,b=(n+1)*tau/32;
  r.DrawEffectRibbon(at(a,false),at(a,true),at(b,true),at(b,false),color,1,1);}
}
void ImpactCrystal(Renderer& r,XMFLOAT3 base,float angle,float height,float width,float fade){
 const XMFLOAT3 tip=Add(base,{std::cos(angle)*height*.27f,height,std::sin(angle)*height*.27f});
 const XMFLOAT3 tail=Add(base,{0,-.035f,0});
 for(int face=0;face<4;++face){
  const float a=angle+face*pi*.5f,b=a+pi*.5f;
  const auto p=Add(base,{std::cos(a)*width,height*.24f,std::sin(a)*width});
  const auto q=Add(base,{std::cos(b)*width,height*.24f,std::sin(b)*width});
  r.DrawEffectSurface(Art::Ice,tail,p,tip,q,{.35f,.71f,1,fade*.65f});
  if(face%2==0)r.DrawEffectStreak(p,tip,.012f,{.77f,.95f,1,fade*.8f});
 }
}
void MajorContact(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 p,float age){
 if(!e.primary)return;
 const auto floor=Ground(e.position);const float bloom=Envelope(age,.86f,.028f);
 if(e.school==C::Mage && (e.ability==8 || e.ability==9 || e.ability==11)){
  const bool frost=e.ability==8,storm=e.ability==11;
  const float radius=(storm?4.5f:3.f)*combat::Smooth(age/(frost?.48f:.38f));
  GroundWave(r,e.position,std::max(.1f,radius),age,frost?XMFLOAT4{.43f,.80f,1,bloom*.55f}:XMFLOAT4{1,.48f,.10f,bloom*.52f});
  if(!frost && age<.46f)PressureCrown(r,e.position,std::max(.15f,radius),age,{1,.48f,.13f,.47f});
  const int count=storm?10:8;
  for(int n=0;n<count;++n){
   const float a=n*tau/count+e.facing,delay=(n%3)*.025f,t=age-delay;
   if(t<=0)continue;
   const float fade=Envelope(t,.78f,.055f),spread=frost?1.05f+combat::Smooth(t/.38f)*1.65f:storm?(.7f+(n%3)*.85f):.45f+t*1.8f;
   const auto base=Ground({e.position.x+std::cos(a)*spread,e.position.z+std::sin(a)*spread});
   if(frost){
    const float h=(.50f+(n%3)*.25f)*combat::Smooth(t/.12f);
    ImpactCrystal(r,base,a,h,.13f,fade);
   }else{
    const float launch=combat::Smooth(t/.10f),height=((storm?1.7f:2.1f)+(n%3)*.35f)*launch*(1-.55f*combat::Smooth(t/.78f));
    const auto at=Add(base,{std::sin(t*3+a)*.16f,height*.48f+t*.3f,0});
    if(n%3==0)r.DrawSpellVolume(Renderer::SpellMedium::Fire,at,{std::cos(a)*.26f,1,std::sin(a)*.26f},
      {.24f+t*.30f,.24f+t*.30f,height*.58f},t,{1,1,1,fade*.82f},e.actor+n*23);
    // Thin torn tongues follow the outward blast and curl upwards. The body
    // is animated 3D density; no duplicate flat flame images are stacked here.
    const ContactFrame frame(at,a);
    ContactRibbon(r,[&](float u){return frame.At(std::sin(u*4+t*7)*.12f*u,(u-.5f)*height,-u*u*.45f);},
      .045f,{1,.62f,.18f,fade*.54f},{frame.sn,0,frame.cs},8);
    r.DrawEffectSprite(Art::Smoke,Add(at,{0,.35f,0}),.35f+t*.65f,.4f+t*.6f,a,{.25f,.24f,.23f,fade*.32f},true);
   }
  }
  if(!frost){
   r.DrawEffectSprite(Art::Sparks,Add(floor,{0,.55f,0}),1.0f,1.1f,0,{1,.88f,.65f,Envelope(age,.24f,.01f)*.8f});
   Dust(r,floor,age,2.7f,e.actor,true);
  }
 }else if(e.school==C::Mage && e.ability==14){
  // Thermal rupture meets hot and cold material in two opposite swept lobes.
  const ContactFrame frame(p,e.facing);
  for(int side:{-1,1})for(int n=0;n<6;++n){const float angle=side*(n*.30f+age*2.2f),radius=.35f+age*2.1f;
   const auto at=frame.At(std::cos(angle)*radius,std::sin(angle)*radius*.65f,side*.20f);
   const float size=(.24f+age*.20f)*(1-n*.065f);
   if(side<0)ImpactCrystal(r,at,angle,size*1.6f,size*.4f,bloom);
   else if(n%3==0)r.DrawSpellVolume(Renderer::SpellMedium::Fire,at,{frame.sn,.45f,frame.cs},
     {size,size*.75f,size*1.6f},age,{1,1,1,bloom*.8f},e.actor+n);}
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
  const float descent=combat::Smooth(age/.13f),collapse=1-combat::Smooth((age-.25f)/.55f);
  for(int side:{-1,1})r.DrawSpellVolume(Renderer::SpellMedium::Holy,Add(floor,{cs*side*.52f,2.6f-descent*.85f,-sn*side*.52f}),{0,1,0},
   {.18f+age*.16f,.18f+age*.16f,1.60f*std::max(.01f,collapse)},age,{1,1,1,bloom*.64f},e.actor+side+1);
 }else if(e.school==C::Priest && e.ability==5){
  const XMFLOAT3 end{e.source.x,e.sourceHeight+1.15f,e.source.z};
  const float front=combat::Smooth(age/.38f);
  for(int side:{-1,1}){
   const auto at=[&](float s){const float u=s*front,wave=std::sin(u*pi);return XMFLOAT3{p.x+(end.x-p.x)*u+std::cos(e.facing)*side*.28f*wave,p.y+(end.y-p.y)*u+.40f*wave,p.z+(end.z-p.z)*u-std::sin(e.facing)*side*.28f*wave};};
   ContactRibbon(r,at,.085f,{1,.67f,.19f,bloom*.35f});
   ContactRibbon(r,at,.022f,{1,.95f,.74f,bloom*.83f});
   for(int n=0;n<3;++n){const float u=std::fmod(age*2.3f+n/3.f,1.f);
    r.DrawEffectGlow(at(u),.06f,{1,.91f,.61f,bloom*.6f*std::sin(u*pi)});}
  }
 }
}
}
void Interrupt(Renderer& r,const rpg::CombatVisualEvent& e,float age) {
 if(e.kind!=K::Interrupt || !std::isfinite(age+e.position.x+e.position.z+e.height+e.facing) || age<0 || age>=.48f)return;
 const bool melee=e.school==C::Warrior,arrow=e.school==C::Scout,holy=e.school==C::Priest;
 const auto tint=melee?XMFLOAT3{1,.64f,.30f}:arrow?XMFLOAT3{.57f,.89f,.72f}:holy?XMFLOAT3{1,.88f,.48f}:XMFLOAT3{.72f,.46f,1};
 const float fade=Envelope(age,.48f,.012f),flash=Envelope(age,.13f,.004f);
 const XMFLOAT3 centre{e.position.x,e.height,e.position.z};
 const float c=std::cos(e.facing),s=std::sin(e.facing);
 const auto at=[&](float x,float y,float z=0.f){return Add(centre,{c*x+s*z,y,-s*x+c*z});};
 const auto color=[&](float alpha){return XMFLOAT4{tint.x,tint.y,tint.z,alpha};};
 // Break a chest-height seal into separated arcs. Its open centre leaves the
 // victim readable; fragments follow the accepted impact plane, not the camera.
 const float radius=.31f+combat::Smooth(age/.32f)*.48f;
 for(int piece=0;piece<6;++piece) {
   const float spin=piece*tau/6+age*(piece%2?.8f:-.8f);
   const float offset=combat::Smooth(age/.22f)*.16f;
   for(int n=0;n<4;++n) {
     const float a=spin+n*.13f,b=a+.13f;
     const auto point=[&](float angle,float edge){return at(std::cos(angle)*(radius+edge)+std::cos(spin)*offset,
       std::sin(angle)*(radius+edge)+std::sin(spin)*offset,age*.18f);};
     r.DrawEffectRibbon(point(a,-.014f),point(a,.014f),point(b,.014f),point(b,-.014f),color(fade*.72f));
   }
   const float travel=.24f+age*1.9f;
   const auto tip=at(std::cos(spin)*travel,std::sin(spin)*travel-age*age*.7f,-age*.22f);
   r.DrawEffectStreak(at(std::cos(spin)*(travel-.12f),std::sin(spin)*(travel-.12f)),tip,.016f,color(fade*.68f));
 }
 if(melee || arrow) {
   const float reach=(melee?.78f:.60f)+age*.45f;
   for(int side:{-1,1})r.DrawEffectStreak(at(-reach,side*reach*.58f),at(reach,-side*reach*.58f),melee?.035f:.017f,color(fade*.8f));
   r.DrawEffectSprite(Art::Slash,centre,melee?.72f:.42f,.65f,-.7f,color(flash*.6f));
 } else if(holy) {
   r.DrawEffectSprite(Art::Seal,centre,.62f+age*.2f,.62f+age*.2f,-age*.8f,color(fade*.32f));
   r.DrawEffectStreak(at(0,-.68f),at(0,.88f),.026f,{1,.95f,.75f,flash*.85f});
   r.DrawEffectStreak(at(-.48f,.12f),at(.48f,.12f),.019f,color(flash*.78f));
 } else {
   for(int side:{-1,1})r.DrawEffectSprite(Art::Arcane,at(side*age*.8f,0),.42f,.66f,side*(.2f+age*1.5f),color(fade*.44f));
 }
 r.DrawEffectSprite(Art::Sparks,centre,.33f,.33f,e.facing,{1,.94f,.83f,flash*.85f});
}
float ResidueLifetime(ResidueKind kind) {
 switch(kind){case ResidueKind::Ember:return 3.2f;case ResidueKind::Frost:case ResidueKind::Thermal:return 2.8f;
 case ResidueKind::Holy:return 1.8f;case ResidueKind::Arcane:return 1.7f;default:return 0;}
}
Residue MakeResidue(const rpg::CombatVisualEvent& e) {
 if(!e.primary || (e.kind!=K::Hit && e.kind!=K::HeavyHit) || !rpg::ValidSpell(e.ability) ||
    (e.school!=C::Mage && e.school!=C::Priest) || !std::isfinite(e.position.x) ||
    !std::isfinite(e.position.z) || !std::isfinite(e.facing))return {};
 const auto school=School(e.ability);
 const auto kind=e.school==C::Priest?ResidueKind::Holy:school==Element::Fire?ResidueKind::Ember:
   school==Element::Frost?ResidueKind::Frost:school==Element::Thermal?ResidueKind::Thermal:ResidueKind::Arcane;
 return {e.position,kind,std::isfinite(e.elapsed)?std::clamp(e.elapsed,0.f,.05f):0,e.facing,
   (e.heavy?1.55f:.85f)*(e.signature || e.ability==8 || e.ability==11?1.25f:1.f),e.actor*37u+unsigned(e.ability)*71u};
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
 if((e.kind!=K::Hit && e.kind!=K::HeavyHit) || !std::isfinite(age)||age<0 || age>=ContactLifetime(e) ||
   !std::isfinite(e.position.x) || !std::isfinite(e.position.z) || !std::isfinite(e.height) || !std::isfinite(e.facing))return;
 if(HasClassImpact(e)){ClassImpact(r,e,age);return;}
 const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.36f,e.height,e.position.z-std::cos(e.facing)*.36f};
 MajorContact(r,e,p,age);
 const float power=(e.heavy?1.15f:.72f)*(e.empowered?1.15f:1)*(e.chain>=2?1.12f:1)*(e.primary?1.f:.62f);
 if(e.school==C::Mage){
  MageContact(r,e,p,age,power*(e.heavy?1.8f:1.4f));
  return;
 }
 if(e.school==C::Priest){
  const float fade=Envelope(age,.88f,.025f),flash=Envelope(age,.22f,.008f),size=power*(e.signature?1.25f:1.f);
  const ContactFrame frame(p,e.facing);
  r.DrawSpellVolume(Renderer::SpellMedium::Holy,frame.At(0,.25f*size,0),{0,1,0},
    {(.22f+age*.25f)*size,(.22f+age*.25f)*size,(e.heavy?1.20f:.65f)*size},age,
    {1,1,1,Envelope(age,.50f,.012f)*.72f},e.actor);
  // Staggered shafts descend, then open into vaulted ribs. Crossed surfaces
  // preserve volume at side angles without hiding the victim behind a disc.
  const int shafts=e.heavy && e.primary?3:1;
  for(int n=0;n<shafts;++n){
   const float t=age-n*.026f;if(t<=0)continue;
   const float side=n==0?0:n==1?-.38f:.38f,alpha=Envelope(t,.55f,.012f)*(n?.40f:.66f);
   const float lower=(-.60f+(1-combat::Smooth(t/.105f))*1.4f)*size,upper=(e.heavy?2.7f:1.3f)*size-t*.7f;
   const float width=(n?.08f:.12f)*size;
   for(int axis=0;axis<2;++axis){const auto at=[&](float x,float y){return frame.At(side*size+(axis?0:x),y,axis?x:0);};
    r.DrawEffectSurface(Art::Slash,at(-width,lower),at(-width,upper),at(width,upper),at(width,lower),{1,.88f,.48f,alpha});}
   r.DrawEffectStreak(frame.At(side*size,lower,0),frame.At(side*size,upper,0),width*.18f,{1,.97f,.83f,alpha*.75f});
  }
  if(e.primary){
   const float open=combat::Smooth(age/.24f),glow=Envelope(age-.035f,.64f,.04f);
   for(int n=0;n<4;++n){const float a=n*tau/4+pi*.25f;
    ContactRibbon(r,[&](float u){const float angle=u*pi,radial=(.20f+open*.80f)*std::sin(angle)*size;
     return frame.At(std::cos(a)*radial,(-.52f+u*2.15f)*size,std::sin(a)*radial);
    },.055f*size,{1,.71f,.20f,glow*.62f},{frame.cs*std::sin(a)-frame.sn*std::cos(a),0,-frame.sn*std::sin(a)-frame.cs*std::cos(a)});
   }
   for(int n=0;n<6;++n){const float a=n*tau/6+age*.45f,radial=(.3f+age*.6f)*size;
    const auto at=frame.At(std::cos(a)*radial,(-.5f+age*(1+n*.12f))*size,std::sin(a)*radial);
    r.DrawEffectSprite(Art::Slash,at,.055f*size,.19f*size,0,{1,.94f,.68f,glow*.45f});}
  }
  if(e.heavy && e.primary)Stamp(r,e.position,.8f*size,0,{1,.82f,.35f,fade*.24f});
  r.DrawEffectSprite(Art::Sparks,p,.65f*size,.65f*size,0,{1,.96f,.83f,flash*.8f});return;
 }
 const float flash=Envelope(age,.18f,.008f),fade=Envelope(age,.32f);
 if(e.school==C::Warrior || e.school==C::Demonologist){
  const auto color=e.school==C::Warrior?XMFLOAT4{.82f,.90f,1,fade*.78f}:XMFLOAT4{1,.24f,.25f,fade*.78f};
  const int cuts=e.ability==14 || e.signature?2:1;
  for(int n=0;n<cuts;++n){
   const float tilt=(n?-.65f:.65f)+(e.ability==12?pi:0),cs=std::cos(e.facing),sn=std::sin(e.facing);
   const float travel=combat::Smooth(age/.13f),length=.36f+travel*.46f;
   const auto surface=[&](float x,float y){const float right=(x*std::cos(tilt)-y*std::sin(tilt))*power;
    const float up=(x*std::sin(tilt)+y*std::cos(tilt))*power;
    return Add(p,{cs*right,up,-sn*right});};
   const float centre=-.24f+travel*.48f;
   r.DrawEffectSurface(Art::Slash,surface(centre-length,-.24f),surface(centre-length,.24f),surface(centre+length,.24f),surface(centre+length,-.24f),color);
  }
  r.DrawEffectSprite(Art::Sparks,p,.38f*power,.38f*power,e.facing,{1,.75f,.42f,flash*.9f});
  // A brief blade flash yields to gravity-driven metal fragments, not a ball
  // of light glued to the victim for the whole recovery.
  const ContactFrame frame(p,e.facing);
  for(unsigned n=0;n<(e.heavy?7u:4u);++n){const float a=Rand(e.actor+n*43u)*tau,speed=.8f+Rand(n*23u+e.actor)*1.8f;
   const auto at=[&](float t){return frame.At(std::cos(a)*speed*t,std::sin(a)*speed*t-3.6f*t*t,-t*.4f);};
   r.DrawEffectStreak(at(std::max(0.f,age-.025f)),at(age),.009f,{1,.68f,.29f,Envelope(age,.42f,.008f)*.8f});}
  if(e.heavy)Dust(r,Ground(e.position,.18f),age,power,e.actor,true);
 }else if(e.school==C::Scout || e.school==C::Engineer){
  r.DrawEffectSprite(Art::Sparks,p,.19f*power,.28f*power,e.facing,{.86f,.76f,.48f,flash*.75f});
  Dust(r,p,age,power*.70f,e.actor+unsigned(e.ability));
  if(e.school==C::Scout){const ContactFrame frame(p,e.facing);
   for(unsigned n=0;n<(e.heavy?6u:3u);++n){const float a=n*tau/6+Rand(e.actor+n)*.3f;
    const auto at=[&](float t){return frame.At(std::cos(a)*t*.70f,std::sin(a)*t*.85f-2.5f*t*t,-t*(.9f+Rand(e.actor+n*11)));};
    r.DrawEffectStreak(at(std::max(0.f,age-.035f)),at(age),.013f,{.65f,.52f,.32f,Envelope(age,.36f,.009f)*.8f});
   }
  }
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
void Flight(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age,const FlightPath* path){
 if((e.school!=C::Mage && e.school!=C::Scout && e.school!=C::Priest) || !std::isfinite(age) || !std::isfinite(e.flight) || e.flight<=0 || age<0 || age>=e.flight+.16f)return;
 if(e.kind!=K::SpellCast || !std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) ||
    !std::isfinite(e.destination.x) || !std::isfinite(e.destination.z) || !std::isfinite(e.destinationHeight) || !std::isfinite(e.facing))return;
 Release(r,e,origin,age);
 const float t=Unit(age/e.flight),after=std::max(0.f,age-e.flight);
 const auto point=[&](float u){return path && path->Active()?path->Point(u):ProjectilePosition(e,origin,u);};
 const auto head=point(t);
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
   if(e.heavy || e.ability==9 || e.ability==11 || e.ability==14){
    const float start=std::max(0.f,u-(e.ability==11?3.2f:2.1f)/length),span=(u-start)*length;
    if(span>.001f)for(int sheet=0;sheet<2;++sheet){
     const auto wake=[&](float v){auto p=Add(point(start+(u-start)*v),offset);
      const float curl=std::sin(v*pi)*std::sin(v*5-age*7+sheet*pi)*.10f;
      p.x+=cx*curl;p.y+=sheet?curl:0;p.z-=sx*curl;return p;};
     FilamentStream(r,wake,sheet?XMFLOAT3{cx,0,-sx}:XMFLOAT3{0,1,0},.105f,span,age,
       {.72f,.85f,.73f,arrowFade*.52f},10);
    }
   }
  }
  if(e.ability==13)r.DrawEffectSprite(Art::Sparks,head,.075f,.075f,age*5,{.65f,.75f,.28f,arrowFade*.5f});
  return;
 }
 MagicFlight(r,e,origin,age,path);
}
void EnemyContact(Renderer& r,const rpg::CombatVisualEvent& e,float age,bool fullDetail,const XMFLOAT3* origin){
 if(!std::isfinite(age)||age<0 || age>=.72f)return;
 if(origin && (!std::isfinite(origin->x)||!std::isfinite(origin->y)||!std::isfinite(origin->z)))origin=nullptr;
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
    const auto p=at(u,(strand%2?1.f:-1.f)*(.18f+std::sin(u*pi)*.55f),.90f+wave*.12f*std::sin(u*pi));
    // Snapshot the release socket once, then meet the committed cone. Moving
    // the caster afterwards must not steer an already resolved attack.
    if(!origin)return p;
    const float join=combat::Smooth(std::min(1.f,u/.22f));
    return XMFLOAT3{origin->x+(p.x-origin->x)*join,origin->y+(p.y-origin->y)*join,origin->z+(p.z-origin->z)*join};};
   const float a=float(n)/segments,b=float(n+1)/segments;
   // The accepted hit is instantaneous. Give its first frame a full discharge
   // rather than a nearly invisible line while the lingering body ramps up.
   const float discharge=std::max(body,flash*.85f)*(1-a*.5f);
   r.DrawEffectStreak(bolt(a),bolt(b),execution?.082f:.055f,{.59f,.32f,1,discharge});
   if(strand==0){
     r.DrawEffectStreak(bolt(a),bolt(b),.12f,{.48f,.22f,1,flash*.28f*(1-a*.5f)});
     r.DrawEffectStreak(bolt(a),bolt(b),.020f,{.88f,.79f,1,std::max(flash,body*.6f)*.9f});
   }
  }
  for(int n=0;n<count;++n){const float u=(n+.5f)/count;const auto p=at(u,0,.90f+age*.35f);
   r.DrawEffectSprite(Art::Arcane,p,.16f+age*.17f,.35f+age*.2f,e.facing+age*1.3f,{.62f,.40f,1,body*.66f});
   r.DrawEffectSprite(Art::Smoke,at(u,0,.22f+age*.3f),.16f+age*.32f,.13f+age*.22f,n*.7f,{.40f,.34f,.49f,residue*.32f},true);
  }
  r.DrawEffectSprite(Art::Seal,origin?*origin:point(e.facing,.25f,1.05f),.25f+age*.4f,.25f+age*.4f,-age*3,{.72f,.50f,1,body*.55f});
 }else if(age<.48f){
  for(int n=0;n<4;++n){const float a=e.facing+(n-1.5f)*.35f;
   r.DrawEffectSprite(Art::Dust,point(a,.7f+age,.18f+age*.4f),.16f+age*.68f,.12f+age*.44f,a,{.64f,.52f,.37f,Envelope(age,.48f)*.44f},true);}
 }
}
}
