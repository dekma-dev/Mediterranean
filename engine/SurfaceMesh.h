#pragma once
#include "engine/Geometry.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace visual {
// Welded marching tetrahedra for small, immutable procedural surfaces. The
// caller supplies a signed field and grid bounds entirely outside the surface.
template<class Field> Geometry SurfaceMesh(DirectX::XMFLOAT3 origin,std::array<int,3> size,float spacing,Field field) {
  using namespace DirectX;
  const int nx=size[0],ny=size[1],nz=size[2];
  const auto id=[=](int x,int y,int z) { return (y*nz+z)*nx+x; };
  std::vector<XMFLOAT3> points(nx*ny*nz); std::vector<float> values(points.size());
  for(int y=0;y<ny;++y) for(int z=0;z<nz;++z) for(int x=0;x<nx;++x) {
    const int i=id(x,y,z); points[i]={origin.x+x*spacing,origin.y+y*spacing,origin.z+z*spacing};
    const float distance=field(points[i]);
    values[i]=std::copysign(std::max(std::abs(distance),spacing*.04f),distance);
  }
  Geometry mesh; std::unordered_map<std::uint64_t,unsigned> edges;
  const auto edge=[&](int a,int b) {
    if(a>b) std::swap(a,b);
    const auto key=(std::uint64_t(a)<<32)|unsigned(b);
    const auto found=edges.find(key); if(found!=edges.end()) return found->second;
    const float t=values[a]/(values[a]-values[b]); XMFLOAT3 p,n;
    XMStoreFloat3(&p,XMLoadFloat3(&points[a])+(XMLoadFloat3(&points[b])-XMLoadFloat3(&points[a]))*t);
    const float e=spacing*.08f;
    n={field({p.x+e,p.y,p.z})-field({p.x-e,p.y,p.z}),field({p.x,p.y+e,p.z})-field({p.x,p.y-e,p.z}),
      field({p.x,p.y,p.z+e})-field({p.x,p.y,p.z-e})};
    XMStoreFloat3(&n,XMVector3Normalize(XMLoadFloat3(&n)));
    const auto index=unsigned(mesh.vertices.size()); mesh.vertices.push_back({p,n}); edges.emplace(key,index); return index;
  };
  const auto triangle=[&](unsigned a,unsigned b,unsigned c) {
    const auto& va=mesh.vertices[a]; const auto& vb=mesh.vertices[b]; const auto& vc=mesh.vertices[c];
    const auto cross=XMVector3Cross(XMLoadFloat3(&vb.position)-XMLoadFloat3(&va.position),XMLoadFloat3(&vc.position)-XMLoadFloat3(&va.position));
    if(XMVectorGetX(XMVector3Dot(cross,XMLoadFloat3(&va.normal)+XMLoadFloat3(&vb.normal)+XMLoadFloat3(&vc.normal)))<0) std::swap(b,c);
    mesh.indices.insert(mesh.indices.end(),{a,b,c});
  };
  constexpr int tetrahedra[6][4]={{0,1,3,7},{0,3,2,7},{0,2,6,7},{0,6,4,7},{0,4,5,7},{0,5,1,7}};
  for(int y=0;y<ny-1;++y) for(int z=0;z<nz-1;++z) for(int x=0;x<nx-1;++x) {
    const int cube[]={id(x,y,z),id(x+1,y,z),id(x,y+1,z),id(x+1,y+1,z),id(x,y,z+1),id(x+1,y,z+1),id(x,y+1,z+1),id(x+1,y+1,z+1)};
    for(const auto& tet : tetrahedra) {
      int inside[4],outside[4],ni=0,no=0;
      for(int corner : tet) { const int i=cube[corner]; if(values[i]<0) inside[ni++]=i; else outside[no++]=i; }
      if(ni==0 || ni==4) continue;
      if(ni==1) triangle(edge(inside[0],outside[0]),edge(inside[0],outside[1]),edge(inside[0],outside[2]));
      else if(ni==3) triangle(edge(outside[0],inside[0]),edge(outside[0],inside[1]),edge(outside[0],inside[2]));
      else {
        const auto a=edge(inside[0],outside[0]),b=edge(inside[0],outside[1]),c=edge(inside[1],outside[0]),d=edge(inside[1],outside[1]);
        triangle(a,b,c); triangle(b,d,c);
      }
    }
  }
  return mesh;
}
}
