#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

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
  using namespace rpg;
  Inventory items;
  Require(items.UsedSlots()==1 && items.Bag()[0].item==Item::Potion && items.Worn(GearSlot::Chest)==Item::Tunic,"Starting gear must not be duplicated into the bag.");
  Require(items.Add(Item::Ward) && items.Equip(Item::Ward),"New armor must equip from the bag.");
  Require(items.Count(Item::Tunic)==1 && items.Find(Item::Tunic)>=0 && items.Count(Item::Ward)==1,"Equipping armor must return the tunic to its source cell.");
  Require(!items.Add(Item::Ward) && !items.Apply({InventoryAction::Equip,0,int(GearSlot::Weapon),Item::Potion}),"Duplicate gear and incompatible slots must be rejected.");
  Require(!items.Apply({InventoryAction::Move,-1,0,Item::Potion}) && !items.Apply({InventoryAction::Move,0,24,Item::Potion}),"Out-of-range bag operations must be rejected.");
  Require(items.Apply({InventoryAction::Move,0,23,Item::Potion}) && items.Find(Item::Potion)==23,"Items must move to the last bag cell.");
  Require(!items.Apply({InventoryAction::Move,23,0,Item::Starter}) && items.Find(Item::Potion)==23,"A stale drag must not move a replacement item.");
  Require(items.Apply({InventoryAction::Unequip,int(GearSlot::Chest),22,Item::Ward}) && items.Worn(GearSlot::Chest)==Item::None,"Gear may be stored in an empty bag cell.");
  Require(items.Apply({InventoryAction::Unequip,int(GearSlot::Weapon),0,Item::Starter}) && items.Worn(GearSlot::Weapon)==Item::None,"Main hand may be empty.");
  items.Sort();
  Require(items.Count(Item::Starter)==1 && items.Count(Item::Ward)==1 && items.Count(Item::Potion)==2,"Sorting must conserve ownership and counts.");
  std::ostringstream serialized; items.Write(serialized); Inventory copy; std::istringstream input(serialized.str());
  Require(copy.Read(input) && copy.Worn(GearSlot::Weapon)==Item::None && copy.Count(Item::Ward)==1,"Inventory serialization must preserve empty equipment slots.");
  Inventory full;
  Require(full.Add(Item::Potion,478) && full.UsedSlots()==24 && !full.Add(Item::Watchguard),"A full bag must reject rewards atomically.");
  Require(!full.Apply({InventoryAction::Unequip,int(GearSlot::Weapon),-1,Item::Starter}) && full.Worn(GearSlot::Weapon)==Item::Starter,"Unequip must retain gear when the bag is full.");
  Require(!full.Apply({InventoryAction::Split,0,-1,Item::Potion,10}) && full.Count(Item::Potion)==480,
    "Splitting a full bag must retain the complete source stack.");
  Inventory stacks; Require(stacks.Add(Item::Potion,3),"Prepare odd potion stack.");
  for (int amount : {-1,0,5,6}) Require(!stacks.Apply({InventoryAction::Split,0,23,Item::Potion,amount}),"Invalid split counts must reject.");
  Require(!stacks.Apply({InventoryAction::Split,0,0,Item::Potion,2}) &&
    !stacks.Apply({InventoryAction::Split,0,23,Item::Starter,2}),"Occupied targets and stale split sources must reject.");
  Require(stacks.Apply({InventoryAction::Split,0,23,Item::Potion,2}) && stacks.Bag()[0].count==3 && stacks.Bag()[23].count==2,
    "Split must conserve an odd stack and use the requested empty cell.");
  Require(stacks.Consume(Item::Potion,23) && stacks.Bag()[0].count==3 && stacks.Bag()[23].count==1,
    "Drinking must consume from the clicked stack, not the first matching item.");
  Require(!stacks.Consume(Item::Potion,22) && !stacks.Remove(23,Item::Starter) && stacks.Count(Item::Potion)==4,
    "Stale consumption/removal must not fall back to another stack.");
  Require(stacks.Apply({InventoryAction::Move,23,0,Item::Potion}) && stacks.Bag()[0].count==4 && stacks.Bag()[23].item==Item::None,
    "Dragging split stacks back together must merge them without duplication.");
  Require(stacks.Apply({InventoryAction::Split,0,-1,Item::Potion,2}) && stacks.Bag()[1].count==2,"Automatic split uses the first empty cell.");
  stacks.Sort(); Require(stacks.UsedSlots()==1 && stacks.Bag()[0].count==4,"Sort must recombine split stacks.");
  Simulation splitGame; auto injuredSave=splitGame.Save();
  auto healthAt=injuredSave.find("100 0 0 0"); Require(healthAt!=std::string::npos,"Starting health fixture.");
  injuredSave.replace(healthAt,3,"50"); Require(splitGame.Load(injuredSave),"Injured split fixture loads.");
  command={}; command.inventory={InventoryAction::Split,0,23,Item::Potion,1}; splitGame.Update(.01f,command);
  const auto splitSave=splitGame.Save();
  Require(restored.Load(splitSave) && restored.Save()==splitSave,"Exact split cells must persist.");
  command={}; command.usePotion=true; command.potionSource=22; splitGame.Update(.01f,command);
  Require(splitGame.Health()==50 && splitGame.Potions()==2 && splitGame.PotionCooldown()==0,"A stale potion source cannot grant free healing.");
  command.potionSource=23; splitGame.Update(.01f,command);
  Require(splitGame.Health()==90 && splitGame.Items().Bag()[0].count==1 && splitGame.Items().Bag()[23].item==Item::None,
    "Simulation must consume the selected split stack exactly once.");
  // Whole-game validation rejects forged inventories before replacing live progress.
  Simulation baseline; const auto stable=baseline.Save();
  const auto reject=[&](std::string data) { Require(!baseline.Load(data) && baseline.Save()==stable,"Malformed equipment save must be transactional."); };
  auto bad=stable; auto at=bad.find("GEAR 0 3 4 5"); Require(at!=std::string::npos,"Expected v4 equipment block.");
  bad.replace(at,12,"GEAR 6 3 4 5"); reject(bad);
  bad=stable; at=bad.find("BAG 2 2"); Require(at!=std::string::npos,"Expected v4 potion stack.");
  bad.replace(at,7,"BAG 2 3"); reject(bad);
  bad=stable; at=bad.find(" -1 0",bad.find("BAG")); bad.replace(at,5," 0 1"); reject(bad);
  bad=stable; at=bad.find(" -1 0",bad.find("BAG")); bad.replace(at,5," 99 1"); reject(bad);
  command={}; command.inventory={InventoryAction::Unequip,0,23,Item::Starter}; baseline.Update(.01f,command);
  Require(baseline.StrikeDamage()==6 && baseline.HeavyDamage()==10,"Unarmed combat must use catalog damage.");
  Require(restored.Load(baseline.Save()) && restored.Items().Worn(GearSlot::Weapon)==Item::None,"Unarmed state must survive a game save.");
  command={}; command.equipWeapon=true; baseline.Update(.01f,command);
  Require(baseline.Items().Worn(GearSlot::Weapon)==Item::Starter && baseline.StrikeDamage()==20,
    "G must re-equip the owned starter sword before the patrol reward exists.");
  // An active patrol with a recorded kill is unreachable: that kill must have
  // advanced the quest to Return. Accepting it strands an already cleared map.
  const std::string cleared="MEDITERRANEAN 1\n0 -10 50 0 2 0\n0 0 0 0\n-4 7 0 0\n4 9 0 0\n0 12 0 0\n";
  Simulation migrated; Require(migrated.Load(cleared),"A cleared patrol awaiting hand-in must remain valid.");
  for (auto stranded : {cleared,migrated.Save()}) {
    const auto questField=stranded.find("50 0 2 0");
    Require(questField!=std::string::npos,"Expected patrol quest field in fixture.");
    stranded.replace(questField,8,"50 0 1 0");
    const auto beforeRejected=restored.Save();
    Require(!restored.Load(stranded) && restored.Save()==beforeRejected,
      "A cleared camp with an unadvanced active patrol must reject without replacing progress.");
  }
  std::cout << "Inventory checks passed: swaps, capacity, stale commands, sorting, equipment and v1-v4 persistence.\n";
}
