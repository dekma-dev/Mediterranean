#include "game/Game.h"
#include "game/SpellTrajectory.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {
using namespace rpg;
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string Record(std::string data,const std::string& key,const std::string& row){
  const auto at=data.find("\n"+key);Check(at!=data.npos,"Missing fixture record");
  data.replace(at+1,data.find('\n',at+1)-at-1,row);return data;
}
void MoveTo(Simulation& s,Vec2 p){
  auto data=s.Save();const auto at=data.find('\n',data.find("PROFILE "))+1;
  std::ostringstream row;row<<p.x<<' '<<p.z<<" 100 0 3 90";data.replace(at,data.find('\n',at)-at,row.str());
  Check(s.Load(data),"Fixture relocation");
}
void AcceptChapter(Simulation& s){
  const auto p=QuestLocation(s.CurrentQuest());MoveTo(s,{p.x+1.6f,p.z});Command c;c.interact=true;s.Update(.001f,c);
  Check(s.QuestPrimaryAction()==QuestAction::AcceptFrontier,"Chapter offer missing");
  c={};c.questAction=QuestAction::AcceptFrontier;s.Update(.001f,c);
}
Simulation Hero(ClassId id,int chapter=4){
  Simulation s;Check(s.Load("MEDITERRANEAN 3\n-4 5.2 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Legacy hero");
  const auto profile="PROFILE "+std::to_string(int(id))+" \"Publication\" ";
  auto data=Record(s.Save(),"PROFILE ",profile+(chapter>=13?"10 0 3":std::to_string(chapter)+" 1 3"));
  if(chapter>=10)data=Record(data,"SURVEY ","SURVEY 7");
  Check(s.Load(data),"Publication hero");
  if(chapter>=13){
    AcceptChapter(s);
    auto& enemies=const_cast<std::vector<Enemy>&>(s.Enemies());
    for(auto& e:enemies){e.health=0;e.looted=true;e.state=EnemyState::Dead;e.windup=0;e.attackCooldown=0;}
    Check(s.Load(Record(s.Save(),"PROFILE ",profile+std::to_string(chapter)+" 0 3")),"Southern chapter fixture");
    AcceptChapter(s);
  }
  return s;
}
void Ownership(){
  auto state=Hero(ClassId::Mage);
  auto& foe=const_cast<Enemy&>(state.Enemies()[0]);foe.health=500;foe.rooted=2;foe.snared=3;foe.exposed=4;foe.stunned=5;
  foe.kind=EnemyKind::Wolf;foe.position={-4,9};foe.home=foe.position;
  presentation::SceneSnapshot live;live.Capture(state,10);const auto frozen=live;
  const auto* captured=frozen.FindActor(100);Check(captured && captured->rooted==2 && captured->snared==3 &&
    captured->exposed==4 && captured->contactHeight==.72f,"Actor status/contact data missing");
  SustainedEffects effects;for(int i=0;i<20;++i)effects.Update(frozen,.01f);
  Check(effects.Weight(100,0)>.99f && effects.Weight(100,3)>.99f,"Frozen status did not drive envelopes");
  const auto* storage=live.Combatants().data();const auto bytes=live.Bytes();
  state=Simulation{};live.Capture(state,11);
  Check(frozen.FindActor(100)->health==500 && frozen.FindActor(100)->rooted==2 && !live.FindActor(100)->rooted,
    "Later world overwrote a retained status");
  for(int i=0;i<300;++i)live.Capture(state,12+i);
  Check(storage==live.Combatants().data() && bytes==live.Bytes(),"Stable scene grew storage");
  effects.Update(live,.01f);Check(!effects.ActorCount(),"Class replacement retained old control effects");

  auto court=Hero(ClassId::Warrior,15);live.Capture(court,400);
  Check(live.World().courtActive && live.World().captain.id==105 && live.World().captainProtected,"Court publication missing captain/ward");
  auto& roster=const_cast<std::vector<Enemy>&>(court.Enemies());std::reverse(roster.begin(),roster.end());
  live.Capture(court,401);Check(live.World().captain.id==105 && live.World().captainProtected,"Court attachment follows array position");
  roster.clear();live.Capture(court,402);Check(!live.World().captain.id && !live.World().captainProtected,"Removed captain retained ward");
  std::cout<<"Owned world/status scene: "<<bytes<<" bytes, 300 stable publications\n";
}
void ProjectileOwnership(){
  auto state=Hero(ClassId::Mage);auto& foe=const_cast<Enemy&>(state.Enemies()[0]);
  foe.health=500;foe.position={-4,10};foe.home=foe.position;foe.attackCooldown=100;
  Command cast;cast.selectActor=100;cast.ability=0;CombatVisualEvent release{};bool found=false;
  for(int n=0;n<120 && !found;++n){state.Update(.005f,n==0?cast:Command{});
    for(std::size_t i=0;i<state.VisualEventCount();++i)if(state.VisualEvents()[i].kind==CombatVisualKind::SpellCast){release=state.VisualEvents()[i];found=true;}}
  Check(found && release.flight>.02f,"Real projectile fixture did not release");
  presentation::SceneSnapshot frozen;frozen.Capture(state,1);
  const DirectX::XMFLOAT3 origin{-4,1.25f,5.2f};CombatEffects fx;fx.Emit(release,&origin);fx.Advance(.01f);fx.TrackProjectiles(frozen);
  Check(fx.ProjectilePath(100)!=nullptr,"Accepted flight missing");const auto target=fx.ProjectilePath(100)->Target();
  foe.position={30,30};foe.kind=EnemyKind::Boar;foe.health=0;
  cast={};cast.selectActor=211;state.Update(.001f,cast);
  fx.Advance(.005f);fx.TrackProjectiles(frozen);const auto retained=fx.ProjectilePath(100)->Target();
  Check(target.x==retained.x && target.y==retained.y && target.z==retained.z && !fx.ProjectilePath(211),
    "Frozen flight followed live target mutation or reselection");
  frozen.Capture(state,2);fx.Advance(.005f);fx.TrackProjectiles(frozen);const auto dead=fx.ProjectilePath(100)->Target();
  Check(dead.x==target.x && dead.y==target.y && dead.z==target.z,"Dead target manufactured a new flight destination");
}
void TrainingVisibility(){
  auto state=Hero(ClassId::Mage);MoveTo(state,{12,-3.8f});
  auto& dummy=const_cast<Enemy&>(state.Dummies()[0]);dummy.rooted=2;
  presentation::SceneSnapshot scene;scene.Capture(state,1);SustainedEffects effects;
  for(int i=0;i<20;++i)effects.Update(scene,.01f);
  Check(scene.FindActor(210) && effects.Weight(210,0)>.99f,"Visible practice control fixture missing");
  Command begin;begin.interact=true;state.Update(.001f,begin);Check(state.Sparring(),"Real sparring did not start");
  scene.Capture(state,2);effects.Update(scene,.01f);
  Check(!scene.FindActor(210) && scene.FindActor(200) && effects.Weight(210,0)==0,
    "Hidden mannequin retained a projectile destination or ghost control effect during sparring");
  state.Update(.001f,begin);Check(!state.Sparring(),"Sparring did not stop");scene.Capture(state,3);effects.Update(scene,.01f);
  Check(scene.FindActor(210) && !scene.FindActor(200) && effects.Weight(210,0)>0 && effects.Weight(210,0)<.2f,
    "Returning practice prop inherited its hidden envelope or lost its live status");
}
using Image=std::vector<char>;
Image Capture(Game& game,Renderer& renderer,const std::filesystem::path& path){
  Check(renderer.BeginFrame(),"Begin publication frame");game.Render(renderer,false);
  Check(renderer.Capture(path.wstring()) && renderer.EndFrame(false),"Capture publication frame");
  std::ifstream input(path,std::ios::binary);Image bytes{std::istreambuf_iterator<char>(input),{}};
  Check(bytes.size()>1000,"Empty publication capture");return bytes;
}
void FrozenRender(Game& game,Renderer& renderer,const std::filesystem::path& out,const std::string& name){
  const auto sequence=game.Scene().Sequence(),revision=game.Scene().Revision();
  const auto feedback=game.Scene().Feedback();
  const auto first=Capture(game,renderer,out/(name+"-published.png"));
  // Combat stress actors deliberately exceed save health/cooldown limits.
  // Restore through a valid authored fixture, not a stress-state serialization.
  const auto restore=Hero(game.Scene().Player().classId).Save();
  // Deliberately advance/replace authority without publishing. This represents
  // a retained render frame, not an allowed production mutation path.
  auto& live=const_cast<Simulation&>(game.State());live=Simulation{};const auto changed=live.Save();
  const auto retained=Capture(game,renderer,out/(name+"-retained.png"));
  Check(first==retained && live.Save()==changed && game.Scene().Sequence()==sequence && game.Scene().Revision()==revision,
    "3D rendering borrowed newer simulation state or advanced a publication");
  Check(feedback.size()==game.Scene().Feedback().size(),"Live replacement changed retained combat text");
  for(std::size_t i=0;i<feedback.size();++i)Check(feedback[i].sequence==game.Scene().Feedback()[i].sequence &&
    feedback[i].age==game.Scene().Feedback()[i].age && feedback[i].amount==game.Scene().Feedback()[i].amount,
    "Live replacement changed retained combat text content");
  game.ExecuteCommand({},.001f);const auto next=Capture(game,renderer,out/(name+"-next.png"));
  Check(first!=next && game.Scene().Revision()>revision,"Next authoritative step did not replace the frozen scene");
  Check(game.RestoreSnapshot(restore),"Public restore rejected the valid fixture");
  Check(game.Effects().BurstCount()==0 && game.Sustained().ActorCount()==0,"Public restore retained an old-world emitter");
}
void CombatFrames(Renderer& renderer,const std::filesystem::path& out){
  for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout,ClassId::Priest})for(int phase=0;phase<3;++phase){
    auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(id).Save()),"Combat scene load");
    auto& foe=const_cast<Enemy&>(game->State().Enemies()[0]);foe.health=500;foe.looted=false;foe.state=EnemyState::Idle;
    foe.position=foe.home={-4,id==ClassId::Warrior?7.f:10.5f};foe.attackCooldown=100;
    game->SetCameraOrbit(0,.95f,12);bool reached=false;
    for(int n=0;n<400 && !reached;++n){Command c;if(n==0){c.selectActor=100;c.ability=1;}
      const auto receipt=game->ExecuteFrame(c,1.f/120);game->TakeSoundEvents();
      reached=phase==0?n==20:phase==1?receipt.Has(CombatVisualKind::SpellCast)||receipt.Has(CombatVisualKind::HeavyStrike):
        receipt.Has(CombatVisualKind::Hit)||receipt.Has(CombatVisualKind::HeavyHit);
    }
    Check(reached,"Actual preparation/release/contact phase missing");
    if(phase==2)Check(!game->Scene().Feedback().empty(),"Contact text not published");
    FrozenRender(*game,renderer,out,"class-"+std::to_string(int(id))+"-phase-"+std::to_string(phase));
  }
  for(auto id:{ClassId::Warrior,ClassId::Mage,ClassId::Scout,ClassId::Priest}){
    auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(id).Save()),"Defense load");
    game->SetCameraOrbit(0,.95f,12);Command c;c.ability=3;game->ExecuteFrame(c,.01f);
    for(int i=0;i<30;++i)game->ExecuteFrame({},.05f);
    auto& foe=const_cast<Enemy&>(game->State().Enemies()[0]);foe.health=500;foe.rooted=foe.stunned=foe.exposed=2;
    foe.position=foe.home={-4,7};foe.attackCooldown=100;for(int i=0;i<20;++i)game->ExecuteFrame({},.01f);
    Check(game->Sustained().ActorCount()>0,"Real controls missing");
    FrozenRender(*game,renderer,out,"defense-"+std::to_string(int(id)));
  }
  auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(ClassId::Mage).Save()),"Field load");
  auto& foe=const_cast<Enemy&>(game->State().Enemies()[0]);foe.health=500;foe.attackCooldown=100;
  Command cast;cast.selectActor=100;cast.ability=8;game->ExecuteFrame(cast,.001f);
  for(int i=0;i<30;++i)game->ExecuteFrame({},.05f);Check(game->Scene().Player().frostSeconds>0,"Field not published");
  game->SetCameraOrbit(0,.95f,12);FrozenRender(*game,renderer,out,"frost-field");
}
void WorldFrames(Renderer& renderer,const std::filesystem::path& out){
  for(int chapter:{13,14,15}){
    auto state=Hero(ClassId::Warrior,chapter);
    const auto site=chapter==13?WorldMap::expeditionSites[6]:chapter==14?WorldMap::ritualSite:WorldMap::courtSeals[0];
    Vec2 position{};bool placed=false;
    for(float radius:{1.5f,2.5f,3.5f})for(int i=0;i<16 && !placed;++i){
      const Vec2 p{site.x+std::sin(i*6.2831853f/16)*radius,site.z+std::cos(i*6.2831853f/16)*radius};
      if(state.CanStand(p)){position=p;placed=true;}}
    Check(placed,"World fixture has no clear observation point");
    auto data=state.Save();const auto at=data.find('\n',data.find("PROFILE "))+1;
    std::ostringstream row;row<<position.x<<' '<<position.z<<" 100 0 3 90";
    data.replace(at,data.find('\n',at)-at,row.str());
    if(chapter==13)data=Record(data,"EXPEDITION ","EXPEDITION 1 0 0");
    if(chapter==14)data=Record(data,"SOUTHERN ","SOUTHERN 2 1 0 0");
    Check(state.Load(data),"World observation save");
    auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(state.Save()),"World observation restore");
    const auto& view=game->Scene().World();
    Check(chapter==13?view.suppliesRecovered[0] && !view.suppliesRecovered[1]:
      chapter==14?view.ritualActive && view.ritualStage==2:view.courtActive && view.captainProtected,
      "World objective publication missing authoritative progress");
    game->SetCameraOrbit(0,1.05f,14);FrozenRender(*game,renderer,out,"chapter-"+std::to_string(chapter));
  }
}
}
int main(){try{
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");Ownership();ProjectileOwnership();TrainingVisibility();
  const auto out=std::filesystem::current_path()/"scene-publication-review";std::filesystem::create_directories(out);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Scene publication checks",false),"Window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,600),"Renderer");CombatFrames(renderer,out);WorldFrames(renderer,out);
  Check(renderer.ValidateDebugMessages(),"Renderer diagnostic failure");
  std::cout<<"PASS: owned world/status/flight state, captain identity and four-class retained 3D frames.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
