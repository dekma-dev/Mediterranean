#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace {
using namespace rpg;
void Check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
std::string Record(std::string s,const std::string& key,const std::string& row) {
  const auto p=s.find(key);Check(p!=s.npos,"Missing fixture record");s.replace(p,s.find('\n',p)-p,row);return s;
}
void Roundtrip(const Simulation& s) {Simulation loaded;Check(loaded.Load(s.Save()) && loaded.Save()==s.Save(),"Expansion save round-trip");}
void Walk(Simulation& s,Vec2 goal) {
  const auto route=s.Map().FindPath(s.Player(),goal);Check(!route.empty(),"Expansion destination unreachable");
  for(auto point:route) for(int n=0;n<3000;++n) {
    const auto p=s.Player();if(std::hypot(p.x-point.x,p.z-point.z)<.15f) break;
    Command c;c.sprint=true;c.movement={point.x-p.x,point.z-p.z};s.Update(.05f,c);
    Check(s.Health()>0,"Safe expansion route crossed a hostile camp");
  }
  Check(std::hypot(s.Player().x-goal.x,s.Player().z-goal.z)<.2f,"Expansion route failed to arrive");
  s.StopMovement();
}
void Speak(Simulation& s,QuestAction action) {
  Command talk;talk.interact=true;s.Update(.01f,talk);
  Check(s.QuestPrimaryAction()==action,"Wrong quest action at expansion speaker");
}
void Fight(Simulation& s) {
  int deaths=0;
  for(int frame=0;frame<24000 && s.DefeatedCount()<3;++frame) {
    Command c;
    if(s.Health()<=0) {c.respawn=true;Check(++deaths<20,"Expansion combat cannot be completed");}
    else {
      if(!s.TargetActor() || s.TargetActor()->health<=0) c.nextTarget=true;
      const auto target=s.TargetActor();const auto p=s.Player();
      const auto goal=target?target->position:s.ObjectivePosition();
      if(std::hypot(goal.x-p.x,goal.z-p.z)>2) {
        const auto route=s.Map().FindPath(p,goal);const auto next=route.empty()?goal:route.front();
        c.movement={next.x-p.x,next.z-p.z};c.sprint=true;
      }
      if(s.Health()<s.MaxHealth()*.65f && std::string(s.AbilityStatus(3))=="READY") c.ability=3;
      else if(std::string(s.AbilityStatus(1))=="READY") c.ability=1;
      else if(std::string(s.AbilityStatus(0))=="READY") c.ability=0;
    }
    s.Update(.05f,c);
  }
  Check(s.DefeatedCount()==3,"Expansion encounter did not finish");
  Check(s.Message().find(DescribeQuest(s.CurrentQuest()).speaker)!=std::string::npos,
    "Expanded encounter sent the player to the wrong quest speaker");
}
void Capture(Renderer& renderer,const Simulation& s,const std::filesystem::path& path,ui::Panel panel=ui::Panel::None) {
  Game game;game.Initialize();Check(game.RestoreSnapshot(s.Save()),"Expanded world render fixture");
  game.OpenPanel(panel);game.SetCameraOrbit(.35f,.36f,8);
  if(panel==ui::Panel::Quest) {Command talk;talk.interact=true;game.ExecuteCommand(talk,.001f);}
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture(path.wstring()) && renderer.EndFrame(false),"Expansion GPU capture");
}
}
int main() {
  const auto root=std::filesystem::current_path()/"frontier-expansion-check";std::filesystem::create_directories(root);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Frontier expansion checks",false),"Hidden window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer");
  Simulation fresh;
  Walk(fresh,{0,-24});
  Capture(renderer,fresh,root/"southern-meadow.png");
  Capture(renderer,fresh,root/"southern-meadow-atlas.png",ui::Panel::Map);
  for(const auto& npc:WorldMap::residents) {
    const Vec2 approach{npc.position.x+1.6f,npc.position.z};
    Walk(fresh,fresh.CanStand(approach)?approach:Vec2{npc.position.x-1.6f,npc.position.z});const auto before=fresh.QuestState();Command talk;talk.interact=true;fresh.Update(.01f,talk);
    Check(fresh.QuestDialogue()==QuestId::None && fresh.QuestState()==before && !fresh.FrontierActive() &&
      fresh.FrontierCompleted()==0,"Future speaker accepted a locked quest");
  }
  Capture(renderer,fresh,root/"expanded-atlas.png",ui::Panel::Map);
  // Migrate a real old veteran, then the completed original seven-dispatch state.
  std::ifstream input(std::filesystem::path(RPG_TEST_FIXTURES)/"veteran-preview.save");std::ostringstream bytes;bytes<<input.rdbuf();
  Simulation s;Check(s.Load(bytes.str()),"Legacy veteran migration");
  auto save=Record(s.Save(),"PROFILE ","PROFILE 0 \"Frontier Veteran\" 7 0 3");
  Check(s.Load(save),"Original completed campaign fixture");
  // Version 12 has no survey record and remains loadable after the expansion.
  auto old=Record(save,"MEDITERRANEAN ","MEDITERRANEAN 12");old.erase(old.find("SURVEY "));
  Check(s.Load(old) && s.FrontierCompleted()==7 && s.SurveyMask()==0,"Version twelve progress migration");
  const auto migrated=s.Save();
  Check(!s.Load(Record(migrated,"SURVEY ","SURVEY 1")) && s.Save()==migrated,"Premature survey notes accepted");
  const int initialGold=s.Gold(),initialXp=s.Experience();
  for(int chapter=7;chapter<10;++chapter) {
    const auto npc=WorldMap::residents[chapter-7].position;const Vec2 approach{npc.x+1.6f,npc.z};Walk(s,approach);
    Speak(s,QuestAction::AcceptFrontier);
    Check(s.DialoguePosition().x==npc.x && s.DialoguePosition().z==npc.z,"Conversation points at the old keeper");
    Capture(renderer,s,root/("offer-"+std::to_string(chapter)+".png"),ui::Panel::Quest);
    Command accept;accept.questAction=QuestAction::AcceptFrontier;s.Update(.01f,accept);
    Check(s.FrontierActive() && !s.FrontierReady(),"Assignment auto-completed on acceptance");Roundtrip(s);
    if(chapter==8) {
      for(unsigned i=0;i<WorldMap::surveySites.size();++i) {
        const auto site=WorldMap::surveySites[i];Walk(s,{site.x,site.z-1.4f});
        Check(!s.CanStand(site),"Visible survey stone has no collision");
        Command read;read.interact=true;s.Update(.01f,read);Check(s.SurveyMask()==(1u<<(i+1))-1,"Survey bearing not recorded");
        const int xp=s.Experience();s.Update(.01f,read);Check(s.Experience()==xp,"Survey inspection farmed experience");
        Roundtrip(s);Check(s.Load(s.Save()),"Survey reload");
        Capture(renderer,s,root/("survey-"+std::to_string(i)+".png"));
      }
      Check(s.FrontierReady() && s.ObjectivePosition().x==npc.x,"Finished survey did not point home");
      const auto stable=s.Save();
      for(const auto row:{"SURVEY 8","SURVEY -1","SURVEY 4294967296","SURVEY"})
        Check(!s.Load(Record(stable,"SURVEY ",row)) && s.Save()==stable,"Invalid survey mask mutated live state");
    } else {
      for(const auto& enemy:s.Enemies()) {
        Check(s.CanStand(enemy.home,.45f),"New enemy home inside geometry");
        Check(chapter==7?enemy.home.x<-17:enemy.home.z>19,"New encounter reused the original patrol ground");
      }
      Fight(s);Roundtrip(s);
      Capture(renderer,s,root/("cleared-"+std::to_string(chapter)+".png"));
    }
    Walk(s,approach);Speak(s,QuestAction::CompleteFrontier);
    Command finish;finish.questAction=QuestAction::CompleteFrontier;s.Update(.01f,finish);
    Check(s.FrontierCompleted()==chapter+1 && s.Level()==10 && s.EarnedPoints()==5,"Expansion changed level/talent cap");
    Check(s.DialogueStage()==QuestStage::Complete,"Reward receipt lost its original speaker");
    const int xp=s.Experience();s.Update(.01f,finish);Check(s.Experience()==xp,"Repeated expansion reward");Roundtrip(s);
    Capture(renderer,s,root/("journal-"+std::to_string(chapter)+".png"),ui::Panel::Journal);
  }
  Check(s.Gold()==initialGold && s.Experience()==initialXp+750,"Expansion reward accounting");
  Check(s.ObjectivePosition().x==WorldMap::residents[3].position.x && s.ObjectivePosition().z==WorldMap::residents[3].position.z,"Completed expansion objective");
  const auto complete=s.Save();
  Check(!s.Load(Record(complete,"SURVEY ","SURVEY 3")) && s.Save()==complete,"Incomplete survey accepted in finished campaign");
  Check(renderer.Resize(960,540),"Small expansion UI resize");
  Capture(renderer,s,root/"journal-small.png",ui::Panel::Journal);
  Capture(renderer,s,root/"atlas-small.png",ui::Panel::Map);
  Check(renderer.ValidateDebugMessages(),"Expanded world D3D validation");
  std::cout<<"PASS: enlarged map, three reachable speakers, two combat regions, survey/save migration and twelve assignments\n";
}
