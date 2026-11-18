#pragma once
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <xapo.h>

namespace sound {
constexpr unsigned roomSampleRate=48000;
XAUDIO2FX_REVERB_PARAMETERS ShortRoomParameters();
// One shared, wet-only stereo room. Effects own their sends; scores and UI
// never enter this bus. No extra effect samples or delay lines per actor.
class RoomReverb {
public:
  ~RoomReverb();
  bool Initialize(IXAudio2* engine,IXAudio2MasteringVoice* master);
  IXAudio2SubmixVoice* Voice() const{return voice_;}
  // Caller must stop the engine before resetting the XAPO processing state.
  void Reset();
  void Shutdown();
private:
  IXAudio2SubmixVoice* voice_=nullptr;
  IXAPO* processor_=nullptr;
};
}
