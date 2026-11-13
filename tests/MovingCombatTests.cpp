#include "game/Game.h"
#include "game/SpellTrajectory.h"
#include <iostream>
#include <stdexcept>
#include <array>
using namespace rpg;
using DirectX::XMFLOAT3;
namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
float Distance(XMFLOAT3 a,XMFLOAT3 b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
void History(){
 for(auto school:{ClassId::Mage,ClassId::Scout,ClassId::Priest})for(int ability:{0,9}){
  CombatVisualEvent event;event.kind=CombatVisualKind::SpellCast;event.school=school;event.ability=ability;
  event.target=100;event.destination={1,7};event.destinationHeight=1.25f;event.flight=.37f;
  const XMFLOAT3 origin{0,1,0},end=spellvfx::ProjectilePosition(event,origin,1);
  std::array<XMFLOAT3,21> baseline{};
  for(int hz:{30,60,144,1000}){
   spellvfx::FlightPath path;path.Reset(event,origin);Check(path.Active(),"Flight did not initialize");
   float age=0;XMFLOAT3 retained{};bool sampled=false;
   while(age<event.flight){age=std::min(event.flight,age+1.f/hz);
    path.Advance(event,origin,age,{end.x+age*4,end.y+age*.3f,end.z-age*2});
    if(age>.15f){if(!sampled){retained=path.Point(.1f);sampled=true;}else Check(Distance(retained,path.Point(.1f))==0,"Steering rewrote the travelled wake");}
   }
   Check(Distance(path.Point(1),{end.x+age*4,end.y+age*.3f,end.z-age*2})<.0001f,"Moving target arrival missed");
   Check(path.Samples()<=spellvfx::FlightPath::capacity,"Flight history overflow");
   const auto head=path.Point(1);path.Advance(event,origin,age+1,{100,20,30});Check(Distance(head,path.Point(1))==0,"Completed flight followed later target motion");
   for(unsigned i=0;i<baseline.size();++i){const auto p=path.Point(i/20.f);if(hz==30)baseline[i]=p;else Check(Distance(p,baseline[i])<.002f,"Flight path depends on refresh rate");}
  }
  spellvfx::FlightPath invalid;event.flight=NAN;invalid.Reset(event,origin);Check(!invalid.Active(),"Invalid flight accepted");
 }
}
Simulation Hero(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Hero fixture");
 auto save=s.Save();const auto at=save.find("PROFILE ");save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Moving\" 8 1 3");
 Check(s.Load(save),"Class fixture");return s;
}
void ActualContacts(){
 for(auto school:{ClassId::Mage,ClassId::Scout,ClassId::Priest})for(int hz:{30,60,144})for(bool changeSelection:{false,true}){
  auto owner=std::make_unique<Game>();auto& game=*owner;game.Initialize();Check(game.RestoreSnapshot(Hero(school).Save()),"Restore fixture");
  auto& enemy=const_cast<Enemy&>(game.State().Enemies()[0]);enemy.kind=EnemyKind::Raider;enemy.health=enemy.MaxHealth();
  enemy.position=enemy.home={-4,11.5f};enemy.state=EnemyState::Pursuing;enemy.attackCooldown=10;
  Command cast;cast.selectActor=100;cast.ability=1;game.ExecuteCommand(cast,1.f/hz);
  bool release=false,contact=false;Vec2 releaseTarget{};
  for(int frame=0;frame<hz*3 && !contact;++frame){
   Command input;if(release && changeSelection)input.selectActor=101;
   game.ExecuteCommand(input,1.f/hz);
   for(std::size_t i=0;i<game.State().VisualEventCount();++i){const auto& e=game.State().VisualEvents()[i];
    if(e.kind==CombatVisualKind::SpellCast){release=true;releaseTarget=e.destination;Check(e.target==100,"Release lost committed target identity");}
    if(e.kind==CombatVisualKind::HeavyHit && e.primary){
     contact=true;Check(e.actor==100,"Selection redirected committed damage");
     const auto* path=game.Effects().ProjectilePath(100);Check(path,"Real release has no tracked path");
     const float error=Distance(path->Point(1),{e.position.x,e.height,e.position.z});
     Check(error<.002f,"Visible flight endpoint differs from confirmed damage contact");
     Check(std::hypot(e.position.x-releaseTarget.x,e.position.z-releaseTarget.z)>.04f,"Fixture did not move target during flight");
     std::cout<<"contact "<<int(school)<<" @ "<<hz<<" Hz, retarget="<<changeSelection<<", error="<<error
      <<", former endpoint miss="<<std::hypot(e.position.x-releaseTarget.x,e.position.z-releaseTarget.z)<<'\n';
    }
   }
  }
  Check(release && contact,"Moving combat did not release and hit");
  Check(game.RestoreSnapshot(Hero(school).Save()) && !game.Effects().ProjectilePath(100),"Load retained a committed projectile");
 }
}
void LostTarget(){
 auto owner=std::make_unique<Game>();auto& game=*owner;game.Initialize();Check(game.RestoreSnapshot(Hero(ClassId::Mage).Save()),"Lost fixture");
 auto& enemy=const_cast<Enemy&>(game.State().Enemies()[0]);enemy.position=enemy.home={-4,11.5f};enemy.health=enemy.MaxHealth();enemy.rooted=100;
 Command cast;cast.selectActor=100;cast.ability=1;game.ExecuteCommand(cast,.01f);
 for(int i=0;i<200 && !game.Effects().ProjectilePath(100);++i)game.ExecuteCommand({},.01f);
 const auto* path=game.Effects().ProjectilePath(100);Check(path,"Lost-target release");const auto destination=path->Target();
 enemy.health=0;enemy.state=EnemyState::Dead;
 bool inspected=false;
 for(int i=0;i<100;++i){Command select;select.selectActor=101;game.ExecuteCommand(select,.01f);
  for(std::size_t j=0;j<game.State().VisualEventCount();++j){const auto k=game.State().VisualEvents()[j].kind;Check(k!=CombatVisualKind::Hit && k!=CombatVisualKind::HeavyHit,"Lost target produced fabricated impact");}
  if(const auto* p=game.Effects().ProjectilePath(100)){Check(Distance(destination,p->Target())==0,"Dead projectile acquired replacement target");inspected=true;}
 }
 Check(inspected && !game.Effects().ProjectilePath(100),"Lost flight did not drain");
}
}
int main(){try{Check(sizeof(CombatEffects)<=16384,"Exceeded existing 16 KiB effects state budget");History();ActualContacts();LostTarget();std::cout<<"PASS: moving arrivals, immutable wakes, refresh-rate agreement, target selection/death/load isolation. Effect state bytes: "<<sizeof(CombatEffects)<<'\n';}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
