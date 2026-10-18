#pragma once
#include "game/Simulation.h"
#include <xaudio2.h>
#include <array>
#include <cstdint>
#include <vector>

namespace sound {
constexpr unsigned sampleRate=22050;
enum class Cue { Strike,Heavy,Enemy,Hit,HeavyHit,Hurt,Interrupt,Mend,Potion,Step,Jump,Land,Dash,Count };
std::vector<std::int16_t> Generate(Cue cue);
std::vector<std::int16_t> GenerateMusic();
// Pure event collection, independent of the audio device and playback.
struct Events {
  std::array<Cue,24> cues{}; unsigned count=0;
  float stepDistance=0;
  void Reset() { *this={}; }
  void Update(const rpg::Simulation& state);
};
}
class Audio {
public:
  ~Audio();
  bool Initialize();
  void Suspend(bool value);
  void Update(const rpg::Simulation& state,unsigned generation,float musicVolume,float effectsVolume);
private:
  IXAudio2* engine_=nullptr;
  IXAudio2MasteringVoice* master_=nullptr;
  IXAudio2SourceVoice* music_=nullptr;
  std::array<IXAudio2SourceVoice*,12> voices_{};
  std::array<std::vector<std::int16_t>,size_t(sound::Cue::Count)> samples_;
  std::vector<std::int16_t> score_;
  sound::Events events_;
  unsigned generation_=~0u;
  bool suspended_=false;
  bool ownsCom_=false;
  float effectGain_=.1f;
  void ClearEffects();
};
