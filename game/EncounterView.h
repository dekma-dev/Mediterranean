#pragma once
#include "game/AttackWarning.h"
#include <array>
#include <algorithm>

namespace ui {
using IncomingAttack=rpg::AttackWarning;
using IncomingAttacks=rpg::AttackWarnings<3>;
// Read-only, bounded presentation model. Hidden enemies must not leak through
// the HUD; no target change or simulation mutation is needed to see a cast.
inline IncomingAttacks ReadIncomingAttacks(const rpg::Simulation& simulation) {
  return rpg::ReadAttackWarnings<3>(simulation);
}
}
