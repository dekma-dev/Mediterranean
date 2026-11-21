#include "game/Game.h"
#include "game/Localization.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include "tests/QuestFixture.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>

namespace {
using namespace rpg;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Warp(Simulation& s,Vec2 p) {
  auto data=s.Save();const auto at=data.find('\n',data.find("PROFILE "))+1;
  std::istringstream row(data.substr(at));float x,z,h,f;int q,g;row>>x>>z>>h>>f>>q>>g;
  std::ostringstream out;out<<p.x<<' '<<p.z<<' '<<s.MaxHealth()<<' '<<f<<' '<<q<<' '<<g;
  data.replace(at,data.find('\n',at)-at,out.str());Check(s.Load(data),"Quest relocation");
}
void Talk(Simulation& s,int i,QuestAction action) {
  Warp(s,WorldMap::residents[9+i].position);Command c;c.interact=true;s.Update(.001f,c);
  Check(s.QuestDialogue()==QuestId(int(QuestId::Hunt1)+i) && s.QuestPrimaryAction()==action,"Local dialogue state");
  c={};c.questAction=action;s.Update(.001f,c);s.CloseQuestDialogue();
}
Simulation AllQuests() {
  Simulation s;for(int i=0;i<6;++i)Talk(s,i,QuestAction::AcceptHunt);
  Warp(s,WorldMap::spawn);s.TrackHunt(-1);return s;
}
void Gather(Simulation& s,int district,int index) {
  const auto p=population::Cache(district,index);const int before=s.HuntProgress(district);
  for(int n=0;n<16;++n) {
    const float a=n*.392699f;const Vec2 candidate{p.x+std::sin(a)*1.7f,p.z+std::cos(a)*1.7f};
    if(!s.CanStand(candidate))continue;Warp(s,candidate);Command c;c.interact=true;s.Update(.001f,c);
    if(s.HuntProgress(district)>before)return;
  }
  throw std::runtime_error("Actual quest cache inaccessible");
}
void Kill(Simulation& s,int index) {
  const auto home=s.Beasts()[index].position;bool found=false;
  for(int n=0;n<16;++n) {
    const float a=n*.392699f;const Vec2 p{home.x+std::sin(a)*1.7f,home.z+std::cos(a)*1.7f};
    if(s.CanStand(p) && s.Map().CanTravel(p,home,.05f)){Warp(s,p);found=true;break;}
  }
  Check(found,"No weapon approach");
  for(auto& other:const_cast<std::vector<Enemy>&>(s.Beasts()))other.stunned=10;
  auto& e=const_cast<Enemy&>(s.Beasts()[index]);e.health=1;
  Command c;c.selectActor=e.id;c.ability=0;s.Update(.01f,c);
  for(int i=0;i<150;++i)s.Update(.01f,{});
  Check(s.Beasts()[index].health==0,"Actual weapon contact did not kill beast");
}
unsigned Notices(const ui::QuestActivity& feed) {
  return unsigned(std::count_if(feed.Notices().begin(),feed.Notices().end(),[](const auto& n){return n.remaining>0;}));
}
std::string Record(std::string data,const std::string& key,const std::string& row) {
  const auto at=data.find(key);Check(at!=std::string::npos,"Missing fixture record");
  data.replace(at,data.find('\n',at)-at,row);return data;
}
void FinishedAndReplayedCampaign() {
  Simulation s;Check(s.Load("MEDITERRANEAN 3\n0 -10 100 0 3 90\n5 1 1 0\n4\n0 0 0 0\n100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Veteran base");
  auto data=Record(s.Save(),"PROFILE ","PROFILE 0 \"Tracker\" 10 0 3");
  data=Record(data,"SURVEY ","SURVEY 7");Check(s.Load(data),"Highland baseline");
  Warp(s,QuestLocation(s.CurrentQuest()));Command c;c.interact=true;s.Update(.001f,c);
  c={};c.questAction=QuestAction::AcceptFrontier;s.Update(.001f,c);data=s.Save();
  for(const auto& e:s.Enemies()) {
    std::ostringstream row;row<<e.id<<' '<<e.home.x<<' '<<e.home.z<<" 0 0 1 3 0 0";
    data=Record(data,std::to_string(e.id)+" ",row.str());
  }
  data=Record(data,"PROFILE ","PROFILE 0 \"Tracker\" 16 0 3");Check(s.Load(data),"Finished campaign baseline");
  Check(ui::ReadQuestTracking(s).count==0,"Finished campaign reappears as a new offer");
  ui::QuestActivity feed;feed.Update(.01f,s);Talk(s,0,QuestAction::AcceptHunt);feed.Update(.01f,s);
  Check(Notices(feed)==1 && ui::ReadQuestTracking(s).count==1,"Local work lost after campaign completion");
  const auto npc=QuestLocation(QuestId::Watch13);Warp(s,{npc.x+1.6f,npc.z});c={};c.interact=true;s.Update(.001f,c);
  Check(s.QuestPrimaryAction()==QuestAction::BeginReplay,"Replay speaker unavailable");
  c={};c.questAction=QuestAction::BeginReplay;s.Update(.001f,c);feed.Update(.01f,s);
  const auto replay=ui::ReadQuestTracking(s);
  Check(s.ReplayActive() && replay.count==1 && replay.entries[0].selected && replay.entries[0].id==QuestId::Watch13 && Notices(feed)==0,"Rehearsal mixes campaign and local routes or issues quest receipts");
  c={};c.endReplay=true;s.Update(.001f,c);feed.Update(.01f,s);
  Check(Notices(feed)==0 && ui::ReadQuestTracking(s).count==1,"Rehearsal exit fabricated quest acceptance");
}
void TrackingAndReceipts() {
  auto s=AllQuests();const auto saved=s.Save();auto view=ui::ReadQuestTracking(s);
  Check(view.count==7 && view.entries[0].selected && s.Save()==saved,"Tracker must read all assignments without mutating them");
  ui::QuestActivity feed;feed.Update(.01f,s);Check(Notices(feed)==0,"Load fabricated quest activity");
  for(int i=0;i<6;++i) {
    s.TrackHunt(i);view=ui::ReadQuestTracking(s);feed.Update(.01f,s);
    unsigned selected=0;for(unsigned j=0;j<view.count;++j)selected+=view.entries[j].selected;
    Check(selected==1 && view.Find(QuestId(int(QuestId::Hunt1)+i))->selected && Notices(feed)==0,"Retracking fabricated progress or duplicated selection");
    Check(s.CampaignObjective().find("PATROL")!=std::string::npos,"Tracking local work erased campaign objective");
  }
  // Untracked collection progresses independently of the selected hunt.
  s.TrackHunt(0);Gather(s,2,0);feed.Update(.01f,s);
  Check(feed.Notices()[0].kind==ui::QuestNoticeKind::Progress && feed.Notices()[0].id==QuestId::Hunt3,"Untracked collection has no real progress receipt");
  Gather(s,2,1);feed.Update(.01f,s);
  Check(Notices(feed)==1 && feed.Notices()[0].detail.find("2 / 3")!=std::string::npos,"Repeated increments should coalesce with current count");
  Gather(s,2,2);feed.Update(.01f,s);view=ui::ReadQuestTracking(s);
  Check(Notices(feed)==1 && feed.Notices()[0].kind==ui::QuestNoticeKind::Ready && view.Find(QuestId::Hunt3)->stage==QuestStage::TurnIn,"Ready receipt not tied to actual completion");
  const auto gold=s.Gold();Talk(s,2,QuestAction::CompleteHunt);feed.Update(.01f,s);
  Check(s.Gold()>gold && feed.Notices()[0].kind==ui::QuestNoticeKind::Delivered && !ui::ReadQuestTracking(s).Find(QuestId::Hunt3),"Delivery receipt differs from accepted transaction");
  feed.Reset();feed.Update(.01f,s);Check(Notices(feed)==0,"Lifecycle reset replayed completed work");
  // A numeric 3/3 is insufficient without the required leader.
  for(int i=18;i<21;++i)Kill(s,i);
  feed.Update(.01f,s);view=ui::ReadQuestTracking(s);
  Check(!s.HuntReady(3) && view.Find(QuestId::Hunt4)->stage==QuestStage::Progress && view.Find(QuestId::Hunt4)->summary.find("Leader required")!=std::string::npos,"Leader requirement disappeared behind the counter");
  Kill(s,23);feed.Update(.01f,s);
  Check(s.HuntReady(3) && feed.Notices()[0].kind==ui::QuestNoticeKind::Ready,"Leader kill did not produce readiness");
  for(int i=0;i<600;++i)feed.Update(.01f,s);Check(Notices(feed)==0,"Quest notices never expire");
  feed.Update(std::numeric_limits<float>::quiet_NaN(),s);feed.Update(-1,s);
  Check(Notices(feed)==0,"Invalid frame produced activity");

  Simulation fresh;ui::QuestActivity accepted;accepted.Update(.01f,fresh);
  for(int i=0;i<6;++i){Talk(fresh,i,QuestAction::AcceptHunt);accepted.Update(.01f,fresh);}
  Check(Notices(accepted)==3 && accepted.Notices()[0].id==QuestId::Hunt6 && accepted.Notices()[2].id==QuestId::Hunt4,"Bounded acceptance feed lost newest-first order");
  fresh.TrackHunt(-1);accepted.Update(.1f,fresh);Check(accepted.Notices()[0].Opacity()>.5f,"Notice reveal animation");

  Simulation campaign;Check(campaign.Load(test::patrolReturn),"Campaign receipt fixture");
  ui::QuestActivity main;main.Update(.01f,campaign);
  Check(ui::ReadQuestTracking(campaign).entries[0].stage==QuestStage::TurnIn,"Patrol needs one kill, not three");
  Command c;c.interact=true;campaign.Update(.001f,c);c={};c.questAction=QuestAction::CompletePatrol;campaign.Update(.001f,c);main.Update(.01f,campaign);
  Check(main.Notices()[0].kind==ui::QuestNoticeKind::Delivered && main.Notices()[0].id==QuestId::Patrol,"Campaign delivery receipt missing");
  Check(ui::ReadQuestTracking(campaign).entries[0].stage==QuestStage::Offer,"Next offer falsely marked accepted");
}
void ViewsAndClicks() {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  Win32Window window;Renderer renderer;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Quest tracking review",false),"Review window");
  Check(renderer.Initialize(window.GetHandle(),1280,720),"Review renderer");
  const auto output=std::filesystem::current_path()/"quest-tracking-review";std::filesystem::create_directories(output);
  const auto initial=AllQuests();auto active=initial;Gather(active,2,0);Gather(active,2,1);
  auto ready=active;Gather(ready,2,2);Warp(active,WorldMap::spawn);Warp(ready,WorldMap::spawn);
  for(auto size:{std::pair<int,int>{1280,720},{960,600},{2560,1080}}) {
    Check(renderer.Resize(size.first,size.second),"Quest review resize");Game game;game.Initialize();Check(game.RestoreSnapshot(active.Save()),"Quest native fixture");
    game.OpenPanel(ui::Panel::None);game.SetCameraOrbit(.2f,.55f,8);ui::Interface hud;
    hud.UpdateCombatFeedback(.01f,initial);hud.UpdateCombatFeedback(.01f,active);hud.UpdateCombatFeedback(.2f,active);
    const auto capture=[&](const char* name) {
      renderer.BeginFrame();game.Render(renderer,false);hud.Render(renderer,game.State(),1,false);
      Check(renderer.Capture((output/(std::string(name)+"-"+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Quest screenshot");
    };
    capture("progress");
    const ui::Layout layout(float(size.first),float(size.second));
    const auto pointer=[&](float y,bool click) {return hud.Pointer(layout.x+1130*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),click,false);};
    std::array<float,6> rows{};
    for(float y=220;y<670;y+=2){const auto hit=pointer(y,true);if(hit.action==ui::Action::TrackHunt && hit.value>=0)rows[hit.value]=y;}
    for(auto y:rows)Check(y>0,"An accepted quest has no clickable HUD row");
    Check(!hud.CapturesMovement(),"Quest list pauses movement");pointer(670,false);Check(!hud.PointerOverUi(),"Tracker captures empty world below it");
    // Exercise the actual Game input dispatch, not just the returned region.
    renderer.BeginFrame();game.Render(renderer);Check(renderer.EndFrame(false),"Native input regions");
    Input input;Input::Frame frame;frame.focused=true;frame.width=size.first;frame.height=size.second;
    frame.client={LONG(layout.x+1130*layout.scale),LONG(layout.y+rows[2]*layout.scale)};
    input.Update(frame);game.Update(.01,input,0);frame.down[VK_LBUTTON]=true;input.Update(frame);game.Update(.01,input,0);
    Check(game.State().TrackedHunt()==2,"Real HUD click did not switch simulation route");
    frame.down[VK_LBUTTON]=false;input.Update(frame);game.Update(.01,input,0);
    capture("selected");
    Check(game.RestoreSnapshot(ready.Save()),"Ready native fixture");hud.UpdateCombatFeedback(.01f,ready);hud.UpdateCombatFeedback(.2f,ready);capture("ready");
    // Loading/character reset clears receipts; selecting a row itself is silent.
    hud.ResetCombatFeedback();hud.UpdateCombatFeedback(.01f,game.State());capture("loaded");
    hud.Show(ui::Panel::Inventory);capture("inventory");
    for(float y=220;y<670;y+=2)Check(pointer(y,true).action!=ui::Action::TrackHunt,"Hidden tracker steals bag clicks");
    hud.SetOnly(ui::Panel::None);hud.ResetCombatFeedback();Simulation batch;
    hud.UpdateCombatFeedback(.01f,batch);
    for(int i=0;i<3;++i){Talk(batch,i,QuestAction::AcceptHunt);hud.UpdateCombatFeedback(.01f,batch);}
    Warp(batch,WorldMap::spawn);Check(!batch.InteractionPrompt().empty(),"Batch near-speaker fixture");
    Check(game.RestoreSnapshot(batch.Save()),"Batch near-speaker native scene");hud.UpdateCombatFeedback(.2f,batch);capture("batch-near-speaker");
    Warp(batch,{0,0});Check(batch.InteractionPrompt().empty(),"Batch open-world fixture");
    Check(game.RestoreSnapshot(batch.Save()),"Batch open-world native scene");capture("batch-in-world");
  }
  Check(renderer.ValidateDebugMessages(),"D3D quest tracking validation");
}
}
int main()try{TrackingAndReceipts();FinishedAndReplayedCampaign();ViewsAndClicks();std::cout<<"PASS: simultaneous objectives, real progress/delivery, lifecycle, bounded notices and native route switching\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
