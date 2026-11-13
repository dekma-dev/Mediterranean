#pragma once
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace spellvfx {
// Open, tapered bands: useful for cutting air and vaulted holy light. No
// camera-facing discs; adjacent sections share both vertices and coordinates.
template<class Path> void FilamentStream(Renderer& r,const Path& path,DirectX::XMFLOAT3 normal,
 float width,float distance,float phase,DirectX::XMFLOAT4 tint,int segments=12) {
 using namespace DirectX;
 const auto section=[&](float u,XMFLOAT3& a,XMFLOAT3& b) {
  const auto p=path(u),lo=path(std::max(0.f,u-.003f)),hi=path(std::min(1.f,u+.003f));
  auto side=XMVector3Cross(XMLoadFloat3(&hi)-XMLoadFloat3(&lo),XMLoadFloat3(&normal));
  if(XMVectorGetX(XMVector3LengthSq(side))<1e-8f)side=XMVectorSet(1,0,0,0);
  side=XMVector3Normalize(side)*(width*(.06f+.94f*std::sin(u*3.14159265f)));
  XMStoreFloat3(&a,XMLoadFloat3(&p)-side);XMStoreFloat3(&b,XMLoadFloat3(&p)+side);
 };
 XMFLOAT3 a,b;section(0,a,b);
 for(int n=0;n<segments;++n){const float u=float(n)/segments,v=float(n+1)/segments;XMFLOAT3 c,d;section(v,d,c);
  r.DrawFilamentRibbon(a,b,c,d,u*distance,v*distance,phase,tint,
    std::sin(u*3.14159265f),std::sin(v*3.14159265f));a=d;b=c;}
}
// A curved material sheet in world space. The same cross-section is shared by
// neighbouring quads; a continuous coordinate carries heat across every join.
template<class Path> void FlameStream(Renderer& r,const Path& path,DirectX::XMFLOAT3 normal,
 float width,float distance,float phase,DirectX::XMFLOAT4 tint,int segments=14) {
 using namespace DirectX;
 const auto section=[&](float u,XMFLOAT3& a,XMFLOAT3& b) {
  const auto p=path(u),lo=path(std::max(0.f,u-.004f)),hi=path(std::min(1.f,u+.004f));
  auto side=XMVector3Cross(XMLoadFloat3(&hi)-XMLoadFloat3(&lo),XMLoadFloat3(&normal));
  if(XMVectorGetX(XMVector3LengthSq(side))<1e-8f)side=XMVectorSet(1,0,0,0);
  const float envelope=.035f+std::sin(u*3.14159265f)*(.35f+.65f*u);
  side=XMVector3Normalize(side)*(width*envelope);
  XMStoreFloat3(&a,XMLoadFloat3(&p)-side);XMStoreFloat3(&b,XMLoadFloat3(&p)+side);
 };
 XMFLOAT3 a,b;section(0,a,b);
 for(int n=0;n<segments;++n){
  const float u=float(n)/segments,v=float(n+1)/segments;XMFLOAT3 c,d;section(v,d,c);
  r.DrawFlameRibbon(a,b,c,d,u*distance,v*distance,phase,tint,
    std::sin(u*3.14159265f),std::sin(v*3.14159265f));a=d;b=c;
 }
}
}
