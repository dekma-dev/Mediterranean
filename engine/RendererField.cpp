#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
using namespace DirectX;
using visual::Vertex;
void Renderer::DrawSpellField(SpellField kind,XMFLOAT2 center,float radius,float age,
 XMFLOAT4 tint,const std::array<float,25>& heights,unsigned seed){
 if((portrait_ && !portraitEffects_) || int(kind)<0 || int(kind)>3 ||
   !std::isfinite(center.x+center.y+radius+age+tint.x+tint.y+tint.z+tint.w) ||
   radius<=0 || radius>6 || age<0 || tint.w<=0 || spellFieldCount_>=spellFields_.size() ||
   EffectVertices()+96>maxEffectVertices)return;
 for(float y:heights)if(!std::isfinite(y))return;
 spellFields_[spellFieldCount_]={{age,float(seed%65536),radius,float(kind)},tint};
 const float material=64.f+spellFieldCount_;
 const auto vertex=[&](int x,int z){const float u=x*.5f-1,v=z*.5f-1;
  return Vertex{{center.x+u*radius,heights[z*5+x]+.045f,center.y+v*radius},{u,v,material},{1,1,1,1}};};
 for(int z=0;z<4;++z)for(int x=0;x<4;++x){const auto a=vertex(x,z),b=vertex(x,z+1),c=vertex(x+1,z+1),d=vertex(x+1,z);
  effects_.insert(effects_.end(),{a,b,c,a,c,d});}
 ++spellFieldCount_;
}
