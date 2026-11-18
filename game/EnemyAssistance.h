#pragma once
#include "game/Simulation.h"

namespace rpg {
inline bool CanRequestAssistance(const Enemy& caller,const Enemy& receiver,Vec2 player){
  const auto excluded=[](const Enemy& e){return e.kind==EnemyKind::Dummy||e.kind==EnemyKind::Trainer||e.Boss();};
  if(caller.id==receiver.id||caller.health<=0||receiver.health<=0||excluded(caller)||excluded(receiver)||
     caller.state!=EnemyState::Pursuing||receiver.state!=EnemyState::Idle||caller.stunned>0)return false;
  if(caller.Beast()!=receiver.Beast()||(caller.Beast()&&caller.kind!=receiver.kind))return false;
  const auto squared=[](Vec2 a,Vec2 b){const float x=a.x-b.x,z=a.z-b.z;return x*x+z*z;};
  const float leash=receiver.Beast()?20.f:13.f;
  return squared(caller.position,receiver.position)<=5.5f*5.5f&&squared(player,receiver.home)<=leash*leash;
}
}
