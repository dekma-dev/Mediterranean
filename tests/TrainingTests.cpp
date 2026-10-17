#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>

void Require(bool value,const char* message) { if (!value) { std::cerr<<message<<'\n'; std::exit(1); } }
std::string Veteran(int health=140) {
  return "MEDITERRANEAN 3\n12 -3.8 "+std::to_string(health)+" 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
}
void Interact(rpg::Simulation& s) { rpg::Command c; c.interact=true; s.Update(.01f,c); }
void Wait(rpg::Simulation& s,int frames) { for (int i=0;i<frames;++i) s.Update(.05f,{}); }
int main() {
  using namespace rpg;
  Simulation armored,plain;
  Require(armored.Load(Veteran()) && plain.Load(Veteran()),"Training fixtures must migrate.");
  Command c; c.inventory={InventoryAction::Unequip,int(GearSlot::Chest),23,Item::Ward}; plain.Update(.01f,c);
  Interact(armored); Interact(plain);
  Require(armored.Sparring() && armored.TargetActor() && armored.TargetActor()->kind==EnemyKind::Trainer,"Practice must select a dedicated opponent.");
  Wait(armored,20); Wait(plain,20);
  Require(armored.Health()==134 && plain.Health()==132,"Actual sparring hits must apply the worn armor's reduction.");
  Require(armored.SparringDamageTaken()==6 && plain.SparringDamageTaken()==8,"Training report must count actual damage taken.");
  const auto durable=armored.Save(); Simulation loaded;
  Require(loaded.Load(durable) && !loaded.Sparring() && !loaded.TargetActor() && loaded.Health()==armored.Health(),"Loading ends transient practice without refunding health.");
  for (int i=0;i<500 && armored.Sparring();++i) {
    c={}; c.ability=armored.Cooldown(1)==0?1:0; armored.Update(.05f,c);
  }
  Require(!armored.Sparring() && armored.SparringResult()=="Victory" && armored.SparringDamageDealt()==180,"The repeatable opponent must be defeatable.");
  Require(armored.Gold()==90 && armored.Experience()==300 && armored.Potions()==5 && armored.DefeatedCount()==3,"Practice victory must not grant loot, XP, gold or quest kills.");
  const float remainingHealth=armored.Health(); Interact(armored); Interact(armored);
  Require(!armored.Sparring() && armored.Health()==remainingHealth && armored.Experience()==300,"Restarting/stopping must not heal or duplicate rewards.");
  Require(armored.Load(Veteran(5)),"Low-health practice fixture."); Interact(armored); Wait(armored,20);
  Require(!armored.Sparring() && armored.Health()==1 && armored.SparringResult()=="Yielded","Practice must end nonlethally at one health.");
  Require(armored.Load(Veteran(90)),"Consumable practice fixture."); Interact(armored); c={}; c.usePotion=true; armored.Update(.01f,c); Interact(armored);
  Require(armored.Health()==130 && armored.Potions()==4 && armored.PotionCooldown()>0,"Practice potions are consumed normally and never refunded.");
  Require(armored.Load(Veteran()),"Timer practice fixture."); Interact(armored); Wait(armored,620);
  Require(!armored.Sparring() && armored.SparringResult()=="Time complete","Round must stop after thirty seconds.");
  Require(armored.Load(Veteran()),"Leash practice fixture."); Interact(armored);
  for (int i=0;i<150 && armored.Sparring();++i) { c={}; c.movement={-1,0}; c.sprint=true; armored.Update(.05f,c); }
  Require(!armored.Sparring() && armored.SparringResult()=="Left the arena","Leaving the training area must stop the round.");
  Require(armored.Load("MEDITERRANEAN 1\n12 -3.8 100 0 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n"),"New character at training post.");
  Interact(armored); Require(!armored.Sparring(),"Practice must unlock only after the campaign.");
  std::cout<<"PASS: repeatable nonlethal practice, armor effects, timers, no farming and transient saves.\n";
}
