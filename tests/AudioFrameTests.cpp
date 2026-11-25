#include "engine/Audio.h"
#include "game/Game.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
rpg::Simulation Hero(rpg::ClassId id){
  rpg::Simulation state;Check(state.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Hero fixture");
  auto save=state.Save();const auto at=save.find("PROFILE ");
  save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Sound\" 7 1 3");
  Check(state.Load(save),"Class fixture");return state;
}
void Ownership(){
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest}){
    auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(id).Save()),"Game fixture");
    rpg::Command c;c.selectActor=210;c.ability=1;c.uiClick=true;game->ExecuteFrame(c,.01f);
    const auto save=game->State().Save();const auto frozen=game->TakeAudioFrame(.01f);
    Check(frozen.Valid() && frozen.alive && frozen.threat && frozen.preparing && frozen.events.count>0,
      "Actual command did not produce a valid audio publication");
    Check(game->State().Save()==save,"Audio publication changed gameplay");
    if(id!=rpg::ClassId::Warrior){bool found=false;for(unsigned i=0;i<frozen.events.count;++i)if(sound::IsPreparation(frozen.events.cues[i])){
      found=true;Check(frozen.preparationToken>0 && frozen.events.preparationTokens[i]==frozen.preparationToken,"Preparation lost its accepted cast identity");}
      Check(found,"Ranged cast preparation missing");}
    const auto* storage=game->TakeAudioFrame(.01f).emitters.data();
    for(int i=0;i<200;++i){const auto& next=game->TakeAudioFrame(.01f);
      Check(!next.events.count && next.emitters.data()==storage,"Repeated publication replayed one-shots or reallocated stable emitters");}
    const auto remembered=frozen.player;const auto token=frozen.preparationToken;const auto emitter=frozen.emitters.front();
    game->Initialize();const auto fresh=game->TakeAudioFrame(.01f);
    Check(fresh.sequence>frozen.sequence && fresh.generation>frozen.generation && !fresh.events.count && !fresh.preparing,
      "World reset reused sequence or retained pending audio");
    Check(frozen.player.x==remembered.x && frozen.player.z==remembered.z && frozen.preparationToken==token && frozen.events.count>0,
      "Retained frame borrowed source state");
    Check(frozen.emitters.front().id==emitter.id && frozen.emitters.front().position.x==emitter.position.x &&
      frozen.emitters.front().home.x==emitter.home.x,"Retained packet borrowed emitter storage");
    auto malformed=frozen;malformed.events.count=unsigned(malformed.events.cues.size()+1);Check(!malformed.Valid(),"Oversized cue batch accepted");
    malformed=frozen;malformed.emitters[0].position.x=std::numeric_limits<float>::infinity();Check(!malformed.Valid(),"Invalid source coordinate accepted");
    malformed=frozen;malformed.events.cues[0]=sound::Cue::Count;Check(!malformed.Valid(),"Invalid cue could index native samples");
    std::cout<<"Audio class "<<int(id)<<": "<<frozen.emitters.size()<<" emitters, "<<frozen.Bytes()<<" owned bytes\n";
  }
  auto state=Hero(rpg::ClassId::Warrior);auto& beast=const_cast<rpg::Enemy&>(state.Beasts()[0]);
  beast.position=state.Player();beast.health=beast.MaxHealth();beast.state=rpg::EnemyState::Pursuing;
  Check(sound::CombatThreat(state),"Attacking wildlife failed to raise combat music");
  state=Hero(rpg::ClassId::Warrior);rpg::Command start;start.interact=true;state.Update(.001f,start);
  Check(state.Sparring() && sound::CombatThreat(state),"Sparring threat missing from audio view");
  presentation::SceneSnapshot scene;scene.Capture(state,1);sound::Frame frame;frame.Capture(scene);
  Check(frame.Find(200)!=nullptr,"Moving trainer absent from emitter publication");
}
void Attachments(){
  sound::Frame frame;frame.emitters.push_back({100,rpg::EnemyKind::Wolf,1,{2,3},{2,.72f,3}});
  sound::Attachment voice;voice.Bind(100,frame);sound::AcousticPoint point{};
  Check(voice.Resolve(frame,point) && point.y==.72f,"Live source binding failed");
  frame.emitters[0].position.x=4;Check(voice.Resolve(frame,point) && point.x==4,"Source motion not followed");
  const auto emitter=frame.emitters[0];frame.emitters.clear();Check(!voice.Resolve(frame,point) && voice.orphaned && point.x==4,"Missing source moved an accepted tail");
  frame.emitters.push_back(emitter);frame.emitters[0].position.x=99;
  Check(!voice.Resolve(frame,point) && point.x==4,"Reappearing ID recaptured a sound tail");
  voice.Bind(100,frame);frame.emitters[0].home.x+=1;
  Check(!voice.Resolve(frame,point) && voice.orphaned,"Reused spawn identity retained voice attachment");
  sound::Events events;events.count=1;events.cues[0]=sound::Cue::EnemyPrepare;events.attachedActors[0]=100;
  events.emitterIdentities[0]={rpg::EnemyKind::Wolf,1,{2,3},true};events.preparationTokens[0]=17;
  sound::DeliveryQueue queue;queue.Append(events);queue.Advance(.02f);const auto accepted=queue.Take();
  Check(accepted.emitterIdentities[0].home.x==2 && accepted.preparationTokens[0]==17,"Delivery lost spawn/cast identity");
  voice.Bind(100,frame,accepted.emitterIdentities[0]);
  Check(voice.orphaned && !voice.Resolve(frame,point),"Queued warning latched to a replacement spawn");
}
std::pair<sound::Frame,sound::Frame> TwoCasts(){
  auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(rpg::ClassId::Mage).Save()),"Mage fixture");
  rpg::Command c;c.selectActor=210;c.ability=1;game->ExecuteFrame(c,.01f);const auto first=game->TakeAudioFrame(.01f);
  Check(first.preparing && first.preparationToken>0,"First cast not preparing");
  c={};c.haltMovement=true;game->ExecuteFrame(c,.01f);
  bool next=false;for(int n=0;n<200 && !next;++n){c={};c.ability=0;c.queueAttack=true;game->ExecuteFrame(c,.005f);
    next=game->Scene().Player().attacking && game->Scene().Player().attackAbility==0;}
  const auto second=game->TakeAudioFrame(.01f);
  Check(next && second.preparing && second.preparationToken>first.preparationToken,"New accepted cast reused preparation identity");
  return {first,second};
}
void Native(){
  Audio audio;if(!audio.Initialize(true)){std::cout<<"No audio endpoint: native packet playback skipped.\n";return;}
  auto frames=TwoCasts();auto& first=frames.first;auto& second=frames.second;
  // Both packets arrive while the first native sample still has a tail. The
  // device must distinguish casts even though preparing remains true.
  audio.Update(first);const auto submitted=audio.Stats().submitted;Check(submitted>0,"Muted native cast not submitted");
  audio.Update(first);Check(audio.Stats().submitted==submitted && audio.Stats().rejected==1,"Duplicate packet replayed sound");
  auto invalid=second;invalid.seconds=std::numeric_limits<float>::quiet_NaN();audio.Update(invalid);
  Check(audio.Stats().submitted==submitted && audio.Stats().rejected==2,"Invalid frame changed playback");
  audio.Update(second);bool oldFaded=false,newLive=false;
  for(const auto& voice:audio.Voices())if(voice.preparation){
    if(voice.token==first.preparationToken)oldFaded|=voice.preparationGain<1;
    if(voice.token==second.preparationToken)newLive|=voice.preparationGain==1;
  }
  Check(oldFaded && newLive,"New preparation kept the old cast sounding at full level");
  audio.Update(first);Check(audio.Stats().rejected==3,"Stale frame rolled device state backwards");
  auto frame=second;frame.events={};frame.preparing=false;++frame.sequence;audio.Update(frame);
  ++frame.sequence;frame.generation++;frame.events.count=1;frame.events.cues[0]=sound::Cue::EnemyPrepare;
  frame.events.origins[0]={frame.player.x+2,frame.player.z};frame.events.heights[0]=frame.worldHeight+1;
  frame.events.spatial[0]=frame.events.environmental[0]=true;frame.events.attachedActors[0]=901;
  frame.events.emitterIdentities[0]={rpg::EnemyKind::Wolf,1,{2,3},true};
  frame.emitters={{901,rpg::EnemyKind::Wolf,1,{2,3},{frame.player.x+2,frame.worldHeight+1,frame.player.z}}};
  audio.Update(frame);frame.events={};frame.emitters[0].position.x+=1;++frame.sequence;audio.Update(frame);
  float last=0;bool tracked=false;for(const auto& v:audio.Voices())if(v.actor==901){last=v.position.x;tracked=true;}
  Check(tracked && last==frame.emitters[0].position.x,"Native emitter did not follow owned position");
  const auto emitter=frame.emitters[0];frame.emitters.clear();++frame.sequence;audio.Update(frame);
  frame.emitters={emitter};frame.emitters[0].position.x+=20;++frame.sequence;audio.Update(frame);
  bool frozen=false;for(const auto& v:audio.Voices())if(v.actor==901)frozen|=v.orphaned && v.position.x==last;
  Check(frozen,"Native sound tail jumped to a respawned ID");
  audio.Suspend(true);++frame.sequence;audio.Update(frame);const auto paused=audio.Stats().submitted;
  audio.Suspend(false);audio.Update(frame);Check(audio.Stats().submitted==paused,"Resume replayed a suspended packet");
  frame.generation++;++frame.sequence;frame.menu=true;frame.hasAudition=true;frame.audition.Reset();
  frame.audition.preparing=true;frame.audition.Add(sound::Cue::HolyGather);audio.Update(frame);
  const auto preview=audio.Stats().submitted;++frame.sequence;audio.Update(frame);
  Check(audio.Stats().submitted==preview,"Unchanged audition sequence replayed its cue");
  frame.hasAudition=false;++frame.sequence;audio.Update(frame);
  Check(audio.SpatialUpdateErrors()==0,"Native owned-frame filters/matrices failed");
  std::cout<<"Muted XAudio2: duplicate/stale/invalid packets, cast replacement, moving/dead emitters, suspend and audition passed.\n";
}
}
int main(){try{Ownership();Attachments();Native();std::cout<<"PASS: owned audio publication and device isolation.\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
