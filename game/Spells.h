#pragma once
#include <array>
#include "game/Talents.h"

namespace rpg {
struct SpellInfo {
 const char* name;
 const char* key;
 const char* description;
 const char* targeting;
 float cooldown,range,contact,recovery;
 std::array<Talent,3> upgrades;
};
// Shared book/hotbar metadata. Simulation owns dynamic values and validation.
inline constexpr std::array<SpellInfo,4> spells{{
 {"Strike","1","A quick weapon cut. Damage lands at contact, after the wind-up. Moving or changing target does not redirect a committed strike.",
  "Living enemy / line of sight",.9f,2.5f,.18f,.52f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Count}},
 {"Heavy strike","2","A deliberate weapon blow. A successful contact interrupts an enemy's unfinished attack. Leaving melee range makes the blow miss.",
  "Living enemy / line of sight",4.f,2.5f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Executioner}},
 {"Dash","3","Rush toward the selected enemy and stop at melee distance. Obstacles can end the approach early. This grants no invulnerability.",
  "Enemy / more than 1.5 m away",5.f,5.f,0,.34f,{Talent::Vitality,Talent::Pursuit,Talent::Count}},
 {"Mend","4","Restore your own health immediately. Healing is capped at maximum health. Casting at full health consumes no cooldown.",
  "Self / missing health",10.f,0,0,0,{Talent::Restoration,Talent::Renewal,Talent::Mercy}}
}};
inline bool ValidSpell(int index) { return index>=0 && index<int(spells.size()); }
}
