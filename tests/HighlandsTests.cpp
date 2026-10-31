#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdlib>
namespace {
using namespace rpg;
void Check(bool ok,const char* message) {if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
std::string Record(std::string s,const std::string& key,const std::string& line) {
 auto at=s.find(key);Check(at!=s.npos,"Fixture record missing");
 const bool newline=key.front()=='\n';if(newline)++at;
 s.replace(at,s.find('\n',at)-at,newline?line.substr(1):line);return s;
}
void Warp(Simulation& s,Vec2 p) {
 auto data=s.Save();const auto at=data.find('\n',data.find("PROFILE "))+1,end=data.find('\n',at);
 std::istringstream row(data.substr(at,end-at));float x,z,h,yaw;int quest,gold;row>>x>>z>>h>>yaw>>quest>>gold;
 std::ostringstream replacement;replacement<<p.x<<' '<<p.z<<' '<<h<<' '<<yaw<<' '<<quest<<' '<<gold;
 data.replace(at,end-at,replacement.str());Check(s.Load(data),"Relocation fixture rejected");
}
void Roundtrip(const Simulation& s) {Simulation copy;Check(copy.Load(s.Save()) && copy.Save()==s.Save(),"Durable state did not round-trip");}
Simulation Veteran(ClassId id,int completed) {
 Simulation s;Check(s.Load("MEDITERRANEAN 3\n0 -10 100 0 3 90\n5 1 1 0\n4\n0 0 0 0\n100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Veteran fixture");
 auto data=Record(s.Save(),"PROFILE ","PROFILE "+std::to_string(int(id))+" \"Highlander\" "+std::to_string(completed)+" 0 3");
 if(completed>=9)data=Record(data,"SURVEY ","SURVEY 7");
 Check(s.Load(data),"Campaign fixture");return s;
}
void Talk(Simulation& s,QuestAction action) {
 const auto p=QuestLocation(s.CurrentQuest());const Vec2 right{p.x+1.6f,p.z};Warp(s,s.CanStand(right)?right:Vec2{p.x-1.6f,p.z});Command c;c.interact=true;s.Update(.001f,c);
 Check(s.QuestPrimaryAction()==action,"Expedition dialogue wrong action");c={};c.questAction=action;s.Update(.001f,c);
}
void Clear(Simulation& s) {
 auto data=s.Save();for(const auto& e:s.Enemies()) {
  std::ostringstream row;row<<e.id<<' '<<e.home.x<<' '<<e.home.z<<" 0 0 1 3 0 0";
  data=Record(data,"\n"+std::to_string(e.id)+" ","\n"+row.str());
 }
 Check(s.Load(data),"Cleared encounter fixture");
}
void Tick(Simulation& s,float duration,int hz) {while(duration>1e-6f){const float dt=std::min(duration,1.f/hz);s.Update(dt,{});duration-=dt;}}
void TerrainChecks() {
 WorldMap map;Check(map.Height({0,-10})==0 && map.Height({-48,26})>6 && map.Height({0,67})>10,"Missing hills or changed legacy settlement floor");
 for(const auto& npc:WorldMap::residents) {Check(map.CanStand(npc.position,.4f),"Resident blocked");Check(!map.FindPath(WorldMap::spawn,npc.position).empty(),"Resident unreachable");}
 for(const auto& t:map.Trails()) Check(map.CanTravel(t.from,t.to,.45f),"A road runs into solid geometry");
 for(int hz:{30,60,144}) for(float direction:{-1.f,1.f}) {
  Simulation s;Warp(s,{-40,25});const float origin=s.WorldHeight();float elapsed=0;bool landed=false;
  for(int n=0;n<hz*2;++n) {
   Command c;c.movement={direction,0};c.jump=n==0;c.sprint=true;const float dt=1.f/hz;s.Update(dt,c);elapsed+=dt;
   Check(s.WorldHeight()>=WorldMap::Height(s.Player())-.0001f,"Player fell below heightfield");
   if(!s.Grounded() && elapsed<.4f) {
    const float ballistic=origin+motion::jumpSpeed*elapsed-.5f*motion::gravity*elapsed*elapsed;
    Check(std::abs(s.WorldHeight()-ballistic)<.002f,"Horizontal terrain travel altered ballistic altitude");
   }
   if(n>3 && s.Grounded()) {landed=true;break;}
  }
  Check(landed,"Jump did not land on slope");Roundtrip(s);
  const auto p=s.Player();Check(std::abs(s.WorldHeight()-map.Height(p))<.0001f,"Loaded actor not on ground");
 }
 Simulation camera;Warp(camera,{-40,25});const float y=camera.WorldHeight()+1.4f;
 const float distance=camera.CameraDistance(camera.Player(),y,{-1,0},-.4f,12);
 Check(distance<12 && distance>=.25f,"Uphill camera failed to retract");
 animation::FootPlacement feet;animation::Sample sample;Vec2 p{-40,25};feet.Reset(p,0);feet.Update(.016f,p,0,true,true,sample);
 for(const auto& foot:sample.feet) Check(std::abs(foot.y+map.Height(p)-map.Height({p.x+foot.x,p.z+foot.z})-.17f)<.0001f,"Foot anchor ignores slope");
}
void Disciplines() {
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) {
  Simulation novice;Check(novice.CreateCharacter(id,"Novice"),"Fresh class");for(int ability:{8,9}) {
   Command c;c.ability=ability;novice.Update(.01f,c);Check(!novice.Attacking() && novice.Cooldown(ability)==0,"Novice used a locked discipline");
  }
  for(int hz:{30,144}) for(int ability:{8,9}) {
   auto s=Veteran(id,7);Talk(s,QuestAction::AcceptFrontier);Warp(s,{-21,10});auto data=s.Save();
   data=Record(data,"\n100 ","\n100 -21 12 200 1 0 0 0 0");
   data=Record(data,"\n101 ","\n101 -20.2 12.3 200 1 0 0 0 0");
   data=Record(data,"\n102 ","\n102 -28 20 200 1 0 0 0 0");Check(s.Load(data),"Advanced combat fixture");
   Command c;c.selectActor=100;c.ability=ability;s.Update(.001f,c);Check(s.Attacking(),"Advanced cast rejected");
   const float contact=s.Spell(ability).contact;Tick(s,contact-.002f,hz);Check(s.Enemies()[0].health==200,"Damage before contact");
   Tick(s,.003f,hz);Check(s.Enemies()[0].health<200,"No damage at contact");
   const bool area=ability==8 || id==ClassId::Mage;
   Check((s.Enemies()[1].health<200)==area,"Wrong secondary target rule");
   Check(s.Enemies()[2].health==200,"Area attack hit an out-of-area enemy");
   if(ability==8 && id==ClassId::Mage) Check(s.Enemies()[0].rooted>1.9f,"Frost bloom did not root");
   if(ability==9 && id!=ClassId::Mage) Check(s.Enemies()[0].exposed>3.9f,"Master technique did not expose");
   Roundtrip(s);Simulation loaded;Check(loaded.Load(s.Save()) && loaded.Cooldown(ability)>0,"Advanced cooldown lost on reload");
  }
 }
 auto caster=Veteran(ClassId::Mage,7);Talk(caster,QuestAction::AcceptFrontier);Warp(caster,{-23,13});Command c;c.selectActor=100;c.ability=9;caster.Update(.001f,c);
 Check(caster.Attacking(),"Cancellation fixture cast");c={};c.movement={1,0};caster.Update(.01f,c);
 Check(!caster.Attacking() && caster.Cooldown(9)>0,"Moving did not cancel preparation or refunded cooldown");
 ui::Interface ui;ui.AssignHotbar(9,18,ClassId::Mage);ui::Interface copy;
 Check(copy.LoadHotbars(ui.SaveHotbars()) && copy.HotbarSpell(18,ClassId::Mage)==9,"Advanced binding persistence");
}
void Expeditions(Renderer& renderer,const std::filesystem::path& root) {
 auto s=Veteran(ClassId::Warrior,10);const int xp=s.Experience();
 for(int chapter=10;chapter<13;++chapter) {
  Talk(s,QuestAction::AcceptFrontier);Check(s.Enemies().size()==6 && !s.FrontierReady(),"New camp population/readiness");Roundtrip(s);
  for(const auto& enemy:s.Enemies()) Check(s.CanStand(enemy.home,.45f),"Enemy home inside prop");
  {
   Game camp;camp.Initialize();Check(camp.RestoreSnapshot(s.Save()),"Live camp snapshot");
   renderer.BeginFrame();camp.Render(renderer,false);renderer.SetCamera({0,80,-90},{0,5,15});
   Check(renderer.Capture((root/("camp-overview-"+std::to_string(chapter)+".png")).wstring()) && renderer.EndFrame(false),"Expanded scene exceeded the renderer capacity");
  }
  if(chapter==10) {
   const auto p=s.ExpeditionSite(0);Warp(s,{p.x,p.z-1.3f});Command read;read.interact=true;s.Update(.001f,read);
   Check(s.ExpeditionMask()==0,"Guarded signal accepted");
  }
  // Fixture-cleared enemies isolate objective/save validation from combat,
  // which is exercised above. Archive collection also supports live enemies.
  if(chapter!=11) Clear(s);
  if(chapter<12) for(int i=0;i<3;++i) {
   const auto p=s.ExpeditionSite(i);Warp(s,{p.x,p.z-1.3f});Command read;read.interact=true;s.Update(.001f,read);
   Check(s.ExpeditionMask()==(1u<<(i+1))-1,"Expedition interaction lost progress");Roundtrip(s);
   const int before=s.Experience();s.Update(.001f,read);Check(s.Experience()==before,"Repeated stone grants XP");
  }
  Check(s.FrontierReady(),"Expedition objective cannot finish");Talk(s,QuestAction::CompleteFrontier);Roundtrip(s);
  Check(s.FrontierCompleted()==chapter+1,"Quest completion lost");
 }
 Check(s.Experience()==xp+750 && s.Level()==10,"Campaign reward/cap accounting");
 auto bad=Record(s.Save(),"EXPEDITION ","EXPEDITION 8 0 0");Check(!s.Load(bad),"Invalid objective mask accepted");
 for(const auto point:{Vec2{-46,25},Vec2{44,37},Vec2{-8,61}}) {
  Warp(s,point);Game game;game.Initialize();Check(game.RestoreSnapshot(s.Save()),"Highland render snapshot");game.SetCameraOrbit(2.7f,.45f,12);
  renderer.BeginFrame();game.Render(renderer);Check(renderer.Capture((root/("hill-"+std::to_string(int(point.x))+".png")).wstring()) && renderer.EndFrame(false),"Highland render");
 }
 Game game;game.Initialize();Check(game.RestoreSnapshot(s.Save()),"Atlas fixture");
 for(auto panel:{ui::Panel::Map,ui::Panel::Journal,ui::Panel::Build}) {
  game.OpenPanel(panel);renderer.BeginFrame();game.Render(renderer);Check(renderer.Capture((root/("panel-"+std::to_string(int(panel))+".png")).wstring()) && renderer.EndFrame(false),"Expanded UI render");
 }
 std::ofstream(root/"highlands.save")<<s.Save();
}
void CombatCaptures(Renderer& renderer,const std::filesystem::path& root) {
 for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) for(int ability:{8,9}) {
  auto s=Veteran(id,10);Talk(s,QuestAction::AcceptFrontier);Warp(s,{-38,46});
  Game game;game.Initialize();Check(game.RestoreSnapshot(s.Save()),"Effect fixture");game.SetCameraOrbit(-.5f,.55f,10);
  Command cast;cast.selectActor=104;cast.ability=ability;game.ExecuteCommand(cast,.001f);
  Check(game.State().Attacking(),"Visual spell did not commit");
  float left=s.Spell(ability).contact+.18f;
  while(left>1e-6f) {const float dt=std::min(left,.01f);game.ExecuteCommand({},dt);left-=dt;}
  Check(game.Effects().BurstCount()>0,"Advanced spell produced no effect bursts");
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/("combat-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png")).wstring()) && renderer.EndFrame(false),"Class effect capture");
 }
}
}
int main() try {
 TerrainChecks();Disciplines();
 const auto root=std::filesystem::current_path()/"highlands-check";std::filesystem::create_directories(root);
 Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Highlands checks",false),"Hidden window");
 Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer");Expeditions(renderer,root);CombatCaptures(renderer,root);
 Check(renderer.ValidateDebugMessages(),"Highland D3D errors");
 std::cout<<"PASS: terrain ballistics/feet/camera/routes, six timed class skills, saves, expeditions and native captures\n";
} catch(const std::exception& error) {
 std::cerr<<"Highlands initialization/render failure: "<<error.what()<<'\n';return 1;
}
