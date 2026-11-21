#include "game/Game.h"
#include "game/Localization.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace rpg;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
const char* fixture="MEDITERRANEAN 1\n-4 6 50 0 2 0\n0 0 0 0\n-4 7 0 0\n-3 7 0 0\n-4 8 0 0\n";
Simulation Corpses(){Simulation s;Check(s.Load(fixture),"Loot fixture load");return s;}
void Tick(Simulation& s,float seconds){for(int n=0;n<int(seconds*100);++n)s.Update(.01f,{});}
void Transactions() {
 {
  Simulation fight;Check(fight.Load("MEDITERRANEAN 1\n-4 6 100 0 1 0\n0 0 0 0\n-4 7 80 0\n-3 7 80 0\n-4 8 80 0\n"),"Live group fixture");
  for(auto& enemy:const_cast<std::vector<Enemy>&>(fight.Enemies())){enemy.health=1;enemy.stunned=100;}
  for(int i=0;i<3;++i){Command strike;strike.selectActor=fight.Enemies()[i].id;strike.ability=0;fight.Update(.01f,strike);Tick(fight,1.5f);}
  Check(fight.DefeatedCount()==3 && fight.QuestState()==Quest::Return,"Real attacks did not finish the group");
  Command gather;gather.interact=true;fight.Update(.01f,gather);Simulation saved;
  Check(fight.Gold()==15 && fight.Potions()==5 && saved.Load(fight.Save()) && saved.Save()==fight.Save(),"Live group loot broke campaign accounting");
 }
 for(bool direct:{false,true}) {
  auto s=Corpses();const auto preview=s.NearbyLoot();
  Check(preview.bodies==3 && preview.gold==15 && preview.potions==3,"Nearby loot preview omits eligible bodies");
  Command c;c.interact=!direct;c.collectLoot=direct;s.Update(.01f,c);
  Check(s.Gold()==15 && s.Potions()==5 && s.NearbyLoot().bodies==0,"One request must collect all accessible bundles");
  Check(s.LastLoot().bodies==3 && s.LastLoot().gold==15 && s.LastLoot().potions==3,"Receipt differs from accepted rewards");
  const auto save=s.Save();s.Update(.01f,c);Check(s.Save()==save,"Repeated loot changes rewards");
  Simulation restored;Check(restored.Load(save) && restored.Save()==save && restored.LastLoot().remaining==0,"Save/load replayed receipt or lost loot ledger");
  restored.Update(.01f,c);Check(restored.Gold()==15,"Load enables duplicate loot");
  Tick(s,4.1f);Check(s.LastLoot().remaining==0,"Receipt did not expire");
 }
 // Leave one stack slot: nearest-first accepts exactly one atomic bundle.
 auto partial=Corpses();auto& bag=const_cast<Inventory&>(partial.Items());
 Check(bag.Add(Item::Potion,477),"Partial-capacity setup");Command take;take.collectLoot=true;partial.Update(.01f,take);
 Check(partial.Gold()==5 && partial.Potions()==480 && partial.Enemies()[0].looted && !partial.Enemies()[1].looted && !partial.Enemies()[2].looted,"Partial collection lost a bundle or ignored distance order");
 Check(partial.LastLoot().bodies==1 && partial.LastLoot().blocked==2 && partial.NearbyLoot().bodies==2,"Partial receipt/preview mismatch");
 partial.Update(.01f,take);Check(partial.Gold()==5 && partial.LastLoot().blocked==2 && partial.LastLoot().gold==0,"Full bag granted gold or forged success");
 Check(bag.Consume(Item::Potion) && bag.Consume(Item::Potion),"Free two item places");partial.Update(.01f,take);
 Check(partial.Gold()==15 && partial.NearbyLoot().bodies==0,"Retry failed to recover retained bundles");
 // Gold-only wildlife remains collectible even when an earlier humanoid is blocked.
 auto mixed=Corpses();Check(const_cast<Inventory&>(mixed.Items()).Add(Item::Potion,478),"Full mixed bag");
 auto& beast=const_cast<Enemy&>(mixed.Beasts()[0]);beast.position={-5,6};beast.health=0;beast.looted=false;
 mixed.Update(.01f,take);Check(mixed.Gold()==2 && mixed.LastLoot().blocked==3 && mixed.LastLoot().bodies==1,"Blocked potions suppressed wildlife gold");
 beast.looted=false;mixed.Update(.01f,take);Check(mixed.Gold()==2,"Wildlife finite spoils duplicated");
}
void Access() {
 auto s=Corpses();auto& enemies=const_cast<std::vector<Enemy>&>(s.Enemies());
 enemies[1].position={30,30};enemies[2].health=80;
 Check(s.NearbyLoot().bodies==1,"Out-of-range or living enemy offered loot");
 bool checked=false;
 for(const auto& prop:s.Map().Obstacles())if(prop.kind==PropKind::Tree) {
  const Vec2 p{prop.center.x-prop.halfX-.6f,prop.center.z},q{prop.center.x+prop.halfX+.6f,prop.center.z};
  if(std::hypot(p.x-q.x,p.z-q.z)>2.5f || !s.CanStand(p) || !s.CanStand(q) || s.Map().CanTravel(p,q,.05f))continue;
  std::ostringstream data;data<<"MEDITERRANEAN 1\n"<<p.x<<' '<<p.z<<" 50 0 2 0\n0 0 0 0\n";
  for(int i=0;i<3;++i)data<<q.x<<' '<<q.z<<" 0 0\n";
  Simulation blocked;Check(blocked.Load(data.str()),"Occluded loot fixture");
  Check(blocked.NearbyLoot().bodies==0,"Occluded loot offered through scenery");
  Command c;c.collectLoot=true;blocked.Update(.001f,c);Check(blocked.Gold()==0 && !blocked.Enemies()[0].looted,"Occluded loot was granted");checked=true;break;
 }
 Check(checked,"No real scenery obstruction exercised");
 auto stale=Corpses();Check(stale.NearbyLoot().bodies==3,"Stale preview fixture");
 for(auto& enemy:const_cast<std::vector<Enemy>&>(stale.Enemies()))enemy.position={30,30};
 Command collect;collect.collectLoot=true;stale.Update(.001f,collect);
 Check(stale.Gold()==0 && stale.LastLoot().remaining==0,"Stale loot card granted remote rewards");
 Simulation resident;auto& corpse=const_cast<Enemy&>(resident.Enemies()[0]);
 corpse.health=0;corpse.position=resident.Player();
 Command talk;talk.interact=true;resident.Update(.001f,talk);
 Check(resident.QuestDialogue()!=QuestId::None && !corpse.looted,"E lost nearby quest-speaker priority");
 resident.CloseQuestDialogue();resident.Update(.001f,collect);
 Check(corpse.looted && resident.Gold()==5 && resident.QuestDialogue()==QuestId::None,"Explicit loot request also opened dialogue");
}
void Views() {
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Win32Window window;Renderer renderer;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Area loot review",false),"Review window");
 Check(renderer.Initialize(window.GetHandle(),1280,720),"Review renderer");
 const auto output=std::filesystem::current_path()/"area-loot-review";std::filesystem::create_directories(output);
 for(auto size:{std::pair<int,int>{1280,720},{960,600},{2560,1080}}) {
  Check(renderer.Resize(size.first,size.second),"Loot review resize");Game game;game.Initialize();Check(game.RestoreSnapshot(fixture),"Native loot scene");
  game.OpenPanel(ui::Panel::None);game.SetCameraOrbit(.3f,.55f,7);ui::Interface hud;
  const auto capture=[&](const char* phase) {
   renderer.BeginFrame();game.Render(renderer,false);hud.Render(renderer,game.State(),0,false);
   Check(renderer.Capture((output/(std::string(phase)+"-"+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Loot capture");
  };
  capture("available");
  const ui::Layout layout(float(size.first),float(size.second));
  const float px=layout.x+120*layout.scale,py=layout.y+578*layout.scale;
  auto click=hud.Pointer(px,py,float(size.first),float(size.second),true,false);
  if(click.action==ui::Action::None)click=hud.Pointer(px,py,float(size.first),float(size.second),false,false,false,true);
  Check(click.action==ui::Action::CollectLoot,"Loot card has no working collect button");
  const auto unchanged=game.State().Save();Check(!hud.CapturesMovement(),"Loot card captures movement");
  Command c;c.collectLoot=true;game.ExecuteCommand(c,.01f);capture("received");
  Check(game.State().Gold()==15 && game.State().Save()!=unchanged,"Native loot command was decorative");
  for(int i=0;i<410;++i)game.ExecuteCommand({},.01f);capture("expired");
  hud.Pointer(px,py,float(size.first),float(size.second),false,false);
  Check(!hud.PointerOverUi(),"Expired loot card blocks camera input");
  Check(game.RestoreSnapshot(fixture),"Partial loot native reset");auto& bag=const_cast<Inventory&>(game.State().Items());
  Check(bag.Add(Item::Potion,477),"Partial loot native bag");game.ExecuteCommand(c,.01f);capture("partial");
  game.ExecuteCommand(c,.01f);capture("full");
  Check(game.State().LastLoot().gold==0 && game.State().LastLoot().blocked==2,"Full bag native receipt claims rewards");
 }
 Check(renderer.ValidateDebugMessages(),"D3D loot validation");
}
}
int main()try{Transactions();Access();Views();std::cout<<"PASS: area loot transactions, capacity, reach, persistence and native cards\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
