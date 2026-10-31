#include "game/Game.h"
#include "game/BuildLayout.h"
#include "game/ClassEffects.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
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
 data.replace(at,data.find('\n',at)-at,line.str());Check(s.Load(data),"Southern relocation rejected");
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
 for(const auto& npc:WorldMap::residents) {
  Check(map.CanStand(npc.position,.45f),"NPC inside prop");
  Check(!map.FindPath(WorldMap::spawn,npc.position).empty(),"NPC unreachable");
 }
 for(const auto& t:map.Trails()) if(!map.CanTravel(t.from,t.to,.45f)) {
  std::cerr<<"Blocked trail "<<t.from.x<<','<<t.from.z<<" -> "<<t.to.x<<','<<t.to.z<<'\n';Check(false,"Blocked route");
 }
 Simulation camera;const Vec2 besideBeacon{-86,-89.4f};
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
void Capture(Renderer& r,const std::filesystem::path& path) {Check(r.Capture(path.wstring()) && r.EndFrame(false),"Native scene capture/budget");}
void Views() {
 const auto root=std::filesystem::current_path()/"southern-check";std::filesystem::create_directories(root);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Southern checks",false),"Hidden window");
 Renderer r;Check(r.Initialize(window.GetHandle(),1280,720),"Renderer");
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
 Check(r.ValidateDebugMessages(),"Southern D3D validation");
}
}
int main() try {Regions();Campaign();Spells();Views();std::cout<<"PASS: southern routes, three quest modes, waves/save migration, six class disciplines and native scenes\n";}
catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
