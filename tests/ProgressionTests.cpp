#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void Require(bool value,const char* message) {
  if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
float Distance(rpg::Vec2 a,rpg::Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
void Interact(rpg::Simulation& game) { rpg::Command c; c.interact=true; game.Update(.05f,c); }
void Go(rpg::Simulation& game,rpg::Vec2 destination,float stop=.5f) {
  const auto route=game.Map().FindPath(game.Player(),destination,.4f);
  Require(!route.empty(),"Test destination must have a route");
  for (auto waypoint : route) {
    for (int i=0;i<600 && Distance(game.Player(),waypoint)>stop;++i) {
      rpg::Command c; c.sprint=true;
      c.movement={waypoint.x-game.Player().x,waypoint.z-game.Player().z};
      if (game.Health()<50) { c.ability=3; c.usePotion=true; }
      game.Update(.05f,c);
      Require(game.Health()>0,"Campaign route must be survivable");
    }
  }
  Require(Distance(game.Player(),destination)<=stop+.2f,"Movement must reach campaign objective");
}
void Defeat(rpg::Simulation& game,int index) {
  for (int i=0;i<2000 && game.Enemies()[index].health>0;++i) {
    rpg::Command c;
    if (game.Target()!=index) c.nextTarget=true;
    const auto target=game.Enemies()[index].position;
    if (Distance(game.Player(),target)>2.1f) c.movement={target.x-game.Player().x,target.z-game.Player().z};
    if (game.Health()<65 && game.Cooldown(3)==0) c.ability=3;
    else if (game.Target()==index) c.ability=game.Cooldown(1)==0?1:0;
    c.usePotion=game.Health()<40;
    game.Update(.05f,c);
    Require(game.Health()>0,"Scripted full campaign must be survivable");
  }
  Require(game.Enemies()[index].health==0,"Combat must defeat the selected enemy");
}
}

int main() {
  rpg::Simulation game;
  Require(game.Level()==1 && game.Experience()==0 && game.MaxHealth()==100,"New character must start at level one");
  Interact(game);
  Defeat(game,0);
  Require(game.Experience()==40 && game.QuestState()==rpg::Quest::Return,"Kill must grant XP and patrol credit once");
  Interact(game);
  Require(game.Gold()==5,"First corpse must remain lootable");
  Go(game,rpg::WorldMap::questGiver,1);
  Interact(game);
  Require(game.Level()==2 && game.Experience()==100 && game.Health()==120 && game.MaxHealth()==120,
    "Patrol hand-in must grant level two and restore its increased health");
  Require(game.AnchorState()==rpg::AnchorQuest::Available && game.HasRewardWeapon(),"Patrol must unlock the follow-up quest");
  rpg::Command c; c.equipWeapon=true; game.Update(.05f,c);
  Require(game.StrikeDamage()==30 && game.HeavyDamage()==50,"Level and equipment bonuses must combine");
  Interact(game); Interact(game);
  Require(game.AnchorState()==rpg::AnchorQuest::Active && game.Gold()==30 && game.Experience()==100,
    "Accepting and repeating the follow-up dialogue must not duplicate rewards");
  Defeat(game,1); Defeat(game,2);
  Require(game.DefeatedCount()==3 && game.Experience()==180,"All three distinct kills must grant exactly 120 XP");
  Go(game,rpg::WorldMap::astralAnchor);
  {
    auto rebound=game; rebound.StopMovement();
    for(int i=0;i<11;++i) { rpg::Command jump; jump.jump=i==0 || i==10; rebound.Update(.05f,jump); }
    const float speed=rebound.VerticalSpeed();
    const float contact=2*rebound.Height()/(std::sqrt(speed*speed+48*rebound.Height())-speed);
    rpg::Command use; use.interact=true; rebound.Update(contact,use);
    Require(rebound.Height()==0 && rebound.VerticalSpeed()>0,"Anchor fixture must end at rebound takeoff");
    Require(!rebound.Channeling(),"Rising at zero height must not permit anchor stabilization");
  }
  Interact(game);
  Require(game.Channeling(),"Cleared camp must allow anchor stabilization");
  const auto channelPosition=game.Player();
  game.Update(.01f,{});
  Require(Distance(channelPosition,game.Player())==0,
    "Accepted anchor channel must discard approach momentum instead of drifting while casting");
  c={}; c.movement={1,0}; game.Update(.05f,c);
  Require(!game.Channeling() && game.AnchorState()==rpg::AnchorQuest::Active,"Movement must cancel channel without quest credit");
  Interact(game);
  for (int i=0;i<20;++i) game.Update(.05f,{});
  Require(game.Channeling(),"Anchor must require the full channel time");
  rpg::Simulation restored;
  Require(restored.Load(game.Save()) && !restored.Channeling() && restored.AnchorState()==rpg::AnchorQuest::Active,
    "An unfinished channel must not be credited by saving and loading");
  for (int i=0;i<45;++i) game.Update(.05f,{});
  Require(game.AnchorState()==rpg::AnchorQuest::Return,"Completed channel must make the reward claimable");
  Go(game,rpg::WorldMap::questGiver,1); Interact(game);
  Require(game.AnchorState()==rpg::AnchorQuest::Complete && game.HasWardArmor() && game.DamageReduction()==2,
    "Final reward must grant ward armor");
  Require(game.Level()==3 && game.Experience()==300 && game.Health()==140 && game.Gold()==80,
    "Final hand-in must grant level three, restored health, and 50 gold");
  const auto completed=game.Save();
  Interact(game);
  Require(game.Experience()==300 && game.Gold()==80,"Final reward must not be duplicated");
  Require(restored.Load(completed) && restored.Save()==completed,"Complete campaign must round-trip through version three");

  // Player death preserves the two-quest campaign; Enter only resets living foes.
  auto fallen=completed;
  const auto health=fallen.find(" 140 ");
  Require(health!=std::string::npos,"Expected level-three health in campaign save");
  fallen.replace(health,5," 0 ");
  Require(restored.Load(fallen),"Dead veteran must be loadable");
  c={}; c.respawn=true; restored.Update(.05f,c);
  Require(restored.Health()==140 && restored.Level()==3 && restored.Gold()==80 && restored.DefeatedCount()==3 &&
    restored.RewardWeaponEquipped() && restored.HasWardArmor(),"Respawn must retain durable campaign rewards and defeated enemies");
  Require(Distance(restored.Player(),rpg::WorldMap::spawn)==0,"Respawn must return to the village");

  const std::string fallenPatrol="MEDITERRANEAN 2\n-4 6 0 0 2 5\n3 0 0 0\n0 0 0 0\n"
    "100 -4 7 0 0 1 3\n101 4 7 30 1 0 1\n102 0 12 80 0 0 0\n";
  Require(restored.Load(fallenPatrol),"Version two must migrate before respawn");
  restored.Update(.05f,c);
  Require(restored.Experience()==40 && restored.Gold()==5 && restored.Potions()==3 && restored.Enemies()[0].looted,
    "Respawn must preserve loot and partial quest progress");
  Require(restored.Enemies()[1].health==80 && restored.Enemies()[2].health==120 &&
    Distance(restored.Enemies()[1].position,restored.Enemies()[1].home)==0,"Living enemies must recover at their homes");
  Require(restored.AnchorState()==rpg::AnchorQuest::Locked,"Legacy incomplete patrol must keep the second quest locked");

  // Reject impossible quest completion and over-level health without replacing the current game.
  auto forged=completed;
  const auto dead=forged.find("100 ",forged.find('\n')+1);
  const auto end=forged.find('\n',dead);
  Require(dead!=std::string::npos,"Completed save must contain stable enemy ID");
  forged.replace(dead,end-dead,"100 -4 7 80 0 0 0 0 0");
  const auto before=restored.Save();
  Require(!restored.Load(forged) && restored.Save()==before,"Anchor completion with a living raider must be rejected");
  auto lowLevel=before;
  const auto hp=lowLevel.find(" 100 ");
  Require(hp!=std::string::npos,"Fixture must contain level-one health");
  lowLevel.replace(hp,5," 140 ");
  Require(!restored.Load(lowLevel),"Health above the earned level maximum must be rejected");
  c={}; c.restart=true; restored.Update(.05f,c);
  Require(restored.Experience()==0 && restored.Level()==1 && !restored.HasWardArmor() && restored.Gold()==0,
    "Explicit restart must clear all progression");
  std::cout << "Two-quest campaign and progression checks passed.\n";
}
