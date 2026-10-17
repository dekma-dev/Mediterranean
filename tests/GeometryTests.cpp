#include "engine/Geometry.h"
#include "game/WorldMap.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void Check(bool condition,const char* message) {
  if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
bool Finite(DirectX::XMFLOAT3 p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
float Cross2(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 p) {
  return (b.x-a.x)*(p.z-a.z)-(b.z-a.z)*(p.x-a.x);
}
bool Covers(const visual::Geometry& g,DirectX::XMFLOAT3 p) {
  for (std::size_t i=0;i<g.indices.size();i+=3) {
    const auto a=g.vertices[g.indices[i]].position,b=g.vertices[g.indices[i+1]].position,c=g.vertices[g.indices[i+2]].position;
    const float x=Cross2(a,b,p),y=Cross2(b,c,p),z=Cross2(c,a,p);
    if ((x>=-.001f && y>=-.001f && z>=-.001f) || (x<=.001f && y<=.001f && z<=.001f)) return true;
  }
  return false;
}
}
int main() {
  using namespace DirectX;
  std::size_t bytes=0,triangles=0;
  for (unsigned mesh=0;mesh<static_cast<unsigned>(visual::Mesh::Count);++mesh) {
    const auto g=visual::BuildMesh(static_cast<visual::Mesh>(mesh));
    Check(!g.vertices.empty() && !g.indices.empty() && g.indices.size()%3==0,"Missing or incomplete mesh.");
    bytes+=g.vertices.size()*sizeof(visual::Vertex)+g.indices.size()*sizeof(std::uint32_t);
    triangles+=g.indices.size()/3;
    for (const auto& v : g.vertices) {
      Check(Finite(v.position) && Finite(v.normal),"Non-finite mesh data would poison bounds or lighting.");
      const float length=XMVectorGetX(XMVector3Length(XMLoadFloat3(&v.normal)));
      Check(std::abs(length-1)<.001f,"Lighting requires normalized vertex normals.");
    }
    for (auto index : g.indices) Check(index<g.vertices.size(),"GPU index exceeds the vertex buffer.");
    for (std::size_t i=0;i<g.indices.size();i+=3) {
      const auto& a=g.vertices[g.indices[i]]; const auto& b=g.vertices[g.indices[i+1]]; const auto& c=g.vertices[g.indices[i+2]];
      const auto normal=XMVector3Cross(XMLoadFloat3(&b.position)-XMLoadFloat3(&a.position),XMLoadFloat3(&c.position)-XMLoadFloat3(&a.position));
      Check(XMVectorGetX(XMVector3LengthSq(normal))>1e-12f,"Degenerate procedural triangle.");
      const auto average=XMLoadFloat3(&a.normal)+XMLoadFloat3(&b.normal)+XMLoadFloat3(&c.normal);
      if (XMVectorGetX(XMVector3Dot(normal,average))<=0) {
        std::cerr << "Mesh " << mesh << ", triangle " << i/3 << ": ";
        Check(false,"Vertex normals oppose the triangle winding.");
      }
    }
  }
  Check(bytes<=256*1024 && triangles<=10000,"Reusable mesh library exceeded its startup geometry budget.");
  const auto terrain=visual::BuildMesh(visual::Mesh::Terrain);
  for (int x=-int(rpg::WorldMap::halfWidth);x<=int(rpg::WorldMap::halfWidth);++x)
    for (int z=-int(rpg::WorldMap::halfDepth);z<=int(rpg::WorldMap::halfDepth);++z)
    Check(Covers(terrain,{float(x),0,float(z)}),"Visible terrain leaves a hole inside the playable collision rectangle.");
  for (const auto& v : terrain.vertices) Check(v.position.y==0 && v.normal.y>.999f,"Terrain must match the flat gameplay floor.");
  const auto cliff=visual::BuildMesh(visual::Mesh::Cliff);
  for (std::size_t i=1;i<terrain.vertices.size();++i) {
    const auto a=terrain.vertices[i].position,b=cliff.vertices[i-1].position;
    Check(a.x==b.x && a.y==b.y && a.z==b.z,"Island edge and cliff must join without a crack.");
  }
  XMFLOAT4X4 camera;
  XMStoreFloat4x4(&camera,XMMatrixPerspectiveFovLH(XM_PIDIV2,1,1,20));
  Check(visual::VisibleSphere({0,0,5},.5f,camera),"Visible object was culled.");
  Check(visual::VisibleSphere({5.3f,0,5},.5f,camera),"Object intersecting the frustum edge was culled.");
  Check(!visual::VisibleSphere({0,0,-5},.5f,camera),"Object behind the camera was retained.");
  Check(!visual::VisibleSphere({0,0,30},.5f,camera),"Object beyond the far plane was retained.");
  Check(!visual::VisibleSphere({20,0,5},.5f,camera),"Object outside the side plane was retained.");
  std::size_t textureBytes=visual::shadowSize*visual::shadowSize*4;
  for (unsigned layer=0;layer<static_cast<unsigned>(visual::Material::Count);++layer) {
    const auto pixels=visual::BuildMaterialLayer(static_cast<visual::Material>(layer),visual::materialSize);
    Check(pixels.size()==visual::materialSize*visual::materialSize*4,"Material upload dimensions are invalid.");
    for (std::size_t i=3;i<pixels.size();i+=4) Check(pixels[i]==255,"Opaque materials contain unintended transparency.");
    for (unsigned size=visual::materialSize;size;size/=2) textureBytes+=size*size*4;
  }
  Check(textureBytes<6*1024*1024,"Material array and shadow map exceeded the 6 MiB budget.");
  std::cout << "PASS: valid mesh topology, floor coverage, joined island boundary and resource budgets.\n";
}
