#include "game/Game.h"
#include "game/CastEffects.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace {
using namespace rpg;
using K=CombatVisualKind;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Tick(Simulation& s,float duration,int hz=60,Command c={}){
 while(duration>1e-6f){const float dt=std::min(duration,1.f/hz);s.Update(dt,c);duration-=dt;}
}
Simulation Hero(ClassId id){
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
 "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Fixture");
 auto save=s.Save();const auto at=save.find("PROFILE ");
 save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Tester\" 7 1 3");
 Check(s.Load(save),"Profile");Command select;select.selectActor=210;s.Update(.001f,select);return s;
}
int Cast(Simulation& s,int ability,int hz=60,bool expectDiscipline=false){
 Command c;c.ability=ability;s.Update(.001f,c);
 if(!s.Attacking() || s.AttackAbility()!=ability)throw std::runtime_error("Attack rejected: class "+std::to_string(int(s.Class()))+" ability "+std::to_string(ability)+" / "+s.Message());
 int hits=0,flagged=0,damage=0;
 while(s.Attacking()){
  s.Update(1.f/hz,{});
  for(std::size_t n=0;n<s.VisualEventCount();++n){const auto& e=s.VisualEvents()[n];
   if(e.kind==K::Hit || e.kind==K::HeavyHit){++hits;flagged+=e.discipline;
    for(const auto& f:s.Feedback())if(f.kind==FeedbackKind::Damage && f.age==0)damage=f.amount;}
  }
 }
 Check(hits==1,"Expected one primary contact");Check(flagged==int(expectDiscipline),"Opportunity contact flag");return damage;
}
void Guard(Simulation& s,float wait,int hz,bool behind=false){
 Command guard;guard.ability=3;s.Update(.001f,guard);Check(s.Barrier()>0,"No guard");
 Tick(s,wait,hz);
 auto& enemy=const_cast<Enemy&>(s.Enemies()[0]);enemy={};enemy.id=100;
 enemy.position=enemy.home={s.Player().x,s.Player().z+(behind?-1.2f:1.2f)};
 enemy.facing=behind?0:3.14159265f;enemy.health=enemy.MaxHealth();enemy.state=EnemyState::Pursuing;enemy.windup=.06f;enemy.attackCooldown=100;
 Tick(s,.12f,hz);enemy.health=0;enemy.windup=0;enemy.state=EnemyState::Dead;
}
void Elements(Simulation& s,int hz){Cast(s,0,hz);Check(s.ElementMask()==1,"Fire not recorded");
 Cast(s,12,hz);Check(s.ElementMask()==3,"Frost not recorded");Cast(s,13,hz);
 Check(s.DisciplineSeconds()>0 && s.ElementMask()==0,"Three schools did not produce confluence");}
void Retreat(Simulation& s,int hz){Command c;c.ability=2;s.Update(.001f,c);Check(s.Dashing(),"Retreat not started");while(s.Dashing())s.Update(1.f/hz,{});}
void Heal(Simulation& s,int hz){const float before=s.Health();Command c;c.ability=3;s.Update(.001f,c);Check(s.CastingRecovery(),"Healing not accepted");while(s.CastingRecovery())s.Update(1.f/hz,{});Check(s.Health()>before && s.DisciplineSeconds()>0,"Healing did not ready benediction");}
void Rules(){
 for(int hz:{30,60,144}){
  auto warrior=Hero(ClassId::Warrior);Guard(warrior,.12f,hz);Check(warrior.DisciplineSeconds()>0,"Timely frontal absorption did not prepare riposte");
  auto plain=Hero(ClassId::Warrior);const int base=Cast(plain,0,hz),boosted=Cast(warrior,0,hz,true);
  Check(std::abs(float(boosted-base)-warrior.StrikeDamage()*.5f)<1.1f && warrior.Barrier()>=12 && warrior.DisciplineSeconds()==0,"Riposte damage/protection/spend");
  auto rear=Hero(ClassId::Warrior);Guard(rear,.12f,hz,true);Check(rear.DisciplineSeconds()==0,"Rear absorption awarded riposte");
  auto late=Hero(ClassId::Warrior);Guard(late,.85f,hz);Check(late.DisciplineSeconds()==0,"Late absorption awarded riposte");
  auto expiry=Hero(ClassId::Warrior);Guard(expiry,.12f,hz);Tick(expiry,5.1f,hz);Check(expiry.DisciplineSeconds()==0,"Riposte did not expire");

  auto mage=Hero(ClassId::Mage);Elements(mage,hz);
  const auto validSave=mage.Save();Simulation loaded;Check(loaded.Load(validSave) && loaded.DisciplineSeconds()==0 && loaded.ElementMask()==0,"Save banks transient confluence");
  auto& dummy=const_cast<Enemy&>(mage.Dummies()[0]);const auto origin=dummy.position;dummy.position.x+=30;
  Command spell;spell.ability=14;mage.Update(.001f,spell);Check(!mage.Attacking() && mage.DisciplineSeconds()>0,"Rejected cast consumed confluence");dummy.position=origin;
  Command moving;moving.movement={.12f,0};Tick(mage,.12f,hz,moving);moving.ability=14;mage.Update(.001f,moving);
  Check(mage.ConfluenceCast() && mage.DisciplineSeconds()==0 && !mage.SteadyAttack(14),"Confluence did not allow commitment while moving");
  moving.ability=-1;int contacts=0;
  while(mage.Attacking()){mage.Update(1.f/hz,moving);for(std::size_t n=0;n<mage.VisualEventCount();++n)if(mage.VisualEvents()[n].kind==K::HeavyHit){++contacts;Check(mage.VisualEvents()[n].discipline,"Mobile impact lost its accepted feature");}}
  Check(contacts==1 && mage.ElementMask()==0 && mage.SteadyAttack(14),"Mobile cast failed or restarted its own combination");
  auto cancelled=Hero(ClassId::Mage);Elements(cancelled,hz);spell.ability=14;cancelled.Update(.001f,spell);cancelled.StopMovement();
  Check(!cancelled.ConfluenceCast() && cancelled.DisciplineSeconds()==0,"Cancel refunded confluence");
  auto ordinary=Hero(ClassId::Mage);Tick(ordinary,.12f,hz,moving);moving.ability=14;ordinary.Update(.001f,moving);Check(!ordinary.Attacking(),"Ordinary stationary spell became mobile");
  auto repeated=Hero(ClassId::Mage);Cast(repeated,0,hz);Tick(repeated,repeated.Cooldown(0)+.01f,hz);Cast(repeated,0,hz);
  Check(repeated.ElementMask()==1 && repeated.DisciplineSeconds()==0,"Repeated fire completed confluence");

  auto scout=Hero(ClassId::Scout);const auto start=scout.Player();Retreat(scout,hz);
  Check(std::hypot(scout.Player().x-start.x,scout.Player().z-start.z)>1.5f && scout.DisciplineSeconds()>0,"Real retreat did not ready pursuit");
  auto normal=Hero(ClassId::Scout);const int normalHit=Cast(normal,12,hz),pursuit=Cast(scout,12,hz,true);
  Check(std::abs(float(pursuit-normalHit)-scout.StrikeDamage()*.45f)<1.1f && scout.DisciplineSeconds()==0,"Pursuit damage/spend");
  auto noTarget=Hero(ClassId::Scout);Check(noTarget.Load(noTarget.Save()) && !noTarget.TargetActor(),"No-target fixture");Retreat(noTarget,hz);Check(noTarget.DisciplineSeconds()==0,"Empty retreat awarded pursuit");
  auto stopped=Hero(ClassId::Scout);Command dash;dash.ability=2;stopped.Update(.001f,dash);stopped.StopMovement();Tick(stopped,.6f,hz);Check(stopped.DisciplineSeconds()==0,"Cancelled retreat awarded pursuit");
  auto miss=Hero(ClassId::Scout);Retreat(miss,hz);Command shot;shot.ability=0;miss.Update(.001f,shot);auto& escaped=const_cast<Enemy&>(miss.Dummies()[0]);escaped.position.x+=30;
  while(miss.Attacking())miss.Update(1.f/hz,{});Check(miss.DisciplineSeconds()>0,"Miss consumed pursuit");

  auto priest=Hero(ClassId::Priest);Heal(priest,hz);auto unblessed=Hero(ClassId::Priest);
  const int plainLight=Cast(unblessed,0,hz),blessedLight=Cast(priest,0,hz,true);
  Check(std::abs(float(blessedLight-plainLight)-priest.StrikeDamage()*.25f)<1.1f && priest.Barrier()>=18 && priest.DisciplineSeconds()==0,"Benediction damage/ward/spend");
  auto interrupted=Hero(ClassId::Priest);Command heal;heal.ability=3;interrupted.Update(.001f,heal);Command move;move.movement={1,0};interrupted.Update(.05f,move);
  Check(!interrupted.CastingRecovery() && interrupted.DisciplineSeconds()==0,"Interrupted heal granted benediction");
  Simulation full;full.CreateCharacter(ClassId::Priest,"Tester");full.Update(.001f,heal);
  Check(!full.CastingRecovery() && full.DisciplineSeconds()==0,"Full-health healing granted benediction");
 }
 ClassDiscipline d;Check(!d.Take(.04f),"Fresh state appears ready within a frame");d.Prime(1);d.Advance(1.02f);Check(d.Take(.03f) && !d.Take(.03f),"Sub-frame opportunity expiry/one-time spend");
 d.Clear();d.Contact(100,0,0);d.Contact(101,12,0);Check(d.mask==2,"Element memory leaked between targets");d.Advance(6.1f);d.Contact(101,13,0);Check(d.mask==4,"Expired element memory persisted");
}
void Native(){
 Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");Win32Window window;
 Check(window.Create(GetModuleHandleW(nullptr),1600,900,L"Class discipline review",false),"Window");Renderer r;
 Check(r.Initialize(window.GetHandle(),1600,900),"Renderer");const auto out=std::filesystem::current_path()/"class-discipline-review";std::filesystem::create_directories(out);
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout,ClassId::Priest}){
  Game g;g.Initialize();Check(g.RestoreSnapshot(Hero(id).Save()),"World fixture");auto& s=const_cast<Simulation&>(g.State());
  Command select;select.selectActor=210;s.Update(.001f,select);Check(s.TargetActor(),"World target selection");
  if(id==ClassId::Warrior)Guard(s,.12f,60);else if(id==ClassId::Mage)Elements(s,60);else if(id==ClassId::Priest)Heal(s,60);else Retreat(s,60);
  g.SetCameraOrbit(2.5f,.38f,6.5f);g.ExecuteCommand({},.016f);
  const auto unchanged=s.Save();for(int i=0;i<2;++i){r.BeginFrame();g.Render(r);Check(r.Capture((out/("ready-"+std::to_string(int(id))+"-"+std::to_string(i)+".png")).wstring()) && r.EndFrame(false),"Ready world capture");}
  Check(s.Save()==unchanged,"Presentation mutated gameplay");
  const auto sockets=castfx::Preview({0,0,0},{.2f,1.1f,.2f},0);r.BeginFrame();r.SetCamera({2,2,-4},{0,1,0});castfx::Discipline(r,s,sockets,2);
  Check(r.EffectVertices()>0 && r.EffectVertices()<512,"Readiness geometry budget");Check(r.EndFrame(false),"Ready render");
  CombatEffects fx;CombatVisualEvent e;e.kind=K::DisciplineReady;e.school=id;fx.Emit(e);fx.Advance(.15f);r.BeginFrame();fx.Render(r,&sockets);
  Check(r.EffectVertices()>0 && r.EffectVertices()<512 && r.EndFrame(false),"Readiness pulse budget");for(int n=0;n<14;++n)fx.Advance(.05f);r.BeginFrame();fx.Render(r,&sockets);Check(r.EffectVertices()==0 && r.EndFrame(false),"Readiness pulse never expired");
  if(id==ClassId::Mage){Command c;c.ability=14;g.ExecuteCommand(c,.001f);c.ability=-1;c.movement={.10f,0};for(int i=0;i<40;++i)g.ExecuteCommand(c,.01f);
   Check(s.ConfluenceCast(),"World lost mobile cast");r.BeginFrame();g.Render(r);Check(r.Capture((out/"mobile-mage.png").wstring()) && r.EndFrame(false),"Mobile cast capture");}
  Check(g.RestoreSnapshot(Hero(id).Save()) && g.State().DisciplineSeconds()==0,"World load retained opportunity");
 }
 Check(r.ValidateDebugMessages(),"DirectX diagnostics");
}
}
int main(){try{Rules();Native();std::cout<<"PASS: class opportunities, exact contacts, mobile confluence, cancellation/expiry/load, native UI and bounded VFX.\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
