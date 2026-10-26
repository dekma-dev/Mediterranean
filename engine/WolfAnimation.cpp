#include "engine/WolfAnimation.h"
#include "engine/AuthoredAssets.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>
namespace authored::wolf {
namespace {
void Require(bool ok,const char* text) {if(!ok)throw std::runtime_error(text);}
struct Reader {
 std::string_view bytes;std::size_t offset=0;
 template<class T>T Read(){Require(sizeof(T)<=bytes.size()-offset,"Truncated wolf animation");T result;std::memcpy(&result,bytes.data()+offset,sizeof(T));offset+=sizeof(T);return result;}
};
}
Data Decode(std::string_view bytes) {
 Require(bytes.size()<=4*1024*1024,"Wolf animation exceeds budget");Reader r{bytes};Data result;
 Require(r.Read<unsigned>()==0x31464c57 && r.Read<unsigned>()==1,"Invalid wolf animation header");result.bones=r.Read<unsigned>();
 Require(result.bones==30 && result.bones<=visual::paletteBones && r.Read<unsigned>()==4,"Invalid wolf skeleton/clip count");
 for(auto& lod:result.weights) {
  unsigned count=r.Read<unsigned>();Require(count>0&&count<=65536,"Invalid wolf weight count");lod.reserve(count);
  for(unsigned i=0;i<count;++i) {
   auto w=r.Read<visual::SkinInfluence>();const float values[]={w.weights.x,w.weights.y,w.weights.z,w.weights.w};float sum=0;
   for(unsigned k=0;k<4;++k){Require(w.bones[k]<result.bones&&std::isfinite(values[k])&&values[k]>=0,"Invalid wolf skin weight");sum+=values[k];}
   Require(std::abs(sum-1)<1e-5f,"Unnormalized wolf weights");lod.push_back(w);
  }
 }
 for(auto& clip:result.clips) {
  clip.frames=r.Read<unsigned>();clip.duration=r.Read<float>();clip.distance=r.Read<float>();
  Require(clip.frames>=2&&clip.frames<=301&&std::isfinite(clip.duration)&&clip.duration>0&&clip.duration<=10&&std::abs((clip.frames-1)/30.f-clip.duration)<1e-5f&&std::isfinite(clip.distance)&&clip.distance>=0&&clip.distance<5,"Invalid wolf clip timing");
  clip.samples.reserve(clip.frames*result.bones);
  for(unsigned i=0;i<clip.frames*result.bones;++i) {
   auto s=r.Read<Sample>();auto q=s.rotation;auto p=s.translation;
   Require(std::isfinite(q.x)&&std::isfinite(q.y)&&std::isfinite(q.z)&&std::isfinite(q.w)&&std::abs(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w-1)<1e-4f,"Invalid wolf quaternion");
   Require(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::abs(p.x)<5&&std::abs(p.y)<5&&std::abs(p.z)<5,"Invalid wolf displacement");clip.samples.push_back(s);
  }
  for(unsigned b=0;b<result.bones;++b) {
   const auto& a=clip.samples[b];const auto& z=clip.samples[(clip.frames-1)*result.bones+b];
   float dot=a.rotation.x*z.rotation.x+a.rotation.y*z.rotation.y+a.rotation.z*z.rotation.z+a.rotation.w*z.rotation.w;
   Require(std::abs(std::abs(dot)-1)<1e-4f&&std::abs(a.translation.x-z.translation.x)+std::abs(a.translation.y-z.translation.y)+std::abs(a.translation.z-z.translation.z)<1e-4f,"Wolf loop is discontinuous");
  }
 }
 Require(r.offset==bytes.size(),"Trailing wolf animation data");return result;
}
std::string_view EmbeddedBytes() {
 auto module=GetModuleHandleW(nullptr);auto resource=FindResourceW(module,MAKEINTRESOURCEW(302),RT_RCDATA);Require(resource!=nullptr,"Missing wolf animation resource");auto handle=LoadResource(module,resource);auto p=handle?LockResource(handle):nullptr;Require(p!=nullptr,"Cannot load wolf animation");return {static_cast<const char*>(p),SizeofResource(module,resource)};
}
const Data& Collection() {static const auto data=[] {
 auto d=Decode(EmbeddedBytes());for(unsigned lod=0;lod<3;++lod)Require(d.weights[lod].size()==authored::Collection()[unsigned(Asset::Wolf)*3+lod].geometry.vertices.size(),"Wolf mesh and weights disagree");return d;
}();return data;}
visual::SkinInfluence Influence(DirectX::XMFLOAT3 p) {
 static const auto lookup=[] {std::map<std::array<float,3>,visual::SkinInfluence> m;const auto& d=Collection();for(unsigned l=0;l<3;++l){const auto& v=authored::Collection()[unsigned(Asset::Wolf)*3+l].geometry.vertices;for(unsigned i=0;i<v.size();++i)m.emplace(std::array<float,3>{v[i].position.x,v[i].position.y,v[i].position.z},d.weights[l][i]);}return m;}();
 auto it=lookup.find({p.x,p.y,p.z});return it==lookup.end()?visual::SkinInfluence{{0,0,0,0},{1,0,0,0}}:it->second;
}
visual::BodyPalette Pose(float time,float gait,float travel) {
 using namespace DirectX;const auto& d=Collection();visual::BodyPalette out{};
 time=std::isfinite(time)?time:0;gait=std::isfinite(gait)?gait:0;travel=std::clamp(std::isfinite(travel)?travel:0.f,0.f,1.f);
 for(unsigned b=0;b<visual::paletteBones;++b){XMStoreFloat4x4(&out.joints[b],XMMatrixIdentity());out.normals[b]=out.joints[b];}
 // The public zero pose remains an exact bind pose for static previews/tests.
 if(time==0&&gait==0&&travel==0)return out;
 const float cycle=gait/XM_2PI-std::floor(gait/XM_2PI);
 const auto sample=[&](unsigned c,unsigned b,float phase) {
  const auto& clip=d.clips[c];float f=(phase-std::floor(phase))*(clip.frames-1);unsigned i=unsigned(f);float t=f-i;
  const auto& a=clip.samples[i*d.bones+b];const auto& z=clip.samples[(i+1)*d.bones+b];Sample s;
  XMStoreFloat4(&s.rotation,XMQuaternionSlerp(XMLoadFloat4(&a.rotation),XMLoadFloat4(&z.rotation),t));XMStoreFloat3(&s.translation,XMVectorLerp(XMLoadFloat3(&a.translation),XMLoadFloat3(&z.translation),t));return s;
 };
 const float g=std::clamp((travel-.35f)/.65f*2,0.f,2.f);const unsigned lo=1+std::min(1u,unsigned(g)),hi=std::min(3u,lo+1);const float blend=g-float(lo-1),moving=std::clamp(travel/.18f,0.f,1.f);
 for(unsigned b=0;b<d.bones;++b) {
  auto a=sample(lo,b,cycle),z=sample(hi,b,cycle),idle=sample(0,b,time/d.clips[0].duration);
  auto q=XMQuaternionSlerp(XMLoadFloat4(&a.rotation),XMLoadFloat4(&z.rotation),blend);auto p=XMVectorLerp(XMLoadFloat3(&a.translation),XMLoadFloat3(&z.translation),blend);
  q=XMQuaternionSlerp(XMLoadFloat4(&idle.rotation),q,moving);p=XMVectorLerp(XMLoadFloat3(&idle.translation),p,moving);auto rotation=XMMatrixRotationQuaternion(q);
  XMStoreFloat4x4(&out.joints[b],rotation*XMMatrixTranslationFromVector(p));XMStoreFloat4x4(&out.normals[b],rotation);
 }return out;
}
}
