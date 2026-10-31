#include "game/Localization.h"
#include "game/Game.h"
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
#include <algorithm>
#include <cmath>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace {
float EnemyStrike(const rpg::Enemy& enemy,bool active,float releaseAge=1) {
  // Cooldowns are frozen while defeated or outside a sparring round. They must
  // not keep driving the presentation toward an unfinished strike indefinitely.
  if (!active || enemy.health<=0) return 0;
  return enemy.windup>0?combat::Smooth(1-enemy.windup/enemy.WindupDuration()):
    1-combat::Smooth(releaseAge/.4f);
}
float EnemyAttackSeconds(const rpg::Enemy& enemy,bool active,float releaseAge) {
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
  worldView_.Reset();
  interface_.SetOnly(ui::Panel::None); LoadHotbarSettings(); orbiting_=false;
  yaw_=0; pitch_=.45f; distance_=10;
  ResetMotion();
}

void Game::ChooseCharacter(const std::filesystem::path& save) {
  rosterFile_=save;LoadHotbarSettings();
  CancelUiGesture(); selection_.Open(save); choosingCharacter_=true; inputBarrier_=false;
}

bool Game::RestoreSnapshot(const std::string& snapshot) {
  if (!simulation_.Load(snapshot)) return false;
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
  const float dt=std::isfinite(seconds)?static_cast<float>(std::clamp(seconds,0.0,.05)):0.f;
  // Require a fresh gesture on either side of the roster/world transition.
  if(inputBarrier_) {
    for(int key=1;key<256;++key) if(input.Held(key)) return;
    inputBarrier_=false;return;
  }
  if(choosingCharacter_) {
    const auto action=selection_.Update(dt,input);
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
  if (uiVisible && input.Focused())
    event=interface_.Pointer(input.ClientX(),input.ClientY(),input.Width(),input.Height(),
      input.Pressed(VK_LBUTTON),input.Pressed(VK_RBUTTON),input.Held(VK_LBUTTON),input.Released(VK_LBUTTON));
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
  case ui::Action::Close: interface_.Close(static_cast<ui::Panel>(event.value)); break;
  case ui::Action::Menu: interface_.Escape(); break;
  case ui::Action::Character: interface_.Toggle(ui::Panel::Character); break;
  case ui::Action::Inventory: interface_.Toggle(ui::Panel::Inventory); break;
  case ui::Action::Map: interface_.Toggle(ui::Panel::Map); break;
  case ui::Action::Journal: interface_.Toggle(ui::Panel::Journal); break;
  case ui::Action::SelectJournal:
    if (event.value>=-1 && event.value<rpg::frontierQuestCount+2) {
      interface_.journalSelection=event.value; interface_.InvalidateHitRegions();
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
    else simulation_.Notify("Draft needs its prerequisite and must fit the five-point budget.");
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
    selection_.Open(rosterFile_,simulation_.Class());
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
    else simulation_.Notify("Place your waypoint on traversable ground.");
    break;
  default: break;
  }
  const bool pointerCaptured=uiVisible && interface_.PointerOverUi();
  // Pointer actions above can open or close a modal panel in this very frame.
  command.haltMovement=interface_.CapturesMovement() || !input.Focused();
  if (!input.Focused() || interface_.CapturesMovement() || interface_.Dragging() || (orbiting_ && pointerCaptured)) CancelOrbit();
  else if (!input.Held(VK_RBUTTON)) orbiting_=false;
  if (input.Pressed(VK_RBUTTON))
    orbiting_=!pointerCaptured && !interface_.CapturesMovement() && !interface_.Dragging();
  if (!interface_.CapturesMovement()) {
    if (!pointerCaptured && !interface_.Dragging()) {
      yaw_=std::remainder(yaw_+(orbiting_?input.MouseX()*.006f:0),motion::tau);
      pitch_=std::clamp(pitch_+(orbiting_?input.MouseY()*.004f:0),-.75f,1.1f);
      if (std::isfinite(wheel)) distance_=std::clamp(distance_-wheel,3.f,18.f);
    }
    const float forward=float(input.Held('W'))-float(input.Held('S'));
    const float right=float(input.Held('D'))-float(input.Held('A'));
    // Predict the same filtered heading used by UpdateCamera below. Steering
    // against the raw target makes WASD drift sideways during fast mouse turns.
    const float movementYaw=motion::FollowAngle(cameraYaw_,yaw_,30.f,dt);
    command.steerFacing=orbiting_ && input.Focused(); command.facingYaw=movementYaw;
    command.movement={std::sin(movementYaw)*forward+std::cos(movementYaw)*right,
      std::cos(movementYaw)*forward-std::sin(movementYaw)*right};
    command.sprint=input.Held(VK_SHIFT); command.jump=input.Pressed(VK_SPACE);
    command.nextTarget=input.Pressed(VK_TAB); command.interact=input.Pressed('E') && !closingMerchant && !closingQuest;
    command.restart=input.Pressed('R');
    command.usePotion=command.usePotion || input.Pressed('Q'); command.equipWeapon=command.equipWeapon || input.Pressed('G');
    for (int i=0;i<10;++i) if (input.Pressed(i==9?'0':'1'+i)) {
      const bool control=input.Held(VK_CONTROL) || input.Held(VK_LCONTROL) || input.Held(VK_RCONTROL);
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
  ExecuteCommand(command,dt);
  if (wasAlive && simulation_.Health()<=0) {
    CancelUiGesture(); interface_.Close(interface_.panel);
  }
  if (simulation_.TradeRequested()) { CancelOrbit(); interface_.merchantNotice.clear(); interface_.Show(ui::Panel::Merchant); }
  if (command.purchase!=-1 || command.trade.action!=rpg::TradeAction::None) interface_.merchantNotice=simulation_.Message();
  if (interface_.panel==ui::Panel::Merchant && !simulation_.CanTrade()) interface_.Close(ui::Panel::Merchant);
  if (command.inventory.action!=rpg::InventoryAction::None) {
    interface_.inventoryNotice=simulation_.Message(); interface_.noticeTime=3;
  }
}
void Game::ExecuteCommand(const rpg::Command& command,float seconds) {
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0.f;
  const bool wasDead=simulation_.Health()<=0,wasSparring=simulation_.Sparring();
  const auto previousVelocity=simulation_.MovementVelocity();
  const auto previousPosition=simulation_.Player(); const float previousFacing=playerPose_.facing;
  const auto priorDialogue=simulation_.QuestDialogue(); const auto priorStage=simulation_.DialogueStage();
  simulation_.Update(dt,command);
  if (dt<=0) return;
  if (simulation_.QuestDialogue()!=rpg::QuestId::None) {
    if (interface_.panel!=ui::Panel::Quest) { CancelUiGesture(); interface_.SetOnly(ui::Panel::Quest); }
    if (priorDialogue!=simulation_.QuestDialogue() || priorStage!=simulation_.DialogueStage()) {
      interface_.questPage=0; interface_.InvalidateHitRegions();
    }
  } else if (interface_.panel==ui::Panel::Quest) interface_.Close(ui::Panel::Quest);
  time_+=dt;
  worldView_.UpdateWildlife(dt,simulation_.Map(),simulation_.Player(),simulation_.Health()>0);
  atmosphereTime_=std::fmod(atmosphereTime_+dt,1800.0);
  if (command.restart || (wasDead && simulation_.Health()>0)) { worldView_.ResetWildlife(); ResetMotion(); return; }
  interface_.UpdateCombatFeedback(dt,simulation_);
  effects_.Advance(dt);
  for (std::size_t i=0;i<simulation_.VisualEventCount();++i) {
    const auto& event=simulation_.VisualEvents()[i];
    if(event.kind==rpg::CombatVisualKind::SpellCast) {
      auto launch=playerAnimation_.pose;
      launch.attackSeconds=-1; launch.cast=0;
      launch.action=animation::SampleAction(event.school,event.opening?animation::ActionClip::Opening:event.signature?animation::ActionClip::Signature:event.technique?animation::ActionClip::Technique:event.heavy?animation::ActionClip::Heavy:animation::ActionClip::Basic,
        combat::StrokeStart(event.heavy));
      const bool shield=simulation_.Items().Worn(rpg::GearSlot::Offhand)!=rpg::Item::None;
      const auto form=rpg::WeaponFor(event.school).form;
      const auto releasePose=BuildCharacterPose(0,playerPose_.gait,shield,launch,form);
      const auto local=rpg::PhysicalLauncher(form)?WeaponReleasePoint(releasePose,form):SpellReleasePoint(releasePose,shield);
      auto actor=playerPose_;
      actor.Update(event.facing,0,simulation_.Grounded(),true,0,std::max(0.f,dt-event.elapsed),false,42.f);
      const float sx=std::sin(actor.facing),cx=std::cos(actor.facing);
      const DirectX::XMFLOAT3 origin{event.position.x+cx*local.x+sx*local.z,event.height+local.y,
        event.position.z-sx*local.x+cx*local.z};
      effects_.Emit(event,&origin);
    } else effects_.Emit(event);
  }
  const auto dashPosition=simulation_.Player();
  effects_.TrackDash({dashPosition.x,simulation_.WorldHeight(),dashPosition.z},simulation_.Dashing(),dt,
    simulation_.Dashing()?motion::DashSpeed(simulation_.DashPhase()):0);
  if (command.haltMovement || simulation_.Health()<=0) effects_.CancelDash();
  if (simulation_.Health()<=0) effects_.CancelTrails();
  if (wasSparring && !simulation_.Sparring()) effects_.CancelTrail(simulation_.SparringOpponent().id);
  playerPose_.Update(simulation_.Facing(),simulation_.GroundMovementDistance(),simulation_.Grounded() && !simulation_.Dashing(),
    simulation_.Health()>0,simulation_.AttackFlash()*5,dt,simulation_.Dashing() || simulation_.DashedThisUpdate(),
    simulation_.Attacking()?42.f:22.f);
  const auto velocity=simulation_.MovementVelocity();
  const auto applyImpacts=[&](animation::State& state,rpg::EntityId actor,bool alive,float facing) {
    if (!alive) return;
    for (std::size_t i=0;i<simulation_.VisualEventCount();++i) {
      const auto& event=simulation_.VisualEvents()[i];
      if (event.actor==actor && (event.kind==rpg::CombatVisualKind::Hurt ||
          event.kind==rpg::CombatVisualKind::Hit || event.kind==rpg::CombatVisualKind::HeavyHit))
        state.ApplyImpact(std::clamp(event.elapsed,0.f,dt),std::sin(event.facing-facing));
    }
  };
  bool castStarted=false,dashImpact=false,dashFinished=false;
  float dashEndElapsed=0;
  for(std::size_t i=0;i<simulation_.VisualEventCount();++i) {
    castStarted|=simulation_.VisualEvents()[i].kind==rpg::CombatVisualKind::Mend;
    const auto& event=simulation_.VisualEvents()[i];
    if (event.kind==rpg::CombatVisualKind::DashImpact || event.kind==rpg::CombatVisualKind::DashStop) {
      dashImpact=event.kind==rpg::CombatVisualKind::DashImpact; dashFinished=true; dashEndElapsed=event.elapsed;
    }
  }
  const float forwardAcceleration=command.haltMovement?0:
    ((velocity.x-previousVelocity.x)*std::sin(playerPose_.facing)+
     (velocity.z-previousVelocity.z)*std::cos(playerPose_.facing))/dt;
  playerAnimation_.Update(dt,simulation_.Height(),simulation_.VerticalSpeed(),simulation_.MovementDistance(),
    simulation_.Health()>0,playerPose_.attack,
    simulation_.LandingSpeed(),simulation_.LandingElapsed(),forwardAcceleration,castStarted);
  applyImpacts(playerAnimation_,0,simulation_.Health()>0,playerPose_.facing);
  playerAnimation_.UpdateDash(dt,simulation_.Dashing(),simulation_.DashPhase(),dashImpact,simulation_.Health()>0,dashFinished,dashEndElapsed);
  playerAnimation_.pose.attackSeconds=simulation_.AttackSeconds();
  playerAnimation_.pose.retreat=simulation_.RetreatMobility();
  if(simulation_.ClassInfo().range>3 && simulation_.Attacking())
    playerAnimation_.pose.attackSeconds=-1;
  playerAnimation_.actions.Update(dt,simulation_.Class(),simulation_.AttackSeconds(),simulation_.HeavyAttack(),
    simulation_.CastingRecovery()?std::min(.30f,simulation_.RecoveryProgress()*1.5f):playerAnimation_.castAge,simulation_.Health()>0,simulation_.TechniqueAttack(),simulation_.SignatureAttack(),simulation_.OpeningAttack());
  playerAnimation_.pose.action=playerAnimation_.actions.pose;
  if(simulation_.Class()!=rpg::ClassId::Warrior) playerAnimation_.pose.cast=0;
  playerAnimation_.pose.heavyAttack=simulation_.HeavyAttack();
  playerAnimation_.SettleWeapon(dt,simulation_.Health()>0);
  const float lateralAcceleration=command.haltMovement?0:
    ((velocity.x-previousVelocity.x)*std::cos(playerPose_.facing)-
     (velocity.z-previousVelocity.z)*std::sin(playerPose_.facing))/dt;
  motion::Spring(playerAnimation_.pose.bank,playerAnimation_.bankVelocity,
    simulation_.Grounded()?std::clamp(lateralAcceleration/50.f,-1.f,1.f):0.f,18.f,dt);
  if (!simulation_.Attacking()) { playerPose_.attack=0; playerPose_.attackVelocity=0; effects_.CancelTrail(0); }
  if (command.haltMovement) effects_.CancelTrail(0);
  if (command.haltMovement) playerAnimation_.CancelDash();
  playerAnimation_.pose.lean*=1-playerAnimation_.pose.dash;
  const float dx=simulation_.Player().x-previousPosition.x,dz=simulation_.Player().z-previousPosition.z;
  const float moved=std::hypot(dx,dz),c=std::cos(playerPose_.facing),s=std::sin(playerPose_.facing);
  playerAnimation_.locomotion.Update(dt,simulation_.GroundMovementDistance(),moved>.000001f?(s*dx+c*dz)/moved:0,
    moved>.000001f?(c*dx-s*dz)/moved:0,motion::AngleDelta(previousFacing,playerPose_.facing)/dt,
    simulation_.Grounded(),simulation_.VerticalSpeed(),simulation_.Height(),simulation_.Health()>0,simulation_.LandingElapsed());
  playerAnimation_.pose.motion=playerAnimation_.locomotion.pose;
  const auto weaponForm=rpg::WeaponFor(simulation_.Class()).form;
  const bool hasWeapon=simulation_.Items().Worn(rpg::GearSlot::Weapon)!=rpg::Item::None;
  const bool staff=rpg::StaffWeapon(weaponForm);
  const bool bow=weaponForm==rpg::WeaponForm::Handbow;
  const bool blade=weaponForm==rpg::WeaponForm::Blade||weaponForm==rpg::WeaponForm::Mace||weaponForm==rpg::WeaponForm::Cleaver;
  const auto equipment=hasWeapon&&staff?playerclips::Equipment::Staff:
    hasWeapon&&blade?playerclips::Equipment::Sword:hasWeapon&&bow?playerclips::Equipment::Bow:playerclips::Equipment::None;
  if(simulation_.Health()>0) animation::ApplyAuthoredLocomotion(playerAnimation_,dt,equipment);
  playerAnimation_.authoredWeapon.Update(dt,simulation_.AttackSeconds(),simulation_.HeavyAttack(),staff,
    simulation_.Health()>0&&hasWeapon&&(staff||blade||bow)&&!simulation_.Dashing()&&!command.haltMovement,simulation_.AttackRate(),bow);
  playerAnimation_.pose.authoredStrike=playerAnimation_.authoredWeapon.frame;
  playerAnimation_.pose.authoredStrikeWeight=playerAnimation_.authoredWeapon.weight;
  playerAnimation_.pose.authoredStrikePhase=playerAnimation_.authoredWeapon.phase;
  playerAnimation_.pose.authoredStrikeHeavy=playerAnimation_.authoredWeapon.heavy;
  playerFeet_.Update(dt,simulation_.Player(),playerPose_.facing,simulation_.Grounded() && !simulation_.Dashing(),
    simulation_.Health()>0,playerAnimation_.pose);
  const auto& enemies=simulation_.Enemies();
  if (enemyPoses_.size()!=enemies.size()+1) ResetMotion();
  for (std::size_t i=0;i<enemyPoses_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:simulation_.SparringOpponent();
    if (i==enemies.size() && !wasSparring && simulation_.Sparring()) {
      enemyPoses_[i].Reset(enemy.facing); enemyAnimations_[i].Reset(float(enemy.id)*.37f);
      enemyFeet_[i].Reset(enemy.position,enemy.facing);
      enemyMovement_[i].Reset(enemy.position.x,enemy.position.z);
    }
    const bool active=simulation_.Health()>0 && enemy.health>0 && (i<enemies.size() || simulation_.Sparring());
    enemyDefeats_[i].Update(dt,enemy.health>0,enemyPoses_[i].attack,enemyPoses_[i].phase,enemyAnimations_[i].pose);
    auto& releaseAge=enemyAnimations_[i].enemyReleaseAge;
    releaseAge=std::min(1.f,releaseAge+dt);
    for(std::size_t eventIndex=0;eventIndex<simulation_.VisualEventCount();++eventIndex) {
      const auto& event=simulation_.VisualEvents()[eventIndex];
      if(event.actor==enemy.id && (event.kind==rpg::CombatVisualKind::EnemySwing || event.kind==rpg::CombatVisualKind::EnemyLance))
        releaseAge=std::clamp(event.elapsed,0.f,dt);
    }
    if(!active || enemy.windup>0) releaseAge=1;
    const float strike=EnemyStrike(enemy,active,releaseAge);
    const float turnRate=motion::AngleDelta(enemyPoses_[i].facing,enemy.facing)/dt;
    // Enemy heading is already smoothed by simulation, then frozen for attacks.
    // Use it exactly so the character always faces the displayed damage cone.
    enemyPoses_[i].Update(enemy.facing,enemy.movementDistance,true,enemy.health>0,strike,dt,true);
    auto& movement=enemyMovement_[i];
    movement.Update(enemy.position.x,enemy.position.z,enemy.facing,dt,enemy.health>0);
    enemyAnimations_[i].Update(dt,0,0,enemy.movementDistance,enemy.health>0,strike,0,0,movement.acceleration);
    motion::Spring(enemyAnimations_[i].pose.bank,enemyAnimations_[i].bankVelocity,
      enemy.health>0?std::clamp(movement.lateralAcceleration/50.f,-1.f,1.f):0.f,18.f,dt);
    applyImpacts(enemyAnimations_[i],enemy.id,enemy.health>0,enemy.facing);
    const float actionSeconds=EnemyAttackSeconds(enemy,active,releaseAge);
    enemyAnimations_[i].pose.attackSeconds=actionSeconds;
    enemyAnimations_[i].pose.heavyAttack=enemy.kind==rpg::EnemyKind::Captain || enemy.kind==rpg::EnemyKind::Reaver;
    const auto gestureClass=enemy.kind==rpg::EnemyKind::Wraith?rpg::ClassId::Mage:
      enemy.kind==rpg::EnemyKind::Reaver?rpg::ClassId::Demonologist:enemy.Boss()?rpg::ClassId::Paladin:rpg::ClassId::Warrior;
    enemyAnimations_[i].actions.Update(dt,gestureClass,actionSeconds,
      enemyAnimations_[i].pose.heavyAttack || enemy.kind==rpg::EnemyKind::Wraith,-1,active,false,enemy.Boss(),false,enemy.Boss()?int(enemy.bossAttack):-1);
    enemyAnimations_[i].pose.action=enemyAnimations_[i].actions.pose;
    if(enemy.kind==rpg::EnemyKind::Wraith) {
      enemyAnimations_[i].pose.attackSeconds=-1;
      enemyAnimations_[i].pose.cast=0;
    }
    enemyAnimations_[i].SettleWeapon(dt,active);
    enemyFeet_[i].Update(dt,enemy.position,enemy.facing,true,enemy.health>0,enemyAnimations_[i].pose);
    enemyAnimations_[i].locomotion.Update(dt,enemy.movementDistance,movement.forward,movement.right,turnRate,true,0,0,enemy.health>0);
    enemyAnimations_[i].pose.motion=enemyAnimations_[i].locomotion.pose;
  }
  UpdateFaces(dt);
  const auto track=[&](rpg::EntityId id,rpg::Vec2 position,float height,const motion::Actor& motion,
    const animation::Sample& animation,bool shield,bool armed) {
    if (!armed) { effects_.CancelTrail(id); return; }
    if (!effects_.Tracking(id)) return;
    const auto form=id==0?rpg::WeaponFor(simulation_.Class()).form:rpg::WeaponForm::Blade;
    const auto pose=BuildCharacterPose(motion.attack,motion.gait,shield,animation,form);
    const auto point=[&](float distance) -> DirectX::XMFLOAT3 {
      const auto p=pose.WeaponPoint(distance);
      return {position.x+std::cos(motion.facing)*p.x+std::sin(motion.facing)*p.z,height+p.y,
        position.z-std::sin(motion.facing)*p.x+std::cos(motion.facing)*p.z};
    };
    const float tip=form==rpg::WeaponForm::Mace?.67f:form==rpg::WeaponForm::Cleaver?.80f:.96f;
    effects_.TrackWeapon(id,point(.22f),point(tip));
  };
  track(0,simulation_.Player(),simulation_.WorldHeight(),playerPose_,playerAnimation_.pose,
    simulation_.Items().Worn(rpg::GearSlot::Offhand)!=rpg::Item::None,
    simulation_.Health()>0 && simulation_.Items().Worn(rpg::GearSlot::Weapon)!=rpg::Item::None);
  for (std::size_t i=0;i<enemyPoses_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:simulation_.SparringOpponent();
    track(enemy.id,enemy.position,rpg::WorldMap::Height(enemy.position),enemyPoses_[i],enemyAnimations_[i].pose,false,
      simulation_.Health()>0 && enemy.health>0 && enemy.kind!=rpg::EnemyKind::Wraith && (i<enemies.size() || simulation_.Sparring()));
  }
  UpdateCamera(dt);
}
void Game::UpdateFaces(float dt) {
  const auto player=simulation_.Player();
  const auto target=simulation_.TargetActor();
  const bool talking=simulation_.QuestDialogue()!=rpg::QuestId::None;
  const bool shopping=interface_.panel==ui::Panel::Merchant;
  const bool alive=simulation_.Health()>0;
  facial::Aim aim;
  if(talking || shopping) {
    const auto npc=talking?simulation_.DialoguePosition():rpg::WorldMap::quartermaster;
    aim=facial::LookAt(npc.x-player.x,rpg::WorldMap::Height(npc)-simulation_.WorldHeight(),npc.z-player.z,playerPose_.facing);
  } else if(target && target->health>0)
    aim=facial::LookAt(target->position.x-player.x,rpg::WorldMap::Height(target->position)-simulation_.WorldHeight(),target->position.z-player.z,playerPose_.facing);
  playerFace_.Update(dt,aim,simulation_.Attacking()?.85f:target && target->health>0?.25f:
    std::max(playerAnimation_.pose.cast,playerAnimation_.pose.action.open)*.35f,
    playerAnimation_.pose.hit,talking?.18f:0,alive);
  playerAnimation_.pose.face=playerFace_.pose;
  const auto& enemies=simulation_.Enemies();
  for(std::size_t i=0;i<enemyFaces_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:simulation_.SparringOpponent();
    const bool active=alive && enemy.health>0 && (i<enemies.size() || simulation_.Sparring());
    const float distance=std::hypot(enemy.position.x-player.x,enemy.position.z-player.z);
    const auto gaze=active && distance<8?facial::LookAt(player.x-enemy.position.x,simulation_.WorldHeight()-rpg::WorldMap::Height(enemy.position),
      player.z-enemy.position.z,enemyPoses_[i].facing):facial::Aim{};
    enemyFaces_[i].Update(dt,gaze,active && distance<8?.65f:0,enemyAnimations_[i].pose.hit,0,active);
    enemyAnimations_[i].pose.face=enemyFaces_[i].pose;
  }
  const auto npc=[&](facial::State& state,rpg::Vec2 position,float facing,bool interaction) {
    const float distance=std::hypot(player.x-position.x,player.z-position.z);
    const auto gaze=alive && distance<4?facial::LookAt(player.x-position.x,simulation_.WorldHeight()-rpg::WorldMap::Height(position),player.z-position.z,facing):facial::Aim{};
    state.Update(dt,gaze,0,0,interaction?.5f:distance<3?.12f:0);
  };
  npc(keeperFace_,rpg::WorldMap::questGiver,1.25f,talking && simulation_.QuestDialogue()<rpg::QuestId::Watch8);
  npc(merchantFace_,rpg::WorldMap::quartermaster,0,shopping);
}
void Game::SetCameraOrbit(float yaw,float pitch,float distance) {
  if (!std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(distance)) return;
  yaw_=std::remainder(yaw,6.2831853f);
  pitch_=std::clamp(pitch,-.75f,1.1f); distance_=std::clamp(distance,3.f,18.f);
  // Explicit positioning is used by captures/tools; interactive input sets targets.
  UpdateCamera(0,true);
}
void Game::ResetMotion() {
  interface_.ResetCombatFeedback();
  ++audioGeneration_;
  effects_.Reset();
  playerPose_.Reset(simulation_.Facing());
  playerAnimation_.Reset();
  playerFace_.Reset(1); keeperFace_.Reset(3); merchantFace_.Reset(2);
  playerFeet_.Reset(simulation_.Player(),simulation_.Facing());
  enemyPoses_.resize(simulation_.Enemies().size()+1);
  enemyAnimations_.resize(enemyPoses_.size());
  enemyDefeats_.resize(enemyPoses_.size());
  enemyFaces_.resize(enemyPoses_.size());
  enemyFeet_.resize(enemyPoses_.size());
  enemyMovement_.resize(enemyPoses_.size());
  for (std::size_t i=0;i<enemyPoses_.size();++i) {
    const auto& enemy=i<simulation_.Enemies().size()?simulation_.Enemies()[i]:simulation_.SparringOpponent();
    enemyPoses_[i].Reset(enemy.facing);
    enemyDefeats_[i].Reset(enemy.health>0);
    enemyAnimations_[i].Reset(float(enemy.id)*.37f);
    enemyFaces_[i].Reset(enemy.id+16);
    enemyFeet_[i].Reset(enemy.position,enemy.facing);
    enemyMovement_[i].Reset(enemy.position.x,enemy.position.z);
    enemyPoses_[i].attack=EnemyStrike(enemy,simulation_.Health()>0 &&
      (i<simulation_.Enemies().size() || simulation_.Sparring()));
  }
  UpdateCamera(0,true);
}
void Game::UpdateCamera(float dt,bool immediate) {
  // Keep the horizon stable: the actor jumps within the frame instead of
  // dragging the entire camera through the full ballistic arc.
  const float height=rpg::WorldMap::Height(simulation_.Player())+simulation_.Height()*.35f+1.4f;
  cameraYaw_=immediate?yaw_:motion::FollowAngle(cameraYaw_,yaw_,30.f,dt);
  cameraPitch_=immediate?pitch_:motion::Ease(cameraPitch_,pitch_,30.f,dt);
  cameraHeight_=immediate?height:motion::Ease(cameraHeight_,height,18.f,dt);
  const rpg::Vec2 direction{-std::sin(cameraYaw_)*std::cos(cameraPitch_),-std::cos(cameraYaw_)*std::cos(cameraPitch_)};
  const float probe=immediate?distance_:std::max(distance_,cameraBoom_.distance);
  const float clearance=simulation_.CameraDistance(simulation_.Player(),cameraHeight_,direction,std::sin(cameraPitch_),probe);
  if (immediate) cameraBoom_.Reset(clearance);
  else cameraBoom_.Update(distance_,clearance,dt);
}

void Game::Render(Renderer& renderer,bool showInterface) {
  if(choosingCharacter_) {ui::RenderCharacterSelection(renderer,selection_);return;}
  renderer.SetAtmosphereTime(atmosphereTime_);
  if (!renderer.HasTerrainSurface() && !renderer.SetTerrainSurface(BuildTerrainSurface(simulation_.Map()),terrainSurfaceSize)) return;
  using DirectX::XMFLOAT3;
  using DirectX::XMFLOAT4;
  const auto player=simulation_.Player();
  const float y=simulation_.WorldHeight();
  const rpg::Vec2 direction{-std::sin(cameraYaw_)*std::cos(cameraPitch_),-std::cos(cameraYaw_)*std::cos(cameraPitch_)};
  const float cameraDistance=cameraBoom_.distance;
  renderer.SetCamera({player.x+direction.x*cameraDistance,cameraHeight_+std::sin(cameraPitch_)*cameraDistance,
    player.z+direction.z*cameraDistance},{player.x,cameraHeight_,player.z});
  worldView_.Draw(renderer,simulation_.Map(),time_,simulation_.Sparring(),merchantFace_.pose,keeperFace_.pose);
  if(simulation_.RitualActive()) {
    const auto center=rpg::WorldMap::ritualSite;
    // The visible boundary is the same five-metre circle used by simulation.
    // Sample the heightfield at every ribbon corner, including sloping edges.
    const bool charging=simulation_.RitualCharge()>0;
    const XMFLOAT4 color=simulation_.RitualStage()==4?XMFLOAT4{.35f,.8f,.55f,.45f}:
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
  if(simulation_.FriendlyTarget()>=0) {
    const auto p=rpg::Simulation::FriendlyPosition(simulation_.FriendlyTarget());
    renderer.DrawEffectRing({p.x,rpg::WorldMap::Height(p)+.035f,p.z},.65f,{.35f,.85f,.4f,.65f});
  }
  for(const auto& dummy:simulation_.Dummies()) if(!(simulation_.Sparring() && dummy.id==210)) {
    if(simulation_.TargetActor() && simulation_.TargetActor()->id==dummy.id)
      renderer.DrawEffectRing({dummy.position.x,.035f,dummy.position.z},.65f,{.85f,.65f,.25f,.65f});
    if(dummy.hitFlash>0) renderer.DrawEffectRing({dummy.position.x,1.3f,dummy.position.z},.35f,{1,.65f,.25f,dummy.hitFlash*3});
  }
  DrawCharacter(renderer,player,y,playerPose_.facing,simulation_.HurtFlash()>0?XMFLOAT4{1,.48f,.35f,1}:
    XMFLOAT4{simulation_.ClassInfo().color[0],simulation_.ClassInfo().color[1],simulation_.ClassInfo().color[2],1},simulation_.ClassInfo().range>3?0:playerPose_.attack,playerPose_.gait,CharacterOutfit::From(simulation_.Items(),simulation_.Class(),simulation_.Look()),playerAnimation_.pose,1);
  const auto anchor=rpg::WorldMap::astralAnchor;
  const bool stable=simulation_.AnchorState()>=rpg::AnchorQuest::Return;
  const XMFLOAT4 crystal=stable?XMFLOAT4{.3f,.95f,.65f,1}:XMFLOAT4{.72f,.4f,.95f,1};
  renderer.DrawMesh(M::Rock,{anchor.x,.12f,anchor.z},{1.2f,.32f,1.2f},{.45f,.42f,.55f,1},S::Stone);
  renderer.DrawMesh(M::Crystal,{anchor.x,1.1f+std::sin(time_*2)*.12f,anchor.z},{.65f,1.4f,.65f},crystal,S::Glow,time_*.5f,0,visual::ShadowMode::None);
  if (simulation_.Channeling()) {
    for (int i=0;i<8;++i) {
      const float angle=time_*2+i*.7854f;
      renderer.DrawMarkerBox({anchor.x+std::sin(angle),.5f+simulation_.ChannelProgress(),anchor.z+std::cos(angle)},
        {.15f,.15f,.15f},crystal,angle);
    }
  }
  const auto& enemies=simulation_.Enemies();
  for (int i=0;i<static_cast<int>(enemies.size())+int(simulation_.Sparring());++i) {
    const auto& enemy=i<int(enemies.size())?enemies[i]:simulation_.SparringOpponent();
    const float ground=rpg::WorldMap::Height(enemy.position);
    if (enemy.health<=0) {
      if (!enemy.looted) {
        if(enemy.kind!=rpg::EnemyKind::Trainer) {
          const auto asset=enemy.kind==rpg::EnemyKind::Wraith?authored::Asset::Revenant:
            (enemy.kind==rpg::EnemyKind::Captain||enemy.kind==rpg::EnemyKind::Reaver)?authored::Asset::Orc:authored::Asset::Skeleton;
          const auto& defeat=enemyDefeats_[i];const float keep=defeat.Remaining();
          renderer.DrawAnimatedAsset(asset,{enemy.position.x,ground,enemy.position.z},enemy.facing,0,
            defeat.gait,defeat.attack*keep,defeat.travel*keep,3,{.8f,.8f,.8f,1},defeat.Gesture());
        } else renderer.DrawMesh(M::Rock,{enemy.position.x,ground+.22f,enemy.position.z},{.55f,.44f,.45f},{.51f,.34f,.15f,1},S::Cloth);
        renderer.DrawMesh(M::Crystal,{enemy.position.x,ground+1.f+std::sin(time_*3)*.1f,enemy.position.z},{.18f,.3f,.18f},{1,.82f,.3f,1},S::Glow,0,0,visual::ShadowMode::None);
      }
      continue;
    }
    const bool captain=enemy.kind==rpg::EnemyKind::Captain;
    CharacterOutfit enemyOutfit; enemyOutfit.armor=captain; enemyOutfit.helm=captain;
    if(enemy.kind==rpg::EnemyKind::Wraith) {enemyOutfit.weapon=false;enemyOutfit.cloak=true;}
    if(enemy.Boss()) {enemyOutfit.shoulders=true;enemyOutfit.cloak=true;enemyOutfit.gloves=true;}
    const auto& pose=enemyPoses_[i];
    if(enemy.kind!=rpg::EnemyKind::Trainer) {
      const auto asset=enemy.kind==rpg::EnemyKind::Wraith?authored::Asset::Revenant:
        (captain||enemy.kind==rpg::EnemyKind::Reaver)?authored::Asset::Orc:authored::Asset::Skeleton;
      renderer.DrawAnimatedAsset(asset,{enemy.position.x,ground,enemy.position.z},pose.facing,time_,pose.phase,pose.attack,enemyAnimations_[i].pose.travel,3,
        enemy.hitFlash>0?XMFLOAT4{1,.7f,.55f,1}:XMFLOAT4{1,1,1,1},AuthoredActorGesture(enemyAnimations_[i].pose));
    } else DrawCharacter(renderer,enemy.position,ground,pose.facing,
      enemy.hitFlash>0?XMFLOAT4{1,.7f,.55f,1}:enemy.kind==rpg::EnemyKind::Trainer?XMFLOAT4{.55f,.46f,.22f,1}:enemy.kind==rpg::EnemyKind::Wraith?XMFLOAT4{.35f,.30f,.61f,1}:enemy.kind==rpg::EnemyKind::Reaver?XMFLOAT4{.53f,.34f,.19f,1}:captain?XMFLOAT4{.36f,.19f,.48f,1}:XMFLOAT4{.65f,.2f,.25f,1},
      enemy.kind==rpg::EnemyKind::Wraith?0.f:pose.attack,pose.gait,enemyOutfit,enemyAnimations_[i].pose,std::uint64_t(enemy.id)+16);
    if (enemy.windup>0 && simulation_.Health()>0) {
      const float progress=1-enemy.windup/enemy.WindupDuration();
      const XMFLOAT4 warning=enemy.Interruptible()?XMFLOAT4{1,.55f*(1-progress),.12f,.68f}:XMFLOAT4{.85f,.4f,.95f,.75f};
      const auto point=[&](float angle,float radius) -> XMFLOAT3 {
        const rpg::Vec2 p{enemy.position.x+std::sin(angle)*radius,enemy.position.z+std::cos(angle)*radius};
        return {p.x,rpg::WorldMap::Height(p)+.045f,p.z};
      };
      for (int segment=0;segment<32;++segment) {
        const float a=enemy.facing-enemy.HalfAngle()+segment*enemy.HalfAngle()/16;
        const float b=a+enemy.HalfAngle()/16,r=enemy.Reach();
        renderer.DrawEffectRibbon(point(a,r-.025f),point(a,r+.025f),point(b,r+.025f),point(b,r-.025f),warning);
        if(enemy.Boss() && enemy.bossAttack!=0)
          renderer.DrawEffectRibbon(point(a,.05f),point(a,r),point(b,r),point(b,.05f),
            {warning.x,warning.y,warning.z,.10f+.12f*progress});
      }
      if(!(enemy.Boss() && enemy.bossAttack==1)) for (float side : {-1.f,1.f}) {
        const float angle=enemy.facing+side*enemy.HalfAngle();
        const auto a=point(angle,.24f),b=point(angle,enemy.Reach());
        const float x=std::cos(angle)*.018f,z=-std::sin(angle)*.018f;
        renderer.DrawEffectRibbon({a.x-x,a.y,a.z-z},{a.x+x,a.y,a.z+z},
          {b.x+x,b.y,b.z+z},{b.x-x,b.y,b.z-z},warning);
      }
      if (enemy.Boss() && enemy.bossAttack==0) for(int segment=0;segment<64;++segment) {
        const float a=segment*motion::tau/64,b=(segment+1)*motion::tau/64;
        for(float radius : {rpg::Enemy::shockInner,rpg::Enemy::shockOuter})
          renderer.DrawEffectRibbon(point(a,radius-.035f),point(a,radius+.035f),
            point(b,radius+.035f),point(b,radius-.035f),warning);
        // A faint annulus makes the dangerous band distinct from its safe centre.
        renderer.DrawEffectRibbon(point(a,rpg::Enemy::shockInner),point(a,rpg::Enemy::shockOuter),
          point(b,rpg::Enemy::shockOuter),point(b,rpg::Enemy::shockInner),{1,.12f,.06f,.10f+.12f*progress});
      }
    }
    if (i==simulation_.Target()) {
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
  if(simulation_.CastingRecovery() || (simulation_.Attacking() && simulation_.ClassInfo().range>3 &&
     simulation_.AttackSeconds()<combat::StrokeStart(simulation_.HeavyAttack()))) {
    const bool shield=simulation_.Items().Worn(rpg::GearSlot::Offhand)!=rpg::Item::None;
    const auto form=rpg::WeaponFor(simulation_.Class()).form;
    const auto pose=BuildCharacterPose(0,playerPose_.gait,shield,playerAnimation_.pose,form);
    const auto local=rpg::PhysicalLauncher(form)?WeaponReleasePoint(pose,form):SpellReleasePoint(pose,shield);
    const float sx=std::sin(playerPose_.facing),cx=std::cos(playerPose_.facing);
    classfx::Prepare(renderer,simulation_,{player.x+cx*local.x+sx*local.z,y+local.y,player.z-sx*local.x+cx*local.z});
  }
  classfx::States(renderer,simulation_,time_);
  effects_.Render(renderer);
  const bool drawInterface=showInterface && (hudVisible_ || interface_.HasPanels() || simulation_.Health()<=0);
  for (std::size_t i=0;drawInterface && i<simulation_.Feedback().size();++i) {
    const auto& feedback=simulation_.Feedback()[i];
    DirectX::XMFLOAT2 screen;
    if (!renderer.Project({feedback.position.x,rpg::WorldMap::Height(feedback.position)+2.5f+feedback.age*.8f+float(i%3)*.25f,feedback.position.z},screen)) continue;
    if (screen.x<.04f || screen.x>.84f || screen.y<.23f || screen.y>.68f) continue;
    XMFLOAT4 color{1,.85f,.35f,1}; std::string text;
    switch (feedback.kind) {
    case rpg::FeedbackKind::Damage: text=std::to_string(feedback.amount); break;
    case rpg::FeedbackKind::Hurt: text="-"+std::to_string(feedback.amount); color={1,.35f,.3f,1}; break;
    case rpg::FeedbackKind::Heal: text="HEAL "+std::to_string(feedback.amount); color={.4f,1,.6f,1}; break;
    case rpg::FeedbackKind::Experience: text="XP "+std::to_string(feedback.amount); color={.65f,.7f,1,1}; break;
    case rpg::FeedbackKind::LevelUp: text="LEVEL "+std::to_string(feedback.amount)+" / N: BUILD"; break;
    case rpg::FeedbackKind::Miss: text="MISSED"; color={.8f,.8f,.8f,1}; break;
    case rpg::FeedbackKind::Interrupt: text="INTERRUPTED"; color={.4f,.85f,1,1}; break;
    case rpg::FeedbackKind::Absorb: text="ABSORB "+std::to_string(feedback.amount);color={.73f,.85f,1,1};break;
    }
    renderer.DrawText(screen.x,screen.y,text,color,2);
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
bool Game::SaveProgress() {
  const auto content=simulation_.Save();
  const auto result=profile::Write(saveFile_,content,savedSnapshot_?&*savedSnapshot_:nullptr);
  switch(result) {
  case profile::WriteResult::Saved:savedSnapshot_=content;simulation_.Notify("Progress saved.");break;
  case profile::WriteResult::Conflict:simulation_.Notify("Save changed outside this game. F9 loads it; current progress was not written.");break;
  case profile::WriteResult::Busy:simulation_.Notify("Another game is saving this character. Try again.");break;
  case profile::WriteResult::Error:simulation_.Notify("Save failed. Previous save retained. Check project folder access.");break;
  }
  if(result==profile::WriteResult::Saved) {
    auto file=rosterFile_;file+=L".hotbars";
    const auto bars=interface_.SaveHotbars();
    if(!savedHotbars_ || *savedHotbars_!=bars) {
      if(profile::Write(file,bars,savedHotbars_?&*savedHotbars_:nullptr)==profile::WriteResult::Saved) savedHotbars_=bars;
      else simulation_.Notify("Progress saved; action-bar settings could not be saved (external change or file access).");
    }
  }
  return result==profile::WriteResult::Saved;
}

void Game::LoadProgress() {
  std::string content;
  if(profile::Read(saveFile_,content)!=profile::ReadResult::Ready) {simulation_.Notify("No valid save found.");return;}
  rpg::Simulation candidate;
  if(!candidate.Load(content) || candidate.Class()!=simulation_.Class() || !RestoreSnapshot(content)) {
    simulation_.Notify("Save rejected. Current progress retained.");return;
  }
  savedSnapshot_=std::move(content);
  LoadHotbarSettings();
}
