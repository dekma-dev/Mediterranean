#include "engine/Audio.h"
#include "tests/DashFixture.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

void Require(bool condition,const char* message) { if(!condition) { std::cerr<<message<<'\n'; std::exit(1); } }
bool Has(const sound::Events& events,sound::Cue cue) {
  return std::find(events.cues.begin(),events.cues.begin()+events.count,cue)!=events.cues.begin()+events.count;
}
void WriteWave(const char* path,const std::vector<std::int16_t>& samples) {
  std::ofstream out(path,std::ios::binary);
  const auto word=[&](unsigned value,int bytes) { for(int i=0;i<bytes;++i) out.put(char((value>>(8*i))&255)); };
  out.write("RIFF",4); word(36+unsigned(samples.size()*2),4); out.write("WAVEfmt ",8);
  word(16,4); word(1,2); word(1,2); word(sound::sampleRate,4); word(sound::sampleRate*2,4); word(2,2); word(16,2);
  out.write("data",4); word(unsigned(samples.size()*2),4);
  out.write(reinterpret_cast<const char*>(samples.data()),std::streamsize(samples.size()*2));
  Require(bool(out),"Audio preview write failed");
}
int main() {
  Require(sound::CueLevel(sound::Cue::Step)<sound::CueLevel(sound::Cue::HeavyHit),"Footsteps must sit behind combat impacts");
  for(unsigned cue=0;cue<unsigned(sound::Cue::Count);++cue) {
    for(unsigned n=0;n<21;++n) {
      const float pitch=sound::CuePitch(sound::Cue(cue),n);
      Require(pitch>.9f && pitch<1.1f,"Playback variation must remain subtle");
      Require(pitch==sound::CuePitch(sound::Cue(cue),n+7),"Variation must repeat deterministically without gameplay RNG");
    }
  }
  Require(sound::CuePitch(sound::Cue::Step,0)!=sound::CuePitch(sound::Cue::Step,1),"Consecutive footsteps must differ");
  Require(sound::MixGain(.85f,.5f,.016f)>.8f,"Sparse effects must not pay the twelve-voice attenuation penalty");
  for(int count=1;count<=12;++count) {
    const float load=count*.9f,gain=sound::MixGain(.85f,load,.016f);
    Require(load*gain<=.70001f,"Simultaneous voices must preserve bus headroom before playback");
  }
  float recovery=0;
  for(int hz : {30,60,144}) {
    float gain=.1f;
    for(int n=0;n<hz;++n) gain=sound::MixGain(gain,.2f,1.f/hz);
    if(recovery) Require(std::abs(recovery-gain)<.00001f,"Gain recovery must be frame-rate independent");
    recovery=gain;
  }
  size_t bytes=0; std::vector<std::int16_t> preview;
  for(unsigned i=0;i<unsigned(sound::Cue::Count);++i) {
    const auto samples=sound::Generate(sound::Cue(i)); bytes+=samples.size()*2;
    Require(samples==sound::Generate(sound::Cue(i)),"Procedural effects must be deterministic");
    Require(samples.front()==0 && samples.back()==0,"Effects must fade at buffer boundaries");
    double energy=0; int peak=0;
    for(auto s:samples) { peak=std::max(peak,std::abs(int(s))); energy+=double(s)*s; }
    Require(peak>100 && peak<30000 && energy/samples.size()>100,"Effects must be audible PCM without clipping");
    // Start/end windows must be quiet even for noisy transients and long spell tails.
    for(size_t edge=0;edge<8;++edge)
      Require(std::abs(int(samples[edge]))<2500 && std::abs(int(samples[samples.size()-1-edge]))<2500,
        "Effect edges must not introduce a full-scale click");
    preview.insert(preview.end(),samples.begin(),samples.end()); preview.insert(preview.end(),sound::sampleRate/4,0);
  }
  Require(sound::Generate(sound::Cue::Bow)!=sound::Generate(sound::Cue::Pistol) &&
    sound::Generate(sound::Cue::Bow)!=sound::Generate(sound::Cue::Cast),"Physical weapons must have distinct release sounds");
  WriteWave("handbow-release.wav",sound::Generate(sound::Cue::Bow));
  WriteWave("pistol-release.wav",sound::Generate(sound::Cue::Pistol));
  for(int i=int(rpg::ClassId::Scout);i<=int(rpg::ClassId::Engineer);++i) {
    const auto cue=sound::ClassReleaseCue(rpg::ClassId(i));
    Require(cue!=sound::Cue::Cast,"A ranged class still uses the generic cast cue");
    for(int j=i+1;j<=int(rpg::ClassId::Engineer);++j)
      Require(sound::Generate(cue)!=sound::Generate(sound::ClassReleaseCue(rpg::ClassId(j))),"Ranged class sound identities collapsed");
  }
  const auto music=sound::GenerateMusic(); bytes+=music.size()*2;
  const auto mend=sound::Generate(sound::Cue::Mend);
  double tailEnergy=0;
  for(size_t i=sound::sampleRate/2;i<mend.size();++i) tailEnergy+=double(mend[i])*mend[i];
  Require(tailEnergy/(mend.size()-sound::sampleRate/2)>100,"Healing reflections must retain an audible tail");
  Require(mend!=sound::Generate(sound::Cue::Potion),"Potion and healing spell must have distinct sounds");
  Require(sound::Generate(sound::Cue::Strike)!=sound::Generate(sound::Cue::Enemy),"Enemy windup must differ from player swing");
  Require(music.size()==sound::sampleRate*sound::musicSeconds && music.front()==0 && music.back()==0,"Music loop must have bounded duration and seamless silent endpoints");
  Require(music==sound::GenerateMusic(),"The original score must render deterministically");
  for(unsigned bar=0;bar<8;++bar) {
    double phraseEnergy=0;
    for(size_t i=bar*6*sound::sampleRate;i<(bar+1)*6*sound::sampleRate;++i)
      phraseEnergy+=double(music[i])*music[i];
    Require(phraseEnergy/(6*sound::sampleRate)>10000,"A score phrase unexpectedly became silent");
  }
  for(const auto cue:{sound::Cue::Holy,sound::Cue::Soul,sound::Cue::Ward,sound::Cue::Song}) {
    const auto pcm=sound::Generate(cue);double resonance=0;
    for(size_t i=sound::sampleRate*7/10;i<pcm.size();++i) resonance+=double(pcm[i])*pcm[i];
    Require(resonance/(pcm.size()-sound::sampleRate*7/10)>10,"Mystical resonance was cut off early");
  }
  int peak=0; double energy=0;
  for(auto s:music) { peak=std::max(peak,std::abs(int(s))); energy+=double(s)*s; }
  Require(peak>1000 && peak<16000 && energy/music.size()>10000,"Music mix needs headroom and non-silent content");
  Require(bytes<4*1024*1024,"Resident PCM must stay below 4 MiB");
  WriteWave("dawnwatch-music.wav",music); WriteWave("effects-preview.wav",preview);
  // Offline audition of repeated steps, combat and recovery using the same
  // cue levels, pitch choices and peak-budget gain as runtime (without music).
  struct PreviewCue { sound::Cue cue; float start; unsigned variant; };
  const PreviewCue sequence[]={{sound::Cue::Step,0,0},{sound::Cue::Step,.3f,1},
    {sound::Cue::Step,.6f,2},{sound::Cue::Strike,1,0},{sound::Cue::Hit,1.08f,0},
    {sound::Cue::Strike,1.6f,1},{sound::Cue::Hit,1.68f,1},{sound::Cue::Heavy,2.2f,0},
    {sound::Cue::HeavyHit,2.3f,0},{sound::Cue::Interrupt,2.3f,0},
    {sound::Cue::Mend,3.2f,0},{sound::Cue::Potion,4.9f,0}};
  std::array<std::vector<std::int16_t>,size_t(sound::Cue::Count)> bank;
  std::array<float,size_t(sound::Cue::Count)> peaks{};
  for(size_t c=0;c<bank.size();++c) {
    bank[c]=sound::Generate(sound::Cue(c));
    for(auto value:bank[c]) peaks[c]=std::max(peaks[c],std::abs(int(value))/32767.f);
  }
  std::vector<std::int16_t> mixed(sound::sampleRate*7); float gain=.85f;
  for(size_t frame=0;frame<mixed.size();++frame) {
    float signal=0,load=0;
    for(const auto& event:sequence) {
      const size_t c=size_t(event.cue); const auto& pcm=bank[c];
      const float position=(float(frame)-event.start*sound::sampleRate)*sound::CuePitch(event.cue,event.variant);
      if(position<0 || position>=float(pcm.size()-1)) continue;
      const size_t index=size_t(position); const float blend=position-float(index),level=sound::CueLevel(event.cue);
      signal+=(pcm[index]*(1-blend)+pcm[index+1]*blend)/32767.f*level;
      load+=peaks[c]*level;
    }
    gain=sound::MixGain(gain,load,1.f/sound::sampleRate);
    Require(std::abs(signal*gain)<.701f,"Audition mix exceeded its peak budget");
    mixed[frame]=std::int16_t(signal*gain*32767);
  }
  WriteWave("combat-mix-preview.wav",mixed);

  rpg::Simulation simulation; sound::Events events; rpg::Command command;
  {
    rpg::Simulation chain; sound::Events chainEvents; int jumps=0,lands=0,steps=0;
    for(int frame=0;frame<13;++frame) {
      rpg::Command input; input.jump=frame==0 || frame==10;
      chain.Update(.05f,input); chainEvents.Update(chain);
      jumps+=Has(chainEvents,sound::Cue::Jump); lands+=Has(chainEvents,sound::Cue::Land);
      steps+=Has(chainEvents,sound::Cue::Step);
    }
    Require(jumps==2 && lands==1 && steps==0,"Buffered landing/takeoff must emit both physical cues once without footsteps");
  }
  command.ability=3; simulation.Update(.01f,command); events.Update(simulation);
  Require(events.count==0,"Rejected full-health Mend must be silent");
  auto injured=simulation.Save(); const auto health=injured.find(" 100 ");
  Require(health!=std::string::npos,"Healing fixture missing"); injured.replace(health,5," 20 ");
  Require(simulation.Load(injured),"Injured audio fixture rejected");
  simulation.Update(.01f,command); events.Update(simulation);
  Require(events.count==1 && Has(events,sound::Cue::Mend),"Accepted healing must emit one sound");
  simulation.Update(.01f,command); events.Update(simulation);
  Require(events.count==0,"Cooldown rejection must not replay healing");
  const char* duel="MEDITERRANEAN 3\n-4 5.2 100 0 1 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.5 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(simulation.Load(duel),"Combat audio fixture rejected"); events.Reset();
  command={}; command.nextTarget=true; command.ability=1;
  simulation.Update(.01f,command); events.Update(simulation);
  Require(events.count==0,"Wind-up must not play a premature impact or swing");
  bool swing=false;
  for(int i=0;i<30;++i) {
    simulation.Update(.01f,{}); events.Update(simulation);
    if (Has(events,sound::Cue::Heavy)) {
      Require(!Has(events,sound::Cue::HeavyHit),"Swing sound must precede the contact sound"); swing=true;
    }
  }
  const auto save=simulation.Save();
  Require(swing && Has(events,sound::Cue::HeavyHit) && Has(events,sound::Cue::Interrupt),"Contact and interruption need distinct cues after the swing");
  Require(simulation.Save()==save,"Audio must not change gameplay/save state");
  simulation=rpg::Simulation{}; events.Reset(); command={}; command.jump=true;
  simulation.Update(1.f/60,command); events.Update(simulation); Require(Has(events,sound::Cue::Jump),"Jump cue missing");
  events.Reset(); events.Update(simulation);
  Require(Has(events,sound::Cue::Jump),"Resetting audio on an accepted jump frame must retain the cue");
  simulation.Update(1.f/60,{}); events.Reset(); events.Update(simulation);
  Require(!Has(events,sound::Cue::Jump),"Resetting audio during ascent must not invent another jump");
  int landings=0;
  for(int i=0;i<70;++i) { simulation.Update(1.f/60,{}); events.Update(simulation); landings+=Has(events,sound::Cue::Land); }
  Require(landings==1,"A jump must produce exactly one landing cue");
  Require(simulation.Load(test::dashFixture),"Target dash audio fixture"); events.Reset();
  command={}; command.ability=2; command.nextTarget=true; simulation.Update(.01f,command); events.Update(simulation);
  Require(Has(events,sound::Cue::Dash) && !Has(events,sound::Cue::Step),"Dash must sound once without false footsteps");
  command.nextTarget=false; simulation.Update(.01f,command); events.Update(simulation); Require(events.count==0,"Rejected dash must not repeat");
  for(int rate : {30,60,144}) {
    Require(simulation.Load(test::dashFixture),"Dash expiry fixture"); events.Reset(); command={}; command.ability=2; command.nextTarget=true;
    simulation.Update(1.f/rate,command); events.Update(simulation);
    while(simulation.Cooldown(2)>1.f/rate) {
      simulation.Update(1.f/rate,{}); events.Update(simulation);
    }
    Require(simulation.Cooldown(2)>0,"Dash fixture must reach the last cooldown frame");
    // The actor moves during the five-second cooldown. Restore a valid distance
    // from its current position, not its spawn (which can now be out of range).
    const auto targetPosition=simulation.Enemies()[0].position;
    auto expiry=simulation.Save(); auto begin=expiry.find('\n')+1;
    if(expiry.compare(begin,8,"PROFILE ")==0) begin=expiry.find('\n',begin)+1;
    const auto end=expiry.find('\n',begin);
    std::ostringstream playerLine;
    playerLine<<targetPosition.x<<' '<<targetPosition.z-4.f<<" 100 3.14159 0 0";
    expiry.replace(begin,end-begin,playerLine.str());
    Require(simulation.Load(expiry),"Repositioned cooldown fixture must load");
    simulation.Update(1.f/rate,command); events.Update(simulation);
    Require(simulation.DashedThisUpdate(),"Expiry fixture must accept a valid targeted dash before testing its sound");
    Require(Has(events,sound::Cue::Dash),"Dash accepted on the cooldown expiry frame must sound");
    events.Reset(); events.Update(simulation);
    Require(Has(events,sound::Cue::Dash),"Resetting audio on an accepted dash frame must retain the cue");
    simulation.Update(0,command); events.Update(simulation);
    Require(events.count==0,"Invalid update must clear an accepted dash cue");
    const auto coolingSave=simulation.Save();
    Require(simulation.Load(coolingSave),"Cooling dash save must load"); events.Reset(); events.Update(simulation);
    Require(!Has(events,sound::Cue::Dash),"Loading a cooldown must not invent a dash cue");
    simulation.Update(0,command); events.Update(simulation);
    Require(events.count==0,"Invalid update must clear transient movement cues");
  }
  for(int rate : {30,60,144}) {
    simulation=rpg::Simulation{}; events.Reset(); command={}; command.movement={0,1}; int steps=0;
    for(int i=0;i<rate;++i) { simulation.Update(1.f/rate,command); events.Update(simulation); steps+=Has(events,sound::Cue::Step); }
    Require(steps==2,"Footsteps must follow distance consistently across rates");
  }
  {
    Audio device;
    if(device.Initialize(true)) {
      simulation=rpg::Simulation{};
      for(int frame=0;frame<13;++frame) {
        command={}; command.jump=frame==0 || frame==10;
        simulation.Update(.05f,command); device.Update(simulation,0,1,1,.05f);
      }
      device.Update(simulation,1,0,0); device.Suspend(true); device.Suspend(true);
      device.Suspend(false); device.Update(simulation,2,0,0);
      std::cout<<"Muted XAudio2 device startup, suspend/resume and cleanup exercised.\n";
    } else std::cout<<"No audio endpoint available; silent initialization fallback exercised.\n";
  }
  std::cout<<"PASS: deterministic PCM, headroom, bounded memory, accepted event cues, landing/dash/footsteps; resident bytes "<<bytes<<". Tests remain muted.\n";
}
