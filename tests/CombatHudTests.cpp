#include "game/Game.h"
#include "game/Localization.h"
#include "game/CombatHud.h"
#include "game/ClassEffects.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
void PlateChecks() {
 ui::WorldPlates plates;
 // A dense pull, deliberately submitted in reverse importance/order.
 for(unsigned id=40;id>0;--id) plates.Add({id,640,300,144,53,float(id),id==40?4:2});
 plates.Arrange();
 Check(plates.count==24 && plates.rows[0].id==40 && plates.rows[0].visible,"Selection must survive the bounded crowd budget");
 unsigned visible=0;
 for(std::size_t i=0;i<plates.count;++i) if(plates.rows[i].visible) {
  ++visible;const auto& row=plates.rows[i];
  Check(std::abs(row.y+row.height-row.anchorY)<=64,"Labels must remain attached to their actor");
  for(std::size_t j=0;j<i;++j) if(plates.rows[j].visible)
   Check(!ui::WorldPlates::Overlaps(row,plates.rows[j]),"Crowded labels must not obscure each other's health/casts");
 }
 Check(visible>=2 && visible<plates.count,"Crowded labels must make bounded room, then cull rather than overlap");
 ui::WorldPlates reordered;
 for(unsigned id=1;id<=40;++id) reordered.Add({id,640,300,144,53,float(id),id==40?4:2});
 reordered.Arrange();
 for(std::size_t i=0;i<plates.count;++i)
  Check(reordered.rows[i].id==plates.rows[i].id && reordered.rows[i].y==plates.rows[i].y && reordered.rows[i].visible==plates.rows[i].visible,
    "Roster order must not change nameplate identity or layout");
 ui::WorldPlates edges;
 edges.Add({1,2,15,144,32,1,4});edges.Add({2,1279,550,144,53,2,2});edges.Arrange();
 for(std::size_t i=0;i<edges.count;++i) {
  const auto& row=edges.rows[i];Check(row.visible && row.x>=4 && row.x+row.width<=1276 && row.y>=8 && row.y+row.height<=578,"Screen-edge labels must remain readable");
 }
 ui::WorldPlates warnings;
 warnings.Reserve(438,24,340,132);warnings.Add({1,640,174,144,53,2,4});warnings.Arrange();
 Check(warnings.rows[0].visible && !ui::WorldPlates::Overlaps(warnings.rows[0],warnings.reserved[0]),"World casts must not show through the incoming-attack board");
}
void CastChecks() {
 for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout}) for(float hz:{30.f,60.f,144.f}) {
  rpg::Simulation state;Check(state.CreateCharacter(id,"Dawnwatch"),"Cast review character");
  for(auto& actor:const_cast<std::vector<rpg::Enemy>&>(state.Enemies())) {
   actor.position=actor.home={state.Player().x,state.Player().z+1.8f};actor.attackCooldown=10;
  }
  rpg::Command command;command.selectActor=100;command.ability=1;state.Update(.001f,command);
  Check(state.Attacking(),"Accepted class heavy attack fixture");
  const auto first=ui::ReadPlayerCast(state);
  Check((first.ability==1)==(id!=rpg::ClassId::Warrior),"Melee swings must not masquerade as spell casts");
  float previous=-1;bool sawRelease=false;
  for(int i=0;i<int(hz*3);++i) {
   const auto cast=ui::ReadPlayerCast(state);
   if(cast.ability>=0) {
    Check(cast.remaining>0 && cast.progress>=previous && cast.progress<1,"Preparation must advance monotonically on the accepted clock");
    previous=cast.progress;
   }
   if(state.Attacking() && state.AttackSeconds()>=combat::StrokeStart(state.HeavyAttack())) {
    sawRelease=true;Check(cast.ability==-1,"Released projectiles and recovery must not keep showing a false cast");
   }
   state.Update(1/hz,{});
  }
  Check(sawRelease && ui::ReadPlayerCast(state).ability==-1,"Cast must finish at every tested refresh rate");
  command.ability=1;state.Update(.001f,command);state.StopMovement();
  Check(ui::ReadPlayerCast(state).ability==-1,"Cancelled attacks must leave no stale cast meter");
 }
 rpg::Simulation engineer;Check(engineer.CreateCharacter(rpg::ClassId::Engineer,"Dawnwatch"),"Ward cast fixture");
 rpg::Command ward;ward.ability=3;engineer.Update(.01f,ward);
 Check(engineer.CastingRecovery() && ui::ReadPlayerCast(engineer).ability==3,"Real defensive casts must retain their own clock");
 engineer.StopMovement();Check(ui::ReadPlayerCast(engineer).ability==-1,"Cancelled recovery must disappear");
 rpg::Simulation mage;Check(mage.CreateCharacter(rpg::ClassId::Mage,"Dawnwatch"),"Instant ward fixture");
 mage.Update(.01f,ward);
 Check(mage.Barrier()>0 && ui::ReadPlayerCast(mage).ability==-1,"An instant mage ward must not acquire a fabricated preparation time");
}
void WorldPickChecks(Renderer& renderer) {
 rpg::Simulation state;
 auto& actors=const_cast<std::vector<rpg::Enemy>&>(state.Enemies());actors.resize(2);
 actors[0].position=actors[0].home={0,-6};actors[1].position=actors[1].home={0,-4};
 for(auto& actor:actors) {actor.state=rpg::EnemyState::Idle;actor.windup=0;}
 for(int reverse=0;reverse<2;++reverse) {
  ui::Interface hud;renderer.BeginFrame();
  renderer.SetCamera(reverse?DirectX::XMFLOAT3{0,3,4}:DirectX::XMFLOAT3{0,3,-18},reverse?DirectX::XMFLOAT3{0,1,-6}:DirectX::XMFLOAT3{0,1,-4});
  hud.Render(renderer,state,0,false);
  DirectX::XMFLOAT2 point;const auto& frontActor=actors[reverse?1:0];
  Check(renderer.Project({frontActor.position.x,1,frontActor.position.z},point),"Overlapping body projection");
  const auto hit=hud.Pointer(point.x*renderer.Width(),point.y*renderer.Height(),float(renderer.Width()),float(renderer.Height()),true,false);
  Check(hit.action==ui::Action::SelectActor && hit.value==int(frontActor.id),"Overlapping body picks must follow camera depth, including a reversed camera");
  Check(!hud.PointerOverUi(),"World actor selection must retain RMB camera input");
  Check(renderer.EndFrame(false),"World picking frame");
 }
}
void PreparationChecks(Renderer& renderer) {
 unsigned fire=0,frost=0;
 for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout}) for(int ability:{1,5}) {
  rpg::Simulation state;Check(state.CreateCharacter(id,"Dawnwatch"),"Preparation fixture");
  for(auto& actor:const_cast<std::vector<rpg::Enemy>&>(state.Enemies())) {
   actor.position=actor.home={state.Player().x,state.Player().z+1.8f};actor.attackCooldown=10;
  }
  rpg::Command cast;cast.ability=ability;cast.selectActor=100;state.Update(.01f,cast);state.Update(.04f,{});
  Check(state.Attacking(),"Preparation effect must belong to an accepted attack");
  const auto save=state.Save();renderer.BeginFrame();renderer.SetCamera({0,1,-3},{0,1,0});
  classfx::Prepare(renderer,state,{0,1,0},state.Facing());const auto vertices=renderer.EffectVertices();
  Check((vertices>0)==(id!=rpg::ClassId::Warrior) && vertices<=512,"Class preparation must be bounded; melee must not gain a fake casting halo");
  if(id==rpg::ClassId::Mage) {if(ability==1)fire=vertices;else frost=vertices;}
  Check(renderer.EndFrame(false) && state.Save()==save,"Preparation rendering must not advance gameplay");
  state.StopMovement();renderer.BeginFrame();classfx::Prepare(renderer,state,{0,1,0},state.Facing());
  Check(renderer.EffectVertices()==0 && renderer.EndFrame(false),"Cancelled preparation must not leave floating weapon effects");
 }
 Check(fire!=frost,"Fire and frost preparations must have different silhouettes");
}
}

int main(int argc,char** argv) {
 try {
  PlateChecks();CastChecks();
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  const auto output=std::filesystem::path(argc>1?argv[1]:"combat-hud-review");
  std::filesystem::create_directories(output);
  Win32Window window;Renderer renderer;
  Check(window.Create(GetModuleHandleW(nullptr),1920,1080,L"Combat HUD review",false),"Hidden review window");
  Check(renderer.Initialize(window.GetHandle(),1920,1080),"Native review renderer");
  WorldPickChecks(renderer);
  PreparationChecks(renderer);
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout}) {
   Game game;game.Initialize();
   auto& state=const_cast<rpg::Simulation&>(game.State());
   Check(state.CreateCharacter(id,"Dawnwatch"),"Review character");
   auto& actors=const_cast<std::vector<rpg::Enemy>&>(state.Enemies());
   for(unsigned i=0;i<actors.size();++i) {
    auto& actor=actors[i];actor.position=actor.home={state.Player().x+(float(i)-1)*1.7f,state.Player().z+2.2f+float(i)*.35f};
    actor.state=rpg::EnemyState::Pursuing;actor.facing=3.14159265f;
    actor.health=actor.MaxHealth()*.75f;actor.windup=actor.WindupDuration()*.8f;
   }
   Check(game.RestoreSnapshot(state.Save()),"Isolated combat snapshot must load");
   game.SetCameraOrbit(.12f,.50f,9);
   rpg::Command select;select.selectActor=100;game.ExecuteCommand(select,.01f);
   for(auto& actor:const_cast<std::vector<rpg::Enemy>&>(state.Enemies())) {
    actor.state=rpg::EnemyState::Pursuing;actor.windup=actor.WindupDuration()*.8f;
   }
   game.ExecuteCommand({},.01f);
   for(const auto size:{std::pair<int,int>{1920,1080},{960,600}}) {
    Check(renderer.Resize(size.first,size.second),"HUD review resize");
    const auto snapshot=state.Save();
    renderer.BeginFrame();game.Render(renderer);
    const auto path=output/(std::to_string(int(id))+"-"+std::to_string(size.first)+".png");
    Check(renderer.Capture(path.wstring()) && renderer.EndFrame(false),"World and HUD capture");
    Check(state.Save()==snapshot,"Rendering must not advance combat or mutate the save");
    ui::Interface hud;
    renderer.BeginFrame();hud.Render(renderer,state,0,false);Check(renderer.EndFrame(false),"Pointer review frame");
    const ui::Layout layout(float(size.first),float(size.second));
    const auto click=[&](float x,float y){return hud.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),true,false);};
    click(640,440);Check(!hud.PointerOverUi(),"The space between combat frames must remain available to world input");
    Check(click(1140,244).action==ui::Action::Journal,"The objective entry must open the real journal");
    const auto anchor=ui::CombatHudAnchors(false).player;
    click(anchor.x+100,anchor.y+49);Check(hud.PointerOverUi(),"Reserve inspection must protect underlying world clicks");
   }
   // Loading intentionally cancels old windups and restores campaign homes.
   // Start a new local pull to capture live preparation, not a stale save cast.
   const_cast<std::vector<rpg::Enemy>&>(state.Enemies()).front().position={state.Player().x-.8f,state.Player().z+1.6f};
   for(auto& actor:const_cast<std::vector<rpg::Enemy>&>(state.Enemies())) {
    actor.home=actor.position;actor.state=rpg::EnemyState::Pursuing;actor.windup=actor.WindupDuration()*.9f;
   }
   rpg::Command heavy;heavy.ability=1;game.ExecuteCommand(heavy,.01f);
   Check(state.Attacking(),"Native heavy preparation must be accepted");
   Check(renderer.Resize(1920,1080),"Cast review resize");
   renderer.BeginFrame();game.Render(renderer);
   Check(renderer.Capture((output/("casting-"+std::to_string(int(id))+".png")).wstring()) && renderer.EndFrame(false),"Live cast screenshot");
   for(int frame=0;frame<3;++frame) game.ExecuteCommand({},.04f);
   renderer.BeginFrame();game.Render(renderer);
   Check(renderer.Capture((output/("preparation-"+std::to_string(int(id))+".png")).wstring()) && renderer.EndFrame(false),"Class preparation screenshot");
  }
  Check(renderer.ValidateDebugMessages(),"HUD DirectX debug messages");
  std::cout<<"PASS: three-class native combat HUD review, two resolutions, read-only rendering.\n";
  return 0;
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
