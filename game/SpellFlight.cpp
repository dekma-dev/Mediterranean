#include "game/SpellVfx.h"
#include "game/SpellFlow.h"
#include "game/SpellTrajectory.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace spellvfx {
namespace {
using namespace DirectX;using Medium=Renderer::SpellMedium;using Art=Renderer::EffectArt;
constexpr float pi=3.14159265f,tau=2*pi;
float Unit(float v){return std::clamp(v,0.f,1.f);}
XMFLOAT3 Add(XMFLOAT3 a,XMFLOAT3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
XMFLOAT3 Mul(XMFLOAT3 a,float v){return {a.x*v,a.y*v,a.z*v};}
XMFLOAT3 Sub(XMFLOAT3 a,XMFLOAT3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float Length(XMFLOAT3 p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
float Random(unsigned seed){seed^=seed>>16;seed*=0x7feb352du;seed^=seed>>15;seed*=0x846ca68bu;seed^=seed>>16;return float(seed&65535)/65535;}
struct Frame {
 XMFLOAT3 p,axis,right,up;
 Frame(XMFLOAT3 point,XMFLOAT3 direction):p(point){
  auto d=XMLoadFloat3(&direction);if(XMVectorGetX(XMVector3LengthSq(d))<1e-8f)d=XMVectorSet(0,0,1,0);
  d=XMVector3Normalize(d);const auto reference=std::abs(XMVectorGetY(d))>.95f?XMVectorSet(1,0,0,0):XMVectorSet(0,1,0,0);
  const auto r=XMVector3Normalize(XMVector3Cross(reference,d));
  XMStoreFloat3(&axis,d);XMStoreFloat3(&right,r);XMStoreFloat3(&up,XMVector3Cross(d,r));
 }
 XMFLOAT3 At(float x,float y,float z)const{return Add(p,Add(Mul(right,x),Add(Mul(up,y),Mul(axis,z))));}
};
// Shared end vertices make a continuous taper. A row of isolated analytic
// streaks multiplies its end mask and reads as beads at ordinary combat zoom.
template<class Path> void Ribbon(Renderer& r,const Path& path,XMFLOAT3 normal,float width,XMFLOAT4 color,int segments=14){
 const auto side=[&](float t){auto d=Sub(path(std::min(1.f,t+.004f)),path(std::max(0.f,t-.004f)));
  auto v=XMVector3Cross(XMLoadFloat3(&d),XMLoadFloat3(&normal));
  if(XMVectorGetX(XMVector3LengthSq(v))<1e-8f)v=XMVectorSet(1,0,0,0);
  XMFLOAT3 result;XMStoreFloat3(&result,XMVector3Normalize(v)*(width*(.12f+.88f*t)));return result;};
 const auto fade=[](float t){return combat::Smooth(t/.18f)*(1-.35f*combat::Smooth((t-.85f)/.15f));};
 auto a=path(0.f),s=side(0.f);
 for(int n=0;n<segments;++n){const float u=float(n)/segments,v=float(n+1)/segments;const auto b=path(v),next=side(v);
  r.DrawEffectRibbon(Sub(a,s),Add(a,s),Add(b,next),Sub(b,next),color,fade(u),fade(v));a=b;s=next;}
}
}

void MagicFlight(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age,const FlightPath* path) {
 const bool holy=e.school==rpg::ClassId::Priest;
 const bool frost=!holy && (e.ability==5 || e.ability==6 || e.ability==8 || e.ability==12);
 const bool arcane=!holy && (e.ability==10 || e.ability==13),thermal=!holy && e.ability==14,meteor=!holy && e.ability==9;
 const Medium medium=holy?Medium::Holy:frost?Medium::Frost:arcane?Medium::Arcane:Medium::Fire;
 const float t=Unit(age/e.flight),after=std::max(0.f,age-e.flight);
 const float front=(1-combat::Smooth(after/.025f))*combat::Smooth(age/.018f);
 const float fade=1-combat::Smooth(after/.16f),power=(e.heavy?1.25f:1.f)*(e.empowered?1.12f:1.f);
 const auto point=[&](float u){return path && path->Active()?path->Point(Unit(u)):ProjectilePosition(e,origin,Unit(u));};
 const float length=std::max(.05f,Length(Sub(ProjectilePosition(e,origin,1),ProjectilePosition(e,origin,0))));
 const auto head=point(t);const Frame frame(head,Sub(point(t+.002f),point(t-.002f)));
 const unsigned seed=e.actor*31u+unsigned(std::max(0,e.ability))*47u;
 const bool fire=!holy && !frost && !arcane;
 const float stretch=combat::Smooth(age/.075f);
 const float bodyLength=(meteor?1.02f:holy?.58f:frost?.48f:fire?.36f+.38f*stretch:.56f)*power;
 const float radius=(meteor?.52f:holy?.25f:frost?.18f:fire?.42f-.08f*stretch:.33f)*power;
 // The leading cap ends at the accepted trace. All mass and wakes trail it;
 // a decorative head must never arrive at a target before gameplay contact.
 const float grown=combat::Smooth(age/.035f);
 const auto centre=frame.At(0,0,-bodyLength*grown);
 if(front>0)r.DrawSpellVolume(medium,centre,frame.axis,{radius*grown,radius*grown,bodyLength*grown},age,
   {1,1,1,front*(holy?.86f:.95f)},seed);
 if(thermal && front>0)r.DrawSpellVolume(Medium::Frost,frame.At(.19f*power,0,-bodyLength*.8f),frame.axis,
   {.16f*power,.16f*power,.52f*power},age,{1,1,1,front*.8f},seed+91);

 const XMFLOAT4 hue=holy?XMFLOAT4{1,.72f,.28f,1}:frost?XMFLOAT4{.25f,.65f,1,1}:
   arcane?XMFLOAT4{.60f,.25f,1,1}:XMFLOAT4{1,.28f,.025f,1};
 const float span=(meteor?3.2f:holy?3.f:frost?1.9f:arcane?2.6f:2.4f)*power;
 const float tail=std::max(0.f,t-span/length);
 if(fire && t>tail+.00001f){
  // Three swept, twisting sheets occupy different world planes. Their torn
  // material flows into the head while the tail is left on the travelled path.
  // Changing the camera cannot turn the complete effect into a flat card.
  const float actualSpan=(t-tail)*length;
  for(int sheet=0;sheet<3;++sheet){
   const float a=sheet*tau/3;
   const auto path=[&](float u){
    const float behind=(1-u)*actualSpan,turn=a+behind*2.7f-age*4.2f;
    const float spread=std::sin(u*pi)*(.12f+.11f*std::sin(behind*3-age*6+a));
    auto p=point(tail+(t-tail)*u);
    p=Add(p,Add(Mul(frame.right,std::cos(turn)*spread*power),Mul(frame.up,std::sin(turn)*spread*power)));
    p.y+=behind*behind*.045f+after*.48f;return p;
   };
   const auto normal=Add(Mul(frame.right,std::cos(a)),Mul(frame.up,std::sin(a)));
   FlameStream(r,path,normal,(meteor?.46f:.32f)*power,actualSpan,age+sheet*.23f,{1,1,1,fade*.64f});
  }
  Ribbon(r,[&](float u){return point(tail+(t-tail)*u);},frame.up,.018f*power,{1,.76f,.35f,fade*.46f});
 } else if(t>tail+.00001f){
  const int strands=holy?(e.ability==5?2:1):arcane?3:thermal?2:2;
  for(int strand=0;strand<strands;++strand){
   const auto path=[&](float s){const float u=tail+(t-tail)*s,behind=(1-s)*span;
    const float angle=behind*(arcane?4.f:2.8f)-age*(arcane?7.f:5.f)+strand*tau/strands;
    const float spread=(arcane?.19f:holy?.16f:.11f)*std::sin(s*pi)*(1+behind*.16f);
    auto p=point(u);p=Add(p,Add(Mul(frame.right,std::cos(angle)*spread),Mul(frame.up,std::sin(angle)*spread)));
    p.y+=!holy&&!frost&&!arcane?behind*behind*.035f+after*.38f:after*.12f;return p;};
   auto color=hue;color.w=fade*(holy?.43f:.42f);
   Ribbon(r,path,frame.up,(holy?.075f:arcane?.065f:.10f)*power,color);
   color=holy?XMFLOAT4{1,.96f,.76f,fade*.82f}:frost?XMFLOAT4{.65f,.91f,1,fade*.72f}:
     arcane?XMFLOAT4{.76f,.67f,1,fade*.74f}:XMFLOAT4{1,.67f,.18f,fade*.64f};
   Ribbon(r,path,frame.up,.014f*power,color);
  }
 }
 if(fire){
  // Fixed birth times on the real path: smoke/embers detach, drift and cool.
  // Their identities survive frame-rate changes and the head reaching contact.
  constexpr float interval=1.f/60,life=.15f;
  const int newest=int(std::min(age,e.flight)/interval);
  for(int n=0;n<9;++n){const int birthIndex=newest-n;if(birthIndex<0)continue;
   const float birth=birthIndex*interval,particleAge=age-birth;if(particleAge<0 || particleAge>=life)continue;
   const float q=particleAge/life,random=Random(seed+unsigned(birthIndex)*19u);
   const float angle=random*tau,drift=particleAge*(.45f+random);
   const float birthProgress=birth/e.flight;
   const Frame emission(point(birthProgress),Sub(point(birthProgress+.002f),point(birthProgress-.002f)));
   auto p=emission.At(std::cos(angle)*drift,std::sin(angle)*drift,0);
   p.y+=particleAge*.85f;const float alpha=combat::Smooth(particleAge/.015f)*(1-combat::Smooth(q));
   if(birthIndex%3==0)r.DrawSpellVolume(Medium::Fire,p,emission.axis,{(.10f+q*.12f)*power,(.12f+q*.17f)*power,.25f*power},
      age+birth,{1,.76f,.48f,alpha*.42f},seed+unsigned(birthIndex));
   r.DrawEffectSprite(Art::Smoke,p,(.08f+q*.25f)*power,(.08f+q*.21f)*power,random*4+particleAge,
     {.25f,.21f,.18f,alpha*.20f},true);
   const auto ember=Add(p,Mul(emission.right,(random-.5f)*q*.44f));
   r.DrawEffectStreak(Sub(ember,Mul(emission.axis,.035f)),ember,.012f*power,{1,.53f,.09f,alpha*.76f});
  }
 }
 if(frost || thermal){
  // A pointed hexagonal projectile is a small closed volume, not a frozen
  // billboard. Its faces and satellites remain legible from all camera angles.
  for(int shard=0;shard<(e.ability==12?3:1);++shard){
   const float offset=(shard-(e.ability==12?1:0))*.16f*std::sin(t*pi);
   const Frame crystal(frame.At(offset,0,-.06f),frame.axis);
   for(int face=0;face<6;++face){const float a=face*tau/6+age*1.7f,b=(face+1)*tau/6+age*1.7f;
    const auto p=crystal.At(std::cos(a)*.13f*power,std::sin(a)*.13f*power,-.31f*power);
    const auto q=crystal.At(std::cos(b)*.13f*power,std::sin(b)*.13f*power,-.31f*power);
    r.DrawEffectSurface(Art::Ice,crystal.At(0,0,-.75f*power),p,crystal.At(0,0,0),q,{.35f,.70f,1,front*.62f});
    r.DrawEffectStreak(p,crystal.At(0,0,0),.008f,{.72f,.94f,1,front*.78f});
   }
  }
 }
 if(holy){
  // Swept open vanes give the prayer a lance silhouette without a rotating
  // ornamental disc facing the camera. Its breathing mist is the volume above.
  const int wings=e.ability==6?6:4;
  for(int n=0;n<wings;++n){const float a=n*tau/wings;
   const auto path=[&](float u){const float width=std::sin(u*pi)*(.32f+(e.ability==6?.16f:0))*power;
    return frame.At(std::cos(a)*width,std::sin(a)*width,-(1-u)*1.35f*power);};
   Ribbon(r,path,frame.right,.035f*power,{1,.91f,.61f,front*.52f},8);
  }
 }
}
}
