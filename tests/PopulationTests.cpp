#include "game/Game.h"
#include "game/Hunts.h"
#include "game/EncounterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <algorithm>
namespace {
using namespace rpg;
void Check(bool ok,const char* text){if(!ok){std::cerr<<text<<'\n';std::exit(1);}}
void Tick(Simulation& s,float duration,int hz=60){while(duration>1e-6f){const float dt=std::min(duration,1.f/hz);s.Update(dt,{});duration-=dt;}}
void Warp(Simulation& s,Vec2 p) {
  auto data=s.Save();const auto at=data.find('\n',data.find("PROFILE "))+1;
  std::istringstream row(data.substr(at));float x,z,h,f;int q,g;row>>x>>z>>h>>f>>q>>g;
  std::ostringstream out;out<<p.x<<' '<<p.z<<' '<<s.MaxHealth()<<' '<<f<<' '<<q<<' '<<g;
  data.replace(at,data.find('\n',at)-at,out.str());
  if(!s.Load(data)){std::cerr<<"Warp "<<p.x<<','<<p.z<<" stand "<<s.CanStand(p)<<'\n';Check(false,"Population relocation rejected");}
}
void Roundtrip(const Simulation& s){Simulation copy;Check(copy.Load(s.Save()) && copy.Save()==s.Save(),"Wilderness save is not stable");}
void Talk(Simulation& s,int district,QuestAction action) {
  Warp(s,WorldMap::residents[9+district].position);Command c;c.interact=true;s.Update(.001f,c);
  Check(s.QuestDialogue()==QuestId(int(QuestId::Hunt1)+district),"Wrong local speaker");
  Check(s.QuestPrimaryAction()==action,"Wrong local quest action");c={};c.questAction=action;s.Update(.001f,c);
}
void Kill(Simulation& s,int index,int hz=60) {
  const auto home=s.Beasts()[index].position;Vec2 approach{};bool found=false;
  for(int n=0;n<16;++n) {
    const float a=n*.392699f;const Vec2 p{home.x+std::sin(a)*1.7f,home.z+std::cos(a)*1.7f};
    if(s.CanStand(p) && s.Map().CanTravel(p,home,.05f)){approach=p;found=true;break;}
  }
  Check(found,"No melee approach to beast");Warp(s,approach);
  for(auto& other:const_cast<std::vector<Enemy>&>(s.Beasts()))other.attackCooldown=4;
  auto& e=const_cast<Enemy&>(s.Beasts()[index]);e.health=1;
  Command c;c.selectActor=e.id;s.Update(.001f,c);Check(s.TargetActor() && s.TargetActor()->id==e.id,"Beast cannot be selected");
  c={};c.ability=0;s.Update(.001f,c);Tick(s,1.f,hz);
  Check(s.Beasts()[index].health==0 && s.Beasts()[index].state==EnemyState::Dead,"Real weapon contact did not kill beast");
}
void Layout() {
  Simulation s;const auto& map=s.Map();int trees=0,props=0,buildings=0;
  for(const auto& o:map.Obstacles())if(o.introducedVersion==22) {
    trees+=o.kind==PropKind::Tree;props+=o.kind==PropKind::Dressing;buildings+=o.kind==PropKind::Building;
  }
  std::cout<<"Population: "<<s.Beasts().size()<<" beasts, "<<WorldMap::residents.size()+2<<" NPCs, "
    <<population::Props().size()<<" authored prop placements, "<<trees<<" added trees, "<<buildings<<" added buildings, "<<props<<" prop colliders\n";
  Check(s.Beasts().size()==36 && population::Props().size()>=180 && trees>=80 && buildings==12,"Population density missing");
  for(const auto& n:WorldMap::residents) {
    if(!map.CanStand(n.position,.45f)){std::cerr<<n.name<<'\n';Check(false,"NPC intersects scenery");}
    Check(!map.FindPath(WorldMap::spawn,n.position).empty(),"Resident unreachable");
  }
  for(const auto& e:s.Beasts()) {
    Check(map.CanStand(e.home,.55f),"Wildlife home intersects scenery");
    Check(!map.FindPath(WorldMap::residents[9+(e.id-400)/6].position,e.home).empty(),"Hunting area unreachable");
  }
  for(const auto& t:map.Trails())Check(map.CanTravel(t.from,t.to,.45f),"Road traverses a prop");
  Roundtrip(s);
  const auto before=s.Save();auto invalid=before;invalid.replace(invalid.find("WILDERNESS "),10,"WILDERNESS 1099511627776");
  Check(!s.Load(invalid) && s.Save()==before,"Invalid wilderness load must be atomic");
  auto old=before;old.erase(old.find("WILDERNESS "));old.replace(14,2,"21");
  Check(s.Load(old) && s.Beasts().size()==36 && s.HuntState(0)==0,"v21 population migration");
  const auto at=old.find('\n',old.find("PROFILE "))+1;
  old.replace(at,old.find('\n',at)-at,"-32 -35 100 0 0 0");
  Check(s.Load(old) && s.CanStand(s.Player()),"Old save covered by new farm must migrate to a clear edge");
}
void Commissions() {
  Simulation s;
  for(int d=0;d<6;++d) {
    const int initialGold=s.Gold();Talk(s,d,QuestAction::AcceptHunt);
    Check(s.HuntState(d)==1 && !s.HuntReady(d) && s.TrackedHunt()==d,"Commission acceptance");
    Command stale;stale.questAction=QuestAction::CompleteHunt;s.Update(.001f,stale);
    Check(s.HuntState(d)==1 && s.Gold()==initialGold,"Premature commission reward");
    if(HuntGather(d))for(int i=0;i<3;++i) {
      const auto p=population::Cache(d,i);bool found=false;
      for(int n=0;n<16 && !found;++n) {
        const float a=n*.392699f;const Vec2 candidate{p.x+std::sin(a)*1.7f,p.z+std::cos(a)*1.7f};
        if(!s.CanStand(candidate))continue;Warp(s,candidate);Command c;c.interact=true;s.Update(.001f,c);found=s.HuntProgress(d)==i+1;
      }
      Check(found,"Commission cache inaccessible");Roundtrip(s);
    } else {
      const int count=(d==3||d==4)?2:4;
      for(int i=0;i<count;++i){Kill(s,d*6+i);Tick(s,1.1f);Roundtrip(s);}
      if(d==3||d==4){Check(!s.HuntReady(d),"Leader requirement bypassed");Kill(s,d*6+5);}
    }
    Check(s.HuntReady(d),"Commission objective did not finish");
    const int xp=s.Experience();Talk(s,d,QuestAction::CompleteHunt);
    Check(s.HuntState(d)==2 && s.Gold()==initialGold+huntQuests[d].gold && s.Experience()==xp+huntQuests[d].experience,"Commission reward mismatch");
    s.Update(.001f,stale);Check(s.Gold()==initialGold+huntQuests[d].gold,"Repeated reward");Roundtrip(s);
  }
  Check(s.Gold()==290 && s.Experience()>1200,"Combined commission ledger lost cumulative rewards");
  Warp(s,{3,-10});const auto earned=s.Gold();Command purchase;purchase.purchase=0;s.Update(.01f,purchase);
  Check(s.Gold()==earned-6,"Commission earnings cannot be spent at the supply merchant");Roundtrip(s);
  Simulation before;Kill(before,0);Talk(before,0,QuestAction::AcceptHunt);
  Check(before.HuntProgress(0)==0,"Pre-acceptance kills counted");
  Warp(before,WorldMap::spawn);Tick(before,122);Check(before.Beasts()[0].health>0,"Distant wildlife did not respawn");
  const int xp=before.Experience();Kill(before,0);Check(before.Experience()==xp && before.HuntProgress(0)==1,"Respawn farm or missing new commission kill");
  Roundtrip(before);
  for(const auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout}) {
    Simulation s;Check(s.CreateCharacter(id,"Hunter"),"Hunt class setup");Kill(s,0);
    Check(s.QuestState()==Quest::Available && s.Experience()==10,"Beast kill changed the main patrol or first-kill XP");
    Command loot;loot.interact=true;s.Update(.001f,loot);Check(s.Gold()==2,"Beast remains did not award first spoils");
    s.Update(.001f,loot);Check(s.Gold()==2,"Duplicate corpse reward");Roundtrip(s);
    const auto home=s.Beasts()[0].home;Vec2 nearby{};bool clear=false;
    for(int n=0;n<16&&!clear;++n) {const float a=n*.392699f;nearby={home.x+std::sin(a)*20,home.z+std::cos(a)*20};clear=s.CanStand(nearby);}
    Check(clear,"No safe respawn observation point");Warp(s,nearby);
    Tick(s,121);Check(s.Beasts()[0].health==0,"Respawn appeared beside the player");
    Warp(s,WorldMap::spawn);Tick(s,.1f);Check(s.Beasts()[0].health>0,"Far respawn failed after waiting nearby");
    Kill(s,0);s.Update(.001f,loot);Check(s.Gold()==2 && s.Experience()==10,"Repeat creature farm changed finite rewards");Roundtrip(s);
  }
}
void Combat() {
  for(int hz:{30,144})for(int index:{0,6}) {
    Simulation s;const auto home=s.Beasts()[index].home;
    Warp(s,{home.x,home.z+(index==0?3.4f:2.f)});
    auto& e=const_cast<Enemy&>(s.Beasts()[index]);e.state=EnemyState::Pursuing;e.facing=0;e.windup=e.WindupDuration();
    for(auto& other:const_cast<std::vector<Enemy>&>(s.Beasts()))if(other.id!=e.id)other.attackCooldown=4;
    const auto before=e.position;const float health=s.Health();
    if(index==0) {
      const auto threats=ui::ReadIncomingAttacks(s);bool warned=false;
      for(unsigned i=0;i<threats.count;++i)warned|=threats.rows[i].enemy->id==e.id && threats.rows[i].threatens;
      Check(warned,"Boar rush HUD incorrectly reports the charge lane safe");
    }
    Tick(s,e.WindupDuration()-.08f,hz);Check(s.Health()==health,"Beast hit before committed contact");
    Tick(s,.1f,hz);Check(s.Health()<health,"Beast contact did not damage player");
    if(e.kind==EnemyKind::Boar)Check(std::hypot(e.position.x-before.x,e.position.z-before.z)>1,"Boar charge lacks physical travel");
    Warp(s,WorldMap::spawn);Tick(s,15,hz);Check(s.Beasts()[index].state==EnemyState::Idle && s.Beasts()[index].health==s.Beasts()[index].MaxHealth(),"Wildlife leash failed");
  }
}
void Counterplay() {
  for(int mode=0;mode<4;++mode) {
    Simulation s;const auto home=s.Beasts()[0].home;
    Warp(s,{home.x+(mode==0?3.f:0.f),home.z+(mode==3?4.5f:3.4f)});
    auto& e=const_cast<Enemy&>(s.Beasts()[0]);e.state=EnemyState::Pursuing;e.facing=0;e.windup=e.WindupDuration();
    for(auto& other:const_cast<std::vector<Enemy>&>(s.Beasts()))if(other.id!=e.id)other.attackCooldown=4;
    if(mode==1)e.rooted=2;
    if(mode==2)e.stunned=2;
    if(mode==3)e.snared=2;
    const auto before=e.position;const float health=s.Health();Tick(s,1.5f,144);
    Check(s.Health()==health,"Sidestep or control failed to prevent the distant charge contact");
    Check(std::abs(e.position.x-before.x)<.001f,"Committed rush bent sideways");
    if(mode==1 || mode==2)Check(std::hypot(e.position.x-before.x,e.position.z-before.z)<.001f,"Root/stun allowed charge movement");
    if(mode==3)Check(e.position.z-before.z<1.3f,"Snare did not reduce rush speed");
  }
  {
    Simulation s;const auto yard=population::districts[0].yard;
    const Vec2 well{yard.x+6,yard.z+1},start{well.x,well.z-2.4f},victim{well.x,well.z+2.6f};
    Check(s.CanStand(start) && s.CanStand(victim),"Charge wall fixture blocked");Warp(s,victim);
    auto& e=const_cast<Enemy&>(s.Beasts()[0]);e.position=e.home=start;e.state=EnemyState::Pursuing;e.facing=0;e.windup=e.WindupDuration();
    const float health=s.Health();Tick(s,1.5f);
    Check(s.Health()==health && s.Map().CanStand(e.position,.45f) && e.position.z<well.z-1.2f && std::abs(e.position.x-start.x)<.001f,
      "Charge crossed or steered around solid authored scenery");
  }
}
void CreaturePoses() {
  using namespace authored;
  for(auto asset:{Asset::Wolf,Asset::Boar}) {
    Gesture bite;bite.jaw=1;Gesture dead;dead.collapse=1;
    const auto rest=Pose(asset,0),strike=Pose(asset,0,0,.9f,0,bite),fallen=Pose(asset,0,0,0,0,dead);
    float change=0,low=100,high=-100;
    for(unsigned lod=0;lod<3;++lod)for(const auto& vertex:Collection()[unsigned(asset)*3+lod].geometry.vertices) {
      const auto weights=Influence(asset,vertex.position);
      const auto a=visual::SkinVertex(vertex,weights,rest),b=visual::SkinVertex(vertex,weights,strike),c=visual::SkinVertex(vertex,weights,fallen);
      change+=std::abs(a.position.y-b.position.y)+std::abs(a.position.z-b.position.z);
      low=std::min(low,c.position.y);high=std::max(high,c.position.y);
    }
    Check(change>1,"Combat creature mesh remains in its idle pose");
    Check(low>=-.001f && low<.04f && high<1,"Defeated beast stands upright or sinks below the floor");
  }
}
void Views() {
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Population COM");
  Win32Window window;Check(window.Create(GetModuleHandle(nullptr),1280,720,L"Population review",false),"Population window");
  Renderer r;Check(r.Initialize(window.GetHandle(),1280,720),"Population renderer");
  std::filesystem::create_directories("population-check");
  for(int d=0;d<6;++d) {
    Simulation s;Talk(s,d,QuestAction::AcceptHunt);s.CloseQuestDialogue();
    Game game;game.Initialize();Check(game.RestoreSnapshot(s.Save()),"Population game fixture");game.OpenPanel(ui::Panel::None);
    game.SetCameraOrbit(-.25f,.55f,17);
    r.BeginFrame();game.Render(r);
    const auto path=std::filesystem::path("population-check")/("district-"+std::to_string(d)+".png");
    Check(r.Capture(path.wstring()) && r.EndFrame(false) && r.ValidateDebugMessages(),"Population district render");
    r.BeginFrame();game.Render(r,false);const auto yard=population::districts[d].yard;const float h=WorldMap::Height(yard);
    r.SetCamera({yard.x+19,h+14,yard.z+23},{yard.x-3,h+1,yard.z});
    Check(r.Capture((std::filesystem::path("population-check")/("yard-"+std::to_string(d)+".png")).wstring()) && r.EndFrame(false),"Yard overview capture");
    if(d==0) {
      ui::Interface panel;panel.panel=ui::Panel::Journal;panel.journalSelection=18;
      r.BeginFrame();game.Render(r,false);panel.Render(r,s,0,false);Check(r.Capture(L"population-check/commissions.png") && r.EndFrame(false),"Commission journal capture");
    }
    std::ofstream("population-check/district-"+std::to_string(d)+".save")<<s.Save();
  }
  for(int index:{0,6}) {
    Simulation s;const auto home=s.Beasts()[index].home;Warp(s,{home.x,home.z+(index?2.f:3.4f)});
    auto& e=const_cast<Enemy&>(s.Beasts()[index]);e.state=EnemyState::Pursuing;e.facing=0;e.windup=e.WindupDuration();
    Game game;game.Initialize();Check(game.RestoreSnapshot(s.Save()),"Beast view setup");game.SetCameraOrbit(2.7f,.55f,9);
    Command c;c.selectActor=e.id;game.ExecuteCommand(c,.01f);
    ui::Interface hud;
    const auto view=[&] {
      r.BeginFrame();game.Render(r,false);
      const auto p=game.State().Player(),beast=game.State().Beasts()[index].position;
      const float x=(p.x+beast.x)*.5f,z=(p.z+beast.z)*.5f,y=WorldMap::Height({x,z});
      r.SetCamera({x+7,y+5,z+6},{x,y+.8f,z});hud.Render(r,game.State(),0,false);
    };
    view();
    Check(r.Capture(index?L"population-check/wolf.png":L"population-check/boar.png") && r.EndFrame(false) && r.ValidateDebugMessages(),"Beast target/portrait/warning capture");
    for(int i=0;i<int(e.WindupDuration()*60)+2;++i)game.ExecuteCommand({},1.f/60);
    view();
    Check(r.Capture(index?L"population-check/wolf-contact.png":L"population-check/boar-contact.png") && r.EndFrame(false),"Beast contact capture");
    Simulation fallen=game.State();Kill(fallen,index);Check(game.RestoreSnapshot(fallen.Save()),"Beast corpse setup");
    view();Check(r.Capture(index?L"population-check/wolf-defeated.png":L"population-check/boar-defeated.png") && r.EndFrame(false),"Settled beast corpse capture");
  }
  // Measure the populated habitat, not an empty village. Input/render time is
  // CPU submission, not a GPU tail or interactive-FPS guarantee.
  Simulation workload;Vec2 observation{};bool clear=false;
  for(int n=0;n<32 && !clear;++n) {
    const float a=n*.1963495f;const auto h=population::districts[0].habitat;
    observation={h.x+std::sin(a)*18,h.z+std::cos(a)*18};clear=workload.CanStand(observation);
  }
  Check(clear,"No populated workload observation point");Warp(workload,observation);
  Game game;game.Initialize();Check(game.RestoreSnapshot(workload.Save()),"Population workload setup");
  std::vector<double> timings;unsigned calls=0,triangles=0;
  for(int frame=0;frame<240;++frame) {
    const auto start=std::chrono::steady_clock::now();
    Command command;command.respawn=game.State().Health()<=0;game.ExecuteCommand(command,1.f/60);
    game.SetCameraOrbit(.4f+frame*.004f,.6f,15);r.BeginFrame();game.Render(r);Check(r.EndFrame(false),"Populated workload render");
    if(frame>=60)timings.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    calls=std::max(calls,r.LastDrawCalls());triangles=std::max(triangles,r.Triangles());
  }
  std::sort(timings.begin(),timings.end());
  std::ofstream metrics("population-check/workload.txt");metrics<<"CPU update/render/submission, 180 samples after 60 warm-up frames\np50 "<<timings[90]<<" ms\np95 "<<timings[171]<<" ms\nmax "<<timings.back()<<" ms\npeak draws "<<calls<<"\npeak triangles "<<triangles<<'\n';
}
}
int main(){Layout();Commissions();Combat();Counterplay();CreaturePoses();Views();std::cout<<"PASS: populated districts, independent commissions, real beast contacts, persistence and native views.\n";}
