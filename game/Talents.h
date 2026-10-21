#pragma once
#include <array>

namespace rpg {
// IDs are durable save bits. Append new talents; never reorder existing ones.
enum class Talent { WeaponTraining,Vitality,Restoration,WeaponMastery,Bulwark,Renewal,Executioner,Pursuit,Mercy,Count };
struct TalentInfo {
 const char* name;
 const char* effect;
 const char* description;
 int level;
 Talent prerequisite;
 float strike=0,heavy=0,health=0,armor=0,mend=0,mendCooldown=0,dashCooldown=0;
};
inline constexpr std::array<TalentInfo,int(Talent::Count)> talents{{
 {"Weapon training","+3 Strike / +5 Heavy","Improve both melee attacks with any weapon, including unarmed attacks.",2,Talent::Count,3,5},
 {"Vitality","+20 maximum health","Increase your maximum health. Learning this talent does not restore missing health.",2,Talent::Count,0,0,20},
 {"Restoration","+10 Mend healing","Mend restores up to 45 health instead of 35. It still cannot exceed maximum health.",2,Talent::Count,0,0,0,0,10},
 {"Weapon mastery","+3 Strike / +5 Heavy","Adds another damage bonus on top of Weapon training: +6 Strike and +10 Heavy in total.",3,Talent::WeaponTraining,3,5},
 {"Bulwark","+2 damage reduction","Reduce incoming damage by two, in addition to worn armor. Hits still deal at least one damage.",3,Talent::Vitality,0,0,0,2},
 {"Renewal","Mend cooldown: 8 seconds","Reduce the cooldown of future Mend casts from ten to eight seconds. Active cooldowns keep their remaining time.",3,Talent::Restoration,0,0,0,0,0,2},
 {"Executioner","Heavy: +25% finishing damage","Heavy strike deals 25% more damage when its victim has at most half health at contact. Choose this instead of Weapon mastery.",3,Talent::WeaponTraining},
 {"Pursuit","Dash cooldown: 3.5 seconds","Rejoin your target sooner. Future dashes recover in 3.5 seconds; range, travel and active cooldowns stay unchanged. Alternative to Bulwark.",3,Talent::Vitality,0,0,0,0,0,0,1.5f},
 {"Mercy","Mend: +20 emergency healing","Mend restores 20 extra health if you have at most 35% health before casting. With Restoration: up to 65. Alternative to Renewal.",3,Talent::Restoration}
}};
inline bool ValidTalent(Talent talent) { return int(talent)>=0 && int(talent)<int(Talent::Count); }
inline bool HasTalent(unsigned build,Talent talent) {
 return ValidTalent(talent) && (build&(1u<<unsigned(talent)))!=0;
}
inline int TalentCount(unsigned build) {
 int count=0;
 for (int i=0;i<int(Talent::Count);++i) if (HasTalent(build,Talent(i))) ++count;
 return count;
}
inline float TalentBonus(unsigned build,float TalentInfo::*stat) {
 float value=0;
 for (int i=0;i<int(Talent::Count);++i) if (HasTalent(build,Talent(i))) value+=talents[i].*stat;
 return value;
}
inline bool ValidBuild(unsigned build,int level) {
 if (level<1 || level>10 || (build>>unsigned(Talent::Count))!=0 || TalentCount(build)>(level<=3?level-1:2+(level-3)/2)) return false;
 for (int i=0;i<int(Talent::Count);++i) if (HasTalent(build,Talent(i))) {
  const auto& info=talents[i];
  if (level<info.level || (info.prerequisite!=Talent::Count && !HasTalent(build,info.prerequisite))) return false;
 }
 return true;
}
enum class BuildAction { None,Apply,Store };
struct BuildCommand { BuildAction action=BuildAction::None; int mask=-1,slot=-1; };
struct BuildStats { float health,strike,heavy,armor,mend,dashCooldown,mendCooldown; };
struct BuildPreset { const char* name; unsigned mask; };
inline constexpr std::array<BuildPreset,6> buildPresets{{
 {"Weapon mastery",(1u<<0)|(1u<<3)}, {"Executioner",(1u<<0)|(1u<<6)},
 {"Bulwark",(1u<<1)|(1u<<4)}, {"Pursuit",(1u<<1)|(1u<<7)},
 {"Renewal",(1u<<2)|(1u<<5)}, {"Mercy",(1u<<2)|(1u<<8)}
}};
}
