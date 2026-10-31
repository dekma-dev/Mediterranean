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
 {"Strike","Heavy strike","Dash","Iron guard"},100,0,0,0,35,2.5f,{.22f,.37f,.54f}},
 {"Paladin","Dawn sentinel","Melee / protection","Plate discipline absorbs four damage from each hit.",{"Judgement","Bastion","Devotion"},
 {"Consecrated edge","Sanctuary","Devotion","Templar oath","Stone faith","Absolution","Judgement","Pilgrim","Last prayer"},
 {"Smite","Judgement","Crusade","Prayer"},120,4,-2,2,38,2.5f,{.73f,.60f,.31f}},
 {"Scout","Woodland scout","Ranged / precision","Heavy shots gain damage beyond four metres.",{"Marksmanship","Survival","Medicine"},
 {"Steady aim","Trailcraft","Field dressing","Eagle eye","Bark cover","Quick dressing","Deadeye","Trail runner","Last bandage"},
 {"Quick shot","Aimed shot","Disengage","Evasive stance"},90,0,-1,0,28,9,{.31f,.48f,.28f}},
 {"Priest","Dawn acolyte","Ranged / healing","Heavy contact also restores eight health to the caster.",{"Radiance","Faith","Grace"},
 {"Radiant word","Faith","Grace","Dawn sermon","Sacred ward","Revelation","Rebuke","Pilgrimage","Deliverance"},
 {"Radiance","Rebuke","Pilgrimage","Greater heal"},95,1,-2,-4,55,8,{.78f,.75f,.57f}},
 {"Druid","Grove keeper","Ranged / nature","Living bark absorbs two damage; strong self-restoration.",{"Storm","Bark","Grove"},
 {"Storm seed","Oak heart","Living sap","Tempest","Ironbark","Spring rain","Thunderhead","Wild trail","Last bloom"},
 {"Thorn","Lightning","Wild step","Regrowth"},110,2,-2,0,48,7,{.26f,.51f,.35f}},
 {"Mage","Astral scholar","Ranged / burst","Heavy spells deal additional damage to healthy targets.",{"Flame","Frost","Arcana"},
 {"Kindling","Frost mantle","Arcane renewal","Inferno","Ice shell","Clear mind","Combustion","Slipstream","Second breath"},
 {"Ember","Flare","Slipstream","Frost ward"},80,0,3,7,25,10,{.30f,.44f,.79f}},
 {"Necromancer","Ash binder","Ranged / life drain","Successful attacks restore health equal to 15% of damage.",{"Blood","Bone","Soul"},
 {"Blood pact","Bone mantle","Soul vessel","Crimson rite","Ossuary","Soul harvest","Wither","Grave walk","Death denied"},
 {"Wither","Soul rend","Grave walk","Bone pact"},95,0,0,1,38,8,{.48f,.27f,.56f}},
 {"Psionic","Veil seeker","Ranged / control","Heavy contact breaks concentration and delays the next attack.",{"Will","Mirror","Clarity"},
 {"Focused will","Mirror ward","Clarity","Mind lance","Quiet mind","Lucidity","Shatter","Thoughtstep","Awakening"},
 {"Mind spike","Shatter","Thoughtstep","Inner focus"},90,1,0,1,32,9,{.64f,.35f,.69f}},
 {"Bard","Road chronicler","Ranged / sustain","Every landed attack restores three health.",{"Discord","Resolve","Harmony"},
 {"Discord","Resolve","Harmony","Crescendo","Steady rhythm","Refrain","Dissonance","Quickstep","Final refrain"},
 {"Discord","Crescendo","Quickstep","Refrain"},105,1,-2,0,45,7,{.60f,.34f,.25f}},
 {"Engineer","Brass warden","Ranged / impact","Heavy contact deals extra damage against armoured captains.",{"Ordnance","Plating","Repair"},
 {"Bore sight","Plating","Repair kit","Overpressure","Riveted shell","Efficient repair","Armour breaker","Booster","Emergency kit"},
 {"Bolt","Shrapnel","Booster","Deploy plating"},110,3,-1,2,30,8,{.49f,.47f,.34f}},
 {"Demonologist","Ember exile","Melee / sacrifice","Heavy contact sacrifices up to five health for ten extra damage.",{"Ruin","Hide","Hunger"},
 {"Ruin pact","Demon hide","Hunger","Hellforged","Obsidian skin","Feast","Ravager","Onslaught","Refuse death"},
 {"Rend","Ravage","Onslaught","Blood carapace"},125,1,1,5,25,2.5f,{.55f,.20f,.19f}}
}};
inline bool ValidClass(ClassId id) {return int(id)>=0 && int(id)<int(ClassId::Count);}
inline const ClassInfo& DescribeClass(ClassId id) {return classes[ValidClass(id)?int(id):0];}
inline bool ClassHealer(ClassId id);
inline TalentInfo ClassTalent(ClassId id,int index) {
 auto result=talents[index];
 if(index>=9) {
  static constexpr const char* names[11][9]={
   {"Measured cuts","Veteran lungs","Steady guard","Relentless pressure","Tempered steel","Shield discipline","Battle fury","Unbroken advance","Iron resolve"},
   {"Litany of blows","Templar endurance","Quiet prayer","Judicial rhythm","Sacred bastion","Swift devotion","Dawn verdict","Pilgrim's shelter","Grace overflowing"},
   {"Quick nock","Trail endurance","Prepared escape","Hunter's rhythm","Hide armour","Fleet preparation","Perfect aim","Covered retreat","Survival reserve"},
   {"Radiant cadence","Enduring faith","Focused prayer","Sermon of wrath","Sacred resilience","Swift benediction","Divine rebuke","Pilgrim's refuge","Abundant grace"},
   {"Thorn rhythm","Deep roots","Still waters","Gathering storm","Ancient bark","Spring awakening","Eye of the storm","Wild shelter","Living abundance"},
   {"Spark cadence","Astral constitution","Crystalline focus","Accelerant","Frozen mantle","Clear casting","Astral conflagration","Slipstream shelter","Frozen reserve"},
   {"Grave cadence","Undying vessel","Bone focus","Rending rhythm","Ossified flesh","Swift binding","Soul eruption","Grave shelter","Death's reserve"},
   {"Thought cadence","Mind over flesh","Inner stillness","Fracture rhythm","Mental fortress","Lucid focus","Mind collapse","Veiled passage","Boundless insight"},
   {"Quick refrain","Long breath","Perfect pitch","Rising tempo","Unbroken resolve","Swift harmony","Grand crescendo","Traveller's song","Resonant abundance"},
   {"Rapid feed","Reinforced frame","Prepared plating","Pressure regulator","Hardened rivets","Quick assembly","Overcharged shell","Safety harness","Reserve plating"},
   {"Rending rhythm","Infernal endurance","Blood focus","Relentless ruin","Obsidian bulwark","Swift pact","Cataclysm","Sheltered onslaught","Blood reserve"}};
  result.name=names[ValidClass(id)?int(id):0][index-9];return result;
 }
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
 if(!ClassHealer(id)) {
   if(index==2 || index==5) result.description="Strengthen the class defensive ability. Recovery power grants half as much barrier capacity; this does not heal.";
   if(index==8) result.description="The class defensive ability grants ten additional barrier capacity at or below 35% health. This does not heal.";
 }
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
  {"Conviction","Empowered judgement: absorb the next 24 damage.",.85f,.72f,3,0,.34f,false},
  {"Focus","Empowered aimed shot: +14 damage and a 3 s snare.",1.1f,.38f,3,3.8f,.42f,true},
  {"Faith","Empowered rebuke: restore 24 health and absorb 12 damage.",.6f,.35f,2,2.8f,.46f,true},
  {"Bloom","Empowered lightning: root for 2 s and restore 18 health.",.7f,.40f,3,3.3f,.42f,false},
  {"Embers","Empowered flare: +36 damage.",.4f,.25f,3,4.f,.38f,true},
  {"Souls","Empowered soul rend: +12 damage and drain 45% of damage.",.6f,.38f,3,2.8f,.46f,true},
  {"Insight","Empowered shatter: stun for 2 s.",.65f,.4f,3,3.6f,.40f,true},
  {"Tempo","Empowered crescendo: heal 20 and reset basic attack cooldown.",.9f,.65f,4,3.2f,.4f,false},
  {"Pressure","Empowered shrapnel: expose for 5 s (+25% incoming damage).",1.f,.5f,3,3.f,.44f,true},
  {"Fury","Empowered ravage: +20 damage and restore 15 health.",.95f,.75f,3,0,.34f,false}
 }};
 return styles[ValidClass(id)?int(id):0];
}
inline float ClassAttackRate(ClassId id,bool heavy) {
 const auto& style=ClassCombat(id);return heavy?style.heavyRate:style.basicRate;
}
// Recovery slots have class mechanics, not universal self healing.
inline bool ClassHealer(ClassId id) {
 return id==ClassId::Paladin || id==ClassId::Priest || id==ClassId::Druid || id==ClassId::Bard;
}
inline float ClassRecoveryCast(ClassId id) {
 switch(id) {
 case ClassId::Paladin:return .9f;case ClassId::Priest:return 1.5f;
 case ClassId::Druid:return 1.2f;case ClassId::Bard:return 1.f;
 case ClassId::Engineer:return .8f;default:return 0;
 }
}
inline float ClassRecoveryCost(ClassId id) {
 return id==ClassId::Necromancer?10.f:id==ClassId::Demonologist?8.f:0.f;
}
inline float ClassRecoveryBarrier(ClassId id) {
 constexpr float wards[]{30,18,0,10,0,35,40,18,0,40,45};
 return wards[ValidClass(id)?int(id):0];
}
inline bool AdvancedClass(ClassId id) {return id==ClassId::Warrior || id==ClassId::Mage || id==ClassId::Scout;}
inline float AdvancedRate(ClassId id,int index) {
 if(index==10) return id==ClassId::Warrior?1.1f:id==ClassId::Scout?.7f:.85f;
 if(index==11) return id==ClassId::Warrior?.32f:id==ClassId::Scout?.15f:.12f;
 return index==8?(id==ClassId::Warrior?.70f:id==ClassId::Scout?.50f:.32f):
   (id==ClassId::Warrior?.48f:id==ClassId::Scout?.25f:.18f);
}
inline SpellInfo ClassSpell(ClassId id,int index) {
 if(!ValidSpell(index)) return spells[0];
 auto result=spells[index];const auto& info=DescribeClass(id);
 if(index>=8) {
   result.range=info.range>3?info.range:3.2f;
   result.contact/=AdvancedRate(id,index);result.recovery/=AdvancedRate(id,index);
   if(!AdvancedClass(id)) {result.name="Untrained discipline";result.description="This discipline is not available to this class.";return result;}
   if(index>=10) {
     if(index==11) result.targeting="Full reserve / living enemy";
     if(id==ClassId::Warrior) {
       result.range=index==11?3.8f:2.5f;
       result.name=index==10?"Pommel strike":"Iron cyclone";
       result.description=index==10?"Level 6. A quick 50% heavy blow stuns for 1.4 seconds, interrupts a vulnerable cast and grants one Rage on contact.":
         "Level 10. Spend full Rage to strike every enemy within 3.8 metres for 150% heavy damage. Grants a 25-point ward on contact. Requires a living selected target.";
     } else if(id==ClassId::Mage) {
       result.name=index==10?"Arcane severance":"Firestorm";
       result.description=index==10?"Level 6. A mobile 60% heavy arcane bolt interrupts a vulnerable cast and grants one Ember on contact. No root or healing.":
         "Level 10. Spend full Embers on a stationary 2.5-second cast. Deal 175% heavy damage within 4.5 metres of the selected target. Moving before release cancels it without refund.";
     } else {
       result.name=index==10?"Disarming arrow":"Horizon shot";
       result.description=index==10?"Level 6. A mobile 55% heavy arrow interrupts a vulnerable cast and slows for four seconds. Keep distance while controlling the enemy.":
         "Level 10. Spend full Focus on a stationary two-second shot for 175% heavy damage. Gain 80% additional heavy damage when the target is over six metres away at contact.";
     }
     return result;
   }
   if(id==ClassId::Warrior) {
     result.name=index==8?"Sweeping steel":"Shieldbreaker";
     result.description=index==8?"Level 5. Strike enemies in a forward 3.2 m arc for 85% heavy damage. Targets beyond the arc or behind obstacles are safe.":
       "Level 8. A deliberate 125% heavy blow. Exposes for four seconds, stuns for one second and interrupts a vulnerable enemy. Grants a ten-point ward on contact.";
   } else if(id==ClassId::Mage) {
     result.name=index==8?"Frost bloom":"Falling star";
     result.description=index==8?"Level 5. Freeze enemies within three metres of the selected target for two seconds; deal 65% heavy damage. Stand still until release.":
       "Level 8. A long stationary cast strikes enemies within three metres of the selected target for 140% heavy damage. Movement before release cancels it.";
   } else {
     result.name=index==8?"Fanned arrows":"Piercing shot";
     result.description=index==8?"Level 5. A narrow forward fan deals 70% heavy damage and slows for two seconds. Can be fired while moving; each arrow checks line of sight.":
       "Level 8. A stationary 130% heavy shot exposes the target for five seconds. Rooted targets take 35% additional heavy damage.";
   }
   return result;
 }
 if(index==7) {
  static constexpr const char* names[]={"Rending cut","Oath strike","Trail shot","Guiding light","Thorn dart","Kindle","Grave touch","Thought spike","Upbeat","Tracer round","Predator claw"};
  static constexpr const char* effects[]={
   "90% basic damage, one Rage and three seconds of exposure.",
   "90% basic damage, one Conviction and a 12-point ward.",
   "90% basic damage, one Focus and a two-second slow. Can be fired while moving.",
   "90% basic damage and two Faith on contact.",
   "90% basic damage, one Bloom and a one-second root.",
   "90% basic damage and two Embers on contact. Can be cast while moving.",
   "90% basic damage, one Soul and a ward equal to half the damage dealt.",
   "90% basic damage, one Insight; interrupts a pending enemy attack.",
   "90% basic damage, one Tempo; removes two seconds from escape cooldown.",
   "90% basic damage, one Pressure and a three-second slow. Can be fired while moving.",
   "90% basic damage, one Fury; heals for half the damage if the target was at half health or less."};
  const int n=ValidClass(id)?int(id):0;result.name=names[n];result.description=effects[n];result.range=info.range;
  const float rate=ClassAttackRate(id,false)*.85f;result.contact/=rate;result.recovery/=rate;return result;
 }
 if(index==6) {
  static constexpr const char* names[]={"Execution","Dawn verdict","Deadeye","Sanctuary strike","Wild judgement","Glacial shatter","Soul eclipse","Mind prison","Grand finale","Overload round","Infernal bargain"};
  static constexpr const char* effects[]={
   "Consumes full Rage: 110% heavy damage, doubled against enemies at half health or below.",
   "Consumes full Conviction: 110% heavy damage and a 40-point ward on contact.",
   "Consumes full Focus: 110% heavy damage, plus 70% against rooted targets; refreshes escape on contact.",
   "Consumes full Faith: 110% heavy damage, heals 30 and grants a 10-point ward on contact.",
   "Consumes full Bloom: 110% heavy damage, roots for four seconds and slows for six.",
   "Consumes full Embers: 110% heavy damage; consumes an existing slow or root for double damage.",
   "Consumes full Souls: 110% heavy damage and restores 80% of damage dealt.",
   "Consumes full Insight: 110% heavy damage; interrupts and stuns for 2.5 seconds.",
   "Consumes full Tempo: 110% heavy damage; refreshes the basic attack and reduces technique cooldown by five seconds.",
   "Consumes full Pressure: 110% heavy damage; exposes the target for eight seconds.",
   "Consumes full Fury and up to 12 health: 160% heavy damage; grants a 25-point ward on contact."};
  const int n=ValidClass(id)?int(id):0;result.name=names[n];result.description=effects[n];result.range=info.range;
  const float rate=ClassAttackRate(id,true)*.7f;result.contact/=rate;result.recovery/=rate;return result;
 }
 if(index==5) {
  static constexpr const char* names[]={"Hamstring","Radiant seal","Pinning arrow","Absolution","Briar grasp","Winter lance","Soul harvest","Mind fracture","Dissonance","Armor piercer","Blood rend"};
  static constexpr const char* effects[]={
   "65% heavy damage; slows for four seconds.",
   "65% heavy damage; grants a 24-point ward on contact.",
   "65% heavy damage; roots for two seconds. Movement before release cancels the shot.",
   "65% heavy damage; heals for half the damage dealt.",
   "65% heavy damage; roots for three seconds.",
   "65% heavy damage, or 110% against a slowed/rooted enemy; slows for three seconds.",
   "65% heavy damage; heals for 60% of damage dealt.",
   "65% heavy damage; stuns for 1.5 seconds and interrupts wind-up.",
   "65% heavy damage; interrupts wind-up and exposes for four seconds.",
   "65% heavy damage; exposes for six seconds. Movement before release cancels the shot.",
   "85% heavy damage; costs six health on commitment, never reducing the caster below one."};
  result.name=names[ValidClass(id)?int(id):0];result.description=effects[ValidClass(id)?int(id):0];result.range=info.range;
  const float rate=ClassAttackRate(id,true)*.8f;result.contact/=rate;result.recovery/=rate;return result;
 }
 result.name=info.actions[index];
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
 if(index==3) {
   result.contact=ClassRecoveryCast(id); result.recovery=result.contact+.3f;
   result.targeting=ClassHealer(id)?"Self / missing health / stationary cast":"Self / defensive preparation";
   result.description=ClassHealer(id)?"Stand still to cast healing. Movement, jumping, damage or Escape interrupts it. Healing occurs only on completion.":
     id==ClassId::Scout?"Evade the next hit within three seconds. Does not heal or make you invulnerable to later attacks.":
     id==ClassId::Warrior?"Raise a guard that absorbs damage and earns Rage when struck. Does not heal.":
     id==ClassId::Mage?"Raise a frost ward. Attacks that strike it slow the attacker. Does not heal.":
     id==ClassId::Demonologist?"Sacrifice eight health for a carapace and reduce Ravage's remaining cooldown by two seconds. Does not heal.":
     id==ClassId::Necromancer?"Sacrifice ten health to form a bone barrier. Cannot be used without enough health. Does not heal.":
     id==ClassId::Psionic?"Fill Insight and form a mental ward. Does not heal.":
     id==ClassId::Engineer?"Stand still to deploy protective plating. Movement or damage interrupts preparation. Does not heal.":
     "Raise a protective barrier. Absorbs incoming damage without healing; stronger wards replace weaker ones.";
 }

 return result;
}
inline constexpr int levelCap=10;
inline constexpr std::array<int,10> levelExperience{0,100,250,500,750,1000,1250,1500,1750,2000};
inline int EarnedTalentPoints(int level) {return level<=3?level-1:2+(level-3)/2;}
}
