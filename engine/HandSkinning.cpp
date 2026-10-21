#include "engine/HandSkinning.h"
#include "engine/SurfaceMesh.h"
#include <algorithm>
#include <cmath>

namespace visual {
using namespace DirectX;
const std::array<BoneSegment,handBones>& HandBindPose() {
  static const auto bones=[] {
    std::array<BoneSegment,handBones> result{};
    result[0]={{0,-.06f,-.043f},{0,.024f,-.043f},.04f,.04f};
    constexpr float lengths[5][3]={{.041f,.026f,.021f},{.045f,.028f,.023f},{.042f,.026f,.022f},{.033f,.022f,.019f},{.035f,.030f,.024f}};
    const XMFLOAT3 thumb[]={{.76f,.55f,.20f},{.48f,.84f,.20f},{.16f,.97f,.18f}};
    for(unsigned f=0;f<5;++f) {
      XMFLOAT3 from=f==4?XMFLOAT3{.040f,-.040f,-.045f}:XMFLOAT3{.033f-f*.022f,f==3?.014f:.024f,-.045f};
      for(unsigned b=0;b<3;++b) {
        auto direction=f==4?XMVector3Normalize(XMLoadFloat3(&thumb[b])):
          XMVector3TransformNormal(XMVectorSet(0,std::cos(.07f),std::sin(.07f),0),XMMatrixRotationZ((float(f)-1.2f)*.10f));
        XMFLOAT3 to; XMStoreFloat3(&to,XMLoadFloat3(&from)+direction*lengths[f][b]);
        const float radius=(f==3?.0085f:.0097f)*(1-b*.10f);
        result[1+f*3+b]={from,to,radius,radius*.94f}; from=to;
      }
    }
    return result;
  }();
  return bones;
}
XMMATRIX HandBoneFrame(XMFLOAT3 from,XMFLOAT3 to,unsigned finger,bool mirrored) {
  const auto up=XMVector3Normalize(XMLoadFloat3(&to)-XMLoadFloat3(&from));
  // A lateral reference follows the bend plane and avoids a forward-axis pole
  // as the fingers curl through 90 degrees. The thumb uses its opposition axis.
  const auto axis=finger==4?XMVectorSet(0,1,0,0):XMVectorSet(mirrored?-1.f:1.f,0,0,0);
  const auto right=XMVector3Normalize(axis-up*XMVector3Dot(axis,up));
  return {right,up,XMVector3Cross(right,up),XMVectorSetW(XMLoadFloat3(&from),1)};
}
namespace {
float Field(unsigned bone,XMFLOAT3 p) {
  if(!bone) {
    // The old 44 mm palm thickness intersected the shaft despite clear
    // finger joints. Keep the inner surface behind the grip's handle envelope.
    const float x=p.x/.049f,y=(p.y+.022f)/.065f,z=(p.z+.043f)/.014f;
    const float radial=std::sqrt(x*x+y*y+z*z);
    const float gradient=std::sqrt(x*x/(.049f*.049f)+y*y/(.065f*.065f)+z*z/(.014f*.014f));
    // Match capsule distance units; scaling by the smallest radius left
    // inappropriate palm influence far along the fingers.
    return gradient>0?radial*(radial-1)/gradient:-.014f;
  }
  const auto& b=HandBindPose()[bone];
  const auto d=XMLoadFloat3(&b.to)-XMLoadFloat3(&b.from),v=XMLoadFloat3(&p)-XMLoadFloat3(&b.from);
  const float t=std::clamp(XMVectorGetX(XMVector3Dot(v,d))/XMVectorGetX(XMVector3LengthSq(d)),0.f,1.f);
  return XMVectorGetX(XMVector3Length(v-d*t))-(b.radiusFrom+(b.radiusTo-b.radiusFrom)*t);
}
float Surface(XMFLOAT3 p) {
  float distance=Field(0,p);
  for(unsigned i=1;i<handBones;++i) {
    const float other=Field(i,p),h=std::max(.003f-std::abs(distance-other),0.f)/.003f;
    distance=std::min(distance,other)-h*h*.003f*.25f;
  }
  return distance;
}
BodyGeometry BuildHand(unsigned level) {
  BodyGeometry result;
  const float spacing=level==0?.0075f:level==1?.011f:.014f;
  const std::array<int,3> size={int(std::ceil(.20f/spacing))+1,int(std::ceil(.25f/spacing))+1,int(std::ceil(.10f/spacing))+1};
  result.mesh=SurfaceMesh({-.085f,-.10f,-.085f},size,spacing,Surface);
  for(auto& vertex : result.mesh.vertices) {
    const auto p=vertex.position;
    // Never blend different fingers across their narrow gaps. Blend only the
    // nearest finger's three bones and the palm, including the web at its base.
    unsigned nearest=1;
    for(unsigned b=2;b<handBones;++b) if(Field(b,p)<Field(nearest,p)) nearest=b;
    const unsigned first=1+((nearest-1)/3)*3;
    SkinInfluence influence; influence.bones={0,first,first+1,first+2};
    float distances[4],weights[4],sum=0;
    for(unsigned k=0;k<4;++k) distances[k]=Field(influence.bones[k],p);
    const float closest=*std::min_element(distances,distances+4);
    for(unsigned k=0;k<4;++k) { weights[k]=std::exp(-(distances[k]-closest)*600.f); sum+=weights[k]; }
    influence.weights={weights[0]/sum,weights[1]/sum,weights[2]/sum,weights[3]/sum}; result.influences.push_back(influence);
    // Subtle warm knuckles and palm/back variation without extra textures.
    const float warm=.025f*std::exp(-std::pow((p.y-.025f)/.025f,2.f));
    vertex.color={1,1-warm,1-warm*1.5f,1};
  }
  return result;
}
}
const BodyGeometry& CharacterHand(unsigned level,bool mirrored) {
  static const auto hands=[] {
    std::array<BodyGeometry,6> result;
    for(unsigned i=0;i<3;++i) {
      result[i*2]=BuildHand(i); result[i*2+1]=result[i*2];
      auto& mesh=result[i*2+1].mesh;
      for(auto& vertex : mesh.vertices) { vertex.position.x=-vertex.position.x; vertex.normal.x=-vertex.normal.x; }
      for(std::size_t t=0;t<mesh.indices.size();t+=3) std::swap(mesh.indices[t+1],mesh.indices[t+2]);
    }
    return result;
  }();
  return hands[std::min(level,2u)*2+unsigned(mirrored)];
}
Vertex SkinHandVertex(const Vertex& vertex,const SkinInfluence& influence,const HandPalette& palette) {
  auto p=XMVectorZero(),n=XMVectorZero();
  const float weights[]={influence.weights.x,influence.weights.y,influence.weights.z,influence.weights.w};
  for(unsigned k=0;k<4;++k) {
    const auto transform=XMLoadFloat4x4(&palette.joints[influence.bones[k]]);
    p+=XMVector3TransformCoord(XMLoadFloat3(&vertex.position),transform)*weights[k];
    n+=XMVector3TransformNormal(XMLoadFloat3(&vertex.normal),transform)*weights[k];
  }
  auto result=vertex; XMStoreFloat3(&result.position,p); XMStoreFloat3(&result.normal,XMVector3Normalize(n)); return result;
}
}
