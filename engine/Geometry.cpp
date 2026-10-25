#include "engine/Geometry.h"
#include "game/WorldMap.h"
#include "engine/Skinning.h"
#include "engine/HandSkinning.h"
#include "Blade.generated.h"
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
    XMStoreFloat3(&v.normal,XMVectorGetX(XMVector3LengthSq(n))>1e-20f?XMVector3Normalize(n):XMVectorSet(0,1,0,0));
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
// Complete, immutable weapons in grip-local metres. Parts are merged once at
// startup: worn trim and fitted hardware do not multiply per-frame draw calls.
Geometry Equipment(Mesh mesh) {
  Geometry g;
  const XMFLOAT4 wood{.30f,.16f,.075f,1},iron{.46f,.51f,.55f,1},brass{.80f,.57f,.24f,1},pale{.85f,.89f,.88f,1};
  const auto append=[&](const Geometry& part,const XMMATRIX& transform,XMFLOAT4 color) {
    const auto offset=std::uint32_t(g.vertices.size());
    const auto normal=XMMatrixTranspose(XMMatrixInverse(nullptr,transform));
    for(auto v:part.vertices) {
      XMStoreFloat3(&v.position,XMVector3TransformCoord(XMLoadFloat3(&v.position),transform));
      XMStoreFloat3(&v.normal,XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&v.normal),normal)));
      v.color={v.color.x*color.x,v.color.y*color.y,v.color.z*color.z,1};g.vertices.push_back(v);
    }
    for(auto i:part.indices) g.indices.push_back(offset+i);
  };
  const auto shape=[&](const std::vector<XMFLOAT3>& profile,unsigned sides,XMFLOAT4 color,const XMMATRIX& transform=XMMatrixIdentity()) {
    append(Lathe(profile,sides),transform,color);
  };
  const auto box=[&](XMFLOAT3 p,XMFLOAT3 size,XMFLOAT4 color,float roll=0.f) {
    append(BuildMesh(Mesh::Box),XMMatrixScaling(size.x,size.y,size.z)*XMMatrixRotationZ(roll)*XMMatrixTranslation(p.x,p.y,p.z),color);
  };
  const auto rod=[&](XMFLOAT3 a,XMFLOAT3 b,float radius,XMFLOAT4 color) {
    const auto delta=XMLoadFloat3(&b)-XMLoadFloat3(&a);
    const float length=XMVectorGetX(XMVector3Length(delta));
    const auto y=delta/length;
    const auto seed=std::abs(XMVectorGetY(y))>.9f?XMVectorSet(0,0,1,0):XMVectorSet(0,1,0,0);
    const auto x=XMVector3Normalize(XMVector3Cross(seed,y)),z=XMVector3Cross(x,y);
    const XMMATRIX frame{x,y,z,XMVectorSet(a.x,a.y,a.z,1)};
    shape({{0,radius,radius},{length,radius*.85f,radius*.85f}},8,color,frame);
  };
  const auto jewel=[&](XMFLOAT3 p,float radius,float height,XMFLOAT4 color) {
    shape({{0,.004f,.004f},{height*.25f,radius,radius},{height*.75f,radius*.70f,radius*.70f},{height,.004f,.004f}},6,color,
      XMMatrixTranslation(p.x,p.y,p.z));
  };
  if(mesh==Mesh::WeaponPistol || mesh==Mesh::WeaponHandbow) {
    shape({{-.13f,.027f,.028f},{-.09f,.024f,.024f},{.09f,.024f,.024f},{.16f,.044f,.05f}},12,wood);
    box({0,.14f,.02f},{.09f,.085f,.27f},brass);
    if(mesh==Mesh::WeaponPistol) {
      // Y-profile is rotated onto +Z. A dark inset sits just behind the rim.
      shape({{-.07f,.049f,.049f},{.30f,.040f,.040f},{.34f,.044f,.044f},{.40f,.044f,.044f}},12,brass,
        XMMatrixRotationX(XM_PIDIV2)*XMMatrixTranslation(0,.16f,0));
      shape({{0,.029f,.029f},{.004f,.029f,.029f}},12,{.035f,.032f,.03f,1},
        XMMatrixRotationX(XM_PIDIV2)*XMMatrixTranslation(0,.16f,.4001f));
      box({0,.216f,-.075f},{.025f,.065f,.045f},iron);
      box({0,.211f,.31f},{.015f,.026f,.025f},iron);
      rod({0,-.01f,.055f},{0,-.035f,.12f},.008f,brass);
      rod({0,-.035f,.12f},{0,.10f,.15f},.008f,brass);
    } else {
      box({0,.16f,.11f},{.046f,.04f,.50f},wood);
      box({0,.184f,.13f},{.012f,.008f,.46f},iron);
      for(float side:{-1.f,1.f}) {
        XMFLOAT3 last{0,.16f,.28f};
        for(int i=1;i<=6;++i) {
          const float t=i/6.f;XMFLOAT3 next{side*t*.33f,.16f+.025f*std::sin(t*XM_PI),.28f-.14f*t*t};
          rod(last,next,.013f,brass);last=next;
        }
        rod(last,{0,.16f,-.06f},.0028f,pale);
      }
      rod({0,.191f,-.065f},{0,.191f,.36f},.004f,pale);
    }
    return g;
  }
  const bool staff=mesh==Mesh::WeaponGroveStaff || mesh==Mesh::WeaponAstralStaff || mesh==Mesh::WeaponAshStaff;
  const float bottom=staff?-.73f:-.12f,top=staff?.67f:mesh==Mesh::WeaponBaton?.54f:.45f;
  shape({{bottom,.026f,.026f},{bottom+.01f,.028f,.028f},{-.09f,.024f,.024f},{.09f,.024f,.024f},
    {top-.04f,.023f,.023f},{top,.035f,.035f}},12,wood);
  for(float y:{staff?bottom+.015f:-.13f,-.12f,.105f,top-.045f})
    shape({{y,.034f,.034f},{y+.022f,.034f,.034f}},12,brass);
  // Leather binding is real raised geometry, kept above the closed grip radius.
  for(int i=0;i<5;++i) shape({{-.08f+i*.034f,.025f,.025f},{-.069f+i*.034f,.025f,.025f}},10,{.14f,.09f,.055f,1});
  if(mesh==Mesh::WeaponMace) {
    shape({{.37f,.05f,.05f},{.42f,.075f,.075f},{.62f,.075f,.075f},{.67f,.038f,.038f}},12,iron);
    for(int i=0;i<6;++i) {
      const float a=i*XM_PI/3;
      append(Lathe({{.40f,.012f,.012f},{.45f,.08f,.024f},{.60f,.08f,.024f},{.64f,.012f,.012f}},6),
        XMMatrixTranslation(.045f,0,0)*XMMatrixRotationY(a),pale);
    }
  } else if(mesh==Mesh::WeaponCleaver) {
    shape({{.11f,.035f,.022f},{.18f,.10f,.024f},{.68f,.14f,.026f},{.76f,.125f,.02f},{.80f,.08f,.004f}},8,iron);
    box({0,.12f,0},{.27f,.045f,.065f},brass);
    for(int i=0;i<3;++i) box({.05f,.27f+i*.14f,.026f},{.04f,.018f,.008f},brass);
  } else if(mesh==Mesh::WeaponBaton) {
    jewel({0,.48f,0},.035f,.14f,pale);
    jewel({0,-.18f,0},.044f,.07f,brass);
  } else {
    const XMFLOAT4 stone=mesh==Mesh::WeaponGroveStaff?XMFLOAT4{.35f,.69f,.25f,1}:
      mesh==Mesh::WeaponAshStaff?XMFLOAT4{.65f,.82f,.72f,1}:
      mesh==Mesh::WeaponScepter?XMFLOAT4{.94f,.79f,.33f,1}:XMFLOAT4{.42f,.51f,.91f,1};
    const float crown=staff?.68f:.42f;
    jewel({0,crown,0},mesh==Mesh::WeaponFocus?.11f:.075f,.22f,stone);
    for(int i=0;i<4;++i) {
      const float a=i*XM_PIDIV2;
      const XMFLOAT3 root{std::sin(a)*.035f,crown-.04f,std::cos(a)*.035f};
      const XMFLOAT3 mid{std::sin(a)*.13f,crown+.08f,std::cos(a)*.13f};
      const XMFLOAT3 tip{std::sin(a)*.07f,crown+.25f,std::cos(a)*.07f};
      const auto trim=mesh==Mesh::WeaponGroveStaff?wood:brass;
      rod(root,mid,.013f,trim);rod(mid,tip,.011f,trim);
      if(mesh==Mesh::WeaponGroveStaff) jewel(mid,.025f,.10f,stone);
    }
    if(mesh==Mesh::WeaponAshStaff) {
      XMFLOAT3 last{0,.65f,0};
      for(int i=1;i<=10;++i) {
        const float a=i*XM_PI*.15f;
        XMFLOAT3 next{.17f*(1-std::cos(a)),.65f+.17f*std::sin(a),0};
        rod(last,next,.024f,wood);last=next;
      }
    }
  }
  return g;
}

std::vector<float> BoundaryAngles() {
  std::vector<float> angles;
  for (int i=0;i<64;++i) angles.push_back(2*pi*i/64);
  const float corner=std::atan2(rpg::WorldMap::halfWidth+.3f,rpg::WorldMap::halfDepth+.3f);
  for (auto angle : {corner,pi-corner,pi+corner,2*pi-corner}) angles.push_back(angle);
  std::sort(angles.begin(),angles.end()); return angles;
}
XMFLOAT3 Edge(float angle,float scale,float height) {
  const float x=std::sin(angle),z=std::cos(angle);
  const float edge=std::min((rpg::WorldMap::halfWidth+.3f)/std::max(.0001f,std::abs(x)),
    (rpg::WorldMap::halfDepth+.3f)/std::max(.0001f,std::abs(z)));
  const float bump=.35f+.35f*(1+std::sin(angle*11))* .5f;
  return {x*(edge+bump)*scale,height,z*(edge+bump)*scale};
}
float Hash(int x,int y) {
  std::uint32_t h=std::uint32_t(x)*374761393u+std::uint32_t(y)*668265263u;
  h=(h^(h>>13))*1274126177u; return float(h^(h>>16))/4294967295.f;
}
}

Geometry BuildMesh(Mesh mesh) {
  if(mesh>=Mesh::WeaponMace && mesh<=Mesh::WeaponCleaver) return Equipment(mesh);
  if (IsBodyMesh(mesh)) return CharacterBody(unsigned(mesh)-unsigned(Mesh::Body)).mesh;
  if (IsHandMesh(mesh)) return CharacterHand((unsigned(mesh)-unsigned(Mesh::Hand))/2,(unsigned(mesh)-unsigned(Mesh::Hand))%2!=0).mesh;
  if (mesh==Mesh::CoatPanel) {
    Geometry g;
    constexpr unsigned rows=10,columns=8,stride=columns+1,sideVertices=(rows+1)*stride;
    for(unsigned side=0;side<2;++side) {
      for(unsigned row=0;row<=rows;++row) for(unsigned column=0;column<=columns;++column) {
        const float t=float(row)/rows,u=2.f*column/columns-1;
        // Pleats gather at the belt and relax toward a curved, turned hem.
        // Preserve the shared topology and two-sided normals at every LOD.
        const float pleat=(.025f+.035f*t)*std::cos(u*3*3.14159265f);
        const float hem=.025f*t*t*t*(1-u*u);
        const XMFLOAT3 p{u*(.36f+.14f*t),.5f-t+hem,.72f+.18f*(1-u*u)+pleat+.025f*t*t*t};
        const bool trim=column==0 || column==columns || row==rows;
        const float shade=1-.12f*t+.04f*std::cos(u*3*3.14159265f);
        g.vertices.push_back({p,{},trim?XMFLOAT4{.72f,.57f,.33f,1}:XMFLOAT4{shade,shade,shade,1}});
      }
      for(unsigned row=0;row<rows;++row) for(unsigned column=0;column<columns;++column) {
        const unsigned a=side*sideVertices+row*stride+column,b=a+1,c=b+stride,d=a+stride;
        if(side==0) {Triangle(g,a,b,c);Triangle(g,a,c,d);}
        else {Triangle(g,a,c,b);Triangle(g,a,d,c);}
      }
    }
    Normals(g);return g;
  }
  if(mesh==Mesh::Pauldron || mesh==Mesh::PauldronLow) {
    // Rolled lower edge, raised crown and elongated shoulder shell. Reuses
    // the same attachment transform and distant-detail policy as other armor.
    auto g=Lathe({{-.5f,.35f,.43f},{-.39f,.43f,.50f},{-.27f,.45f,.50f},
      {-.15f,.40f,.47f},{.10f,.37f,.43f},{.34f,.26f,.32f},{.5f,.035f,.05f}},
      mesh==Mesh::Pauldron?32:12);
    for(auto& v:g.vertices) {
      const float ridge=std::max(0.f,1-std::abs(v.position.x)/.10f);
      v.position.y+=.025f*ridge*std::max(0.f,v.position.y+.15f);
      if(v.position.y<-.27f) v.color={.81f,.76f,.64f,1};
    }
    Normals(g);return g;
  }
  if(mesh==Mesh::Helm || mesh==Mesh::HelmLow) {
    // Fitted bowl with rolled brow and a raised central ridge, replacing the
    // spherical cap. The distant variant retains the same silhouette.
    auto g=Lathe({{-.5f,.46f,.45f},{-.44f,.50f,.49f},{-.34f,.47f,.47f},
      {-.05f,.46f,.46f},{.22f,.37f,.39f},{.40f,.23f,.27f},{.5f,.035f,.045f}},
      mesh==Mesh::Helm?32:12);
    for(auto& v:g.vertices) {
      const float crown=std::max(0.f,1-std::abs(v.position.x)/.09f);
      v.position.y+=.035f*crown*std::max(0.f,v.position.y+.34f);
      if(v.position.y<-.34f) v.color={.77f,.73f,.62f,1};
    }
    Normals(g);return g;
  }
  if (mesh==Mesh::Cloak) {
    Geometry g;
    // One continuous flared sheet, shared smooth normals and a separate back
    // surface. Coordinates are metres from the pinned shoulder seam.
    constexpr unsigned columns=16,rows=16,stride=columns+1,sideVertices=stride*(rows+1);
    for (unsigned side=0;side<2;++side) {
      for (unsigned row=0;row<=rows;++row) for (unsigned column=0;column<=columns;++column) {
        const float t=float(row)/rows,u=2.f*column/columns-1;
        const float fold=std::cos(u*4*3.14159265f)*(.009f+t*.022f)*std::sin(t*1.5707963f);
        const float edge=std::abs(u);
        const XMFLOAT3 p{u*(.22f+t*.14f),-.82f*t,-.025f-.115f*t-fold-.035f*u*u};
        const bool trim=column==0 || column==columns || row==rows;
        const float shade=1-.18f*t-.12f*edge;
        g.vertices.push_back({p,{},trim?XMFLOAT4{.66f,.48f,.22f,1}:XMFLOAT4{.13f*shade,.28f*shade,.39f*shade,1}});
      }
      for (unsigned row=0;row<rows;++row) for (unsigned column=0;column<columns;++column) {
        const unsigned a=side*sideVertices+row*stride+column,b=a+1,c=b+stride,d=a+stride;
        if (side==0) { Triangle(g,a,b,c); Triangle(g,a,c,d); }
        else { Triangle(g,a,c,b); Triangle(g,a,d,c); }
      }
    }
    Normals(g); return g;
  }
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
      // Six tapered edge points give each leaf a fuller blade instead of a
      // four-corner diamond. The shallow ridge avoids a rigid paper-fold look.
      const float curl=(Hash(int(leaf),17)-.5f)*.012f;
      const XMVECTOR points[]={c-up*length,c-right*width-up*length*.38f,
        c-right*width*.78f+up*length*.40f,c+up*length+normal*curl,
        c+right*width*.78f+up*length*.40f,c+right*width-up*length*.38f,c+normal*.010f};
      const float shade=.76f+Hash(int(leaf),11)*.24f;
      const float age=Hash(int(leaf),23);
      for (unsigned side=0;side<2;++side) {
        const auto start=static_cast<unsigned>(g.vertices.size());
        unsigned pointIndex=0;
        for (const auto& point : points) {
          XMFLOAT3 p; XMStoreFloat3(&p,point);
          const float pigment=pointIndex==6?1.f:pointIndex==0?.84f:.92f;
          const float underside=(side?.91f:1.f)*pigment;
          g.vertices.push_back({p,{},{shade*(.88f+.12f*age)*underside,shade*underside,
            shade*(.74f+.14f*age)*(side?1.12f:1.f)*pigment,1}});
          ++pointIndex;
        }
        for (unsigned edge=0;edge<6;++edge)
          if (side==0) Triangle(g,start+edge,start+(edge+1)%6,start+6);
          else Triangle(g,start+(edge+1)%6,start+edge,start+6);
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
  if (mesh==Mesh::Hair || mesh==Mesh::HairLow) {
    auto g=Lathe({{.17f,.46f,.45f},{.24f,.45f,.44f},{.32f,.43f,.42f},{.39f,.39f,.38f},{.46f,.30f,.30f},{.51f,.19f,.19f}},mesh==Mesh::Hair?32:12);
    for (auto& v : g.vertices) {
      auto& p=v.position;
      const float angle=std::atan2(p.x,p.z),lower=std::clamp((.34f-p.y)/.17f,0.f,1.f);
      const float back=std::clamp((.18f-p.z)/.50f,0.f,1.f);
      // A swept, uneven hairline and longer nape replace the straight cap edge.
      p.y-=lower*(.20f*back+.018f*std::sin(angle*3+.6f));
      const float lock=std::sin(angle*11+p.y*13);
      const float relief=.009f*lock*std::clamp((.51f-p.y)/.15f,0.f,1.f);
      p.x+=std::sin(angle)*relief;p.z+=std::cos(angle)*relief;
      if(p.z>0) p.z=std::min(p.z,.36f+.012f*std::sin(p.x*12));
      const float shade=.88f+.10f*(lock*.5f+.5f);v.color={shade,shade,shade,1};
    }
    Normals(g); return g;
  }
  if (IsHeadMesh(mesh)) {
    const std::vector<XMFLOAT3> profile={{-.5f,.16f,.23f},{-.46f,.23f,.29f},{-.42f,.29f,.33f},{-.36f,.34f,.36f},
      {-.30f,.38f,.38f},{-.25f,.40f,.39f},{-.20f,.43f,.40f},{-.15f,.45f,.41f},
      {-.10f,.46f,.42f},{-.05f,.46f,.43f},{0,.46f,.44f},{.05f,.45f,.44f},
      {.10f,.44f,.43f},{.15f,.44f,.44f},{.20f,.45f,.44f},{.26f,.44f,.43f},
      {.32f,.42f,.40f},{.38f,.38f,.37f},{.44f,.30f,.29f},{.49f,.19f,.19f}};
    std::vector<XMFLOAT3> rings;
    for(std::size_t row=0;row<profile.size();++row) {
      rings.push_back(profile[row]);
      if(mesh==Mesh::Head && row+1<profile.size()) {
        const auto a=profile[row],b=profile[row+1];
        rings.push_back({(a.x+b.x)*.5f,(a.y+b.y)*.5f,(a.z+b.z)*.5f});
      }
    }
    auto g=Lathe(rings,mesh==Mesh::Head?64:mesh==Mesh::HeadMedium?24:12);
    // Continuous bridge, nose tip, cheeks, philtrum and chin. Extra rings are
    // concentrated around the features; the nose is no longer a floating ball.
    for (auto& v : g.vertices) {
      auto& p=v.position;
      if (p.z>0) {
        const float front=std::clamp(p.z/.20f,0.f,1.f);
        p.z=std::min(p.z,.34f);
        const auto bump=[](float x,float y) { return std::exp(-x*x-y*y); };
        p.z+=front*(.035f*bump((std::abs(p.x)-.20f)/.18f,(p.y-.15f)/.065f)
          +.055f*bump((std::abs(p.x)-.29f)/.14f,(p.y+.055f)/.10f)
          -.050f*bump((std::abs(p.x)-.20f)/.12f,(p.y-.07f)/.06f)
          +.105f*bump(p.x/.075f,(p.y-.01f)/.17f)
          +.180f*bump(p.x/.105f,(p.y+.10f)/.075f)
          +.040f*bump((std::abs(p.x)-.10f)/.055f,(p.y+.14f)/.05f)
          -.018f*bump(p.x/.035f,(p.y+.20f)/.06f)
          +.030f*bump(p.x/.20f,(p.y+.35f)/.075f));
        const float cheek=bump((std::abs(p.x)-.26f)/.15f,(p.y+.10f)/.13f)*front;
        const float beard=bump(p.x/.30f,(p.y+.34f)/.17f)*front;
        v.color={1-.08f*beard,1-.055f*cheek-.11f*beard,1-.055f*cheek-.13f*beard,1};
      }
    }
    Normals(g); return g;
  }
  if (mesh==Mesh::Boot || mesh==Mesh::BootLow) {
    auto g=Lathe({{-.5f,.37f,.44f},{-.38f,.44f,.49f},{-.18f,.43f,.47f},{.02f,.36f,.37f},
      {.19f,.28f,.24f},{.5f,.27f,.24f}},mesh==Mesh::Boot?24:12);
    // Flat sole and extended toe; the upper rows form an ankle instead of a ball.
    for (auto& v : g.vertices) v.position.z+=.12f*(.5f-v.position.y);
    Normals(g); return g;
  }
  if (mesh==Mesh::Sphere || mesh==Mesh::SphereLow) return Lathe(SphereProfile(),mesh==Mesh::Sphere?24:12);
  if (mesh==Mesh::Palm) return Lathe({{-.5f,.28f,.30f},{-.36f,.35f,.40f},{-.12f,.47f,.50f},
    {.20f,.50f,.43f},{.42f,.48f,.32f},{.5f,.40f,.23f}},12);
  if (mesh==Mesh::Finger) return Lathe({{-.5f,.35f,.33f},{-.35f,.50f,.47f},{.20f,.46f,.43f},
    {.40f,.42f,.40f},{.5f,.30f,.28f}},8);
  if (mesh==Mesh::Feature) return Lathe({{-.5f,.16f,.12f},{-.35f,.38f,.31f},{0,.5f,.5f},
    {.35f,.38f,.31f},{.5f,.16f,.12f}},12);
  if (mesh==Mesh::Rock) return Lathe(SphereProfile(),12,.24f,.05f);
  if (mesh==Mesh::Trunk) return Lathe({{-.5f,.5f,.5f},{-.4f,.38f,.39f},{0,.25f,.28f},{.4f,.19f,.22f},{.5f,.12f,.14f}},10,.12f,.14f);
  if (mesh==Mesh::Torso || mesh==Mesh::TorsoLow) return Lathe({{-.5f,.39f,.4f},{-.38f,.37f,.38f},{-.26f,.35f,.36f},
    {-.13f,.36f,.38f},{-.05f,.37f,.4f},{.08f,.45f,.44f},{.22f,.5f,.46f},
    {.34f,.47f,.4f},{.43f,.35f,.32f},{.5f,.22f,.25f}},mesh==Mesh::Torso?24:12);
  if (mesh==Mesh::Limb || mesh==Mesh::LimbLow) return Lathe({{-.5f,.24f,.24f},{-.40f,.36f,.36f},{-.25f,.38f,.39f},{-.1f,.4f,.4f},
    {.08f,.44f,.45f},{.25f,.46f,.46f},{.42f,.42f,.42f},{.5f,.25f,.25f}},mesh==Mesh::Limb?20:10);
  if (mesh==Mesh::Blade) return compiled_mesh::Blade();
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
      // Warp fine grain with periodic, elongated growth variation. Keep the
      // relief shallow so timber reads as wood rather than corrugated metal.
      const float grain=.5f+.5f*std::sin(2*pi*(u*18+.75f*noise(u*3,v,4)+.15f*noise(u,v,8)));
      const float plank=u*4-std::floor(u*4);
      const float edge=smooth(std::min(plank,1-plank)/.055f);
      const float growth=noise(u,v,8);
      value=(.72f+.10f*grain+.12f*broad)*(.72f+.28f*edge);
      height=.0025f*grain+.009f*edge+.002f*growth; roughness=.76f+.16f*growth; break;
    }
    case Material::Stone: {
      // Mineral strata and small pits, periodic in both axes for triplanar wraps.
      const float strata=.5f+.5f*std::sin(2*pi*(v*5+.35f*noise(u,v,4)));
      const float pit=smooth((fine-.63f)*7);
      value=.65f+.22f*broad+.08f*fine+.045f*strata-.09f*pit;
      height=.022f*broad+.004f*strata-.007f*pit;
      roughness=.78f+.18f*fine; break;
    }
    case Material::Ground: {
      // Packed soil grains and shallow gravel, baked into the existing mip chain.
      const float grain=noise(u,v,48),gravel=smooth((noise(u,v,16)-.56f)*6);
      value=.72f+.16f*noise(u,v,10)+.08f*fine+.06f*grain+.06f*gravel;
      height=.005f*noise(u,v,16)+.002f*grain+.006f*gravel;
      roughness=.86f+.10f*fine-.07f*gravel; break;
    }
    case Material::Cloth: {
      // Uneven warp/weft and diagonal twill; periodic and mip-filtered by the
      // existing material array. No extra samples or resident texture layers.
      const float warp=std::sin(u*2*pi*24),weft=std::sin(v*2*pi*24);
      const float weave=warp*weft,twill=std::sin(2*pi*(u*12-v*12));
      const float fold=std::sin(2*pi*(u*3+.18f*noise(u,v,4)));
      value=.86f+.035f*weave+.018f*twill+.035f*fold+.065f*broad+.015f*fine;
      height=.0006f*weave+.0003f*twill+.0015f*fold;
      roughness=.86f+.08f*broad+.035f*(.5f+.5f*weave); break;
    }
    case Material::Roof: {
      const float row=std::floor(v*6),ty=v*6-row;
      const float tx=u*4+row*.5f-std::floor(u*4+row*.5f);
      const float edge=smooth(std::min(tx,1-tx)/.09f)*smooth(ty/.12f);
      const int column=int(std::floor(u*4+row*.5f));
      const float tileShade=Hash(column%4,int(row));
      value=(.56f+edge*(.26f+.10f*ty)+.04f*fine)*(.84f+.20f*tileShade);
      height=.032f*edge+.012f*ty; roughness=.72f+.16f*fine; break;
    }
    case Material::Plaster:
      value=.87f+.10f*broad+.03f*fine; height=.004f*fine; roughness=.94f; break;
    case Material::Metal: {
      const float scratch=noise(u,v*4,32);
      const float tarnish=smooth((noise(u,v,8)-.45f)*3);
      const float brushed=.5f+.5f*std::sin(2*pi*(u*48+.12f*noise(u,v,8)));
      const float pits=smooth((.24f-noise(u,v,32))*6);
      value=.84f+.10f*broad+.035f*scratch+.015f*brushed-.06f*tarnish-.025f*pits;
      height=.0003f*scratch+.00018f*brushed-.0006f*pits;
      roughness=.27f+.15f*broad+.08f*scratch+.12f*tarnish+.06f*pits; break;
    }
    case Material::Hair: {
      // Swept strands, not the cloth weave previously used by the hair cap.
      const float strand=.5f+.5f*std::sin(2*pi*(u*20+.18f*std::sin(v*2*pi)));
      value=.77f+.11f*broad+.10f*strand;
      height=.0005f*strand;roughness=.52f+.15f*broad;break;
    }
    case Material::Leather: {
      // Broad irregular grain and shallow creases, rather than woven cloth.
      const float grain=noise(u,v,40),creases=smooth((.32f-noise(u,v,12))*5);
      const float wear=smooth((noise(u,v,8)-.52f)*3)*(1-creases);
      value=.78f+.13f*broad+.07f*grain-.055f*creases+.025f*wear;
      height=.0008f*grain-.0014f*creases;
      roughness=.57f+.17f*broad+.08f*creases-.14f*wear;break;
    }
    case Material::Skin:
      // Low-contrast pores avoid the plastic uniform sheen without baking
      // lighting into skin or changing the selected complexion.
      value=.96f+.025f*broad+.015f*fine;
      height=.00015f*fine;roughness=.61f+.09f*broad+.025f*fine;break;
    case Material::Leaves: {
      // Fine cuticle grain and wax variation remain in the existing mipmapped
      // layer; avoid a repeated painted vein that would cross leaf boundaries.
      const float grain=noise(u,v,32);
      value=.88f+.09f*broad+.03f*grain;
      height=.00025f*grain;
      roughness=.65f+.18f*broad+.06f*fine;break;
    }
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
