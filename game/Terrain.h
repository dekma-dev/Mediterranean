#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace rpg::terrain {
inline constexpr float halfWidth=120,halfDepth=128,cell=2;
inline constexpr int columns=123,rows=131;
inline constexpr float left=-122,front=-130;
inline constexpr float foundationPads[][2]={{-50,18},{48,30},{-12,53},{25,-60},{-39,-91},{-79,-66},{79,-87},{-86,-91},
  {-47,-77},{-45,-115},{-84,-65},{-59,-104},{-96,-80},{-103,-93},{81,-106},{93,-94},{107,-105},{-32,-35},{-44,-40},{34,-36},{22,-41},{-73,20},{-85,15},
  {78,35},{66,30},{-75,70},{-87,65},{78,81},{66,76}};
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
  for(const auto& p:foundationPads) {
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
    // Close terraced sites used to produce near-vertical banks between pads.
    // Lower only excessive rises, once at construction, using an eight-neighbour
    // grade envelope. Village zeroes stay zero; render/feet/camera share it.
    for(int pass=0;pass<4;++pass) {
      for(int step=0;step<columns*rows;++step) {
        const int i=pass%2?columns*rows-1-step:step,x=i%columns,z=i/columns;
        for(int dz=-1;dz<=1;++dz) for(int dx=-1;dx<=1;++dx) {
          const int nx=x+dx,nz=z+dz;
          if((dx==0 && dz==0) || nx<0 || nx>=columns || nz<0 || nz>=rows)continue;
          result[i]=std::min(result[i],result[nz*columns+nx]+.95f*(dx && dz?1.41421356f:1.f));
        }
      }
    }
    // Preserve level foundations after limiting grades. Nearby terraces share
    // compatible levels; intersecting slope envelopes avoid cliffs between them.
    constexpr int count=int(sizeof(foundationPads)/sizeof(foundationPads[0]));
    std::array<float,count> floors{};
    for(int p=0;p<count;++p) {
      const int x=int(std::round((foundationPads[p][0]-left)/cell)),z=int(std::round((foundationPads[p][1]-front)/cell));
      floors[p]=result[z*columns+x];
    }
    for(int pass=0;pass<count;++pass) for(int a=0;a<count;++a) for(int b=0;b<count;++b) {
      const float gap=std::max(0.f,std::max(std::abs(foundationPads[a][0]-foundationPads[b][0]),
        std::abs(foundationPads[a][1]-foundationPads[b][1]))-6.f);
      floors[a]=std::min(floors[a],floors[b]+gap*.475f);
    }
    for(int z=0;z<rows;++z)for(int x=0;x<columns;++x) {
      float low=0,high=1000;
      for(int p=0;p<count;++p) {
        const float gap=std::max(0.f,std::max(std::abs(left+x*cell-foundationPads[p][0]),
          std::abs(front+z*cell-foundationPads[p][1]))-3.f);
        low=std::max(low,floors[p]-gap*.475f);high=std::min(high,floors[p]+gap*.475f);
      }
      result[z*columns+x]=std::max(low,std::min(result[z*columns+x],high));
    }
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
