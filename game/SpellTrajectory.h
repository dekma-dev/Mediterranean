#pragma once
#include "game/SpellVfx.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace spellvfx {
// Cosmetic flight history. Damage, release and arrival clocks remain owned by
// Simulation. Fixed-time samples are immutable: steering cannot drag old fire,
// smoke or arrows sideways. One live endpoint joins the most recent sample.
class FlightPath {
public:
 static constexpr unsigned capacity=64;
 void Reset(const rpg::CombatVisualEvent& e,DirectX::XMFLOAT3 origin){
  *this={};
  if(e.kind!=rpg::CombatVisualKind::SpellCast || !e.target || !Finite(origin) ||
     !std::isfinite(e.flight) || e.flight<=0 || !std::isfinite(e.destination.x) ||
     !std::isfinite(e.destination.z) || !std::isfinite(e.destinationHeight) || !std::isfinite(e.facing))return;
  duration_=e.flight;interval_=std::max(1.f/120,duration_/float(capacity-2));
  target_=ProjectilePosition(e,origin,1);head_=samples_[0]=ProjectilePosition(e,origin,0);count_=1;
 }
 bool Active()const{return count_!=0;}
 float Age()const{return age_;}
 unsigned Samples()const{return count_;}
 DirectX::XMFLOAT3 Target()const{return target_;}
 void Advance(const rpg::CombatVisualEvent& e,DirectX::XMFLOAT3 origin,float age,DirectX::XMFLOAT3 target){
  if(!Active() || age_>=duration_ || !std::isfinite(age) || !Finite(target))return;
  const float next=std::clamp(age,age_,duration_);if(next<=age_)return;
  const auto end=ProjectilePosition(e,origin,1);
  const auto evaluate=[&](float time){
   const float u=std::clamp(time/duration_,0.f,1.f),blend=u*u*(3-2*u);
   const auto destination=Lerp(target_,target,std::clamp((time-age_)/(next-age_),0.f,1.f));
   auto p=ProjectilePosition(e,origin,u);
   p.x+=(destination.x-end.x)*blend;p.y+=(destination.y-end.y)*blend;p.z+=(destination.z-end.z)*blend;
   return p;
  };
  while(count_<capacity && float(count_)*interval_<=next){
   samples_[count_]=evaluate(float(count_)*interval_);++count_;
  }
  head_=evaluate(next);target_=target;age_=next;
 }
 DirectX::XMFLOAT3 Point(float progress)const{
  if(!Active())return {};
  const float time=std::isfinite(progress)?std::clamp(progress*duration_,0.f,age_):0;
  const unsigned index=std::min(unsigned(time/interval_),count_-1);
  const float start=float(index)*interval_;
  if(index+1<count_)return Lerp(samples_[index],samples_[index+1],std::clamp((time-start)/interval_,0.f,1.f));
  return age_>start?Lerp(samples_[index],head_,std::clamp((time-start)/(age_-start),0.f,1.f)):head_;
 }
private:
 static bool Finite(DirectX::XMFLOAT3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
 static DirectX::XMFLOAT3 Lerp(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,float u){return {a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u,a.z+(b.z-a.z)*u};}
 std::array<DirectX::XMFLOAT3,capacity> samples_{};
 DirectX::XMFLOAT3 target_{},head_{};
 float age_=0,duration_=0,interval_=1.f/120;unsigned count_=0;
};
}
