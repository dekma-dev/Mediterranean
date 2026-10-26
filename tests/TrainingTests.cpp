#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>
#include <cmath>

void Require(bool value,const char* message) { if (!value) { std::cerr<<message<<'\n'; std::exit(1); } }
std::string Veteran(int health=140) {
  return "MEDITERRANEAN 3\n12 -3.8 "+std::to_string(health)+" 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
}
void Interact(rpg::Simulation& s) { rpg::Command c; c.interact=true; s.Update(.01f,c); }
void Wait(rpg::Simulation& s,int frames) { for (int i=0;i<frames;++i) s.Update(.05f,{}); }
void FirstHit(rpg::Simulation& s) {
  const float health=s.Health();
  for (int i=0;i<40 && s.Health()==health;++i) s.Update(.05f,{});
  Require(s.Health()<health,"Training opponent must turn, telegraph and hit within two seconds.");
}
int main() {
  {
    rpg::Simulation friendly; rpg::Command select; select.selectActor=303;
    friendly.Update(.01f,select);
    Require(friendly.FriendlyTarget()==3 && !friendly.TargetActor(),"Keeper must be a friendly, non-combat selection");
    rpg::Command attack; attack.ability=0;friendly.Update(.01f,attack);
    Require(!friendly.Attacking() && friendly.Cooldown(0)==0,"Friendly targeting must not spend an attack");
    select.selectActor=999;friendly.Update(.01f,select);
    Require(friendly.FriendlyTarget()==3,"Unknown identities must not retarget");
  }
  for(int hz:{30,60,144}) {
    rpg::Simulation practice;Require(practice.Load(Veteran()),"Dummy fixture loads");
    rpg::Command select;select.selectActor=210;practice.Update(1.f/hz,select);
    Require(practice.TargetActor() && practice.TargetActor()->kind==rpg::EnemyKind::Dummy,"Scenery dummy must be selectable without starting sparring");
    const auto hp=practice.Health();const auto gold=practice.Gold(),xp=practice.Experience();
    rpg::Command hit;hit.ability=0;practice.Update(1.f/hz,hit);
    bool damaged=false;
    for(int n=0;n<hz*2;++n) {practice.Update(1.f/hz,{});if(practice.Dummies()[0].health<200) {damaged=true;break;}}
    Require(damaged,"Practice attack must visibly reduce dummy HP");
    const float before=practice.Dummies()[0].health;practice.Update(1.f/hz,{});
    Require(std::abs(practice.Dummies()[0].health-std::min(200.f,before+200.f/hz))<.002f,"Dummy regeneration must be exactly 200 HP per simulated second");
    for(int n=0;n<hz;++n) practice.Update(1.f/hz,{});
    Require(practice.Dummies()[0].health==200 && practice.Health()==hp,"Dummy must fully recover without retaliating");
    Require(practice.Gold()==gold && practice.Experience()==xp && !practice.Sparring(),"Dummy practice grants no progression and does not start sparring");
    auto& dummy=const_cast<rpg::Enemy&>(practice.Dummies()[0]);dummy.health=1;
    hit.ability=1;practice.Update(1.f/hz,hit);
    bool lethalContact=false;
    for(int n=0;n<hz*2;++n) {
      dummy.health=1; practice.Update(1.f/hz,{});
      if(dummy.damagedThisUpdate) {lethalContact=true;break;}
    }
    Require(lethalContact,"A normally lethal practice hit must actually land");
    Require(dummy.health>0 && dummy.state==rpg::EnemyState::Idle && !dummy.looted,"Dummy stays alive and never becomes loot");
    rpg::Simulation reload;Require(reload.Load(practice.Save()) && !reload.TargetActor() && reload.Dummies()[0].health==200,"Practice state is transient across load");
  }
  using namespace rpg;
  {
    Simulation dash; Require(dash.Load(Veteran()),"Dash training fixture must load");
    Command input; input.ability=2; input.interact=true; dash.Update(.01f,input);
    Require(!dash.Dashing() && dash.Cooldown(2)==0 && dash.Sparring(),
      "A dash with no target must not consume cooldown or prevent starting training");
    input={}; input.ability=2; dash.Update(.01f,input);
    Require(dash.Dashing(),"The selected sparring opponent must accept a targeted dash");
    input={}; input.interact=true; dash.Update(.01f,input);
    Require(!dash.Sparring() && !dash.Dashing() && dash.Cooldown(2)>0,
      "Ending practice must cancel travel toward the disappearing opponent without refunding cooldown");
  }
  {
    Simulation rebound; Require(rebound.Load(Veteran()),"Rebound training fixture must load");
    for(int i=0;i<11;++i) { Command jump; jump.jump=i==0 || i==10; rebound.Update(.05f,jump); }
    const float speed=rebound.VerticalSpeed();
    const float contact=2*rebound.Height()/(std::sqrt(speed*speed+48*rebound.Height())-speed);
    Command use; use.interact=true; rebound.Update(contact,use);
    Require(rebound.Height()==0 && rebound.VerticalSpeed()>0,"Training fixture must end at rebound takeoff");
    Require(!rebound.Sparring(),"Rising at zero height must not permit a sparring start");
  }
  Simulation armored,plain;
  Require(armored.Load(Veteran()) && plain.Load(Veteran()),"Training fixtures must migrate.");
  Command c; c.inventory={InventoryAction::Unequip,int(GearSlot::Chest),23,Item::Ward}; plain.Update(.01f,c);
  Interact(armored); Interact(plain);
  Require(armored.Sparring() && armored.TargetActor() && armored.TargetActor()->kind==EnemyKind::Trainer,"Practice must select a dedicated opponent.");
  FirstHit(armored); FirstHit(plain);
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
  Require(armored.Load(Veteran(5)),"Low-health practice fixture."); Interact(armored); FirstHit(armored);
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
