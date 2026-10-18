#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void Require(bool value,const char* message) {
  if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void Wait(rpg::Simulation& game,int steps) {
  for (int i=0;i<steps;++i) game.Update(.05f,{});
}
void BeginSwing(rpg::Simulation& game,int enemy=0) {
  const float health=game.Health();
  for (int i=0;i<50 && game.Enemies()[enemy].windup==0;++i) game.Update(.01f,{});
  Require(game.Enemies()[enemy].windup>0 && game.Health()==health,
    "Enemy must turn and begin its telegraph before dealing damage");
}
const std::string duel="MEDITERRANEAN 1\n-4 5.2 100 3.14159 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n";
bool HasFeedback(const rpg::Simulation& game,rpg::FeedbackKind kind) {
  for (const auto& event : game.Feedback()) if (event.kind==kind) return true;
  return false;
}
}

int main() {
  {
    bool passed=true;
    const auto check=[&](bool value,const char* message) {
      if (!value) { std::cerr<<"Defeat: "<<message<<'\n'; passed=false; }
    };
    const std::string crowd="MEDITERRANEAN 3\n-4 5.8 1 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
      "100 -4 7 80 0 0 1 0.4 3.14159\n101 -4.5 7 80 0 0 1 0.01 2.7468\n102 -4 9.5 120 0 0 1 0 1.0122\n";
    rpg::Simulation defeated;
    Require(defeated.Load(crowd),"Crowded fatal encounter must load");
    const auto before=defeated.Enemies();
    defeated.Update(.05f,{});
    Require(defeated.Health()==0,"Second enemy must resolve the fatal strike");
    check(defeated.Enemies()[0].windup==0,
      "Defeat must cancel an earlier enemy's pending telegraph in the same update");
    check(defeated.Enemies()[2].position.x==before[2].position.x && defeated.Enemies()[2].position.z==before[2].position.z,
      "Enemies later in the update order must not take a final movement step after defeat");
    for (int i=0;i<2;++i)
      check(defeated.Enemies()[i].position.x==before[i].position.x && defeated.Enemies()[i].position.z==before[i].position.z,
        "Cancelling overlapping windups must not permit a separation step on the defeat frame");
    const auto snapshot=defeated.Save();
    Wait(defeated,20);
    check(defeated.Save()==snapshot,"The defeated encounter must remain suspended while cosmetic effects settle");
    rpg::Command recover; recover.respawn=true; defeated.Update(.05f,recover);
    check(defeated.Health()==defeated.MaxHealth(),"Recovery must remain available after cancelled enemy attacks");
    for (const auto& enemy : defeated.Enemies())
      check(enemy.windup==0 && enemy.attackCooldown==0 && enemy.state==rpg::EnemyState::Idle,
        "Recovery must reset surviving enemies without replaying cancelled attacks");
    Require(passed,"Defeat transition regressions failed");
  }
  rpg::Simulation game;
  Require(game.Load(duel),"Duel fixture must load");
  BeginSwing(game);
  Require(game.Enemies()[0].windup>0 && game.Health()==100,"A swing must telegraph before causing damage");
  const auto committed=game.Enemies()[0].position;
  Wait(game,10);
  Require(game.Health()==100 && game.Enemies()[0].position.x==committed.x &&
    game.Enemies()[0].position.z==committed.z,"Windup must hold position and delay damage");
  Wait(game,5);
  Require(game.Health()==92 && HasFeedback(game,rpg::FeedbackKind::Hurt),"An undefended swing must hit once");
  Wait(game,10);
  Require(game.Health()==92,"Recovery must prevent damage every frame");

  Require(game.Load(duel),"Reload duel");
  BeginSwing(game);
  rpg::Simulation restored;
  const auto midSwing=game.Save();
  Require(restored.Load(midSwing) && restored.Save()==midSwing,"Committed swing must survive save/load exactly");
  Wait(restored,15);
  Require(restored.Health()==92,"Loading must retain the pending attack rather than skip it");

  rpg::Command command; command.ability=2;
  game.Update(.05f,command); Wait(game,15);
  Require(game.Health()==100 && HasFeedback(game,rpg::FeedbackKind::Miss),"Dash out of range must avoid a committed swing");

  Require(game.Load(duel),"Reload interrupt scenario");
  BeginSwing(game);
  command={}; command.nextTarget=true; command.ability=1;
  game.Update(.05f,command);
  Require(game.Enemies()[0].windup==0 && game.Enemies()[0].health==45 &&
    HasFeedback(game,rpg::FeedbackKind::Interrupt),"Heavy strike must damage and interrupt a pending swing");
  Wait(game,14);
  Require(game.Health()==100,"Interrupted attack must not resolve later");

  // Stand behind the direction of an already committed swing, still within reach.
  const std::string behind="MEDITERRANEAN 3\n-4 8 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.1 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(game.Load(behind),"Committed-facing scenario must load");
  Wait(game,3);
  Require(game.Health()==100 && HasFeedback(game,rpg::FeedbackKind::Miss),"Moving outside the attack cone must avoid damage");

  const std::string captain="MEDITERRANEAN 3\n0 10.2 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -12 -10 80 0 0 2 0 0\n101 12 -10 80 0 0 2 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(game.Load(captain),"Captain scenario must load");
  BeginSwing(game,2); Wait(game,15);
  Require(game.Health()==100 && game.Enemies()[2].windup>0,"Captain must provide a longer readable windup");
  Wait(game,6);
  Require(game.Health()==84,"Captain must use its own damage profile");

  auto corrupt=midSwing;
  const auto identity=corrupt.find("\n100 ");
  Require(identity!=std::string::npos,"Expected stable identity in save");
  const auto facing=identity+1;
  const auto end=corrupt.find('\n',facing);
  corrupt.replace(facing,end-facing,"100 -4 7 80 0 0 0 0.5 0");
  const auto before=game.Save();
  Require(!game.Load(corrupt) && game.Save()==before,"An idle enemy with a pending swing must be rejected transactionally");

  const std::string fatalCounter="MEDITERRANEAN 3\n-4 5.2 1 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.01 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(game.Load(fatalCounter),"Fatal counterattack fixture must load");
  command={}; command.nextTarget=true; command.ability=0; game.Update(.05f,command);
  Require(game.Health()==0 && game.Enemies()[0].health==60,"Player strike and fatal counterattack must both resolve");
  Require(game.AttackFlash()==0,"Defeat must clear the strike pose instead of leaving the sword raised indefinitely");
  Wait(game,10);
  Require(game.AttackFlash()==0,"A defeated player must not retain an active strike");
  std::cout << "Combat telegraph checks passed.\n";
}
