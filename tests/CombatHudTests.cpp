#include "game/Game.h"
#include "game/Localization.h"
#include "game/CombatHud.h"
#include "game/CombatText.h"
#include "game/ClassEffects.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
void CombatTextChecks() {
  rpg::CombatFeedback feedback{{1,2},rpg::FeedbackKind::Damage,72,.3f,9};
  const auto before=ui::ReadCombatText(feedback);
  std::vector<rpg::CombatFeedback> rows{feedback,feedback};rows[0].sequence=8;
  rows.erase(rows.begin());const auto after=ui::ReadCombatText(rows[0]);
  Check(before.x==after.x && before.y==after.y && before.opacity==after.opacity,"Expiring a neighbour moved an existing combat number");
  Check(before.text=="72" && before.opacity==1,"Combat text changed accepted damage or lost its readable hold");
  feedback.age=0;Check(ui::ReadCombatText(feedback).opacity==0,"Combat number must start without an opaque pop");
  feedback.age=1.49f;Check(ui::ReadCombatText(feedback).opacity<.01f,"Combat number vanished without a tail fade");
  feedback.age=1.5f;Check(ui::ReadCombatText(feedback).opacity==0,"Expired combat number remained visible");
  for(float hz:{30.f,60.f,144.f}) {
    feedback.age=0;for(int i=0;i<int(hz);++i)feedback.age+=1/hz;
    const auto view=ui::ReadCombatText(feedback);
    Check(std::abs(view.y-(-14-72*(1-std::exp(-1.3f))))<.001f,"Combat number motion depends on frame rate");
  }
  feedback.age=.3f;feedback.kind=rpg::FeedbackKind::Hurt;const auto hurt=ui::ReadCombatText(feedback);
  feedback.kind=rpg::FeedbackKind::Heal;const auto heal=ui::ReadCombatText(feedback);
  Check(hurt.text=="-72" && heal.text=="+72" && hurt.x<before.x && heal.x>before.x,"Incoming damage and healing share the outgoing number channel");
  rpg::Simulation state;state.CreateCharacter(rpg::ClassId::Mage,"Feedback");
  for(auto& actor:const_cast<std::vector<rpg::Enemy>&>(state.Enemies())){actor.position=actor.home={state.Player().x,state.Player().z+1.8f};actor.attackCooldown=100;}
  rpg::Command command;command.selectActor=100;command.ability=0;
  state.Update(.01f,command);bool hit=false;
  for(int n=0;n<300 && !hit;++n){state.Update(.01f,{});for(const auto& entry:state.Feedback())if(entry.kind==rpg::FeedbackKind::Damage){hit=true;Check(entry.sequence>0,"Actual accepted hit did not receive stable presentation identity");}}
  Check(hit,"Combat number integration fixture never hit its target");
}
void AbilityFeedbackChecks(Renderer* renderer=nullptr,const std::filesystem::path& output={}) {
 for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest})for(int hz:{30,60,144}) {
  rpg::Simulation state;state.CreateCharacter(id,"Spell feedback");ui::Interface hud;
  auto& enemy=const_cast<rpg::Enemy&>(state.Enemies()[0]);enemy.position=enemy.home={state.Player().x,state.Player().z+1.8f};
  enemy.attackCooldown=100;enemy.health=1000;
  hud.UpdateCombatFeedback(.001f,state);Check(hud.AbilitySignals().Ready(0)==0,"Loading a ready ability flashes every icon");
  rpg::Command cast;cast.selectActor=enemy.id;cast.ability=0;state.Update(.001f,cast);hud.UpdateCombatFeedback(.001f,state);
  Check(state.Attacking() && hud.AbilitySignals().Used(0)>0,"Accepted cast has no icon receipt");
  bool returned=false;
  for(int n=0;n<hz*6 && !returned;++n){state.Update(1.f/hz,{});hud.UpdateCombatFeedback(1.f/hz,state);returned=hud.AbilitySignals().Ready(0)>0;}
  Check(returned && state.Cooldown(0)==0 && hud.AbilitySignals().Ready(1)==0,"Cooldown return missing or attached to another ability");
  for(int n=0;n<int(.14f*hz);++n){state.Update(1.f/hz,{});hud.UpdateCombatFeedback(1.f/hz,state);}
  const float pulse=hud.AbilitySignals().Ready(0);hud.MoveHotbar(0,9,id);
  Check(hud.HotbarSpell(9,id)==0 && hud.AbilitySignals().Ready(hud.HotbarSpell(9,id))==pulse,"Moving a spell detached its feedback");
  if(renderer && hz==60) {
    const auto save=state.Save();renderer->BeginFrame();hud.Render(*renderer,state,3,false);
    Check(renderer->Capture((output/("cooldown-return-"+std::to_string(int(id))+".png")).wstring()) && renderer->EndFrame(false),"Cooldown receipt capture");
    renderer->BeginFrame();hud.Render(*renderer,state,3,false);Check(renderer->EndFrame(false) && pulse==hud.AbilitySignals().Ready(0) && save==state.Save(),"Rendering advanced ability feedback or gameplay");
  }
  for(int n=0;n<hz;++n){state.Update(1.f/hz,{});hud.UpdateCombatFeedback(1.f/hz,state);}
  Check(hud.AbilitySignals().Ready(0)==0 && hud.AbilitySignals().Used(0)==0,"Ability receipt did not expire");
  hud.ResetCombatFeedback();hud.UpdateCombatFeedback(.001f,state);Check(hud.AbilitySignals().Ready(0)==0,"Reset replayed a cooldown return");
 }
 rpg::Simulation rejected;rejected.CreateCharacter(rpg::ClassId::Mage,"Rejected");ui::AbilityFeedback view;
 rejected.Update(.01f,{});view.Update(.01f,rejected);rpg::Command miss;miss.ability=0;
 rejected.Update(.01f,miss);view.Update(.01f,rejected);Check(view.Used(0)==0,"Rejected cast illuminated its icon");
 view.Update(NAN,rejected);Check(view.Ready(0)==0 && view.Used(0)==0,"Invalid clock retained receipts");
}
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
void CastReceiptChecks() {
 for(auto id:{rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest})for(float hz:{30.f,60.f,144.f}) {
  rpg::Simulation state;state.CreateCharacter(id,"Tester");
  for(auto& actor:const_cast<std::vector<rpg::Enemy>&>(state.Enemies())) {
   actor.position=actor.home={state.Player().x,state.Player().z+1.8f};actor.attackCooldown=100;
  }
  ui::CastReceipt receipt;rpg::Command attack;attack.selectActor=100;attack.ability=1;
  state.Update(.001f,attack);receipt.Update(.001f,state);
  Check(receipt.pending==1 && receipt.Opacity()==0,"Preparation must not show a release receipt");
  bool emitted=false;
  for(int n=0;n<int(hz*4) && !emitted;++n){state.Update(1/hz,{});receipt.Update(1/hz,state);emitted=receipt.ability==1;}
  Check(emitted && !receipt.interrupted && receipt.Opacity()>0,"Real projectile release lost its receipt");
  // Cancelling follow-through cannot retroactively label a successful cast interrupted.
  state.StopMovement();receipt.Update(.001f,state);
  Check(!receipt.interrupted,"Post-release movement changed a successful cast into an interruption");
  for(int n=0;n<int(hz*5);++n){state.Update(1/hz,{});receipt.Update(1/hz,state);}
  Check(receipt.Opacity()==0 && receipt.ability<0,"Release receipt never expired");
  state.Update(.001f,attack);receipt.Update(.001f,state);Check(receipt.pending==1,"Interruption fixture failed");
  state.StopMovement();receipt.Update(.001f,state);
  Check(receipt.interrupted && receipt.ability==1 && receipt.Opacity()>0,"Interrupted preparation vanished without feedback");
  state.Update(1/hz,{});receipt.Update(1/hz,state);
  Check(receipt.interrupted,"Subsequent quiet frame rewrote interruption");
  receipt.Reset();Check(receipt.Opacity()==0 && receipt.pending<0,"Load/reset retained a cast receipt");
 }
 rpg::Simulation mage;mage.CreateCharacter(rpg::ClassId::Mage,"Tester");ui::CastReceipt receipt;
 rpg::Command ward;ward.ability=3;mage.Update(.01f,ward);receipt.Update(.01f,mage);
 Check(receipt.Opacity()==0,"Instant protection pretended to finish a cast");
}
void TargetReceiptChecks(Renderer& renderer,const std::filesystem::path& output) {
 using R=ui::TargetCastReceipt::Result;
 for(int mode=0;mode<5;++mode)for(float hz:{30.f,60.f,144.f}) {
  auto game=std::make_unique<Game>();game->Initialize();
  auto& state=const_cast<rpg::Simulation&>(game->State());
  auto& actors=const_cast<std::vector<rpg::Enemy>&>(state.Enemies());
  for(auto& actor:actors){actor.health=0;actor.windup=0;}
  auto& target=actors[0];target.health=target.MaxHealth();target.position=target.home={state.Player().x,state.Player().z+1.8f};
  target.state=rpg::EnemyState::Pursuing;target.attackCooldown=100;
  rpg::Command select;select.selectActor=target.id;game->ExecuteCommand(select,.001f);
  if(mode==4){target.kind=rpg::EnemyKind::Captain;target.rank=2;target.bossAttack=1;target.health=target.MaxHealth();}
  target.windup=target.WindupDuration();target.facing=0; // A normal swing will miss the player behind it.
  ui::Interface hud;hud.UpdateCombatFeedback(.001f,state);
  Check(hud.TargetCastSignal().pending && hud.TargetCastSignal().Opacity()==0,"A live enemy windup fabricated a result");
  const float health=state.Health();
  if(mode==2){target.windup=0;hud.UpdateCombatFeedback(.001f,state);}
  else if(mode==3){target.position.x+=20;hud.UpdateCombatFeedback(.001f,state);}
  else {
   if(mode==0 || mode==4){rpg::Command attack;attack.ability=1;game->ExecuteCommand(attack,.001f);hud.UpdateCombatFeedback(.001f,state);}
   for(int n=0;n<int(hz*3) && hud.TargetCastSignal().Opacity()==0;++n){
    game->ExecuteCommand({},1/hz);hud.UpdateCombatFeedback(1/hz,state);
   }
  }
  const auto expected=mode==0?R::Interrupted:mode==2?R::Stopped:mode==3?R::None:R::Released;
  Check(hud.TargetCastSignal().result==expected,"Enemy receipt disagrees with accepted interrupt/release/visibility state");
  if(mode==1)Check(state.Health()==health,"Missed-release fixture accidentally hit the player");
  if(mode==0)Check(target.stunned==0 && target.windup==0,"Interrupt test did not end the committed windup");
  if(hz==60 && mode<3) {
   // Stable debuff columns retain their identity while earlier effects expire.
   target.rooted=2.4f;target.snared=1.3f;target.exposed=.65f;
   for(auto size:{std::pair<int,int>{1920,1080},{960,600}}) {
    Check(renderer.Resize(size.first,size.second),"Receipt resize");
    const auto save=state.Save();const float age=hud.TargetCastSignal().age;
    game->SetCameraOrbit(.12f,.5f,7);renderer.BeginFrame();game->Render(renderer,false);hud.Render(renderer,state,0,false);
    Check(renderer.Capture((output/("target-receipt-"+std::to_string(mode)+"-"+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Target result native capture");
    Check(age==hud.TargetCastSignal().age && save==state.Save(),"Rendering advanced the enemy result or simulation");
   }
  }
  target.windup=0;target.attackCooldown=100;
  for(int n=0;n<int(hz);++n){game->ExecuteCommand({},1/hz);hud.UpdateCombatFeedback(1/hz,state);}
  Check(hud.TargetCastSignal().Opacity()==0,"Enemy result did not expire after one second");
  target.position=target.home={state.Player().x,state.Player().z+1.8f};target.state=rpg::EnemyState::Pursuing;target.windup=.4f;
  hud.UpdateCombatFeedback(.001f,state);Check(hud.TargetCastSignal().pending,"New windup did not replace the old result");
  hud.ResetCombatFeedback();Check(!hud.TargetCastSignal().pending && hud.TargetCastSignal().actor==0,"Load/reset retained target feedback");
  hud.UpdateCombatFeedback(.001f,state);target.health=0;hud.UpdateCombatFeedback(.001f,state);
  Check(hud.TargetCastSignal().actor==0,"Dead target retained a cast result");
 }
 {
  rpg::Simulation state;ui::TargetCastReceipt receipt;
  auto& actors=const_cast<std::vector<rpg::Enemy>&>(state.Enemies());
  for(auto& actor:actors){actor.position=actor.home={state.Player().x,state.Player().z+1.8f};actor.attackCooldown=100;}
  rpg::Command select;select.selectActor=actors[0].id;state.Update(.001f,select);
  actors[0].state=rpg::EnemyState::Pursuing;actors[0].windup=.4f;receipt.Update(.001f,state);
  actors[0].windup=0;receipt.Update(.001f,state);Check(receipt.result==R::Stopped,"Switch fixture has no receipt");
  select.selectActor=actors[1].id;state.Update(.001f,select);receipt.Update(.001f,state);
  Check(receipt.actor==actors[1].id && receipt.result==R::None && !receipt.pending,"Switching targets carried another actor's cast result");
  receipt.Update(.5f,state);Check(receipt.actor==0 && receipt.Opacity()==0,"Discontinuous simulation retained enemy feedback");
  actors[1].kind=rpg::EnemyKind::Dummy;receipt.Update(.001f,state);Check(receipt.actor==0,"A practice dummy showed enemy cast feedback");
 }
 rpg::Enemy enemy;enemy.health=100;enemy.stunned=2;enemy.rooted=3;enemy.snared=4;enemy.exposed=5;
 const auto before=ui::TargetStatusEffects(enemy);enemy.stunned=enemy.snared=0;const auto after=ui::TargetStatusEffects(enemy);
 Check(before.size()==4 && after.size()==2 && before[1].slot==after[0].slot && before[3].slot==after[1].slot,"Expiring effects moved unrelated target status slots");
 enemy.health=0;Check(ui::TargetStatusEffects(enemy).empty(),"A corpse kept status icons");
 Check(renderer.Resize(1920,1080),"Restore HUD size");
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
  CombatTextChecks();
  PlateChecks();CastChecks();CastReceiptChecks();
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  const auto output=std::filesystem::path(argc>1?argv[1]:"combat-hud-review");
  std::filesystem::create_directories(output);
  Win32Window window;Renderer renderer;
  Check(window.Create(GetModuleHandleW(nullptr),1920,1080,L"Combat HUD review",false),"Hidden review window");
  Check(renderer.Initialize(window.GetHandle(),1920,1080),"Native review renderer");
  TargetReceiptChecks(renderer,output);
  AbilityFeedbackChecks(&renderer,output);
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
    click(600,570);Check(!hud.PointerOverUi(),"Old utility row still blocks the space above the hotbars");
    const ui::Action actions[]={ui::Action::Character,ui::Action::Inventory,ui::Action::Build,ui::Action::Map,ui::Action::Journal,ui::Action::Menu};
    for(int i=0;i<6;++i)Check(click(1052+i*39.f,694).action==actions[i],"Lower-edge utility shortcut did not dispatch its real panel action");
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
   for(int tick=0;tick<500 && state.Feedback().empty();++tick)game.ExecuteCommand({},.01f);
   for(int tick=0;tick<14;++tick)game.ExecuteCommand({},.01f);
   Check(!state.Feedback().empty(),"Live combat-number capture has no accepted contact");
   const auto unchanged=state.Save();
   renderer.BeginFrame();game.Render(renderer);
   Check(renderer.Capture((output/("contact-text-"+std::to_string(int(id))+".png")).wstring()) && renderer.EndFrame(false),"Live contact number screenshot");
   Check(state.Save()==unchanged,"Combat numbers advanced simulation during rendering");
  }
  Check(renderer.ValidateDebugMessages(),"HUD DirectX debug messages");
  std::cout<<"PASS: three-class native combat HUD review, two resolutions, read-only rendering.\n";
  return 0;
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
