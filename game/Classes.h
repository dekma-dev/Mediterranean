#pragma once
#include "game/Spells.h"
namespace rpg {
// Durable profile IDs. These are original prototype kits, not imported assets.
enum class ClassId { Warrior,Paladin,Scout,Priest,Druid,Mage,Necromancer,Psionic,Bard,Engineer,Demonologist,Count };
struct ClassInfo {
 const char* name; const char* title; const char* role; const char* identity;
 std::array<const char*,3> branches;
 std::array<const char*,9> nodes;
 std::array<const char*,4> actions;
 float health,armor,strike,heavy,healing,range;
 std::array<float,3> color;
};
inline constexpr std::array<ClassInfo,int(ClassId::Count)> classes{{
 {"Warrior","Frontier guard","Melee / pressure","Heavy contact interrupts an enemy wind-up.",{"Arms","Guard","Spirit"},
 {"Weapon training","Vitality","Restoration","Weapon mastery","Bulwark","Renewal","Executioner","Pursuit","Mercy"},
 {"Strike","Heavy strike","Dash","Mend"},100,0,0,0,35,2.5f,{.22f,.37f,.54f}},
 {"Paladin","Dawn sentinel","Melee / protection","Plate discipline absorbs four damage from each hit.",{"Judgement","Bastion","Devotion"},
 {"Consecrated edge","Sanctuary","Devotion","Templar oath","Stone faith","Absolution","Judgement","Pilgrim","Last prayer"},
 {"Smite","Judgement","Crusade","Prayer"},120,4,-2,2,38,2.5f,{.73f,.60f,.31f}},
 {"Scout","Woodland scout","Ranged / precision","Heavy shots gain damage beyond four metres.",{"Marksmanship","Survival","Medicine"},
 {"Steady aim","Trailcraft","Field dressing","Eagle eye","Bark cover","Quick dressing","Deadeye","Trail runner","Last bandage"},
 {"Quick shot","Aimed shot","Disengage","Bandage"},90,0,-1,0,28,9,{.31f,.48f,.28f}},
 {"Priest","Dawn acolyte","Ranged / healing","Heavy contact also restores eight health to the caster.",{"Radiance","Faith","Grace"},
 {"Radiant word","Faith","Grace","Dawn sermon","Sacred ward","Revelation","Rebuke","Pilgrimage","Deliverance"},
 {"Radiance","Rebuke","Pilgrimage","Greater heal"},95,1,-2,-4,55,8,{.78f,.75f,.57f}},
 {"Druid","Grove keeper","Ranged / nature","Living bark absorbs two damage; strong self-restoration.",{"Storm","Bark","Grove"},
 {"Storm seed","Oak heart","Living sap","Tempest","Ironbark","Spring rain","Thunderhead","Wild trail","Last bloom"},
 {"Thorn","Lightning","Wild step","Regrowth"},110,2,-2,0,48,7,{.26f,.51f,.35f}},
 {"Mage","Astral scholar","Ranged / burst","Heavy spells deal additional damage to healthy targets.",{"Flame","Frost","Arcana"},
 {"Kindling","Frost mantle","Arcane renewal","Inferno","Ice shell","Clear mind","Combustion","Slipstream","Second breath"},
 {"Ember","Flare","Slipstream","Renewal"},80,0,3,7,25,10,{.30f,.44f,.79f}},
 {"Necromancer","Ash binder","Ranged / life drain","Successful attacks restore health equal to 15% of damage.",{"Blood","Bone","Soul"},
 {"Blood pact","Bone mantle","Soul vessel","Crimson rite","Ossuary","Soul harvest","Wither","Grave walk","Death denied"},
 {"Wither","Soul rend","Grave walk","Blood rite"},95,0,0,1,38,8,{.48f,.27f,.56f}},
 {"Psionic","Veil seeker","Ranged / control","Heavy contact breaks concentration and delays the next attack.",{"Will","Mirror","Clarity"},
 {"Focused will","Mirror ward","Clarity","Mind lance","Quiet mind","Lucidity","Shatter","Thoughtstep","Awakening"},
 {"Mind spike","Shatter","Thoughtstep","Clarity"},90,1,0,1,32,9,{.64f,.35f,.69f}},
 {"Bard","Road chronicler","Ranged / sustain","Every landed attack restores three health.",{"Discord","Resolve","Harmony"},
 {"Discord","Resolve","Harmony","Crescendo","Steady rhythm","Refrain","Dissonance","Quickstep","Final refrain"},
 {"Discord","Crescendo","Quickstep","Refrain"},105,1,-2,0,45,7,{.60f,.34f,.25f}},
 {"Engineer","Brass warden","Ranged / impact","Heavy contact deals extra damage against armoured captains.",{"Ordnance","Plating","Repair"},
 {"Bore sight","Plating","Repair kit","Overpressure","Riveted shell","Efficient repair","Armour breaker","Booster","Emergency kit"},
 {"Bolt","Shrapnel","Booster","Repair"},110,3,-1,2,30,8,{.49f,.47f,.34f}},
 {"Demonologist","Ember exile","Melee / sacrifice","Heavy contact sacrifices up to five health for ten extra damage.",{"Ruin","Hide","Hunger"},
 {"Ruin pact","Demon hide","Hunger","Hellforged","Obsidian skin","Feast","Ravager","Onslaught","Refuse death"},
 {"Rend","Ravage","Onslaught","Feast"},125,1,1,5,25,2.5f,{.55f,.20f,.19f}}
}};
inline bool ValidClass(ClassId id) {return int(id)>=0 && int(id)<int(ClassId::Count);}
inline const ClassInfo& DescribeClass(ClassId id) {return classes[ValidClass(id)?int(id):0];}
inline TalentInfo ClassTalent(ClassId id,int index) {
 auto result=talents[index];if(id==ClassId::Warrior) return result;
 result.name=DescribeClass(id).nodes[index];
 // Per-class progression changes the actual stats used by attacks, protection,
 // recovery and build previews. Durable node IDs retain the shared save schema.
 struct Growth {float strike,heavy,health,armor,mend,healingRecovery,mobilityRecovery;};
 constexpr std::array<Growth,int(ClassId::Count)> growth{{
  {3,5,20,2,10,2,1.5f}, {2,6,30,3,12,2,1}, {4,7,15,1,8,2,2},
  {2,4,20,2,20,3,1}, {3,5,25,3,16,2.5f,1.5f}, {4,9,15,1,8,2,2},
  {3,6,20,2,14,2.5f,1}, {3,7,18,2,10,2,2}, {3,5,22,2,18,3,1.5f},
  {3,8,25,3,10,2.5f,1}, {4,8,30,2,12,2,1.5f}
 }};
 const auto& g=growth[ValidClass(id)?int(id):0];
 if(index==0 || index==3) {result.strike=g.strike;result.heavy=g.heavy;}
 if(index==1) result.health=g.health;
 if(index==2) result.mend=g.mend;
 if(index==4) result.armor=g.armor;
 if(index==5) result.mendCooldown=g.healingRecovery;
 if(index==7) result.dashCooldown=g.mobilityRecovery;
 result.description=index==6?"The heavy ability deals 25% extra damage against a target at or below half health. The condition is checked at contact.":
  index==8?"Self healing restores 20 extra health when cast at or below 35% health. Healing still cannot exceed maximum health.":
  "Strengthen this class discipline. The listed bonus adds to current abilities and equipment; learning never refills health or active cooldowns.";
 return result;
}
inline const char* ClassPresetName(ClassId id,int index) {
 constexpr int leaves[]{3,6,4,7,5,8};return ClassTalent(id,leaves[index]).name;
}
// Action clocks drive both contact and authored clips. Resources are earned on
// contact, never on input; a full reserve empowers the next committed heavy.
struct CombatStyle {
 const char* resource;
 const char* payoff;
 float basicRate,heavyRate;
 int capacity;
 float retreatDistance,retreatDuration;
 bool steadyHeavy;
};
inline const CombatStyle& ClassCombat(ClassId id) {
 static constexpr std::array<CombatStyle,int(ClassId::Count)> styles{{
  {"Rage","Empowered heavy: +24 damage.",1,1,3,0,.34f,false},
  {"Conviction","Empowered judgement: absorb the next 24 damage.",.9f,.85f,3,0,.34f,false},
  {"Focus","Empowered aimed shot: +14 damage and a 3 s snare.",1.2f,.65f,3,3.8f,.42f,true},
  {"Faith","Empowered rebuke: restore 24 health and absorb 12 damage.",.95f,.8f,2,2.8f,.46f,true},
  {"Bloom","Empowered lightning: root for 2 s and restore 18 health.",1,.85f,3,3.3f,.42f,false},
  {"Embers","Empowered flare: +36 damage.",.9f,.6f,3,4.f,.38f,true},
  {"Souls","Empowered soul rend: +12 damage and drain 45% of damage.",.95f,.75f,3,2.8f,.46f,true},
  {"Insight","Empowered shatter: stun for 2 s.",1.05f,.7f,3,3.6f,.40f,true},
  {"Tempo","Empowered crescendo: heal 20 and reset basic attack cooldown.",1.15f,.95f,4,3.2f,.4f,false},
  {"Pressure","Empowered shrapnel: expose for 5 s (+25% incoming damage).",1.05f,.7f,3,3.f,.44f,true},
  {"Fury","Empowered ravage: +20 damage and restore 15 health.",1.15f,.9f,3,0,.34f,false}
 }};
 return styles[ValidClass(id)?int(id):0];
}
inline float ClassAttackRate(ClassId id,bool heavy) {
 const auto& style=ClassCombat(id);return heavy?style.heavyRate:style.basicRate;
}
// Recovery wards are capacities, not invulnerability or stackable healing.
inline float ClassRecoveryBarrier(ClassId id) {
 switch(id) {
 case ClassId::Paladin:return 18;
 case ClassId::Priest:return 10;
 case ClassId::Mage:return 14;
 case ClassId::Psionic:return 12;
 case ClassId::Engineer:return 8;
 default:return 0;
 }
}
inline SpellInfo ClassSpell(ClassId id,int index) {
 auto result=spells[index];const auto& info=DescribeClass(id);result.name=info.actions[index];
 if(index<2) {
  result.range=info.range;
  const float rate=ClassAttackRate(id,index==1);
  result.contact/=rate; result.recovery/=rate;
  if(info.range>3) {
   result.description="Commit a ranged attack. Damage resolves at contact against the original target; range and line of sight are checked again.";
   result.targeting="Living enemy / clear line of sight";
  }
  if(index==1 && ClassCombat(id).steadyHeavy)
   result.description="Plant your feet to prepare this heavy attack. Moving or jumping before release interrupts it. A full class reserve empowers the hit.";
 }
 if(index==2 && ClassCombat(id).retreatDistance>0) {
  result.range=ClassCombat(id).retreatDistance;result.recovery=ClassCombat(id).retreatDuration;
  result.targeting="No target required / backward escape";
  result.description="Step backward while facing the threat. Travel follows collision and stops at walls. No teleport or invulnerability.";
 }
 if(index==3 && ClassRecoveryBarrier(id)>0)
  result.description="Restore missing health and establish a class ward. Wards do not stack; the stronger remaining capacity wins. Full health rejects the cast.";
 return result;
}
inline constexpr int levelCap=10;
inline constexpr std::array<int,10> levelExperience{0,100,250,500,750,1000,1250,1500,1750,2000};
inline int EarnedTalentPoints(int level) {return level<=3?level-1:2+(level-3)/2;}
}
