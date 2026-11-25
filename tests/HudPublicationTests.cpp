#include "game/Game.h"
#include "game/Localization.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include "tests/DashFixture.h"
#include "tests/QuestFixture.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using namespace rpg;
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
Simulation Hero(ClassId id){
  Simulation state;Check(state.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Hero fixture");
  auto save=state.Save();const auto at=save.find("PROFILE ");
  save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(id))+" \"Published hero\" 7 1 3");
  Check(state.Load(save),"Hero class");return state;
}
void SameBuild(const BuildStats& a,const BuildStats& b){
  Check(a.health==b.health && a.strike==b.strike && a.heavy==b.heavy && a.armor==b.armor && a.mend==b.mend &&
    a.dashCooldown==b.dashCooldown && a.mendCooldown==b.mendCooldown && a.basicCooldown==b.basicCooldown &&
    a.heavyCooldown==b.heavyCooldown && a.recoveryCast==b.recoveryCast,"Published preview disagrees with authority");
}
void OwnershipAndRules(){
  for(int cls=0;cls<int(ClassId::Count);++cls){
    auto state=Hero(ClassId(cls));Command c;c.selectActor=210;c.ability=1;state.Update(.01f,c);
    auto live=std::make_unique<ui::HudSnapshot>();live->Capture(state,17);const auto frozen=*live;
    Check(frozen.Nickname()==std::string(state.Nickname()) && frozen.Level()==state.Level() && frozen.MaxHealth()==state.MaxHealth(),"Hero identity/stats missing");
    Check(frozen.TargetActor() && frozen.TargetActor()->id==210 && frozen.TargetActor()->MaxHealth()==200,"Practice target publication");
    for(int i=-1;i<=int(spells.size());++i){
      Check(std::string(frozen.AbilityStatus(i))==state.AbilityStatus(i),"Spell readiness changed");
      Check(frozen.Cooldown(i)==state.Cooldown(i) && frozen.AbilityCooldown(i)==state.AbilityCooldown(i),"Cooldown publication changed");
      Check(frozen.CanBufferAttack(i)==state.CanBufferAttack(i),"Buffer permission changed");
    }
    for(unsigned mask:{0u,state.BuildMask(),buildPresets[0].mask,buildPresets[5].mask,~0u}){
      SameBuild(frozen.PreviewBuild(mask),state.PreviewBuild(mask));
      for(auto action:{BuildAction::Apply,BuildAction::Store})for(int slot=-1;slot<4;++slot){
        const BuildCommand b{action,int(mask),slot};Check(std::string(frozen.LoadoutStatus(b))==state.LoadoutStatus(b),"Build availability changed");
      }
    }
    for(int item=-1;item<int(Item::Count);++item)for(auto action:{TradeAction::Sell,TradeAction::Buyback}){
      const TradeCommand t{action,Item(item),state.Items().Find(Item(item))};
      Check(std::string(frozen.TradeStatus(t))==state.TradeStatus(t),"Trade availability changed");
    }
    const auto bytes=frozen.Bytes();const auto* storage=live->Combatants().data();
    state=Simulation{};
    for(unsigned i=0;i<100;++i)live->Capture(state,18+i);
    Check(live->Combatants().data()==storage && live->StorageGrowths()==1,"Stable HUD roster grew storage");
    Check(std::string(frozen.Nickname())=="Published hero" && frozen.TargetActor()->id==210 && frozen.Class()==ClassId(cls),"Frozen HUD borrowed character/target state");
    live.reset();Check(frozen.Map().CanStand({0,-10},.4f) && frozen.Items().Count(Item::Potion)==5,"Retained frame borrowed destroyed map/inventory");
    std::cout<<"Class "<<cls<<": retained HUD "<<bytes<<" bytes; authority-independent build/trade checks passed\n";
  }
  Simulation state;Check(state.Load(test::dashFixture),"Warning fixture");ui::HudSnapshot frame;frame.Capture(state,1);
  Check(frame.Incoming().count && frame.Incoming().rows[0].actor.id==100,"Owned incoming warning missing");
  state=Simulation{};Check(frame.Incoming().rows[0].actor.windup>0 && frame.Incoming().rows[0].actor.MaxHealth()==80,"Warning borrows old roster pointer");
}
using Image=std::vector<char>;
Image Capture(Game& g,Renderer& r,const std::filesystem::path& p){
  Check(r.BeginFrame(),"Begin frame");g.Render(r);Check(r.Capture(p.wstring()) && r.EndFrame(false),"Capture HUD");
  std::ifstream input(p,std::ios::binary);return {std::istreambuf_iterator<char>(input),{}};
}
void Frozen(Game& game,Renderer& renderer,const std::filesystem::path& out,const std::string& name){
  const auto first=Capture(game,renderer,out/(name+"-published.png"));
  const auto sequence=game.Hud().Sequence(),revision=game.Hud().Revision();
  const auto health=game.HudInterface().PlayerHealthSignal();const auto searches=game.HudInterface().Navigation().Searches();
  auto& source=const_cast<Simulation&>(game.State());source=Simulation{};const auto saved=source.Save();
  const auto retained=Capture(game,renderer,out/(name+"-retained.png"));
  Check(first==retained && saved==source.Save() && game.Hud().Revision()==revision,"Full frame/HUD reads live authority or changes its publication");
  Check(game.HudInterface().PlayerHealthSignal().trail==health.trail && game.HudInterface().Navigation().Searches()==searches,
    "Repeated rendering advanced health/navigation");
  game.ExecuteCommand({},.01f);const auto next=Capture(game,renderer,out/(name+"-next.png"));
  Check(first!=next && game.Hud().Revision()>revision && game.Hud().Sequence()!=sequence,"Accepted update did not publish a new HUD");
}
void Native(Renderer& renderer,const std::filesystem::path& out){
  for(auto cls:{ClassId::Warrior,ClassId::Mage,ClassId::Scout,ClassId::Priest}){
    auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(cls).Save()),"Restore class");
    Command cast;cast.selectActor=210;cast.ability=1;game->ExecuteFrame(cast,.02f);
    Frozen(*game,renderer,out,"cast-"+std::to_string(int(cls)));
  }
  for(auto panel:{ui::Panel::Character,ui::Panel::Inventory,ui::Panel::Map,ui::Panel::Journal,ui::Panel::Build,ui::Panel::Menu,ui::Panel::Merchant}){
    auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(ClassId::Priest).Save()),"Restore panel");
    game->OpenPanel(panel);Frozen(*game,renderer,out,"panel-"+std::to_string(int(panel)));
  }
  auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(test::patrolReturn),"Quest fixture");
  Command interact;interact.interact=true;game->ExecuteFrame(interact,.01f);
  Check(game->Hud().QuestDialogue()!=QuestId::None,"Quest dialogue did not open");Frozen(*game,renderer,out,"quest");
  game->Initialize();Check(game->RestoreSnapshot(test::dashFixture),"Health fixture");
  auto& attacker=const_cast<Enemy&>(game->State().Enemies()[0]);
  attacker.position=attacker.home={-4,3.4f};attacker.windup=.1f;attacker.facing=3.14159265f;
  bool hit=false;for(int i=0;i<40 && !hit;++i){game->ExecuteFrame({},.025f);hit=game->Hud().Health()<100;}
  Check(hit && game->HudInterface().PlayerHealthSignal().trail>game->HudInterface().PlayerHealthSignal().value,"Actual damage did not start trail");
  auto& source=const_cast<Simulation&>(game->State());for(auto& e:const_cast<std::vector<Enemy>&>(source.Enemies())){
    e.position=e.home={50,50};e.state=EnemyState::Idle;e.windup=0;e.attackCooldown=100;}
  for(int i=0;i<30;++i)game->ExecuteFrame({},.05f); // No rendering at all.
  const auto signal=game->HudInterface().PlayerHealthSignal();
  Check(signal.trail-signal.value<.001f,"Hidden HUD froze an old damage trail");
  const auto first=Capture(*game,renderer,out/"hidden-hud.png");
  Check(first==Capture(*game,renderer,out/"hidden-hud-repeat.png"),"Drawing resumes/advances a hidden trail");
  game->SuspendInput();Check(!game->Hud().Attacking() && game->Hud().QuestDialogue()==QuestId::None,"Focus cancellation not published");
  Check(game->RestoreSnapshot(Hero(ClassId::Mage).Save()) && game->Hud().Class()==ClassId::Mage,"Restore retained previous hero HUD");
}
}
int main(){try{
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");OwnershipAndRules();
  const auto out=std::filesystem::current_path()/"hud-publication-review";std::filesystem::create_directories(out);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"HUD publication checks",false),"Window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,600),"Renderer");Native(renderer,out);
  Check(renderer.ValidateDebugMessages(),"Native rendering diagnostics");
  std::cout<<"PASS: owned complete HUD, shared transaction previews, retained full frames and update-driven health/navigation.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
