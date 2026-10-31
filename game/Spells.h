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
inline constexpr std::array<SpellInfo,12> spells{{
 {"Strike","1","A quick weapon cut. Damage lands at contact, after the wind-up. Moving or changing target does not redirect a committed strike.",
  "Living enemy / line of sight",.9f,2.5f,.18f,.52f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Count}},
 {"Heavy strike","2","A deliberate weapon blow. A successful contact interrupts an enemy's unfinished attack. Leaving melee range makes the blow miss.",
  "Living enemy / line of sight",4.f,2.5f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Executioner}},
 {"Dash","3","Rush toward the selected enemy and stop at melee distance. Obstacles can end the approach early. This grants no invulnerability.",
  "Enemy / more than 1.5 m away",5.f,5.f,0,.34f,{Talent::Vitality,Talent::Pursuit,Talent::Count}},
 {"Mend","4","Restore your own health immediately. Healing is capped at maximum health. Casting at full health consumes no cooldown.",
  "Self / missing health",10.f,0,0,0,{Talent::Restoration,Talent::Renewal,Talent::Mercy}},
 {"Potion","5","","",0,0,0,0,{Talent::Count,Talent::Count,Talent::Count}},
 {"Technique","6","A committed class technique.","Living enemy / line of sight",12,2.5f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Count}},
 {"Signature","7","Spend a full class reserve on a signature attack.","Full reserve / living enemy",18,2.5f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Overwhelming}},
 {"Opening","8","Build reserve with a class setup attack.","Living enemy / line of sight",8,2.5f,.18f,.52f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Count}},
 {"Veteran technique","9","An advanced class discipline.","Living enemy / line of sight",14,3.2f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Count}},
 {"Master technique","0","An advanced class discipline.","Living enemy / line of sight",24,3.2f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Executioner}},
 {"Control discipline","Ctrl+6","A class control discipline.","Living enemy / line of sight",16,3.2f,.30f,.76f,{Talent::WeaponTraining,Talent::WeaponMastery,Talent::Count}},
 {"Capstone discipline","Ctrl+7","A full-reserve finishing discipline.","Full reserve / living enemy",32,3.2f,.30f,.76f,{Talent::WeaponTraining,Talent::Overwhelming,Talent::Executioner}}
}};
inline int SpellLevel(int index) {return index==8?5:index==9?8:index==10?6:index==11?10:1;}
inline bool ValidSpell(int index) { return index>=0 && index!=4 && index<int(spells.size()); }
}
