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
 Check(bytes.size()<=512*1024,"Player clip budget exceeded");Reader r{bytes};
 Check(r.Get<unsigned>()==0x324c4350&&r.Get<unsigned>()==2&&r.Get<unsigned>()==unsigned(Clip::Count),"Invalid player clip header");Library result;
 for(auto& track:result){
  auto count=r.Get<unsigned>();track.duration=r.Get<float>();
  track.load=r.Get<float>();track.contact=r.Get<float>();track.takeoff=r.Get<float>();track.landing=r.Get<float>();track.stance=r.Get<float>();track.speed=r.Get<float>();
  auto flags=r.Get<unsigned>();Check(flags<=1,"Invalid player clip flags");track.loop=flags!=0;
  Check(count>=2&&count<=601&&std::isfinite(track.duration)&&track.duration>0&&track.duration<=10,"Invalid player clip timing");
  for(float v:{track.load,track.contact,track.takeoff,track.landing,track.stance})Check(std::isfinite(v)&&v>=0&&v<=1,"Invalid clip marker");
  Check(std::isfinite(track.speed)&&track.speed>=0&&track.speed<=10,"Invalid clip travel speed");
  for(unsigned i=0;i<count;++i){
   auto f=r.Get<Frame>();std::array<float,19> values;std::memcpy(values.data(),&f,sizeof(f));
   for(float v:values)Check(std::isfinite(v)&&std::abs(v)<=3.2f,"Invalid player clip channel");
   Check(std::abs(f.weapon.x*f.weapon.x+f.weapon.y*f.weapon.y+f.weapon.z*f.weapon.z-1)<1e-4f,"Invalid player weapon direction");track.frames.push_back(f);
  }
 }
 for(unsigned i=0;i<result.size();++i) if(result[i].loop) {
  std::array<float,19> a,b;std::memcpy(a.data(),&result[i].frames.front(),sizeof(Frame));std::memcpy(b.data(),&result[i].frames.back(),sizeof(Frame));
  for(unsigned k=0;k<a.size();++k)Check(std::abs(a[k]-b[k])<.0001f,"Discontinuous player gait loop");
 }
 for(auto clip:{Clip::Slash,Clip::Overhead,Clip::StaffSweep,Clip::StaffThrust,Clip::BowRelease,Clip::BowPowerRelease}) {
  auto& t=result[unsigned(clip)];Check(!t.loop&&t.load>0&&t.contact>t.load&&t.contact<1,"Missing attack contact markers");
  if(clip==Clip::BowRelease||clip==Clip::BowPowerRelease)continue;
  // Measure the complete retargeted weapon path before compressing its timing.
  // A uniform source-time scale amplified the source's fastest few keys into
  // an almost invisible cut. Arc-length retiming distributes that travel.
  const auto point=[](const Frame& f,float length) {
   float x=.4f+f.hands[1].x+f.weapon.x*length,y=1.02f+f.hands[1].y+f.weapon.y*length-.95f,z=.10f+f.hands[1].z+f.weapon.z*length;
   const float pitch=f.torso.x,yaw=f.torso.y,roll=f.torso.z;
   const float y1=y*std::cos(pitch)-z*std::sin(pitch),z1=y*std::sin(pitch)+z*std::cos(pitch);
   const float x1=x*std::cos(roll)-y1*std::sin(roll),y2=x*std::sin(roll)+y1*std::cos(roll);
   return DirectX::XMFLOAT3{x1*std::cos(yaw)+z1*std::sin(yaw),y2+f.hip,-x1*std::sin(yaw)+z1*std::cos(yaw)};
  };
  const auto distance=[](auto a,auto b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));};
  t.arc.push_back(0);
  for(std::size_t k=1;k<t.frames.size();++k) {
   const auto& a=t.frames[k-1];const auto& b=t.frames[k];
   t.arc.push_back(t.arc.back()+std::max({distance(point(a,.96f),point(b,.96f)),distance(point(a,0),point(b,0)),distance(a.hands[0],b.hands[0]),.000001f}));
  }
 }
 const auto& jump=result[unsigned(Clip::Jump)];Check(jump.takeoff>0&&jump.landing>jump.takeoff&&jump.landing<1,"Invalid jump flight markers");
 for(auto clip:{Clip::WalkForward,Clip::WalkBackward,Clip::WalkLeft,Clip::WalkRight,Clip::RunForward,Clip::RunBackward,Clip::RunLeft,Clip::RunRight,Clip::SwordWalk,Clip::SwordRun,Clip::StaffWalk,Clip::StaffRun,Clip::BowWalk,Clip::BowRun}) {
  const auto& t=result[unsigned(clip)];Check(t.loop&&t.stance>.1f&&t.stance<.9f&&t.speed>0,"Invalid gait contact metadata");
 }
 Check(r.offset==bytes.size(),"Trailing player clip data");return result;
}
std::string_view EmbeddedBytes(){auto module=GetModuleHandleW(nullptr);auto resource=FindResourceW(module,MAKEINTRESOURCEW(303),MAKEINTRESOURCEW(10));Check(resource!=nullptr,"Missing player Blender clips");auto handle=LoadResource(module,resource);auto data=handle?LockResource(handle):nullptr;Check(data!=nullptr,"Cannot load player clips");return {static_cast<const char*>(data),SizeofResource(module,resource)};}
const Library& Collection(){static const auto clips=Decode(EmbeddedBytes());return clips;}
bool Looping(Clip clip){Check(unsigned(clip)<unsigned(Clip::Count),"Invalid clip selection");return Collection()[unsigned(clip)].loop;}
Frame Sample(Clip clip,float phase){
 Check(unsigned(clip)<unsigned(Clip::Count),"Invalid player clip selection");const auto& frames=Collection()[unsigned(clip)].frames;
 phase=std::isfinite(phase)?phase:0.f;
 const bool loop=Looping(clip);
 const float p=(loop?phase-std::floor(phase):std::clamp(phase,0.f,1.f))*(frames.size()-1);unsigned i=std::min(unsigned(p),unsigned(frames.size()-2));float t=p-i;
 std::array<float,19>a,b,c;std::memcpy(a.data(),&frames[i],sizeof(Frame));std::memcpy(b.data(),&frames[i+1],sizeof(Frame));
 {
  // Periodic Catmull-Rom preserves velocity at authored keys and wrap-around.
  std::array<float,19> left,right;
  std::memcpy(left.data(),&frames[i?i-1:(loop?frames.size()-2:0)],sizeof(Frame));
  std::memcpy(right.data(),&frames[i+2<frames.size()?i+2:(loop?1:frames.size()-1)],sizeof(Frame));
  const float t2=t*t,t3=t2*t;
  for(unsigned k=0;k<c.size();++k)c[k]=a[k]*(2*t3-3*t2+1)+(b[k]-left[k])*.5f*(t3-2*t2+t)+b[k]*(-2*t3+3*t2)+(right[k]-a[k])*.5f*(t3-t2);
 }
 Frame result;std::memcpy(&result,c.data(),sizeof(Frame));using namespace DirectX;XMStoreFloat3(&result.weapon,XMVector3Normalize(XMLoadFloat3(&result.weapon)));return result;
}
Frame Blend(const Frame& a,const Frame& b,float weight) {
 weight=std::clamp(std::isfinite(weight)?weight:0.f,0.f,1.f);
 std::array<float,19> x,y;std::memcpy(x.data(),&a,sizeof(a));std::memcpy(y.data(),&b,sizeof(b));
 for(unsigned i=0;i<x.size();++i)x[i]+=(y[i]-x[i])*weight;
 Frame f;std::memcpy(&f,x.data(),sizeof(f));return f;
}
Frame Locomotion(const std::array<float,4>& weights,float run,float phase,Equipment equipment){
 run=std::clamp(std::isfinite(run)?run:0.f,0.f,1.f);
 std::array<float,19> sum{};
 float total=0;
 for(unsigned d=0;d<4;++d) {
  const float weight=std::clamp(std::isfinite(weights[d])?weights[d]:0.f,0.f,1.f);
  if(weight<.00001f)continue;
  total+=weight;
  for(unsigned pace=0;pace<2;++pace) {
   const float w=weight*(pace?run:1-run);if(w<.00001f)continue;
   auto frame=Sample(Clip(unsigned(Clip::WalkForward)+pace*4+d),phase);
   if(equipment!=Equipment::None) {
    const auto armed=Sample(equipment==Equipment::Staff?(pace?Clip::StaffRun:Clip::StaffWalk):equipment==Equipment::Bow?(pace?Clip::BowRun:Clip::BowWalk):(pace?Clip::SwordRun:Clip::SwordWalk),phase);
    frame.hands=armed.hands;frame.weapon=armed.weapon;
   }
   std::array<float,19> v;std::memcpy(v.data(),&frame,sizeof(Frame));
   for(unsigned k=0;k<v.size();++k)sum[k]+=v[k]*w;
  }
 }
 for(auto& v:sum)v/=std::max(1.f,total);
 Frame result;std::memcpy(&result,sum.data(),sizeof(Frame));return result;
}
float SwingHeight(const std::array<float,4>& weights,float run,int side,float phase){
 if(side<0||side>1||!std::isfinite(phase))return 0;
 const float t=std::clamp(phase,0.f,1.f);float h=0,total=0;
 run=std::clamp(std::isfinite(run)?run:0.f,0.f,1.f);
 for(unsigned d=0;d<4;++d)for(unsigned pace=0;pace<2;++pace) {
  const float w=std::clamp(std::isfinite(weights[d])?weights[d]:0.f,0.f,1.f)*(pace?run:1-run);if(w<.00001f)continue;
  const auto clip=Clip(unsigned(Clip::WalkForward)+pace*4+d);const auto& track=Collection()[unsigned(clip)];
  const float begin=track.stance-side*.5f,end=1-side*.5f;
  const auto a=Sample(clip,begin),b=Sample(clip,end),f=Sample(clip,begin+(end-begin)*t);
  h+=std::max(0.f,f.feet[side].y-a.feet[side].y*(1-t)-b.feet[side].y*t)*w;total+=w;
 }
 h/=std::max(1.f,total);
 const float edge=std::clamp(std::min(t,1-t)*10.f,0.f,1.f);
 return std::clamp(h,0.f,.145f)*edge*edge*(3-2*edge);
}
float AttackPhase(Clip clip,float seconds,float start,float contact,float end){
 if(!std::isfinite(seconds)||seconds<0)return 0;
 Check(std::isfinite(end)&&start>0&&contact>start&&end>contact,"Invalid player attack retiming");
 Check(unsigned(clip)<unsigned(Clip::Count),"Invalid attack clip");const auto& t=Collection()[unsigned(clip)];
 Check(t.contact>t.load&&t.load>0,"Clip has no attack markers");
 if(clip==Clip::BowRelease||clip==Clip::BowPowerRelease) {
  // A held draw has almost no path length. Arc-length retiming skips that
  // pause and snaps the releasing wrist. Preserve source time for archery.
  const std::array<float,4> time={0,start,contact,end},phase={0,t.load,t.contact,1};
  const float a=t.load/start,b=(t.contact-t.load)/(contact-start),c=(1-t.contact)/(end-contact);
  const std::array<float,4> velocity={0,2*a*b/(a+b),2*b*c/(b+c),0};
  const unsigned segment=seconds<start?0:seconds<contact?1:2;
  const float span=time[segment+1]-time[segment],u=std::clamp((seconds-time[segment])/span,0.f,1.f),u2=u*u,u3=u2*u;
  return std::clamp(phase[segment]*(2*u3-3*u2+1)+phase[segment+1]*(-2*u3+3*u2)+
    span*(velocity[segment]*(u3-2*u2+u)+velocity[segment+1]*(u3-u2)),0.f,1.f);
 }
 if(!t.arc.empty()) {
  const auto at=[&](float p) {const float key=p*(t.arc.size()-1);const auto k=std::min(std::size_t(key),t.arc.size()-2);return t.arc[k]+(t.arc[k+1]-t.arc[k])*(key-k);};
  const std::array<float,4> time={0,start,contact,end},length={0,at(t.load),at(t.contact),t.arc.back()};
  const float a=length[1]/start,b=(length[2]-length[1])/(contact-start),c=(length[3]-length[2])/(end-contact);
  const std::array<float,4> velocity={0,2*a*b/(a+b),2*b*c/(b+c),0};
  const unsigned segment=seconds<start?0:seconds<contact?1:2;
  const float span=time[segment+1]-time[segment],u=std::clamp((seconds-time[segment])/span,0.f,1.f),u2=u*u,u3=u2*u;
  const float path=std::clamp(length[segment]*(2*u3-3*u2+1)+length[segment+1]*(-2*u3+3*u2)+
    span*(velocity[segment]*(u3-2*u2+u)+velocity[segment+1]*(u3-u2)),0.f,t.arc.back());
  const auto upper=std::upper_bound(t.arc.begin(),t.arc.end(),path);
  const auto k=std::min(std::size_t(upper-t.arc.begin()-1),t.arc.size()-2);
  return (float(k)+(path-t.arc[k])/(t.arc[k+1]-t.arc[k]))/float(t.arc.size()-1);
 }
 if(seconds<start)return t.load*seconds/start;
 if(seconds<contact)return t.load+(t.contact-t.load)*(seconds-start)/(contact-start);
 return std::min(1.f,t.contact+(1-t.contact)*(seconds-contact)/(end-contact));
}
float FlightPhase(float progress) {
 const auto& t=Collection()[unsigned(Clip::Jump)];
 return t.takeoff+(t.landing-t.takeoff)*std::clamp(std::isfinite(progress)?progress:0.f,0.f,1.f);
}
}
