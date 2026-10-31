#include "game/Game.h"
#include "game/BuildLayout.h"
#include "game/ClassEffects.h"
#include "engine/Renderer.h"
#include "engine/BenchmarkStats.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <algorithm>
namespace {
using namespace rpg;
void Check(bool b,const char* message) {if(!b) {std::cerr<<message<<'\n';std::exit(1);}}
std::string Record(std::string s,const std::string& key,const std::string& replacement) {
 const auto line=s.find("\n"+key);Check(line!=s.npos,"Missing fixture record");const auto at=line+1;
 s.replace(at,s.find('\n',at)-at,replacement);return s;
}
void Warp(Simulation& s,Vec2 p) {
 auto data=s.Save();const auto at=data.find('\n',data.find("PROFILE "))+1;
 std::istringstream row(data.substr(at,data.find('\n',at)-at));float x,z,h,f;int q,g;row>>x>>z>>h>>f>>q>>g;
 std::ostringstream line;line<<p.x<<' '<<p.z<<' '<<h<<' '<<f<<' '<<q<<' '<<g;
 data.replace(at,data.find('\n',at)-at,line.str());
 if(!s.Load(data)) {std::cerr<<"Warp rejected "<<p.x<<','<<p.z<<" chapter "<<s.FrontierCompleted()<<" stand "<<s.CanStand(p)<<'\n'<<data;Check(false,"Southern relocation rejected");}
}
void Tick(Simulation& s,float duration,int hz=100) {
 while(duration>1e-6f) {const float dt=std::min(duration,1.f/hz);s.Update(dt,{});duration-=dt;}
}
void Roundtrip(const Simulation& s) {Simulation copy;Check(copy.Load(s.Save()) && copy.Save()==s.Save(),"Southern save round-trip");}
void Talk(Simulation& s,QuestAction action) {
 const auto p=QuestLocation(s.CurrentQuest());Warp(s,{p.x+1.6f,p.z});Command c;c.interact=true;s.Update(.001f,c);
 Check(s.QuestPrimaryAction()==action,"Southern dialogue action");c={};c.questAction=action;s.Update(.001f,c);
}
void Clear(Simulation& s) {
 auto data=s.Save();for(const auto& e:s.Enemies()) {
  std::ostringstream line;line<<e.id<<' '<<e.home.x<<' '<<e.home.z<<" 0 0 1 3 0 0";
  data=Record(data,std::to_string(e.id)+" ",line.str());
 }
 if(s.CourtActive()) data=Record(data,"COURT ","COURT 7");
 Check(s.Load(data),"Cleared wave fixture rejected");
}
Simulation Veteran(ClassId id,int completed=13) {
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n0 -10 100 0 3 90\n5 1 1 0\n4\n0 0 0 0\n100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Legacy hero");
 auto data=Record(s.Save(),"PROFILE ","PROFILE "+std::to_string(int(id))+" \"Southguard\" 10 0 3");
 data=Record(data,"SURVEY ","SURVEY 7");Check(s.Load(data),"Highland hero");
 Talk(s,QuestAction::AcceptFrontier);Clear(s);
 data=Record(s.Save(),"PROFILE ","PROFILE "+std::to_string(int(id))+" \"Southguard\" "+std::to_string(completed)+" 0 3");
 Check(s.Load(data),"Southern hero");return s;
}
void Regions() {
 WorldMap map;Check(WorldMap::halfWidth==120 && WorldMap::halfDepth==128,"Province dimensions");
 Check(map.Height({86,-99})>7 && map.Height({-90,-96})>7 && map.Height({0,-10})==0,"Relief or preserved village floor");
 for(int z=0;z<terrain::rows-1;++z)for(int x=0;x<terrain::columns-1;++x) {
  const auto& h=terrain::Heights();const int i=z*terrain::columns+x;
  Check(std::hypot(h[i+1]-h[i],h[i+terrain::columns]-h[i])/terrain::cell<.68f,"Near-vertical terrain triangle");
 }
 for(const auto& npc:WorldMap::residents) {
  Check(map.CanStand(npc.position,.45f),"NPC inside prop");
  Check(!map.FindPath(WorldMap::spawn,npc.position).empty(),"NPC unreachable");
 }
 for(const auto& t:map.Trails()) if(!map.CanTravel(t.from,t.to,.45f)) {
  std::cerr<<"Blocked trail "<<t.from.x<<','<<t.from.z<<" -> "<<t.to.x<<','<<t.to.z<<'\n';Check(false,"Blocked route");
 }
 Simulation camera;const Vec2 besideBeacon{-86,-89.4f};
 const Vec2 courtApproach{99,-118};
 Check(camera.CameraDistance(courtApproach,map.Height(courtApproach)+1.4f,
   {-std::sin(-.7f)*std::cos(.55f),-std::cos(-.7f)*std::cos(.55f)},std::sin(.55f),15)>1.5f,
   "Overlapping tree crown traps the camera inside the player");
 Check(camera.CameraDistance(besideBeacon,map.Height(besideBeacon)+1.4f,{0,-std::cos(.45f)},std::sin(.45f),10)>3,
   "Small ritual marker retracts the camera inside the champion");
 for(int completed:{13,14,15}) {
  auto s=Veteran(ClassId::Warrior,completed);Talk(s,QuestAction::AcceptFrontier);
  for(const auto& e:s.Enemies()) {Check(map.CanStand(e.home,.45f),"Enemy camp inside prop");Check(!map.FindPath(QuestLocation(s.CurrentQuest()),e.home).empty(),"Enemy camp inaccessible");}
 }
}
void Campaign() {
 for(int hz:{30,144}) {
  auto s=Veteran(ClassId::Warrior);const int xp=s.Experience();
  // Version 19 migrates without manufacturing new quest progress or cooldowns.
  auto legacy=s.Save();legacy.erase(legacy.find("SOUTHERN "));legacy.replace(14,2,"19");Simulation migrated;
  Check(migrated.Load(legacy) && migrated.CurrentQuest()==QuestId::Watch14 && migrated.Cooldown(11)==0,"v19 migration");
  Talk(s,QuestAction::AcceptFrontier);Check(s.ExpeditionActive() && s.DefeatedCount()==0,"Supply encounter");
  for(int i=0;i<3;++i) {
   const auto p=s.ExpeditionSite(i);Warp(s,{p.x,p.z-1.3f});Command c;c.interact=true;s.Update(.001f,c);Roundtrip(s);
   Check(s.ExpeditionMask()==(1u<<(i+1))-1,"Supply cache interaction");
  }
  Check(s.FrontierReady() && s.DefeatedCount()==0,"Supply quest requires unwanted kills");
  Talk(s,QuestAction::CompleteFrontier);Check(s.Experience()==xp+250,"Supply reward");Roundtrip(s);
  Talk(s,QuestAction::AcceptFrontier);Check(s.RitualActive() && s.RitualStage()==0 && !s.FrontierReady(),"Vigil started or completed prematurely");
  const auto beacon=WorldMap::ritualSite;Warp(s,{beacon.x,beacon.z+1.6f});Command c;c.interact=true;s.Update(.001f,c);
  Check(s.RitualStage()==1 && s.DefeatedCount()==4,"Wave did not activate exactly two enemies");
  auto invalid=Record(s.Save(),"SOUTHERN ","SOUTHERN 0 0 0 0");Check(!s.Load(invalid),"Unstarted ritual permits live attackers");
  for(int wave=1;wave<=3;++wave) {
   Check(s.RitualStage()==wave && s.DefeatedCount()==4,"Unexpected wave population");Clear(s);
   Tick(s,2,hz);Check(s.RitualCharge()>.49f && s.RitualCharge()<.51f && !s.FrontierReady(),"Four-second charge timing");Roundtrip(s);
   if(wave==1) {
    c={};c.jump=true;s.Update(.01f,c);Check(s.RitualCharge()==0,"Jump preserved charge");Tick(s,1,hz);
    c={};c.movement={1,0};for(int n=0;n<hz*2;++n)s.Update(1.f/hz,c);
    Check(s.RitualCharge()==0 && s.RitualStage()==1,"Leaving beacon advanced ritual");
    Warp(s,{beacon.x,beacon.z+1.6f});Tick(s,2,hz);
   }
   Tick(s,2.02f,hz);Check(s.RitualStage()==wave+1,"Wave transition failed");Roundtrip(s);
  }
  Check(s.FrontierReady(),"Vigil never completed");Talk(s,QuestAction::CompleteFrontier);
  Check(s.Experience()==xp+500 && s.RitualStage()==0,"Vigil reward or cleanup");
  Talk(s,QuestAction::AcceptFrontier);Check(s.Enemies().size()==6 && s.Enemies().back().Boss(),"Court lacks captain");
  Clear(s);Talk(s,QuestAction::CompleteFrontier);Roundtrip(s);
  Check(s.Experience()==xp+750 && s.FrontierCompleted()==16 && s.Level()==10,"Final reward/cap");
  c={};c.questAction=QuestAction::CompleteFrontier;s.Update(.01f,c);Check(s.Experience()==xp+750,"Repeated reward");
 }
}
// Freeze opposing AI only in isolated contact tests. Reserve is earned through
// actual attacks, never injected; campaign tests above use the real wave lifecycle.
void CombatFixture(Simulation& s) {
 Talk(s,QuestAction::AcceptFrontier);
 Warp(s,{-21,10});
 for(std::size_t i=0;i<s.Enemies().size();++i) {
  auto& e=const_cast<Enemy&>(s.Enemies()[i]);e.position=e.home={-21.f,12.f+float(i)*10};
  e.health=e.MaxHealth();e.looted=false;e.state=EnemyState::Idle;e.stunned=1000;e.rooted=1000;
 }
 auto& side=const_cast<Enemy&>(s.Enemies()[1]);side.position=side.home={-20,8};
}
void FillReserve(Simulation& s,int hz) {
 for(int i=0;i<s.CombatStyleInfo().capacity;++i) {
  Command c;c.selectActor=100;c.ability=0;s.Update(.001f,c);
  Tick(s,std::max(s.Spell(0).recovery,s.AbilityCooldown(0))+.02f,hz);
  auto& e=const_cast<Enemy&>(s.Enemies()[0]);e.health=e.MaxHealth();
 }
 Check(s.EmpoweredReady(),"Basic contacts did not build reserve");
}
void Spells() {
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) {
  Simulation novice;Check(novice.CreateCharacter(id,"Novice"),"Fresh profile");
  for(int ability:{10,11}) {Command c;c.ability=ability;novice.Update(.01f,c);Check(novice.Cooldown(ability)==0 && !novice.Attacking(),"Level gate bypass");}
  for(int hz:{30,144}) for(int ability:{10,11}) {
   auto s=Veteran(id);CombatFixture(s);
   if(ability==11) {
    Command early;early.selectActor=100;early.ability=11;s.Update(.001f,early);
    Check(s.Cooldown(11)==0 && !s.Attacking(),"Empty reserve accepted");FillReserve(s,hz);
   }
   const float before=s.Enemies()[0].health,secondary=s.Enemies()[1].health;
   Command c;c.selectActor=100;c.ability=ability;s.Update(.001f,c);Check(s.Attacking(),"Discipline rejected");
   if(ability==11)Check(s.ClassResource()==0,"Capstone did not spend reserve");
   Tick(s,s.Spell(ability).contact-.002f,hz);Check(s.Enemies()[0].health==before,"Damage before contact");
   Tick(s,.003f,hz);Check(s.Enemies()[0].health<before,"Missing contact");
   Check((s.Enemies()[1].health<secondary)==(ability==11 && id!=ClassId::Scout),"Class area shape or rear target rule");
   if(ability==10 && id!=ClassId::Scout)Check(s.ClassResource()==1,"Control contact reserve");
   if(ability==11 && id==ClassId::Warrior)Check(s.Barrier()>=25,"Cyclone barrier");
   Roundtrip(s);Simulation loaded;Check(loaded.Load(s.Save()) && loaded.Cooldown(ability)>0 && !loaded.Attacking(),"Discipline cooldown/reset on load");
   if(id!=ClassId::Warrior) {
    auto move=Veteran(id);CombatFixture(move);if(ability==11)FillReserve(move,hz);
    c={};c.selectActor=100;c.ability=ability;move.Update(.001f,c);
    c={};c.movement={1,0};move.Update(.02f,c);
    Check(move.Attacking()==(ability==10),"Mobile control / stationary finisher distinction");
    if(ability==11)Check(move.Cooldown(11)>0 && move.ClassResource()==0,"Cancellation refunded resources");
   }
  }
 }
 ui::Interface book;Check(book.HotbarSpell(15,ClassId::Mage)==10 && book.HotbarSpell(16,ClassId::Mage)==11,"New default bindings");
 book.AssignHotbar(11,0,ClassId::Mage);ui::Interface copy;Check(copy.LoadHotbars(book.SaveHotbars()) && copy.HotbarSpell(0,ClassId::Mage)==11,"Capstone drag/binding persistence");
}
void DefeatSentry(Simulation& s,int index) {
 auto& e=const_cast<Enemy&>(s.Enemies()[index]);e.health=0;e.state=EnemyState::Dead;e.looted=true;e.windup=0;
}
void Siege() {
 for(int hz:{30,144}) {
  auto s=Veteran(ClassId::Warrior,15);Talk(s,QuestAction::AcceptFrontier);
  Check(s.CourtActive() && s.CourtWardCount()==3 && !s.FrontierReady(),"Court initial state");
  for(int i=0;i<3;++i) Check(s.CourtGuarded(i),"Unguarded starting seal");
  // Real contact against the captain cannot damage, stun or build reserve.
  Warp(s,{99,-113});Command c;c.selectActor=105;c.ability=10;s.Update(.001f,c);
  const float hp=s.Enemies().back().health;Tick(s,.5f,hz);
  Check(s.Enemies().back().health==hp && s.ClassResource()==0 && s.Enemies().back().stunned==0,"Protected captain granted contact benefits");
  const auto first=WorldMap::courtSeals[0];Warp(s,{first.x,first.z+1.6f});
  c={};c.interact=true;s.Update(.001f,c);Check(s.CourtChannelSite()<0,"Living sentries permit channel");
  for(int site=0;site<3;++site) {
   for(int i=site*2;i<std::min(site*2+2,5);++i)DefeatSentry(s,i);
   const auto p=WorldMap::courtSeals[site];Warp(s,{p.x,p.z+(site==1?1.6f:-1.6f)});
   c={};c.interact=true;s.Update(.001f,c);Tick(s,1,hz);
   Check(s.CourtChannelProgress()>.39f && s.CourtChannelProgress()<.41f,"Seal channel timing");
   // Saving retains completed seals but never restores an unfinished cast.
   Simulation copy;Check(copy.Load(s.Save()) && copy.CourtSeals()==s.CourtSeals() && copy.CourtChannelSite()<0,"Seal load/cast reset");
   if(site==0) {
    c={};c.movement={1,0};s.Update(.01f,c);Check(s.CourtChannelSite()<0,"Movement did not cancel channel");
    Warp(s,{p.x,p.z-1.6f});c={};c.interact=true;s.Update(.001f,c);Tick(s,.5f,hz);
    c={};c.haltMovement=true;s.Update(.01f,c);Check(s.CourtChannelSite()<0,"Escape did not cancel channel");
    c={};c.interact=true;s.Update(.001f,c);Tick(s,1,hz);
   }
   Tick(s,1.52f,hz);Check((s.CourtSeals()&(1u<<site))!=0,"Seal never completed");Roundtrip(s);
   c={};c.interact=true;s.Update(.01f,c);Check(s.CourtChannelSite()<0,"Completed seal restarted");
  }
  Check(s.CourtWardCount()==0 && !s.FrontierReady(),"Captain must still be defeated");
  Warp(s,{99,-113});auto& boss=const_cast<Enemy&>(s.Enemies().back());boss.position=boss.home;boss.stunned=10;
  c={};c.selectActor=105;c.ability=0;s.Update(.001f,c);Tick(s,.2f,hz);
  Check(s.Enemies().back().health<hp,"Broken ward still blocks damage");
  DefeatSentry(s,5);Check(s.FrontierReady(),"Court cannot complete");const int xp=s.Experience();
  Talk(s,QuestAction::CompleteFrontier);Check(s.Experience()==xp+250 && s.CourtSeals()==0,"Court reward/cleanup");Roundtrip(s);
 }
 auto s=Veteran(ClassId::Mage,15);Talk(s,QuestAction::AcceptFrontier);
 const auto before=s.Save();Check(!s.Load(Record(before,"COURT ","COURT 1")) && s.Save()==before,"Seal progress bypasses living sentries or mutates failed load");
 DefeatSentry(s,0);DefeatSentry(s,1);
 auto old=s.Save();old.erase(old.find("COURT "));old.replace(14,2,"20");
 Simulation migrated;Check(migrated.Load(old) && migrated.CourtSeals()==1,"v20 cleared patrol migration");
 DefeatSentry(s,5);old=s.Save();old.erase(old.find("COURT "));old.replace(14,2,"20");
 Check(migrated.Load(old) && migrated.CourtSeals()==7 && migrated.Enemies()[2].health>0,"Legacy captain victory migration");Roundtrip(migrated);
 // A former walkable location occupied by a new foundation must load nearby.
 auto legacy=Veteran(ClassId::Mage).Save();legacy.erase(legacy.find("COURT "));legacy.replace(14,2,"20");
 const auto at=legacy.find('\n',legacy.find("PROFILE "))+1;
 legacy.replace(at,legacy.find('\n',at)-at,"-47 -77 100 0 3 90");
 Check(migrated.Load(legacy) && migrated.CanStand(migrated.Player()),"v20 new foundation migration");
}
void TacticalContacts() {
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) for(int hz:{30,144}) {
  auto s=Veteran(id);CombatFixture(s);auto& e=const_cast<Enemy&>(s.Enemies()[0]);
  e.stunned=0;e.rooted=10;e.snared=e.exposed=0;e.windup=1.1f;e.state=EnemyState::Pursuing;e.attackCooldown=0;
  Command c;c.selectActor=100;c.ability=10;s.Update(.001f,c);Tick(s,s.Spell(10).contact+.005f,hz);
  Check(e.windup==0 && s.ClassResource()==(id==ClassId::Mage?2:1),"Class interrupt reward");
  if(id==ClassId::Warrior)Check(s.Barrier()==40,"Warrior reactive guard");
  if(id==ClassId::Scout)Check(e.exposed>5.9f,"Scout interruption setup");
  // Compare prepared and unprepared contact using earned reserve in both cases.
  float dealt[2]{};
  for(int prepared=0;prepared<2;++prepared) {
   auto finisher=Veteran(id);CombatFixture(finisher);FillReserve(finisher,hz);
   auto& victim=const_cast<Enemy&>(finisher.Enemies()[0]);
   victim.health=victim.MaxHealth();victim.exposed=0;victim.rooted=0;victim.snared=0;
   if(prepared) {if(id==ClassId::Mage) victim.snared=6;else victim.exposed=6;}
   const float health=victim.health;c={};c.selectActor=100;c.ability=11;finisher.Update(.001f,c);
   Tick(finisher,finisher.Spell(11).contact+.005f,hz);dealt[prepared]=health-victim.health;
   Check(victim.exposed==0 && victim.snared==0,"Finisher did not consume setup");
   if(prepared && id==ClassId::Warrior)Check(finisher.Barrier()==40,"Prepared cyclone guard");
  }
  Check(dealt[1]>dealt[0]+10,"Prepared finishing attack has no payoff");
  // Exercise the usable window through real skill recovery, not only injected
  // statuses: rend -> cyclone, winter lance -> firestorm, pierce -> horizon.
  auto chain=Veteran(id);CombatFixture(chain);FillReserve(chain,hz);
  auto& target=const_cast<Enemy&>(chain.Enemies()[0]);target.rooted=target.snared=target.exposed=0;
  c={};c.selectActor=100;c.ability=id==ClassId::Warrior?7:id==ClassId::Mage?5:9;
  chain.Update(.001f,c);Tick(chain,chain.Spell(c.ability).recovery+.01f,hz);
  Check(chain.FinisherSetup(target) && chain.EmpoweredReady(),"Setup expires before the next action can begin");
  c={};c.ability=11;chain.Update(.001f,c);Tick(chain,chain.Spell(11).contact+.005f,hz);
  Check(!chain.FinisherSetup(target) && chain.ClassResource()==0,"Real combo failed to consume reserve/setup");
  auto immune=Veteran(id);CombatFixture(immune);auto& captain=const_cast<Enemy&>(immune.Enemies()[0]);
  captain.kind=EnemyKind::Captain;captain.rank=16;captain.bossAttack=1;captain.windup=1.4f;
  captain.stunned=captain.exposed=captain.snared=0;captain.state=EnemyState::Pursuing;
  c={};c.selectActor=100;c.ability=10;immune.Update(.001f,c);Tick(immune,immune.Spell(10).contact+.005f,hz);
  Check(captain.windup>0 && captain.stunned==0 && captain.exposed==0 && immune.Barrier()==0 &&
    immune.ClassResource()==(id==ClassId::Scout?0:1),"Uninterruptible attack granted counterplay reward");
 }
}
void Capture(Renderer& r,const std::filesystem::path& path) {Check(r.Capture(path.wstring()) && r.EndFrame(false),"Native scene capture/budget");}
void WalkTo(Simulation& s,Vec2 goal) {
 const auto path=s.Map().FindPath(s.Player(),goal);Check(!path.empty(),"Replay walking route missing");
 for(const auto p:path) {
  int steps=0;
  while(std::hypot(s.Player().x-p.x,s.Player().z-p.z)>.18f && steps++<4000) {
   const auto from=s.Player();const float d=std::hypot(p.x-from.x,p.z-from.z);
   Command c;const float scale=std::min(1.f,d*.8f)/std::max(d,.001f);
   c.movement={(p.x-from.x)*scale,(p.z-from.z)*scale};s.Update(.02f,c);
  }
  Check(steps<4000,"Replay walking route stuck");
 }
 Tick(s,.3f);
}
void ClearLive(Simulation& s) {
 for(auto& e:const_cast<std::vector<Enemy>&>(s.Enemies())) {
  e.health=0;e.looted=true;e.state=EnemyState::Dead;e.windup=0;
 }
}
void ReplayEncounters() {
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) for(int chapter:{12,14,15}) {
  auto s=Veteran(id,16);const int xp=s.Experience(),gold=s.Gold(),potions=s.Potions();
  const auto quest=QuestId(int(QuestId::Watch1)+chapter);const auto npc=QuestLocation(quest);
  Warp(s,{npc.x+1.6f,npc.z});const auto campaign=s.Save();
  Command c;c.interact=true;s.Update(.001f,c);
  Check(s.QuestPrimaryAction()==QuestAction::BeginReplay,"Completed leader did not offer replay");
  c={};c.questAction=QuestAction::BeginReplay;s.Update(.001f,c);
  Check(s.ReplayActive() && s.CurrentQuest()==quest && s.FrontierCompleted()==16 && s.Experience()==xp,"Replay changed campaign or level");
  Check(s.Enemies().size()==6 && !s.FrontierReady(),"Replay encounter missing");
  const auto save=s.Save();Simulation saved;Check(saved.Load(save) && !saved.ReplayActive() && saved.FrontierCompleted()==16,"Replay leaked into save");
  Check(s.ReplayActive(),"Saving ended live rehearsal");
  c.questAction=QuestAction::CompleteFrontier;s.Update(.001f,c);
  Check(s.FrontierCompleted()==16 && s.Experience()==xp,"Forged turn-in awarded replay XP");
  // Exercise real enemy damage, defence, reserve and a control contact at the
  // entrance. This diagnostic arrangement is independent of chapter balance.
  if(chapter==12) {
   auto& e=const_cast<Enemy&>(s.Enemies()[0]);const auto p=s.Player();
   e.position={p.x+1.3f,p.z};e.home=e.position;e.state=EnemyState::Pursuing;e.facing=-1.5707963f;e.windup=.02f;
   c={};c.ability=3;s.Update(.001f,c);Tick(s,.04f);
   Check(s.ReplayReport().taken+s.ReplayReport().absorbed>0 || id==ClassId::Scout,"Replay damage not counted");
   e.stunned=0;e.windup=2;e.attackCooldown=0;
   c={};c.selectActor=e.id;c.ability=10;s.Update(.001f,c);Tick(s,s.Spell(10).contact+.02f);
   Check(s.ReplayReport().damage>0 && s.ReplayReport().interrupts==1,"Replay contact report incorrect");
   e.windup=0;e.stunned=1000;
  }
  // Cancel first; no restore of consumed resources or old position.
  const float hp=s.Health();c={};c.endReplay=true;s.Update(.001f,c);
  Check(!s.ReplayActive() && s.Health()==hp && s.Gold()==gold && s.Potions()==potions && s.Experience()==xp,"Replay cancellation changed resources");
  Check(s.ReplayReportTime()>0 && !s.ReplayReport().victory,"Missing abandoned report");Roundtrip(s);
  // Actual objectives are completed through real E/channelling, never by
  // changing serialized mission bits. Deaths alone are arranged for isolation.
  Check(s.Load(campaign),"Restore completed campaign");c={};c.interact=true;s.Update(.001f,c);
  c={};c.questAction=QuestAction::BeginReplay;s.Update(.001f,c);
  if(chapter==14) {
   const auto p=WorldMap::ritualSite;WalkTo(s,{p.x,p.z+1.6f});
   c={};c.interact=true;s.Update(.001f,c);
   for(int wave=1;wave<=3;++wave) {ClearLive(s);Tick(s,4.05f);}
  } else {
   ClearLive(s);
   if(chapter==15) for(const auto p:WorldMap::courtSeals) {
    WalkTo(s,{p.x,p.z+1.6f});c={};c.interact=true;s.Update(.001f,c);Tick(s,2.55f);
   }
  }
  Check(s.FrontierReady(),"Replay objectives failed");const float elapsed=s.ReplayReport().seconds;Tick(s,.5f);
  Check(s.ReplayReport().seconds==elapsed,"Finished replay timer kept running");
  WalkTo(s,{npc.x+1.6f,npc.z});c={};c.interact=true;s.Update(.001f,c);
  Check(s.QuestPrimaryAction()==QuestAction::FinishReplay,"Missing replay debrief");
  c={};c.questAction=QuestAction::FinishReplay;s.Update(.001f,c);
  Check(!s.ReplayActive() && s.ReplayReport().victory && s.FrontierCompleted()==16 && s.Experience()==xp && s.Gold()==gold,"Replay awarded campaign rewards");
  Roundtrip(s);
 }
 auto novice=Veteran(ClassId::Warrior,12);Check(!novice.ReplayAvailable(QuestId::Watch13),"Unfinished chapter replay unlocked");
 auto active=Veteran(ClassId::Warrior,14);Talk(active,QuestAction::AcceptFrontier);
 Check(!active.ReplayAvailable(QuestId::Watch13),"Replay replaced active campaign");
 // Consumables are real costs, and a load during a live rehearsal must restore
 // the campaign without restoring the pre-rehearsal inventory or health.
 auto costly=Veteran(ClassId::Warrior,16);const auto npc=QuestLocation(QuestId::Watch13);
 Warp(costly,{npc.x+1.6f,npc.z});Command c;c.interact=true;costly.Update(.001f,c);
 c={};c.questAction=QuestAction::BeginReplay;costly.Update(.001f,c);
 auto& attacker=const_cast<Enemy&>(costly.Enemies()[0]);const auto p=costly.Player();
 attacker.position=attacker.home={p.x+1.3f,p.z};attacker.state=EnemyState::Pursuing;
 attacker.facing=-1.5707963f;attacker.windup=.01f;Tick(costly,.02f);
 const int beforePotions=costly.Potions();c={};c.usePotion=true;costly.Update(.001f,c);
 Check(costly.Potions()==beforePotions-1,"Replay potion cost missing");
 const auto live=costly.Save();const float hp=costly.Health();
 Check(!costly.Load("broken") && costly.ReplayActive(),"Invalid load destroyed rehearsal");
 Simulation disk;Check(disk.Load(live) && !disk.ReplayActive() && disk.Potions()==beforePotions-1 && disk.Health()==hp,"Saved rehearsal refunded costs");
 attacker.windup=.01f;attacker.attackCooldown=0;attacker.health=attacker.MaxHealth();
 for(int hit=0;hit<25 && costly.Health()>0;++hit) {attacker.windup=.01f;Tick(costly,.02f);}
 Check(costly.Health()==0 && costly.ReplayActive(),"Rehearsal was incorrectly nonlethal");
 c={};c.respawn=true;costly.Update(.001f,c);
 Check(!costly.ReplayActive() && costly.FrontierCompleted()==16 && costly.Potions()==beforePotions-1,"Defeat recovery lost campaign/costs");Roundtrip(costly);
}
void TacticalAreas() {
 for(int hz:{30,144}) {
  auto w=Veteran(ClassId::Warrior);CombatFixture(w);
  for(int i=0;i<3;++i) {auto& e=const_cast<Enemy&>(w.Enemies()[i]);e.position=e.home={-21.f+(i-1)*.75f,12};}
  Command c;c.selectActor=100;c.ability=8;w.Update(.001f,c);Tick(w,w.Spell(8).contact+.002f,hz);
  Check(w.Barrier()==18,"Warrior group guard missing or uncapped");
  auto a=Veteran(ClassId::Scout);CombatFixture(a);
  const Vec2 positions[]={{-21,13},{-21,17},{-19.8f,17},{-21,8},{-21,26},{-21.6f,17}};
  for(int i=0;i<6;++i) {auto& e=const_cast<Enemy&>(a.Enemies()[i]);e.position=e.home=positions[i];e.rooted=0;}
  c={};c.selectActor=100;c.ability=9;a.Update(.001f,c);Tick(a,a.Spell(9).contact+.002f,hz);
  const auto& e=a.Enemies();
  Check(e[0].health<e[0].MaxHealth() && e[1].health<e[1].MaxHealth(),"Piercing line missed target behind primary");
  Check(std::abs((e[0].MaxHealth()-e[0].health)*.5f-(e[1].MaxHealth()-e[1].health))<.01f,"Secondary piercing damage wrong");
  for(int i:{2,3,4,5})Check(e[i].health==e[i].MaxHealth(),"Piercing shot hit outside corridor/range");
  auto blocked=Veteran(ClassId::Scout);CombatFixture(blocked);Warp(blocked,{8,-2});
  for(auto& actor:const_cast<std::vector<Enemy>&>(blocked.Enemies())) actor.stunned=1000;
  auto& front=const_cast<Enemy&>(blocked.Enemies()[0]);front.position=front.home={8,-4.8f};
  auto& behind=const_cast<Enemy&>(blocked.Enemies()[1]);behind.position=behind.home={8,-11.6f};
  c={};c.selectActor=100;c.ability=9;blocked.Update(.001f,c);Tick(blocked,blocked.Spell(9).contact+.002f,hz);
  Check(front.health<front.MaxHealth() && behind.health==behind.MaxHealth(),"Piercing shot crossed a building");
  auto m=Veteran(ClassId::Mage);CombatFixture(m);c={};c.selectActor=100;c.ability=8;m.Update(.001f,c);
  Tick(m,m.Spell(8).contact+.01f,hz);Check(m.FrostFieldSeconds()>4.9f,"Frost field not created at contact");
  auto& entrant=const_cast<Enemy&>(m.Enemies()[2]);entrant.position=entrant.home={-19,12};entrant.rooted=entrant.snared=0;
  Tick(m,.1f,hz);Check(entrant.snared>0 && entrant.rooted==0,"Field did not slow late entrant or repeatedly rooted it");
  entrant.position=entrant.home={-15,12};Tick(m,.4f,hz);Check(entrant.snared==0,"Field slow stuck outside area");
  Simulation loaded;Check(loaded.Load(m.Save()) && loaded.FrostFieldSeconds()==0,"Transient field saved");
  Tick(m,5,hz);Check(m.FrostFieldSeconds()==0,"Frost field never expired");
 }
}
void SixActorWorkload(Renderer& r,const std::filesystem::path& root) {
 auto s=Veteran(ClassId::Warrior,15);Talk(s,QuestAction::AcceptFrontier);Warp(s,{87,-109});
 auto snapshot=s.Save();const auto at=snapshot.find('\n',snapshot.find("PROFILE "))+1;
 snapshot.replace(at,snapshot.find('\n',at)-at,"87 -109 "+std::to_string(s.MaxHealth())+" 0 3 90");
 Game g;g.Initialize();std::vector<double> cpu;unsigned warnings=0,hits=0,effectPeak=0;float travel=0;
 for(int frame=0;frame<390;++frame) {
  if(frame%130==0) {
   Check(g.RestoreSnapshot(snapshot),"Stress snapshot");
   // A diagnostic arrangement, not a normal campaign spawn: exercise six live
   // AI/animation controllers at once without teleporting during measurement.
   auto& actors=const_cast<std::vector<Enemy>&>(g.State().Enemies());
   for(int i=0;i<6;++i) {
    auto& e=actors[i];const float a=i*1.04719755f;
    e.position=e.home={87+std::sin(a)*4, -109+std::cos(a)*4};
    Check(g.State().CanStand(e.position,.45f),"Stress actor in obstacle");
    e.facing=a+3.14159265f;e.state=EnemyState::Pursuing;e.windup=0;
   }
  }
  Command c;c.movement={frame%120<60?.35f:-.35f,0};c.ability=frame%130==0?3:-1;
  const auto before=g.State().Player();const float hp=g.State().Health();
  const auto start=std::chrono::steady_clock::now();g.ExecuteCommand(c,1.f/60);
  g.SetCameraOrbit(.4f+frame*.004f,.6f,16);
  r.BeginFrame();g.Render(r);Check(r.EndFrame(false),"Six-actor native frame failed");
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  if(frame<90)continue;cpu.push_back(ms);
  for(const auto& e:g.State().Enemies()) if(e.windup>0)++warnings;
  hits+=g.State().Health()<hp;effectPeak=std::max(effectPeak,r.EffectVertices());
  const auto after=g.State().Player();travel+=std::hypot(after.x-before.x,after.z-before.z);
 }
 Check(warnings>0 && hits>0 && effectPeak>0 && travel>1,"Six-actor workload did not exercise combat and motion");
 std::ofstream report(root/"six-actors.txt");
 report<<"Six live enemies in a diagnostic court arrangement; real AI, movement, warnings and hits.\n"
   <<"Hidden 1280x720, 90 warmup / 300 measured frames, dt=1/60; setup excluded.\n"
   <<"CPU wall time includes update, preparation and submission; not GPU time or interactive FPS.\n";
 diagnostics::WriteDistribution(report,"CPU frame",cpu);
 report<<"\nCast observations / hit frames / effect peak / distance: "<<warnings<<" / "<<hits<<" / "<<effectPeak<<" / "<<travel<<'\n';
 r.BeginFrame();g.Render(r);Capture(r,root/"six-actors.png");
}
void Views() {
 const auto root=std::filesystem::current_path()/"southern-check";std::filesystem::create_directories(root);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Southern checks",false),"Hidden window");
 Renderer r;Check(r.Initialize(window.GetHandle(),1280,720),"Renderer");
 // Review the actual localized entry/exit interface, rather than a UI mockup.
 {
  auto veteran=Veteran(ClassId::Mage,16);const auto npc=QuestLocation(QuestId::Watch15);
  Warp(veteran,{npc.x+1.6f,npc.z});Game g;g.Initialize();Check(g.RestoreSnapshot(veteran.Save()),"Veteran view");
  g.SetCameraOrbit(-.65f,.42f,12);Command c;c.interact=true;g.ExecuteCommand(c,.001f);
  r.BeginFrame();g.Render(r);Capture(r,root/"replay-offer.png");
  c={};c.questAction=QuestAction::BeginReplay;g.ExecuteCommand(c,.001f);
  Check(g.State().ReplayActive(),"Native rehearsal did not start");
  r.BeginFrame();g.Render(r);Capture(r,root/"replay-active.png");
  ui::Interface screen;r.BeginFrame();screen.Render(r,g.State(),0,false);Check(r.EndFrame(false),"Replay button layout");
  auto click=screen.Pointer(195,144,1280,720,true,false);
  if(click.action==ui::Action::None)click=screen.Pointer(195,144,1280,720,false,false,false,true);
  Check(click.action==ui::Action::EndReplay,"Replay leave button has no working hit region");
  c={};c.endReplay=true;g.ExecuteCommand(c,.001f);
  r.BeginFrame();g.Render(r);Capture(r,root/"replay-report.png");
  for(int camp=0;camp<3;++camp) {
   const Vec2 p=camp==0?Vec2{-12,53}:camp==1?Vec2{-79,-66}:Vec2{79,-87};
   r.BeginFrame();g.Render(r,false);const float h=WorldMap::Height(p);
   r.SetCamera({p.x+6,h+4,p.z-9},{p.x,h+1.5f,p.z});
   Capture(r,root/("veteran-camp-"+std::to_string(camp)+".png"));
  }
  auto mage=Veteran(ClassId::Mage);CombatFixture(mage);Check(g.RestoreSnapshot(mage.Save()),"Field view");
  for(auto& enemy:const_cast<std::vector<Enemy>&>(g.State().Enemies())) {enemy.stunned=1000;enemy.home=enemy.position;}
  g.SetCameraOrbit(-.1f,.85f,12);c={};c.selectActor=100;c.ability=8;g.ExecuteCommand(c,.001f);
  for(int n=0;n<160;++n)g.ExecuteCommand({},.01f);
  Check(g.State().FrostFieldSeconds()>0,"Native frost field missing");
  r.BeginFrame();g.Render(r);Check(r.EffectVertices()<20000,"Persistent field exceeded effect budget");Capture(r,root/"frost-field.png");
 }
 for(int chapter:{13,14,15}) {
  auto s=Veteran(ClassId::Warrior,chapter);Talk(s,QuestAction::AcceptFrontier);Roundtrip(s);
  Game g;g.Initialize();Check(g.RestoreSnapshot(s.Save()),"Scene save");g.SetCameraOrbit(2.8f,.45f,13);
  r.BeginFrame();g.Render(r);Capture(r,root/("region-"+std::to_string(chapter)+".png"));
  r.BeginFrame();g.Render(r,false);r.SetCamera({0,100,-160},{0,7,-60});Capture(r,root/("overview-"+std::to_string(chapter)+".png"));
  if(chapter==14) {
   Warp(s,{-86,-89.4f});Command c;c.interact=true;s.Update(.001f,c);Clear(s);Tick(s,1.8f);
   Check(g.RestoreSnapshot(s.Save()),"Charging scene");r.BeginFrame();g.Render(r);Capture(r,root/"beacon-charge.png");
  }
  if(chapter==15) {
   std::ofstream(root/"southern.save")<<s.Save();
   for(auto panel:{ui::Panel::Map,ui::Panel::Journal,ui::Panel::Build}) {
    g.OpenPanel(panel);r.BeginFrame();g.Render(r);Capture(r,root/("panel-"+std::to_string(int(panel))+".png"));
   }
   Warp(s,{99,-118});Command select;select.selectActor=105;s.Update(.001f,select);
   Check(g.RestoreSnapshot(s.Save()),"Court ward view");g.OpenPanel(ui::Panel::None);g.ExecuteCommand(select,.001f);
   g.SetCameraOrbit(-.7f,.55f,15);r.BeginFrame();g.Render(r);Capture(r,root/"court-ward.png");
   DefeatSentry(s,0);DefeatSentry(s,1);const auto p=WorldMap::courtSeals[0];Warp(s,{p.x,p.z-1.6f});
   Check(g.RestoreSnapshot(s.Save()),"Seal channel view");Command begin;begin.interact=true;g.ExecuteCommand(begin,.001f);
   for(int n=0;n<90;++n)g.ExecuteCommand({},.01f);
   r.BeginFrame();g.Render(r);Capture(r,root/"court-channel.png");
  }
 }
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) for(int ability:{10,11}) {
  auto s=Veteran(id);CombatFixture(s);Game g;g.Initialize();
  // A normal save carries no transient reserve; visuals use the same events as
  // tested contacts while maintaining a stable camera for image review.
  auto view=Veteran(id);Warp(view,{-40,25});Check(g.RestoreSnapshot(view.Save()),"Effect landscape");
  r.BeginFrame();g.Render(r,false);r.SetCamera({-40,WorldMap::Height({-40,25})+7,16},{-40,WorldMap::Height({-40,25})+1,25});
  CombatVisualEvent e{CombatVisualKind::HeavyHit,{-40,25},WorldMap::Height({-40,25})+1.25f,0,100};
  e.school=id;e.ability=ability;e.heavy=true;e.source={-40,23};e.sourceHeight=WorldMap::Height(e.source);
  classfx::Impact(r,e,.12f);Check(r.EffectVertices()>0 && r.EffectVertices()<2500,"New effect vertex budget");
  Capture(r,root/("effect-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png"));
 }
 SixActorWorkload(r,root);
 Check(r.ValidateDebugMessages(),"Southern D3D validation");
}
}
int main() try {Regions();Campaign();Spells();Siege();TacticalContacts();ReplayEncounters();TacticalAreas();Views();std::cout<<"PASS: southern routes, waves, court siege/migration, veteran replays, tactical areas and native scenes\n";}
catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
