#include "game/Simulation.h"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
using namespace rpg;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
void Tick(Simulation& s,float duration,int hz,std::vector<CombatVisualEvent>* events=nullptr){while(duration>1e-6f){const float dt=std::min(duration,1.f/hz);s.Update(dt,{});duration-=dt;
 if(events)for(size_t i=0;i<s.VisualEventCount();++i)events->push_back(s.VisualEvents()[i]);}}
Simulation Hero(bool actors=true){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Practice fixture");
 auto save=s.Save();const auto at=save.find("PROFILE ");save.replace(at,save.find('\n',at)-at,"PROFILE 3 \"Tester\" 7 0 3");
 if(!actors){const auto player=save.find('\n',save.find("PROFILE "))+1;save.replace(player,save.find('\n',player)-player,"12 -3.8 50 0 3 90");}
 Check(s.Load(save) && s.Class()==ClassId::Priest && s.Level()==10,"Priest veteran");
 if(!actors){Command c;c.selectActor=210;s.Update(.001f,c);Check(s.TargetActor() && s.TargetActor()->id==210,"Persistent training target");return s;}
 for(int i=0;i<3;++i){auto& e=const_cast<Enemy&>(s.Enemies()[i]);e.kind=EnemyKind::Raider;e.rank=20;e.health=e.MaxHealth();e.looted=false;
  e.position=e.home={-4.f+i*2.f,7};e.state=EnemyState::Idle;e.windup=0;e.rooted=100;e.attackCooldown=100;}
 Command c;c.selectActor=100;s.Update(.001f,c);Check(s.TargetActor() && s.TargetActor()->id==100,"Select priest target");return s;
}
void Start(Simulation& s,int ability){Command c;c.ability=ability;s.Update(.001f,c);Check(s.Attacking() && s.AttackAbility()==ability,"Priest action rejected");}
void Cast(Simulation& s,int ability,int hz){Start(s,ability);Tick(s,s.Spell(ability).recovery+.01f,hz);Check(!s.Attacking(),"Priest recovery did not finish");}
void Charge(Simulation& s,int hz){for(int i=0;i<8 && !s.EmpoweredReady();++i){if(s.Cooldown(0)>0)Tick(s,s.Cooldown(0)+.01f,hz);Cast(s,0,hz);}Check(s.EmpoweredReady(),"Faith generation");}
void Area(int hz){
 auto s=Hero();const auto hp=s.Health();const float before=s.Enemies()[0].health,near=s.Enemies()[1].health,far=s.Enemies()[2].health;
 Start(s,8);Tick(s,s.Spell(8).contact-.002f,hz);
 Check(s.Enemies()[0].health==before && s.Barrier()==0,"Consecration rewarded before contact");
 std::vector<CombatVisualEvent> contacts;Tick(s,.004f,hz,&contacts);
 Check(std::abs(before-s.Enemies()[0].health-s.HeavyDamage()*.65f)<.01f,"Consecration primary damage");
 Check(s.Enemies()[1].health<near && s.Enemies()[2].health==far,"Consecration area boundary");
 Check(s.Enemies()[0].exposed>2.9f && s.Enemies()[1].exposed>2.9f && s.Enemies()[2].exposed==0,"Consecration exposure footprint");
 Check(s.Barrier()==18 && s.Health()==hp,"Consecration duplicated or invented recovery");
 int primary=0,secondary=0;for(const auto& e:contacts)if(e.kind==CombatVisualKind::HeavyHit){primary+=e.primary;secondary+=!e.primary;}
 Check(primary==1 && secondary==1,"Area spectacle needs one primary owner");
 auto ascension=Hero();Charge(ascension,hz);const float life=ascension.Health(),reserve=ascension.ClassResource();
 auto missed=ascension;Start(missed,11);const_cast<Enemy&>(missed.Enemies()[0]).position.x+=30;Tick(missed,4,hz);
 Check(missed.Health()==life && missed.Barrier()==0 && missed.ClassResource()==0,"Missed ascension granted recovery or refunded Faith");
 auto cancelled=ascension;Start(cancelled,11);cancelled.StopMovement();Tick(cancelled,4,hz);
 Check(cancelled.Health()==life && cancelled.Barrier()==0 && cancelled.ClassResource()==0 && cancelled.Cooldown(11)>0,"Cancelled ascension replayed rewards");
 const auto nearBefore=ascension.Enemies()[1].health,farBefore=ascension.Enemies()[2].health;
 Start(ascension,11);Check(reserve>0 && ascension.ClassResource()==0,"Ascension spends reserve on commitment");
 Tick(ascension,ascension.Spell(11).contact+.01f,hz);
 Check(std::abs(ascension.Health()-life-30)<.01f && ascension.Barrier()==24,"Area victims duplicated healing or ward");
 Check(ascension.Enemies()[1].health<nearBefore && ascension.Enemies()[2].health==farBefore,"Ascension area boundary");
 // The area fixture intentionally relocates/raises enemy ranks. Save checks
 // use an unmodified campaign and its immortal practice target instead.
 auto persistent=Hero(false);Charge(persistent,hz);Start(persistent,11);Simulation restored;
 Check(restored.Load(persistent.Save()) && restored.Cooldown(11)>0 && !restored.Attacking(),"Load replays ascension or loses cooldown");
 const float restoredHealth=restored.Health();Tick(restored,4,hz);Check(restored.Health()==restoredHealth && restored.Barrier()==0,"Loaded cast granted a delayed reward");
}
void Judgment(int hz){
 auto s=Hero();auto& enemy=const_cast<Enemy&>(s.Enemies()[0]);enemy.exposed=10;
 const float before=enemy.health,life=s.Health();Cast(s,9,hz);
 Check(enemy.exposed==0 && std::abs(before-enemy.health-s.HeavyDamage()*1.60f*1.25f)<.01f,"Judgment did not consume exposure for damage");
 Check(std::abs(s.Health()-life-16)<.01f,"Judgment recovery");
 auto plain=Hero();const float unmarked=plain.Enemies()[0].health;Cast(plain,9,hz);
 Check(std::abs(unmarked-plain.Enemies()[0].health-plain.HeavyDamage()*1.25f)<.01f,"Unmarked judgment invented setup bonus");
 auto miss=Hero();const float missedHealth=miss.Health();Start(miss,9);const_cast<Enemy&>(miss.Enemies()[0]).position.x+=30;Tick(miss,3,hz);
 Check(miss.Health()==missedHealth && miss.ClassResource()==0,"Missed judgment rewarded");
}
void Bell(int hz){
 for(bool protectedCast:{false,true}){auto s=Hero();auto& enemy=const_cast<Enemy&>(s.Enemies()[0]);
  enemy.kind=protectedCast?EnemyKind::Captain:EnemyKind::Wraith;enemy.bossAttack=1;enemy.health=enemy.MaxHealth();enemy.windup=2;
  Check(enemy.Interruptible()!=protectedCast,"Interrupt fixture");Start(s,10);Tick(s,s.Spell(10).contact+.01f,hz);
  Check((enemy.windup>0)==protectedCast && s.ClassResource()==1,"Bell interrupted a protected cast or lost Faith");
  Check(s.Barrier()==(protectedCast?0:24),"Bell ward must require an actual interrupt");
 }
}
void Chains(int hz){
 for(int links:{0,2,3}){auto s=Hero();if(links){Cast(s,0,hz);Cast(s,12,hz);}if(links==3)Cast(s,13,hz);
  const float life=s.Health();Start(s,14);Tick(s,s.Spell(14).contact-.002f,hz);Check(s.Health()==life,"Verdict healed before contact");
  Tick(s,.004f,hz);Check(std::abs(s.Health()-life-8*links)<.01f && s.Barrier()==8*links && s.ChainSteps()==0,"Verdict chain reward/consumption");
 }
 auto benediction=Hero();Command recover;recover.ability=3;benediction.Update(.001f,recover);Tick(benediction,4,hz);
 Check(benediction.DisciplineSeconds()>0,"Benediction fixture");Cast(benediction,12,hz);
 Check(benediction.DisciplineSeconds()>0,"Votive spark consumed reserved Benediction");Cast(benediction,0,hz);
 Check(benediction.DisciplineSeconds()==0,"Radiance stopped consuming Benediction");
}
void Gates(){
 Simulation fresh;Check(fresh.CreateCharacter(ClassId::Priest,"Tester"),"Fresh priest");
 std::set<std::string> names;for(int spell=8;spell<=14;++spell){names.insert(fresh.Spell(spell).name);
  Check(std::string(fresh.AbilityStatus(spell))!="DISABLED","Priest advanced spell disabled");
  if(spell!=12)Check(std::string(fresh.AbilityStatus(spell)).find("REQUIRES LEVEL")==0,"Priest level gate");}
 Check(names.size()==7,"Priest disciplines share names");
 for(int spell:{8,9,10,11,12,13,14}){auto s=Hero();if(spell==11)Charge(s,60);Start(s,spell);
  Command move;move.movement={.2f,0};s.Update(.03f,move);
  Check(s.Attacking()==(spell==10 || spell>=12),"Priest stationary/mobile rule");}
}
}
int main(){try{Gates();for(int hz:{30,60,144,1000}){Area(hz);Judgment(hz);Bell(hz);Chains(hz);}
 std::cout<<"PASS: seven Priest disciplines, contact-only recovery, area ownership, real interrupts, Faith, chains, movement and load at 30/60/144/1000 Hz.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
