#include "engine/Audio.h"
#include "engine/RecordedAudio.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
void Require(bool condition,const char* message){if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
struct Callback final:IXAudio2VoiceCallback {
 std::atomic<unsigned> ended{0},errors{0};
 void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override{}
 void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override{}
 void STDMETHODCALLTYPE OnStreamEnd() override{}
 void STDMETHODCALLTYPE OnBufferStart(void*) override{}
 void STDMETHODCALLTYPE OnBufferEnd(void*) override{++ended;}
 void STDMETHODCALLTYPE OnLoopEnd(void*) override{}
 void STDMETHODCALLTYPE OnVoiceError(void*,HRESULT) override{++errors;}
};
std::vector<std::uint8_t> Resource(unsigned id){
 const auto module=GetModuleHandleW(nullptr);const auto r=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);
 Require(r!=nullptr,"Missing recorded audio resource");
 const auto* data=static_cast<const std::uint8_t*>(LockResource(LoadResource(module,r)));
 return {data,data+SizeofResource(module,r)};
}
}
int main(){
 using Q=sound::Cue;std::size_t musicBytes=0,pcmBytes=0;
 Require(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization");
 IXAudio2* engine=nullptr;IXAudio2MasteringVoice* master=nullptr;
 Require(SUCCEEDED(XAudio2Create(&engine)) && SUCCEEDED(engine->CreateMasteringVoice(&master)),"XAudio2 device");
 master->SetVolume(0); // Native decoder verification never plays on the user's speakers.
 for(unsigned track=0;track<3;++track){
  sound::WaveView music;Require(sound::MusicRecording(track,music),"Missing or corrupt music");
  musicBytes+=music.bytes;Require(music.frames>44100*50 && music.frames<44100*150,"Incomplete music composition");
  Callback callback;IXAudio2SourceVoice* voice=nullptr;
  Require(SUCCEEDED(engine->CreateSourceVoice(&voice,&music.format.wfx,0,2,&callback)),"ADPCM voice creation");
  // Exercise nonzero audio, then an actual block-aligned repeat at the cadence.
  for(bool ending:{false,true}){
   XAUDIO2_BUFFER buffer{};buffer.pAudioData=music.data;buffer.AudioBytes=music.bytes;
   buffer.PlayBegin=ending?music.frames-512:44100/256*256*12;buffer.PlayLength=512;
   buffer.LoopBegin=buffer.PlayBegin;buffer.LoopLength=512;buffer.LoopCount=1;
   Require(SUCCEEDED(voice->SubmitSourceBuffer(&buffer)) && SUCCEEDED(voice->Start()),"Native music buffer");
   for(unsigned wait=0;wait<100 && callback.ended<(ending?2u:1u) && !callback.errors;++wait)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
   Require(callback.ended==(ending?2u:1u) && !callback.errors,"ADPCM decode or cadence loop failed");
   voice->Stop();
  }
  voice->DestroyVoice();
  auto raw=Resource(620+track);sound::WaveView bad;
  for(std::size_t cut:{std::size_t(0),std::size_t(11),std::size_t(47),raw.size()-1})
   Require(!sound::ParseMusicWave(raw.data(),cut,bad) && !bad.data,"Truncated music accepted");
  sound::WaveView copy;Require(sound::ParseMusicWave(raw.data(),raw.size(),copy),"Copied resource parse");
  const auto dataOffset=std::size_t(copy.data-raw.data());
  raw[dataOffset]=7;Require(!sound::ParseMusicWave(raw.data(),raw.size(),bad),"Invalid ADPCM predictor accepted");
 }
 Require(musicBytes<13*1024*1024,"Music resource budget exceeded");
 for(Q cue:{Q::Bow,Q::BowHeavy,Q::Fire,Q::FireGather,Q::FireHeavy,Q::FireImpact,Q::Holy,Q::HolyGather,Q::HolyImpact,Q::HolyMend,
             Q::WarriorEffort,Q::Step,Q::StepDirt,Q::StepGrass,Q::Land,Q::LandDirt,Q::LandGrass}){
  std::vector<std::int16_t> previous;
  for(unsigned variant=0;variant<3;++variant){
   const auto pcm=sound::RecordedEffect(cue,variant);pcmBytes+=pcm.size()*2;
   Require(pcm.size()>1000 && pcm.size()<=sound::sampleRate*4,"Missing or oversized recorded effect");
   Require(pcm.front()==0 && pcm.back()==0,"Recording boundary click");
   Require(pcm!=previous,"Recorded variants are identical");previous=pcm;
   Require(pcm==sound::Generate(cue,variant),"Runtime still uses procedural fallback instead of recording");
   int peak=0;for(auto sample:pcm)peak=std::max(peak,std::abs(int(sample)));
   Require(peak>5000 && peak<30000,"Recorded effect normalization");
  }
 }
 Require(pcmBytes<3*1024*1024,"Recorded effects PCM budget exceeded");
 auto bank=Resource(610);std::vector<std::int16_t> out;
 Require(sound::ParseRecordingBank(bank.data(),bank.size(),"Bow0",out),"Bank lookup");
 Require(!sound::ParseRecordingBank(bank.data(),bank.size(),"Missing",out) && out.empty(),"Missing cue returned stale PCM");
 Require(!sound::ParseRecordingBank(bank.data(),bank.size()-1,"Bow0",out),"Truncated bank accepted");
 bank[44]=bank[45]=bank[46]=bank[47]=255;
 Require(!sound::ParseRecordingBank(bank.data(),bank.size(),"Bow0",out),"Oversized sample count accepted");
 Require(sound::RecordedEffect(Q::Strike).empty() && sound::RecordedEffect(Q::HeavyHit).empty(),"Sword sound was replaced");
 master->DestroyVoice();engine->Release();CoUninitialize();
 std::cout<<"PASS: 51 recorded variations; native muted ADPCM playback and loop boundaries; corrupt input rejected. Music "
          <<musicBytes<<" compressed bytes, recordings "<<pcmBytes<<" PCM bytes.\n";
}
