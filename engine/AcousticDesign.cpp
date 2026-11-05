#include "engine/AcousticDesign.h"
#include "engine/Audio.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace sound {
namespace {
constexpr float pi=3.14159265359f,tau=2*pi;
struct Noise {std::uint32_t seed;float Next(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return float(seed&65535)/32767.5f-1;}};
struct Low {float value=0;float Sample(float x,float amount){return value+=amount*(x-value);}};
struct Mode {
 float a,b,g,y=0,z=0;
 Mode(float hz,float width):a(2*std::exp(-pi*width/sampleRate)*std::cos(tau*hz/sampleRate)),b(-std::exp(-tau*width/sampleRate)),g(1-std::exp(-pi*width/sampleRate)){}
 float Sample(float x){const float next=g*x+a*y+b*z;z=y;return y=next;}
};
float Env(float t,float attack,float decay){return t<0?0:std::min(1.f,t/attack)*std::exp(-decay*t);}
float Smooth(float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);}
void Room(std::vector<float>& pcm,float wet,float decay){
 // Four damped feedback paths, rendered once. No reverb work in the callback.
 const float delays[]={.0371f,.0533f,.0719f,.0899f};
 std::vector<float> result(pcm.size());
 for(float seconds:delays){const auto count=std::size_t(seconds*sampleRate);std::vector<float> line(count);std::size_t head=0;float low=0;
  const float feedback=std::pow(.001f,seconds/decay);
  for(std::size_t n=0;n<pcm.size();++n){const float old=line[head];low+=.24f*(old-low);line[head]=pcm[n]+low*feedback;
   result[n]+=old*.25f;if(++head==count)head=0;}}
 for(std::size_t n=0;n<pcm.size();++n)pcm[n]+=result[n]*wet;
}
std::vector<std::int16_t> Finish(std::vector<float> pcm,float ceiling){
 float peak=.0001f;for(float x:pcm)peak=std::max(peak,std::abs(x));const float gain=std::min(1.f,ceiling/peak);
 std::vector<std::int16_t> result(pcm.size());
 for(std::size_t n=0;n<pcm.size();++n){const float edge=Smooth(float(n)/(sampleRate*.002f))*Smooth(float(pcm.size()-1-n)/(sampleRate*.04f));result[n]=std::int16_t(std::clamp(pcm[n]*gain*edge,-.90f,.90f)*32767);}
 return result;
}
}
bool HasAcousticDesign(Cue c){return c==Cue::Bow || c==Cue::Fire || c==Cue::Holy || c==Cue::Mend || c==Cue::Cast ||
 c==Cue::Step || c==Cue::StepGrass || c==Cue::StepDirt || c==Cue::StepWood || IsLanding(c) || (c>=Cue::BowDraw && c<Cue::Count);}
std::vector<std::int16_t> AcousticEffect(Cue cue){
 const bool holy=cue==Cue::Holy || cue==Cue::HolyGather || cue==Cue::HolyImpact || cue==Cue::HolyMend || cue==Cue::Mend;
 const bool fire=cue==Cue::Fire || cue==Cue::FireGather || cue==Cue::FireHeavy || cue==Cue::FireImpact;
 const bool frost=cue==Cue::FrostGather || cue==Cue::FrostRelease || cue==Cue::FrostImpact;
 const bool arcane=cue==Cue::ArcaneGather || cue==Cue::ArcaneRelease || cue==Cue::ArcaneImpact || cue==Cue::Cast;
 const bool bow=cue==Cue::Bow || cue==Cue::BowHeavy;
 const bool gather=cue==Cue::FireGather || cue==Cue::FrostGather || cue==Cue::ArcaneGather || cue==Cue::HolyGather || cue==Cue::Cast;
 const bool impact=cue==Cue::FireImpact || cue==Cue::FrostImpact || cue==Cue::ArcaneImpact || cue==Cue::HolyImpact;
 const bool healing=cue==Cue::HolyMend || cue==Cue::Mend;
 const bool step=IsStep(cue),land=IsLanding(cue);
 const float duration=holy?(healing?2.6f:2.1f):fire?(cue==Cue::FireHeavy || impact?1.7f:1.15f):frost || arcane?1.35f:step?.32f:land?.55f:cue==Cue::BowDraw?.64f:cue==Cue::WarriorEffort?.56f:.65f;
 std::vector<float> pcm(std::size_t(duration*sampleRate));Noise random{0x36af91u+unsigned(cue)*1741};Low body,airflow,slow;
 Mode chest(160,90),wood(240,90),bowBody(380,130),ice1(1733,19),ice2(2917,31),ice3(4211,55);
 Mode vowel1(620,110),vowel2(1120,145),vowel3(2450,260),grunt1(520,130),grunt2(1250,220);
 const std::size_t stringLength=std::size_t(sampleRate/(cue==Cue::BowHeavy?105.f:145.f));std::vector<float> string(stringLength);for(auto& x:string)x=random.Next()*.8f;
 std::size_t stringHead=0;float phase=0,crack=0;
 for(std::size_t n=0;n<pcm.size();++n){
  const float t=float(n)/sampleRate,noise=random.Next(),low=body.Sample(noise,.045f),mid=airflow.Sample(noise,.28f),breath=slow.Sample(noise,.002f),high=noise-mid;
  float x=0;
  if(bow){
   const float pluck=string[stringHead];const auto next=(stringHead+1)%stringLength;string[stringHead]=.965f*(pluck+string[next])*.5f;stringHead=next;
   x=.45f*pluck+.40f*wood.Sample(pluck)+.26f*bowBody.Sample(pluck);
   x+=.33f*high*Env(t,.0018f,100)+.22f*mid*Env(t-.022f,.014f,18);
  }else if(cue==Cue::BowDraw){
   const float flex=std::pow(std::sin(pi*std::min(t/.55f,1.f)),2.f);
   x=flex*(.65f*wood.Sample(mid)+.20f*mid)+.12f*high*Env(t-.12f,.01f,28)+.17f*wood.Sample(noise)*Env(t-.40f,.002f,30);
  }else if(cue==Cue::ArrowImpact){
   x=.75f*chest.Sample(noise)*Env(t,.001f,35)+.35f*mid*Env(t,.001f,45)+.18f*high*Env(t-.013f,.002f,75);
   x+=.20f*wood.Sample(noise)*Env(t-.025f,.003f,18);
  }else if(fire){
   const float envelope=gather?Smooth(t/.27f)*std::exp(-t*3.5f):Env(t,impact?.004f:.018f,impact?3.8f:5.5f);
   const float turbulence=.62f+.38f*std::sin(t*31+breath*20)*std::sin(t*47+breath*31);
   if(random.Next()>.9975f)crack=.25f+.75f*std::abs(random.Next());else crack*=.89f;
   x=envelope*(1.5f*low+.62f*mid*turbulence+.13f*high)+crack*high*envelope*.55f;
   x+=.34f*chest.Sample(noise)*Env(t,.006f,impact?8.f:14.f);
   if(cue==Cue::FireHeavy || impact)x+=.17f*std::sin(tau*(67*t-13*t*t))*Env(t,.009f,8);
  }else if(frost){
   const float excitation=(noise> .989f?noise*10:noise*.08f)*Env(t,gather?.12f:.002f,gather?3.f:7.f);
   x=.45f*(ice1.Sample(excitation)+.60f*ice2.Sample(excitation)+.25f*ice3.Sample(excitation));
   x+=.29f*high*Env(t,gather?.18f:.015f,4.5f)+.65f*low*Env(t,.012f,9);
  }else if(arcane){
   const float envelope=Env(t,gather?.16f:.012f,3.7f);
   x=(.70f*low+.22f*mid)*envelope;
   x+=.24f*(ice1.Sample(noise)+ice2.Sample(noise)*.4f)*envelope;
   x+=.07f*(std::sin(tau*73.42f*t)+.45f*std::sin(tau*109.8f*t))*envelope;
  }else if(holy){
   // Glottal excitation and fixed vowel formants give a choral body rather
   // than an ascending notification triad. No intelligible sampled voice.
   phase+=(healing?110.f:146.83f)*(1+.003f*std::sin(tau*4.9f*t)+breath*.02f)/sampleRate;
   const bool pulse=phase>=1;if(pulse)phase-=1;
   const float glottal=(pulse?2.8f:0)+noise*.035f;
   const float choir=.7f*vowel1.Sample(glottal)+.4f*vowel2.Sample(glottal)+.14f*vowel3.Sample(glottal);
   const float sustain=Smooth(t/(gather?.20f:.075f))*std::exp(-t*(healing?1.1f:2.f));
   x=choir*3.5f*sustain+.035f*std::sin(tau*73.415f*t)*sustain;
   if(!gather){const float bell=Env(t,.005f,2.9f);
    x+=bell*(.11f*std::sin(tau*293.66f*t)+.06f*std::sin(tau*613.7f*t)+.022f*std::sin(tau*812.4f*t));}
   x+=mid*Env(t,.015f,impact?8.f:14.f)*.30f;
  }else if(cue==Cue::WarriorEffort){
   phase+=(91.f+14.f*std::sin(t*7)+breath*20)/sampleRate;const bool pulse=phase>=1;if(pulse)phase-=1;
   const float excitation=(pulse?2.7f:0)+noise*.12f;
   x=(grunt1.Sample(excitation)*3+grunt2.Sample(excitation)*.9f+mid*.20f)*Env(t,.035f,9.f);
  }else if(step || land){
   const bool timber=cue==Cue::StepWood || cue==Cue::LandWood,dirt=cue==Cue::StepDirt || cue==Cue::LandDirt,grass=cue==Cue::StepGrass || cue==Cue::LandGrass;
   const float heel=Env(t,.0025f,land?22.f:38.f),toe=Env(t-(land?.055f:.075f),.006f,32.f);
   x=(.62f*low+.27f*mid)*(heel+.65f*toe);
   if(timber)x+=wood.Sample(noise)*(.65f*heel+.35f*toe);
   else if(grass)x+=high*.23f*Env(t-.025f,.014f,15);
   else if(dirt){if(random.Next()>.994f)crack=.6f;else crack*=.82f;x+=high*(.16f+crack)*Env(t-.014f,.004f,21);}
   else x+=.14f*high*(heel+.3f*toe)+bowBody.Sample(noise)*heel*.2f;
  }
  pcm[n]=x;
 }
 if(holy)Room(pcm,.58f,healing?2.3f:1.7f);else if(fire || frost || arcane)Room(pcm,.16f,.55f);
 return Finish(std::move(pcm),step?.45f:land?.57f:.78f);
}

std::vector<std::int16_t> ComposeScore(bool menu){
 // Original slow modal score. Sparse phrases and ensemble attacks replace
 // the repeating bright lead/arpeggio. A wavetable keeps startup bounded.
 constexpr std::size_t tableSize=2048;std::array<std::array<float,tableSize>,4> tables{};
 for(unsigned kind=0;kind<tables.size();++kind)for(std::size_t n=0;n<tableSize;++n){
  const float phase=tau*float(n)/tableSize;float value=0;
  for(int h=1;h<=24;++h){float weight=0;
   if(kind==0)weight=1.f/std::pow(float(h),1.7f); // bowed low strings
   if(kind==1)weight=(h==1?.35f:std::exp(-std::pow((h-5.f)/2.3f,2.f))*.7f+std::exp(-std::pow((h-10.f)/3.f,2.f))*.23f)/std::sqrt(float(h)); // vowel
   if(kind==2)weight=1.f/std::pow(float(h),2.2f); // quiet horn
   if(kind==3)weight=h%2?1.f/(h*h):0; // pipe ranks
   value+=std::sin(phase*h)*weight;
  }tables[kind][n]=value;
 }
 std::vector<float> pcm(sampleRate*musicSeconds);
 const auto note=[&](unsigned kind,float pitch,float start,float length,float level,float attack){
  const auto begin=std::size_t(std::max(0.f,start)*sampleRate),end=std::min(pcm.size(),std::size_t((start+length+1.7f)*sampleRate));
  float phase=0,other=0;const float increment=pitch/sampleRate,detuned=increment*1.0027f;
  for(std::size_t n=begin;n<end;++n){const float t=float(n-begin)/sampleRate;
   const float env=Smooth(t/attack)*Smooth((length+1.7f-t)/1.7f);
   const auto sample=[&](float p){const float index=p*tableSize;const auto at=std::size_t(index)%tableSize;return tables[kind][at]+(tables[kind][(at+1)%tableSize]-tables[kind][at])*(index-std::floor(index));};
   pcm[n]+=level*env*(sample(phase)+.38f*sample(other));phase+=increment;other+=detuned;if(phase>=1)phase-=1;if(other>=1)other-=1;
  }
 };
 constexpr float roots[]={73.416f,58.270f,65.406f,55.f,87.307f,65.406f,55.f,73.416f};
 for(int bar=0;bar<8;++bar){const float root=roots[bar],start=bar*6.f;
  note(menu?3:0,root,start,5.5f,menu?.038f:.045f,1.1f);
  note(0,root*1.4983f,start+.11f,5.1f,.024f,1.5f);
  note(menu?1:0,root*2,start+.23f,4.7f,menu?.052f:.026f,1.35f);
  note(1,root*(bar==1 || bar==4?2.51984f:2.37841f),start+.36f,4.2f,menu?.032f:.016f,1.6f);
 }
 const float melody[]={146.83f,174.61f,164.81f,130.81f,146.83f,110.f};
 const float times[]={3.8f,10.1f,16.5f,25.3f,32.6f,40.2f};
 for(int n=0;n<6;++n)note(menu?3:2,melody[n],times[n],n%2?3.1f:2.3f,menu?.026f:.047f,.4f);
 Noise random{menu?0x5637abu:0x17bca2u};Low low;
 for(std::size_t n=0;n<pcm.size();++n){const float t=float(n)/sampleRate,noise=low.Sample(random.Next(),.04f);
  if(t>23 && t<43){const float beat=std::fmod(t-24,6.f);if(beat>=0)pcm[n]+=(.045f*noise+.026f*std::sin(tau*(61*beat-9*beat*beat)))*Env(beat,.012f,6);}
 }
 Room(pcm,menu?.65f:.28f,menu?3.1f:1.7f);
 for(std::size_t n=0;n<pcm.size();++n){const float t=float(n)/sampleRate;pcm[n]*=Smooth(t/2.f)*Smooth((musicSeconds-t)/3.2f);}
 return Finish(std::move(pcm),.38f);
}
}
