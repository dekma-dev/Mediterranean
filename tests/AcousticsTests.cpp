#include "engine/Audio.h"
#include <xapobase.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>

void Require(bool condition,const char* message);
namespace {
float Flat(rpg::Vec2){return 0;}
float Ridge(rpg::Vec2 p){return std::abs(p.x+2.5f)<.3f?3.f:0.f;}
double Energy(const std::vector<float>& samples,size_t begin=0){
  double e=0;for(size_t i=begin;i<samples.size();++i)e+=double(samples[i])*samples[i];return e;
}
// Exercise the exact native effect and preset used on the runtime wet bus.
// This offline host never opens an endpoint or plays audio into the room.
std::vector<float> Reflect(const std::vector<float>& input){
  IUnknown* effect=nullptr;Require(SUCCEEDED(XAudio2CreateReverb(&effect)),"Native room effect must instantiate");
  IXAPO* processor=nullptr;IXAPOParameters* parameters=nullptr;
  Require(SUCCEEDED(effect->QueryInterface(__uuidof(IXAPO),reinterpret_cast<void**>(&processor))),"Room must expose IXAPO");
  Require(SUCCEEDED(effect->QueryInterface(__uuidof(IXAPOParameters),reinterpret_cast<void**>(&parameters))),"Room must expose parameters");
  const auto preset=sound::ShortRoomParameters();
  WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;format.nChannels=2;
  format.nSamplesPerSec=sound::roomSampleRate;format.wBitsPerSample=32;format.nBlockAlign=8;format.nAvgBytesPerSec=format.nSamplesPerSec*8;
  constexpr UINT32 block=256;XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS lock{&format,block};
  Require(SUCCEEDED(processor->LockForProcess(1,&lock,1,&lock)),"Room must lock stereo 48 kHz buffers");
  parameters->SetParameters(&preset,sizeof(preset));
  std::vector<float> output(input.size());std::array<float,block*2> in{},out{};
  for(size_t at=0;at<input.size();at+=in.size()){
    in.fill(0);out.fill(0);const size_t n=std::min(in.size(),input.size()-at);
    std::copy_n(input.data()+at,n,in.data());
    XAPO_PROCESS_BUFFER_PARAMETERS source{in.data(),XAPO_BUFFER_VALID,UINT32(n/2)};
    XAPO_PROCESS_BUFFER_PARAMETERS target{out.data(),XAPO_BUFFER_VALID,UINT32(n/2)};
    processor->Process(1,&source,1,&target,TRUE);
    if(target.BufferFlags==XAPO_BUFFER_VALID)std::copy_n(out.data(),n,output.data()+at);
  }
  // Prime a fresh tail, then reset while it is still alive. Testing only after
  // the end of a long preview would accidentally pass even with a no-op reset.
  for(unsigned pass=0;pass<8;++pass){
    in.fill(0);out.fill(0);if(pass==0)in[0]=in[1]=.5f;
    XAPO_PROCESS_BUFFER_PARAMETERS source{in.data(),XAPO_BUFFER_VALID,block};
    XAPO_PROCESS_BUFFER_PARAMETERS target{out.data(),XAPO_BUFFER_VALID,block};
    processor->Process(1,&source,1,&target,TRUE);
  }
  processor->Reset();in.fill(0);
  for(unsigned pass=0;pass<32;++pass){
    out.fill(0);XAPO_PROCESS_BUFFER_PARAMETERS source{in.data(),XAPO_BUFFER_SILENT,block};
    XAPO_PROCESS_BUFFER_PARAMETERS target{out.data(),XAPO_BUFFER_VALID,block};
    processor->Process(1,&source,1,&target,TRUE);
    if(target.BufferFlags==XAPO_BUFFER_VALID)for(float value:out)Require(std::abs(value)<1e-7f,"Reset must discard room tails before a load/menu/focus boundary");
  }
  processor->UnlockForProcess();parameters->Release();processor->Release();effect->Release();
  for(float value:output)Require(std::isfinite(value),"Native reflections must remain finite");
  return output;
}
void WriteReview(const char* name,const std::vector<float>& pcm){
  std::ofstream f(name,std::ios::binary);const auto word=[&](unsigned v,int n){for(int i=0;i<n;++i)f.put(char(v>>(8*i)));};
  f.write("RIFF",4);word(36+unsigned(pcm.size()*2),4);f.write("WAVEfmt ",8);
  word(16,4);word(1,2);word(2,2);word(sound::roomSampleRate,4);word(sound::roomSampleRate*4,4);word(4,2);word(16,2);
  f.write("data",4);word(unsigned(pcm.size()*2),4);
  for(float sample:pcm){Require(std::isfinite(sample)&&std::abs(sample)<.98f,"Environmental review must retain PCM headroom");word(unsigned(int(std::clamp(sample,-1.f,1.f)*32767)),2);}
  Require(bool(f),"Environmental audio review must write");
}
}
void RunAcousticsTests(){
  std::cout<<std::unitbuf;
  const auto com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  using P=sound::AcousticPoint;using B=particles::Box;
  const P source{-4,1.5f,0},listener{4,1.5f,0};
  sound::AcousticWorld field({},Flat),wall({B{{-.2f,0,-2},{.2f,4,2}}},Flat);
  Require(field.Obstruction(source,listener)==0&&field.Enclosure(listener)==0,"Open field must be dry and unobstructed");
  Require(wall.Obstruction(source,listener)==1,"Solid walls must obstruct every ear sample");
  Require(wall.Obstruction({-4,5,0},{4,5,0})==0,"Sources above architecture must not be treated as ground-level sound");
  const float edge=wall.Obstruction(source,{4,1.5f,4});
  Require(edge>0&&edge<1,"A wall edge must produce partial occlusion, not binary flicker");
  sound::AcousticWorld ridge({},Ridge);
  Require(ridge.Obstruction(source,listener)==1,"Off-center terrain ridges must block sound between clear endpoints");
  Require(ridge.Obstruction({-4,4,0},{4,4,0})==0,"Sound above a ridge must stay clear");
  Require(sound::Place3D({4,8,0},0).gain<sound::Place3D({4,0,0},0).gain,"Height difference must contribute to audible distance");
  const auto overhead=sound::Place3D({0,8,0},0);
  Require(overhead.gain>0&&std::abs(overhead.left-overhead.right)<1e-6f,"Overhead sources must stay centered, not jump to one ear");
  const float nan=std::numeric_limits<float>::quiet_NaN();
  Require(wall.Obstruction({nan,0,0},listener)==0&&wall.Obstruction({-100000,0,0},listener)==0,"Invalid coordinates must not enter grid loops");
  sound::AcousticWorld courtyard({B{{-3,0,-3},{-2.8f,4,3}},B{{2.8f,0,-3},{3,4,3}},B{{-3,0,-3},{3,4,-2.8f}},B{{-3,0,2.8f},{3,4,3}}},Flat);
  const float room=courtyard.Enclosure({0,1.5f,0});
  Require(room>.65f&&room<=1,"Nearby masonry must supply a bounded room response");
  Require(courtyard.Enclosure({0,5,0})==0,"Room rays must respect wall height");
  sound::AcousticResponse previous{};
  for(int rate:{30,60,144,1000}){
    sound::AcousticResponse r;for(int i=0;i<rate;++i)r.Update(1,room,1.f/rate);
    Require(r.DirectGain()>.219f&&r.DirectGain()<.222f&&r.FilterCoefficient()<.16f,"Occlusion must reduce gain and brightness together");
    if(rate!=30)Require(std::abs(r.obstruction-previous.obstruction)<.0001f&&std::abs(r.enclosure-previous.enclosure)<.0001f,"Acoustic smoothing must agree across refresh rates");
    previous=r;
    for(int i=0;i<rate;++i)r.Update(0,0,1.f/rate);
    Require(r.DirectGain()>.999f&&r.FilterCoefficient()>.999f,"Leaving a wall must restore the original direct sound");
  }
  sound::AcousticResponse invalid{nan,nan};invalid.Update(nan,nan,nan);
  Require(std::isfinite(invalid.DirectGain()+invalid.FilterCoefficient()+invalid.WetSend()),"Invalid acoustic state must recover safely");
  rpg::WorldMap map;sound::AcousticWorld world(map);size_t expected=0;
  for(const auto& p:map.Obstacles())expected+=p.kind==rpg::PropKind::Building||p.kind==rpg::PropKind::Ruin||p.kind==rpg::PropKind::SurveyStone;
  Require(world.WallCount()==expected,"Trees/canopies/decorations must not become acoustic walls");
  const auto begin=std::chrono::steady_clock::now();float checksum=0;
  for(int i=0;i<2400;++i){const float a=float(i)*.03f;P p{std::cos(a)*14,0,std::sin(a)*14};p.y=map.Height({p.x,p.z})+1;
    checksum+=world.Obstruction(p,{0,map.Height({0,0})+1.55f,0});
  }
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
  std::cout<<"World acoustics: "<<world.WallCount()<<" hard volumes; 2400 five-ray queries in "<<ms<<" ms; checksum "<<checksum<<".\n";
  // Impulse: there must be actual delayed energy, not a louder dry duplicate.
  std::vector<float> impulse(sound::roomSampleRate*4);impulse[0]=impulse[1]=.5f;
  const auto reflected=Reflect(impulse);
  Require(Energy(reflected,size_t(sound::roomSampleRate*.025)*2)>1e-7,"Native room must produce delayed reflections");
  Require(Energy(reflected,size_t(sound::roomSampleRate*1.5)*2)<Energy(reflected)*.01,"Short-room tail must decay without a lingering cathedral wash");
  std::vector<float> crowded(sound::roomSampleRate*4),crowdedWet(crowded.size());
  const auto transient=sound::Generate(sound::Cue::FireImpact);
  // Coherent sources driven to the runtime's conservative .70/1.25 ceiling.
  // Sum complete repeated transients, not a prefix that can miss their peak.
  float crowdedPeak=0;
  for(size_t frame=0;frame<crowded.size()/2;++frame){
    const size_t at=size_t(double(frame)*sound::sampleRate/sound::roomSampleRate);float value=0;
    for(size_t onset=0;onset<sound::sampleRate*2;onset+=sound::sampleRate/5)
      if(at>=onset&&at-onset<transient.size())value+=transient[at-onset]/32768.f;
    crowded[frame*2]=crowded[frame*2+1]=value;crowdedPeak=std::max(crowdedPeak,std::abs(value));
  }
  Require(crowdedPeak>0,"Dense mix fixture must contain the full attack");
  for(size_t i=0;i<crowded.size();++i){crowded[i]*=.56f/crowdedPeak;crowdedWet[i]=crowded[i]*.5f;}
  const auto crowdedReturn=Reflect(crowdedWet);float crowdPeak=0;
  for(size_t i=0;i<crowded.size();++i)crowdPeak=std::max(crowdPeak,std::abs(crowded[i]+crowdedReturn[i]));
  Require(crowdPeak<.8f,"Repeated dense contacts plus the real wet DSP must preserve headroom");
  std::cout<<"Dense native dry+wet peak: "<<crowdPeak<<".\n";
  // Same four existing contact recordings, repeated in three environments:
  // open field, courtyard, courtyard with a solid wall. No new source assets.
  const sound::Cue cues[]={sound::Cue::HeavyHit,sound::Cue::FireImpact,sound::Cue::ArrowImpact,sound::Cue::HolyImpact};
  constexpr unsigned segment=sound::roomSampleRate*3;
  std::vector<float> before(segment*2*12),after(before.size()),wet(before.size());
  for(unsigned scene=0;scene<3;++scene)for(unsigned c=0;c<4;++c){
    const auto pcm=sound::Generate(cues[c]);sound::AcousticResponse response{scene==2?1.f:0.f,scene==0?0.f:room};
    float filtered=0;const size_t offset=size_t(scene*4+c)*segment*2;
    for(unsigned f=0;f<segment;++f){const double cursor=double(f)*sound::sampleRate/sound::roomSampleRate;
      const size_t index=size_t(cursor);float value=0;
      if(index<pcm.size()){const float t=float(cursor-index);value=(pcm[index]*(1-t)+(index+1<pcm.size()?pcm[index+1]:0)*t)/32768.f*.45f;}
      // Match the source-rate one-pole after resampling for this offline host.
      const float coefficient=1-std::pow(1-response.FilterCoefficient(),float(sound::sampleRate)/sound::roomSampleRate);
      filtered+=coefficient*(value-filtered);
      for(unsigned channel=0;channel<2;++channel){const size_t at=offset+f*2+channel;
        before[at]=value*.70710678f;after[at]=filtered*response.DirectGain()*.70710678f;wet[at]=after[at]*response.WetSend();
      }
    }
  }
  const auto returns=Reflect(wet);for(size_t i=0;i<after.size();++i)after[i]+=returns[i];
  WriteReview("environment-combat-before.wav",before);WriteReview("environment-combat-after.wav",after);
  if(SUCCEEDED(com))CoUninitialize();
  std::cout<<"PASS: five-ray wall/terrain occlusion, height/edge handling, refresh-independent response, native wet DSP/reset and four-class 36-second before/after audio.\n";
}
