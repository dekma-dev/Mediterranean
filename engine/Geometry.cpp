#include "engine/Geometry.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace visual {
using namespace DirectX;
bool VisibleSphere(const XMFLOAT3& center,float radius,const XMFLOAT4X4& m) {
  const XMFLOAT4 planes[]={
    {m._14+m._11,m._24+m._21,m._34+m._31,m._44+m._41},
    {m._14-m._11,m._24-m._21,m._34-m._31,m._44-m._41},
    {m._14+m._12,m._24+m._22,m._34+m._32,m._44+m._42},
    {m._14-m._12,m._24-m._22,m._34-m._32,m._44-m._42},
    {m._13,m._23,m._33,m._43},{m._14-m._13,m._24-m._23,m._34-m._33,m._44-m._43}};
  for (const auto& p : planes)
    if (center.x*p.x+center.y*p.y+center.z*p.z+p.w < -radius*std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z)) return false;
  return true;
}
namespace {
constexpr float pi=3.14159265359f;
void Triangle(Geometry& g,unsigned a,unsigned b,unsigned c) { g.indices.insert(g.indices.end(),{a,b,c}); }
void Quad(Geometry& g,XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 d) {
  const auto first=static_cast<unsigned>(g.vertices.size());
  for (auto p : {a,b,c,d}) g.vertices.push_back({p,{}});
  Triangle(g,first,first+1,first+2); Triangle(g,first,first+2,first+3);
}
void Normals(Geometry& g) {
  for (auto& v : g.vertices) v.normal={};
  for (std::size_t i=0;i<g.indices.size();i+=3) {
    auto& a=g.vertices[g.indices[i]]; auto& b=g.vertices[g.indices[i+1]]; auto& c=g.vertices[g.indices[i+2]];
    const auto n=XMVector3Cross(XMLoadFloat3(&b.position)-XMLoadFloat3(&a.position),XMLoadFloat3(&c.position)-XMLoadFloat3(&a.position));
    for (auto index : {g.indices[i],g.indices[i+1],g.indices[i+2]}) {
      auto& v=g.vertices[index]; XMStoreFloat3(&v.normal,XMLoadFloat3(&v.normal)+n);
    }
  }
  for (auto& v : g.vertices) {
    const auto n=XMLoadFloat3(&v.normal);
    XMStoreFloat3(&v.normal,XMVectorGetX(XMVector3LengthSq(n))>.0000001f?XMVector3Normalize(n):XMVectorSet(0,1,0,0));
  }
}
// Each row is height, horizontal radius, depth radius. Shared ring vertices give
// smooth curved surfaces without storing a separate model for every body part.
Geometry Lathe(const std::vector<XMFLOAT3>& profile,unsigned segments,float irregular=0,float bend=0) {
  Geometry g;
  for (unsigned row=0;row<profile.size();++row) for (unsigned j=0;j<segments;++j) {
    const float angle=2*pi*j/segments;
    const auto p=profile[row];
    const float noise=1+irregular*(.5f*std::sin(angle*5+row*1.7f)+.3f*std::cos(angle*3-row));
    const float shift=bend*std::sin((p.x+.5f)*2);
    const float shade=1-irregular*.2f+irregular*.2f*std::sin(j*3.f+row);
    g.vertices.push_back({{std::sin(angle)*p.y*noise+shift,p.x,std::cos(angle)*p.z*noise},{},{shade,shade,shade,1}});
  }
  for (unsigned row=0;row+1<profile.size();++row) for (unsigned j=0;j<segments;++j) {
    const auto a=row*segments+j,b=row*segments+(j+1)%segments,c=b+segments,d=a+segments;
    Triangle(g,a,b,d); Triangle(g,b,c,d);
  }
  // Caps need separate vertices: blending their normals into a flared trunk or
  // narrow limb reverses the lighting on some triangles at the rim.
  const unsigned last=static_cast<unsigned>(profile.size()-1)*segments;
  const unsigned bottomRing=static_cast<unsigned>(g.vertices.size());
  for (unsigned j=0;j<segments;++j) { const auto vertex=g.vertices[j]; g.vertices.push_back(vertex); }
  const unsigned topRing=static_cast<unsigned>(g.vertices.size());
  for (unsigned j=0;j<segments;++j) { const auto vertex=g.vertices[last+j]; g.vertices.push_back(vertex); }
  const unsigned bottom=static_cast<unsigned>(g.vertices.size());
  g.vertices.push_back({{bend*std::sin((profile.front().x+.5f)*2),profile.front().x,0},{}});
  const unsigned top=static_cast<unsigned>(g.vertices.size());
  g.vertices.push_back({{bend*std::sin((profile.back().x+.5f)*2),profile.back().x,0},{}});
  for (unsigned j=0;j<segments;++j) {
    Triangle(g,bottomRing+j,bottom,bottomRing+(j+1)%segments);
    Triangle(g,topRing+j,topRing+(j+1)%segments,top);
  }
  Normals(g); return g;
}
std::vector<XMFLOAT3> SphereProfile() {
  std::vector<XMFLOAT3> p;
  for (int i=0;i<=10;++i) {
    const float angle=.08f+(pi-.16f)*i/10;
    p.push_back({-.5f*std::cos(angle),.5f*std::sin(angle),.5f*std::sin(angle)});
  }
  return p;
}
std::vector<float> BoundaryAngles() {
  std::vector<float> angles;
  for (int i=0;i<64;++i) angles.push_back(2*pi*i/64);
  const float corner=std::atan2(17.3f,19.3f);
  for (auto angle : {corner,pi-corner,pi+corner,2*pi-corner}) angles.push_back(angle);
  std::sort(angles.begin(),angles.end()); return angles;
}
XMFLOAT3 Edge(float angle,float scale,float height) {
  const float x=std::sin(angle),z=std::cos(angle);
  const float edge=std::min(17.3f/std::max(.0001f,std::abs(x)),19.3f/std::max(.0001f,std::abs(z)));
  const float bump=.35f+.35f*(1+std::sin(angle*11))* .5f;
  return {x*(edge+bump)*scale,height,z*(edge+bump)*scale};
}
float Hash(int x,int y) {
  std::uint32_t h=std::uint32_t(x)*374761393u+std::uint32_t(y)*668265263u;
  h=(h^(h>>13))*1274126177u; return float(h^(h>>16))/4294967295.f;
}
float Noise(float x,float y) {
  const int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y));
  float a=x-ix,b=y-iy; a=a*a*(3-2*a); b=b*b*(3-2*b);
  return (Hash(ix,iy)*(1-a)+Hash(ix+1,iy)*a)*(1-b)+(Hash(ix,iy+1)*(1-a)+Hash(ix+1,iy+1)*a)*b;
}
}

Geometry BuildMesh(Mesh mesh) {
  if (mesh==Mesh::Sphere) return Lathe(SphereProfile(),16);
  if (mesh==Mesh::Rock) return Lathe(SphereProfile(),12,.24f,.05f);
  if (mesh==Mesh::Trunk) return Lathe({{-.5f,.5f,.5f},{-.4f,.38f,.39f},{0,.25f,.28f},{.4f,.19f,.22f},{.5f,.12f,.14f}},10,.12f,.14f);
  if (mesh==Mesh::Foliage) return Lathe({{-.48f,.13f,.13f},{-.35f,.36f,.36f},{-.18f,.49f,.48f},{.02f,.47f,.5f},
    {.2f,.38f,.4f},{.37f,.23f,.27f},{.49f,.05f,.07f}},14,.24f,.03f);
  if (mesh==Mesh::Torso) return Lathe({{-.5f,.39f,.4f},{-.26f,.35f,.36f},{-.05f,.37f,.4f},
    {.22f,.5f,.46f},{.34f,.47f,.4f},{.5f,.22f,.25f}},14);
  if (mesh==Mesh::Limb) return Lathe({{-.5f,.24f,.24f},{-.40f,.36f,.36f},{-.1f,.4f,.4f},
    {.25f,.46f,.46f},{.42f,.42f,.42f},{.5f,.25f,.25f}},12);
  if (mesh==Mesh::Blade) return Lathe({{-.5f,.28f,.16f},{-.35f,.42f,.20f},{.28f,.35f,.15f},{.5f,.02f,.02f}},4);
  if (mesh==Mesh::Crystal) return Lathe({{-.5f,.07f,.06f},{-.25f,.4f,.35f},{.28f,.3f,.29f},{.5f,.04f,.03f}},6,.12f);
  Geometry g;
  if (mesh==Mesh::Box) {
    Quad(g,{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f},{.5f,-.5f,-.5f});
    Quad(g,{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f},{-.5f,-.5f,.5f});
    Quad(g,{-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f},{-.5f,-.5f,-.5f});
    Quad(g,{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f},{.5f,-.5f,.5f});
    Quad(g,{-.5f,.5f,-.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f});
    Quad(g,{-.5f,-.5f,.5f},{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f});
  } else if (mesh==Mesh::Roof) {
    Quad(g,{-.5f,-.5f,-.5f},{-.5f,-.5f,.5f},{0,.5f,.5f},{0,.5f,-.5f});
    Quad(g,{0,.5f,-.5f},{0,.5f,.5f},{.5f,-.5f,.5f},{.5f,-.5f,-.5f});
    g.vertices.insert(g.vertices.end(),{{{-.5f,-.5f,-.5f},{}},{{0,.5f,-.5f},{}},{{.5f,-.5f,-.5f},{}},
      {{.5f,-.5f,.5f},{}},{{0,.5f,.5f},{}},{{-.5f,-.5f,.5f},{}}});
    Triangle(g,8,9,10); Triangle(g,11,12,13);
  } else if (mesh==Mesh::Terrain || mesh==Mesh::Cliff) {
    const auto angles=BoundaryAngles(); const auto count=static_cast<unsigned>(angles.size());
    if (mesh==Mesh::Terrain) {
      g.vertices.push_back({{0,0,0},{}});
      for (float angle : angles) g.vertices.push_back({Edge(angle,1,0),{}});
      for (unsigned i=0;i<count;++i) Triangle(g,0,i+1,(i+1)%count+1);
    } else {
      for (int ring=0;ring<4;++ring) for (float angle : angles) {
        const float scale=ring==0?1.f:ring==1?1.02f:ring==2?.75f:.25f;
        const float y=ring==0?0.f:ring==1?-1.8f:ring==2?-4.5f:-7.5f;
        auto p=Edge(angle,scale,y+(ring==0?0.f:std::sin(angle*9+ring)*.5f));
        const float shade=1.f-ring*.12f;
        g.vertices.push_back({p,{},{shade,shade,shade,1}});
      }
      for (unsigned ring=0;ring<3;++ring) for (unsigned i=0;i<count;++i) {
        const auto a=ring*count+i,b=ring*count+(i+1)%count,c=b+count,d=a+count;
        Triangle(g,a,d,b); Triangle(g,b,d,c);
      }
    }
  }
  Normals(g); return g;
}

std::vector<std::uint8_t> BuildMaterialLayer(Material material,unsigned size) {
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size)*size*4);
  for (unsigned y=0;y<size;++y) for (unsigned x=0;x<size;++x) {
    const float u=float(x)/size,v=float(y)/size;
    const float fine=Hash(static_cast<int>(x),static_cast<int>(y));
    float value=.8f+.18f*Noise(u*12,v*12)+.06f*fine;
    if (material==Material::Wood) value=.72f+.2f*Noise(u*35+Noise(u*3,v*3)*2,v*3)+.08f*std::sin(u*180+Noise(u*5,v*5)*4);
    if (material==Material::Stone) value=.7f+.23f*Noise(u*8,v*8)+.09f*fine;
    if (material==Material::Ground) value=.69f+.23f*Noise(u*10,v*10)+.13f*fine;
    if (material==Material::Cloth) value=.9f+.05f*float((x+y)%2)+.025f*fine;
    if (material==Material::Roof) {
      const float row=std::floor(v*8),ty=v*8-row;
      const float tx=u*5+row*.5f-std::floor(u*5+row*.5f);
      value=tx<.035f || ty<.08f?.48f:.78f+.14f*ty+.05f*fine;
    }
    if (material==Material::Plain || material==Material::Glow) value=1;
    const auto intensity=static_cast<std::uint8_t>(std::clamp(value,0.f,1.f)*255);
    const auto i=(static_cast<std::size_t>(y)*size+x)*4;
    pixels[i]=pixels[i+1]=pixels[i+2]=intensity; pixels[i+3]=255;
  }
  return pixels;
}
}
