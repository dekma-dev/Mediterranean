#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

void Require(bool condition,const char* message) {
  if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
float Distance(rpg::Vec2 a,rpg::Vec2 b) {
  return std::hypot(a.x-b.x,a.z-b.z);
}
int main() {
  rpg::Simulation game;
  Require(!game.CanStand({-8,-8}),"Building interior must block movement");
  Require(!game.CanStand({18,0}),"Island edge must block movement");
  Require(game.CanStand({0,0}),"Main path must remain walkable");
  Require(game.CameraDistance({0,-8},1.4f,{-1,0},0,10)<6,"Camera must pull forward before entering a building");
  Require(game.CameraDistance({0,-8},5.7f,{-1,0},0,10)<5,"Camera must avoid the pitched roof above the wall bounds");
  Require(game.CameraDistance({10,10},3.2f,{1,0},0,6)<2.2f,"Camera must avoid the tree crown, not only its trunk");
  const float belowCrown=game.CameraDistance({10,10},1.4f,{1,0},0,6);
  Require(belowCrown>3 && belowCrown<4,"Below foliage the camera must avoid the trunk without colliding with empty air");
  Require(game.CameraDistance({12,-3.8f},1.4f,{0,-std::cos(.45f)},std::sin(.45f),7)>6.5f,
    "Training-post camera must be able to retreat underneath the tree crown");
  Require(game.CameraDistance({0,-8},8.f,{-1,0},0,10)==10,"Camera above the roof envelope should remain unobstructed");
  Require(game.CanStand({12.5f,10}),"Expanded camera bounds must not block movement underneath foliage");

  rpg::Command command;
  command.movement={-1,0}; command.sprint=true;
  for (int i=0;i<100;++i) game.Update(.05f,command);
  Require(game.Player().x>-5,"Sprint must not tunnel into the west building");
  Require(game.CanStand(game.Player()),"Movement must leave player in a valid position");
  game=rpg::Simulation{};
  command={}; command.ability=0; game.Update(.05f,command);
  Require(game.Cooldown(0)==0,"Attack without target must not consume cooldown");
  command={}; command.nextTarget=true; game.Update(.05f,command);
  command={}; command.ability=0; game.Update(.05f,command);
  Require(game.Cooldown(0)==0,"Out-of-range attack must not consume cooldown");
  game=rpg::Simulation{};
  command={}; command.jump=true; game.Update(.05f,command);
  Require(game.Height()>0,"Jump must leave ground");
  command={}; for (int i=0;i<80;++i) game.Update(.05f,command);
  Require(game.Height()==0,"Jump must land without sinking below terrain");

  game=rpg::Simulation{};
  command={}; command.interact=true; game.Update(.05f,command);
  Require(game.QuestState()==rpg::Quest::Available && game.QuestDialogue()==rpg::QuestId::Patrol,"Beacon must offer a dialogue before accepting");
  command={}; command.questAction=rpg::QuestAction::AcceptPatrol; game.Update(.01f,command);
  Require(game.QuestState()==rpg::Quest::Active,"Accept button must start the patrol");
  command={}; command.nextTarget=true; game.Update(.05f,command);
  Require(game.Target()>=0,"Tab must select a living enemy");
  for (int i=0;i<1000 && game.QuestState()!=rpg::Quest::Return;++i) {
    Require(game.Health()>0,"Patrol combat scenario must be survivable");
    const auto target=game.Enemies()[game.Target()].position;
    const auto player=game.Player();
    command={};
    if (Distance(target,player)>2.1f) command.movement={target.x-player.x,target.z-player.z};
    command.ability=game.Cooldown(1)==0?1:0;
    game.Update(.05f,command);
  }
  Require(game.QuestState()==rpg::Quest::Return,"Defeating a raider must advance patrol");
  for (int i=0;i<400 && Distance(game.Player(),rpg::Simulation::questGiver)>2.5f;++i) {
    command={}; command.sprint=true;
    command.movement={rpg::Simulation::questGiver.x-game.Player().x,rpg::Simulation::questGiver.z-game.Player().z};
    game.Update(.05f,command);
  }
  command={}; command.interact=true; game.Update(.05f,command);
  Require(game.QuestState()==rpg::Quest::Return && game.Gold()==0,"Talking must preview the reward without granting it");
  command={}; command.questAction=rpg::QuestAction::CompletePatrol; game.Update(.01f,command);
  Require(game.QuestState()==rpg::Quest::Complete && game.Gold()==25,"Complete button must award patrol reward");
  game.Update(.05f,command);
  Require(game.Gold()==25,"Repeated interaction must not duplicate reward");

  const std::string save=game.Save();
  rpg::Simulation restored;
  Require(restored.Load(save),"Valid save must load");
  Require(restored.Save()==save,"Save/load must preserve durable simulation state");
  Require(!restored.Load("MEDITERRANEAN 999\n"),"Unsupported version must be rejected");
  Require(!restored.Load(save+"garbage"),"Trailing corruption must be rejected");
  Require(!restored.Load(save.substr(0,save.size()/2)),"Truncated save must be rejected");
  Require(restored.Save()==save,"Failed load must not mutate current state");
  const std::string deadSave="MEDITERRANEAN 1\n0 -10 0 0 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n";
  rpg::Simulation dead;
  Require(dead.Load(deadSave),"A defeated character save must load");
  command={}; command.movement={0,1}; command.ability=3;
  dead.Update(.05f,command);
  Require(dead.Health()==0 && dead.Player().z==-10,"Death must block movement and self-healing");
  command.restart=true; dead.Update(.05f,command);
  Require(dead.Health()==100,"Restart must recover from death");
  command={}; restored.Update(std::numeric_limits<float>::quiet_NaN(),command);
  Require(restored.Save()==save,"Invalid timestep must not corrupt simulation");
  command.restart=true; restored.Update(.05f,command);
  Require(restored.Gold()==0 && restored.Health()==100 && restored.QuestState()==rpg::Quest::Available,
    "Restart must reset the session");
  const std::string chasing="MEDITERRANEAN 2\n12 1 100 0 0 0\n2 0 0 0\n0 0 0 0\n100 -4 7 80 0 0 0\n101 8 1 80 0 0 1\n102 0 12 80 0 0 0\n";
  rpg::Simulation navigator;
  Require(navigator.Load(chasing),"Navigation scenario must be a valid save");
  command={};
  for (int i=0;i<180;++i) navigator.Update(.05f,command);
  Require(Distance(navigator.Enemies()[1].position,navigator.Player())<2,
    "Pursuing enemy must navigate around the tower, not remain stuck against it");
  Require(navigator.CanStand(navigator.Enemies()[1].position,.45f),"Navigating enemy must remain outside obstacles");
  rpg::Simulation replayA,replayB;
  for (int i=0;i<240;++i) {
    command={}; command.movement={i<120?.3f:-.3f,1}; command.jump=i==40; command.nextTarget=i==100;
    replayA.Update(1.f/60,command); replayB.Update(1.f/60,command);
  }
  Require(replayA.Save()==replayB.Save(),"Identical command streams must reproduce the same state in one build");
  std::cout << "Simulation checks passed.\n";
}
