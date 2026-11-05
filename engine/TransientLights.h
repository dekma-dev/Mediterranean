#pragma once
#include <DirectXMath.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <tuple>

namespace visual {
struct TransientLight {
 DirectX::XMFLOAT4 positionRadius{},colorPower{};
};
struct TransientLightBlock {
 DirectX::XMFLOAT4 count{};
 std::array<TransientLight,4> lights{};
};
static_assert(sizeof(TransientLightBlock)==144);
// A frame-local top four. Stable geometric tie breaking makes dense effects
// independent of event submission order; no per-light textures or shadow maps.
class TransientLights {
 struct Entry {TransientLight light;float score=0;};
 std::array<Entry,4> entries_{};unsigned count_=0;
public:
 void Reset(){count_=0;}
 unsigned Count()const{return count_;}
 void Add(DirectX::XMFLOAT3 p,float radius,DirectX::XMFLOAT3 color,float power,DirectX::XMFLOAT3 eye){
  for(float x:{p.x,p.y,p.z,radius,color.x,color.y,color.z,power,eye.x,eye.y,eye.z})if(!std::isfinite(x))return;
  if(radius<=0 || power<=.001f)return;
  radius=std::min(radius,6.f);power=std::min(power,8.f);
  color={std::clamp(color.x,0.f,1.f),std::clamp(color.y,0.f,1.f),std::clamp(color.z,0.f,1.f)};
  const float brightness=std::max({color.x,color.y,color.z});if(brightness==0)return;
  const float distance=(p.x-eye.x)*(p.x-eye.x)+(p.y-eye.y)*(p.y-eye.y)+(p.z-eye.z)*(p.z-eye.z);
  Entry item{{{p.x,p.y,p.z,radius},{color.x,color.y,color.z,power}},power*brightness*radius/(1+distance*.08f)};
  const auto better=[](const Entry& a,const Entry& b){
   const auto& x=a.light;const auto& y=b.light;
   return std::tie(a.score,x.positionRadius.x,x.positionRadius.y,x.positionRadius.z,x.positionRadius.w,x.colorPower.x,x.colorPower.y,x.colorPower.z,x.colorPower.w)>
          std::tie(b.score,y.positionRadius.x,y.positionRadius.y,y.positionRadius.z,y.positionRadius.w,y.colorPower.x,y.colorPower.y,y.colorPower.z,y.colorPower.w);
  };
  if(count_<entries_.size())entries_[count_++]=item;
  else if(better(item,entries_[count_-1]))entries_[count_-1]=item;
  else return;
  std::sort(entries_.begin(),entries_.begin()+count_,better);
 }
 TransientLightBlock Block()const{TransientLightBlock b;b.count.x=float(count_);for(unsigned i=0;i<count_;++i)b.lights[i]=entries_[i].light;return b;}
};
}
