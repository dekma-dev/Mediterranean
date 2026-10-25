#pragma once
#include "game/Simulation.h"
#include <array>
#include <algorithm>

namespace ui {
struct IncomingAttack { const rpg::Enemy* enemy=nullptr; bool threatens=false; };
struct IncomingAttacks { std::array<IncomingAttack,3> rows{}; unsigned count=0; };
// Read-only, bounded presentation model. Hidden enemies must not leak through
// the HUD; no target change or simulation mutation is needed to see a cast.
inline IncomingAttacks ReadIncomingAttacks(const rpg::Simulation& simulation) {
  IncomingAttacks result;
  const auto add=[&](const rpg::Enemy& enemy) {
    if(!simulation.VisibleEnemyCast(enemy)) return;
    const float dx=simulation.Player().x-enemy.position.x,dz=simulation.Player().z-enemy.position.z;
    const float distance=std::hypot(dx,dz);
    const float dot=distance<.001f?1.f:(std::sin(enemy.facing)*dx+std::cos(enemy.facing)*dz)/distance;
    const IncomingAttack row{&enemy,enemy.Hits(distance,dot) && simulation.Height()<1.5f};
    unsigned slot=0;
    while(slot<result.count && (result.rows[slot].enemy->windup<enemy.windup ||
      (result.rows[slot].enemy->windup==enemy.windup && result.rows[slot].enemy->id<enemy.id))) ++slot;
    if(slot>=result.rows.size()) return;
    for(unsigned i=std::min<unsigned>(result.count,2);i>slot;--i) result.rows[i]=result.rows[i-1];
    result.rows[slot]=row;result.count=std::min<unsigned>(3,result.count+1);
  };
  if(simulation.Health()<=0) return result;
  for(const auto& enemy:simulation.Enemies()) add(enemy);
  if(simulation.Sparring()) add(simulation.SparringOpponent());
  return result;
}
}
