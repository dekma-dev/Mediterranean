#include "engine/RecordedAudio.h"
#include "engine/Audio.h"
#include <algorithm>
#include <cstring>
#include <string>

namespace sound {
namespace {
std::uint16_t U16(const std::uint8_t* p){return p[0]|std::uint16_t(p[1])<<8;}
std::uint32_t U32(const std::uint8_t* p){return U16(p)|std::uint32_t(U16(p+2))<<16;}
const void* Resource(unsigned id,std::size_t& size){
 const auto module=GetModuleHandleW(nullptr);const auto r=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);
 if(!r)return nullptr;size=SizeofResource(module,r);return LockResource(LoadResource(module,r));
}
const char* Name(Cue cue){
 switch(cue){
 case Cue::Bow:return "Bow";case Cue::BowHeavy:return "BowHeavy";
 case Cue::Fire:return "Fire";case Cue::FireHeavy:return "FireHeavy";case Cue::FireImpact:return "FireImpact";
 case Cue::WarriorEffort:return "WarriorEffort";
 case Cue::Step:return "Step";case Cue::StepGrass:return "StepGrass";case Cue::StepDirt:return "StepDirt";
 case Cue::Land:return "Land";case Cue::LandGrass:return "LandGrass";case Cue::LandDirt:return "LandDirt";
 default:return nullptr;
 }
}
}
bool ParseMusicWave(const void* raw,std::size_t size,WaveView& out){
 out={};if(!raw || size<12 || size>24*1024*1024)return false;
 const auto* p=static_cast<const std::uint8_t*>(raw);
 if(std::memcmp(p,"RIFF",4) || std::memcmp(p+8,"WAVE",4) || U32(p+4)!=size-8)return false;
 WaveView candidate;bool fmt=false,fact=false,data=false;
 for(std::size_t at=12;at<size;){
  if(size-at<8)return false;const auto n=U32(p+at+4);const auto* payload=p+at+8;
  if(n>size-at-8)return false;
  if(!std::memcmp(p+at,"fmt ",4)){
   if(fmt || n!=50)return false;fmt=true;std::memcpy(&candidate.format,payload,50);
  }else if(!std::memcmp(p+at,"fact",4)){
   if(fact || n!=4)return false;fact=true;candidate.frames=U32(payload);
  }else if(!std::memcmp(p+at,"data",4)){
   if(data || !n)return false;data=true;candidate.data=payload;candidate.bytes=n;
  }
  const std::size_t advance=8+std::size_t(n)+(n&1);if(advance>size-at)return false;at+=advance;
 }
 const auto& f=candidate.format;const auto& w=f.wfx;
 if(!fmt || !fact || !data || w.wFormatTag!=WAVE_FORMAT_ADPCM || w.nChannels!=2 || w.nSamplesPerSec!=44100 ||
    w.wBitsPerSample!=4 || w.cbSize!=32 || f.wNumCoef!=7 || f.wSamplesPerBlock!=256 || w.nBlockAlign!=268 ||
    w.nAvgBytesPerSec!=44100*268/256 || candidate.bytes%268 || candidate.frames!=candidate.bytes/268*256)return false;
 constexpr short coefficients[7][2]={{256,0},{512,-256},{0,0},{192,64},{240,0},{460,-208},{392,-232}};
 for(unsigned i=0;i<7;++i)if(f.aCoef[i].iCoef1!=coefficients[i][0] || f.aCoef[i].iCoef2!=coefficients[i][1])return false;
 // Invalid predictor indices can make a native decoder read beyond its table.
 for(std::size_t at=0;at<candidate.bytes;at+=268)if(candidate.data[at]>6 || candidate.data[at+1]>6)return false;
 out=candidate;return true;
}
bool MusicRecording(unsigned track,WaveView& out){
 out={};if(track>2)return false;std::size_t size=0;const auto* p=Resource(620+track,size);return ParseMusicWave(p,size,out);
}
bool ParseRecordingBank(const void* raw,std::size_t size,const char* name,std::vector<std::int16_t>& out){
 out.clear();if(!raw || !name || size<12 || size>8*1024*1024)return false;
 const auto* p=static_cast<const std::uint8_t*>(raw);
 if(std::memcmp(p,"MSFX",4) || U32(p+4)!=1 || U32(p+8)>192)return false;
 const auto count=U32(p+8);std::size_t at=12,match=0;unsigned frames=0;
 for(unsigned i=0;i<count;++i){
  if(size-at<36 || !std::memchr(p+at,0,32))return false;
  const auto n=U32(p+at+32);if(!n || n>sampleRate*4 || n>(size-at-36)/2)return false;
  if(!std::strcmp(reinterpret_cast<const char*>(p+at),name)){
   if(match)return false;match=at+36;frames=n;
  }
  at+=36+std::size_t(n)*2;
 }
 if(at!=size || !match)return false;
 out.resize(frames);std::memcpy(out.data(),p+match,frames*2);return true;
}
unsigned RecordingVariants(Cue cue){return Name(cue)?3:0;}
std::vector<std::int16_t> RecordedEffect(Cue cue,unsigned variant){
 std::vector<std::int16_t> out;const auto* name=Name(cue);if(!name)return out;
 std::size_t size=0;const auto* p=Resource(610,size);const auto key=std::string(name)+std::to_string(variant%3);
 ParseRecordingBank(p,size,key.c_str(),out);return out;
}
}
