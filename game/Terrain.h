#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace rpg::terrain {
inline constexpr float halfWidth=120,halfDepth=128,cell=2;
inline constexpr int columns=123,rows=131;
inline constexpr float left=-122,front=-130;
inline float Smooth(float x) {x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);}
inline float NaturalHeight(float x,float z) {
  // Keep the established settlement and old saved positions on their original
  // floor. Beyond it, broad traversable hills connect three remote districts.
  const float outside=std::max(std::abs(x)-30,std::abs(z)-32);
  const auto hill=[&](float cx,float cz,float sx,float sz,float h) {
    const float a=(x-cx)/sx,b=(z-cz)/sz;return h*std::exp(-a*a-b*b);
  };
  float h=hill(-48,24,25,35,10)+hill(43,41,26,29,8)+hill(-6,65,34,26,13)+hill(22,-57,38,23,6);
  h+=.55f*std::sin(x*.12f)*std::sin(z*.09f);
  // The outer province joins the established heightfield gradually. Existing
  // hills and saved footing inside the former boundary keep the same heights.
  const float outer=Smooth(std::max(std::abs(x)-72,std::abs(z)-80)/18.f);
  h+=outer*(hill(-81,-86,33,32,17)+hill(86,-91,31,34,21)+hill(-88,91,36,32,14)+hill(84,93,33,29,18)
    +1.2f*std::sin(x*.075f)*std::cos(z*.06f));
  return std::max(0.f,h)*Smooth(outside/14.f);
}
inline float Sculpt(float x,float z) {
  float height=NaturalHeight(x,z);
  // Terraced foundations under the four new buildings. The apron eases back
  // into the surrounding slope; render and collision sample this same grid.
  constexpr float pads[][2]={{-50,18},{48,30},{-12,53},{25,-60},{-39,-91},{-79,-66},{79,-87},{-86,-91}};
  for(const auto& p:pads) {
    const float distance=std::max(std::abs(x-p[0]),std::abs(z-p[1]));
    const float weight=1-Smooth((distance-4)/5);
    height+=(NaturalHeight(p[0],p[1])-height)*weight;
  }
  return height;
}
inline const auto& Heights() {
  static const auto values=[] {
    std::array<float,columns*rows> result{};
    for(int z=0;z<rows;++z) for(int x=0;x<columns;++x)
      result[z*columns+x]=Sculpt(left+x*cell,front+z*cell);
    return result;
  }();return values;
}
inline float Height(float x,float z) {
  if(!std::isfinite(x)||!std::isfinite(z)) return 0;
  const float gx=std::clamp((x-left)/cell,0.f,float(columns-1)),gz=std::clamp((z-front)/cell,0.f,float(rows-1));
  const int ix=std::min(int(gx),columns-2),iz=std::min(int(gz),rows-2);
  const float u=gx-ix,v=gz-iz;const auto& h=Heights();const int i=iz*columns+ix;
  // Exactly the same diagonal and planar interpolation as the rendered mesh.
  return u+v<=1?h[i]+u*(h[i+1]-h[i])+v*(h[i+columns]-h[i]):
    h[i+columns+1]+(1-u)*(h[i+columns]-h[i+columns+1])+(1-v)*(h[i+1]-h[i+columns+1]);
}
inline float Slope(float x,float z) {
  return std::hypot((Height(x+.2f,z)-Height(x-.2f,z))/.4f,(Height(x,z+.2f)-Height(x,z-.2f))/.4f);
}
}
