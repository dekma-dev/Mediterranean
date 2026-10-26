#include "engine/AuthoredAssets.h"
#include "engine/WolfAnimation.h"
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace authored {
visual::SkinInfluence Influence(Asset asset,DirectX::XMFLOAT3 p) {
 if(asset==Asset::Wolf)return wolf::Influence(p);
 // Smooth spatial envelopes also work on the decimated meshes. The archive
 // remains an editable static source; these are presentation rigs, not gameplay.
 unsigned bone=0;float weight=0;
 if(asset==Asset::Wolf || asset==Asset::Boar) {
  if(p.y<.8f) {bone=2+(p.x>0?1u:0u)+(p.z<0?2u:0u);weight=std::clamp((.85f-p.y)/.3f,0.f,1.f);}
  else if(p.z>.45f) {bone=1;weight=std::clamp((p.z-.45f)/.35f,0.f,1.f);}
 } else {
  const float width=asset==Asset::Orc?1.22f:1.f;
  if(p.y<.95f && std::abs(p.x)<.4f*width) {
   bone=p.x<0?2:4;weight=std::clamp((.99f-p.y)/.18f,0.f,1.f);
  } else if(std::abs(p.x)>.32f*width) {
   bone=p.x<0?6:8;weight=std::clamp((std::abs(p.x)/width-.32f)/.15f,0.f,1.f);
  } else {bone=1;weight=std::clamp((p.y-.95f)/.4f,0.f,1.f);}
 }
 return {{bone,0,0,0},{weight,1-weight,0,0}};
}
visual::BodyPalette Pose(Asset asset,float time,float gait,float attack,float travel,Gesture gesture) {
 if(asset==Asset::Wolf)return wolf::Pose(time,gait,travel);
 using namespace DirectX;
 visual::BodyPalette palette{};
 const auto finite=[](float v) {return std::isfinite(v)?v:0.f;};
 time=finite(time);gait=finite(gait);attack=std::clamp(finite(attack),-1.f,1.f);travel=std::clamp(finite(travel),0.f,1.f);
 const float breath=std::sin(time*1.8f)*.012f,step=std::sin(gait)*.38f*travel;
 const bool beast=asset==Asset::Wolf||asset==Asset::Boar;
 const float collapse=beast?0.f:std::clamp(finite(gesture.collapse),0.f,1.f);
 const auto fallRotation=XMMatrixRotationX(collapse*DirectX::XM_PIDIV2);
 // At rest the model's forward-most point becomes its lowest point.
 const float floorHeight=collapse>0?Collection()[unsigned(asset)*lodCount].bounds.maximum.z:0.f;
 const auto fall=fallRotation*XMMatrixTranslation(0,std::max(0.f,floorHeight)*collapse*collapse+.025f*collapse,0);
 const auto bounded=[&](float v,float limit) {return std::clamp(finite(v),-limit,limit);};
 const auto torsoRotation=XMMatrixRotationRollPitchYaw(bounded(gesture.pitch,.3f),bounded(gesture.yaw,.4f),bounded(gesture.roll,.25f));
 const auto torso=XMMatrixTranslation(0,-.96f,0)*torsoRotation*
   XMMatrixTranslation(0,.96f+bounded(gesture.hip,.08f),0);
 for(unsigned i=0;i<visual::bodyBones;++i) {
  XMFLOAT3 pivot{};float angle=0;
  if(beast) {
   if(i==1) {pivot={0,1,.45f};angle=breath*2;}
   if(i>=2&&i<=5) {pivot={(i%2? .26f:-.26f),.8f,i<4?.5f:-.5f};angle=step*((i==2||i==5)?1.f:-1.f);}
  } else {
   if(i==1) {pivot={0,.96f,0};angle=breath;}
   if(i==2||i==4) {pivot={i==2?-.2f:.2f,.96f,0};angle=i==2?step:-step;}
   if(i==6||i==8) {pivot={(i==6?-.4f:.4f)*(asset==Asset::Orc?1.22f:1.f),1.69f,0};angle=(i==6?-step:step)*.55f+breath+(i==8?-attack*.9f:0);}
  }
  float open=0;
  if(!beast && (i==6 || i==8)) {
   angle-=bounded(i==6?gesture.leftLift:gesture.rightLift,1.05f);
   open=bounded(i==6?gesture.leftOpen:gesture.rightOpen,.5f)*(i==6?-1.f:1.f);
  }
  auto rotation=XMMatrixRotationX(angle)*XMMatrixRotationZ(open);
  auto transform=XMMatrixTranslation(-pivot.x,-pivot.y,-pivot.z)*rotation*XMMatrixTranslation(pivot.x,pivot.y,pivot.z);
  // Shoulder transforms inherit torso rotation, keeping arm roots attached.
  if(!beast && (i==1 || i==6 || i==8)) {transform*=torso;rotation*=torsoRotation;}
  if(collapse>0) {transform*=fall;rotation*=fallRotation;}
  XMStoreFloat4x4(&palette.joints[i],transform);
  XMStoreFloat4x4(&palette.normals[i],rotation);
 }
 return palette;
}
static_assert(unsigned(visual::Material::Count)==13,"Update authored shader material sentinel with material layers");
static_assert(sizeof(visual::Vertex)==40,"Authored archive stores ten float32 values per vertex");
namespace {
void Require(bool condition,const char* message) {if(!condition) throw std::runtime_error(message);}
class Reader {
public:
  explicit Reader(std::string_view source):bytes(source) {}
  template<class T> T Read() {
    Require(sizeof(T)<=bytes.size()-offset,"Truncated authored asset archive");
    T value;std::memcpy(&value,bytes.data()+offset,sizeof(T));offset+=sizeof(T);return value;
  }
  std::size_t Remaining() const {return bytes.size()-offset;}
private:
  std::string_view bytes;std::size_t offset=0;
};
bool Finite(DirectX::XMFLOAT3 v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
}
Library Decode(std::string_view bytes) {
  Require(bytes.size()<=archiveBudget,"Authored asset archive exceeds 32 MiB");Reader read(bytes);
  Require(read.Read<std::uint32_t>()==0x3141444d,"Invalid authored asset signature");
  Require(read.Read<std::uint32_t>()==1,"Unsupported authored asset version");
  Require(read.Read<std::uint32_t>()==assetCount && read.Read<std::uint32_t>()==lodCount,"Unexpected authored asset/LOD count");
  Library result;
  for(auto& entry:result) {
    const auto vertices=read.Read<std::uint32_t>(),indices=read.Read<std::uint32_t>();
    Require(vertices>0&&vertices<=65536&&indices>=3&&indices<=60000&&indices%3==0,"Authored mesh exceeds vertex/triangle budget");
    entry.bounds.minimum=read.Read<DirectX::XMFLOAT3>();entry.bounds.maximum=read.Read<DirectX::XMFLOAT3>();
    const auto low=entry.bounds.minimum,high=entry.bounds.maximum;
    Require(Finite(low)&&Finite(high)&&low.x<=high.x&&low.y<=high.y&&low.z<=high.z,"Invalid authored mesh bounds");
    Require(std::size_t(vertices)*sizeof(visual::Vertex)+std::size_t(indices)*4<=read.Remaining(),"Truncated authored mesh data");
    auto& g=entry.geometry;g.vertices.reserve(vertices);g.indices.reserve(indices);
    for(unsigned i=0;i<vertices;++i) {
      auto v=read.Read<visual::Vertex>();Require(Finite(v.position)&&Finite(v.normal),"Non-finite authored vertex");
      const auto p=v.position;const float n=v.normal.x*v.normal.x+v.normal.y*v.normal.y+v.normal.z*v.normal.z;
      Require(std::abs(p.x)<=32&&std::abs(p.y)<=32&&std::abs(p.z)<=32&&std::abs(n-1)<.005f,"Invalid authored position/normal");
      Require(p.x>=low.x-.00001f&&p.x<=high.x+.00001f&&p.y>=low.y-.00001f&&p.y<=high.y+.00001f&&p.z>=low.z-.00001f&&p.z<=high.z+.00001f,"Authored bounds exclude a vertex");
      const auto c=v.color;
      Require(std::isfinite(c.x)&&std::isfinite(c.y)&&std::isfinite(c.z)&&std::isfinite(c.w)&&c.x>=0&&c.x<=1&&c.y>=0&&c.y<=1&&c.z>=0&&c.z<=1&&c.w>=0&&c.w<unsigned(visual::Material::Count)&&std::floor(c.w)==c.w,"Invalid authored color/material");
      g.vertices.push_back(v);
    }
    for(unsigned i=0;i<indices;++i) {const auto index=read.Read<std::uint32_t>();Require(index<vertices,"Authored index out of range");g.indices.push_back(index);}
    for(unsigned i=0;i<indices;i+=3) {
      const auto& a=g.vertices[g.indices[i]];const auto& b=g.vertices[g.indices[i+1]];const auto& c=g.vertices[g.indices[i+2]];
      Require(a.color.w==b.color.w&&b.color.w==c.color.w,"Authored triangle crosses material boundary");
      using namespace DirectX;
      const auto cross=XMVector3Cross(XMLoadFloat3(&b.position)-XMLoadFloat3(&a.position),XMLoadFloat3(&c.position)-XMLoadFloat3(&a.position));
      Require(XMVectorGetX(XMVector3LengthSq(cross))>1e-18f,"Degenerate authored triangle");
      Require(XMVectorGetX(XMVector3Dot(cross,XMLoadFloat3(&a.normal)+XMLoadFloat3(&b.normal)+XMLoadFloat3(&c.normal)))>0,"Authored normals oppose winding");
    }
  }
  Require(read.Remaining()==0,"Trailing authored asset data");
  for(unsigned a=0;a<assetCount;++a) for(unsigned lod=1;lod<lodCount;++lod)
    Require(result[a*lodCount+lod].geometry.indices.size()<result[a*lodCount+lod-1].geometry.indices.size(),"Authored LOD does not reduce geometry");
  return result;
}
std::string_view EmbeddedBytes() {
  const auto module=GetModuleHandleW(nullptr);const auto resource=FindResourceW(module,MAKEINTRESOURCEW(301),RT_RCDATA);
  Require(resource!=nullptr,"Missing embedded Blender collection");
  const auto handle=LoadResource(module,resource);const auto data=handle?LockResource(handle):nullptr;
  Require(data!=nullptr,"Cannot load embedded Blender collection");
  return {static_cast<const char*>(data),SizeofResource(module,resource)};
}
const Library& Collection() {static const auto data=Decode(EmbeddedBytes());return data;}
const char* Name(Asset asset) {
  static constexpr const char* names[]={"Dawnwarden","Moonweaver","Ironjaw","Oathless","Ash sentry","Frostfang","Thornback","Elder oak","Highland pine","Blighted elder","Hearth cottage","Moon inn","Border watch"};
  return unsigned(asset)<assetCount?names[unsigned(asset)]:"Unknown asset";
}
unsigned SelectLod(float projectedHeight) {return projectedHeight>=210?0:projectedHeight>=85?1:2;}
}
