#pragma once
#include "game/Simulation.h"
#include <cmath>

namespace rpg {
inline bool UsesMeleeFormation(const Enemy& e) {
  return e.health>0 && e.state==EnemyState::Pursuing && !e.Boss() &&
    (e.kind==EnemyKind::Raider || e.kind==EnemyKind::Reaver || e.kind==EnemyKind::Captain);
}

// Recovery-only local formation. No reserved slots or saved tactical state:
// dead/controlled neighbours and a moving player cannot leave stale claims.
// The caller supplies world clearance; physical actor sweeps still own movement.
template<class Range,class Clear>
Vec2 MeleeRecoveryGoal(const Enemy& actor,Vec2 player,const Range& actors,Clear clear) {
  const auto distance=[](Vec2 a,Vec2 b){return std::hypot(a.x-b.x,a.z-b.z);};
  if(!UsesMeleeFormation(actor) || actor.windup>0 || actor.rooted>0 || actor.stunned>0 ||
     actor.attackCooldown<=.25f || distance(actor.position,player)>3.8f)
    return actor.position;
  const auto pressure=[&](Vec2 p) {
    float value=0;
    for(const auto& other:actors) {
      if(other.id==actor.id || !UsesMeleeFormation(other) || distance(other.position,player)>4.f) continue;
      const float crowd=std::max(0.f,2.7f-distance(p,other.position));
      value+=crowd*crowd;
    }
    return value;
  };
  const auto corridor=[&](Vec2 p) {
    const float dx=p.x-actor.position.x,dz=p.z-actor.position.z;
    const float length2=dx*dx+dz*dz;
    if(length2<1e-8f)return false;
    const float px=player.x-actor.position.x,pz=player.z-actor.position.z;
    const float pt=std::clamp((px*dx+pz*dz)/length2,0.f,1.f);
    const float nearPlayerX=px-pt*dx,nearPlayerZ=pz-pt*dz;
    if(nearPlayerX*nearPlayerX+nearPlayerZ*nearPlayerZ+1e-7f<
       std::min(1.25f*1.25f,px*px+pz*pz))return false;
    for(const auto& other:actors) {
      if(other.id==actor.id || other.health<=0)continue;
      const float x=other.position.x-actor.position.x,z=other.position.z-actor.position.z;
      const float t=std::clamp((x*dx+z*dz)/length2,0.f,1.f);
      const float nearX=x-t*dx,nearZ=z-t*dz;
      if(nearX*nearX+nearZ*nearZ+1e-7f<std::min(.9f*.9f,x*x+z*z))return false;
    }
    return true;
  };
  const float initial=pressure(actor.position);
  if(initial<.16f) return actor.position;
  const float bearing=std::atan2(actor.position.x-player.x,actor.position.z-player.z);
  const float radius=std::clamp(distance(actor.position,player),1.65f,1.85f);
  const float side=actor.id&1?1.f:-1.f;
  Vec2 goal=actor.position;
  float best=initial-.12f; // Hysteresis prevents perpetual shuffling at equilibrium.
  for(float turn:{.28f,-.28f,.55f,-.55f,.85f,-.85f,1.1f,-1.1f}) {
    const Vec2 candidate{player.x+std::sin(bearing+turn*side)*radius,
                         player.z+std::cos(bearing+turn*side)*radius};
    const float score=pressure(candidate)+.18f*distance(candidate,actor.position);
    if(score>=best || distance(candidate,actor.home)>12.5f || !corridor(candidate) || !clear(candidate)) continue;
    best=score;goal=candidate;
  }
  return goal;
}
}
