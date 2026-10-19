#include "game/Simulation.h"
#include "tests/DashFixture.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace {
using namespace rpg;
void Require(bool ok,const char* message) {
  if (!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); }
}
float Distance(Vec2 a,Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
bool Has(const Simulation& s,CombatVisualKind kind) {
  for (std::size_t i=0;i<s.VisualEventCount();++i) if (s.VisualEvents()[i].kind==kind) return true;
  return false;
}
std::string Fixture(Vec2 player,Vec2 enemy,float windup=.7f) {
  std::ostringstream out;
  out<<"MEDITERRANEAN 3\n"<<player.x<<' '<<player.z<<" 100 -1.5707963 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    <<"100 "<<enemy.x<<' '<<enemy.z<<" 80 0 0 1 "<<windup<<" 3.14159\n"
    <<"101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  return out.str();
}
Command Dash() { Command c; c.ability=2; c.nextTarget=true; return c; }
void Reject(const std::string& save,const char* status) {
  Simulation s; Require(s.Load(save),"Rejected target fixture must load");
  Command select; select.nextTarget=true; s.Update(.001f,select);
  const auto position=s.Player(); const auto facing=s.Facing();
  Require(std::string(s.AbilityStatus(2))==status,"HUD must show the target rejection reason");
  Command c; c.ability=2; s.Update(.01f,c);
  Require(!s.Dashing() && !s.DashedThisUpdate() && s.Cooldown(2)==0 &&
    !Has(s,CombatVisualKind::DashStart),"Rejected targeting must consume no cooldown or launch effects");
  Require(Distance(position,s.Player())==0 && s.Facing()==facing,"Rejected dash must not move or turn the champion");
}
}
int main() {
  using namespace rpg;
  Simulation noTarget,walking; Command move; move.movement={1,0};
  for (int i=0;i<10;++i) { noTarget.Update(.01f,move); walking.Update(.01f,move); }
  Require(std::string(noTarget.AbilityStatus(2))=="NO TARGET","Dash must require selection");
  auto rejected=move; rejected.ability=2;
  noTarget.Update(.01f,rejected); walking.Update(.01f,move);
  Require(noTarget.Save()==walking.Save() && !noTarget.DashedThisUpdate() &&
    noTarget.VisualEventCount()==0,"A no-target dash must preserve movement and cooldown");

  Reject(Fixture({-4,5.6f},{-4,7}),"TOO CLOSE");
  Reject(Fixture({-4,1.9f},{-4,7}),"OUT OF RANGE");
  Reject(Fixture({-10,.5f},{-10,5.5f}),"BLOCKED");
  Reject(Fixture({-4,-6.5f},{-4,-3},0),"RETURNING");
  {
    std::string dead=test::dashFixture;
    const std::string actor="100 -4 7 80 0 0 1 0.7 3.14159";
    dead.replace(dead.find(actor),actor.size(),"100 -4 7 0 0 0 3 0 0");
    Simulation s; Require(s.Load(dead),"Dead target fixture"); s.Update(.01f,Dash());
    Require(s.TargetActor() && s.TargetActor()->id!=100 && !s.Dashing() && s.Cooldown(2)==0,
      "Selection must skip a corpse and reject a farther living target");
  }
  for (int rate : {22,30,60,144,1000}) for (Vec2 start : {Vec2{-4,2},Vec2{-4,4},Vec2{-1,7},Vec2{-4,10}}) {
    Simulation s; Require(s.Load(Fixture(start,{-4,7})),"Target approach fixture");
    float elapsed=0,travel=0; int launches=0,finishes=0;
    while (elapsed<.35f-1e-7f) {
      const float dt=std::min(1.f/rate,.35f-elapsed); const auto before=s.Player();
      s.Update(dt,elapsed==0?Dash():Command{}); elapsed+=dt;
      travel+=Distance(before,s.Player());
      launches+=Has(s,CombatVisualKind::DashStart); finishes+=Has(s,CombatVisualKind::DashStop);
      Require(Distance(before,s.Player())<1 && s.CanStand(s.Player()),"Approach must be continuous and collision safe");
      Require(Distance(s.Player(),s.Enemies()[0].position)>=motion::dashStandOff-.0001f,
        "Approach must never pass through or overlap the target");
      const auto direction=Vec2{s.Enemies()[0].position.x-s.Player().x,s.Enemies()[0].position.z-s.Player().z};
      Require(std::abs(motion::AngleDelta(s.Facing(),std::atan2(direction.x,direction.z)))<.0001f,
        "Dash must face the target when starting sideways or backward");
    }
    Require(launches==1 && finishes==1 && !s.Dashing(),"Accepted approach must launch and finish once");
    Require(std::abs(Distance(s.Player(),s.Enemies()[0].position)-motion::dashStandOff)<.0001f &&
      std::abs(travel-(Distance(start,{-4,7})-motion::dashStandOff))<.0001f,
      "Stationary targets must finish at melee stand-off, including shortened dashes");
    Require(travel<=motion::dashDistance+.0001f && s.Cooldown(2)>4.6f,"Travel budget and cooldown must remain bounded");
    Command hit; hit.ability=0; s.Update(.01f,hit);
    Require(s.Enemies()[0].health<80,"Dash arrival must allow a normal melee attack");
  }
  {
    Simulation selected,control;
    const auto save=Fixture({-4,2},{-3,6},0);
    Require(selected.Load(save) && control.Load(save),"Moving target fixture");
    const auto targetStart=selected.Enemies()[0].position; float travel=0;
    for (int i=0;i<35;++i) {
      auto c=i==0?Dash():Command{}; const auto before=selected.Player();
      control.Update(.01f,c);
      if (i==8) c.nextTarget=true;
      selected.Update(.01f,c); travel+=Distance(before,selected.Player());
      Require(Distance(selected.Player(),control.Player())<.0001f,
        "Changing selection must not redirect a dash from its accepted actor");
    }
    Require(selected.TargetActor()->id==101 && Distance(targetStart,selected.Enemies()[0].position)>.01f,
      "Test must change selection while the original target is moving");
    Require(Distance(selected.Player(),selected.Enemies()[0].position)<=motion::dashStandOff+.001f &&
      travel<=motion::dashDistance+.0001f,"Dash must follow a moving target within its travel budget");
  }
  {
    Simulation s; Require(s.Load(test::dashWallFixture),"Grazing wall fixture");
    bool impact=false; float travel=0;
    for (int i=0;i<40;++i) {
      const auto before=s.Player(); s.Update(.01f,i==0?Dash():Command{});
      travel+=Distance(before,s.Player()); impact|=Has(s,CombatVisualKind::DashImpact);
    }
    Require(impact && !s.Dashing() && s.CanStand(s.Player()) && s.Player().z<3.5f &&
      travel<motion::dashDistance,"Body clearance must stop a visible target approach at a wall");
  }
  {
    Simulation s; Require(s.Load(Fixture({-4,-6.5f},{-4,-3},0)),"Target loss fixture");
    s.Update(.01f,Dash()); Require(s.Dashing(),"Dash must start before the actor begins returning");
    const auto before=s.Player(); s.Update(.01f,{});
    Require(!s.Dashing() && Distance(before,s.Player())==0 && Has(s,CombatVisualKind::DashStop),
      "Returning targets must terminate travel without chasing home");
  }
  {
    Simulation s; Require(s.Load(test::dashFixture),"Cancellation fixture"); s.Update(.02f,Dash());
    const auto before=s.Player(); const auto snapshot=s.Save();
    Command halt; halt.haltMovement=true; s.Update(.01f,halt);
    Require(!s.Dashing() && Distance(before,s.Player())==0 && !Has(s,CombatVisualKind::DashStop),
      "Cancellation must discard the accepted target without a false arrival");
    Require(s.Load(snapshot) && !s.Dashing() && Distance(before,s.Player())==0 && !s.TargetActor(),
      "Loading must retain position without restoring transient target travel");
  }
  std::cout<<"PASS: selected-target dash, rejection, moving actor lock, melee arrival, collision and cancellation.\n";
}
