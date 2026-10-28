#include "engine/PlayerClips.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace playerclips {
static_assert(sizeof(Frame)==76,"Keep player clip export layout synchronized");
namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Reader {
 std::string_view bytes;std::size_t offset=0;
 template<class T>T Get(){Check(sizeof(T)<=bytes.size()-offset,"Truncated player clips");T v;std::memcpy(&v,bytes.data()+offset,sizeof(T));offset+=sizeof(T);return v;}
};
}
Library Decode(std::string_view bytes){
 Check(bytes.size()<=128*1024,"Player clip budget exceeded");Reader r{bytes};
 Check(r.Get<unsigned>()==0x314c4350&&r.Get<unsigned>()==1&&r.Get<unsigned>()==unsigned(Clip::Count),"Invalid player clip header");Library result;
 for(auto& track:result){
  auto count=r.Get<unsigned>();track.duration=r.Get<float>();
  Check(count>=2&&count<=301&&std::isfinite(track.duration)&&track.duration>0&&std::abs(track.duration-(count-1)/30.f)<1e-5f,"Invalid player clip timing");
  for(unsigned i=0;i<count;++i){
   auto f=r.Get<Frame>();std::array<float,19> values;std::memcpy(values.data(),&f,sizeof(f));
   for(float v:values)Check(std::isfinite(v)&&std::abs(v)<=3.2f,"Invalid player clip channel");
   Check(std::abs(f.weapon.x*f.weapon.x+f.weapon.y*f.weapon.y+f.weapon.z*f.weapon.z-1)<1e-4f,"Invalid player weapon direction");track.frames.push_back(f);
  }
 }
 for(unsigned i=unsigned(Clip::WalkForward);i<=unsigned(Clip::RunRight);++i) {
  std::array<float,19> a,b;std::memcpy(a.data(),&result[i].frames.front(),sizeof(Frame));std::memcpy(b.data(),&result[i].frames.back(),sizeof(Frame));
  for(unsigned k=0;k<a.size();++k)Check(std::abs(a[k]-b[k])<.0001f,"Discontinuous player gait loop");
 }
 Check(r.offset==bytes.size(),"Trailing player clip data");return result;
}
std::string_view EmbeddedBytes(){auto module=GetModuleHandleW(nullptr);auto resource=FindResourceW(module,MAKEINTRESOURCEW(303),MAKEINTRESOURCEW(10));Check(resource!=nullptr,"Missing player Blender clips");auto handle=LoadResource(module,resource);auto data=handle?LockResource(handle):nullptr;Check(data!=nullptr,"Cannot load player clips");return {static_cast<const char*>(data),SizeofResource(module,resource)};}
const Library& Collection(){static const auto clips=Decode(EmbeddedBytes());return clips;}
bool Looping(Clip clip){return clip>=Clip::WalkForward&&clip<=Clip::RunRight;}
Frame Sample(Clip clip,float phase){
 Check(unsigned(clip)<unsigned(Clip::Count),"Invalid player clip selection");const auto& frames=Collection()[unsigned(clip)].frames;
 phase=std::isfinite(phase)?phase:0.f;
 const bool loop=Looping(clip);
 const float p=(loop?phase-std::floor(phase):std::clamp(phase,0.f,1.f))*(frames.size()-1);unsigned i=std::min(unsigned(p),unsigned(frames.size()-2));float t=p-i;
 std::array<float,19>a,b,c;std::memcpy(a.data(),&frames[i],sizeof(Frame));std::memcpy(b.data(),&frames[i+1],sizeof(Frame));
 if(loop) {
  // Periodic Catmull-Rom preserves velocity at authored keys and wrap-around.
  std::array<float,19> left,right;
  std::memcpy(left.data(),&frames[i?i-1:frames.size()-2],sizeof(Frame));
  std::memcpy(right.data(),&frames[i+2<frames.size()?i+2:1],sizeof(Frame));
  const float t2=t*t,t3=t2*t;
  for(unsigned k=0;k<c.size();++k)c[k]=a[k]*(2*t3-3*t2+1)+(b[k]-left[k])*.5f*(t3-2*t2+t)+b[k]*(-2*t3+3*t2)+(right[k]-a[k])*.5f*(t3-t2);
 } else for(unsigned k=0;k<c.size();++k)c[k]=a[k]+(b[k]-a[k])*t;
 Frame result;std::memcpy(&result,c.data(),sizeof(Frame));using namespace DirectX;XMStoreFloat3(&result.weapon,XMVector3Normalize(XMLoadFloat3(&result.weapon)));return result;
}
Frame Locomotion(const std::array<float,4>& weights,float run,float phase){
 run=std::clamp(std::isfinite(run)?run:0.f,0.f,1.f);
 std::array<float,19> sum{};
 float total=0;
 for(unsigned d=0;d<4;++d) {
  const float weight=std::clamp(std::isfinite(weights[d])?weights[d]:0.f,0.f,1.f);
  if(weight<.00001f)continue;
  total+=weight;
  for(unsigned pace=0;pace<2;++pace) {
   const float w=weight*(pace?run:1-run);if(w<.00001f)continue;
   const auto frame=Sample(Clip(unsigned(Clip::WalkForward)+pace*4+d),phase);
   std::array<float,19> v;std::memcpy(v.data(),&frame,sizeof(Frame));
   for(unsigned k=0;k<v.size();++k)sum[k]+=v[k]*w;
  }
 }
 for(auto& v:sum)v/=std::max(1.f,total);
 Frame result;std::memcpy(&result,sum.data(),sizeof(Frame));return result;
}
float SwingHeight(const std::array<float,4>& weights,float run,int side,float phase){
 if(side<0||side>1||!std::isfinite(phase))return 0;
 const float t=std::clamp(phase,0.f,1.f),p=(side?.5f:0.f)+t*.5f;
 const auto frame=Locomotion(weights,run,p);
 // Source run starts with a small flight offset; contact ownership is local.
 // Remove that offset and ease both endpoints so a locked sole never pops.
 const auto a=Locomotion(weights,run,side?.5f:0.f),b=Locomotion(weights,run,side?1.f:.5f);
 const float h=frame.feet[side].y-(a.feet[side].y*(1-t)+b.feet[side].y*t);
 const float edge=std::clamp(std::min(t,1-t)*10.f,0.f,1.f);
 return std::clamp(h,0.f,.145f)*edge*edge*(3-2*edge);
}
float AttackPhase(float seconds,float start,float contact,float end){
 if(!std::isfinite(seconds)||seconds<0)return 0;
 Check(start>0&&contact>start&&end>contact,"Invalid player attack retiming");
 // Blender marker .55 is retimed to the authoritative contact, never vice versa.
 if(seconds<start)return .34f*seconds/start;
 if(seconds<contact)return .34f+.21f*(seconds-start)/(contact-start);
 return std::min(1.f,.55f+.45f*(seconds-contact)/(end-contact));
}
}
