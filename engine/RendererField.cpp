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

void Renderer::DrawAttackField(XMFLOAT2 center,const AttackField& f,const AttackSurface& surface){
 if(portrait_ || !std::isfinite(center.x+center.y+f.reach+f.halfAngle+f.facing+f.inner+f.outer+f.progress) ||
   f.reach<=0 || f.reach>9 || f.halfAngle<=0 || f.halfAngle>3.141593f || f.inner<0 || f.outer<0 ||
   (f.outer>0 && f.inner>=f.outer) || f.outer>9 || f.progress<0 || f.progress>1 ||
   spellFieldCount_>=spellFields_.size() || !std::isfinite(surface.origin.x+surface.origin.y+surface.step) ||
   surface.step<=0 || surface.step>4 || surface.columns<2 || surface.columns>11 || surface.rows<2 || surface.rows>11)return;
 for(unsigned i=0;i<surface.columns*surface.rows;++i)if(!std::isfinite(surface.heights[i]))return;
 const float sx=std::sin(f.facing),cx=std::cos(f.facing);
 const auto distance=[&](float x,float z){const float radius=std::hypot(x,z),angle=std::abs(std::atan2(cx*x-sx*z,sx*x+cx*z));
  const float cone=f.halfAngle>3.1415f?radius-f.reach:
    std::max(radius-f.reach,radius*std::sin(std::clamp(angle-f.halfAngle,-1.5707963f,1.5707963f)));
  return f.outer>0?std::min(cone,std::max(f.inner-radius,radius-f.outer)):cone;};
 const auto vertex=[&](unsigned x,unsigned z){const float wx=surface.origin.x+x*surface.step,wz=surface.origin.y+z*surface.step;
  return Vertex{{wx,surface.heights[z*surface.columns+x]+.055f,wz},
    {(wx-center.x)/f.Extent(),(wz-center.y)/f.Extent(),64.f+spellFieldCount_},{1,1,1,1}};};
 std::array<Vertex,600> patch{};unsigned count=0;
 for(unsigned z=0;z+1<surface.rows;++z)for(unsigned x=0;x+1<surface.columns;++x){
  // Signed distances are conservative at cone corners. The half-diagonal
  // bound retains every intersecting cell, including narrow crossing lances.
  if(distance(surface.origin.x+(x+.5f)*surface.step-center.x,surface.origin.y+(z+.5f)*surface.step-center.y)>
    surface.step*.707107f+.16f)continue;
  const auto a=vertex(x,z),b=vertex(x,z+1),c=vertex(x+1,z+1),d=vertex(x+1,z);
  for(const auto& v:{a,b,d,b,c,d})patch[count++]=v;
 }
 if(count==0 || groundMarks_.size()+count>3600 || EffectVertices()+count>maxEffectVertices)return;
 const auto tint=f.interruptible?XMFLOAT4{1,.38f,.075f,1}:XMFLOAT4{.70f,.43f,1,1};
 spellFields_[spellFieldCount_]={{f.progress,f.reach,f.Extent(),4},tint,
   {sx,cx,f.inner,f.outer},{f.halfAngle,f.threatens?1.f:0.f,0,0}};
 groundMarks_.insert(groundMarks_.end(),patch.begin(),patch.begin()+count);
 ++spellFieldCount_;
}
