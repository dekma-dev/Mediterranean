#include "engine/Audio.h"
#include <algorithm>
#include <cmath>

namespace sound {
namespace {
constexpr float pi=3.14159265359f;
float Tone(float hz,float t) { return std::sin(2*pi*hz*t); }
std::int16_t Pcm(float value) { return std::int16_t(std::clamp(value,-.95f,.95f)*32767); }
}
float Sweep(float from,float to,float duration,float t) {
  // Integrate frequency so falling and rising sweeps have continuous phase.
  return std::sin(2*pi*(from*t+.5f*(to-from)*t*t/duration));
}
float Decay(float t,float attack,float rate) {
  return t<0?0:std::min(t/attack,1.f)*std::exp(-rate*t);
}
float Bell(float hz,float t) {
  if(t<0) return 0;
  return Decay(t,.006f,5.f)*(Tone(hz,t)+.28f*Tone(hz*2.01f,t)+.09f*Tone(hz*3.98f,t));
}
float Metal(float hz,float t) {
  if(t<0) return 0;
  // Inharmonic modes with separately damped partials: a short metal contact,
  // distinct from the musical bell stack reserved for magic.
  return Decay(t,.002f,18.f)*(Tone(hz,t)+.45f*Tone(hz*1.47f,t)*std::exp(-12*t)+
    .22f*Tone(hz*2.09f,t)*std::exp(-30*t));
}
float CueLevel(Cue cue) {
  return cue==Cue::Step?.35f:cue==Cue::Jump?.55f:cue==Cue::Land?.75f:1.f;
}
float CuePitch(Cue cue,unsigned sequence) {
  constexpr float offsets[]={-.65f,.35f,-.2f,.8f,0.f,-.4f,.55f};
  const float range=cue==Cue::Mend || cue==Cue::Potion || cue==Cue::Interrupt?.012f:
    cue==Cue::Step?.085f:.045f;
  return 1+offsets[sequence%7]*range;
}
float MixGain(float previous,float peakLoad,float seconds) {
  const float target=std::min(.85f,.70f/std::max(.0001f,peakLoad));
  // Attenuate immediately before voices start; recover over 180 ms as tails
  // finish, avoiding sudden upward gain jumps when a buffer leaves the pool.
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.1f):0;
  return previous>=target?target:previous+(target-previous)*-std::expm1(-dt/.18f);
}
std::vector<std::int16_t> Generate(Cue cue) {
  const bool healing=cue==Cue::Mend || cue==Cue::Potion;
  const float duration=cue==Cue::Mend?1.65f:healing?.95f:cue==Cue::Interrupt?.85f:
    cue==Cue::HeavyHit?.7f:cue==Cue::Step?.18f:cue==Cue::Land?.38f:cue==Cue::Jump?.3f:.48f;
  std::vector<std::int16_t> result(size_t(duration*sampleRate));
  std::uint32_t seed=12345+unsigned(cue)*713;
  float low=0,body=0;
  for (size_t i=0;i<result.size();++i) {
    const float t=float(i)/sampleRate;
    seed^=seed<<13; seed^=seed>>17; seed^=seed<<5;
    const float noise=float(seed&65535)/32767.5f-1;
    low+=.22f*(noise-low); body+=.035f*(noise-body);
    const float air=noise-low;
    float value=0;
    switch(cue) {
    case Cue::Mend:
      // Ascending triad and quiet reflections baked into the sample.
      for(int n=0;n<3;++n) {
        const float hz=n==0?523.25f:n==1?659.25f:783.99f;
        value+=.16f*Bell(hz,t-n*.12f)+.045f*Bell(hz,t-n*.12f-.19f);
      }
      value+=.08f*low*Decay(t,.09f,6.f);
      break;
    case Cue::Potion:
      value=.16f*Bell(1450,t)+.13f*body*Decay(t,.002f,30.f);
      for(int n=0;n<4;++n) {
        const float local=t-.09f-n*.12f;
        if(local>=0) value+=.19f*Sweep(420.f+n*45,170.f,.12f,local)*Decay(local,.008f,32.f);
      }
      value+=.09f*Bell(1046.5f,t-.48f);
      break;
    case Cue::Strike: case Cue::Heavy: case Cue::Enemy: case Cue::Dash: {
      const bool heavy=cue==Cue::Heavy,dash=cue==Cue::Dash;
      const float swell=std::pow(std::sin(pi*std::min(t/(heavy?.38f:.28f),1.f)),2.f);
      value=swell*((dash?.28f:.18f)*air+(heavy?1.1f:.65f)*low);
      value+=.12f*Sweep(heavy?210.f:450.f,dash?90.f:140.f,duration,t)*Decay(t,.025f,10.f);
      if(!dash) value+=.08f*Metal(cue==Cue::Enemy?620.f:heavy?780.f:1250.f,t-.07f);
      break;
    }
    case Cue::Hit: case Cue::HeavyHit: case Cue::Hurt: {
      const bool heavy=cue==Cue::HeavyHit,hurt=cue==Cue::Hurt;
      value=.44f*Sweep(heavy?125.f:190.f,heavy?38.f:65.f,duration,t)*Decay(t,.003f,heavy?12.f:23.f);
      value+=.26f*air*Decay(t,.0015f,65.f)+.65f*body*Decay(t,.004f,19.f);
      if(!hurt) value+=.18f*Metal(heavy?430.f:1180.f,t)+.07f*Metal(heavy?647.f:1777.f,t-.018f);
      break;
    }
    case Cue::Interrupt:
      value=.18f*Bell(1174.66f,t)+.14f*Bell(1567.98f,t-.045f)+.11f*Bell(2093,t-.09f);
      value+=.25f*air*Decay(t,.002f,35.f);
      break;
    case Cue::Step: case Cue::Land: {
      const bool land=cue==Cue::Land;
      value=.34f*Sweep(land?105.f:155.f,55.f,duration,t)*Decay(t,.003f,land?23.f:48.f);
      value+=.65f*low*Decay(t,.002f,land?22.f:42.f);
      value+=.12f*air*Decay(t-.025f,.006f,45.f);
      break;
    }
    case Cue::Jump:
      value=.45f*low*Decay(t,.013f,22.f)+.16f*Sweep(120,280,duration,t)*Decay(t,.01f,18.f);
      break;
    default:break;
    }
    const float edge=std::min({1.f,t/.002f,(duration-t)/.055f});
    result[i]=Pcm(value*edge);
  }
  result.front()=result.back()=0;
  return result;
}
std::vector<std::int16_t> GenerateMusic() {
  // Original 32-second ambient phrase: soft minor pads and plucked melody.
  constexpr float roots[]={110,87.307f,130.813f,97.999f};
  constexpr float melody[]={440,523.25f,659.25f,587.33f,523.25f,440,392,329.63f,
    523.25f,659.25f,783.99f,659.25f,587.33f,493.88f,392,440};
  std::vector<std::int16_t> result(sampleRate*32);
  for (size_t i=0;i<result.size();++i) {
    const float t=float(i)/sampleRate,local=std::fmod(t,8.f),noteTime=std::fmod(t,2.f);
    const int chord=std::min(3,int(t/8)); const float root=roots[chord];
    const float pad=std::pow(std::sin(pi*local/8),2.f);
    const float third=chord==0?1.189207f:1.259921f;
    float value=.075f*pad*(Tone(root,t)+.65f*Tone(root*third,t)+.45f*Tone(root*1.498307f,t));
    value+=.13f*std::min(noteTime/.025f,1.f)*std::exp(-2.7f*noteTime)*
      (Tone(melody[std::min(15,int(t/2))],noteTime)+.2f*Tone(melody[std::min(15,int(t/2))]*2,noteTime));
    value*=std::min({1.f,t/.15f,(32-t)/.35f});
    result[i]=Pcm(value);
  }
  result.front()=result.back()=0;
  return result;
}
void Events::Update(const rpg::Simulation& state) {
  count=0;
  const auto add=[&](Cue cue) { if(count<cues.size()) cues[count++]=cue; };
  using K=rpg::CombatVisualKind;
  for(size_t i=0;i<state.VisualEventCount();++i) {
    const auto& event=state.VisualEvents()[i];
    const auto p=state.Player();
    if(std::hypot(event.position.x-p.x,event.position.z-p.z)>18) continue;
    switch(event.kind) {
    case K::Strike:add(Cue::Strike);break; case K::HeavyStrike:add(Cue::Heavy);break;
    case K::EnemySwing:add(Cue::Enemy);break; case K::Hit:add(Cue::Hit);break;
    case K::HeavyHit:add(Cue::HeavyHit);break; case K::Hurt:add(Cue::Hurt);break;
    case K::Interrupt:add(Cue::Interrupt);break; case K::Mend:add(Cue::Mend);break;
    case K::Potion:add(Cue::Potion);break;
    // Dash activation already has its authoritative one-shot cue below.
    case K::DashStart: case K::DashStop: case K::DashImpact: break;
    }
  }
  if(state.Health()>0) {
    if(state.JumpedThisUpdate()) add(Cue::Jump);
    if(state.LandingSpeed()>0) add(Cue::Land);
    if(state.DashedThisUpdate()) add(Cue::Dash);
    if(state.Grounded()) {
      stepDistance+=state.GroundMovementDistance();
      if(stepDistance>=1.35f) { stepDistance=std::fmod(stepDistance,1.35f); add(Cue::Step); }
    } else stepDistance=0;
    if(state.GroundMovementDistance()==0) stepDistance=0;
  } else stepDistance=0;
}
}
Audio::~Audio() {
  for(auto* voice:voices_) if(voice) voice->DestroyVoice();
  if(music_) music_->DestroyVoice();
  if(master_) master_->DestroyVoice();
  if(engine_) engine_->Release();
  if(ownsCom_) CoUninitialize();
}
bool Audio::Initialize(bool muteOutput) {
  const HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  ownsCom_=SUCCEEDED(com);
  if(FAILED(com) && com!=RPC_E_CHANGED_MODE) return false;
  if(FAILED(XAudio2Create(&engine_,0,XAUDIO2_DEFAULT_PROCESSOR)) || FAILED(engine_->CreateMasteringVoice(&master_))) return false;
  if(muteOutput && FAILED(master_->SetVolume(0))) return false;
  WAVEFORMATEX format{}; format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=1;
  format.nSamplesPerSec=sound::sampleRate; format.wBitsPerSample=16;
  format.nBlockAlign=2; format.nAvgBytesPerSec=sound::sampleRate*2;
  for(size_t i=0;i<samples_.size();++i) samples_[i]=sound::Generate(sound::Cue(i));
  for(size_t i=0;i<samples_.size();++i)
    for(auto value:samples_[i]) peaks_[i]=std::max(peaks_[i],std::abs(int(value))/32767.f);
  score_=sound::GenerateMusic();
  for(auto*& voice:voices_) if(FAILED(engine_->CreateSourceVoice(&voice,&format))) return false;
  if(FAILED(engine_->CreateSourceVoice(&music_,&format))) return false;
  XAUDIO2_BUFFER buffer{}; buffer.AudioBytes=UINT32(score_.size()*2); buffer.pAudioData=reinterpret_cast<const BYTE*>(score_.data());
  buffer.LoopCount=XAUDIO2_LOOP_INFINITE;
  music_->SetVolume(0);
  return SUCCEEDED(music_->SubmitSourceBuffer(&buffer)) && SUCCEEDED(music_->Start());
}
void Audio::ClearEffects() {
  for(auto* voice:voices_) if(voice) { voice->Stop(); voice->FlushSourceBuffers(); }
  voiceLevels_.fill(0); voicePeaks_.fill(0); effectGain_=.85f;
}
void Audio::Suspend(bool value) {
  if(!engine_ || value==suspended_) return;
  suspended_=value;
  if(value) { engine_->StopEngine(); ClearEffects(); events_.Reset(); generation_=~0u; }
  else engine_->StartEngine();
}
void Audio::Update(const rpg::Simulation& state,unsigned generation,float musicVolume,float effectsVolume,float seconds) {
  if(!music_ || suspended_) return;
  music_->SetVolume(std::clamp(musicVolume,0.f,1.f)*.30f);
  if(generation!=generation_) {
    generation_=generation; ClearEffects(); events_.Reset(); variations_.fill(0);
  }
  events_.Update(state);
  if(effectsVolume<=0) { ClearEffects(); return; }
  // Apply the voice changes as one XAudio2 operation set so a new effect never
  // starts with the preceding frame's louder sparse-mix gain.
  constexpr UINT32 mixOperation=1;
  for(unsigned i=0;i<events_.count;++i) for(size_t v=0;v<voices_.size();++v) {
    auto* voice=voices_[v];
    XAUDIO2_VOICE_STATE status{}; voice->GetState(&status,XAUDIO2_VOICE_NOSAMPLESPLAYED);
    if(status.BuffersQueued) continue;
    const auto cue=events_.cues[i]; const auto index=size_t(cue);
    const auto& pcm=samples_[index];
    voice->Stop();
    voice->SetFrequencyRatio(sound::CuePitch(cue,variations_[index]++),mixOperation);
    XAUDIO2_BUFFER buffer{}; buffer.Flags=XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes=UINT32(pcm.size()*2); buffer.pAudioData=reinterpret_cast<const BYTE*>(pcm.data());
    if(SUCCEEDED(voice->SubmitSourceBuffer(&buffer))) {
      voiceLevels_[v]=sound::CueLevel(cue); voicePeaks_[v]=peaks_[index];
      voice->Start(0,mixOperation);
    }
    break;
  }
  float peakLoad=0;
  for(size_t v=0;v<voices_.size();++v) {
    XAUDIO2_VOICE_STATE status{}; voices_[v]->GetState(&status,XAUDIO2_VOICE_NOSAMPLESPLAYED);
    if(!status.BuffersQueued) voiceLevels_[v]=voicePeaks_[v]=0;
    peakLoad+=voiceLevels_[v]*voicePeaks_[v];
  }
  effectGain_=sound::MixGain(effectGain_,peakLoad,seconds);
  for(size_t v=0;v<voices_.size();++v)
    voices_[v]->SetVolume(std::clamp(effectsVolume,0.f,1.f)*effectGain_*voiceLevels_[v],mixOperation);
  engine_->CommitChanges(mixOperation);
}
