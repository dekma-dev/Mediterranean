#include "engine/PlayerClips.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace playerclips {
static_assert(sizeof(Frame)==frameChannels*sizeof(float),"Keep player clip export layout synchronized");
namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Reader {
 std::string_view bytes;std::size_t offset=0;
 template<class T>T Get(){Check(sizeof(T)<=bytes.size()-offset,"Truncated player clips");T v;std::memcpy(&v,bytes.data()+offset,sizeof(T));offset+=sizeof(T);return v;}
};
}
Library Decode(std::string_view bytes){
 Check(bytes.size()<=2*1024*1024,"Player clip budget exceeded");Reader r{bytes};
 const auto magic=r.Get<unsigned>(),version=r.Get<unsigned>();
 Check(((magic==0x334c4350&&version==3)||(magic==0x344c4350&&version==4)||(magic==0x354c4350&&version==5))&&r.Get<unsigned>()==unsigned(Clip::Count),"Invalid player clip header");Library result;
 for(auto& track:result){
  auto count=r.Get<unsigned>();track.duration=r.Get<float>();
  track.load=r.Get<float>();track.contact=r.Get<float>();track.takeoff=r.Get<float>();track.landing=r.Get<float>();track.stance=r.Get<float>();track.speed=r.Get<float>();
  auto flags=r.Get<unsigned>();Check(flags<=1,"Invalid player clip flags");track.loop=flags!=0;
  Check(count>=2&&count<=601&&std::isfinite(track.duration)&&track.duration>0&&track.duration<=10,"Invalid player clip timing");
  for(float v:{track.load,track.contact,track.takeoff,track.landing,track.stance})Check(std::isfinite(v)&&v>=0&&v<=1,"Invalid clip marker");
  Check(std::isfinite(track.speed)&&track.speed>=0&&track.speed<=10,"Invalid clip travel speed");
  for(unsigned i=0;i<count;++i){
   Frame f;
   if(version==5)f=r.Get<Frame>();
   else if(version==4){const auto legacy=r.Get<std::array<float,40>>();std::memcpy(&f,legacy.data(),sizeof(legacy));}
   else {const auto legacy=r.Get<std::array<float,30>>();std::memcpy(&f,legacy.data(),sizeof(legacy));}
   std::array<float,frameChannels> values;std::memcpy(values.data(),&f,sizeof(f));
   for(float v:values)Check(std::isfinite(v)&&std::abs(v)<=3.2f,"Invalid player clip channel");
   for(const auto& elbow:f.elbows) {
    const float n=elbow.x*elbow.x+elbow.y*elbow.y+elbow.z*elbow.z;
    Check(n<1e-8f||std::abs(n-1)<1e-4f,"Invalid player elbow plane");
   }
   for(const auto& knee:f.knees) {
    const float n=knee.x*knee.x+knee.y*knee.y+knee.z*knee.z;
    Check(n<1e-8f||std::abs(n-1)<1e-4f,"Invalid player knee plane");
   }
   for(const auto& ankle:f.ankles)Check(std::abs(ankle.x)<=1.5f&&std::abs(ankle.y)<=1.5f,"Invalid player ankle rotation");
   Check(f.articulated==0||f.articulated==1,"Invalid articulated source flag");
   for(const auto& shoulder:f.shoulders)Check(std::hypot(shoulder.x,shoulder.y,shoulder.z)<.12f,"Invalid clavicle translation");
   Check(std::abs(f.weapon.x*f.weapon.x+f.weapon.y*f.weapon.y+f.weapon.z*f.weapon.z-1)<1e-4f,"Invalid player weapon direction");track.frames.push_back(f);
  }
 }
 for(unsigned i=0;i<result.size();++i) if(result[i].loop) {
  std::array<float,frameChannels> a,b;std::memcpy(a.data(),&result[i].frames.front(),sizeof(Frame));std::memcpy(b.data(),&result[i].frames.back(),sizeof(Frame));
  for(unsigned k=0;k<a.size();++k)Check(std::abs(a[k]-b[k])<.0001f,"Discontinuous player gait loop");
 }
 for(auto clip:{Clip::Slash,Clip::Overhead,Clip::StaffSweep,Clip::StaffThrust,Clip::BowRelease,Clip::BowPowerRelease}) {
  auto& t=result[unsigned(clip)];Check(!t.loop&&t.load>0&&t.contact>t.load&&t.contact<1,"Missing attack contact markers");

 }
 const auto& jump=result[unsigned(Clip::Jump)];Check(jump.takeoff>0&&jump.landing>jump.takeoff&&jump.landing<1,"Invalid jump flight markers");
 for(auto clip:{Clip::WalkForward,Clip::WalkBackward,Clip::WalkLeft,Clip::WalkRight,Clip::RunForward,Clip::RunBackward,Clip::RunLeft,Clip::RunRight,Clip::SwordWalk,Clip::SwordRun,Clip::StaffWalk,Clip::StaffRun,Clip::BowWalk,Clip::BowRun}) {
  const auto& t=result[unsigned(clip)];Check(t.loop&&t.stance>.1f&&t.stance<.9f&&t.speed>0,"Invalid gait contact metadata");
 }
 for(unsigned i=unsigned(Clip::SwordCarry);i<unsigned(Clip::SwordJump);++i) {
  const auto& t=result[i];Check(t.loop,"Carrying and directional weapon clips must loop");
  if(i>=unsigned(Clip::SwordWalkBackward))Check(t.stance>.1f&&t.stance<.9f&&t.speed>0,"Invalid armed gait metadata");
 }
 for(auto clip:{Clip::SwordJump,Clip::StaffJump,Clip::BowJump}) {
  const auto& t=result[unsigned(clip)];
  Check(!t.loop&&t.takeoff>0&&t.landing>t.takeoff&&t.landing<1,"Invalid equipped jump markers");
  Check(t.takeoff==jump.takeoff&&t.landing==jump.landing&&t.duration==jump.duration,"Equipped jump lost shared foot timing");
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
 std::array<float,frameChannels>a,b,c;std::memcpy(a.data(),&frames[i],sizeof(Frame));std::memcpy(b.data(),&frames[i+1],sizeof(Frame));
 {
  // Periodic Catmull-Rom preserves velocity at authored keys and wrap-around.
  std::array<float,frameChannels> left,right;
  std::memcpy(left.data(),&frames[i?i-1:(loop?frames.size()-2:0)],sizeof(Frame));
  std::memcpy(right.data(),&frames[i+2<frames.size()?i+2:(loop?1:frames.size()-1)],sizeof(Frame));
  const float t2=t*t,t3=t2*t;
  for(unsigned k=0;k<c.size();++k)c[k]=a[k]*(2*t3-3*t2+1)+(b[k]-left[k])*.5f*(t3-2*t2+t)+b[k]*(-2*t3+3*t2)+(right[k]-a[k])*.5f*(t3-t2);
 }
 Frame result;std::memcpy(&result,c.data(),sizeof(Frame));using namespace DirectX;XMStoreFloat3(&result.weapon,XMVector3Normalize(XMLoadFloat3(&result.weapon)));return result;
}
Frame Blend(const Frame& a,const Frame& b,float weight) {
 weight=std::clamp(std::isfinite(weight)?weight:0.f,0.f,1.f);
 std::array<float,frameChannels> x,y;std::memcpy(x.data(),&a,sizeof(a));std::memcpy(y.data(),&b,sizeof(b));
 for(unsigned i=0;i<x.size();++i)x[i]+=(y[i]-x[i])*weight;
 Frame f;std::memcpy(&f,x.data(),sizeof(f));return f;
}
Frame Locomotion(const std::array<float,4>& weights,float run,float phase,Equipment equipment){
 run=std::clamp(std::isfinite(run)?run:0.f,0.f,1.f);
 std::array<float,frameChannels> sum{};
 float total=0;
 for(unsigned d=0;d<4;++d) {
  const float weight=std::clamp(std::isfinite(weights[d])?weights[d]:0.f,0.f,1.f);
  if(weight<.00001f)continue;
  total+=weight;
  for(unsigned pace=0;pace<2;++pace) {
   const float w=weight*(pace?run:1-run);if(w<.00001f)continue;
   auto frame=Sample(Clip(unsigned(Clip::WalkForward)+pace*4+d),phase);
   if(equipment!=Equipment::None) {
    const auto forward=equipment==Equipment::Staff?(pace?Clip::StaffRun:Clip::StaffWalk):equipment==Equipment::Bow?(pace?Clip::BowRun:Clip::BowWalk):(pace?Clip::SwordRun:Clip::SwordWalk);
    const auto first=equipment==Equipment::Staff?Clip::StaffWalkBackward:equipment==Equipment::Bow?Clip::BowWalkBackward:Clip::SwordWalkBackward;
    const auto armed=Sample(d?Clip(unsigned(first)+pace*3+d-1):forward,phase);
    frame.hands=armed.hands;frame.elbows=armed.elbows;frame.shoulders=armed.shoulders;
    // The source already authors the load lag. Keep socket and elbow samples
    // at the same instant instead of introducing a second orientation delay.
    frame.weapon=armed.weapon;
   }
   std::array<float,frameChannels> v;std::memcpy(v.data(),&frame,sizeof(Frame));
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
 return std::clamp(h,0.f,.48f)*edge*edge*(3-2*edge);
}
float SwingPitch(const std::array<float,4>& weights,float run,int side,float phase,float releasePitch){
 if(side<0||side>1||!std::isfinite(phase)||!std::isfinite(releasePitch))return 0;
 const float t=std::clamp(phase,0.f,1.f),u=t*t*(3-2*t);
 run=std::clamp(std::isfinite(run)?run:0.f,0.f,1.f);
 float shape=0,total=0;
 for(unsigned d=0;d<4;++d)for(unsigned pace=0;pace<2;++pace){
  const float w=std::clamp(std::isfinite(weights[d])?weights[d]:0.f,0.f,1.f)*(pace?run:1-run);if(w<.00001f)continue;
  const auto clip=Clip(unsigned(Clip::WalkForward)+pace*4+d);const auto& track=Collection()[unsigned(clip)];
  const float begin=track.stance-side*.5f,end=1-side*.5f;
  const float a=Sample(clip,begin).ankles[side].x,b=Sample(clip,end).ankles[side].x;
  shape+=(Sample(clip,begin+(end-begin)*t).ankles[side].x-a*(1-u)-b*u)*w;total+=w;
 }
 // Keep the actual toe-off orientation and the flat landing anchor. Only the
 // interior shape comes from the source; neither endpoint can pop on contact.
 return std::clamp(releasePitch*(1-u)+shape/std::max(1.f,total),-.3f,1.3f);
}
float SwingProgress(const std::array<float,4>& weights,float run,int side,float phase){
 if(side<0||side>1||!std::isfinite(phase))return 0;
 const float t=std::clamp(phase,0.f,1.f),base=t*t*(3-2*t);
 if(t==0||t==1)return t;
 run=std::clamp(std::isfinite(run)?run:0.f,0.f,1.f);
 float deviation=0,total=0;
 for(unsigned d=0;d<4;++d)for(unsigned pace=0;pace<2;++pace){
  const float w=std::clamp(std::isfinite(weights[d])?weights[d]:0.f,0.f,1.f)*(pace?run:1-run);
  if(w<.00001f)continue;
  const auto clip=Clip(unsigned(Clip::WalkForward)+pace*4+d);
  const auto& track=Collection()[unsigned(clip)];
  const float begin=track.stance-side*.5f,end=1-side*.5f;
  const auto a=Sample(clip,begin).feet[side],b=Sample(clip,end).feet[side],f=Sample(clip,begin+(end-begin)*t).feet[side];
  const float x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
  if(length>.0025f){
   const float along=((f.x-a.x)*x+(f.z-a.z)*z)/length;
   deviation+=(std::clamp(along,-.12f,1.12f)-base)*w;
  }
  total+=w;
 }
 // Ease only the endpoint correction; the middle retains source acceleration.
 const float edge=std::clamp(std::min(t,1-t)*10.f,0.f,1.f);
 return base+deviation/std::max(1.f,total)*edge*edge*(3-2*edge);
}
float AttackPhase(Clip clip,float seconds,float start,float contact,float end){
 if(!std::isfinite(seconds)||seconds<0)return 0;
 Check(std::isfinite(end)&&start>0&&contact>start&&end>contact,"Invalid player attack retiming");
 Check(unsigned(clip)<unsigned(Clip::Count),"Invalid attack clip");const auto& t=Collection()[unsigned(clip)];
 Check(t.contact>t.load&&t.load>0,"Clip has no attack markers");
 if(clip==Clip::BowRelease||clip==Clip::BowPowerRelease||clip==Clip::Slash||clip==Clip::Overhead||clip==Clip::StaffSweep||clip==Clip::StaffThrust) {
  // PCL3 actions already contain acceleration, contact and deceleration.
  // Arc-length playback would erase their authored acceleration and convert
  // near-stationary recovery into abrupt changes in source time.
  const std::array<float,4> time={0,start,contact,end},phase={0,t.load,t.contact,1};
  const float a=t.load/start,b=(t.contact-t.load)/(contact-start),c=(1-t.contact)/(end-contact);
  const std::array<float,4> velocity={0,2*a*b/(a+b),2*b*c/(b+c),0};
  const unsigned segment=seconds<start?0:seconds<contact?1:2;
  const float span=time[segment+1]-time[segment],u=std::clamp((seconds-time[segment])/span,0.f,1.f),u2=u*u,u3=u2*u;
  return std::clamp(phase[segment]*(2*u3-3*u2+1)+phase[segment+1]*(-2*u3+3*u2)+
    span*(velocity[segment]*(u3-2*u2+u)+velocity[segment+1]*(u3-u2)),0.f,1.f);
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
