#include "engine/Audio.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

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
  const auto music=sound::GenerateMusic(); bytes+=music.size()*2;
  const auto mend=sound::Generate(sound::Cue::Mend);
  double tailEnergy=0;
  for(size_t i=sound::sampleRate/2;i<mend.size();++i) tailEnergy+=double(mend[i])*mend[i];
  Require(tailEnergy/(mend.size()-sound::sampleRate/2)>100,"Healing reflections must retain an audible tail");
  Require(mend!=sound::Generate(sound::Cue::Potion),"Potion and healing spell must have distinct sounds");
  Require(sound::Generate(sound::Cue::Strike)!=sound::Generate(sound::Cue::Enemy),"Enemy windup must differ from player swing");
  Require(music.size()==sound::sampleRate*32 && music.front()==0 && music.back()==0,"Music loop must have bounded duration and seamless silent endpoints");
  int peak=0; double energy=0;
  for(auto s:music) { peak=std::max(peak,std::abs(int(s))); energy+=double(s)*s; }
  Require(peak>1000 && peak<16000 && energy/music.size()>10000,"Music mix needs headroom and non-silent content");
  Require(bytes<2*1024*1024,"Resident PCM must stay below 2 MiB");
  WriteWave("dawnwatch-music.wav",music); WriteWave("effects-preview.wav",preview);

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
  simulation.Update(.01f,command); const auto save=simulation.Save(); events.Update(simulation);
  Require(Has(events,sound::Cue::Heavy) && Has(events,sound::Cue::HeavyHit) && Has(events,sound::Cue::Interrupt),"Heavy hit and interruption need distinct cues");
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
  command={}; command.ability=2; simulation.Update(.01f,command); events.Update(simulation);
  Require(Has(events,sound::Cue::Dash) && !Has(events,sound::Cue::Step),"Dash must sound once without false footsteps");
  simulation.Update(.01f,command); events.Update(simulation); Require(events.count==0,"Rejected dash must not repeat");
  for(int rate : {30,60,144}) {
    simulation=rpg::Simulation{}; events.Reset(); command={}; command.ability=2;
    simulation.Update(1.f/rate,command); events.Update(simulation);
    while(simulation.Cooldown(2)>1.f/rate) {
      simulation.Update(1.f/rate,{}); events.Update(simulation);
    }
    Require(simulation.Cooldown(2)>0,"Dash fixture must reach the last cooldown frame");
    simulation.Update(1.f/rate,command); events.Update(simulation);
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
    if(device.Initialize()) {
      device.Update(simulation,1,0,0); device.Suspend(true); device.Suspend(true);
      device.Suspend(false); device.Update(simulation,2,0,0);
      std::cout<<"Muted XAudio2 device startup, suspend/resume and cleanup exercised.\n";
    } else std::cout<<"No audio endpoint available; silent initialization fallback exercised.\n";
  }
  std::cout<<"PASS: deterministic PCM, headroom, bounded memory, accepted event cues, landing/dash/footsteps; resident bytes "<<bytes<<". Tests remain muted.\n";
}
