#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>

void Require(bool value,const char* message) {
  if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
  rpg::Simulation game;
  rpg::Command command;
  command.usePotion=true; game.Update(.01f,command);
  Require(game.Potions()==2 && game.PotionCooldown()==0,"Full health must retain potion");
  // Legacy v1 save: injured player beside a dead raider, other enemies distant.
  const std::string legacy="MEDITERRANEAN 1\n-4 6 50 0 2 0\n0 0 0 0\n-4 7 0 0\n4 9 80 0\n0 12 80 0\n";
  Require(game.Load(legacy),"Legacy save must migrate to current inventory defaults");
  command={}; command.interact=true; game.Update(.01f,command);
  Require(game.Gold()==5 && game.Potions()==3 && game.Enemies()[0].looted,"Corpse must provide one loot bundle");
  game.Update(.01f,command);
  Require(game.Gold()==5 && game.Potions()==3,"Loot must not be claimed twice");
  command={}; command.usePotion=true; game.Update(.01f,command);
  Require(game.Potions()==2 && game.Health()==90,"Potion must consume one item and restore health");
  game.Update(.01f,command);
  Require(game.Potions()==2,"Potion cooldown must prevent repeated consumption");
  const auto save=game.Save();
  rpg::Simulation restored;
  Require(restored.Load(save) && restored.Save()==save,"Inventory and loot flags must survive save/load");
  Require(!restored.Load(save+"junk") && restored.Save()==save,"Rejected save must preserve inventory");
  const std::string completed="MEDITERRANEAN 1\n0 -10 100 0 3 25\n0 0 0 0\n-4 7 0 0\n4 9 80 0\n0 12 80 0\n";
  Require(game.Load(completed) && game.HasRewardWeapon(),"Completed legacy quest must grant its new equipment reward");
  const float starter=game.StrikeDamage();
  command={}; command.equipWeapon=true; game.Update(.01f,command);
  Require(game.RewardWeaponEquipped() && game.StrikeDamage()>starter,"Equipping reward must increase attack damage");
  Require(restored.Load(game.Save()) && restored.RewardWeaponEquipped(),"Equipment state must persist");
  game.Update(.01f,command);
  Require(!game.RewardWeaponEquipped() && game.StrikeDamage()==starter,"Equipment toggle must restore starter damage");
  auto malformed=game.Save();
  const auto id=malformed.find("100 -4");
  Require(id!=std::string::npos,"Fixture must contain expected enemy ID");
  malformed.replace(id,3,"999");
  Require(!restored.Load(malformed),"Unknown entity ID must be rejected");
  std::cout << "Inventory checks passed.\n";
}
