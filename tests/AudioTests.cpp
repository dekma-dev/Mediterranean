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
void WriteWave(const char* path,const std::vector<std::int16_t>& samples,unsigned channels=1) {
  std::ofstream out(path,std::ios::binary);
  const auto word=[&](unsigned value,int bytes) { for(int i=0;i<bytes;++i) out.put(char((value>>(8*i))&255)); };
  out.write("RIFF",4); word(36+unsigned(samples.size()*2),4); out.write("WAVEfmt ",8);
  word(16,4); word(1,2); word(channels,2); word(sound::sampleRate,4); word(sound::sampleRate*2*channels,4); word(2*channels,2); word(16,2);
  out.write("data",4); word(unsigned(samples.size()*2),4);
  out.write(reinterpret_cast<const char*>(samples.data()),std::streamsize(samples.size()*2));
  Require(bool(out),"Audio preview write failed");
}
int main() {
  for(int hz:{30,60,144,1000}) for(bool boss:{false,true}) {
    rpg::Simulation fight;
    auto& enemies=const_cast<std::vector<rpg::Enemy>&>(fight.Enemies());
    for(auto& enemy:enemies) {enemy.health=0;enemy.state=rpg::EnemyState::Dead;}
    auto& enemy=enemies[0];enemy.kind=boss?rpg::EnemyKind::Captain:rpg::EnemyKind::Raider;
    enemy.rank=boss?1:0;enemy.health=enemy.MaxHealth();enemy.state=rpg::EnemyState::Pursuing;
    enemy.position=enemy.home={fight.Player().x,fight.Player().z+1.8f};enemy.facing=3.14159265f;
    sound::Events cues;const auto expected=boss?sound::Cue::BossPrepare:sound::Cue::EnemyPrepare;
    int preparations=0;
    for(int frame=0;frame<hz/5;++frame) {
      fight.Update(1.f/hz,{});cues.Update(fight);
      for(unsigned i=0;i<cues.count;++i) if(cues.cues[i]==expected) {
        ++preparations;
        Require(cues.spatial[i] && std::hypot(cues.origins[i].x-enemy.position.x,cues.origins[i].z-enemy.position.z)<.01f,
          "Enemy preparation must originate at the committed caster");
      }
    }
    Require(preparations==1 && enemy.windup>0,"Preparation must sound once at commitment, not every windup frame");
    // Use a campaign-valid pending cast for persistence: the synthetic boss
    // above deliberately overrides rank/health without earning its campaign.
    rpg::Simulation pending;
    Require(pending.Load("MEDITERRANEAN 3\n-4 5.2 100 0 1 0\n2 0 0 0\n0\n0 0 0 0\n"
      "100 -4 7 80 0 0 1 0.5 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n"),"Pending cast fixture");
    const auto save=pending.Save();Require(fight.Load(save),"Preparation save must reload");
    cues.Reset();cues.Update(fight);Require(!Has(cues,expected),"Loading a pending cast must not replay its preparation");
    fight.Update(0,{});cues.Update(fight);Require(!Has(cues,expected),"Invalid updates must not create preparation cues");
  }
  Require(sound::Generate(sound::Cue::EnemyPrepare)!=sound::Generate(sound::Cue::BossPrepare),
    "Boss and ordinary preparation must have distinct sound signatures");
  float volumeReference=-1;
  for(int hz:{30,60,144,1000}) {
    float level=0;
    for(int i=0;i<hz;++i) level=sound::MusicGain(level,.7f,1.f/hz);
    Require(level>.699f && level<=.7f,"Music gain must settle without overshooting");
    for(int i=0;i<hz/2;++i) level=sound::MusicGain(level,0,1.f/hz);
    Require(level<.002f,"Music mute must settle promptly");
    if(volumeReference>=0) Require(std::abs(level-volumeReference)<.00001f,"Music slider smoothing depends on frame rate");
    volumeReference=level;
  }
  Require(sound::MusicGain(.5f,1,0)==.5f && std::isfinite(sound::MusicGain(NAN,NAN,NAN)),"Invalid audio timing must not poison music gain");
  {
    rpg::WorldMap map;
    const rpg::Vec2 positions[]={{0,-10},{25,-27},rpg::WorldMap::trainingPost,{-11.5f,-1}};
    const sound::Cue steps[]={sound::Cue::Step,sound::Cue::StepGrass,sound::Cue::StepDirt,sound::Cue::StepWood};
    const sound::Cue lands[]={sound::Cue::Land,sound::Cue::LandGrass,sound::Cue::LandDirt,sound::Cue::LandWood};
    std::vector<std::int16_t> surfacePreview;
    for(int surface=0;surface<4;++surface) {
      const auto p=positions[surface];
      Require(map.CanStand(p,.45f),"Surface fixture must be walkable");
      Require(sound::GroundCue(map,p)==steps[surface] && sound::GroundCue(map,p,true)==lands[surface],"Surface sound disagrees with terrain/dock");
      for(auto cue:{steps[surface],lands[surface]}) {
        const auto pcm=sound::Generate(cue);
        surfacePreview.insert(surfacePreview.end(),pcm.begin(),pcm.end());
        surfacePreview.insert(surfacePreview.end(),sound::sampleRate/3,0);
        Require(sound::CueLevel(cue)<sound::CueLevel(sound::Cue::HeavyHit),"Ground contacts must not mask combat");
      }
      for(int other=0;other<surface;++other)
        Require(sound::Generate(steps[surface])!=sound::Generate(steps[other]) && sound::Generate(lands[surface])!=sound::Generate(lands[other]),"Ground materials sound identical");
      for(int rate:{30,60,144}) {
        rpg::Simulation walker;auto snapshot=walker.Save();auto begin=snapshot.find('\n')+1;
        if(snapshot.compare(begin,8,"PROFILE ")==0) begin=snapshot.find('\n',begin)+1;
        const auto end=snapshot.find('\n',begin);std::ostringstream line;line<<p.x<<' '<<p.z<<" 100 0 0 0";
        snapshot.replace(begin,end-begin,line.str());Require(walker.Load(snapshot),"Surface fixture load failed");
        sound::Events contacts;int footsteps=0,landings=0;
        for(int frame=0;frame<rate;++frame) {
          rpg::Command move;move.movement={0,1};walker.Update(1.f/rate,move);contacts.Update(walker);
          for(unsigned i=0;i<contacts.count;++i) if(sound::IsStep(contacts.cues[i])) {
            ++footsteps;Require(contacts.cues[i]==sound::GroundCue(map,walker.Player()),"Footstep used stale surface");
          }
        }
        Require(footsteps==2,"Material selection changed distance-based cadence");
        for(int frame=0;frame<rate*2;++frame) {
          rpg::Command jump;jump.jump=frame==0;walker.Update(1.f/rate,jump);contacts.Update(walker);
          for(unsigned i=0;i<contacts.count;++i) {
            Require(!sound::IsStep(contacts.cues[i]),"Jump or idle produced false steps");
            if(sound::IsLanding(contacts.cues[i])) {
              ++landings;Require(contacts.cues[i]==sound::GroundCue(map,walker.Player(),true),"Landing used stale surface");
            }
          }
        }
        Require(landings==1,"Material jump must have one landing");
      }
    }
    WriteWave("ground-contacts-preview.wav",surfacePreview);
  }
  const auto village=sound::AmbientWeights({0,-10}),meadow=sound::AmbientWeights({0,-24}),astral=sound::AmbientWeights(rpg::WorldMap::astralAnchor);
  Require(village[1]>.99f && meadow[0]>.99f && astral[2]>.99f,"Regional ambience is assigned to the wrong place");
  for(int x=-29;x<=29;++x) for(int z=-31;z<=31;++z) {
    const auto weights=sound::AmbientWeights({float(x),float(z)});
    Require(weights[0]>=0 && weights[1]>=0 && weights[2]>=0 && std::abs(weights[0]+weights[1]+weights[2]-1)<.00001f,"Ambient regions must share a bounded mix");
  }
  float ambientReference=-1;
  for(int hz:{30,60,144,1000}) {
    sound::AmbientMix mix;
    for(int n=0;n<hz*3;++n) mix.Update({0,-10},0,1.f/hz);
    for(int n=0;n<hz*3;++n) mix.Update({0,-24},1,1.f/hz);
    Require(mix.levels[0]>0 && mix.levels[1]>0 && mix.levels[0]+mix.levels[1]<.6f,"Ambient region change must crossfade and duck during combat");
    if(ambientReference>=0) Require(std::abs(mix.levels[0]-ambientReference)<.0001f,"Ambient transition depends on frame rate");
    ambientReference=mix.levels[0];mix.Reset();Require(mix.levels==std::array<float,3>{},"Ambient gain survived reset");
  }
  float intensityReference=-1;
  for(int hz:{30,60,144,1000}) {
    sound::MusicIntensity intensity;
    for(int n=0;n<hz*2;++n) intensity.Update(true,1.f/hz);
    Require(intensity.value>.8f && intensity.value<.9f,"Combat music must fade in gradually");
    for(int n=0;n<hz*2;++n) intensity.Update(false,1.f/hz);
    Require(intensity.value>.9f,"Brief combat gaps must not pump the music");
    for(int n=0;n<hz*5;++n) intensity.Update(false,1.f/hz);
    if(intensityReference>=0) Require(std::abs(intensity.value-intensityReference)<.0001f,"Music fade depends on frame rate");
    intensityReference=intensity.value;
    intensity.Update(true,NAN);Require(std::isfinite(intensity.value),"Invalid time poisoned music gain");
    intensity.Reset();Require(intensity.value==0 && intensity.hold==0,"Music state survived reset");
  }
  Require(!sound::CombatThreat(rpg::Simulation{}),"Idle exploration must not start combat music");
  const auto center=sound::Place({0,0},0),left=sound::Place({-5,0},0),right=sound::Place({5,0},0);
  Require(center.gain==1 && std::abs(center.left-center.right)<.00001f,"Local sounds must remain centered");
  Require(left.left>.999f && left.right<.001f && right.right>.999f && right.left<.001f,"Stereo placement reversed left/right");
  const auto turned=sound::Place({5,0},3.14159265f);
  Require(turned.left>.999f,"Camera orbit must rotate the sound field");
  float previous=1;
  for(float distance=0;distance<=20;distance+=.125f) {
    const auto p=sound::Place({distance,0},.7f);
    Require(p.gain<=previous && p.gain>=0,"Distance attenuation must be monotone and bounded");previous=p.gain;
    Require(std::abs(p.left*p.left+p.right*p.right-1)<.00001f,"Panning must preserve constant power");
  }
  Require(sound::Place({18,0},0).gain==0 && sound::Place({0,0},NAN).gain==0,"Spatial range/invalid input must be silent");
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
  for(int region=0;region<3;++region) {
    const auto pcm=sound::GenerateAmbience(sound::Ambience(region));bytes+=pcm.size()*2;
    Require(pcm.size()==sound::sampleRate*6 && pcm.front()==0 && pcm.back()==0 &&
      pcm==sound::GenerateAmbience(sound::Ambience(region)),"Ambient loop length, endpoints or determinism failed");
    int peak=0;double energy=0;for(auto sample:pcm) {peak=std::max(peak,std::abs(int(sample)));energy+=double(sample)*sample;}
    Require(peak>100 && peak<6000 && energy/pcm.size()>100,"Ambience must be audible with room for combat");
    const auto path="ambience-"+std::to_string(region)+".wav";WriteWave(path.c_str(),pcm);
  }
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
  const auto combatMusic=sound::GenerateCombatLayer();bytes+=combatMusic.size()*2;
  Require(combatMusic.size()==sound::sampleRate*6 && combatMusic.front()==0 && combatMusic.back()==0 &&
    combatMusic==sound::GenerateCombatLayer(),"Combat layer duration, edges or determinism failed");
  int combatPeak=0;for(auto sample:combatMusic) combatPeak=std::max(combatPeak,std::abs(int(sample)));
  Require(combatPeak>1000 && combatPeak<10000,"Combat percussion lacks headroom or is silent");
  WriteWave("combat-music-layer.wav",combatMusic);
  std::vector<std::int16_t> adaptive(sound::sampleRate*20);sound::MusicIntensity audition;
  for(size_t n=0;n<adaptive.size();++n) {
    const float t=float(n)/sound::sampleRate;audition.Update(t>=4 && t<10,1.f/sound::sampleRate);
    const float value=music[n]+combatMusic[n%combatMusic.size()]*audition.value;
    Require(std::abs(value)<30000,"Adaptive music sum clips");
    adaptive[n]=std::int16_t(value);
  }
  // Audition ends partway through the composition, so soften its file boundary.
  for(size_t n=0;n<sound::sampleRate;++n) adaptive[adaptive.size()-1-n]=std::int16_t(adaptive[adaptive.size()-1-n]*float(n)/sound::sampleRate);
  WriteWave("adaptive-score-preview.wav",adaptive);
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
  const auto impact=sound::Generate(sound::Cue::HeavyHit);
  std::vector<std::int16_t> stereo(sound::sampleRate*7*2);
  for(unsigned hit=0;hit<7;++hit) {
    const auto placement=sound::Place({-6.f+hit*2.f,2},0);
    for(size_t n=0;n<impact.size();++n) {
      const size_t at=(hit*sound::sampleRate+n)*2;
      stereo[at]=std::int16_t(impact[n]*placement.gain*placement.left);
      stereo[at+1]=std::int16_t(impact[n]*placement.gain*placement.right);
    }
  }
  WriteWave("spatial-combat-preview.wav",stereo,2);
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
  Require(events.count==1,"Full-health defensive guard must emit its sound");
  simulation.CreateCharacter(rpg::ClassId::Warrior,"Tester");
  auto injured=simulation.Save(); const auto health=injured.find(" 100 ");
  Require(health!=std::string::npos,"Healing fixture missing"); injured.replace(health,5," 20 ");
  Require(simulation.Load(injured),"Injured audio fixture rejected");
  simulation.Update(.01f,command); events.Update(simulation);
  Require(events.count==1 && Has(events,sound::Cue::Ward),"Accepted defensive recovery must emit one sound");
  simulation.Update(.01f,command); events.Update(simulation);
  Require(events.count==0,"Cooldown rejection must not replay healing");
  {
    rpg::Simulation focus;focus.CreateCharacter(rpg::ClassId::Psionic,"Tester");
    rpg::Command cast;cast.ability=3;focus.Update(.01f,cast);sound::Events cues;cues.Update(focus);
    Require(Has(cues,sound::Cue::ReserveReady),"Filling a reserve must announce readiness");
    focus.Update(.01f,{});cues.Update(focus);Require(!Has(cues,sound::Cue::ReserveReady),"Ready reserve must not repeat its cue");
    focus.CreateCharacter(rpg::ClassId::Priest,"Tester");auto wounded=focus.Save();const auto hp=wounded.find(" 95 ");
    Require(hp!=std::string::npos,"Priest recovery fixture");wounded.replace(hp,4," 40 ");Require(focus.Load(wounded),"Priest fixture load");
    focus.Update(.01f,cast);cues.Update(focus);Require(Has(cues,sound::Cue::Cast),"Preparation cue missing");
    rpg::Command move;move.movement={1,0};focus.Update(.01f,move);cues.Update(focus);
    Require(Has(cues,sound::Cue::CastCancelled) && !Has(cues,sound::Cue::Holy),"Interrupted cast must fizzle without completion audio");
    focus.Update(.01f,{});cues.Update(focus);Require(!Has(cues,sound::Cue::CastCancelled),"Cancelled cast replayed its cue");
  }
  const char* duel="MEDITERRANEAN 3\n-4 5.2 100 0 1 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.5 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(simulation.Load(duel),"Combat audio fixture rejected"); events.Reset();
  Require(sound::CombatThreat(simulation),"Pursuing enemy must raise musical tension");
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
  for(unsigned i=0;i<events.count;++i) if(events.cues[i]==sound::Cue::HeavyHit)
    Require(events.spatial[i] && std::hypot(events.origins[i].x-simulation.Player().x,events.origins[i].z-simulation.Player().z)>1,
      "Impact sound must retain its target location");
  Require(simulation.Save()==save,"Audio must not change gameplay/save state");
  simulation=rpg::Simulation{}; events.Reset(); command={}; command.jump=true;
  simulation.Update(1.f/60,command); events.Update(simulation); Require(Has(events,sound::Cue::Jump),"Jump cue missing");
  for(unsigned i=0;i<events.count;++i) if(events.cues[i]==sound::Cue::Jump) Require(!events.spatial[i],"Player jump must follow the listener");
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
