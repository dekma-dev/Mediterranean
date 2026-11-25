#include "game/CastEffects.h"
#include "game/Localization.h"
#include "game/Game.h"
#include "game/CourtView.h"
#include "game/ProfileStorage.h"

#include "engine/Renderer.h"
#include "game/WorldView.h"
#include "game/AuthoredActorMotion.h"
#include "game/AuthoredLocomotion.h"
#include "game/CharacterView.h"
#include "game/HandPose.h"
#include "game/CharacterPose.h"
#include "game/ClassEffects.h"
#include "game/TerrainSurface.h"
#include "game/ExpeditionView.h"
#include "game/PopulationView.h"
#include "game/CombatText.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace {
float EnemyStrike(const presentation::ActorFrame& enemy,bool active,float releaseAge=1) {
  // Cooldowns are frozen while defeated or outside a sparring round. They must
  // not keep driving the presentation toward an unfinished strike indefinitely.
  if (!active || enemy.health<=0) return 0;
  return enemy.windup>0?combat::Smooth(1-enemy.windup/enemy.WindupDuration()):
    1-combat::Smooth(releaseAge/.4f);
}
float EnemyAttackSeconds(const presentation::ActorFrame& enemy,bool active,float releaseAge) {
  if (!active || enemy.health<=0) return -1;
  const bool heavy=enemy.kind==rpg::EnemyKind::Captain || enemy.kind==rpg::EnemyKind::Reaver || enemy.kind==rpg::EnemyKind::Wraith;
  const float contact=enemy.kind==rpg::EnemyKind::Wraith?combat::StrokeStart(true):combat::Contact(heavy),start=contact-.09f;
  if (enemy.windup>0) {
    if (enemy.windup<=.09f) return contact-enemy.windup;
    return start*combat::Smooth((enemy.WindupDuration()-enemy.windup)/(enemy.WindupDuration()-.09f));
  }
  const float age=contact+releaseAge;
  return age<combat::Duration(heavy)?age:-1.f;
}
}

void Game::Initialize() {
  saveFile_=std::filesystem::path(RPG_SAVE_DIRECTORY)/"patrol.save";
  rosterFile_=saveFile_;
  savedSnapshot_.reset();
  choosingCharacter_=inputBarrier_=false; quitRequested_=fullscreenRequest_=false;
  simulation_=rpg::Simulation{}; time_=0; atmosphereTime_=0;
  execution_={};schedule_={};scene_={};soundEvents_.Reset();soundDelivery_={};
  // Construct the immutable particle collision grid during loading, not on
  // the first accepted hit. No scenery allocations occur during combat.
  (void)impactfx::Environment();
  worldView_.Reset();
  interface_.SetOnly(ui::Panel::None); LoadHotbarSettings(); CancelOrbit();
  yaw_=0; pitch_=.45f; distance_=10;
  ResetMotion();
}

void Game::ChooseCharacter(const std::filesystem::path& save) {
  execution_.InvalidateTrace();soundEvents_.Reset();soundDelivery_.Reset();
  rosterFile_=save;LoadHotbarSettings();
  CancelUiGesture(); selection_.Open(save); choosingCharacter_=true; inputBarrier_=false;
}

bool Game::RestoreSnapshot(const std::string& snapshot) {
  if (!simulation_.Load(snapshot)) return false;
  execution_.InvalidateTrace();
  worldView_.ResetWildlife();
  CancelUiGesture();
  interface_.SetOnly(ui::Panel::None);
  interface_.previewYaw=0;
  interface_.menuNotice.clear();
  interface_.inventoryNotice.clear();
  interface_.merchantNotice.clear();
  interface_.noticeTime=0;
  yaw_=0; pitch_=.45f; distance_=10; time_=0; atmosphereTime_=0; ResetMotion();
  return true;
}

void Game::Update(double seconds,const Input& input,float wheel) {
  creatureSteps_.Reset();
  const float dt=std::isfinite(seconds)?static_cast<float>(std::clamp(seconds,0.0,.05)):0.f;
  // Require a fresh gesture on either side of the roster/world transition.
  if(inputBarrier_) {
    for(int key=1;key<256;++key) if(input.Held(key)) return;
    inputBarrier_=false;return;
  }
  if(choosingCharacter_) {
    const auto action=selection_.Update(dt,input,wheel);
    if(action==ui::CharacterSelection::Action::Quit) quitRequested_=true;
    if(action==ui::CharacterSelection::Action::Enter) {
      if(!RestoreSnapshot(selection_.Snapshot())) return;
      saveFile_=selection_.SaveFile();savedSnapshot_=selection_.Snapshot();
      choosingCharacter_=false;inputBarrier_=true;
    }
    return;
  }
  interface_.noticeTime=std::max(0.f,interface_.noticeTime-dt);
  // Engine changes the window after this update. Cancel before dispatching a
  // simultaneous mouse release, or that release would commit a stale drop.
  if (input.Pressed(VK_F11)) { CancelUiGesture(); interface_.InvalidateHitRegions(); }
  if (input.Pressed(VK_F2)) { hudVisible_=!hudVisible_; interface_.InvalidateHitRegions(); }
  if (input.Pressed(VK_F3)) debugOverlay_=!debugOverlay_;
  if (input.Pressed(VK_F5)) {SaveProgress();interface_.menuNotice=simulation_.Message();}
  if (input.Pressed(VK_F9)) LoadProgress();
  const auto previousUi=interface_.StateKey();
  if (input.Pressed(VK_ESCAPE)) interface_.Escape();
  if (input.Pressed('C')) interface_.Toggle(ui::Panel::Character);
  if (input.Pressed('I') || input.Pressed('B')) interface_.Toggle(ui::Panel::Inventory);
  if (input.Pressed('J')) interface_.Toggle(ui::Panel::Journal);
  if (input.Pressed('M')) interface_.Toggle(ui::Panel::Map);
  if (input.Pressed('N')) interface_.Toggle(ui::Panel::Build);
  const bool closingQuest=interface_.panel==ui::Panel::Quest && input.Pressed('E');
  if (closingQuest) interface_.Close(ui::Panel::Quest);
  const bool closingMerchant=interface_.panel==ui::Panel::Merchant && input.Pressed('E');
  if (closingMerchant) interface_.Close(ui::Panel::Merchant);
  ui::Event event;
  if (!input.Focused()) CancelUiGesture();
  const bool uiVisible=hudVisible_ || interface_.HasPanels() || simulation_.Health()<=0;
  const bool worldGesture=orbitButtons_!=0;
  if (uiVisible && input.Focused())
    event=interface_.Pointer(input.ClientX(),input.ClientY(),input.Width(),input.Height(),
      !worldGesture && input.Pressed(VK_LBUTTON),!worldGesture && input.Pressed(VK_RBUTTON),
      !worldGesture && input.Held(VK_LBUTTON),!worldGesture && input.Released(VK_LBUTTON));
  // An actor press may become a free-look drag. Commit the original actor only
  // on a stationary release; moving the camera must not silently retarget combat.
  if(event.action==ui::Action::SelectActor && input.Pressed(VK_LBUTTON)) {
    pendingWorldTarget_=event.value>0?rpg::EntityId(event.value):0;event={};
  }
  rpg::Command command;
  command.uiClick=event.action!=ui::Action::None || previousUi!=interface_.StateKey();
  switch (event.action) {
  case ui::Action::SelectThreat: if(event.value>0) command.selectThreat=rpg::EntityId(event.value); break;
  case ui::Action::SelectActor: if(event.value>0) command.selectActor=rpg::EntityId(event.value); break;
  case ui::Action::Language:
    interface_.InvalidateHitRegions();
    interface_.menuNotice=localization::Select(event.value==0?localization::Language::Russian:localization::Language::English)?"":"Could not save language preference.";
    break;
  case ui::Action::MusicVolume: interface_.musicVolume=std::clamp(interface_.musicVolume+event.value,0,100); break;
  case ui::Action::EffectsVolume: interface_.effectsVolume=std::clamp(interface_.effectsVolume+event.value,0,100); break;
  case ui::Action::ActionBarSize:
    interface_.SetActionBarPercent(interface_.ActionBarPercent()+event.value);
    interface_.menuNotice=SaveHotbarSettings()?"":"Could not save action-bar settings.";
    break;
  case ui::Action::Close: interface_.Close(static_cast<ui::Panel>(event.value)); break;
  case ui::Action::Menu: interface_.Escape(); break;
  case ui::Action::Character: interface_.Toggle(ui::Panel::Character); break;
  case ui::Action::Inventory: interface_.Toggle(ui::Panel::Inventory); break;
  case ui::Action::Map: interface_.Toggle(ui::Panel::Map); break;
  case ui::Action::Journal: interface_.Toggle(ui::Panel::Journal); break;
  case ui::Action::TrackHunt:
    execution_.InvalidateTrace();
    simulation_.TrackHunt(event.value);interface_.hasWaypoint=false;interface_.InvalidateHitRegions();break;
  case ui::Action::CollectLoot: command.collectLoot=true; break;
  case ui::Action::SelectJournal:
    execution_.InvalidateTrace();
    if (event.value>=-1 && event.value<rpg::frontierQuestCount+8) {
      interface_.journalSelection=event.value; if(event.value==-1)simulation_.TrackHunt(-1); interface_.InvalidateHitRegions();
    }
    break;
  case ui::Action::Build: interface_.Toggle(ui::Panel::Build); break;
  case ui::Action::SelectTalent:
    interface_.selectedTalent=event.value; interface_.selectedSpell=-1;
    interface_.talentPage=event.value/9;
    interface_.confirmTalentReset=false; interface_.InvalidateHitRegions(); break;
  case ui::Action::LearnTalent: command.learnTalent=event.value; break;
  case ui::Action::SelectSpell:
    interface_.selectedSpell=event.value; interface_.confirmTalentReset=false; interface_.InvalidateHitRegions(); break;
  case ui::Action::ConfirmTalentReset:
    interface_.confirmTalentReset=true; interface_.InvalidateHitRegions(); break;
  case ui::Action::CancelTalentReset:
    interface_.confirmTalentReset=false; interface_.InvalidateHitRegions(); break;
  case ui::Action::ResetTalents:
    command.resetTalents=true; interface_.confirmTalentReset=false; interface_.InvalidateHitRegions(); break;
  case ui::Action::BuildPage:
    interface_.CancelBuildConfirmation(); interface_.loadoutsPage=!interface_.loadoutsPage;
    if(!interface_.planningBuild) interface_.previewBuild=int(simulation_.BuildMask());
    interface_.selectedLoadout=0;
    interface_.InvalidateHitRegions(); break;
  case ui::Action::PlanBuild:
    interface_.CancelBuildConfirmation();interface_.planningBuild=!interface_.planningBuild;
    interface_.previewBuild=int(simulation_.BuildMask());interface_.InvalidateHitRegions();break;
  case ui::Action::EditBuild:
    interface_.CancelBuildConfirmation();interface_.planningBuild=true;interface_.loadoutsPage=false;
    if(interface_.previewBuild<0) interface_.previewBuild=int(simulation_.BuildMask());
    interface_.InvalidateHitRegions();break;
  case ui::Action::DraftTalent: {
    if(!interface_.planningBuild || interface_.previewBuild<0) break;
    if(event.value==-1) {interface_.previewBuild=0;interface_.InvalidateHitRegions();break;}
    unsigned draft=unsigned(interface_.previewBuild);
    if(rpg::TogglePlannedTalent(draft,rpg::Talent(event.value))) interface_.previewBuild=int(draft);
    else Notify("Draft needs its prerequisite and must fit the five-point budget.");
    interface_.InvalidateHitRegions();break;
  }
  case ui::Action::SelectLoadout:
    if (event.value>=0 && event.value<3) {
      interface_.CancelBuildConfirmation(); interface_.selectedLoadout=event.value;
      if (simulation_.Loadout(event.value)>=0) interface_.previewBuild=simulation_.Loadout(event.value);
      interface_.InvalidateHitRegions();
    }
    break;
  case ui::Action::SelectBuildPreset:
    if (event.value>=-1 && event.value<int(rpg::buildPresets.size())) {
      interface_.CancelBuildConfirmation();
      interface_.previewBuild=event.value<0?int(simulation_.BuildMask()):int(rpg::buildPresets[event.value].mask);
      interface_.InvalidateHitRegions();
    }
    break;
  case ui::Action::RequestBuildApply:
  case ui::Action::RequestBuildStore:
    interface_.pendingBuild={event.action==ui::Action::RequestBuildApply?rpg::BuildAction::Apply:rpg::BuildAction::Store,
      interface_.previewBuild,interface_.selectedLoadout};
    interface_.InvalidateHitRegions(); break;
  case ui::Action::ConfirmBuild:
    if (interface_.loadoutsPage && interface_.pendingBuild.action!=rpg::BuildAction::None)
      command.build=interface_.pendingBuild;
    interface_.CancelBuildConfirmation(); break;
  case ui::Action::CancelBuild: interface_.CancelBuildConfirmation(); break;
  case ui::Action::QuestConfirm: command.questAction=static_cast<rpg::QuestAction>(event.value); break;
  case ui::Action::EndReplay: command.endReplay=true; break;
  case ui::Action::QuestPage:
    interface_.questPage=std::max(0,interface_.questPage+event.value); interface_.InvalidateHitRegions(); break;
  case ui::Action::Save: SaveProgress(); interface_.menuNotice=simulation_.Message(); break;
  case ui::Action::Load: LoadProgress(); interface_.menuNotice=simulation_.Message(); break;
  case ui::Action::Fullscreen: fullscreenRequest_=true; break;
  case ui::Action::Quit: quitRequested_=true; break;
  case ui::Action::CharacterSelection:
    if(!SaveProgress()) {interface_.menuNotice=simulation_.Message();break;}
    SuspendInput();
    interface_.SetOnly(ui::Panel::None);
    interface_.menuNotice.clear();
    ResetMotion();
    selection_.Open(rosterFile_,simulation_.Class(),saveFile_);
    choosingCharacter_=true;
    inputBarrier_=true;
    return;
  case ui::Action::SelectItem:
    interface_.selected=static_cast<ui::Item>(event.value); interface_.selectedBag=event.inventory.source; interface_.Show(ui::Panel::Inventory); break;
  case ui::Action::UseItem: ui::ApplyItem(command,simulation_,static_cast<ui::Item>(event.value),event.inventory.source); break;
  case ui::Action::InventoryChange: command.inventory=event.inventory; break;
  case ui::Action::Purchase: command.purchase=event.value; break;
  case ui::Action::Sell: command.trade={rpg::TradeAction::Sell,static_cast<ui::Item>(event.value),event.inventory.source}; break;
  case ui::Action::Buyback: command.trade={rpg::TradeAction::Buyback,static_cast<ui::Item>(event.value)}; break;
  case ui::Action::RotatePreview: interface_.previewYaw=event.value==0?0:std::remainder(interface_.previewYaw+event.value*.45f,6.2831853f); break;
  case ui::Action::Ability: command.ability=event.value; break;
  case ui::Action::ClearWaypoint: interface_.hasWaypoint=false; break;
  case ui::Action::PlaceWaypoint:
    if (simulation_.CanStand(interface_.pendingWaypoint)) { interface_.waypoint=interface_.pendingWaypoint; interface_.hasWaypoint=true; }
    else Notify("Place your waypoint on traversable ground.");
    break;
  default: break;
  }
  const bool pointerCaptured=uiVisible && interface_.PointerOverUi();
  const unsigned priorOrbit=orbitButtons_;
  // Pointer actions above can open or close a modal panel in this very frame.
  command.haltMovement=interface_.CapturesMovement() || !input.Focused();
  const bool cameraBlocked=!input.Focused() || interface_.CapturesMovement() || interface_.Dragging() || pointerCaptured ||
    previousUi!=interface_.StateKey() || input.Pressed(VK_F11) || input.Pressed(VK_F9);
  if(cameraBlocked) CancelOrbit();
  else {
    const bool releasedLeft=(orbitButtons_&MK_LBUTTON) && input.Released(VK_LBUTTON);
    orbitButtons_&=(input.Held(VK_LBUTTON)?MK_LBUTTON:0u)|(input.Held(VK_RBUTTON)?MK_RBUTTON:0u);
    if(input.Pressed(VK_LBUTTON)) {
      orbitButtons_|=MK_LBUTTON;leftOrbitTravel_=0;leftOrbitDragged_=false;
      if(!(orbitButtons_&MK_RBUTTON))freeLook_=true;
    }
    if(input.Pressed(VK_RBUTTON)) {orbitButtons_|=MK_RBUTTON;freeLook_=false;}
    if((orbitButtons_&MK_LBUTTON) && !(orbitButtons_&MK_RBUTTON))freeLook_=true;
    if(orbitButtons_&MK_RBUTTON) {leftOrbitDragged_=true;pendingWorldTarget_=0;}
    if(orbitButtons_&MK_LBUTTON) {
      if(orbitButtons_&priorOrbit)leftOrbitTravel_+=std::hypot(input.MouseX(),input.MouseY());
      if(leftOrbitTravel_>4)leftOrbitDragged_=true;
    }
    if(releasedLeft) {
      if(!leftOrbitDragged_ && pendingWorldTarget_) {command.selectActor=pendingWorldTarget_;command.uiClick=true;}
      pendingWorldTarget_=0;
    }
  }
  if (!interface_.CapturesMovement()) {
    if (!pointerCaptured && !interface_.Dragging()) {
      // Do not inherit motion from a held UI button when the other button
      // starts a fresh world gesture. Only an already-owned drag supplies it.
      const bool orbitMotion=(orbitButtons_&priorOrbit)!=0;
      yaw_=std::remainder(yaw_+(orbitMotion?input.MouseX()*.006f:0),motion::tau);
      pitch_=std::clamp(pitch_+(orbitMotion?input.MouseY()*.004f:0),-.75f,1.1f);
      if (std::isfinite(wheel)) distance_=std::clamp(distance_-wheel,3.f,18.f);
    }
    const float forward=float(input.Held('W'))-float(input.Held('S'));
    const float right=float(input.Held('D'))-float(input.Held('A'));
    // Predict the same filtered heading used by UpdateCamera below. Steering
    // against the raw target makes WASD drift sideways during fast mouse turns.
    const float movementYaw=freeLook_?movementYaw_:motion::FollowAngle(cameraYaw_,yaw_,30.f,dt);
    command.steerFacing=(orbitButtons_&MK_RBUTTON) && input.Focused(); command.facingYaw=movementYaw;
    command.movement={std::sin(movementYaw)*forward+std::cos(movementYaw)*right,
      std::cos(movementYaw)*forward-std::sin(movementYaw)*right};
    command.sprint=input.Held(VK_SHIFT); command.jump=input.Pressed(VK_SPACE);
    command.nextTarget=input.Pressed(VK_TAB); command.interact=input.Pressed('E') && !closingMerchant && !closingQuest;
    command.restart=input.Pressed('R');
    command.usePotion=command.usePotion || input.Pressed('Q'); command.equipWeapon=command.equipWeapon || input.Pressed('G');
    for (int i=0;i<10;++i) if (input.Pressed(i==9?'0':'1'+i)) {
      const bool control=(input.Modifiers(i==9?'0':'1'+i)&KeyTransitions::Control)!=0;
      const int spell=interface_.HotbarSpell(i+(control?10:0),simulation_.Class());
      if(spell==4) command.usePotion=true; else if(spell>=0) command.ability=spell;
      break;
    }
  }
  // Recovery stays reachable even if the player opens a central panel after death.
  command.respawn=simulation_.Health()<=0 && input.Pressed(VK_RETURN);
  if (command.restart || command.respawn) {
    CancelUiGesture();
    interface_.SetOnly(ui::Panel::None); yaw_=0; pitch_=.45f; distance_=10;
    interface_.previewYaw=0;
    if (command.restart) interface_.hasWaypoint=false;
  }
  if (simulation_.QuestDialogue()!=rpg::QuestId::None &&
    (interface_.panel!=ui::Panel::Quest || !input.Focused())) command.questAction=rpg::QuestAction::Close;
  const bool wasAlive=simulation_.Health()>0;
  // Menus capture local commands, but never stop simulation time.
  command.queueAttack=true;
  const auto frame=ExecuteFrame(command,dt);
  if (wasAlive && simulation_.Health()<=0) {
    CancelUiGesture(); interface_.Close(interface_.panel);
  }
  if (frame.tradeRequested) { CancelOrbit(); interface_.merchantNotice.clear(); interface_.Show(ui::Panel::Merchant); }
  if (command.purchase!=-1 || command.trade.action!=rpg::TradeAction::None) interface_.merchantNotice=frame.notice;
  if (interface_.panel==ui::Panel::Merchant && !simulation_.CanTrade()) interface_.Close(ui::Panel::Merchant);
  if (command.inventory.action!=rpg::InventoryAction::None) {
    interface_.inventoryNotice=frame.notice; interface_.noticeTime=3;
  }
}
Game::FrameResult Game::ExecuteFrame(const rpg::Command& command,float seconds) {
  const auto plan=schedule_.Begin(seconds,command);FrameResult result;
  unsigned executed=0;
  for(unsigned i=0;i<plan.count;++i) {
    const auto generation=audioGeneration_;
    const auto step=ExecuteCommand(plan.At(i),plan.seconds);++executed;
    result.events+=static_cast<unsigned>(step.events.count);result.droppedEvents+=step.events.dropped;
    static_assert(static_cast<unsigned>(rpg::CombatVisualKind::DisciplineSpent)<64);
    for(std::size_t n=0;n<step.events.count;++n) {
      const auto& event=step.events.values[n];const auto kind=static_cast<unsigned>(event.kind);
      if(kind>=result.firstEventOffsets.size())continue;
      const auto bit=std::uint64_t(1)<<kind;
      const float offset=i*plan.seconds+step.seconds-std::clamp(event.elapsed,0.f,step.seconds);
      if(!(result.eventKinds&bit))result.firstEventOffsets[kind]=offset;
      else result.firstEventOffsets[kind]=std::min(result.firstEventOffsets[kind],offset);
      result.eventKinds|=bit;
    }
    result.tradeRequested|=simulation_.TradeRequested();
    if(i==0 && (command.purchase!=-1 || command.trade.action!=rpg::TradeAction::None ||
        command.inventory.action!=rpg::InventoryAction::None))result.notice=simulation_.Message();
    // A reset has already published the replacement world. Remaining held
    // controls from the outgoing frame must not move it or restart a cast.
    if(generation!=audioGeneration_){result.boundary=true;break;}
  }
  schedule_.Complete(plan,executed);
  result.steps=executed;
  return result;
}
rpg::StepReceipt Game::ExecuteCommand(const rpg::Command& command,float seconds) {
  creatureSteps_.Reset();
  const auto step=execution_.Advance(simulation_,seconds,command);
  const auto start=std::chrono::steady_clock::now();
  scene_.Capture(simulation_,step.sequence);
  if(step.seconds<=0)return step;
  const auto generation=audioGeneration_;
  soundDelivery_.Advance(step.seconds);
  PresentStep(step);
  // Resets cancel old transients instead of replaying pre-reset receipts.
  if(generation==audioGeneration_) {
    soundEvents_.Update(scene_,simulation_.Map(),&creatureSteps_,step.events);
    soundDelivery_.Append(soundEvents_);
  }
  execution_.RecordPresentation(std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
  return step;
}
const sound::Frame& Game::TakeAudioFrame(float seconds){
  audioFrame_.Capture(scene_);++audioFrame_.sequence;audioFrame_.generation=audioGeneration_;
  audioFrame_.events=soundDelivery_.Take();audioFrame_.preparationToken=soundEvents_.preparationSerial;
  audioFrame_.musicVolume=MusicVolume();audioFrame_.effectsVolume=EffectsVolume();audioFrame_.cameraYaw=cameraYaw_;
  audioFrame_.seconds=std::isfinite(seconds)?std::clamp(seconds,0.f,.1f):0;
  audioFrame_.menu=choosingCharacter_;audioFrame_.hasAudition=choosingCharacter_;
  audioFrame_.audition=choosingCharacter_?selection_.PreviewAudio():sound::Audition{};
  return audioFrame_;
}
std::string Game::StepDiagnostics()const {
  const auto& step=execution_.Stats();const auto& sound=soundDelivery_.Stats();std::ostringstream out;
  const auto& frames=schedule_.Stats();
  out<<"Simulation steps / empty steps: "<<step.steps<<" / "<<step.emptySteps
    <<"\nScheduled frames / subdivided / steps / peak / cancelled: "<<frames.frames<<" / "<<frames.subdivided<<" / "<<frames.steps<<" / "<<frames.peakSteps<<" / "<<frames.cancelledSteps
    <<"\nScheduled admitted / boundary-cancelled seconds: "<<frames.admittedSeconds<<" / "<<frames.cancelledSeconds
    <<"\nScheduled ignored / capped frames / capped seconds: "<<frames.invalid<<" / "<<frames.cappedFrames<<" / "<<frames.cappedSeconds
    <<"\nAdmitted simulation seconds: "<<step.admittedSeconds
    <<"\nSimulation / presentation CPU seconds: "<<step.simulationSeconds<<" / "<<step.presentationSeconds
    <<"\nAccepted / dropped visual events: "<<step.events<<" / "<<step.droppedEvents
    <<"\nScene revision / sequence / actors / bytes / storage growths: "<<scene_.Revision()<<" / "<<scene_.Sequence()<<" / "<<scene_.Combatants().size()<<" / "<<scene_.Bytes()<<" / "<<scene_.StorageGrowths()
    <<"\nSound accepted / delivered / expired / dropped / peak: "<<sound.accepted<<" / "<<sound.delivered<<" / "<<sound.expired<<" / "<<sound.dropped<<" / "<<sound.peak<<'\n';
  out<<"Audio publication / emitters / owned bytes: "<<audioFrame_.sequence<<" / "<<audioFrame_.emitters.size()<<" / "<<audioFrame_.Bytes()<<'\n';
  return out.str();
}
void Game::PresentStep(const rpg::StepReceipt& step) {
  const float dt=step.seconds;const auto& command=step.command;
  const bool wasDead=step.wasDead,wasSparring=step.wasSparring,wasReplay=step.wasReplay;
  const auto previousVelocity=step.previousVelocity,previousPosition=step.previousPosition;
  const float previousFacing=playerPose_.facing;
  const auto priorDialogue=step.priorDialogue;const auto priorStage=step.priorStage;
  if (scene_.Dialogue()!=rpg::QuestId::None) {
    if (interface_.panel!=ui::Panel::Quest) { CancelUiGesture(); interface_.SetOnly(ui::Panel::Quest); }
    if (priorDialogue!=scene_.Dialogue() || priorStage!=scene_.DialogueStage()) {
      interface_.questPage=0; interface_.InvalidateHitRegions();
    }
  } else if (interface_.panel==ui::Panel::Quest) interface_.Close(ui::Panel::Quest);
  time_+=dt;
  // Runtime wildlife belongs to Simulation; the old ambient preview is test-only.
  atmosphereTime_=std::fmod(atmosphereTime_+dt,1800.0);
  if (command.restart || (wasDead && scene_.Player().health>0) || wasReplay!=scene_.Replay()) { worldView_.ResetWildlife(); ResetMotion(); return; }
  interface_.UpdateCombatFeedback(dt,simulation_);
  effects_.Advance(dt);
  sustained_.Update(scene_,dt);
  for (std::size_t i=0;i<step.events.count;++i) {
    const auto& event=step.events.values[i];
    // Attach hostile beams after their shared actor pose has advanced below.
    if(event.kind==rpg::CombatVisualKind::EnemyLance || event.kind==rpg::CombatVisualKind::EnemyExecution)continue;
    if(event.kind==rpg::CombatVisualKind::SpellCast) {
      auto launch=playerAnimation_.pose;
      launch.attackSeconds=-1; launch.cast=0;
      launch.action=animation::SampleAction(event.school,event.opening?animation::ActionClip::Opening:event.signature?animation::ActionClip::Signature:event.technique?animation::ActionClip::Technique:event.heavy?animation::ActionClip::Heavy:animation::ActionClip::Basic,
        combat::StrokeStart(event.heavy));
      const bool shield=scene_.Player().outfit.shield;
      const auto form=rpg::WeaponFor(event.school).form;
      const auto releasePose=BuildCharacterPose(0,playerPose_.gait,shield,launch,form);
      const auto local=rpg::PhysicalLauncher(form)?WeaponReleasePoint(releasePose,form):SpellReleasePoint(releasePose,shield);
      auto actor=playerPose_;
      actor.Update(event.facing,0,scene_.Player().grounded,true,0,std::max(0.f,dt-event.elapsed),false,42.f);
      const float sx=std::sin(actor.facing),cx=std::cos(actor.facing);
      const DirectX::XMFLOAT3 origin{event.position.x+cx*local.x+sx*local.z,event.height+local.y,
        event.position.z-sx*local.x+cx*local.z};
      effects_.Emit(event,&origin);
    } else effects_.Emit(event);
  }
  effects_.TrackProjectiles(scene_);
  const auto dashPosition=scene_.Player().position;
  effects_.TrackDash({dashPosition.x,scene_.Player().worldHeight,dashPosition.z},scene_.Player().dashing,dt,
    scene_.Player().dashing?motion::DashSpeed(scene_.Player().dashPhase):0);
  if (command.haltMovement || scene_.Player().health<=0) effects_.CancelDash();
  if (scene_.Player().health<=0) effects_.CancelTrails();
  if (wasSparring && !scene_.Sparring()) effects_.CancelTrail(scene_.Trainer().id);
  playerPose_.Update(scene_.Player().facing,scene_.Player().groundMovementDistance,scene_.Player().grounded && !scene_.Player().dashing,
    scene_.Player().health>0,scene_.Player().attackFlash*5,dt,scene_.Player().dashing || scene_.Player().dashed,
    scene_.Player().attacking?42.f:22.f);
  const auto velocity=scene_.Player().velocity;
  const auto applyImpacts=[&](animation::State& state,rpg::EntityId actor,bool alive,float facing) {
    if (!alive) return;
    for (std::size_t i=0;i<step.events.count;++i) {
      const auto& event=step.events.values[i];
      if (event.actor==actor && (event.kind==rpg::CombatVisualKind::Hurt ||
          event.kind==rpg::CombatVisualKind::Hit || event.kind==rpg::CombatVisualKind::HeavyHit)) {
        const auto profile=event.kind==rpg::CombatVisualKind::Hurt?animation::ImpactProfile{}:
          animation::ClassImpact(event.school,event.heavy || event.kind==rpg::CombatVisualKind::HeavyHit);
        // Contacts carry the incoming attack direction. Rear hits bend forward;
        // lateral contacts roll the body instead of always throwing it backward.
        state.ApplyImpact(std::clamp(event.elapsed,0.f,dt),std::sin(event.facing-facing),profile,-std::cos(event.facing-facing));
      }
    }
  };
  bool castStarted=false,dashImpact=false,dashFinished=false;
  float dashEndElapsed=0;
  for(std::size_t i=0;i<step.events.count;++i) {
    castStarted|=step.events.values[i].kind==rpg::CombatVisualKind::Mend;
    const auto& event=step.events.values[i];
    if (event.kind==rpg::CombatVisualKind::DashImpact || event.kind==rpg::CombatVisualKind::DashStop) {
      dashImpact=event.kind==rpg::CombatVisualKind::DashImpact; dashFinished=true; dashEndElapsed=event.elapsed;
    }
  }
  const float forwardAcceleration=command.haltMovement?0:
    ((velocity.x-previousVelocity.x)*std::sin(playerPose_.facing)+
     (velocity.z-previousVelocity.z)*std::cos(playerPose_.facing))/dt;
  playerAnimation_.Update(dt,scene_.Player().height,scene_.Player().verticalSpeed,scene_.Player().movementDistance,
    scene_.Player().health>0,playerPose_.attack,
    scene_.Player().landingSpeed,scene_.Player().landingElapsed,forwardAcceleration,castStarted,
    std::remainder(playerPose_.facing-previousFacing,6.2831853f)/dt);
  applyImpacts(playerAnimation_,0,scene_.Player().health>0,playerPose_.facing);
  for(std::size_t i=0;i<dummyAnimations_.size();++i) {
    const auto& dummy=scene_.Dummies()[i]; auto& reaction=dummyAnimations_[i];
    if(!scene_.ShowsDummy(dummy)) {reaction.Reset();continue;}
    reaction.Update(dt,0,0,0,true,0);
    applyImpacts(reaction,dummy.id,true,DirectX::XM_PI);
  }
  playerAnimation_.UpdateDash(dt,scene_.Player().dashing,scene_.Player().dashPhase,dashImpact,scene_.Player().health>0,dashFinished,dashEndElapsed);
  playerAnimation_.pose.attackSeconds=scene_.Player().attackSeconds;
  playerAnimation_.pose.retreat=scene_.Player().retreat;
  if(rpg::DescribeClass(scene_.Player().classId).range>3 && scene_.Player().attacking)
    playerAnimation_.pose.attackSeconds=-1;
  playerAnimation_.actions.Update(dt,scene_.Player().classId,scene_.Player().attackSeconds,scene_.Player().heavy,
    scene_.Player().recovery?std::min(.30f,scene_.Player().recoveryProgress*1.5f):playerAnimation_.castAge,scene_.Player().health>0,scene_.Player().technique,scene_.Player().signature,scene_.Player().opening,-1,scene_.Player().attackAbility);
  playerAnimation_.pose.action=playerAnimation_.actions.pose;
  if(scene_.Player().classId!=rpg::ClassId::Warrior) playerAnimation_.pose.cast=0;
  playerAnimation_.pose.heavyAttack=scene_.Player().heavy;
  playerAnimation_.SettleWeapon(dt,scene_.Player().health>0);
  const float lateralAcceleration=command.haltMovement?0:
    ((velocity.x-previousVelocity.x)*std::cos(playerPose_.facing)-
     (velocity.z-previousVelocity.z)*std::sin(playerPose_.facing))/dt;
  motion::Spring(playerAnimation_.pose.bank,playerAnimation_.bankVelocity,
    scene_.Player().grounded?std::clamp(lateralAcceleration/50.f,-1.f,1.f):0.f,18.f,dt);
  if (!scene_.Player().attacking) { playerPose_.attack=0; playerPose_.attackVelocity=0; effects_.CancelTrail(0); }
  if (command.haltMovement) effects_.CancelTrail(0);
  if (command.haltMovement) playerAnimation_.CancelDash();
  playerAnimation_.pose.lean*=1-playerAnimation_.pose.dash;
  const float dx=scene_.Player().position.x-previousPosition.x,dz=scene_.Player().position.z-previousPosition.z;
  const float moved=std::hypot(dx,dz),c=std::cos(playerPose_.facing),s=std::sin(playerPose_.facing);
  playerAnimation_.locomotion.Update(dt,scene_.Player().groundMovementDistance,moved>.000001f?(s*dx+c*dz)/moved:0,
    moved>.000001f?(c*dx-s*dz)/moved:0,motion::AngleDelta(previousFacing,playerPose_.facing)/dt,
    scene_.Player().grounded,scene_.Player().verticalSpeed,scene_.Player().height,scene_.Player().health>0,scene_.Player().landingElapsed);
  playerAnimation_.pose.motion=playerAnimation_.locomotion.pose;
  const auto weaponForm=rpg::WeaponFor(scene_.Player().classId).form;
  const bool hasWeapon=scene_.Player().outfit.weapon;
  const bool staff=rpg::StaffWeapon(weaponForm);
  const bool prayer=weaponForm==rpg::WeaponForm::Scepter;
  const bool bow=weaponForm==rpg::WeaponForm::Handbow;
  const bool blade=weaponForm==rpg::WeaponForm::Blade||weaponForm==rpg::WeaponForm::Mace||weaponForm==rpg::WeaponForm::Cleaver;
  const auto equipment=hasWeapon&&staff?playerclips::Equipment::Staff:
    hasWeapon&&blade?playerclips::Equipment::Sword:hasWeapon&&bow?playerclips::Equipment::Bow:playerclips::Equipment::None;
  if(scene_.Player().health>0) animation::ApplyAuthoredLocomotion(playerAnimation_,dt,equipment);
  playerAnimation_.authoredWeapon.Update(dt,scene_.Player().attackSeconds,scene_.Player().heavy,staff||prayer,
    scene_.Player().health>0&&hasWeapon&&(staff||blade||bow||prayer)&&!scene_.Player().dashing&&!command.haltMovement,scene_.Player().attackRate,bow,
    scene_.Player().attackAbility,scene_.Player().classId==rpg::ClassId::Warrior?0:scene_.Player().classId==rpg::ClassId::Scout?1:scene_.Player().classId==rpg::ClassId::Mage?2:scene_.Player().classId==rpg::ClassId::Priest?3:-1);
  playerAnimation_.pose.authoredStrike=playerAnimation_.authoredWeapon.frame;
  playerAnimation_.pose.authoredStrikeWeight=playerAnimation_.authoredWeapon.weight;
  playerAnimation_.pose.authoredStrikePhase=playerAnimation_.authoredWeapon.phase;
  playerAnimation_.pose.authoredStrikeHeavy=playerAnimation_.authoredWeapon.heavy;
  playerAnimation_.pose.authoredAccent=playerAnimation_.authoredWeapon.accent;
  playerAnimation_.pose.authoredMechanics=playerAnimation_.authoredWeapon.mechanics;
  playerAnimation_.pose.authoredMechanicsTracked=true;
  playerAnimation_.pose.authoredReady=playerAnimation_.authoredWeapon.ready;
  playerAnimation_.pose.authoredEntryReady=playerAnimation_.authoredWeapon.entryReady;
  playerFeet_.Update(dt,scene_.Player().position,playerPose_.facing,scene_.Player().grounded && !scene_.Player().dashing,
    scene_.Player().health>0,playerAnimation_.pose);
  const auto& enemies=scene_.Combatants();
  presentation::Bind(enemyActors_,enemies,scene_.Trainer());
  for (std::size_t i=0;i<enemyActors_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:scene_.Trainer();
    if (i==enemies.size() && !wasSparring && scene_.Sparring()) {
      enemyActors_[i].Reset(enemy);
    }
    const bool active=scene_.Player().health>0 && enemy.health>0 && (i<enemies.size() || scene_.Sparring());
    auto& actor=enemyActors_[i];
    actor.defeat.UpdatePose(dt,enemy.health>0,actor.motion.attack,actor.motion.phase,
      actor.animation.pose.travel,actor.Gesture(),float(actor.clock));
    if(enemy.health>0)actor.clock+=dt;
    auto& releaseAge=enemyActors_[i].animation.enemyReleaseAge;
    releaseAge=std::min(1.f,releaseAge+dt);
    for(std::size_t eventIndex=0;eventIndex<step.events.count;++eventIndex) {
      const auto& event=step.events.values[eventIndex];
      if(event.actor==enemy.id && (event.kind==rpg::CombatVisualKind::EnemySwing || event.kind==rpg::CombatVisualKind::BeastBite || event.kind==rpg::CombatVisualKind::BeastCharge || event.kind==rpg::CombatVisualKind::EnemyLance))
        releaseAge=std::clamp(event.elapsed,0.f,dt);
    }
    if(!active || enemy.windup>0) releaseAge=1;
    const float strike=EnemyStrike(enemy,active,releaseAge);
    const float turnRate=motion::AngleDelta(enemyActors_[i].motion.facing,enemy.facing)/dt;
    // Enemy heading is already smoothed by simulation, then frozen for attacks.
    // Use it exactly so the character always faces the displayed damage cone.
    enemyActors_[i].motion.Update(enemy.facing,enemy.movementDistance,true,enemy.health>0,strike,dt,true);
    auto& movement=enemyActors_[i].movement;
    movement.Update(enemy.position.x,enemy.position.z,enemy.facing,dt,enemy.health>0);
    enemyActors_[i].animation.Update(dt,0,0,enemy.movementDistance,enemy.health>0,strike,0,0,movement.acceleration);
    motion::Spring(enemyActors_[i].animation.pose.bank,enemyActors_[i].animation.bankVelocity,
      enemy.health>0?std::clamp(movement.lateralAcceleration/50.f,-1.f,1.f):0.f,18.f,dt);
    applyImpacts(enemyActors_[i].animation,enemy.id,enemy.health>0,enemy.facing);
    if(enemy.Beast()) {
      const auto p=scene_.Player().position;
      const bool detailed=std::hypot(enemy.position.x-p.x,enemy.position.z-p.z)<40;
      actor.beast.Update(dt,enemy,movement,actor.animation.pose,releaseAge,active,detailed);
      for(unsigned contact=0;contact<actor.beast.feet.count;++contact)creatureSteps_.Add(actor.beast.feet.events[contact],p);
      actor.motion.phase=actor.beast.phase;
      actor.animation.pose.travel=actor.beast.travel;
      continue;
    }
    const float actionSeconds=EnemyAttackSeconds(enemy,active,releaseAge);
    enemyActors_[i].animation.pose.attackSeconds=actionSeconds;
    enemyActors_[i].animation.pose.heavyAttack=enemy.kind==rpg::EnemyKind::Captain || enemy.kind==rpg::EnemyKind::Reaver;
    const auto gestureClass=enemy.kind==rpg::EnemyKind::Wraith?rpg::ClassId::Mage:
      enemy.kind==rpg::EnemyKind::Reaver?rpg::ClassId::Demonologist:enemy.Boss()?rpg::ClassId::Paladin:rpg::ClassId::Warrior;
    enemyActors_[i].animation.actions.Update(dt,gestureClass,actionSeconds,
      enemyActors_[i].animation.pose.heavyAttack || enemy.kind==rpg::EnemyKind::Wraith,-1,active,false,enemy.Boss(),false,enemy.Boss()?int(enemy.bossAttack):-1);
    enemyActors_[i].animation.pose.action=enemyActors_[i].animation.actions.pose;
    if(enemy.kind==rpg::EnemyKind::Wraith) {
      enemyActors_[i].animation.pose.attackSeconds=-1;
      enemyActors_[i].animation.pose.cast=0;
    }
    enemyActors_[i].animation.SettleWeapon(dt,active);
    enemyActors_[i].feet.Update(dt,enemy.position,enemy.facing,true,enemy.health>0,enemyActors_[i].animation.pose);
    enemyActors_[i].animation.locomotion.Update(dt,enemy.movementDistance,movement.forward,movement.right,turnRate,true,0,0,enemy.health>0);
    enemyActors_[i].animation.pose.motion=enemyActors_[i].animation.locomotion.pose;
  }
  UpdateFaces(dt);
  for(std::size_t i=0;i<step.events.count;++i) {
    const auto& event=step.events.values[i];
    if(event.kind!=rpg::CombatVisualKind::EnemyLance && event.kind!=rpg::CombatVisualKind::EnemyExecution)continue;
    const auto enemy=std::find_if(enemies.begin(),enemies.end(),[&](const presentation::ActorFrame& e){return e.id==event.actor;});
    const auto actor=std::find_if(enemyActors_.begin(),enemyActors_.end(),[&](const presentation::Actor& a){return a.id==event.actor;});
    if(enemy!=enemies.end() && actor!=enemyActors_.end()) {
      const auto origin=castfx::FromEnemy(*enemy,*actor).focus;effects_.Emit(event,&origin);
    } else effects_.Emit(event);
  }
  for(unsigned i=0;i<creatureSteps_.count;++i)effects_.EmitFootstep(creatureSteps_.events[i],simulation_.Map());
  const auto track=[&](rpg::EntityId id,rpg::Vec2 position,float height,const motion::Actor& motion,
    const animation::Sample& animation,bool shield,bool armed) {
    if (!armed) { effects_.CancelTrail(id); return; }
    if (!effects_.Tracking(id)) return;
    const auto form=id==0?rpg::WeaponFor(scene_.Player().classId).form:rpg::WeaponForm::Blade;
    const auto pose=BuildCharacterPose(motion.attack,motion.gait,shield,animation,form);
    const auto point=[&](float distance) -> DirectX::XMFLOAT3 {
      const auto p=pose.WeaponPoint(distance);
      return {position.x+std::cos(motion.facing)*p.x+std::sin(motion.facing)*p.z,height+p.y,
        position.z-std::sin(motion.facing)*p.x+std::cos(motion.facing)*p.z};
    };
    effects_.TrackWeapon(id,point(rpg::weaponTraceRoot),point(rpg::WeaponTraceTip(form)));
  };
  track(0,scene_.Player().position,scene_.Player().worldHeight,playerPose_,playerAnimation_.pose,
    scene_.Player().outfit.shield,
    scene_.Player().health>0 && scene_.Player().outfit.weapon);
  for (std::size_t i=0;i<enemyActors_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:scene_.Trainer();
    track(enemy.id,enemy.position,rpg::WorldMap::Height(enemy.position),enemyActors_[i].motion,enemyActors_[i].animation.pose,false,
      scene_.Player().health>0 && enemy.health>0 && !enemy.Beast() && enemy.kind!=rpg::EnemyKind::Wraith && (i<enemies.size() || scene_.Sparring()));
  }
  UpdateCamera(dt);
}
void Game::UpdateFaces(float dt) {
  const auto player=scene_.Player().position;
  const auto target=scene_.Target();
  const bool talking=scene_.Dialogue()!=rpg::QuestId::None;
  const bool shopping=interface_.panel==ui::Panel::Merchant;
  const bool alive=scene_.Player().health>0;
  facial::Aim aim;
  if(talking || shopping) {
    const auto npc=talking?scene_.DialoguePosition():rpg::WorldMap::quartermaster;
    aim=facial::LookAt(npc.x-player.x,rpg::WorldMap::Height(npc)-scene_.Player().worldHeight,npc.z-player.z,playerPose_.facing);
  } else if(target && target->health>0)
    aim=facial::LookAt(target->position.x-player.x,rpg::WorldMap::Height(target->position)-scene_.Player().worldHeight,target->position.z-player.z,playerPose_.facing);
  playerFace_.Update(dt,aim,scene_.Player().attacking?.85f:target && target->health>0?.25f:
    std::max(playerAnimation_.pose.cast,playerAnimation_.pose.action.open)*.35f,
    playerAnimation_.FacialImpact(),talking?.18f:0,alive);
  playerAnimation_.pose.face=playerFace_.pose;
  const auto& enemies=scene_.Combatants();
  for(std::size_t i=0;i<enemyActors_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:scene_.Trainer();
    const bool active=alive && enemy.health>0 && (i<enemies.size() || scene_.Sparring());
    const float distance=std::hypot(enemy.position.x-player.x,enemy.position.z-player.z);
    const auto gaze=active && distance<8?facial::LookAt(player.x-enemy.position.x,scene_.Player().worldHeight-rpg::WorldMap::Height(enemy.position),
      player.z-enemy.position.z,enemyActors_[i].motion.facing):facial::Aim{};
    enemyActors_[i].face.Update(dt,gaze,active && distance<8?.65f:0,enemyActors_[i].animation.FacialImpact(),0,active);
    enemyActors_[i].animation.pose.face=enemyActors_[i].face.pose;
  }
  const auto npc=[&](facial::State& state,rpg::Vec2 position,float facing,bool interaction) {
    const float distance=std::hypot(player.x-position.x,player.z-position.z);
    const auto gaze=alive && distance<4?facial::LookAt(player.x-position.x,scene_.Player().worldHeight-rpg::WorldMap::Height(position),player.z-position.z,facing):facial::Aim{};
    state.Update(dt,gaze,0,0,interaction?.5f:distance<3?.12f:0);
  };
  npc(keeperFace_,rpg::WorldMap::questGiver,1.25f,talking && scene_.Dialogue()<rpg::QuestId::Watch8);
  npc(merchantFace_,rpg::WorldMap::quartermaster,0,shopping);
  worldView_.UpdateResidents(dt,player,alive,talking,scene_.DialoguePosition());
}
void Game::SetCameraOrbit(float yaw,float pitch,float distance) {
  if (!std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(distance)) return;
  yaw_=std::remainder(yaw,6.2831853f);
  pitch_=std::clamp(pitch,-.75f,1.1f); distance_=std::clamp(distance,3.f,18.f);
  freeLook_=false;
  // Explicit positioning is used by captures/tools; interactive input sets targets.
  UpdateCamera(0,true);
}
void Game::ResetMotion() {
  scene_.Capture(simulation_,scene_.Sequence());
  soundEvents_.Reset();soundDelivery_.Reset();
  freeLook_=false;movementYaw_=yaw_;
  creatureSteps_.Reset();
  interface_.ResetCombatFeedback();
  ++audioGeneration_;
  effects_.Reset();
  sustained_.Reset();
  playerPose_.Reset(scene_.Player().facing);
  playerAnimation_.Reset();
  for(auto& reaction:dummyAnimations_)reaction.Reset();
  playerFace_.Reset(1); keeperFace_.Reset(3); merchantFace_.Reset(2);
  worldView_.ResetResidents();
  playerFeet_.Reset(scene_.Player().position,scene_.Player().facing);
  enemyActors_.clear();
  presentation::Bind(enemyActors_,scene_.Combatants(),scene_.Trainer());
  for(std::size_t i=0;i<enemyActors_.size();++i) {
    if(scene_.Player().health<=0 || (i==scene_.Combatants().size() && !scene_.Sparring())) {
      enemyActors_[i].motion.attack=0;
      enemyActors_[i].beast.gesture.jaw=enemyActors_[i].beast.jaw=0;
    }
  }
  UpdateCamera(0,true);
}
void Game::UpdateCamera(float dt,bool immediate) {
  // Keep the horizon stable: the actor jumps within the frame instead of
  // dragging the entire camera through the full ballistic arc.
  const float height=rpg::WorldMap::Height(scene_.Player().position)+scene_.Player().height*.35f+1.4f;
  cameraYaw_=immediate?yaw_:motion::FollowAngle(cameraYaw_,yaw_,30.f,dt);
  if(!freeLook_)movementYaw_=cameraYaw_;
  cameraPitch_=immediate?pitch_:motion::Ease(cameraPitch_,pitch_,30.f,dt);
  cameraHeight_=immediate?height:motion::Ease(cameraHeight_,height,18.f,dt);
  const rpg::Vec2 direction{-std::sin(cameraYaw_)*std::cos(cameraPitch_),-std::cos(cameraYaw_)*std::cos(cameraPitch_)};
  const float probe=immediate?distance_:std::max(distance_,cameraBoom_.distance);
  const float clearance=simulation_.CameraDistance(scene_.Player().position,cameraHeight_,direction,std::sin(cameraPitch_),probe);
  if (immediate) cameraBoom_.Reset(clearance);
  else cameraBoom_.Update(distance_,clearance,dt);
}

void Game::Render(Renderer& renderer,bool showInterface) {
  if(!renderer.FrameActive())return;
  if(choosingCharacter_) {ui::RenderCharacterSelection(renderer,selection_);return;}
  renderer.SetAtmosphereTime(atmosphereTime_);
  if (!renderer.HasTerrainSurface() && !renderer.SetTerrainSurface(BuildTerrainSurface(simulation_.Map()),terrainSurfaceSize)) return;
  using DirectX::XMFLOAT3;
  using DirectX::XMFLOAT4;
  const auto player=scene_.Player().position;
  const float y=scene_.Player().worldHeight;
  const rpg::Vec2 direction{-std::sin(cameraYaw_)*std::cos(cameraPitch_),-std::cos(cameraYaw_)*std::cos(cameraPitch_)};
  const float cameraDistance=cameraBoom_.distance;
  renderer.SetCamera({player.x+direction.x*cameraDistance,cameraHeight_+std::sin(cameraPitch_)*cameraDistance,
    player.z+direction.z*cameraDistance},{player.x,cameraHeight_,player.z});
  SustainedEffects::Warnings(renderer,scene_.Warnings(),&enemyActors_);
  std::array<TrainingPropPose,3> training;
  for(std::size_t i=0;i<training.size();++i) {
    const auto& reaction=dummyAnimations_[i].pose;
    // Rock the intact prop around its planted base, without deforming its mesh.
    training[i]=TrainingPropPose::FromRecoil(animation::ForwardRecoil(reaction),reaction.hitSide);
  }
  worldView_.Draw(renderer,simulation_.Map(),time_,scene_.Sparring(),merchantFace_.pose,keeperFace_.pose,scene_.Player().position,training);
  populationview::Embers(renderer,scene_.Player().position,time_);
  courtview::Draw(renderer,scene_,time_);
  expeditionview::Draw(renderer,scene_,time_);
  if(scene_.World().ritualActive) {
    const auto center=rpg::WorldMap::ritualSite;
    // The visible boundary is the same five-metre circle used by simulation.
    // Sample the heightfield at every ribbon corner, including sloping edges.
    const bool charging=scene_.World().ritualCharge>0;
    const XMFLOAT4 color=scene_.World().ritualStage==4?XMFLOAT4{.35f,.8f,.55f,.45f}:
      charging?XMFLOAT4{.55f,.76f,1,.5f}:XMFLOAT4{.65f,.58f,.36f,.32f};
    const auto at=[&](float a,float radius) -> XMFLOAT3 {
      const rpg::Vec2 p{center.x+std::cos(a)*radius,center.z+std::sin(a)*radius};
      return {p.x,rpg::WorldMap::Height(p)+.065f,p.z};
    };
    for(int i=0;i<48;++i) {
      const float a=i*6.2831853f/48,b=(i+1)*6.2831853f/48;
      renderer.DrawEffectRibbon(at(a,4.97f),at(a,5.03f),at(b,5.03f),at(b,4.97f),color,1,1);
    }
  }
  using M=visual::Mesh;
  using S=visual::Material;
  if(scene_.FriendlyTarget()>=0) {
    const auto p=rpg::Simulation::FriendlyPosition(scene_.FriendlyTarget());
    renderer.DrawEffectRing({p.x,rpg::WorldMap::Height(p)+.035f,p.z},.65f,{.35f,.85f,.4f,.65f});
  }
  for(std::size_t i=0;i<scene_.Dummies().size();++i) {
    const auto& dummy=scene_.Dummies()[i];
    if(!scene_.ShowsDummy(dummy))continue;
    const float ground=rpg::WorldMap::Height(dummy.position);
    if(scene_.Target() && scene_.Target()->id==dummy.id)
      renderer.DrawEffectRing({dummy.position.x,ground+.035f,dummy.position.z},.65f,{.85f,.65f,.25f,.65f});
  }
  const auto playerOutfit=scene_.Player().outfit;
  DrawCharacter(renderer,player,y,playerPose_.facing,scene_.Player().hurtFlash>0?XMFLOAT4{1,.48f,.35f,1}:
    XMFLOAT4{rpg::DescribeClass(scene_.Player().classId).color[0],rpg::DescribeClass(scene_.Player().classId).color[1],rpg::DescribeClass(scene_.Player().classId).color[2],1},rpg::DescribeClass(scene_.Player().classId).range>3?0:playerPose_.attack,playerPose_.gait,playerOutfit,playerAnimation_.pose,1);
  const auto anchor=rpg::WorldMap::astralAnchor;
  const bool stable=scene_.World().anchorStable;
  const XMFLOAT4 crystal=stable?XMFLOAT4{.3f,.95f,.65f,1}:XMFLOAT4{.72f,.4f,.95f,1};
  renderer.DrawMesh(M::Rock,{anchor.x,.12f,anchor.z},{1.2f,.32f,1.2f},{.45f,.42f,.55f,1},S::Stone);
  renderer.DrawMesh(M::Crystal,{anchor.x,1.1f+std::sin(time_*2)*.12f,anchor.z},{.65f,1.4f,.65f},crystal,S::Glow,time_*.5f,0,visual::ShadowMode::None);
  if (scene_.World().channeling) {
    for (int i=0;i<8;++i) {
      const float angle=time_*2+i*.7854f;
      renderer.DrawMarkerBox({anchor.x+std::sin(angle),.5f+scene_.World().channelProgress,anchor.z+std::cos(angle)},
        {.15f,.15f,.15f},crystal,angle);
    }
  }
  const auto& enemies=scene_.Combatants();
  for (int i=0;i<static_cast<int>(enemies.size())+int(scene_.Sparring());++i) {
    const auto& enemy=i<int(enemies.size())?enemies[i]:scene_.Trainer();
    if(enemy.Beast() && std::hypot(enemy.position.x-player.x,enemy.position.z-player.z)>36)continue;
    const float ground=rpg::WorldMap::Height(enemy.position);
    if (enemy.health<=0) {
      if (!enemy.looted) {
        if(enemy.kind!=rpg::EnemyKind::Trainer) {
          const auto asset=enemy.kind==rpg::EnemyKind::Wolf?authored::Asset::Wolf:enemy.kind==rpg::EnemyKind::Boar?authored::Asset::Boar:enemy.kind==rpg::EnemyKind::Wraith?authored::Asset::Revenant:
            (enemy.kind==rpg::EnemyKind::Captain||enemy.kind==rpg::EnemyKind::Reaver)?authored::Asset::Orc:authored::Asset::Skeleton;
          const auto& defeat=enemyActors_[i].defeat;const float keep=defeat.Remaining();
          renderer.DrawAnimatedAsset(asset,{enemy.position.x,ground,enemy.position.z},enemy.facing,defeat.time,
            defeat.gait,defeat.attack*keep,defeat.travel*keep,3,{.8f,.8f,.8f,1},defeat.Gesture());
        } else renderer.DrawMesh(M::Rock,{enemy.position.x,ground+.22f,enemy.position.z},{.55f,.44f,.45f},{.51f,.34f,.15f,1},S::Cloth);
        renderer.DrawMesh(M::Crystal,{enemy.position.x,ground+1.f+std::sin(time_*3)*.1f,enemy.position.z},{.18f,.3f,.18f},{1,.82f,.3f,1},S::Glow,0,0,visual::ShadowMode::None);
      }
      continue;
    }
    const bool captain=enemy.kind==rpg::EnemyKind::Captain;
    CharacterOutfit enemyOutfit; enemyOutfit.armor=captain; enemyOutfit.helm=captain;
    enemyOutfit.face=HumanFace::Broad;enemyOutfit.appearance={0,1,0};
    if(enemy.kind==rpg::EnemyKind::Wraith) {enemyOutfit.weapon=false;enemyOutfit.cloak=true;}
    if(enemy.Boss()) {enemyOutfit.shoulders=true;enemyOutfit.cloak=true;enemyOutfit.gloves=true;}
    const auto& pose=enemyActors_[i].motion;
    if(enemy.kind!=rpg::EnemyKind::Trainer) {
      const auto asset=enemy.kind==rpg::EnemyKind::Wolf?authored::Asset::Wolf:enemy.kind==rpg::EnemyKind::Boar?authored::Asset::Boar:enemy.kind==rpg::EnemyKind::Wraith?authored::Asset::Revenant:
        (captain||enemy.kind==rpg::EnemyKind::Reaver)?authored::Asset::Orc:authored::Asset::Skeleton;
      const auto gesture=enemyActors_[i].Gesture();
      renderer.DrawAnimatedAsset(asset,{enemy.position.x,ground,enemy.position.z},pose.facing,float(enemyActors_[i].clock),pose.phase,pose.attack,enemyActors_[i].animation.pose.travel,3,
        enemy.hitFlash>0?XMFLOAT4{1,.7f,.55f,1}:XMFLOAT4{1,1,1,1},gesture);
    } else DrawCharacter(renderer,enemy.position,ground,pose.facing,
      enemy.hitFlash>0?XMFLOAT4{1,.7f,.55f,1}:enemy.kind==rpg::EnemyKind::Trainer?XMFLOAT4{.55f,.46f,.22f,1}:enemy.kind==rpg::EnemyKind::Wraith?XMFLOAT4{.35f,.30f,.61f,1}:enemy.kind==rpg::EnemyKind::Reaver?XMFLOAT4{.53f,.34f,.19f,1}:captain?XMFLOAT4{.36f,.19f,.48f,1}:XMFLOAT4{.65f,.2f,.25f,1},
      enemy.kind==rpg::EnemyKind::Wraith?0.f:pose.attack,pose.gait,enemyOutfit,enemyActors_[i].animation.pose,std::uint64_t(enemy.id)+16);
    if (scene_.Target() && enemy.id==scene_.Target()->id) {
      // An open ground outline leaves feet and shadows visible. Health belongs
      // to the target HUD instead of a floating opaque block above the head.
      const auto point=[&](float angle,float radius) -> XMFLOAT3 {
        const rpg::Vec2 p{enemy.position.x+std::sin(angle)*radius,enemy.position.z+std::cos(angle)*radius};
        return {p.x,rpg::WorldMap::Height(p)+.035f,p.z};
      };
      for(int segment=0;segment<40;++segment) {
        const float a=segment*motion::tau/40,b=(segment+1)*motion::tau/40;
        renderer.DrawEffectRibbon(point(a,.62f),point(a,.66f),point(b,.66f),point(b,.62f),{1,.68f,.18f,.7f});
      }
    }
  }
  const bool preparing=scene_.Player().recovery || (scene_.Player().attacking && rpg::DescribeClass(scene_.Player().classId).range>3 &&
    scene_.Player().attackSeconds<combat::StrokeStart(scene_.Player().heavy));
  auto sockets=castfx::Preview({player.x,y,player.z},{player.x,y+1.25f,player.z},playerPose_.facing);
  sockets.alive=scene_.Player().health>0;
  if(preparing || effects_.NeedsPose() || scene_.Player().disciplineSeconds>0 || scene_.Player().guardWindow>0 || scene_.Player().elementMask!=0) {
    const bool shield=playerOutfit.shield;
    const auto form=rpg::WeaponFor(scene_.Player().classId).form;
    const auto pose=BuildCharacterPose(rpg::DescribeClass(scene_.Player().classId).range>3?0:playerPose_.attack,playerPose_.gait,shield,playerAnimation_.pose,form);
    sockets=castfx::FromPose(pose,{player.x,y,player.z},playerPose_.facing,form,
      shield && !(scene_.Player().classId==rpg::ClassId::Scout && playerOutfit.weapon),playerOutfit.weapon,sockets.alive);
  }
  if(preparing)castfx::Prepare(renderer,scene_.Player(),sockets);
  effects_.Illuminate(renderer,scene_.Player(),sockets);
  castfx::Discipline(renderer,scene_.Player(),sockets,time_);
  sustained_.Render(renderer,scene_.Player(),time_,playerPose_.facing);
  effects_.Render(renderer,&sockets);
  const bool drawInterface=showInterface && (hudVisible_ || interface_.HasPanels() || scene_.Player().health<=0);
  const ui::Layout textLayout(float(renderer.Width()),float(renderer.Height()));
  for (std::size_t i=0;drawInterface && i<scene_.Feedback().size();++i) {
    const auto& feedback=scene_.Feedback()[i];
    const auto view=ui::ReadCombatText(feedback);if(view.opacity<=0)continue;
    DirectX::XMFLOAT2 screen;
    if (!renderer.Project({feedback.position.x,rpg::WorldMap::Height(feedback.position)+2.25f,feedback.position.z},screen)) continue;
    const float size=view.size*textLayout.scale;
    const float width=renderer.TextWidth(view.text,size);
    screen.x+=(view.x*textLayout.scale-width*.5f)/renderer.Width();screen.y+=view.y*textLayout.scale/renderer.Height();
    if(screen.x<.015f || screen.x+width/renderer.Width()>.985f || screen.y<.045f || screen.y>.80f)continue;
    const float edge=std::max(1.f,textLayout.scale);
    for(const auto offset:{DirectX::XMFLOAT2{-edge,0},DirectX::XMFLOAT2{edge,0},DirectX::XMFLOAT2{0,-edge},DirectX::XMFLOAT2{0,edge}})
      renderer.DrawText(screen.x+offset.x/renderer.Width(),screen.y+offset.y/renderer.Height(),view.text,{.025f,.018f,.012f,view.opacity*.9f},size);
    renderer.DrawText(screen.x,screen.y,view.text,{view.color[0],view.color[1],view.color[2],view.opacity},size);
  }
  if (drawInterface) interface_.Render(renderer,simulation_,time_,debugOverlay_,cameraYaw_);
}

std::wstring Game::WindowTitle() const {
  return L"Mediterranean - Astral Frontier | Native prototype";
}

void Game::LoadHotbarSettings() {
  savedHotbars_.reset();std::string content;
  ui::Interface defaults;interface_.LoadHotbars(defaults.SaveHotbars());
  auto file=rosterFile_;file+=L".hotbars";
  if(profile::Read(file,content)==profile::ReadResult::Ready && interface_.LoadHotbars(content)) savedHotbars_=content;
}
bool Game::SaveHotbarSettings() {
  auto file=rosterFile_;file+=L".hotbars";
  const auto bars=interface_.SaveHotbars();
  if(savedHotbars_ && *savedHotbars_==bars) return true;
  if(profile::Write(file,bars,savedHotbars_?&*savedHotbars_:nullptr)!=profile::WriteResult::Saved) return false;
  savedHotbars_=bars;return true;
}
bool Game::SaveProgress() {
  const auto content=simulation_.Save();
  const auto result=profile::Write(saveFile_,content,savedSnapshot_?&*savedSnapshot_:nullptr);
  switch(result) {
  case profile::WriteResult::Saved:savedSnapshot_=content;Notify("Progress saved.");break;
  case profile::WriteResult::Conflict:Notify("Save changed outside this game. F9 loads it; current progress was not written.");break;
  case profile::WriteResult::Busy:Notify("Another game is saving this character. Try again.");break;
  case profile::WriteResult::Error:Notify("Save failed. Previous save retained. Check project folder access.");break;
  }
  if(result==profile::WriteResult::Saved) {
    if(!SaveHotbarSettings()) Notify("Progress saved; action-bar settings could not be saved (external change or file access).");
  }
  return result==profile::WriteResult::Saved;
}

void Game::LoadProgress() {
  std::string content;
  if(profile::Read(saveFile_,content)!=profile::ReadResult::Ready) {Notify("No valid save found.");return;}
  rpg::Simulation candidate;
  if(!candidate.Load(content) || candidate.Class()!=simulation_.Class() || !RestoreSnapshot(content)) {
    Notify("Save rejected. Current progress retained.");return;
  }
  savedSnapshot_=std::move(content);
  LoadHotbarSettings();
}
