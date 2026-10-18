#include "engine/Geometry.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace visual {
using namespace DirectX;
Frustum::Frustum(const XMFLOAT4X4& m) {
  planes_={{
    {m._14+m._11,m._24+m._21,m._34+m._31,m._44+m._41},
    {m._14-m._11,m._24-m._21,m._34-m._31,m._44-m._41},
    {m._14+m._12,m._24+m._22,m._34+m._32,m._44+m._42},
    {m._14-m._12,m._24-m._22,m._34-m._32,m._44-m._42},
    {m._13,m._23,m._33,m._43},{m._14-m._13,m._24-m._23,m._34-m._33,m._44-m._43}}};
  for (std::size_t i=0;i<planes_.size();++i) {
    const auto& p=planes_[i]; lengths_[i]=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
  }
}
bool Frustum::VisibleSphere(const XMFLOAT3& center,float radius) const {
  for (std::size_t i=0;i<planes_.size();++i) {
    const auto& p=planes_[i];
    if (center.x*p.x+center.y*p.y+center.z*p.z+p.w < -radius*lengths_[i]) return false;
  }
  return true;
}
bool VisibleSphere(const XMFLOAT3& center,float radius,const XMFLOAT4X4& m) {
  return Frustum(m).VisibleSphere(center,radius);
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
}

Geometry BuildMesh(Mesh mesh) {
  if (mesh==Mesh::Masonry) {
    Geometry g;
    const auto face=[&](std::vector<XMFLOAT3> points) {
      const auto a=XMLoadFloat3(&points[0]),b=XMLoadFloat3(&points[1]),c=XMLoadFloat3(&points[2]);
      if (XMVectorGetX(XMVector3Dot(XMVector3Cross(b-a,c-a),a+b+c))<0) std::reverse(points.begin(),points.end());
      const auto first=static_cast<unsigned>(g.vertices.size());
      for (auto p : points) g.vertices.push_back({p,{}});
      for (unsigned i=1;i+1<points.size();++i) Triangle(g,first,first+i,first+i+1);
    };
    const auto point=[](int axis,float fixed,float u,float v) {
      if (axis==0) return XMFLOAT3{fixed,u,v};
      if (axis==1) return XMFLOAT3{v,fixed,u};
      return XMFLOAT3{u,v,fixed};
    };
    for (int axis=0;axis<3;++axis) {
      for (float side : {-1.f,1.f})
        face({point(axis,side*.5f,-.4f,-.4f),point(axis,side*.5f,.4f,-.4f),
          point(axis,side*.5f,.4f,.4f),point(axis,side*.5f,-.4f,.4f)});
      for (float u : {-1.f,1.f}) for (float v : {-1.f,1.f})
        face({point(axis,-.4f,u*.5f,v*.4f),point(axis,-.4f,u*.4f,v*.5f),
          point(axis,.4f,u*.4f,v*.5f),point(axis,.4f,u*.5f,v*.4f)});
    }
    for (float x : {-1.f,1.f}) for (float y : {-1.f,1.f}) for (float z : {-1.f,1.f})
      face({{x*.5f,y*.4f,z*.4f},{x*.4f,y*.5f,z*.4f},{x*.4f,y*.4f,z*.5f}});
    Normals(g); return g;
  }
  if (mesh==Mesh::Barrel) return Lathe({{-.5f,.40f,.40f},{-.43f,.43f,.43f},{-.25f,.48f,.48f},
    {0,.50f,.50f},{.25f,.48f,.48f},{.43f,.43f,.43f},{.5f,.40f,.40f}},16);
  if (mesh==Mesh::Arch) {
    Geometry g;
    // Separate masonry wedges retain mortar gaps and crisp face normals.
    for (int stone=0;stone<12;++stone) {
      const float a=stone*pi/12+.012f,b=(stone+1)*pi/12-.012f;
      const auto p=[](float angle,float radius,float depth) {
        return XMFLOAT3{std::cos(angle)*radius,std::sin(angle)*radius,depth};
      };
      const auto ia=p(a,.36f,-.5f),ib=p(b,.36f,-.5f),oa=p(a,.5f,-.5f),ob=p(b,.5f,-.5f);
      const auto iA=p(a,.36f,.5f),iB=p(b,.36f,.5f),oA=p(a,.5f,.5f),oB=p(b,.5f,.5f);
      Quad(g,ia,ib,ob,oa); Quad(g,iA,oA,oB,iB);
      Quad(g,oa,ob,oB,oA); Quad(g,ia,iA,iB,ib);
      Quad(g,ia,oa,oA,iA); Quad(g,ib,iB,oB,ob);
    }
    Normals(g); return g;
  }
  if (mesh==Mesh::Canopy) {
    Geometry g;
    // A bowed sheet with stripes and a short valance, opaque both sides.
    const auto panel=[&](XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 d,float shade) {
      const auto start=g.vertices.size(); Quad(g,a,b,c,d); Quad(g,d,c,b,a);
      for (auto i=start;i<g.vertices.size();++i) g.vertices[i].color={shade,shade,shade,1};
    };
    for (int strip=0;strip<10;++strip) {
      const float left=-.5f+strip*.1f,right=left+.1f,shade=strip%2?.58f:1.f;
      for (int row=0;row<8;++row) {
        const float z=-.5f+row*.125f,next=z+.125f;
        const auto height=[](float depth) { return .10f*std::cos(depth*pi)-.12f*depth; };
        panel({left,height(z),z},{left,height(next),next},{right,height(next),next},{right,height(z),z},shade);
      }
      panel({left,.06f,-.5f},{left,-.09f,-.5f},{right,-.09f,-.5f},{right,.06f,-.5f},shade);
    }
    Normals(g); return g;
  }
  if (mesh==Mesh::Foliage) {
    Geometry g;
    // Folded, opaque leaves with separate back faces give the crown real gaps.
    // No alpha texture, transparency sorting or per-frame mesh generation.
    for (unsigned leaf=0;leaf<144;++leaf) {
      const float y=-.92f+1.84f*(leaf%108+.5f)/108,angle=leaf*2.399963f+Hash(int(leaf),5)*.3f;
      const float radial=std::sqrt(1-y*y),radius=(leaf<108?.30f:.18f)+Hash(int(leaf),9)*.08f;
      const XMFLOAT3 center{std::sin(angle)*radial*radius,y*radius,std::cos(angle)*radial*radius};
      const auto normal=XMVector3Normalize(XMVectorSet(std::sin(angle)*radial,y+.6f,std::cos(angle)*radial,0));
      const auto tangent=XMVector3Normalize(XMVector3Cross(normal,XMVectorSet(0,1,0,0)));
      const auto bitangent=XMVector3Cross(tangent,normal);
      const float roll=Hash(int(leaf),13)*6.2831853f;
      const auto right=tangent*std::cos(roll)+bitangent*std::sin(roll);
      const auto up=XMVector3Cross(right,normal);
      const float width=.035f+Hash(int(leaf),3)*.03f,length=.075f+Hash(7,int(leaf))*.045f;
      const auto c=XMLoadFloat3(&center);
      const XMVECTOR points[]={c-up*length,c-right*width,c+up*length,c+right*width,c+normal*.024f};
      const float shade=.72f+Hash(int(leaf),11)*.28f;
      for (unsigned side=0;side<2;++side) {
        const auto start=static_cast<unsigned>(g.vertices.size());
        for (const auto& point : points) { XMFLOAT3 p; XMStoreFloat3(&p,point); g.vertices.push_back({p,{},{shade,shade,shade,1}}); }
        for (unsigned edge=0;edge<4;++edge)
          if (side==0) Triangle(g,start+edge,start+(edge+1)%4,start+4);
          else Triangle(g,start+(edge+1)%4,start+edge,start+4);
      }
    }
    Normals(g); return g;
  }
  if (mesh==Mesh::Grass) {
    Geometry g;
    for (unsigned blade=0;blade<12;++blade) {
      const float angle=blade*2.399963f,width=.022f+Hash(int(blade),1)*.018f,height=.22f+Hash(2,int(blade))*.23f;
      const float radius=.22f*std::sqrt(Hash(int(blade),6));
      const float x=std::sin(angle)*radius,z=std::cos(angle)*radius,dx=std::cos(angle)*width,dz=-std::sin(angle)*width;
      const XMFLOAT3 points[]={{x-dx,.005f,z-dz},{x+dx,.005f,z+dz},
        {x-dx*.6f,height*.58f,z-dz*.6f},{x+dx*.6f,height*.58f,z+dz*.6f},
        {x+std::sin(angle)*.10f,height,z+std::cos(angle)*.10f}};
      for (unsigned side=0;side<2;++side) {
        const auto start=static_cast<unsigned>(g.vertices.size());
        for (const auto& p : points) {
          const float shade=.6f+.4f*p.y/height; g.vertices.push_back({p,{},{shade,shade,shade,1}});
        }
        const unsigned faces[][3]={{0,2,1},{1,2,3},{2,4,3}};
        for (const auto& face : faces) Triangle(g,start+face[0],start+face[side?2:1],start+face[side?1:2]);
      }
    }
    Normals(g); return g;
  }
  if (mesh==Mesh::Hair) {
    auto g=Lathe({{.17f,.46f,.45f},{.24f,.45f,.44f},{.39f,.39f,.38f},{.51f,.19f,.19f}},20);
    for (auto& v : g.vertices) if (v.position.z>0) v.position.z=std::min(v.position.z,.36f);
    Normals(g); return g;
  }
  if (mesh==Mesh::Head) {
    auto g=Lathe({{-.5f,.20f,.26f},{-.38f,.34f,.35f},{-.19f,.43f,.40f},{.02f,.46f,.44f},
      {.21f,.45f,.44f},{.38f,.38f,.37f},{.49f,.19f,.19f}},20);
    // A narrower jaw, flatter face, cheekbones and brow replace the spherical head.
    for (auto& v : g.vertices) {
      auto& p=v.position;
      if (p.z>0) {
        p.z=std::min(p.z,.34f);
        const float brow=(p.y-.05f)*8,cheekHeight=p.y*7,cheekWidth=(std::abs(p.x)-.28f)*10;
        p.z+=.045f*std::exp(-brow*brow);
        p.z+=.035f*std::exp(-cheekHeight*cheekHeight)*std::exp(-cheekWidth*cheekWidth);
      }
    }
    Normals(g); return g;
  }
  if (mesh==Mesh::Boot) {
    auto g=Lathe({{-.5f,.37f,.44f},{-.38f,.44f,.49f},{-.18f,.43f,.47f},{.02f,.36f,.37f},
      {.19f,.28f,.24f},{.5f,.27f,.24f}},16);
    // Flat sole and extended toe; the upper rows form an ankle instead of a ball.
    for (auto& v : g.vertices) v.position.z+=.12f*(.5f-v.position.y);
    Normals(g); return g;
  }
  if (mesh==Mesh::Sphere) return Lathe(SphereProfile(),16);
  if (mesh==Mesh::Rock) return Lathe(SphereProfile(),12,.24f,.05f);
  if (mesh==Mesh::Trunk) return Lathe({{-.5f,.5f,.5f},{-.4f,.38f,.39f},{0,.25f,.28f},{.4f,.19f,.22f},{.5f,.12f,.14f}},10,.12f,.14f);
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
  std::vector<float> heights(static_cast<std::size_t>(size)*size);
  // Periodic lattice noise keeps height and color continuous across wrap seams.
  const auto noise=[](float u,float v,int period) {
    const float x=u*period,y=v*period;
    const int ix=int(std::floor(x)),iy=int(std::floor(y));
    const auto wrap=[&](int value) { return (value%period+period)%period; };
    const float a=x-std::floor(x),b=y-std::floor(y),sx=a*a*(3-2*a),sy=b*b*(3-2*b);
    const float low=Hash(wrap(ix),wrap(iy))*(1-sx)+Hash(wrap(ix+1),wrap(iy))*sx;
    const float high=Hash(wrap(ix),wrap(iy+1))*(1-sx)+Hash(wrap(ix+1),wrap(iy+1))*sx;
    return low*(1-sy)+high*sy;
  };
  const auto smooth=[](float value) { value=std::clamp(value,0.f,1.f); return value*value*(3-2*value); };
  for (unsigned y=0;y<size;++y) for (unsigned x=0;x<size;++x) {
    const float u=float(x)/size,v=float(y)/size;
    const float broad=noise(u,v,4),fine=noise(u,v,24);
    float value=.86f+.12f*broad,height=0,roughness=.8f;
    switch (material) {
    case Material::Wood: {
      const float grain=.5f+.5f*std::sin(2*pi*(u*18+.45f*noise(u,v,4)));
      const float plank=u*4-std::floor(u*4);
      const float edge=smooth(std::min(plank,1-plank)/.055f);
      value=(.64f+.21f*grain+.12f*broad)*(.65f+.35f*edge);
      height=.007f*grain+.013f*edge; roughness=.72f+.18f*broad; break;
    }
    case Material::Stone:
      value=.68f+.23f*broad+.09f*fine; height=.028f*broad+.008f*fine;
      roughness=.78f+.18f*fine; break;
    case Material::Ground:
      value=.72f+.20f*noise(u,v,10)+.08f*fine; height=.009f*noise(u,v,16);
      roughness=.96f; break;
    case Material::Cloth: {
      const float weave=std::sin(u*2*pi*24)*std::sin(v*2*pi*24);
      value=.94f+.035f*weave; height=.0008f*weave; roughness=.94f; break;
    }
    case Material::Roof: {
      const float row=std::floor(v*6),ty=v*6-row;
      const float tx=u*4+row*.5f-std::floor(u*4+row*.5f);
      const float edge=smooth(std::min(tx,1-tx)/.09f)*smooth(ty/.12f);
      value=.56f+edge*(.26f+.10f*ty)+.04f*fine;
      height=.032f*edge+.012f*ty; roughness=.72f+.16f*fine; break;
    }
    case Material::Plaster:
      value=.87f+.10f*broad+.03f*fine; height=.004f*fine; roughness=.94f; break;
    case Material::Metal:
      value=.89f+.08f*broad+.03f*fine; height=.0007f*noise(u,v,32);
      roughness=.28f+.13f*broad; break;
    case Material::Skin: value=.97f+.03f*broad; roughness=.64f; break;
    case Material::Leaves: value=.87f+.13f*broad; roughness=.86f; break;
    case Material::Plain: case Material::Glow: value=1; break;
    default: break;
    }
    const auto i=(static_cast<std::size_t>(y)*size+x)*4;
    pixels[i]=static_cast<std::uint8_t>(std::clamp(value,0.f,1.f)*255);
    pixels[i+3]=static_cast<std::uint8_t>(roughness*255);
    heights[static_cast<std::size_t>(y)*size+x]=height;
  }
  for (unsigned y=0;y<size;++y) for (unsigned x=0;x<size;++x) {
    const float du=(heights[static_cast<std::size_t>(y)*size+(x+1)%size]-
      heights[static_cast<std::size_t>(y)*size+(x+size-1)%size])*size*.5f;
    const float dv=(heights[static_cast<std::size_t>((y+1)%size)*size+x]-
      heights[static_cast<std::size_t>((y+size-1)%size)*size+x])*size*.5f;
    const auto encode=[](float slope) { return static_cast<std::uint8_t>(std::lround(std::clamp(128.f+slope*32.f,0.f,255.f))); };
    const auto i=(static_cast<std::size_t>(y)*size+x)*4;
    pixels[i+1]=encode(du); pixels[i+2]=encode(dv);
  }
  return pixels;
}
}
