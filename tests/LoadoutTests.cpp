#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using namespace rpg;
void Require(bool ok,const char* message) {
 if (!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); }
}
const char* veteran="MEDITERRANEAN 3\n0 -10 140 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
const char* novice="MEDITERRANEAN 3\n0 -10 120 0 3 30\n3 1 1 0\n1\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
std::string Record(std::string save,const char* tag,const std::string& replacement) {
 const auto at=save.find(tag); Require(at!=std::string::npos,"Expected save record");
 save.replace(at,save.find('\n',at)-at+1,replacement); return save;
}
void Change(Simulation& game,BuildAction action,int mask,int slot=-1) {
 Command command; command.build={action,mask,slot}; game.Update(.01f,command);
}
void CheckPreview(const Simulation& game,const BuildStats& stats) {
 Require(stats.health==game.MaxHealth() && stats.strike==game.StrikeDamage() && stats.heavy==game.HeavyDamage() &&
  stats.armor==game.DamageReduction() && stats.mend==game.MendHealing() &&
  stats.dashCooldown==game.AbilityCooldown(2) && stats.mendCooldown==game.AbilityCooldown(3),
  "Every preview field must match the applied build with unchanged equipment");
}
}
int main() {
 using namespace rpg;
 Simulation game; Require(game.Load(veteran),"Veteran fixture");
 const auto fresh=game.Save();
 Require(game.Loadout(-1)==-1 && game.Loadout(3)==-1,"Slot queries must be bounds safe");
 for (int slot=0;slot<3;++slot) Require(game.Loadout(slot)==-1,"Old saves start with empty slots");
 // Enumerate masks independently of production validation: exactly the empty
 // build, three foundations, three foundation pairs and six specializations.
 for (const auto fixture : {veteran,novice}) {
  Require(game.Load(fixture),"Point budget fixture");
  const int level=game.Level();
  const auto baseline=game.Save();
  for (int mask=0;mask<512;++mask) {
   Require(game.Load(baseline),"Reset mask fixture");
   const bool allowed=mask==0 || mask==1 || mask==2 || mask==4 ||
     (level==3 && (mask==3 || mask==5 || mask==6 || mask==9 || mask==18 || mask==36 || mask==65 || mask==130 || mask==260));
   const auto preview=game.PreviewBuild(unsigned(mask));
   Require(game.Save()==baseline,"Inspecting a build must not mutate the simulation");
   Change(game,BuildAction::Store,mask,0);
   Require(game.Loadout(0)==(allowed?mask:-1) && game.BuildMask()==0,"Storage must enforce earned points and prerequisites without applying");
   Change(game,BuildAction::Apply,mask);
   Require(game.BuildMask()==unsigned(allowed?mask:0),"Applying must atomically reject an invalid or unearned build");
   if (allowed) {
    CheckPreview(game,preview);
    Require(game.Health()==(level==3?140.f:120.f),"Changing a build cannot heal");
   }
  }
 }
 Require(game.Load(veteran),"Persistent slots fixture");
 Change(game,BuildAction::Store,9,0); Change(game,BuildAction::Store,130,1); Change(game,BuildAction::Store,260,2);
 Change(game,BuildAction::Apply,130);
 const auto stored=game.Save(); Simulation copy;
 Require(copy.Load(stored) && copy.Save()==stored && copy.BuildMask()==130 &&
   copy.Loadout(0)==9 && copy.Loadout(1)==130 && copy.Loadout(2)==260,"All three slots and the active build must round-trip");
 Change(game,BuildAction::Store,3,1);
 Require(game.BuildMask()==130 && game.Loadout(0)==9 && game.Loadout(1)==3 && game.Loadout(2)==260,
   "Overwriting one slot preserves the active build and other slots");
 Change(game,BuildAction::Store,0,2);
 Require(game.Loadout(2)==0,"An empty talent allocation is distinct from an unused slot");
 for (int mask : {-1,-2,512,1024,std::numeric_limits<int>::max()}) {
  const auto stable=game.Save();
  Change(game,BuildAction::Apply,mask); Change(game,BuildAction::Store,mask,0);
  Require(game.Save()==stable,"Malformed commands cannot change the build or slots");
 }
 for (int slot : {-1,3,100}) {
  const auto stable=game.Save(); Change(game,BuildAction::Store,9,slot);
  Require(game.Save()==stable,"Invalid storage destination cannot change state");
 }
 Command invalid; invalid.build={BuildAction(99),9,0};
 auto stable=game.Save(); game.Update(.01f,invalid);
 Require(game.Save()==stable,"Unknown build action must be rejected");
 invalid.build={BuildAction::Apply,9,0};
 for (float dt : {0.f,-1.f,std::numeric_limits<float>::quiet_NaN()}) game.Update(dt,invalid);
 Require(game.Save()==stable,"Invalid time cannot apply a build");

 // Switching while cooldowns run never shortens them, and cycling Vitality
 // cannot heal. Values must follow the selected build on subsequent casts.
 auto wounded=Record(stored,"0 -10 140 ","0 -10 40 0 3 90\n");
 Require(game.Load(wounded),"Wounded fixture");
 Command mend; mend.ability=3; game.Update(.01f,mend);
 const float health=game.Health(),cooldown=game.Cooldown(3);
 Change(game,BuildAction::Apply,36);
 Require(game.Health()==health && std::abs(game.Cooldown(3)-(cooldown-.01f))<.0001f &&
   game.AbilityCooldown(3)==8,"Switching to Renewal cannot heal or shorten an already active cooldown");
 Change(game,BuildAction::Apply,130);
 Require(game.Health()==health && game.MaxHealth()==160,"Restoring Vitality cannot restore health");
 Require(game.Load(Record(stored,"0 -10 140 ","0 -10 160 0 3 90\n")),"Full Vitality fixture");
 Change(game,BuildAction::Apply,9);
 Require(game.Health()==140 && game.MaxHealth()==140,"Losing Vitality clamps excess health");
 Change(game,BuildAction::Apply,130);
 Require(game.Health()==140,"Cycling builds cannot recover the clamped health");
 Require(game.Load(Record(stored,"0 0 0 0 \n","0.8 3 4.5 9\n")),"Four active cooldowns fixture");
 Change(game,BuildAction::Apply,36);
 const float expected[]={.79f,2.99f,4.49f,8.99f};
 for (int i=0;i<4;++i) Require(std::abs(game.Cooldown(i)-expected[i])<.0001f,"All four cooldown clocks survive build switching");

 Require(game.Load(stored),"Airborne fixture"); Command jump; jump.jump=true; game.Update(.01f,jump);
 Change(game,BuildAction::Apply,9); Require(game.BuildMask()==130,"Airborne switching is forbidden");
 Require(game.Load(Record(stored,"0 -10 140 ","12 -3.8 140 0 3 90\n")),"Combat fixture");
 Command interact; interact.interact=true; game.Update(.01f,interact);
 Require(game.Sparring(),"Training must start");
 Change(game,BuildAction::Apply,9); Require(game.BuildMask()==130,"Combat switching is forbidden");
 Change(game,BuildAction::Store,3,0); Require(game.Loadout(0)==3,"Storing a plan during combat does not change the active character");
 Command strike; strike.ability=0; game.Update(.01f,strike);
 Change(game,BuildAction::Apply,9); Require(game.BuildMask()==130,"An attack cannot change specialization before contact");
 Require(game.Load(Record(stored,"0 -10 140 ","0 -10 0 0 3 90\n")),"Defeated fixture");
 Change(game,BuildAction::Apply,9); Change(game,BuildAction::Store,3,0);
 Require(game.BuildMask()==130 && game.Loadout(0)==9,"Defeated characters cannot change builds or slots");
 Command respawn; respawn.respawn=true; game.Update(.01f,respawn);
 Require(game.BuildMask()==130 && game.Loadout(0)==9 && game.Loadout(2)==260,"Recovery preserves saved and active builds");

 // Old version-nine files have the new talent IDs, but no loadout record.
 auto legacy=Record(Record(stored,"PROFILE ",""),"LOADOUTS ",""); legacy=Record(Record(legacy,"APPEARANCE ",""),"SURVEY ",""); legacy=Record(legacy,"TECHNIQUE ",""); legacy=Record(legacy,"SIGNATURE ",""); legacy=Record(legacy,"OPENING ",""); legacy=Record(Record(legacy,"ENCOUNTERS", ""),"EXPEDITION ",""); legacy=Record(legacy,"SOUTHERN ",""); legacy.replace(14,legacy.find('\n')-14,"9");
 Require(copy.Load(legacy) && copy.BuildMask()==130 && copy.Loadout(0)==-1 && copy.Loadout(2)==-1,
   "Version nine retains learned alternatives and migrates empty loadouts");
 Require(copy.Load(copy.Save()) && copy.TalentPoints()==0,"Migration/reload cannot grant extra points");
 Require(copy.Load(stored),"Restore rejection baseline"); stable=copy.Save();
 for (const auto line : {"","LOADOUTS 9 130\n","LOADOUTS 9 130 260 0\n","LOADOUTS -2 130 260\n",
   "LOADOUTS 8 130 260\n","LOADOUTS 7 130 260\n","LOADOUTS 512 130 260\n","LOADOUTS 4294967296 130 260\n",
   "LOADOUTS 9 130 260\nLOADOUTS -1 -1 -1\n"}) {
  Require(!copy.Load(Record(stored,"LOADOUTS ",line)) && copy.Save()==stable,
    "Corrupt, incomplete, duplicated or unearned loadouts must reject the entire save atomically");
 }
 Simulation levelOne;
 Require(!copy.Load(Record(levelOne.Save(),"LOADOUTS ","LOADOUTS 1 -1 -1\n")) && copy.Save()==stable,
   "Slots cannot contain talents the champion has not earned");
 Command restart; restart.restart=true; copy.Update(.01f,restart);
 Require(copy.BuildMask()==0 && copy.Loadout(0)==-1 && copy.Loadout(2)==-1,"New expedition clears the active build and all slots");
 unsigned draft=0;
 Require(!TogglePlannedTalent(draft,Talent::Overwhelming) && draft==0,"Planner accepted missing ancestors");
 for(auto node:{Talent::WeaponTraining,Talent::Tempo,Talent::Pressure,Talent::Overwhelming,Talent::Vitality})
   Require(TogglePlannedTalent(draft,node),"Planner rejected a valid path");
 const auto full=draft;
 Require(!TogglePlannedTalent(draft,Talent::Restoration) && draft==full,"Planner exceeded five points or partially mutated");
 Require(TogglePlannedTalent(draft,Talent::Tempo) && draft==((1u<<unsigned(Talent::WeaponTraining))|(1u<<unsigned(Talent::Vitality))),
   "Removing a draft ancestor did not remove descendants");
 Require(!TogglePlannedTalent(draft,Talent::Count),"Planner accepted invalid node");
 std::cout<<"Build previews, draft planning, persistent slots, switching restrictions and save migration passed.\n";
}
