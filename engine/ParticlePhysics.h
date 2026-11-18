#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Cosmetic dynamics only: never feeds damage, navigation or authoritative actors.
namespace particles {
struct Vec3 {float x=0,y=0,z=0;};
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool Finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline Vec3 Normal(Vec3 p){const float d=std::sqrt(Dot(p,p));return d>1e-6f?p*(1/d):Vec3{0,1,0};}
struct Contact {
 float fraction=1;
 Vec3 point{},normal{0,1,0};
 float restitution=.5f,friction=.45f;
 bool ground=false;
};
struct Box {Vec3 low,high;};
// Swept sphere against an expanded box. An embedded spawn is ejected through
// its nearest face instead of tunnelling out through the opposite wall.
inline bool SweepBox(Vec3 from,Vec3 to,float radius,const Box& box,Contact& hit){
 const Vec3 low=box.low-Vec3{radius,radius,radius},high=box.high+Vec3{radius,radius,radius};
 const float f[]={from.x,from.y,from.z},d[]={to.x-from.x,to.y-from.y,to.z-from.z};
 const float lo[]={low.x,low.y,low.z},hi[]={high.x,high.y,high.z};
 float enter=0,leave=hit.fraction;Vec3 n{};
 bool inside=true;float nearest=1e30f;int face=0;float sign=1;
 for(int axis=0;axis<3;++axis){
  inside=inside && f[axis]>=lo[axis] && f[axis]<=hi[axis];
  if(f[axis]-lo[axis]<nearest){nearest=f[axis]-lo[axis];face=axis;sign=-1;}
  if(hi[axis]-f[axis]<nearest){nearest=hi[axis]-f[axis];face=axis;sign=1;}
  if(std::abs(d[axis])<1e-8f){if(f[axis]<lo[axis]||f[axis]>hi[axis])return false;continue;}
  float a=(lo[axis]-f[axis])/d[axis],b=(hi[axis]-f[axis])/d[axis];
  const float side=d[axis]>0?-1.f:1.f;if(a>b)std::swap(a,b);
  if(a>=enter){enter=a;n={};if(axis==0)n.x=side;else if(axis==1)n.y=side;else n.z=side;}
  leave=std::min(leave,b);if(enter>leave)return false;
 }
 if(inside){
  n={};if(face==0)n.x=sign;else if(face==1)n.y=sign;else n.z=sign;
  hit={0,from+n*(std::max(0.f,nearest)+.001f),n,.45f,.55f,false};return true;
 }
 if(enter<0 || enter>hit.fraction || leave<0)return false;
 hit={enter,from+(to-from)*enter,n,.45f,.55f,false};return true;
}
class CollisionWorld {
public:
 virtual ~CollisionWorld()=default;
 virtual bool Trace(Vec3 from,Vec3 to,float radius,Contact& hit) const=0;
};
struct Particle {
 Vec3 position{},previous{},velocity{},contactPoint{},contactNormal{0,1,0};
 double age=0;
 float lifetime=0,radius=.02f,drag=.6f,gravity=9.81f,bounce=.6f;
 float contactAge=-10;
 std::uint32_t seed=0;
 std::uint16_t ticks=0;
 std::uint8_t style=0,contacts=0;
 bool resting=false,groundContact=false;
 bool Alive()const{return age<lifetime;}
 // One fixed step of interpolation latency; rendering never advances physics.
 Vec3 Position()const{return previous+(position-previous)*std::clamp(float(age*120-ticks),0.f,1.f);}
};
class Pool {
public:
 static constexpr unsigned capacity=96;
 static constexpr double step=1.0/120;
 const std::array<Particle,capacity>& Items()const{return items_;}
 unsigned Count()const{unsigned n=0;for(const auto& p:items_)n+=p.Alive();return n;}
 unsigned CollisionCount()const{return collisions_;}
 void Reset(){items_={};cursor_=collisions_=0;}
 bool Spawn(Particle p,float elapsed,const CollisionWorld& world){
  if(!Finite(p.position)||!Finite(p.velocity)||!std::isfinite(p.lifetime)||p.lifetime<=0)return false;
  p.lifetime=std::min(p.lifetime,3.f);p.radius=Safe(p.radius,.005f,.10f,.02f);
  p.drag=Safe(p.drag,0,12,.6f);p.gravity=Safe(p.gravity,0,30,9.81f);p.bounce=Safe(p.bounce,0,1,.6f);
  const float speed=std::sqrt(Dot(p.velocity,p.velocity));if(!std::isfinite(speed))return false;
  if(speed>20)p.velocity=p.velocity*(20/speed);
  p.previous=p.position;p.age=0;p.ticks=0;p.contacts=0;p.resting=false;p.contactAge=-10;p.groundContact=false;
  unsigned available=capacity;
  for(unsigned i=0;i<capacity;++i){const unsigned slot=(cursor_+i)%capacity;
   if(!items_[slot].Alive()){available=slot;break;}}
  // Drop secondary cosmetics under saturation; don't erase existing tails or
  // run collisions for a fragment that cannot enter the pool.
  if(available==capacity)return false;
  // Invalid/late notifications cannot invent a long catch-up workload.
  AdvanceOne(p,Safe(elapsed,0,.05f,0),world);
  items_[available]=p;cursor_=(available+1)%capacity;return true;
 }
 void Advance(float dt,const CollisionWorld& world){
  if(!std::isfinite(dt)||dt<=0)return;
  for(auto& p:items_)if(p.Alive())AdvanceOne(p,std::min(dt,.05f),world);
 }
private:
 std::array<Particle,capacity> items_{};
 unsigned cursor_=0,collisions_=0;
 static float Safe(float n,float low,float high,float fallback){return std::isfinite(n)?std::clamp(n,low,high):fallback;}
 void AdvanceOne(Particle& p,float dt,const CollisionWorld& world){
  p.age=std::min(double(p.lifetime),p.age+dt);
  const auto target=unsigned(std::floor(p.age/step+1e-6));
  while(p.ticks<target){++p.ticks;p.previous=p.position;
   if(p.resting)continue;
   const float h=float(step),decay=std::exp(-p.drag*h);
   const Vec3 gravity{0,-p.gravity,0};
   // Exact constant gravity + linear drag between contacts.
   const float travel=p.drag>1e-5f?(1-decay)/p.drag:h;
   const Vec3 displacement=p.velocity*travel+gravity*(p.drag>1e-5f?(h-travel)/p.drag:h*h*.5f);
   p.velocity=p.velocity*decay+gravity*travel;
   Vec3 destination=p.position+displacement;
   float remaining=h;
   for(int bounce=0;bounce<2;++bounce){
    Contact hit;
    if(!world.Trace(p.position,destination,p.radius,hit)){p.position=destination;break;}
    ++collisions_;p.contacts=std::uint8_t(std::min(255,int(p.contacts)+1));
    p.position=hit.point+hit.normal*.001f;
    p.contactPoint=hit.point-hit.normal*p.radius;p.contactNormal=hit.normal;p.contactAge=float(p.ticks*step);p.groundContact=hit.ground;
    // Don't interpolate a straight line through a corner after a bounce.
    p.previous=p.position;
    const float incoming=Dot(p.velocity,hit.normal);
    if(incoming<0){const Vec3 tangent=p.velocity-hit.normal*incoming;
     p.velocity=tangent*(1-std::clamp(hit.friction,0.f,1.f))-hit.normal*(incoming*p.bounce*hit.restitution);}
    if(hit.normal.y>.45f && (Dot(p.velocity,p.velocity)<.22f || p.contacts>=5)){
     p.velocity={};p.resting=true;break;
    }
    remaining*=1-std::clamp(hit.fraction,0.f,1.f);destination=p.position+p.velocity*remaining;
    // At the iteration cap retain the last safe contact, never an unchecked endpoint.
   }
  }
 }
};
}
