#include "game/CommandExecution.h"
#include "game/Game.h"
#include "game/AttackWarning.h"
#include "game/CastEffects.h"
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
void SceneOwnership() {
  presentation::SceneSnapshot scene;auto state=Hero(rpg::ClassId::Mage);
  rpg::Command cast;cast.selectActor=210;cast.ability=1;state.Update(.01f,cast);scene.Capture(state,7);
  Check(scene.Target() && scene.Target()->id==210 && scene.Player().attacking && scene.Player().classId==rpg::ClassId::Mage,
    "Snapshot missed committed cast/target identity");
  const auto frozen=scene;const auto* storage=scene.Combatants().data();
  const auto bytes=scene.Bytes(),growths=scene.StorageGrowths();
  // Destroy the source's roster/target storage while retaining an owned frame.
  state=rpg::Simulation{};
  for(int i=0;i<300;++i){state.Update(.01f,{});scene.Capture(state,8+i);}
  Check(frozen.Sequence()==7 && frozen.Player().attacking && frozen.Target()->id==210 && frozen.Player().attackSeconds<.02f,
    "Publication retained borrowed live combat state");
  Check(!scene.Target() && !scene.Player().attacking && scene.Sequence()==307 && scene.Combatants().data()==storage &&
    scene.Bytes()==bytes && scene.StorageGrowths()==growths,"Ordinary publications grew storage or retained an old target/action");
  std::vector<presentation::Actor> actors;auto roster=frozen.Combatants();
  // The practice save contains cleared campaign corpses; make this isolated
  // lifecycle fixture alive before checking a real alive -> dead -> alive cycle.
  for(auto& e:roster)e.health=80;
  presentation::Bind(actors,roster,frozen.Trainer());
  actors[0].motion.phase=2.25f;const auto id=actors[0].id;
  std::swap(roster[0],roster[1]);presentation::Bind(actors,roster,frozen.Trainer());
  Check(actors[1].id==id && actors[1].motion.phase==2.25f,"Snapshot reorder swapped actor animation");
  const auto alive=roster[1];roster[1].health=0;presentation::Bind(actors,roster,frozen.Trainer());
  Check(actors[1].motion.phase==2.25f,"Snapshot death erased the outgoing pose");
  roster[1]=alive;presentation::Bind(actors,roster,frozen.Trainer());Check(actors[1].motion.phase==0,"Snapshot revival inherited previous life");
  actors[1].motion.phase=1;roster[1].home.x+=10;presentation::Bind(actors,roster,frozen.Trainer());
  Check(actors[1].motion.phase==0,"Reused snapshot ID kept a different spawn's pose");
  rpg::Enemy enemy;enemy.id=17;enemy.kind=rpg::EnemyKind::Wraith;enemy.position={3,-10};
  presentation::ActorFrame captured(enemy);presentation::Actor actor;actor.Reset(captured);
  const auto before=castfx::FromEnemy(captured,actor);enemy.position={100,100};enemy.kind=rpg::EnemyKind::Boar;enemy.health=0;
  const auto after=castfx::FromEnemy(captured,actor);
  Check(before.focus.x==after.focus.x && before.focus.y==after.focus.y && before.focus.z==after.focus.z && after.alive,
    "Spell socket followed a mutated authoritative actor");
  std::cout<<"Owned scene: "<<scene.Combatants().size()<<" actors, "<<scene.Bytes()<<" bytes, "<<scene.StorageGrowths()<<" growth, 300 stable publications\n";
}
void SceneWarnings() {
  rpg::Simulation state;
  Check(state.Load("MEDITERRANEAN 1\n-4 5.2 100 3.14159 0 0\n0 0 0 0\n-4 7 80 0\n4 9 80 0\n0 12 80 0\n"),"Warning scene fixture");
  unsigned n=0;for(const auto& view:state.Combatants()) {
    auto& e=const_cast<rpg::Enemy&>(view);e.position={-4.f+float(n%3)*.1f,7};
    e.kind=n%3==0?rpg::EnemyKind::Captain:rpg::EnemyKind::Wraith;e.rank=1;e.state=rpg::EnemyState::Pursuing;
    e.health=e.MaxHealth();e.facing=3.14159265f;e.windup=.15f+.01f*float(n++);e.bossAttack=n%3;
  }
  const auto selected=rpg::ReadAttackWarnings<presentation::WarningBatch::capacity>(state);
  presentation::SceneSnapshot scene;scene.Capture(state,19);
  Check(selected.count==8 && scene.Warnings().count==8,"Crowded snapshot warning budget/visibility differs");
  for(unsigned i=0;i<selected.count;++i) {
    const auto& original=*selected.rows[i].enemy;const auto& captured=scene.Warnings().rows[i];
    Check(captured.actor.id==original.id && captured.threatens==selected.rows[i].threatens && captured.reach==original.WarningReach() &&
      captured.halfAngle==original.HalfAngle() && captured.interruptible==original.Interruptible(),
      "Published warning changed real damage footprint/order");
  }
  const auto warnings=scene.Warnings();state=rpg::Simulation{};scene.Capture(state,20);
  Check(!scene.Warnings().count && warnings.count==8 && warnings.rows[0].actor.windup>0,"Warning batch borrowed a reset world or leaked after reset");
}
void SceneLifecycle() {
  auto game=std::make_unique<Game>();game->Initialize();
  Check(game->Scene().Revision()>0 && game->Scene().Combatants().size()==game->State().Combatants().size(),"First render has no actor publication");
  for(auto school:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest}) {
    Check(game->RestoreSnapshot(Hero(school).Save()),"Scene class load");
    rpg::Command cast;cast.selectActor=210;cast.ability=1;game->ExecuteCommand(cast,.01f);
    Check(game->Scene().Player().attacking && game->Scene().Target() && game->Scene().Target()->id==210 &&
      game->Scene().Player().classId==school && game->Scene().Player().outfit.classId==school,"Game scene missed committed class/target/action");
    const auto sequence=game->Scene().Sequence(),revision=game->Scene().Revision();game->SuspendInput();
    Check(game->Scene().Revision()>revision && game->Scene().Sequence()==sequence && !game->Scene().Player().attacking &&
      game->Scene().Player().attackSeconds<0,"Focus loss leaves an unpublished cancelled cast");
    const auto validRevision=game->Scene().Revision();
    Check(!game->RestoreSnapshot("broken") && game->Scene().Revision()==validRevision,"Rejected save replaced the published scene");
    rpg::Command jump;jump.jump=true;game->ExecuteCommand(jump,.02f);
    Check(!game->Scene().Player().grounded && game->Scene().Player().height>0,"Accepted jump was not published with its motion state");
    Check(game->RestoreSnapshot(Hero(school).Save()) && game->Scene().Player().grounded && game->Scene().Player().height==0 &&
      !game->Scene().Target(),"Load retained airborne/target state");
  }
  rpg::Command restart;restart.restart=true;game->ExecuteCommand(restart,.01f);
  Check(game->Scene().Player().health==game->State().Health() && !game->Scene().Player().attacking,"Restart reset animation against an old publication");
}
void FrameScheduling() {
  rpg::CommandSchedule schedule;rpg::Command command;
  command.movement={.4f,.8f};command.sprint=command.steerFacing=command.queueAttack=true;command.facingYaw=.7f;
  command.jump=command.nextTarget=command.interact=command.usePotion=command.equipWeapon=command.uiClick=true;
  command.selectActor=210;command.selectThreat=100;command.ability=1;command.purchase=0;command.collectLoot=true;
  command.inventory={rpg::InventoryAction::Sort};command.trade={rpg::TradeAction::Sell,rpg::Item::Potion,0};
  command.questAction=rpg::QuestAction::AcceptPatrol;command.learnTalent=0;command.resetTalents=true;
  const auto plan=schedule.Begin(.05f,command);
  Check(plan.count==6 && plan.seconds<=rpg::CommandSchedule::maximumStep && std::abs(plan.seconds*plan.count-.05f)<1e-7f,
    "Long frame was not fully admitted into bounded short steps");
  for(unsigned i=1;i<plan.count;++i){const auto& c=plan.At(i);
    Check(c.movement.x==.4f && c.movement.z==.8f && c.sprint && c.steerFacing && c.facingYaw==.7f,
      "Held movement/steering disappeared in a subdivided frame");
    Check(!c.jump && !c.nextTarget && !c.interact && !c.usePotion && !c.equipWeapon && !c.uiClick && !c.collectLoot &&
      !c.selectActor && !c.selectThreat && c.ability==-1 && c.purchase==-1 && c.inventory.action==rpg::InventoryAction::None &&
      c.trade.action==rpg::TradeAction::None && c.questAction==rpg::QuestAction::None && c.learnTalent==-1 && !c.resetTalents,
      "A one-shot action leaked into a continuation step");
  }
  for(float bad:{0.f,-1.f,NAN,INFINITY})Check(schedule.Begin(bad,command).count==0,"Invalid frame admitted gameplay");
  Check(schedule.Begin(.001f,command).count==1 && schedule.Begin(8,command).count==6,"Fast/overlong frame escaped bounds");
  auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(rpg::ClassId::Warrior).Save()),"Frame hero");
  const auto weapon=game->State().Items().Worn(rpg::GearSlot::Weapon);
  command={};command.equipWeapon=true;command.uiClick=true;const auto receipt=game->ExecuteFrame(command,.05f);
  const auto sounds=game->TakeSoundEvents();unsigned clicks=0;for(unsigned i=0;i<sounds.count;++i)clicks+=sounds.cues[i]==sound::Cue::UiClick;
  Check(clicks==1 && game->State().Items().Worn(rpg::GearSlot::Weapon)!=weapon && game->ScheduleStats().steps==6,
    "Long frame repeated an equipment toggle/click or skipped continuation");
  Check(receipt.Has(rpg::CombatVisualKind::UiClick) && receipt.events>0 && receipt.steps==6 && !receipt.droppedEvents &&
    !game->State().VisualEventCount(),"Frame summary lost an early substep event or confused it with the last tick");
  Check(game->RestoreSnapshot("MEDITERRANEAN 3\n3 -10 140 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n"),"Frame shop");
  command={};command.interact=true;const auto opened=game->ExecuteFrame(command,.05f);
  Check(opened.tradeRequested && !game->State().TradeRequested(),"Early-step merchant request was lost before UI dispatch");
  const auto gold=game->State().Gold(),potions=game->State().Potions();command={};command.purchase=0;
  const auto bought=game->ExecuteFrame(command,.05f);
  Check(game->State().Gold()==gold-6 && game->State().Potions()==potions+1 && !bought.notice.empty(),
    "Long frame repeated a purchase or lost its receipt");
  command={};command.restart=true;command.movement={1,1};const auto cancelled=game->ScheduleStats().cancelledSteps;
  const auto reset=game->ExecuteFrame(command,.05f);
  Check(game->State().Player().x==rpg::WorldMap::spawn.x && game->State().Player().z==rpg::WorldMap::spawn.z &&
    game->ScheduleStats().cancelledSteps==cancelled+5 && reset.boundary && reset.steps==1,
    "Outgoing held controls moved the replacement world");
  const auto save=game->State().Save();game->ExecuteFrame(command,0);Check(game->State().Save()==save,"Empty frame executed a one-shot");
}
void ScheduledCadence() {
  // Observe authoritative contact offsets, not floating combat-text ages (which
  // intentionally start at the end of their emission tick).
  for(auto school:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest}) {
    double referenceTime=-1;int referenceDamage=-1;
    for(int cadence:{120,60,30,20,144,240,1000,-20}) {
      const bool scheduled=cadence>0;const int hz=std::abs(cadence);
      auto game=std::make_unique<Game>();game->Initialize();Check(game->RestoreSnapshot(Hero(school).Save()),"Cadence hero");
      std::uint64_t last=0;int damage=0;double first=-1;
      for(int n=0;n<hz*2;++n) {
        rpg::Command c;if(n==0){c.selectActor=210;c.ability=1;c.queueAttack=true;}
        if(scheduled) {
          const auto receipt=game->ExecuteFrame(c,1.f/hz);
          for(auto kind:{rpg::CombatVisualKind::Hit,rpg::CombatVisualKind::HeavyHit})
            if(first<0 && receipt.Has(kind))first=double(n)/hz+receipt.FirstOffset(kind);
        } else {
          const auto receipt=game->ExecuteCommand(c,1.f/hz);
          for(std::size_t i=0;i<receipt.events.count;++i){const auto& e=receipt.events.values[i];
            if(first<0 && (e.kind==rpg::CombatVisualKind::Hit || e.kind==rpg::CombatVisualKind::HeavyHit))
              first=double(n)/hz+receipt.seconds-e.elapsed;}
        }
        game->TakeSoundEvents();
        for(const auto& f:game->State().Feedback())if(f.sequence>last){
          last=f.sequence;if(f.kind==rpg::FeedbackKind::Damage)damage+=f.amount;}
      }
      Check(damage>0 && first>=0,"Cadence check did not exercise a real contact");
      if(hz==120){referenceTime=first;referenceDamage=damage;}
      std::cout<<(scheduled?"Scheduled":"Legacy single-step")<<" class "<<int(school)<<" at "<<hz<<" Hz: contact "<<first<<", damage "<<damage<<'\n';
      if(scheduled) {
        Check(damage==referenceDamage && std::abs(first-referenceTime)<(hz<=120?.0001:rpg::CommandSchedule::maximumStep+.0001),
          "Display cadence changes damage or escapes bounded input quantization");
        Check(game->ScheduleStats().steps==unsigned(std::max(120,hz)*2),"Admitted duration produced incorrect bounded steps");
      }
    }
  }
  rpg::Vec2 reference{};float height=0,vertical=0;
  for(int hz:{120,60,30,20}) {
    auto game=std::make_unique<Game>();game->Initialize();
    for(int n=0;n<hz;++n){rpg::Command c;c.movement={.3f,1};c.sprint=true;c.jump=n==hz/2;game->ExecuteFrame(c,1.f/hz);game->TakeSoundEvents();}
    const auto p=game->State().Player();
    if(hz==120){reference=p;height=game->State().Height();vertical=game->State().VerticalSpeed();}
    Check(std::hypot(p.x-reference.x,p.z-reference.z)<.0001f && std::abs(game->State().Height()-height)<.0001f &&
      std::abs(game->State().VerticalSpeed()-vertical)<.0001f,"Display cadence changes real sprint/jump trajectory");
  }
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
  ReceiptsAndReplay();Delivery();GameBoundary();EventCapacity();SceneOwnership();SceneWarnings();SceneLifecycle();FrameScheduling();ScheduledCadence();
  std::cout<<"PASS: owned step receipts, bounded replay, one-shot sound delivery, overload, expiry and lifecycle resets\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
