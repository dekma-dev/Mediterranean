#include "game/CommandExecution.h"
#include "game/Game.h"
#include "engine/Audio.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void Check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
bool Has(const sound::Events& events,sound::Cue cue){return std::find(events.cues.begin(),events.cues.begin()+events.count,cue)!=events.cues.begin()+events.count;}
rpg::Simulation Hero(rpg::ClassId school) {
  rpg::Simulation state;
  Check(state.Load("MEDITERRANEAN 3\n12 -3.8 50 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Practice fixture");
  auto save=state.Save();const auto at=save.find("PROFILE ");
  save.replace(at,save.find('\n',at)-at,"PROFILE "+std::to_string(int(school))+" \"Replay\" 7 1 3");
  Check(state.Load(save),"Practice class");return state;
}
void ReceiptsAndReplay() {
  for(auto school:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest}) {
    auto state=Hero(school);rpg::CommandExecution execution;execution.BeginTrace(state,200);
    rpg::Command cast;cast.selectActor=210;cast.ability=1;cast.uiClick=true;
    const auto first=execution.Advance(state,.01f,cast);
    Check(first.sequence==1 && first.seconds==.01f && first.events.count>0,"First accepted receipt missing");
    const auto remembered=first.events.values[0];bool release=false,hit=false;
    for(int n=0;n<199;++n) {
      rpg::Command command;if(n==45)command.selectActor=211;
      if(n==140)command.jump=true;
      const auto step=execution.Advance(state,n%3==0?.01f:.02f,command);
      for(std::size_t i=0;i<step.events.count;++i){const auto kind=step.events.values[i].kind;
        release|=kind==rpg::CombatVisualKind::SpellCast||kind==rpg::CombatVisualKind::HeavyStrike;
        hit|=kind==rpg::CombatVisualKind::HeavyHit;}
    }
    Check(first.events.values[0].kind==remembered.kind && first.events.values[0].position.x==remembered.position.x,"Later simulation overwrote an owned receipt");
    const auto save=state.Save(),message=state.Message();const auto revision=execution.Stats().steps;
    const auto replay=execution.VerifyTrace();
    Check(release && hit && replay.available && replay.valid && replay.matched && replay.sealed && replay.steps==200,"Four-class command replay diverged");
    Check(state.Save()==save && state.Message()==message && execution.Stats().steps==revision,"Verification changed live state or counters");
    std::cout<<"Replay class "<<int(school)<<": "<<replay.steps<<" steps, release/contact/retarget/jump matched\n";
  }
  rpg::Simulation state;rpg::CommandExecution execution;execution.BeginTrace(state,3);
  execution.Advance(state,.01f,{});state.Notify("Unrecorded mutation");execution.Advance(state,.01f,{});
  const auto mismatch=execution.VerifyTrace();Check(mismatch.valid && !mismatch.matched && mismatch.mismatch==2,"Replay failed to detect unrecorded mutation");
  execution.InvalidateTrace();Check(!execution.VerifyTrace().valid,"External state boundary kept trace valid");
  execution.BeginTrace(state,9999);
  for(int i=0;i<260;++i)execution.Advance(state,0,{});
  Check(execution.VerifyTrace().steps==256 && execution.VerifyTrace().sealed,"Trace storage exceeded hard capacity");
  rpg::CommandExecution admission;admission.BeginTrace(state,4);
  rpg::Command jump;jump.jump=true;
  for(float seconds:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
    const auto receipt=admission.Advance(state,seconds,jump);Check(!receipt.seconds && !receipt.events.count,"Invalid time admitted gameplay/events");
  }
  Check(admission.Stats().emptySteps==4 && admission.VerifyTrace().matched,"Zero-time replay/admission accounting");
  Check(admission.Advance(state,4.f,{}).seconds==.05f,"Existing variable-step cap changed");
}
sound::Events One(sound::Cue cue,float x=0) {
  sound::Events events;events.count=1;events.cues[0]=cue;events.origins[0]={x,2};events.heights[0]=3;
  events.spatial[0]=events.environmental[0]=true;events.attachedActors[0]=100;return events;
}
void Delivery() {
  sound::DeliveryQueue queue;
  queue.Append(One(sound::Cue::FireGather,1));queue.Advance(.02f);queue.Append(One(sound::Cue::Fire,2));
  queue.Advance(.02f);queue.Append(One(sound::Cue::FireImpact,3));
  const auto frame=queue.Take();Check(frame.count==3 && frame.cues[0]==sound::Cue::FireGather && frame.cues[2]==sound::Cue::FireImpact,"Multi-step delivery lost order/cues");
  Check(frame.origins[0].x==1 && frame.origins[2].x==3 && frame.attachedActors[2]==100 && frame.heights[2]==3 && frame.spatial[2],"Delivery lost accepted emitter");
  Check(!queue.Take().count,"Repeated device update replayed accepted cues");
  queue.Append(One(sound::Cue::Fire));queue.Advance(.251f);Check(!queue.Take().count && queue.Stats().expired==1,"Stale deferred cue replayed");
  queue.Append(One(sound::Cue::Fire));queue.Reset();Check(!queue.Take().count,"Session reset retained old sound");
  for(unsigned i=0;i<120;++i)queue.Append(One(sound::Cue::StepGrass,float(i)));
  queue.Append(One(sound::Cue::BossPrepare,999));queue.Append(One(sound::Cue::FireImpact,1000));
  Check(queue.Pending()==sound::DeliveryQueue::capacity && queue.Stats().peak==96,"Unbounded sound queue");
  const auto overload=queue.Take();Check(overload.count==24 && Has(overload,sound::Cue::BossPrepare) && Has(overload,sound::Cue::FireImpact),"Footsteps displaced combat warning/impact on overload");
  Check(overload.origins[overload.count-1].x==1000 && queue.Stats().dropped>0,"Overload lost chronology or diagnostics");
}
void GameBoundary() {
  auto game=std::make_unique<Game>();game->Initialize();game->BeginCommandTrace(8);
  rpg::Command command;command.uiClick=true;
  for(int i=0;i<8;++i)game->ExecuteCommand(command,.01f);
  auto cues=game->TakeSoundEvents();Check(cues.count==8,"Actual game lost earlier commands before sound update");
  for(unsigned i=0;i<cues.count;++i)Check(cues.cues[i]==sound::Cue::UiClick,"Actual game emitted an unaccepted cue");
  Check(!game->TakeSoundEvents().count && game->VerifyCommandTrace().matched,"Game delivery repeats or command trace diverges");
  command={};command.jump=true;game->ExecuteCommand(command,.01f);game->ExecuteCommand({},.01f);
  Check(Has(game->TakeSoundEvents(),sound::Cue::Jump),"Later step erased actual jump sound");
  command={};command.uiClick=true;game->ExecuteCommand(command);game->SuspendInput();Check(!game->TakeSoundEvents().count,"Focus loss retained sound queue");
  game->ExecuteCommand(command);const auto save=game->State().Save();Check(game->RestoreSnapshot(save) && !game->TakeSoundEvents().count,"Load retained previous-world sound");
  game->BeginCommandTrace();game->ExecuteCommand(command);game->SuspendInput();Check(!game->VerifyCommandTrace().valid,"Suspend bypassed trace boundary");
  game->ExecuteCommand(command);command={};command.restart=true;game->ExecuteCommand(command);
  Check(!game->TakeSoundEvents().count,"Restart replayed previous-world sound");
  const auto& stats=game->StepStats();Check(stats.steps>10 && stats.events>=8 && stats.simulationSeconds>0 && stats.presentationSeconds>0,"Phase diagnostics missing");
  std::cout<<game->StepDiagnostics();
}
void EventCapacity() {
  // Pure emission can be exercised through real UI-click commands. Actual
  // accepted receipts remain bounded even with repeated consumers.
  rpg::Simulation state;rpg::CommandExecution execution;rpg::Command command;command.uiClick=true;
  auto receipt=execution.Advance(state,.01f,command);state.Update(.01f,{});
  sound::Events sounds;sounds.Update(state,nullptr,&receipt.events);
  Check(Has(sounds,sound::Cue::UiClick),"Sound converter reread erased simulation events instead of receipt");
  receipt.events.count=receipt.events.values.size();
  for(auto& event:receipt.events.values){event.kind=rpg::CombatVisualKind::UiClick;event.position=state.Player();}
  sounds.Update(state,nullptr,&receipt.events);Check(sounds.count==24 && sounds.dropped==40,"Per-step sound truncation was not accounted");
  receipt.events.values.back().kind=rpg::CombatVisualKind::BossPrepare;
  sounds.Update(state,nullptr,&receipt.events);Check(Has(sounds,sound::Cue::BossPrepare) && sounds.dropped==40,"One-step incidental cues displaced a boss warning");
  // Real simultaneous telegraphs exceeded the former sixteen-event buffer.
  rpg::Simulation crowded;Check(crowded.Load("MEDITERRANEAN 1\n-4 5.2 100 3.14159 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n"),"Crowded fixture");
  for(const auto& view:crowded.Combatants()) {
    auto& enemy=const_cast<rpg::Enemy&>(view); // Fixture-owned non-const simulation.
    enemy.kind=rpg::EnemyKind::Raider;enemy.position=enemy.home={-4,7};enemy.health=80;
    enemy.state=rpg::EnemyState::Pursuing;enemy.facing=3.14159265f;enemy.windup=enemy.attackCooldown=0;
    enemy.hasSeenPlayer=true;enemy.lastSeenPlayer=crowded.Player();
  }
  const auto crowd=execution.Advance(crowded,.01f,{});
  Check(crowd.events.count>16 && crowd.events.count<=rpg::Simulation::visualEventCapacity && !crowd.events.dropped,"Simultaneous warnings still use the former small event buffer");
  std::cout<<"Simultaneous accepted warning events: "<<crowd.events.count<<'\n';
}
}
int main()try {
  ReceiptsAndReplay();Delivery();GameBoundary();EventCapacity();
  std::cout<<"PASS: owned step receipts, bounded replay, one-shot sound delivery, overload, expiry and lifecycle resets\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
