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
bool IsStep(Cue cue) {return cue==Cue::Step || (cue>=Cue::StepGrass && cue<=Cue::StepWood);}
bool IsLanding(Cue cue) {return cue==Cue::Land || (cue>=Cue::LandGrass && cue<=Cue::LandWood);}
Cue GroundCue(const rpg::WorldMap& map,rpg::Vec2 p,bool landing) {
  if(!std::isfinite(p.x) || !std::isfinite(p.z)) return landing?Cue::LandGrass:Cue::StepGrass;
  if(p.x>=rpg::WorldMap::dockMin.x && p.x<=rpg::WorldMap::dockMax.x &&
     p.z>=rpg::WorldMap::dockMin.z && p.z<=rpg::WorldMap::dockMax.z)
    return landing?Cue::LandWood:Cue::StepWood;
  const auto surface=map.SurfaceAt(p);
  if(surface.paving>=.5f) return landing?Cue::Land:Cue::Step;
  if(surface.dirt>=.5f) return landing?Cue::LandDirt:Cue::StepDirt;
  return landing?Cue::LandGrass:Cue::StepGrass;
}
float CueLevel(Cue cue) {
  return cue==Cue::UiClick?.35f:IsStep(cue)?.35f:cue==Cue::Jump?.55f:IsLanding(cue)?.75f:1.f;
}
float CuePitch(Cue cue,unsigned sequence) {
  constexpr float offsets[]={-.65f,.35f,-.2f,.8f,0.f,-.4f,.55f};
  const float range=cue==Cue::Mend || cue==Cue::Potion || cue==Cue::Interrupt || cue==Cue::Song || cue==Cue::Holy?.012f:
    IsStep(cue)?.085f:.045f;
  return 1+offsets[sequence%7]*range;
}
float MixGain(float previous,float peakLoad,float seconds) {
  const float target=std::min(.85f,.70f/std::max(.0001f,peakLoad));
  // Attenuate immediately before voices start; recover over 180 ms as tails
  // finish, avoiding sudden upward gain jumps when a buffer leaves the pool.
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.1f):0;
  return previous>=target?target:previous+(target-previous)*-std::expm1(-dt/.18f);
}
Placement Place(rpg::Vec2 offset,float cameraYaw) {
  if(!std::isfinite(offset.x) || !std::isfinite(offset.z) || !std::isfinite(cameraYaw)) return {0,0,0};
  const float distance=std::hypot(offset.x,offset.z);
  // Preserve nearby warning readability; soften the edge of the 18 m range.
  const float fade=std::clamp((18.f-distance)/4.f,0.f,1.f);
  const float gain=fade*fade*(3-2*fade)/(1+.025f*distance*distance);
  const float pan=std::clamp((offset.x*std::cos(cameraYaw)-offset.z*std::sin(cameraYaw))/std::max(1.f,distance),-1.f,1.f);
  const float angle=(pan+1)*pi*.25f;
  return {gain,std::cos(angle),std::sin(angle)};
}
std::vector<std::int16_t> Generate(Cue cue) {
  const bool healing=cue==Cue::Mend || cue==Cue::Potion;
  const bool mystical=(cue>=Cue::Holy && cue<=Cue::Ward) || cue==Cue::Cast || cue==Cue::Mend || cue==Cue::Interrupt;
  const float duration=cue==Cue::ReserveReady?.65f:cue==Cue::CastCancelled?.22f:cue==Cue::UiClick?.12f:cue==Cue::Mend?1.65f:healing?.95f:cue==Cue::Interrupt?.85f:
    mystical?1.05f:cue==Cue::HeavyHit?.7f:IsStep(cue)?.18f:IsLanding(cue)?.38f:cue==Cue::Jump?.3f:cue==Cue::SpellHit?.28f:.48f;
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
    case Cue::ReserveReady:
      value=.12f*Bell(293.665f,t)+.07f*Bell(440.f,t-.045f)+.045f*body*Decay(t,.01f,14.f);break;
    case Cue::CastCancelled:
      value=.18f*body*Decay(t,.008f,24.f)+.04f*air*Decay(t,.01f,32.f);break;
    case Cue::UiClick:
      value=.20f*body*Decay(t,.001f,60.f)+.12f*Metal(430,t)+.055f*air*Decay(t,.002f,95.f);
      break;
    case Cue::Holy:
      value=.18f*Bell(783.99f,t)+.11f*Bell(1174.66f,t-.024f)+.09f*low*Decay(t,.018f,18.f);
      break;
    case Cue::Fire:
      value=.55f*low*Decay(t,.012f,11.f)+.18f*air*Decay(t,.008f,18.f);
      value+=.17f*Sweep(180,55,duration,t)*Decay(t,.012f,12.f);
      break;
    case Cue::Thorn:
      value=.36f*air*Decay(t,.002f,45.f)+.18f*Sweep(920,160,duration,t)*Decay(t,.005f,25.f);
      value+=.10f*Bell(587.33f,t-.065f);
      break;
    case Cue::Soul:
      value=.16f*Sweep(240,420,duration,t)*Decay(t,.035f,8.f)+.12f*Sweep(244,414,duration,t)*Decay(t,.045f,9.f);
      value+=.16f*low*Decay(t,.04f,12.f);
      break;
    case Cue::Mind:
      value=.15f*Sweep(740,300,duration,t)*Decay(t,.015f,13.f)+.12f*Sweep(375,690,duration,t)*Decay(t,.02f,12.f);
      value+=.06f*Bell(1567.98f,t-.04f);
      break;
    case Cue::Song:
      for(int n=0;n<3;++n) value+=.14f*Bell(n==0?659.25f:n==1?783.99f:987.77f,t-n*.035f);
      break;
    case Cue::Rend:
      value=.35f*air*Decay(t,.012f,15.f)+.22f*Sweep(210,65,duration,t)*Decay(t,.015f,12.f);
      value+=.075f*Metal(370,t-.025f);
      break;
    case Cue::Ward:
      value=.16f*Bell(1046.5f,t)+.10f*Bell(1396.91f,t-.02f)+.14f*body*Decay(t,.002f,25.f);
      break;
    case Cue::SpellHit:
      value=.26f*Sweep(140,50,duration,t)*Decay(t,.003f,26.f)+.28f*low*Decay(t,.003f,30.f);
      value+=.12f*air*Decay(t,.002f,45.f);
      break;
    case Cue::Bow:
      value=.22f*Sweep(460,140,duration,t)*Decay(t,.002f,22.f);
      value+=.14f*Tone(840,t)*Decay(t,.003f,38.f)+.27f*air*Decay(t,.002f,65.f);
      break;
    case Cue::Pistol:
      value=.58f*air*Decay(t,.002f,80.f)+.65f*low*Decay(t,.002f,36.f);
      value+=.22f*Sweep(155,55,duration,t)*Decay(t,.003f,24.f)+.065f*Metal(1150,t-.035f);
      break;
    case Cue::Cast:
      value=.10f*Sweep(165,440,duration,t)*Decay(t,.08f,6.f);
      value+=.08f*Bell(659.25f,t-.09f)+.18f*body*Decay(t,.08f,6.f);
      break;
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
    case Cue::StepGrass: case Cue::LandGrass: {
      const bool land=IsLanding(cue);
      value=.22f*body*Decay(t,.006f,land?18.f:35.f)+.28f*low*Decay(t,.004f,land?23.f:40.f);
      value+=.14f*air*Decay(t-.018f,.012f,land?22.f:32.f);
      value+=.10f*Sweep(90,45,duration,t)*Decay(t,.006f,land?24.f:48.f);
      break;
    }
    case Cue::StepDirt: case Cue::LandDirt: {
      const bool land=IsLanding(cue);
      value=.38f*low*Decay(t,.003f,land?21.f:40.f)+.21f*air*Decay(t,.003f,land?25.f:42.f);
      value+=.18f*Sweep(120,50,duration,t)*Decay(t,.004f,land?25.f:45.f);
      value+=.10f*air*Decay(t-.035f,.005f,48.f);
      break;
    }
    case Cue::StepWood: case Cue::LandWood: {
      const bool land=IsLanding(cue);
      value=(.23f*Tone(165,t)+.11f*Tone(291,t)+.06f*Tone(457,t))*Decay(t,.002f,land?17.f:32.f);
      value+=.35f*low*Decay(t,.002f,land?28.f:55.f)+.10f*air*Decay(t,.002f,70.f);
      break;
    }
    case Cue::Step: case Cue::Land: {
      const bool land=IsLanding(cue);
      value=.34f*Sweep(land?105.f:155.f,55.f,duration,t)*Decay(t,.003f,land?23.f:48.f);
      value+=.65f*low*Decay(t,.002f,land?22.f:42.f);
      value+=.12f*air*Decay(t-.025f,.006f,45.f);
      break;
    }
    case Cue::Jump:
      // Cloth and boot push-off, rather than a rising electronic jump tone.
      value=.48f*low*Decay(t,.013f,22.f)+.12f*body*Decay(t,.025f,15.f);
      value+=.065f*Sweep(95,48,duration,t)*Decay(t,.008f,24.f);
      break;
    default:break;
    }
    if(mystical) {
      // Dark fundamental below the readable attack, with a slower resonant
      // bloom. Pitch families retain holy/nature/arcane identity.
      const float root=cue==Cue::Holy || cue==Cue::Ward?146.83f:cue==Cue::Thorn?130.81f:
        cue==Cue::Song?164.81f:cue==Cue::Fire || cue==Cue::Rend?73.42f:110.f;
      value=.70f*value+.09f*Decay(t,.07f,4.8f)*(Tone(root,t)+.35f*Tone(root*1.5f,t));
      value+=.13f*body*Decay(t,.04f,6.f);
      // Inharmonic low resonances and a soft airy onset replace a pure-tone beep.
      value+=.035f*Tone(root*2.013f,t)*Decay(t,.035f,7.f);
      value+=.035f*Tone(root*3.927f,t)*Decay(t,.02f,12.f);
      value+=.055f*air*Decay(t,.012f,28.f);
      // Quiet, dark reflections are baked once, never processed in the mixer.
      for(const auto tap:{std::pair<float,float>{.097f,.13f},{.173f,.09f},{.271f,.06f}}) {
        const size_t delay=size_t(tap.first*sampleRate);
        if(i>=delay) value+=float(result[i-delay])/32767.f*tap.second;
      }
    } else if(cue==Cue::Hit || cue==Cue::HeavyHit || cue==Cue::Enemy) {
      value=.86f*value+.13f*Sweep(cue==Cue::HeavyHit?88.f:125.f,42.f,duration,t)*Decay(t,.004f,18.f);
    }
    const float edge=std::min({1.f,t/.002f,(duration-t)/.055f});
    result[i]=Pcm(value*edge);
  }
  result.front()=result.back()=0;
  return result;
}
std::vector<std::int16_t> GenerateMusic() {
  // Original eight-bar, 80 BPM chronicle in D minor. Layered bowed partials,
  // breath-like choir, distant horns, harp and sparse frame drums. Every note
  // is evaluated in local time; releases overlap the next harmony naturally.
  constexpr float roots[]={146.832f,116.541f,174.614f,130.813f,146.832f,116.541f,110.f,146.832f};
  constexpr float melody[]={293.665f,349.228f,440.f,391.995f,349.228f,293.665f,261.626f,220.f,
    349.228f,440.f,523.251f,440.f,391.995f,329.628f,277.183f,293.665f};
  const auto envelope=[](float t,float attack,float sustain,float release) {
    if(t<0 || t>=sustain+release) return 0.f;
    const float rise=std::clamp(t/attack,0.f,1.f),fall=std::clamp((sustain+release-t)/release,0.f,1.f);
    return rise*rise*(3-2*rise)*fall*fall*(3-2*fall);
  };
  const auto bowed=[](float hz,float t) {
    const float phase=t+.00032f*Tone(4.7f,t);
    return Tone(hz,phase)+.27f*Tone(hz*2,phase)+.12f*Tone(hz*3,phase)+.055f*Tone(hz*4,phase);
  };
  std::vector<std::int16_t> result(sampleRate*musicSeconds);
  std::uint32_t seed=0x41535452;float breath=0;
  for (size_t i=0;i<result.size();++i) {
    const float t=float(i)/sampleRate;
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    breath+=.04f*(float(seed&65535)/32767.5f-1-breath);
    float value=0;
    const int bar=std::min(7,int(t/6));
    for(int b=std::max(0,bar-1);b<=bar;++b) {
      const float local=t-b*6.f,root=roots[b],third=(b==0 || b==4 || b==7)?1.189207f:1.259921f;
      const float swell=envelope(local,1.3f,5.4f,1.6f);
      value+=.043f*swell*(bowed(root,local)+.65f*bowed(root*third,local)+.55f*bowed(root*1.498307f,local));
      value+=.026f*swell*(Tone(root*.5f,local)+.55f*Tone(root*2,local)+breath*.6f);
      if(b>=4) value+=.038f*envelope(local,.32f,2.5f,1.5f)*
        (Tone(root*.5f,local)+.4f*Tone(root,local)+.18f*Tone(root*1.5f,local));
    }
    const int note=std::min(15,int(t/3));
    for(int n=std::max(0,note-1);n<=note;++n) {
      const float local=t-n*3.f,hz=melody[n];
      value+=.047f*envelope(local,.24f,1.9f,1.5f)*(Tone(hz,local+.0004f*Tone(5.f,local))+.18f*Tone(hz*2,local));
      value+=.024f*Bell(hz*2,local-.42f);
    }
    // Sparse low percussion enters in the second half and leaves room for SFX.
    if(t>=24 && t<44) {
      const float beat=std::fmod(t,1.5f);
      value+=.068f*Sweep(78,38,.6f,beat)*Decay(beat,.008f,9.f)+.06f*breath*Decay(beat,.008f,18.f);
    }
    // Long, quiet chamber reflections, bounded to the generated buffer.
    for(const auto tap:{std::pair<float,float>{.337f,.12f},{.593f,.08f}}) {
      const size_t delay=size_t(tap.first*sampleRate);
      if(i>=delay) value+=float(result[i-delay])/32767.f*tap.second;
    }
    value*=std::min({1.f,t/1.5f,(musicSeconds-t)/2.8f});
    result[i]=Pcm(value);
  }
  result.front()=result.back()=0;
  return result;
}
Cue ClassRecoveryCue(rpg::ClassId school) {
  using C=rpg::ClassId;
  switch(school) {
    case C::Paladin:case C::Priest:return Cue::Holy;
    case C::Warrior:case C::Scout:case C::Mage:case C::Engineer:return Cue::Ward;
    case C::Psionic:return Cue::Mind;
    case C::Bard:return Cue::Song;
    case C::Necromancer:case C::Demonologist:return Cue::Soul;
    default:return Cue::Mend;
  }
}
void MusicIntensity::Update(bool threat,float seconds) {
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.1f):0;
  if(threat) hold=3.f;
  const float held=std::min(hold,dt);
  if(threat || held>0) value+=(1-value)*-std::expm1(-(threat?dt:held)/1.2f);
  if(!threat) {
    hold=std::max(0.f,hold-dt);
    value*=std::exp(-(dt-held)/3.f);
  }
}
bool CombatThreat(const rpg::Simulation& state) {
  if(state.Health()<=0) return false;
  if(state.Attacking()) return true;
  const auto player=state.Player();
  for(const auto& enemy:state.Enemies())
    if(enemy.health>0 && enemy.state==rpg::EnemyState::Pursuing &&
      std::hypot(enemy.position.x-player.x,enemy.position.z-player.z)<18) return true;
  return false;
}
std::array<float,3> AmbientWeights(rpg::Vec2 p) {
  if(!std::isfinite(p.x) || !std::isfinite(p.z)) return {};
  const auto proximity=[&](rpg::Vec2 center,float radius) {
    const float t=std::clamp(1-std::hypot(p.x-center.x,p.z-center.z)/radius,0.f,1.f);
    return t*t*(3-2*t);
  };
  float village=proximity({0,-10},13),astral=std::max(proximity(rpg::WorldMap::astralAnchor,11),proximity({0,27},9));
  const float sum=village+astral;if(sum>1) {village/=sum;astral/=sum;}
  return {std::max(0.f,1-village-astral),village,astral};
}
void AmbientMix::Update(rpg::Vec2 p,float combat,float seconds) {
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.1f):0;
  const float duck=1-.5f*(std::isfinite(combat)?std::clamp(combat,0.f,1.f):0);
  const auto target=AmbientWeights(p);
  for(size_t i=0;i<levels.size();++i) levels[i]+=(target[i]*duck-levels[i])*-std::expm1(-dt/1.5f);
}
std::vector<std::int16_t> GenerateAmbience(Ambience kind) {
  std::vector<std::int16_t> result(sampleRate*6);
  std::uint32_t seed=0x57494e44+unsigned(kind)*1237;float slow=0,soft=0;
  for(size_t i=0;i<result.size();++i) {
    const float t=float(i)/sampleRate;
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    const float noise=float(seed&65535)/32767.5f-1;
    slow+=.008f*(noise-slow);soft+=.045f*(noise-soft);
    const float gust=.65f+.25f*Tone(1.f/6,t)+.10f*Tone(.5f,t);
    float value=.24f*slow*gust+.045f*soft;
    if(kind==Ambience::Village) {
      value*=.65f;
      for(float onset:{.7f,3.4f}) {
        const float local=t-onset;
        if(local>=0) value+=.024f*Metal(210,local)+.035f*soft*Decay(local,.01f,7.f);
      }
      value+=.017f*Bell(587.33f,t-1.8f);
    } else if(kind==Ambience::Astral) {
      value=.10f*slow+.025f*gust*(Tone(146.832f,t)+.38f*Tone(220,t)+.2f*Tone(293.665f,t));
      value+=.025f*Bell(880,t-1.f)+.018f*Bell(1174.66f,t-3.7f);
    } else {
      for(float onset:{1.2f,1.48f,4.1f}) {
        const float local=t-onset;
        if(local>=0 && local<.16f) value+=.022f*Sweep(1700,2100,.16f,local)*std::pow(std::sin(pi*local/.16f),2.f);
      }
    }
    value*=std::min({1.f,t/.25f,(6.f-t)/.35f});result[i]=Pcm(value);
  }
  result.front()=result.back()=0;return result;
}
std::vector<std::int16_t> GenerateCombatLayer() {
  // Two bars at the score's 80 BPM. Unpitched frame drums and low strikes
  // remain compatible with every harmony in the 48-second main phrase.
  std::vector<std::int16_t> result(sampleRate*6);
  std::uint32_t seed=0x4452554d;float low=0;
  for(size_t i=0;i<result.size();++i) {
    const float t=float(i)/sampleRate;
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    const float noise=float(seed&65535)/32767.5f-1;low+=.08f*(noise-low);
    float value=0;
    for(int beat=0;beat<8;++beat) {
      const float local=t-beat*.75f;if(local<0 || local>.65f) continue;
      const bool strong=beat%4==0;
      value+=(strong?.18f:.075f)*Sweep(strong?82.f:125.f,38.f,.65f,local)*Decay(local,.006f,strong?10.f:18.f);
      value+=(strong?.12f:.08f)*low*Decay(local,.004f,22.f);
      if(beat%2) value+=.025f*(noise-low)*Decay(local,.003f,34.f);
    }
    value*=std::min({1.f,t/.004f,(6.f-t)/.04f});result[i]=Pcm(value);
  }
  result.front()=result.back()=0;return result;
}
Cue ClassReleaseCue(rpg::ClassId school) {
  using C=rpg::ClassId;
  switch(school) {
    case C::Scout:return Cue::Bow;case C::Engineer:return Cue::Pistol;
    case C::Paladin:case C::Priest:return Cue::Holy;case C::Mage:return Cue::Fire;
    case C::Druid:return Cue::Thorn;case C::Necromancer:return Cue::Soul;
    case C::Psionic:return Cue::Mind;case C::Bard:return Cue::Song;
    case C::Demonologist:return Cue::Rend;default:return Cue::Cast;
  }
}
void Events::Update(const rpg::Simulation& state) {
  count=0;
  rpg::Vec2 origin{};bool placed=false;
  const auto add=[&](Cue cue) { if(count<cues.size()) {origins[count]=origin;spatial[count]=placed;cues[count++]=cue;} };
  using K=rpg::CombatVisualKind;
  for(size_t i=0;i<state.VisualEventCount();++i) {
    const auto& event=state.VisualEvents()[i];
    const auto p=state.Player();
    if(std::hypot(event.position.x-p.x,event.position.z-p.z)>18) continue;
    origin=event.position;
    placed=event.kind==K::EnemySwing || event.kind==K::Hit || event.kind==K::HeavyHit || event.kind==K::Interrupt;
    switch(event.kind) {
    case K::Strike:add(event.school==rpg::ClassId::Demonologist?Cue::Rend:Cue::Strike);break;
    case K::HeavyStrike:add(event.school==rpg::ClassId::Warrior?Cue::Heavy:ClassReleaseCue(event.school));break;
    case K::UiClick:add(Cue::UiClick);break;
    case K::ReserveReady:add(Cue::ReserveReady);break;
    case K::CastCancelled:add(Cue::CastCancelled);break;
    case K::CastStart:add(Cue::Cast);break;
    case K::SpellCast:add(ClassReleaseCue(event.school));break;
    case K::EnemySwing:add(Cue::Enemy);break;
    case K::Hit:case K::HeavyHit:
      add(rpg::DescribeClass(event.school).range>3 && event.school!=rpg::ClassId::Scout && event.school!=rpg::ClassId::Engineer?
        Cue::SpellHit:event.kind==K::HeavyHit?Cue::HeavyHit:Cue::Hit);break;
    case K::Hurt:add(Cue::Hurt);break;
    case K::BarrierHit:add(Cue::Ward);break;
    case K::Interrupt:add(Cue::Interrupt);break; case K::Mend:add(ClassRecoveryCue(event.school));break;
    case K::Potion:add(Cue::Potion);break;
    // Dash activation already has its authoritative one-shot cue below.
    case K::DashStart: case K::DashStop: case K::DashImpact: break;
    }
  }
  placed=false;
  if(state.Health()>0) {
    if(state.JumpedThisUpdate()) add(Cue::Jump);
    if(state.LandingSpeed()>0) add(GroundCue(state.Map(),state.Player(),true));
    if(state.DashedThisUpdate()) add(Cue::Dash);
    if(state.Grounded()) {
      stepDistance+=state.GroundMovementDistance();
      if(stepDistance>=1.35f) { stepDistance=std::fmod(stepDistance,1.35f); add(GroundCue(state.Map(),state.Player())); }
    } else stepDistance=0;
    if(state.GroundMovementDistance()==0) stepDistance=0;
  } else stepDistance=0;
}
}
Audio::~Audio() {
  for(auto* voice:ambience_) if(voice) voice->DestroyVoice();
  for(auto* voice:voices_) if(voice) voice->DestroyVoice();
  if(music_) music_->DestroyVoice();
  if(combatMusic_) combatMusic_->DestroyVoice();
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
  XAUDIO2_VOICE_DETAILS details{};master_->GetVoiceDetails(&details);outputChannels_=details.InputChannels;
  WAVEFORMATEX format{}; format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=1;
  format.nSamplesPerSec=sound::sampleRate; format.wBitsPerSample=16;
  format.nBlockAlign=2; format.nAvgBytesPerSec=sound::sampleRate*2;
  for(size_t i=0;i<samples_.size();++i) samples_[i]=sound::Generate(sound::Cue(i));
  for(size_t i=0;i<samples_.size();++i)
    for(auto value:samples_[i]) peaks_[i]=std::max(peaks_[i],std::abs(int(value))/32767.f);
  score_=sound::GenerateMusic();
  combatScore_=sound::GenerateCombatLayer();
  for(auto*& voice:voices_) if(FAILED(engine_->CreateSourceVoice(&voice,&format))) return false;
  if(FAILED(engine_->CreateSourceVoice(&music_,&format))) return false;
  if(FAILED(engine_->CreateSourceVoice(&combatMusic_,&format))) return false;
  XAUDIO2_BUFFER buffer{}; buffer.AudioBytes=UINT32(score_.size()*2); buffer.pAudioData=reinterpret_cast<const BYTE*>(score_.data());
  buffer.LoopCount=XAUDIO2_LOOP_INFINITE;
  music_->SetVolume(0);
  if(FAILED(music_->SubmitSourceBuffer(&buffer))) return false;
  buffer.AudioBytes=UINT32(combatScore_.size()*2);buffer.pAudioData=reinterpret_cast<const BYTE*>(combatScore_.data());
  combatMusic_->SetVolume(0);
  if(FAILED(combatMusic_->SubmitSourceBuffer(&buffer))) return false;
  for(size_t i=0;i<ambience_.size();++i) {
    ambienceSamples_[i]=sound::GenerateAmbience(sound::Ambience(i));
    if(FAILED(engine_->CreateSourceVoice(&ambience_[i],&format))) return false;
    buffer.AudioBytes=UINT32(ambienceSamples_[i].size()*2);
    buffer.pAudioData=reinterpret_cast<const BYTE*>(ambienceSamples_[i].data());
    ambience_[i]->SetVolume(0);
    if(FAILED(ambience_[i]->SubmitSourceBuffer(&buffer)) || FAILED(ambience_[i]->Start(0,1))) return false;
  }
  // Start both loop clocks together so combat can fade in on the same beat.
  return SUCCEEDED(music_->Start(0,1)) && SUCCEEDED(combatMusic_->Start(0,1)) && SUCCEEDED(engine_->CommitChanges(1));
}
void Audio::ClearEffects() {
  for(auto* voice:voices_) if(voice) { voice->Stop(); voice->FlushSourceBuffers(); }
  voiceLevels_.fill(0); voicePeaks_.fill(0); spatial_.fill(false); effectGain_=.85f;
}
void Audio::Suspend(bool value) {
  if(!engine_ || value==suspended_) return;
  suspended_=value;
  if(value) {ambientMix_.Reset();for(auto* voice:ambience_) if(voice) voice->SetVolume(0);}
  if(value) { engine_->StopEngine(); ClearEffects(); events_.Reset(); intensity_.Reset();combatMusic_->SetVolume(0);generation_=~0u; }
  else engine_->StartEngine();
}
void Audio::Update(const rpg::Simulation& state,unsigned generation,float musicVolume,float effectsVolume,float seconds,float cameraYaw) {
  if(!music_ || suspended_) return;
  music_->SetVolume(std::clamp(musicVolume,0.f,1.f)*.30f);
  if(generation!=generation_) {
    generation_=generation; ClearEffects(); events_.Reset(); variations_.fill(0);intensity_.Reset();
    ambientMix_.Reset();
  }
  if(state.Health()<=0) intensity_.Reset();
  else intensity_.Update(sound::CombatThreat(state),seconds);
  combatMusic_->SetVolume(std::clamp(musicVolume,0.f,1.f)*.30f*intensity_.value);
  ambientMix_.Update(state.Player(),intensity_.value,seconds);
  for(size_t i=0;i<ambience_.size();++i)
    ambience_[i]->SetVolume(std::clamp(effectsVolume,0.f,1.f)*.35f*ambientMix_.levels[i]);
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
      origins_[v]=events_.origins[i];spatial_[v]=events_.spatial[i];
      voice->Start(0,mixOperation);
    }
    break;
  }
  float peakLoad=0;
  std::array<float,12> levels{};
  for(size_t v=0;v<voices_.size();++v) {
    XAUDIO2_VOICE_STATE status{}; voices_[v]->GetState(&status,XAUDIO2_VOICE_NOSAMPLESPLAYED);
    if(!status.BuffersQueued) voiceLevels_[v]=voicePeaks_[v]=0;
    const auto player=state.Player();
    const auto place=spatial_[v]?sound::Place({origins_[v].x-player.x,origins_[v].z-player.z},cameraYaw):sound::Placement{};
    levels[v]=voiceLevels_[v]*place.gain;
    if(outputChannels_==2) {
      const float matrix[]={place.left,place.right};
      voices_[v]->SetOutputMatrix(master_,1,2,matrix,mixOperation);
    }
    // Mono and surround endpoints retain XAudio2's native routing; attenuation
    // still applies. Stereo gets equal-power pan, including already-playing tails.
    peakLoad+=levels[v]*voicePeaks_[v];
  }
  effectGain_=sound::MixGain(effectGain_,peakLoad,seconds);
  for(size_t v=0;v<voices_.size();++v)
    voices_[v]->SetVolume(std::clamp(effectsVolume,0.f,1.f)*effectGain_*levels[v],mixOperation);
  engine_->CommitChanges(mixOperation);
}
