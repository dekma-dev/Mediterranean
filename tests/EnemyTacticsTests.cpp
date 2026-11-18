#include "game/EnemyTactics.h"
#include <cstdlib>
#include <iostream>

namespace {
void Require(bool ok,const char* message) {
  if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
float Distance(rpg::Vec2 a,rpg::Vec2 b){return std::hypot(a.x-b.x,a.z-b.z);}
const char* fixture="MEDITERRANEAN 1\n-4 5.2 100 3.14159 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n";
void Arrange(rpg::Simulation& sim) {
  Require(sim.Load(fixture),"Fixture must load");
  auto& actors=const_cast<std::vector<rpg::Enemy>&>(sim.Enemies());
  for(int i=0;i<3;++i){auto& e=actors[i];e.kind=rpg::EnemyKind::Raider;
    e.position=e.home={-4.f+(i-1)*1.0f,6.8f};e.state=rpg::EnemyState::Pursuing;
    e.attackCooldown=2;e.windup=0;e.facing=3.14159265f;}
}
}
int main(){
  rpg::Simulation base;Arrange(base);
  auto actors=base.Enemies();const auto player=base.Player();
  const auto open=[](rpg::Vec2){return true;};
  const auto original=actors[0].position;
  const auto goal=rpg::MeleeRecoveryGoal(actors[0],player,actors,open);
  Require(Distance(goal,original)>.3f,"Crowded melee recovery must seek a new angle");
  Require(Distance(goal,player)>=1.65f-.001f && Distance(goal,player)<=1.85f+.001f,"Flank must stay at melee distance");
  Require(Distance(rpg::MeleeRecoveryGoal(actors[0],player,actors,[](rpg::Vec2){return false;}),original)==0,"Blocked world must veto repositioning");
  for(auto kind:{rpg::EnemyKind::Dummy,rpg::EnemyKind::Trainer,rpg::EnemyKind::Wolf,rpg::EnemyKind::Boar,rpg::EnemyKind::Wraith}) {
    auto e=actors[0];e.kind=kind;
    Require(Distance(rpg::MeleeRecoveryGoal(e,player,actors,open),e.position)==0,"Special roles retain their existing tactics");
  }
  for(int lock=0;lock<7;++lock){auto e=actors[0];
    if(lock==0)e.rooted=1;if(lock==1)e.stunned=1;if(lock==2)e.windup=1;
    if(lock==3)e.attackCooldown=0;if(lock==4)e.state=rpg::EnemyState::Returning;
    if(lock==5)e.health=0;if(lock==6){e.kind=rpg::EnemyKind::Captain;e.rank=1;}
    Require(Distance(rpg::MeleeRecoveryGoal(e,player,actors,open),e.position)==0,"Locked/committed actors must never flank");
  }
  auto solo=actors;solo[1].health=solo[2].health=0;
  Require(Distance(rpg::MeleeRecoveryGoal(solo[0],player,solo,open),original)==0,"Corpses must not create tactical pressure");
  for(int lock=0;lock<3;++lock) {
    rpg::Simulation sim;Arrange(sim);auto& pinned=const_cast<rpg::Enemy&>(sim.Enemies()[0]);
    if(lock==0)pinned.rooted=2;if(lock==1)pinned.stunned=2;
    if(lock==2){pinned.windup=2;pinned.attackCooldown=0;}
    const auto p=pinned.position;const float facing=pinned.facing;
    for(int n=0;n<60;++n)sim.Update(1.f/60,{});
    Require(Distance(p,pinned.position)<.00001f,"Formation displaced a controlled or committed ally");
    if(lock==2)Require(pinned.facing==facing,"Formation rotated a committed attack warning");
  }
  {
    rpg::Simulation normal,slow;Arrange(normal);Arrange(slow);
    auto& slowed=const_cast<rpg::Enemy&>(slow.Enemies()[0]);slowed.snared=2;
    float normalTravel=0,slowTravel=0;
    for(int n=0;n<30;++n){normal.Update(1.f/60,{});slow.Update(1.f/60,{});
      normalTravel+=normal.Enemies()[0].movementDistance;slowTravel+=slow.Enemies()[0].movementDistance;}
    Require(slowTravel<normalTravel*.75f,"Formation must respect the actual snare movement penalty");
  }
  std::array<rpg::Vec2,3> baseline{};
  for(int hz:{30,60,144,1000}) {
    rpg::Simulation sim;Arrange(sim);const auto initial=sim.Enemies();
    for(int frame=0;frame<hz;++frame){const auto previous=sim.Enemies();sim.Update(1.f/hz,{});
      for(int i=0;i<3;++i){const auto& e=sim.Enemies()[i];
        Require(sim.Map().CanTravel(previous[i].position,e.position,.45f),"Formation movement crossed scenery");
        Require(Distance(previous[i].position,e.position)<=2.21f/hz+.001f,"Formation movement teleported");
        Require(Distance(e.position,sim.Player())>1.2f,"Formation movement crossed the player");
        for(int j=0;j<i;++j)Require(Distance(e.position,sim.Enemies()[j].position)>=.899f,"Formation actors overlapped");
      }
    }
    Require(Distance(sim.Enemies()[0].position,sim.Enemies()[2].position)>Distance(initial[0].position,initial[2].position)+.25f,"Group did not visibly open its front");
    for(int i=0;i<3;++i){if(hz==30)baseline[i]=sim.Enemies()[i].position;
      else Require(Distance(baseline[i],sim.Enemies()[i].position)<.18f,"Recovery formation diverged across refresh rates");}
    const auto saved=sim.Save();rpg::Simulation restored;Require(restored.Load(saved),"Repositioned encounter must remain loadable");
    for(int i=0;i<3;++i)Require(Distance(restored.Enemies()[i].position,sim.Enemies()[i].position)<.001f,"Loading must preserve actual formation positions");
    bool cast=false,hit=false;
    for(int frame=0;frame<hz*4;++frame){sim.Update(1.f/hz,{});for(const auto& e:sim.Enemies())cast|=e.windup>0;hit|=sim.Health()<100;}
    Require(cast&&hit,"Repositioning must yield to actual telegraphed attacks");
    std::cout<<hz<<" Hz: formation, physical clearance, persistence and attacks passed\n";
  }
}
