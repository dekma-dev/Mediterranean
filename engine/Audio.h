#pragma once
#include "game/Simulation.h"
#include <xaudio2.h>
#include <array>
#include <cstdint>
#include <vector>

namespace sound {
constexpr unsigned sampleRate=22050;
constexpr unsigned musicSeconds=48;
enum class Cue { Strike,Heavy,Enemy,Hit,HeavyHit,Hurt,Interrupt,Mend,Potion,Step,Jump,Land,Dash,Cast,Bow,Pistol,
  Holy,Fire,Thorn,Soul,Mind,Song,Rend,Ward,SpellHit,StepGrass,StepDirt,StepWood,LandGrass,LandDirt,LandWood,Count };
bool IsStep(Cue cue);
bool IsLanding(Cue cue);
Cue GroundCue(const rpg::WorldMap& map,rpg::Vec2 position,bool landing=false);
Cue ClassReleaseCue(rpg::ClassId school);
Cue ClassRecoveryCue(rpg::ClassId school);
std::vector<std::int16_t> Generate(Cue cue);
std::vector<std::int16_t> GenerateMusic();
std::vector<std::int16_t> GenerateCombatLayer();
enum class Ambience { Wind,Village,Astral,Count };
std::vector<std::int16_t> GenerateAmbience(Ambience kind);
std::array<float,3> AmbientWeights(rpg::Vec2 position);
struct AmbientMix {
  std::array<float,3> levels{};
  void Update(rpg::Vec2 position,float combat,float seconds);
  void Reset() {levels.fill(0);}
};
bool CombatThreat(const rpg::Simulation& state);
struct MusicIntensity {
  float value=0,hold=0;
  void Update(bool threat,float seconds);
  void Reset() {value=hold=0;}
};
float CueLevel(Cue cue);
float CuePitch(Cue cue,unsigned sequence);
float MixGain(float previous,float peakLoad,float seconds);
struct Placement { float gain=1,left=.70710678f,right=.70710678f; };
Placement Place(rpg::Vec2 offset,float cameraYaw);
// Pure event collection, independent of the audio device and playback.
struct Events {
  std::array<Cue,24> cues{}; unsigned count=0;
  std::array<rpg::Vec2,24> origins{};
  std::array<bool,24> spatial{};
  float stepDistance=0;
  void Reset() { *this={}; }
  void Update(const rpg::Simulation& state);
};
}
class Audio {
public:
  ~Audio();
  bool Initialize(bool muteOutput=false);
  void Suspend(bool value);
  void Update(const rpg::Simulation& state,unsigned generation,float musicVolume,float effectsVolume,float seconds=1.f/60,float cameraYaw=0);
private:
  IXAudio2* engine_=nullptr;
  IXAudio2MasteringVoice* master_=nullptr;
  IXAudio2SourceVoice* music_=nullptr;
  IXAudio2SourceVoice* combatMusic_=nullptr;
  std::array<IXAudio2SourceVoice*,3> ambience_{};
  std::array<std::vector<std::int16_t>,3> ambienceSamples_;
  sound::AmbientMix ambientMix_;
  std::array<IXAudio2SourceVoice*,12> voices_{};
  std::array<float,12> voiceLevels_{},voicePeaks_{};
  std::array<rpg::Vec2,12> origins_{};
  std::array<bool,12> spatial_{};
  unsigned outputChannels_=0;
  std::array<float,size_t(sound::Cue::Count)> peaks_{};
  std::array<unsigned,size_t(sound::Cue::Count)> variations_{};
  std::array<std::vector<std::int16_t>,size_t(sound::Cue::Count)> samples_;
  std::vector<std::int16_t> score_;
  std::vector<std::int16_t> combatScore_;
  sound::MusicIntensity intensity_;
  sound::Events events_;
  unsigned generation_=~0u;
  bool suspended_=false;
  bool ownsCom_=false;
  float effectGain_=.85f;
  void ClearEffects();
};
