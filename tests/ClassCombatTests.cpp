#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>

namespace {
using namespace rpg;
void Check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
void Tick(Simulation& s,float seconds,int hz=100,std::vector<CombatVisualEvent>* events=nullptr) {
  while(seconds>1e-6f) {
    const float dt=std::min(seconds,1.f/hz);s.Update(dt,{});seconds-=dt;
    if(events) for(std::size_t n=0;n<s.VisualEventCount();++n) events->push_back(s.VisualEvents()[n]);
  }
}
void Profile(Simulation& s,ClassId id) {
  auto save=s.Save();const auto p=save.find("PROFILE ");
  save.replace(p,save.find('\n',p)-p,"PROFILE "+std::to_string(int(id))+" \"Tester\" 0 0 0");
  Check(s.Load(save),"Class profile fixture");
}
void Duel(Simulation& s,ClassId id) {
  Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 0 0 1 0.65 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n"),"Duel fixture");
  Profile(s,id);Command c;c.nextTarget=true;s.Update(.001f,c);
}
void Cadence() {
  std::set<int> contactTimes;
  for(int id=0;id<int(ClassId::Count);++id) for(int hz:{30,60,144,1000}) for(int ability:{0,1}) {
    Simulation s;Duel(s,ClassId(id));Command c;c.ability=ability;s.Update(.001f,c);
    Check(s.Attacking(),"Accepted class attack");
    const float contact=s.Spell(ability).contact;
    std::vector<CombatVisualEvent> events;
    Tick(s,contact-.002f,hz,&events);Check(s.Enemies()[0].health==80,"Class attack landed early");
    Tick(s,.004f,hz,&events);Check(s.Enemies()[0].health<80,"Class attack missed its contact clock");
    int releases=0,hits=0;
    for(const auto& event:events) {
      if(event.kind==CombatVisualKind::SpellCast || event.kind==CombatVisualKind::Strike || event.kind==CombatVisualKind::HeavyStrike) ++releases;
      else if(event.kind==CombatVisualKind::Hit || event.kind==CombatVisualKind::HeavyHit) ++hits;
      else continue;
      Check(event.school==ClassId(id) && event.heavy==(ability==1) && !event.empowered,"Action effect lost its class/weight snapshot");
      Check(event.source.x==s.Player().x && event.source.z==s.Player().z,"Action effect lost caster position");
    }
    Check(releases==1 && hits==1,"Class attack emitted duplicate or missing release/contact");
    Check(s.ClassResource()==(ability==0?1:0),"Only landed basics generate a reserve");
    contactTimes.insert(int(contact*1000));
    Tick(s,s.Spell(ability).recovery,hz);Check(!s.Attacking(),"Class recovery did not finish");
  }
  Check(contactTimes.size()>=10,"Class cadence collapsed to a shared clock");
}
void Mobility() {
  for(int id=0;id<int(ClassId::Count);++id) for(int hz:{30,60,144,1000}) {
    Simulation s;Check(s.CreateCharacter(ClassId(id),"Tester"),"Mobility hero");
    const auto origin=s.Player();Command c;c.ability=2;
    if(!s.RetreatMobility()) {
      s.Update(.01f,c);Check(!s.Dashing() && s.Cooldown(2)==0,"Melee charge lost target gating");continue;
    }
    const float heading=s.Facing();int starts=0,stops=0;float elapsed=0;
    while(elapsed<s.MobilityDuration()+.02f) {
      const auto before=s.Player();const float dt=1.f/hz;s.Update(dt,elapsed==0?c:Command{});elapsed+=dt;
      Check(s.CanStand(s.Player()),"Retreat penetrated collision");
      Check(std::hypot(before.x-s.Player().x,before.z-s.Player().z)<1.1f,"Retreat teleported");
      for(std::size_t n=0;n<s.VisualEventCount();++n) {
        starts+=s.VisualEvents()[n].kind==CombatVisualKind::DashStart;
        stops+=s.VisualEvents()[n].kind==CombatVisualKind::DashStop;
      }
    }
    Check(starts==1 && stops==1 && !s.Dashing(),"Retreat lifecycle");
    Check(std::abs(s.Facing()-heading)<.00001f && s.Player().z<origin.z,"Retreat turned its back to the threat");
    Check(std::abs(std::hypot(s.Player().x-origin.x,s.Player().z-origin.z)-s.CombatStyleInfo().retreatDistance)<.001f,"Retreat distance depends on refresh rate");
    Simulation loaded;Check(loaded.Load(s.Save()) && !loaded.Dashing(),"Retreat survived save/load");
    Check(s.CreateCharacter(ClassId(id),"Tester"),"Reset escape");s.Update(.01f,c);s.StopMovement();
    const auto stopped=s.Player();Tick(s,.2f,hz);
    Check(s.Player().x==stopped.x && s.Player().z==stopped.z && s.Cooldown(2)>0,"UI cancellation refunded or replayed escape");
  }
  // The southern island boundary must stop, rather than teleport or slide through.
  Simulation wall;Duel(wall,ClassId::Mage);auto save=wall.Save();
  const auto p=save.find('\n',save.find("PROFILE "))+1;
  save.replace(p,save.find('\n',p)-p,"0 "+std::to_string(-WorldMap::halfDepth+1.5f)+" 50 0 0 0");
  Check(wall.Load(save),"Boundary fixture");Command escape;escape.ability=2;wall.Update(.01f,escape);
  Tick(wall,.5f);Check(wall.CanStand(wall.Player()) && wall.Player().z>=-WorldMap::halfDepth+.399f,"Escape crossed island boundary");
}
void Preparation() {
  for(auto id:{ClassId::Scout,ClassId::Mage,ClassId::Engineer,ClassId::Priest,ClassId::Necromancer,ClassId::Psionic}) {
    Simulation s;Duel(s,id);Command heavy;heavy.ability=1;s.Update(.001f,heavy);
    Command move;move.movement={1,0};s.Update(.01f,move);
    Check(!s.Attacking() && s.Cooldown(1)>0 && s.Enemies()[0].health==80,"Moving preparation was not cancelled");
    Duel(s,id);s.Update(.001f,heavy);const float release=combat::StrokeStart(true)/ClassAttackRate(id,true);
    Tick(s,release+.002f);s.Update(.001f,move);
    Check(s.Attacking(),"Movement recalled a released projectile");Tick(s,s.Spell(1).contact);
    Check(s.Enemies()[0].health<80,"Released projectile lost damage");
    Duel(s,id);s.Update(.05f,move);heavy.movement={1,0};s.Update(.01f,heavy);
    Check(!s.Attacking() && s.Cooldown(1)==0,"Moving heavy spent cooldown on rejection");
    Duel(s,id);Command basic;basic.movement={1,0};basic.ability=0;s.Update(.01f,basic);
    Check(s.Attacking(),"Mobile basic attack was locked by heavy preparation rules");
  }
}
void Reserves() {
  for(int hz:{30,60,144,1000}) for(int i=0;i<int(ClassId::Count);++i) {
    const auto id=ClassId(i);Simulation s;
    Check(s.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
      "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Reserve training fixture");
    Profile(s,id);Command equip;equip.inventory={InventoryAction::Equip,s.Items().Find(Item::Starter),int(GearSlot::Weapon),Item::Starter};s.Update(.001f,equip);
    Command train;train.interact=true;s.Update(.001f,train);Check(s.Sparring(),"Reserve trainer");
    for(int hit=0;hit<s.CombatStyleInfo().capacity;++hit) {
      Command basic;basic.ability=0;s.Update(.001f,basic);Tick(s,std::max(1.f,s.Spell(0).recovery+.01f),hz);
      Check(s.ClassResource()==hit+1,"Landed basic did not build resource exactly once");
    }
    Check(s.EmpoweredReady(),"Full reserve not advertised");
    Simulation loaded;Check(loaded.Load(s.Save()) && loaded.ClassResource()==0 && loaded.Barrier()==0,"Transient reserves duplicated across load");
    auto cancelled=s;Command cancelHeavy;cancelHeavy.ability=1;cancelled.Update(.001f,cancelHeavy);cancelled.StopMovement();
    Check(cancelled.ClassResource()==0 && cancelled.Cooldown(1)>0,"Cancellation refunded a spent reserve");
    const float victimBefore=s.SparringOpponent().health;
    Command heavy;heavy.ability=1;s.Update(.001f,heavy);
    Check(s.EmpoweredAttack() && s.ClassResource()==0,"Heavy did not commit/spend full reserve");
    std::vector<CombatVisualEvent> empoweredEvents;
    Tick(s,s.Spell(1).contact+.001f,hz,&empoweredEvents);
    bool empoweredContact=false;
    for(const auto& event:empoweredEvents) if(event.kind==CombatVisualKind::HeavyHit) {
      Check(event.empowered && event.heavy && event.school==id,"Payoff effect lost committed reserve");empoweredContact=true;
    }
    Check(empoweredContact,"Missing payoff contact event");
    const auto& target=s.SparringOpponent();Check(target.health<victimBefore,"Empowered heavy did not land");
    if(id==ClassId::Paladin) Check(s.Barrier()>0,"Paladin barrier missing");
    if(id==ClassId::Priest) Check(s.Barrier()>0 && s.Health()>40,"Priest healing/ward missing");
    if(id==ClassId::Scout) Check(target.snared>2.9f,"Scout snare missing");
    if(id==ClassId::Druid) Check(target.rooted>1.9f,"Druid root missing");
    if(id==ClassId::Psionic) Check(target.stunned>1.9f && target.windup==0,"Psionic stun missing");
    if(id==ClassId::Engineer) Check(target.exposed>4.9f,"Engineer exposure missing");
    if(id==ClassId::Mage) Check(victimBefore-target.health>=std::min(victimBefore,s.HeavyDamage()+35),"Mage empowered burst missing");
    if(id==ClassId::Warrior) Check(victimBefore-target.health>=std::min(victimBefore,s.HeavyDamage()+23.9f),"Warrior empowered damage missing");
    if(id==ClassId::Bard) Check(s.Cooldown(0)==0 && s.Health()>40,"Bard cadence/healing missing");
    if(id==ClassId::Necromancer) Check(s.Health()>65,"Necromancer empowered drain missing");
    if(id==ClassId::Demonologist) Check(s.Health()>35,"Demonologist empowered recovery missing");
    if(id==ClassId::Paladin || id==ClassId::Priest) {
      s.StopMovement();const float stronger=s.Barrier();Command heal;heal.ability=3;s.Update(.001f,heal);
      Check(s.Cooldown(3)>0 && s.Barrier()==stronger,"Recovery stacked or weakened an existing empowered ward");
      const float ward=s.Barrier(),health=s.Health();
      for(int tick=0;tick<hz*4 && s.Barrier()==ward;++tick) s.Update(1.f/hz,{});
      Check(s.Barrier()<ward && s.Health()>=health && !s.DamagedThisUpdate(),"Barrier did not absorb a real enemy hit");
      int wardHits=0;for(std::size_t n=0;n<s.VisualEventCount();++n) wardHits+=s.VisualEvents()[n].kind==CombatVisualKind::BarrierHit;
      Check(wardHits==1,"Absorbed hit must produce exactly one barrier ripple");
      bool absorb=false;for(const auto& f:s.Feedback()) absorb|=f.kind==FeedbackKind::Absorb;
      Check(absorb,"Absorption feedback missing");
    }
    if(id==ClassId::Psionic) {
      const auto position=target.position;const float health=s.Health();Tick(s,1.5f,hz);
      Check(target.position.x==position.x && target.position.z==position.z && s.Health()==health && target.windup==0,"Stun did not stop movement and attacks");
      Tick(s,.6f,hz);Check(target.stunned==0,"Stun outlived its duration");
    }
    if(id==ClassId::Druid) {
      const auto position=target.position;Command step;step.movement={1,0};
      for(int n=0;n<hz/2;++n) s.Update(1.f/hz,step);
      Check(target.position.x==position.x && target.position.z==position.z,"Root did not prevent pursuit");
      Tick(s,1.6f,hz);Check(target.rooted==0,"Root outlived its duration");
    }
    if(id==ClassId::Scout) {Tick(s,3.1f,hz);Check(target.snared==0,"Snare outlived its duration");}
    if(id==ClassId::Engineer) {
      Tick(s,s.Spell(1).recovery,hz);const float hp=target.health;
      Command basic;basic.ability=0;s.Update(.001f,basic);Tick(s,s.Spell(0).contact+.001f,hz);
      Check(std::abs(hp-target.health-std::min(hp,s.StrikeDamage()*1.25f))<.001f,"Exposure did not amplify actual damage");
    }
    Tick(s,.1f);Check(s.ClassResource()==(id==ClassId::Engineer?1:0),"Heavy incorrectly generated basic resource");
    Command restart;restart.restart=true;s.Update(.01f,restart);
    Check(s.ClassResource()==0 && s.Barrier()==0 && !s.Attacking(),"Reset kept combat reserves");
  }
  Simulation miss;Duel(miss,ClassId::Mage);Command attack;attack.ability=0;miss.Update(.001f,attack);
  miss.StopMovement();Tick(miss,.5f);Check(miss.ClassResource()==0,"Cancelled basic generated resource");
}
}
void Recovery() {
  for(int id=0;id<int(rpg::ClassId::Count);++id) for(int hz:{30,60,144}) {
    rpg::Simulation s;Duel(s,rpg::ClassId(id));
    auto save=s.Save();const auto line=save.find('\n',save.find('\n')+1)+1;
    save.replace(line,save.find('\n',line)-line,"0 -10 50 0 0 0");
    Check(s.Load(save),"Safe recovery fixture");
    const float initial=s.Health(),cast=ClassRecoveryCast(s.Class());
    const float expected=ClassHealer(s.Class())?std::min(s.MaxHealth(),initial+s.MendHealingNow()):initial-ClassRecoveryCost(s.Class());
    rpg::Command heal;heal.ability=3;s.Update(.001f,heal);
    if(cast>0) {
      Check(s.CastingRecovery() && s.Health()==initial,"Healing must wait for cast completion");
      Tick(s,cast-.01f,hz);Check(s.CastingRecovery() && s.Health()==initial,"Cast completed early");
      Tick(s,.02f,hz);
    }
    Check(std::abs(s.Health()-expected)<.001f && s.Barrier()==ClassRecoveryBarrier(s.Class()),"Class recovery health/ward contract");
    Check(s.Load(save),"Interruption fixture");
    s.Update(.001f,heal);const float before=s.Health();
    Command move;move.movement={1,0};s.Update(.01f,move);Tick(s,cast+.1f,hz);
    if(cast>0) Check(!s.CastingRecovery() && s.Health()==before && s.Barrier()==0,"Movement must cancel unfinished recovery");
    Check(s.Load(save),"Menu interruption fixture");s.Update(.001f,heal);
    Command menu;menu.haltMovement=true;s.Update(.01f,menu);Tick(s,cast+.1f,hz);
    if(cast>0) Check(!s.CastingRecovery() && s.Health()==initial,"Escape cannot complete a pending heal");
    Check(s.Load(s.Save()) && !s.CastingRecovery() && s.Barrier()==0,"Transient recovery state survived load");
  }
}
void DefensiveIdentity() {
  {
    Simulation s;Duel(s,ClassId::Priest);
    auto& attacker=const_cast<Enemy&>(s.Enemies()[0]);attacker.windup=.001f;
    Command heal;heal.ability=3;s.Update(.01f,heal);
    Check(s.DamagedThisUpdate() && !s.CastingRecovery(),"Damage on the starting frame must interrupt healing");
  }
  for(int hz:{30,60,144}) for(auto id:{ClassId::Warrior,ClassId::Scout,ClassId::Mage,ClassId::Priest}) {
    Simulation s;Duel(s,id);const float health=s.Health();Command guard;guard.ability=3;s.Update(.001f,guard);
    Tick(s,.7f,hz);
    if(id==ClassId::Priest) Check(!s.CastingRecovery() && s.Health()<health,"Enemy damage must interrupt healing before completion");
    else Check(s.Health()==health,"Class defense must prevent the first attack");
    if(id==ClassId::Warrior) Check(s.ClassResource()==1,"Blocking must generate Rage");
    if(id==ClassId::Mage) Check(s.Enemies()[0].snared>1.8f,"Frost ward must slow the attacker");
    if(id==ClassId::Scout) {
      Check(s.EvasionSeconds()==0 && s.Barrier()==0,"Scout evasion is consumed by one hit, not a barrier");
      Tick(s,2.5f,hz);Check(s.Health()<health,"Evasion cannot suppress later hits");
    }
  }
}
int main() {Cadence();Mobility();Preparation();Reserves();Recovery();DefensiveIdentity();std::cout<<"PASS: class clocks, retreat collision, casting movement, empowered combat and recovery wards.\n";}
