#include "engine/Skinning.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace visual {
using namespace DirectX;
const std::array<BoneSegment,bodyBones>& BodyBindPose() {
  static const std::array<BoneSegment,bodyBones> bones={{
    {{0,.80f,0},{0,1.02f,0},.22f,.23f}, {{0,1.04f,0},{0,1.43f,0},.23f,.28f},
    {{-.17f,.93f,0},{-.17f,.53f,0},.13f,.095f}, {{-.17f,.53f,0},{-.18f,.17f,0},.095f,.065f},
    {{.17f,.93f,0},{.17f,.53f,0},.13f,.095f}, {{.17f,.53f,0},{.18f,.17f,0},.095f,.065f},
    {{-.305f,1.43f,0},{-.45f,1.19f,-.025f},.115f,.083f}, {{-.45f,1.19f,-.025f},{-.52f,.93f,.02f},.083f,.058f},
    {{.305f,1.43f,0},{.45f,1.19f,-.025f},.115f,.083f}, {{.45f,1.19f,-.025f},{.52f,.93f,.02f},.083f,.058f}
  }};
  return bones;
}
XMMATRIX BoneFrame(XMFLOAT3 from,XMFLOAT3 to,XMFLOAT3 forward) {
  const auto delta=XMLoadFloat3(&to)-XMLoadFloat3(&from);
  const auto up=XMVector3Normalize(delta);
  auto right=XMVector3Cross(up,XMLoadFloat3(&forward));
  if (XMVectorGetX(XMVector3LengthSq(right))<.000001f)
    right=XMVector3Cross(up,std::abs(XMVectorGetX(up))<.9f?XMVectorSet(1,0,0,0):XMVectorSet(0,1,0,0));
  right=XMVector3Normalize(right);
  return XMMATRIX(right,delta,XMVector3Cross(right,up),XMVectorSet(from.x,from.y,from.z,1));
}
Vertex SkinVertex(const Vertex& vertex,const SkinInfluence& influence,const BodyPalette& palette) {
  auto p=XMVectorZero(),n=XMVectorZero();
  const float weights[]={influence.weights.x,influence.weights.y,influence.weights.z,influence.weights.w};
  for(unsigned i=0;i<4;++i) {
    p+=XMVector3TransformCoord(XMLoadFloat3(&vertex.position),XMLoadFloat4x4(&palette.joints[influence.bones[i]]))*weights[i];
    n+=XMVector3TransformNormal(XMLoadFloat3(&vertex.normal),XMLoadFloat4x4(&palette.normals[influence.bones[i]]))*weights[i];
  }
  Vertex result=vertex; XMStoreFloat3(&result.position,p); XMStoreFloat3(&result.normal,XMVector3Normalize(n));
  return result;
}
namespace {
float Capsule(XMFLOAT3 p,const BoneSegment& b) {
  const auto a=XMLoadFloat3(&b.from),d=XMLoadFloat3(&b.to)-a,v=XMLoadFloat3(&p)-a;
  const float t=std::clamp(XMVectorGetX(XMVector3Dot(v,d))/XMVectorGetX(XMVector3LengthSq(d)),0.f,1.f);
  return XMVectorGetX(XMVector3Length(v-d*t))-(b.radiusFrom+(b.radiusTo-b.radiusFrom)*t);
}
float Ellipsoid(XMFLOAT3 p,XMFLOAT3 center,XMFLOAT3 radius) {
  const float x=(p.x-center.x)/radius.x,y=(p.y-center.y)/radius.y,z=(p.z-center.z)/radius.z;
  return (std::sqrt(x*x+y*y+z*z)-1)*std::min({radius.x,radius.y,radius.z});
}
float Field(unsigned bone,XMFLOAT3 p) {
  if (bone==0) return Ellipsoid(p,{0,.91f,0},{.255f,.18f,.165f});
  if (bone==1) return std::min(Ellipsoid(p,{0,1.235f,0},{.29f,.31f,.17f}),
    Capsule(p,{{0,1.43f,0},{0,1.59f,0},.09f,.072f}));
  return Capsule(p,BodyBindPose()[bone]);
}
float Surface(XMFLOAT3 p) {
  float distance=Field(0,p);
  for(unsigned i=1;i<bodyBones;++i) {
    const float other=Field(i,p),h=std::max(.035f-std::abs(distance-other),0.f)/.035f;
    distance=std::min(distance,other)-h*h*.035f*.25f;
  }
  return distance;
}
BodyGeometry BuildBody(unsigned level) {
  BodyGeometry body;
  // A welded isosurface joins the chest, pelvis and limbs. Edge reuse produces
  // one connected surface, including the underarms and crotch, without spheres
  // covering gaps. No file assets or per-frame topology generation.
  const float spacing=level==0?.035f:level==1?.05f:.07f;
  const int nx=level==0?39:int(std::ceil(1.33f/spacing))+1;
  const int ny=level==0?48:int(std::ceil(1.645f/spacing))+1,nz=level==0?19:int(std::ceil(.63f/spacing))+1;
  const auto id=[=](int x,int y,int z) { return (y*nz+z)*nx+x; };
  std::vector<XMFLOAT3> points(nx*ny*nz); std::vector<float> values(points.size());
  for(int y=0;y<ny;++y) for(int z=0;z<nz;++z) for(int x=0;x<nx;++x) {
    const int i=id(x,y,z); points[i]={-.665f+x*spacing,.035f+y*spacing,-.315f+z*spacing};
    const float distance=Surface(points[i]);
    // Keep crossings away from grid corners: otherwise near-zero field values
    // create sliver triangles that collapse at float/GPU precision. The narrow
    // band adjustment is below 2 mm and preserves every sample's inside sign.
    values[i]=std::copysign(std::max(std::abs(distance),.002f),distance);
  }
  std::unordered_map<std::uint64_t,unsigned> edges;
  const auto edge=[&](int a,int b) {
    if(a>b) std::swap(a,b);
    const std::uint64_t key=(std::uint64_t(a)<<32)|unsigned(b);
    const auto found=edges.find(key); if(found!=edges.end()) return found->second;
    const float t=values[a]/(values[a]-values[b]);
    XMFLOAT3 p; XMStoreFloat3(&p,XMLoadFloat3(&points[a])+(XMLoadFloat3(&points[b])-XMLoadFloat3(&points[a]))*t);
    constexpr float e=.0005f;
    XMFLOAT3 n{Surface({p.x+e,p.y,p.z})-Surface({p.x-e,p.y,p.z}),
      Surface({p.x,p.y+e,p.z})-Surface({p.x,p.y-e,p.z}),Surface({p.x,p.y,p.z+e})-Surface({p.x,p.y,p.z-e})};
    XMStoreFloat3(&n,XMVector3Normalize(XMLoadFloat3(&n)));
    std::array<std::pair<float,unsigned>,bodyBones> ranked;
    for(unsigned i=0;i<bodyBones;++i) ranked[i]={Field(i,p),i};
    std::sort(ranked.begin(),ranked.end());
    SkinInfluence influence; float weights[4],sum=0;
    for(unsigned i=0;i<4;++i) {
      influence.bones[i]=ranked[i].second;
      weights[i]=std::exp(-std::max(0.f,ranked[i].first-ranked[0].first)*85.f); sum+=weights[i];
    }
    influence.weights={weights[0]/sum,weights[1]/sum,weights[2]/sum,weights[3]/sum};
    const unsigned bone=ranked[0].second;
    const unsigned region=bone==0?(p.y>.94f?0:3):bone==1?(p.y>1.50f?5:0):bone<6?(bone%2?4:3):(bone%2?2:1);
    const unsigned index=unsigned(body.mesh.vertices.size());
    body.mesh.vertices.push_back({p,n,{1,1,1,float(region)}}); body.influences.push_back(influence); edges.emplace(key,index);
    return index;
  };
  const auto triangle=[&](unsigned a,unsigned b,unsigned c) {
    const auto& va=body.mesh.vertices[a]; const auto& vb=body.mesh.vertices[b]; const auto& vc=body.mesh.vertices[c];
    const auto cross=XMVector3Cross(XMLoadFloat3(&vb.position)-XMLoadFloat3(&va.position),XMLoadFloat3(&vc.position)-XMLoadFloat3(&va.position));
    if(XMVectorGetX(XMVector3Dot(cross,XMLoadFloat3(&va.normal)+XMLoadFloat3(&vb.normal)+XMLoadFloat3(&vc.normal)))<0) std::swap(b,c);
    body.mesh.indices.insert(body.mesh.indices.end(),{a,b,c});
  };
  constexpr int tetrahedra[6][4]={{0,1,3,7},{0,3,2,7},{0,2,6,7},{0,6,4,7},{0,4,5,7},{0,5,1,7}};
  for(int y=0;y<ny-1;++y) for(int z=0;z<nz-1;++z) for(int x=0;x<nx-1;++x) {
    const int cube[]={id(x,y,z),id(x+1,y,z),id(x,y+1,z),id(x+1,y+1,z),
      id(x,y,z+1),id(x+1,y,z+1),id(x,y+1,z+1),id(x+1,y+1,z+1)};
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
  return body;
}
}
const BodyGeometry& CharacterBody(unsigned level) {
  static const std::array<BodyGeometry,bodyLodCount> bodies={BuildBody(0),BuildBody(1),BuildBody(2)};
  return bodies[std::min(level,bodyLodCount-1)];
}
}
