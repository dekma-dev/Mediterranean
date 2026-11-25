#include "engine/Audio.h"
#include <algorithm>
#include <cmath>

namespace sound {
void Frame::Capture(const presentation::SceneSnapshot& scene){
  const auto& p=scene.Player();player=p.position;worldHeight=p.worldHeight;alive=p.health>0;
  preparing=Preparing(p);threat=CombatThreat(scene);
  emitters.clear();emitters.reserve(scene.Combatants().size()+unsigned(scene.Sparring()));
  const auto add=[&](const presentation::ActorFrame& a){if(a.health>0)
    emitters.push_back({a.id,a.kind,a.rank,a.home,{a.position.x,rpg::WorldMap::Height(a.position)+a.contactHeight,a.position.z}});};
  for(const auto& a:scene.Combatants())add(a);
  if(scene.Sparring())add(scene.Trainer());
}
const Emitter* Frame::Find(rpg::EntityId id)const{
  if(!id || id==~0u)return nullptr;
  for(const auto& e:emitters)if(e.id==id)return &e;return nullptr;
}
bool Frame::Valid()const{
  if(!sequence || !std::isfinite(seconds) || seconds<0 || seconds>.1f || !std::isfinite(cameraYaw) ||
    !std::isfinite(worldHeight) || !std::isfinite(player.x) || !std::isfinite(player.z) ||
    !std::isfinite(musicVolume) || !std::isfinite(effectsVolume) || events.count>events.cues.size() || audition.count>audition.cues.size())return false;
  for(unsigned i=0;i<events.count;++i)if(unsigned(events.cues[i])>=unsigned(Cue::Count) ||
    !std::isfinite(events.origins[i].x) || !std::isfinite(events.origins[i].z) || !std::isfinite(events.heights[i]) ||
    (events.emitterIdentities[i].valid && (!std::isfinite(events.emitterIdentities[i].home.x) || !std::isfinite(events.emitterIdentities[i].home.z))))return false;
  if(menu && hasAudition)for(unsigned i=0;i<audition.count;++i)if(unsigned(audition.cues[i])>=unsigned(Cue::Count))return false;
  for(const auto& e:emitters)if(!std::isfinite(e.position.x) || !std::isfinite(e.position.y) || !std::isfinite(e.position.z) ||
    !std::isfinite(e.home.x) || !std::isfinite(e.home.z))return false;
  return true;
}
void Attachment::Bind(rpg::EntityId actor,const Frame& frame,const EmitterIdentity& accepted){
  *this={};id=actor;if(!id || id==~0u)return;
  if(const auto* e=frame.Find(id)){
    if(accepted.valid && (e->kind!=accepted.kind || e->rank!=accepted.rank || e->home.x!=accepted.home.x || e->home.z!=accepted.home.z)){orphaned=true;return;}
    identity=*e;bound=true;
  }else orphaned=true;
}
bool Attachment::Resolve(const Frame& frame,AcousticPoint& point){
  if(!bound || orphaned)return false;
  const auto* e=frame.Find(id);
  if(!e || e->kind!=identity.kind || e->rank!=identity.rank || e->home.x!=identity.home.x || e->home.z!=identity.home.z){orphaned=true;return false;}
  point=e->position;return true;
}
}
