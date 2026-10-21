#pragma once
#include <algorithm>

namespace ui::skin {
// Identical nine-slice boundaries for D3D and GDI+. Keep the relief in fixed
// corners; stretch only the straight rails and quiet leather centre.
template<class Draw> void Panel(float x,float y,float width,float height,float corner,Draw draw,bool centre=true) {
  if (width<=0 || height<=0) return;
  corner=std::max(0.f,std::min({corner,width*.5f,height*.5f}));
  const float xx[]={x,x+corner,x+width-corner,x+width};
  const float yy[]={y,y+corner,y+height-corner,y+height};
  constexpr float uv[]={0,.25f,.75f,1};
  for(int row=0;row<3;++row) for(int col=0;col<3;++col) {
    if(!centre && row==1 && col==1) continue;
    if(xx[col+1]<=xx[col] || yy[row+1]<=yy[row]) continue;
    draw(xx[col],yy[row],xx[col+1]-xx[col],yy[row+1]-yy[row],uv[col],uv[row],uv[col+1],uv[row+1]);
  }
}
}
