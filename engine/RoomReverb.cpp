#include "engine/RoomReverb.h"

namespace sound {
XAUDIO2FX_REVERB_PARAMETERS ShortRoomParameters(){
  XAUDIO2FX_REVERB_PARAMETERS p{};
  p.WetDryMix=100;p.ReflectionsDelay=18;p.ReverbDelay=24;p.RearDelay=3;
  p.PositionLeft=6;p.PositionRight=6;p.PositionMatrixLeft=27;p.PositionMatrixRight=27;
  p.EarlyDiffusion=8;p.LateDiffusion=10;p.LowEQGain=8;p.LowEQCutoff=4;
  p.HighEQGain=5;p.HighEQCutoff=6;p.RoomFilterFreq=5000;p.RoomFilterMain=-2;p.RoomFilterHF=-8;
  p.ReflectionsGain=-4;p.ReverbGain=-8;p.DecayTime=.72f;p.Density=70;p.RoomSize=35;
  return p;
}
RoomReverb::~RoomReverb(){Shutdown();}
bool RoomReverb::Initialize(IXAudio2* engine,IXAudio2MasteringVoice* master){
  IUnknown* effect=nullptr;
  if(!engine||!master||FAILED(XAudio2CreateReverb(&effect)))return false;
  const HRESULT queried=effect->QueryInterface(__uuidof(IXAPO),reinterpret_cast<void**>(&processor_));
  XAUDIO2_EFFECT_DESCRIPTOR descriptor{effect,TRUE,2};XAUDIO2_EFFECT_CHAIN chain{1,&descriptor};
  XAUDIO2_SEND_DESCRIPTOR output{0,master};XAUDIO2_VOICE_SENDS sends{1,&output};
  const HRESULT created=SUCCEEDED(queried)?engine->CreateSubmixVoice(&voice_,2,roomSampleRate,0,0,&sends,&chain):queried;
  effect->Release();
  if(FAILED(created)){Shutdown();return false;}
  const auto parameters=ShortRoomParameters();
  if(FAILED(voice_->SetEffectParameters(0,&parameters,sizeof(parameters)))){Shutdown();return false;}
  return true;
}
void RoomReverb::Reset(){if(processor_)processor_->Reset();}
void RoomReverb::Shutdown(){
  if(voice_){voice_->DestroyVoice();voice_=nullptr;}
  if(processor_){processor_->Release();processor_=nullptr;}
}
}
