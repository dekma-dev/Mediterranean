#pragma once
#include "game/Simulation.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace rpg {
// Shared read-only selection for world warnings and the incoming-attack HUD.
// Spatial danger follows accepted hit rules. The boar extension is conservative
// because its remaining charge can still be stopped by scenery or the player.
// Visibility/line of sight is checked once by ReadAttackWarnings below.
inline bool AttackFootprintContainsPlayer(const Simulation& s,const Enemy& e) {
  if(s.Health()<=0 || s.Height()>=1.5f)return false;
  const float dx=s.Player().x-e.position.x,dz=s.Player().z-e.position.z;
  const float distance=std::hypot(dx,dz);
  const float dot=distance<.001f?1.f:(std::sin(e.facing)*dx+std::cos(e.facing)*dz)/distance;
  return e.Hits(distance,dot) || (e.kind==EnemyKind::Boar && distance<=e.WarningReach() && dot>=std::cos(e.HalfAngle()));
}
struct AttackWarning {const Enemy* enemy=nullptr;bool threatens=false;};
template<std::size_t Capacity> struct AttackWarnings {
  std::array<AttackWarning,Capacity> rows{};
  unsigned count=0;
};
template<std::size_t Capacity> AttackWarnings<Capacity> ReadAttackWarnings(const Simulation& s) {
  static_assert(Capacity>0);
  AttackWarnings<Capacity> result;
  if(s.Health()<=0)return result;
  const auto before=[](const AttackWarning& a,const AttackWarning& b){
    if(a.threatens!=b.threatens)return a.threatens;
    if(a.enemy->windup!=b.enemy->windup)return a.enemy->windup<b.enemy->windup;
    return a.enemy->id<b.enemy->id;
  };
  const auto add=[&](const Enemy& e){
    if(!s.VisibleEnemyCast(e) || !std::isfinite(e.position.x+e.position.z+e.facing+e.windup))return;
    const AttackWarning row{&e,AttackFootprintContainsPlayer(s,e)};
    unsigned slot=0;while(slot<result.count && before(result.rows[slot],row))++slot;
    if(slot>=Capacity)return;
    for(unsigned n=std::min<unsigned>(result.count,unsigned(Capacity)-1);n>slot;--n)result.rows[n]=result.rows[n-1];
    result.rows[slot]=row;result.count=std::min<unsigned>(result.count+1,unsigned(Capacity));
  };
  for(const auto& e:s.Combatants())add(e);
  if(s.Sparring())add(s.SparringOpponent());
  return result;
}
}
