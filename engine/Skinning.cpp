#include "engine/Skinning.h"
#include <algorithm>
#include <cmath>
#include "engine/PlayerBody.h"
#include <stdexcept>

namespace visual {
using namespace DirectX;
void ValidateBodyGeometry(const BodyGeometry& body) {
  const auto& mesh=body.mesh;
  const auto require=[](bool ok) {if(!ok) throw std::runtime_error("Invalid body preview geometry");};
  require(mesh.vertices.size()>=3 && mesh.vertices.size()<=24576 && body.influences.size()==mesh.vertices.size());
  require(!mesh.indices.empty() && mesh.indices.size()<=98304 && mesh.indices.size()%3==0);
  for(std::size_t i=0;i<mesh.vertices.size();++i) {
    const auto& v=mesh.vertices[i];const auto& skin=body.influences[i];
    const float radius=v.position.x*v.position.x+v.position.y*v.position.y+v.position.z*v.position.z;
    const float normal=v.normal.x*v.normal.x+v.normal.y*v.normal.y+v.normal.z*v.normal.z;
    require(std::isfinite(radius) && radius<=3000000 && std::isfinite(normal) && std::abs(normal-1)<.001f);
    require(std::isfinite(v.color.x) && std::isfinite(v.color.y) && std::isfinite(v.color.z) &&
      std::isfinite(v.color.w) && v.color.w>=0 && v.color.w<bodyRegions && std::floor(v.color.w)==v.color.w);
    const float weights[]={skin.weights.x,skin.weights.y,skin.weights.z,skin.weights.w};float sum=0;
    for(unsigned k=0;k<4;++k) {require(skin.bones[k]<bodyBones && std::isfinite(weights[k]) && weights[k]>=0);sum+=weights[k];}
    require(std::abs(sum-1)<.00001f);
  }
  for(std::size_t i=0;i<mesh.indices.size();i+=3) {
    for(unsigned k=0;k<3;++k) require(mesh.indices[i+k]<mesh.vertices.size());
    const auto a=XMLoadFloat3(&mesh.vertices[mesh.indices[i]].position),b=XMLoadFloat3(&mesh.vertices[mesh.indices[i+1]].position),c=XMLoadFloat3(&mesh.vertices[mesh.indices[i+2]].position);
    require(XMVectorGetX(XMVector3LengthSq(XMVector3Cross(b-a,c-a)))>1e-16f);
  }
}
const std::array<BoneSegment,bodyBones>& BodyBindPose() {
  static const std::array<BoneSegment,bodyBones> bones={{
    {{0,.80f,0},{0,1.02f,0},.22f,.23f}, {{0,1.04f,0},{0,1.43f,0},.23f,.28f},
    {{-.17f,.93f,0},{-.17f,.53f,0},.13f,.095f}, {{-.17f,.53f,0},{-.18f,.17f,0},.095f,.065f},
    {{.17f,.93f,0},{.17f,.53f,0},.13f,.095f}, {{.17f,.53f,0},{.18f,.17f,0},.095f,.065f},
    {{-.305f,1.43f,0},{-.45f,1.19f,-.025f},.115f,.083f}, {{-.45f,1.19f,-.025f},{-.52f,.93f,.02f},.083f,.058f},
    {{.305f,1.43f,0},{.45f,1.19f,-.025f},.115f,.083f}, {{.45f,1.19f,-.025f},{.52f,.93f,.02f},.083f,.058f},
    {{0,.95f,0},{0,1.10f,0},.22f,.23f},{{0,1.10f,0},{0,1.27f,0},.23f,.25f},
    {{-.12f,1.43f,0},{-.305f,1.43f,0},.10f,.10f},{{.12f,1.43f,0},{.305f,1.43f,0},.10f,.10f}
  }};
  return bones;
}
SkinInfluence ArticulateTorso(XMFLOAT3 p,const SkinInfluence& source,bool rigidChest) {
  const auto smooth=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
  std::array<float,bodyBones> weight{};
  const float values[]={source.weights.x,source.weights.y,source.weights.z,source.weights.w};
  const float lower=smooth((p.y-1.04f)/.16f),upper=smooth((p.y-1.20f)/.18f);
  const float shoulder=smooth((std::abs(p.x)-.15f)/.16f)*smooth((p.y-1.28f)/.13f)*(1-smooth((p.y-1.49f)/.10f));
  for(unsigned i=0;i<4;++i){
    const unsigned bone=source.bones[i];const float w=values[i];
    if(bone==1&&!rigidChest){
      weight[p.x<0?12:13]+=w*shoulder;
      const float trunk=w*(1-shoulder);
      weight[10]+=trunk*(1-lower);weight[11]+=trunk*lower*(1-upper);weight[1]+=trunk*lower*upper;
    } else weight[bone]+=w;
  }
  SkinInfluence result;float retained[4]{};
  for(unsigned i=0;i<4;++i){
    const auto found=std::max_element(weight.begin(),weight.end());result.bones[i]=unsigned(found-weight.begin());retained[i]=*found;*found=0;
  }
  const float sum=retained[0]+retained[1]+retained[2]+retained[3];
  result.weights={retained[0]/sum,retained[1]/sum,retained[2]/sum,retained[3]/sum};return result;
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
namespace {
XMVECTOR Rotate(XMVECTOR q,XMVECTOR v) {
  const auto xyz=XMVectorSetW(q,0);
  return v+2*XMVector3Cross(xyz,XMVector3Cross(xyz,v)+XMVectorSplatW(q)*v);
}
XMVECTOR Translation(XMVECTOR q,XMVECTOR dual) {
  return 2*(XMVectorSplatW(q)*XMVectorSetW(dual,0)-XMVectorSplatW(dual)*XMVectorSetW(q,0)+XMVector3Cross(q,dual));
}
}
void PreserveBodyVolume(BodyPalette& palette) {
  palette.deformation={};
  for(unsigned bone=0;bone<bodyBones;++bone){
    XMVECTOR scale,q,t;
    if(!XMMatrixDecompose(&scale,&q,&t,XMLoadFloat4x4(&palette.joints[bone])))return;
    q=XMQuaternionNormalize(q);t=XMVectorSetW(t,0);
    auto dual=(XMVectorSplatW(q)*t+XMVector3Cross(t,q))*.5f;
    dual=XMVectorSetW(dual,-.5f*XMVectorGetX(XMVector3Dot(t,q)));
    XMStoreFloat4(&palette.volume[bone].rotation,q);XMStoreFloat4(&palette.volume[bone].translation,dual);
  }
  palette.deformation.x=1;
}
Vertex SkinVertex(const Vertex& vertex,const SkinInfluence& influence,const BodyPalette& palette) {
  auto p=XMVectorZero(),n=XMVectorZero();
  const float weights[]={influence.weights.x,influence.weights.y,influence.weights.z,influence.weights.w};
  auto real=XMVectorZero(),dual=XMVectorZero(),rigidP=XMVectorZero(),rigidN=XMVectorZero();
  const bool volume=palette.deformation.x>0;
  unsigned reference=0;for(unsigned i=1;i<4;++i)if(weights[i]>weights[reference])reference=i;
  const auto anchor=volume?XMLoadFloat4(&palette.volume[influence.bones[reference]].rotation):XMVectorZero();
  for(unsigned i=0;i<4;++i) {
    p+=XMVector3TransformCoord(XMLoadFloat3(&vertex.position),XMLoadFloat4x4(&palette.joints[influence.bones[i]]))*weights[i];
    n+=XMVector3TransformNormal(XMLoadFloat3(&vertex.normal),XMLoadFloat4x4(&palette.normals[influence.bones[i]]))*weights[i];
    if(volume){
      const auto& dq=palette.volume[influence.bones[i]];
      const auto q=XMLoadFloat4(&dq.rotation),d=XMLoadFloat4(&dq.translation);
      const float sign=XMVectorGetX(XMVector4Dot(q,anchor))<0?-1.f:1.f;
      real+=q*(weights[i]*sign);dual+=d*(weights[i]*sign);
      rigidP+=(Rotate(q,XMLoadFloat3(&vertex.position))+Translation(q,d))*weights[i];
      rigidN+=Rotate(q,XMLoadFloat3(&vertex.normal))*weights[i];
    }
  }
  if(volume){
    const float length=XMVectorGetX(XMVector4Length(real));
    if(length>1e-6f){
      real/=length;dual/=length;
      p+=Rotate(real,XMLoadFloat3(&vertex.position))+Translation(real,dual)-rigidP;
      n+=Rotate(real,XMLoadFloat3(&vertex.normal))-rigidN;
    }
  }
  Vertex result=vertex; XMStoreFloat3(&result.position,p); XMStoreFloat3(&result.normal,XMVector3Normalize(n));
  return result;
}
const BodyGeometry& CharacterBody(unsigned level) {
  return playerbody::Collection()[std::min(level,bodyLodCount-1)];
}
}
