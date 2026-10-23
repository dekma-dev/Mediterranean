#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

namespace {
using namespace rpg;
void Require(bool ok,const char* message) {
 if (!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); }
}
const char* levelTwo="MEDITERRANEAN 3\n0 -10 120 0 3 30\n3 1 1 0\n1\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
const char* veteran="MEDITERRANEAN 3\n0 -10 140 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
void Learn(Simulation& game,Talent talent) {
 Command command; command.learnTalent=int(talent); game.Update(.01f,command);
}
std::string Record(std::string save,const char* tag,const std::string& replacement) {
 const auto at=save.find(tag); Require(at!=std::string::npos,"Expected save record");
 save.replace(at,save.find('\n',at)-at+1,replacement); return save;
}
std::string Player(std::string save,float x,float z,float health) {
 auto start=save.find('\n')+1;if(save.compare(start,8,"PROFILE " )==0) start=save.find('\n',start)+1;
 const auto end=save.find('\n',start);
 std::istringstream in(save.substr(start,end-start));
 float oldX,oldZ,oldHealth,facing; int quest,gold;
 Require(bool(in>>oldX>>oldZ>>oldHealth>>facing>>quest>>gold),"Expected player record");
 std::ostringstream line; line<<x<<' '<<z<<' '<<health<<' '<<facing<<' '<<quest<<' '<<gold;
 save.replace(start,end-start,line.str()); return save;
}
void Training(Simulation& game) {
 Require(game.Load(Player(game.Save(),12,-3.8f,game.Health())),"Training position must load");
 Command use; use.interact=true; game.Update(.01f,use);
 Require(game.Sparring(),"Veteran must start training");
}
void Advance(Simulation& game,float seconds,int hz=60) {
 while (seconds>1e-6f) {
  const float dt=std::min(seconds,1.f/hz); game.Update(dt,{}); seconds-=dt;
 }
}
void ExpandedBuildChecks(const std::string& veteranSave) {
 Simulation game;
 for(int spell=0;spell<4;++spell) {
  Require(game.AbilityCooldown(spell)==spells[spell].cooldown,"Foundation spellbook cooldowns must match simulation");
  if (spell<2) Require(spells[spell].contact==combat::Contact(spell==1) &&
    spells[spell].recovery==combat::Duration(spell==1),"Spellbook melee timings must match the authoritative clock");
 }
 for(int invalid : {-1,4,100}) Require(game.AbilityCooldown(invalid)==0 && game.Cooldown(invalid)==0,
   "Invalid spell queries must not index cooldown storage");
 for(int hz : {30,60,144}) {
  Require(game.Load(veteranSave),"Executioner fixture");
  Learn(game,Talent::WeaponTraining); Learn(game,Talent::Executioner);
  Require(game.HeavyDamageAgainst(90,180)==game.HeavyDamage()*1.25f &&
    game.HeavyDamageAgainst(90.01f,180)==game.HeavyDamage(),
    "Executioner must include exactly half health and leave healthy targets unchanged");
  Training(game);
  for(int hit=0;hit<5 && game.SparringOpponent().health>90;++hit) {
   Command strike; strike.ability=0; game.Update(.01f,strike); Advance(game,1,hz);
  }
  const float before=game.SparringOpponent().health;
  Require(before>game.HeavyDamage() && before<=90,"Finishing fixture must distinguish empowered Heavy from ordinary Heavy");
  const float expected=std::min(before,game.HeavyDamageAgainst(before,180));
  Command heavy; heavy.ability=1; game.Update(.01f,heavy);
  Advance(game,.29f,hz);
  Require(game.SparringOpponent().health==before,"Finishing damage cannot arrive before contact");
  Advance(game,.02f,hz);
  Require(std::abs(before-game.SparringOpponent().health-expected)<.0001f,
    "Executioner must affect actual Heavy contact across update rates");
  Simulation restored; Require(restored.Load(game.Save()) && restored.HasTalent(Talent::Executioner),
    "Executioner must persist after a real encounter");
 }
 for(float health : {49.f,49.01f,139.f}) {
  Require(game.Load(Player(veteranSave,0,-10,health)),"Mercy fixture");
  Learn(game,Talent::Restoration); Learn(game,Talent::Mercy);
  const float expected=std::min(140.f,health+(health<=49?65.f:45.f));
  Command mend; mend.ability=3; game.Update(.01f,mend);
  Require(std::abs(game.Health()-expected)<.0001f && game.Cooldown(3)==10,
    "Mercy must check pre-cast health, include its exact threshold, and cap real healing");
  Simulation restored; Require(restored.Load(game.Save()) && restored.Save()==game.Save(),
    "New spell talents and their cooldowns must round-trip");
 }
 Require(game.Load(veteranSave),"Pursuit fixture");
 Learn(game,Talent::Vitality); Learn(game,Talent::Pursuit); Training(game);
 Command retreat; retreat.movement={0,-1}; retreat.sprint=true;
 for(int i=0;i<20 && std::hypot(game.Player().x-game.SparringOpponent().position.x,
   game.Player().z-game.SparringOpponent().position.z)<3.2f;++i) game.Update(.05f,retreat);
 Command dash; dash.ability=2; game.Update(.01f,dash);
 Require(game.DashedThisUpdate() && game.Cooldown(2)==3.5f,"Pursuit must shorten an accepted dash, not just its tooltip");
 Simulation loaded; Require(loaded.Load(game.Save()) && loaded.HasTalent(Talent::Pursuit) && loaded.Cooldown(2)==3.5f,
   "Pursuit and its remaining cooldown must round-trip while transient travel resets");
 Command reset; reset.resetTalents=true; game.Update(.01f,reset);
 Require(game.HasTalent(Talent::Pursuit),"Resetting while dashing or sparring must be rejected");
 loaded.Update(.01f,reset);
 Require(loaded.Cooldown(2)>3.48f && loaded.AbilityCooldown(2)==5 && loaded.TalentPoints()==2,
   "Removing Pursuit must preserve remaining cooldown while restoring future casts to five seconds");

 Require(game.Load(Player(veteranSave,0,-10,60)),"Reset cooldown fixture");
 Learn(game,Talent::Restoration); Learn(game,Talent::Renewal);
 Command mend; mend.ability=3; game.Update(.01f,mend);
 const float health=game.Health();
 game.Update(.01f,reset);
 Require(game.TalentPoints()==2 && game.Health()==health && game.Cooldown(3)>7.98f &&
   game.AbilityCooldown(3)==10,"Reset must refund points without healing or replacing active cooldowns");
 Learn(game,Talent::Vitality); Learn(game,Talent::Bulwark);
 Require(game.Health()==health && game.MaxHealth()==160,"Relearning maximum health cannot heal");
 auto high=Player(game.Save(),0,-10,160);
 Require(game.Load(high),"High-health reset fixture");
 game.Update(.01f,reset);
 Require(game.Health()==140 && game.MaxHealth()==140 && game.TalentPoints()==2,"Reset must clamp health after losing Vitality");
 game.Update(.01f,reset);
 Require(game.TalentPoints()==2,"Repeated resets cannot mint points");
 Learn(game,Talent::Vitality);
 Command jump; jump.jump=true; game.Update(.01f,jump); game.Update(.01f,reset);
 Require(game.HasTalent(Talent::Vitality),"Airborne reset must be rejected");
 Require(game.Load(Player(high,0,-10,0)),"Defeated reset fixture"); game.Update(.01f,reset);
 Require(game.HasTalent(Talent::Vitality),"Defeated reset must be rejected");
 Require(game.Load(high),"Invalid reset time fixture");
 game.Update(0,reset); game.Update(std::numeric_limits<float>::quiet_NaN(),reset);
 Require(game.Save()==high,"Invalid elapsed time cannot change a build");
 // A veteran requires all campaign enemies dead, so use a valid level-two
 // encounter to check the combat gate without weakening save validation.
 Simulation novice; Require(novice.Load(levelTwo),"Combat reset fixture"); Learn(novice,Talent::Vitality);
 const auto combat=Record(novice.Save(),"101 ","101 0 -8 80 0 0 1 0.5 3.14159\n");
 Require(game.Load(combat),"Hostile reset fixture"); game.Update(.01f,reset);
 Require(game.HasTalent(Talent::Vitality),"Pursuing enemies must prevent talent resets");
}
}
int main() {
 using namespace rpg;
 Simulation fresh;
 Require(fresh.TalentPoints()==0 && fresh.StrikeBonus()==0 && fresh.MaxHealth()==100 &&
   fresh.MendHealing()==35 && fresh.AbilityCooldown(3)==10,"Level one must keep its original foundation abilities");
 const auto initial=fresh.Save();
 for (int index : {-2,0,3,6,99}) {
  Command command; command.learnTalent=index; fresh.Update(.01f,command);
  Require(fresh.Save()==initial,"Invalid or level-locked learning must not change progression");
 }
 Simulation novice; Require(novice.Load(levelTwo) && novice.TalentPoints()==1,"Level two must earn exactly one point");
 const auto noviceSave=novice.Save();
 Learn(novice,Talent::WeaponTraining);
 Require(novice.TalentPoints()==0 && novice.StrikeDamage()==33 && novice.HeavyDamage()==55,
   "Weapon training must improve actual equipped-weapon damage");
 const auto learnedOnce=novice.Save();
 Learn(novice,Talent::WeaponTraining); Learn(novice,Talent::Vitality); Learn(novice,Talent::WeaponMastery);
 Require(novice.Save()==learnedOnce,"Duplicate, overspent and level-three learning must be rejected");
 Simulation master; Require(master.Load(veteran) && master.TalentPoints()==2,"Level three must retain two earned points");
 const auto veteranSave=master.Save(); const float strike=master.StrikeDamage(),heavy=master.HeavyDamage();
 ExpandedBuildChecks(veteranSave);
 Learn(master,Talent::WeaponMastery);
 Require(!master.HasTalent(Talent::WeaponMastery) && master.TalentPoints()==2,"A top-rank talent requires its own branch");
 Learn(master,Talent::WeaponTraining); Learn(master,Talent::WeaponMastery);
 Require(master.TalentPoints()==0 && master.StrikeDamage()==strike+6 && master.HeavyDamage()==heavy+10,
   "A full Arms branch must stack both damage ranks");
 Training(master);
 const float before=master.SparringOpponent().health;
 Command attack; attack.ability=0; master.Update(.01f,attack);
 for(int i=0;i<18;++i) master.Update(.01f,{});
 Require(master.SparringOpponent().health==before-master.StrikeDamage(),"Talents must affect a real strike, not only the panel");
 Simulation hybrid; Require(hybrid.Load(veteranSave),"Hybrid fixture");
 Learn(hybrid,Talent::Vitality); Learn(hybrid,Talent::WeaponTraining);
 Require(hybrid.TalentPoints()==0 && hybrid.MaxHealth()==160 && hybrid.Health()==140 && hybrid.StrikeDamage()==strike+3,
   "Two first-rank talents may form a hybrid; extra maximum health must not heal");
 Simulation guard; Require(guard.Load(veteranSave),"Guard fixture");
 Learn(guard,Talent::Vitality); Learn(guard,Talent::Bulwark);
 Require(guard.DamageReduction()==4,"Bulwark must add to the two points of worn ward protection");
 Command mend; mend.ability=3; guard.Update(.01f,mend);
 Require(guard.Health()==160,"Mend must fill the newly increased maximum health");
 Simulation restored;
 Require(restored.Load(guard.Save()) && restored.Health()==160 && restored.MaxHealth()==160 &&
   restored.HasTalent(Talent::Bulwark),"Talent-dependent health must round-trip");
 auto fallen=Player(guard.Save(),0,-10,0);
 Require(restored.Load(fallen),"Fallen talented champion must load");
 Command recover; recover.respawn=true; restored.Update(.01f,recover);
 Require(restored.Health()==160 && restored.HasTalent(Talent::Bulwark) && restored.TalentPoints()==0,
   "Respawn must retain the build and restore its actual maximum health");
 Training(guard);
 for (int i=0;i<100 && guard.SparringDamageTaken()==0;++i) guard.Update(.05f,{});
 Require(guard.SparringDamageTaken()==4,"An actual eight-damage training hit must respect Bulwark plus armor");
 Simulation healer; Require(healer.Load(Player(veteranSave,0,-10,60)),"Injured healer fixture");
 Learn(healer,Talent::Restoration); healer.Update(.01f,mend);
 Require(healer.Health()==105 && healer.Cooldown(3)==10,"Restoration must increase a real Mend cast to 45 health");
 Learn(healer,Talent::Renewal);
 Require(healer.AbilityCooldown(3)==8 && healer.Cooldown(3)>9.9f,
   "Renewal must shorten future casts without resetting an existing cooldown");
 Require(restored.Load(healer.Save()) && restored.Save()==healer.Save(),"An inherited ten-second cooldown must remain loadable");
 for (int i=0;i<201;++i) healer.Update(.05f,{});
 healer.Update(.01f,mend);
 Require(healer.Health()==140 && healer.Cooldown(3)==8,"Improved Mend must cap healing and start the reduced cooldown");
 Require(restored.Load(veteranSave),"Reset learning guard fixture");
 Training(restored); Learn(restored,Talent::WeaponTraining);
 Require(restored.TalentPoints()==2 && !restored.HasTalent(Talent::WeaponTraining),"Learning must be blocked during sparring");
 auto combat=std::string(levelTwo);
 combat=Record(combat,"101 ","101 0 -8 80 0 0 1 0.5 3.14159\n");
 Require(restored.Load(combat),"Live-combat talent fixture must load"); Learn(restored,Talent::Vitality);
 Require(restored.TalentPoints()==1 && !restored.HasTalent(Talent::Vitality),"Learning must be blocked during a hostile attack");
 Require(restored.Load(Player(veteranSave,0,-10,0)),"Defeated talent fixture");
 Learn(restored,Talent::Vitality);
 Require(restored.TalentPoints()==2 && !restored.HasTalent(Talent::Vitality),"Defeat must block learning without losing points");
 Require(restored.Load(veteranSave),"Reset invalid-input fixture");
 Command learn; learn.learnTalent=0;
 restored.Update(0,learn); restored.Update(std::numeric_limits<float>::quiet_NaN(),learn);
 Require(restored.Save()==veteranSave,"Invalid update times cannot spend points");
 Command jump; jump.jump=true; restored.Update(.01f,jump); Learn(restored,Talent::Vitality);
 Require(!restored.HasTalent(Talent::Vitality),"Airborne learning must be rejected");

 // Version seven has no BUILD record. Migration derives all earned points once.
 auto old=Record(Record(Record(veteranSave,"PROFILE ",""),"BUILD ",""),"LOADOUTS ",""); old=Record(Record(old,"APPEARANCE ",""),"SURVEY ",""); old.replace(14,old.find('\n')-14,"7");
 Require(restored.Load(old) && restored.Save()==veteranSave && restored.TalentPoints()==2,"Version seven must migrate with unspent points");
 Require(restored.Load(restored.Save()) && restored.TalentPoints()==2,"Reloading must not duplicate earned points");
 const auto stable=restored.Save();
 const auto reject=[&](const std::string& save) {
  Require(!restored.Load(save) && restored.Save()==stable,"Rejected talent saves must preserve the current champion");
 };
 reject(Record(veteranSave,"BUILD ",""));
 reject(Record(veteranSave,"BUILD ","BUILD 64\n"));
 reject(Record(veteranSave,"BUILD ","BUILD 512\n")); // Unknown talent bit.
 reject(Record(veteranSave,"BUILD ","BUILD -1\n"));
 reject(Record(veteranSave,"BUILD ","BUILD -4294967295\n"));
 reject(Record(veteranSave,"BUILD ","BUILD 4294967296\n"));
 reject(Record(veteranSave,"BUILD ","BUILD 8\n")); // Missing prerequisite.
 reject(Record(veteranSave,"BUILD ","BUILD 7\n")); // Three points, only two earned.
 reject(Record(noviceSave,"BUILD ","BUILD 9\n")); // Rank two requires level three.
 reject(Record(initial,"BUILD ","BUILD 1\n")); // No points at level one.
 reject(veteranSave+"BUILD 0\n");
 reject(Player(veteranSave,0,-10,160)); // Maximum health without Vitality.
 Require(restored.Load(hybrid.Save()) && restored.Save()==hybrid.Save(),"A mixed build must round-trip exactly");
 auto version8=Record(Record(hybrid.Save(),"PROFILE ",""),"LOADOUTS ",""); version8=Record(Record(version8,"APPEARANCE ",""),"SURVEY ",""); version8.replace(14,version8.find('\n')-14,"8");
 Require(restored.Load(version8) && restored.Save()==hybrid.Save(),"Version-eight learned builds must migrate without losing choices");
 auto newInOld=Record(version8,"BUILD ","BUILD 65\n");
 Require(!restored.Load(newInOld),"Version eight must not accept talent IDs introduced in version nine");
 Command restart; restart.restart=true; restored.Update(.01f,restart);
 Require(restored.Level()==1 && restored.TalentPoints()==0 && !restored.HasTalent(Talent::Vitality),
   "An explicit new expedition must clear all build choices");
 std::cout<<"Champion talents, combat effects and save migration checks passed.\n";
}
