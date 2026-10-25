#include "game/Game.h"
#include "game/Campaign.h"
#include "engine/Renderer.h"
#include "engine/Audio.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <set>
#include <cmath>
#include <sstream>
namespace {
using namespace rpg;
void Check(bool value,const char* message) {if(!value) {std::cerr<<message<<'\n';std::exit(1);}}
std::string Read(const std::filesystem::path& path) {std::ifstream in(path);std::ostringstream out;out<<in.rdbuf();return out.str();}
std::string Record(std::string value,const std::string& key,const std::string& line) {
 const auto at=value.find(key);Check(at!=std::string::npos,"Record missing");value.replace(at,value.find('\n',at)-at+1,line+"\n");return value;
}
void Roundtrip(const Simulation& s) {Simulation copy;Check(copy.Load(s.Save()) && copy.Save()==s.Save(),"Class/campaign round-trip");}
void Walk(Simulation& s,Vec2 goal) {
 for(auto point:s.Map().FindPath(s.Player(),goal)) for(int n=0;n<2000;++n) {
  const auto p=s.Player();if(std::hypot(p.x-point.x,p.z-point.z)<.12f) break;
  Command c;c.sprint=true;c.movement={point.x-p.x,point.z-p.z};s.Update(.05f,c);
 }
}
void Roster() {
 std::set<std::string> names,branches;
 for(int i=0;i<int(ClassId::Count);++i) {
  Simulation s;Check(s.CreateCharacter(ClassId(i),"Test Hero"),"Class creation");
  names.insert(s.ClassInfo().name);branches.insert(s.ClassInfo().branches[0]);
  Check(s.Health()==s.MaxHealth() && s.Level()==1 && s.EarnedPoints()==0,"Starting class stats");
  Roundtrip(s);const auto stable=s.Save();
  Check(!s.CreateCharacter(ClassId::Count,"Bad") && s.Save()==stable,"Invalid class mutation");
  Check(!s.CreateCharacter(ClassId(i),"  ") && s.Save()==stable,"Invalid name mutation");
  for(int spell=0;spell<4;++spell) Check(*s.Spell(spell).name && s.AbilityCooldown(spell)>0,"Class spell metadata");
  for(int talent=0;talent<int(Talent::Count);++talent) Check(*s.TalentDefinition(talent).name,"Class talent metadata");
  auto invalid=Record(stable,"PROFILE ","PROFILE 99 \"Bad\" 0 0 0");
  Check(!s.Load(invalid) && s.Save()==stable,"Invalid profile must reject atomically");
  Command reset;reset.restart=true;s.Update(.01f,reset);Check(s.Class()==ClassId(i) && std::string(s.Nickname())=="Test Hero","Restart lost identity");
 }
 Check(names.size()==11 && branches.size()==11,"Class identities are not distinct");
}
std::string WithProfile(const std::string& source,ClassId id) {
 Simulation s;Check(s.Load(source),"Profile source");auto save=s.Save();
 save=Record(save,"PROFILE ","PROFILE "+std::to_string(int(id))+" \"Tester\" 0 0 0");
 const auto start=save.find('\n',save.find("PROFILE "))+1,end=save.find('\n',start);
 std::istringstream in(save.substr(start,end-start));float x,z,h,f;int q,g;in>>x>>z>>h>>f>>q>>g;
 std::ostringstream row;row<<x<<' '<<z<<" 50 "<<f<<' '<<q<<' '<<g;save.replace(start,end-start,row.str());return save;
}
void ClassCombat(const std::string& veteran) {
 const std::string duel="MEDITERRANEAN 3\n-4 5.2 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 0 0 1 0.65 3.14159\n101 4 9 80 0 0 0 0 0\n102 -4 7 120 0 0 0 0 0\n";
 for(int id=0;id<int(ClassId::Count);++id) {
  Simulation build;Check(build.Load(WithProfile(veteran,ClassId(id))),"Class veteran migration");
  const float base=build.StrikeDamage(),health=build.Health();
  for(int talent:{0,3}) {Command c;c.learnTalent=talent;build.Update(.01f,c);}
  Check(build.BuildMask()==9 && build.StrikeDamage()>base && build.Health()==health && build.TalentPoints()==0,"Class talents must change real attacks without healing");
  Roundtrip(build);
  Command reset;reset.resetTalents=true;build.Update(.01f,reset);Check(build.StrikeDamage()==base,"Class talent reset");
  for(int talent:{2,5}) {Command c;c.learnTalent=talent;build.Update(.01f,c);}
  Check(build.MendHealing()>DescribeClass(ClassId(id)).healing && build.AbilityCooldown(3)<10,"Class recovery build has no gameplay effect");
  Roundtrip(build);
  for(int hz:{30,60,144,1000}) {
   Simulation s;auto fixture=duel;
   const bool ranged=DescribeClass(ClassId(id)).range>3;
   if(ranged && ClassId(id)!=ClassId::Psionic) fixture.replace(fixture.find("5.2 50"),3,"1");
   Check(s.Load(WithProfile(fixture,ClassId(id))),"Class attack fixture");
   Command select;select.nextTarget=true;s.Update(.001f,select);
   if(ClassId(id)==ClassId::Engineer) {s.Update(.001f,select);s.Update(.001f,select);}
   for(auto& e:const_cast<std::vector<Enemy>&>(s.Enemies())) {if(e.windup>0) e.windup=2;e.attackCooldown=10;}
   const auto target=s.TargetActor()->id;const auto maximum=s.TargetActor()->health;
   const float damage=s.HeavyDamage();int cast=0,hit=0,castSound=0;
   const auto observe=[&] {
    const auto releaseCue=ClassId(id)==ClassId::Warrior?sound::Cue::Heavy:sound::ClassReleaseCue(ClassId(id));
    sound::Events audio;audio.Update(s);for(unsigned n=0;n<audio.count;++n) castSound+=audio.cues[n]==releaseCue;
    for(std::size_t n=0;n<s.VisualEventCount();++n) {
     const auto& event=s.VisualEvents()[n];
     if(event.kind==CombatVisualKind::SpellCast) {
      ++cast;Check(event.school==ClassId(id) && event.flight>0,"Cast lacks class/timing data");
     }
     hit+=event.kind==CombatVisualKind::HeavyHit;
    }
   };
   Command attack;attack.ability=1;s.Update(.001f,attack);observe();Check(s.Attacking(),"Class heavy rejected");
   const float contactEnd=s.Spell(1).contact+.011f;
   for(float t=.001f;t<contactEnd-1e-7f;) {const float dt=std::min(1.f/hz,contactEnd-t);s.Update(dt,{});t+=dt;observe();}
   const auto& enemy=s.Enemies()[target-100];
   const float extra=ClassId(id)==ClassId::Mage?10.f:ClassId(id)==ClassId::Scout?8.f:ClassId(id)==ClassId::Engineer?12.f:ClassId(id)==ClassId::Demonologist?10.f:0.f;
   Check(std::abs(enemy.health-(maximum-damage-extra))<.001f,"Class heavy damage/passive differs across rates");
   const float healing=ClassId(id)==ClassId::Necromancer?(damage+extra)*.15f:ClassId(id)==ClassId::Bard?3.f:ClassId(id)==ClassId::Priest?8.f:ClassId(id)==ClassId::Demonologist?-5.f:0.f;
   Check(std::abs(s.Health()-(50+healing))<.001f,"Class sustain/sacrifice passive");
   Check(hit==1 && cast==int(ranged) && castSound==1,"Release sound and contact must each occur once");
   if(ClassId(id)==ClassId::Psionic) Check(enemy.windup==0 && enemy.attackCooldown>1.4f,"Psionic interruption duration");
   for(auto& e:const_cast<std::vector<Enemy>&>(s.Enemies())) {e.windup=std::min(e.windup,e.WindupDuration());e.attackCooldown=e.windup>0?0:std::min(e.attackCooldown,e.AttackInterval());}
   Roundtrip(s);
  }
 }
 // Range gating must follow the class without altering target identity or
 // spending resources on rejection. The ranged attack still has wind-up.
 auto distant=duel;distant.replace(distant.find("5.2 50"),3,"1");
 for(auto id:{ClassId::Warrior,ClassId::Mage}) {
  Simulation s;Check(s.Load(WithProfile(distant,id)),"Range fixture");Command c;c.nextTarget=true;c.ability=0;s.Update(.01f,c);
  Check(s.Attacking()==(id==ClassId::Mage) && s.Enemies()[0].health==80,"Class range/wind-up contract");
  if(id==ClassId::Warrior) Check(s.Cooldown(0)==0,"Rejected melee spent cooldown");
 }
}
std::string Campaign(const std::string& veteran) {
 {
  auto unclaimed=Record(veteran,"0 -10 ","0 -10 140 0 3 85");
  unclaimed=Record(unclaimed,"5 1 1 0","4 1 1 0");
  unclaimed=Record(unclaimed,"100 ","100 -4 7 0 0 0 3 0 0");
  Simulation pending;Check(pending.Load(unclaimed),"Unclaimed original loot fixture");Walk(pending,Simulation::questGiver);
  Command talk;talk.interact=true;pending.Update(.01f,talk);const auto before=pending.Save();
  Command accept;accept.questAction=QuestAction::AcceptFrontier;pending.Update(.01f,accept);
  Check(!pending.FrontierActive() && pending.Save()==before,"New squad discarded unclaimed original loot");
  Walk(pending,pending.Enemies()[0].position);pending.Update(.01f,talk);Check(pending.Gold()==90,"Original loot could not be recovered");
  Walk(pending,Simulation::questGiver);pending.Update(.01f,talk);pending.Update(.01f,accept);
  Check(pending.FrontierActive(),"Recovered loot did not unlock next squad");Roundtrip(pending);
 }
 Simulation s;Check(s.Load(veteran),"Legacy veteran migration");
 const int originalGold=s.Gold();
 for(int chapter=0;chapter<7;++chapter) {
  Walk(s,Simulation::questGiver);Command talk;talk.interact=true;s.Update(.01f,talk);
  Check(s.QuestPrimaryAction()==QuestAction::AcceptFrontier,"Next frontier offer");
  Command accept;accept.questAction=QuestAction::AcceptFrontier;s.Update(.01f,accept);
  Check(s.FrontierActive() && s.Enemies()[0].rank==chapter+1 && s.DefeatedCount()==0,"New enemy squad");
  Roundtrip(s);
  if(chapter==0) {
   // Sparring remains independent of an accepted frontier assignment.
   Simulation practice;Check(practice.Load(s.Save()),"Practice campaign fixture");Walk(practice,WorldMap::trainingPost);
   Command begin;begin.interact=true;practice.Update(.01f,begin);Check(practice.Sparring(),"Frontier sparring did not start");
   for(int frame=0;frame<500 && practice.Sparring();++frame) {
    Command hit;hit.ability=std::string(practice.AbilityStatus(1))=="READY"?1:0;practice.Update(.05f,hit);
   }
   Check(!practice.Sparring() && practice.DefeatedCount()==0 && practice.FrontierActive(),"Sparring victory corrupted campaign");Roundtrip(practice);
  }
  int deaths=0;
  for(int frame=0;frame<24000 && s.DefeatedCount()<3;++frame) {
   Command c;
   if(s.Health()<=0) {c.respawn=true;++deaths;Check(deaths<20,"Campaign cannot be cleared with current kit");}
   else {
    if(!s.TargetActor() || s.TargetActor()->health<=0) c.nextTarget=true;
    const auto* target=s.TargetActor();
    if(target) {
     const auto p=s.Player();const float distance=std::hypot(target->position.x-p.x,target->position.z-p.z);
     if(distance>2.f) {
      auto path=s.Map().FindPath(p,target->position);const auto goal=path.empty()?target->position:path.front();
      c.movement={goal.x-p.x,goal.z-p.z};c.sprint=true;
     }
     if(s.Health()<s.MaxHealth()*.65f && std::string(s.AbilityStatus(3))=="READY") c.ability=3;
     else if(std::string(s.AbilityStatus(1))=="READY") c.ability=1;
     else if(std::string(s.AbilityStatus(0))=="READY") c.ability=0;
    } else c.movement={-4-s.Player().x,7-s.Player().z};
   }
   s.Update(.05f,c);
  }
  Check(s.DefeatedCount()==3,"Campaign combat loop failed");Roundtrip(s);
  Check(s.ObjectivePosition().x==Simulation::questGiver.x && s.ObjectivePosition().z==Simulation::questGiver.z,"Turn-in marker must point to the keeper");
  {
   auto broken=s.Save();const auto start=broken.find("\n100 ")+1,end=broken.find('\n',start);
   std::istringstream in(broken.substr(start,end-start));unsigned id;float x,z,h,cd,windup,facing;int looted,state;
   in>>id>>x>>z>>h>>cd>>looted>>state>>windup>>facing;
   std::ostringstream row;row<<id<<' '<<x<<' '<<z<<' '<<h<<' '<<cd<<" 0 "<<state<<' '<<windup<<' '<<facing;
   broken.replace(start,end-start,row.str());Simulation invalid;const auto stable=invalid.Save();
   Check(!invalid.Load(broken) && invalid.Save()==stable,"Campaign corpse must not grant repeat loot");
  }
  Walk(s,Simulation::questGiver);s.Update(.01f,talk);
  Check(s.QuestPrimaryAction()==QuestAction::CompleteFrontier,"Frontier turn-in");
  Command finish;finish.questAction=QuestAction::CompleteFrontier;s.Update(.01f,finish);
  Check(s.FrontierCompleted()==chapter+1 && s.Level()==chapter+4,"Level progression to ten");
  const auto xp=s.Experience();s.Update(.01f,finish);Check(s.Experience()==xp,"Repeated reward farming");Roundtrip(s);
 }
 Check(s.Level()==10 && s.Gold()==originalGold && s.EarnedPoints()==5,"Level cap and rewards");
 Check(s.ObjectivePosition().x==WorldMap::residents[0].position.x && s.ObjectivePosition().z==WorldMap::residents[0].position.z,
   "Completed original campaign must point to the western expansion");
 for(auto talent:{Talent::WeaponTraining,Talent::Vitality,Talent::Restoration,Talent::WeaponMastery,Talent::Bulwark}) {
  Command learn;learn.learnTalent=int(talent);s.Update(.01f,learn);
 }
 Check(s.TalentPoints()==0 && TalentCount(s.BuildMask())==5,"Level-ten build budget");Roundtrip(s);
 return s.Save();
}
void Mastery(const std::string& levelTen) {
 for(int id=0;id<int(ClassId::Count);++id) for(int branch=0;branch<3;++branch) {
  auto fixture=levelTen;const auto at=fixture.find("PROFILE ")+8;
  fixture.replace(at,fixture.find(' ',at)-at,std::to_string(id));
  fixture=Record(fixture,"BUILD ","BUILD 0");
  const auto player=fixture.find('\n',fixture.find("PROFILE "))+1;
  std::istringstream row(fixture.substr(player,fixture.find('\n',player)-player));float x,z,h,f;int q,g;row>>x>>z>>h>>f>>q>>g;
  std::ostringstream line;line<<"0 -10 40 "<<f<<' '<<q<<' '<<g;fixture.replace(player,fixture.find('\n',player)-player,line.str());
  Simulation s;Check(s.Load(fixture),"Mastery level ten fixture");
  const float health=s.Health(),base=s.MaxHealth(),armor=s.DamageReduction();
  for(int node:{branch,9+branch,12+branch,15+branch}) {Command learn;learn.learnTalent=node;s.Update(.01f,learn);Check(s.HasTalent(Talent(node)),"Mastery prerequisite chain could not be learned");}
  Check(s.TalentPoints()==1 && s.Health()==health,"Mastery spent wrong budget or healed on learning");Roundtrip(s);
  const auto preview=s.PreviewBuild(s.BuildMask());
  Check(std::abs(preview.basicCooldown-s.AbilityCooldown(0))<.0001f && std::abs(preview.heavyCooldown-s.AbilityCooldown(1))<.0001f &&
    std::abs(preview.recoveryCast-s.RecoveryCastDuration())<.0001f,"Mastery preview disagrees with simulation");
  if(branch==0) Check(s.AbilityCooldown(0)<.76f && s.AbilityCooldown(1)<3.21f,"Tempo/Pressure did not shorten future cooldowns");
  if(branch==1) Check(s.MaxHealth()>=base+45 && s.DamageReduction()>=armor+2,"Defensive mastery lacks real stats");
  if(branch==2) {
    Check(std::abs(s.RecoveryCastDuration()-ClassRecoveryCast(s.Class())/1.45f)<.0001f,"Cast speed calculation");
    if(!ClassHealer(s.Class())) Check(s.RecoveryWard()>=ClassRecoveryBarrier(s.Class())+12,"Overflow lacks defensive capacity");
    Command cast;cast.ability=3;s.Update(.001f,cast);
    const float duration=s.RecoveryCastDuration();
    for(float t=0;t<duration+.05f;t+=.01f)s.Update(.01f,{});
    Check(!s.CastingRecovery() && s.Cooldown(3)>0,"Advanced recovery failed to finish");
  }
  auto old=s.Save();old.replace(14,old.find('\n')-14,"13");Simulation restored;const auto stable=restored.Save();
  Check(!restored.Load(old) && restored.Save()==stable,"Legacy save accepted mastery bits");
  unsigned missing=s.BuildMask()&~(1u<<branch);Check(!ValidBuild(missing,10),"Mastery without foundation was accepted");
  Check(!ValidBuild(s.BuildMask(),4),"Advanced build accepted before level gate");
  // Exercise capstones through actual contacts/completions, not just tooltips.
  const auto placeEnemy=[](Simulation& actor,float range) {
    auto& enemy=const_cast<Enemy&>(actor.Enemies()[0]);
    enemy.health=10000;enemy.position=enemy.home={0,-10+range};enemy.state=EnemyState::Idle;
    enemy.attackCooldown=100;enemy.windup=0;
    Command select;select.nextTarget=true;actor.Update(.001f,select);
  };
  if(branch==0) {
    Simulation plain;auto lesser=Record(s.Save(),"BUILD ","BUILD "+std::to_string(s.BuildMask()&~(1u<<unsigned(Talent::Overwhelming))));
    Check(plain.Load(lesser),"Unempowered mastery comparison fixture");
    float damage[2]{};int comparison=0;
    for(auto* actor:{&plain,&s}) {
      placeEnemy(*actor,1.8f);
      for(int n=0;n<3000 && (!actor->EmpoweredReady() || actor->Attacking());++n) {
        Command basic;if(!actor->Attacking() && actor->Cooldown(0)==0) basic.ability=0;actor->Update(.01f,basic);
      }
      Check(actor->EmpoweredReady(),"Capstone resource setup");
      const float before=actor->Enemies()[0].health;Command heavy;heavy.ability=1;actor->Update(.001f,heavy);
      for(float time=0;time<actor->Spell(1).contact+.02f;time+=.01f)actor->Update(.01f,{});
      damage[comparison++]=before-actor->Enemies()[0].health;
    }
    Check(damage[0]>0 && std::abs(damage[1]-damage[0]*1.2f)<.01f,"Capstone empowered damage did not reach actual contact");
  }
  if(branch==1) {
    const auto beforeMobility=s.Save();
    placeEnemy(s,3.5f);Command dash;dash.ability=2;s.Update(.001f,dash);
    Check(s.Dashing(),"Second wind mobility fixture");
    for(int n=0;n<100 && s.Dashing();++n)s.Update(.01f,{});
    Check(s.Barrier()>=18,"Completed mobility did not grant Second wind ward");
    Check(s.Load(beforeMobility),"Cancelled mobility fixture");placeEnemy(s,3.5f);
    s.Update(.001f,dash);s.StopMovement();for(int n=0;n<100;++n)s.Update(.01f,{});
    Check(s.Barrier()==0,"Cancelled mobility granted a capstone ward");
  }
  if(branch==2 && ClassHealer(s.Class())) {
    auto almost=s.Save();const auto start=almost.find('\n',almost.find("PROFILE "))+1;
    std::ostringstream nearFull;nearFull<<"0 -10 "<<s.MaxHealth()-1<<" 0 "<<q<<' '<<g;
    almost.replace(start,almost.find('\n',start)-start,nearFull.str());
    Check(s.Load(almost),"Overflow fixture");
    for(int n=0;n<1200;++n)s.Update(.01f,{});
    const float excess=(s.MendHealingNow()-1)*.5f;Command heal;heal.ability=3;s.Update(.001f,heal);
    for(int n=0;n<200;++n)s.Update(.01f,{});
    Check(s.Health()==s.MaxHealth() && s.Barrier()>=excess-.01f,"Overhealing did not become a barrier");
  }
 }
}
void Captures(const std::filesystem::path& root,const std::string& levelTen) {
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Class campaign test",false),"Test window");
 Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer");
 Game game;game.Initialize();Check(game.RestoreSnapshot(levelTen),"Level ten game");
 Simulation master;Check(master.Load(levelTen),"Mastery render fixture");
 for(const auto size:{std::pair{960,600},std::pair{1280,720},std::pair{2560,1080}}) {
  Check(renderer.Resize(size.first,size.second),"Mastery resize");
  ui::Interface book;book.SetOnly(ui::Panel::Build);book.talentPage=1;book.selectedTalent=15;
  renderer.BeginFrame();book.Render(renderer,master,1,false);
  Check(renderer.Capture((root/("mastery-"+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Mastery UI capture");
  const ui::Layout layout(float(size.first),float(size.second));
  const auto hit=book.Pointer(layout.x+200*layout.scale,layout.y+490*layout.scale,float(size.first),float(size.second),true,false);
  Check(hit.action==ui::Action::SelectTalent && hit.value==15,"Mastery node hit routing");
 }
 Check(renderer.Resize(1280,720),"Restore capture size");
 for(const auto panel:{ui::Panel::Build,ui::Panel::Journal,ui::Panel::Map}) {
  game.OpenPanel(panel);renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/("panel-"+std::to_string(int(panel))+".png")).wstring()) && renderer.EndFrame(false),"Campaign UI capture");
 }
 Simulation mage;Check(mage.CreateCharacter(ClassId::Mage,"Aster"),"Mage");
 auto save=Record(levelTen,"PROFILE ","PROFILE 5 \"Aster\" 7 0 0");
 // Use the actual legacy-loot count from the completed fixture.
 const auto profileEnd=levelTen.find('\n',levelTen.find("PROFILE "));
 const auto loot=levelTen.substr(profileEnd-1,1);save=Record(save,"PROFILE ","PROFILE 5 \"Aster\" 7 0 "+loot);
 const auto playerStart=save.find('\n',save.find("PROFILE "))+1;
 std::istringstream row(save.substr(playerStart,save.find('\n',playerStart)-playerStart));float x,z,h,f;int q,g;row>>x>>z>>h>>f>>q>>g;
 std::ostringstream fixed;fixed<<x<<' '<<z<<" 100 "<<f<<' '<<q<<' '<<g;
 save.replace(playerStart,save.find('\n',playerStart)-playerStart,fixed.str());
 Check(game.RestoreSnapshot(save),"Mage veteran");game.OpenPanel(ui::Panel::Build);renderer.BeginFrame();game.Render(renderer);
 Check(renderer.Capture((root/"mage-build.png").wstring()) && renderer.EndFrame(false),"Mage build capture");
 for(int id=0;id<int(ClassId::Count);++id) {
  const auto profile=Record(save,"PROFILE ","PROFILE "+std::to_string(id)+" \"Tester\" 7 0 "+loot);
  Check(game.RestoreSnapshot(profile),"Class build capture fixture");game.OpenPanel(ui::Panel::Build);
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/("class-build-"+std::to_string(id)+".png")).wstring()) && renderer.EndFrame(false),"Class build capture");
 }
 const std::string duel="MEDITERRANEAN 3\n-4 1 50 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 0 0 0 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
 Check(game.RestoreSnapshot(WithProfile(duel,ClassId::Mage)),"Ranged animation fixture");
 game.OpenPanel(ui::Panel::None);game.SetCameraOrbit(-.65f,.35f,7.f);
 Input input;Input::Frame frame;frame.focused=true;input.Update(frame);game.Update(.01,input,0);
 frame.down[VK_TAB]=true;frame.down['2']=true;input.Update(frame);game.Update(.01,input,0);
 frame.down.fill(false);
 for(int step=0;step<17;++step) {
  input.Update(frame);game.Update(.05,input,0);
  if(step==4 || step==5 || step==16) {
   renderer.BeginFrame();game.Render(renderer);
   Check(renderer.Capture((root/("mage-cast-"+std::to_string(step)+".png")).wstring()) && renderer.EndFrame(false),"Ranged animation capture");
  }
 }
 Check(game.PlayerAnimation().attackSeconds<0 && game.PlayerAnimation().cast<.01f,"Ranged pose did not recover");
 Check(renderer.ValidateDebugMessages(),"D3D validation");
}
}
int main() {
 const auto root=std::filesystem::current_path()/"class-campaign-check";std::filesystem::create_directories(root);
 const auto veteran=Read(std::filesystem::path(RPG_TEST_FIXTURES)/"veteran-preview.save");
 Roster();ClassCombat(veteran);const auto end=Campaign(veteran);Mastery(end);
 Captures(root,end);std::cout<<"PASS: eleven class profiles, save migration, seven playable frontier assignments, enemy squads, level ten, build budget and UI captures\n";
}
