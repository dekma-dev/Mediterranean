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
 Check(r.Get<unsigned>()==0x314c4350&&r.Get<unsigned>()==1&&r.Get<unsigned>()==4,"Invalid player clip header");Library result;
 for(auto& track:result){
  auto count=r.Get<unsigned>();track.duration=r.Get<float>();
  Check(count>=2&&count<=301&&std::isfinite(track.duration)&&track.duration>0&&std::abs(track.duration-(count-1)/30.f)<1e-5f,"Invalid player clip timing");
  for(unsigned i=0;i<count;++i){
   auto f=r.Get<Frame>();std::array<float,19> values;std::memcpy(values.data(),&f,sizeof(f));
   for(float v:values)Check(std::isfinite(v)&&std::abs(v)<=3.2f,"Invalid player clip channel");
   Check(std::abs(f.weapon.x*f.weapon.x+f.weapon.y*f.weapon.y+f.weapon.z*f.weapon.z-1)<1e-4f,"Invalid player weapon direction");track.frames.push_back(f);
  }
 }
 Check(r.offset==bytes.size(),"Trailing player clip data");return result;
}
std::string_view EmbeddedBytes(){auto module=GetModuleHandleW(nullptr);auto resource=FindResourceW(module,MAKEINTRESOURCEW(303),MAKEINTRESOURCEW(10));Check(resource!=nullptr,"Missing player Blender clips");auto handle=LoadResource(module,resource);auto data=handle?LockResource(handle):nullptr;Check(data!=nullptr,"Cannot load player clips");return {static_cast<const char*>(data),SizeofResource(module,resource)};}
const Library& Collection(){static const auto clips=Decode(EmbeddedBytes());return clips;}
Frame Sample(Clip clip,float phase){
 Check(unsigned(clip)<unsigned(Clip::Count),"Invalid player clip selection");const auto& frames=Collection()[unsigned(clip)].frames;
 const float p=std::clamp(std::isfinite(phase)?phase:0.f,0.f,1.f)*(frames.size()-1);unsigned i=std::min(unsigned(p),unsigned(frames.size()-2));float t=p-i;
 std::array<float,19>a,b,c;std::memcpy(a.data(),&frames[i],sizeof(Frame));std::memcpy(b.data(),&frames[i+1],sizeof(Frame));
 for(unsigned k=0;k<c.size();++k)c[k]=a[k]+(b[k]-a[k])*t;
 Frame result;std::memcpy(&result,c.data(),sizeof(Frame));using namespace DirectX;XMStoreFloat3(&result.weapon,XMVector3Normalize(XMLoadFloat3(&result.weapon)));return result;
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
