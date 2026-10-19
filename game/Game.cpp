#include "game/Game.h"

#include "engine/Renderer.h"
#include "game/WorldView.h"
#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "game/TerrainSurface.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace {
float EnemyStrike(const rpg::Enemy& enemy,bool active) {
  // Cooldowns are frozen while defeated or outside a sparring round. They must
  // not keep driving the presentation toward an unfinished strike indefinitely.
  if (!active || enemy.health<=0) return 0;
  return enemy.windup>0?1-enemy.windup/enemy.WindupDuration():
    std::max(0.f,1-(enemy.AttackInterval()-enemy.attackCooldown)/.2f);
}
}

void Game::Initialize() {
  simulation_=rpg::Simulation{}; time_=0; atmosphereTime_=0;
  yaw_=0; pitch_=.45f; distance_=10;
  ResetMotion();
}

bool Game::RestoreSnapshot(const std::string& snapshot) {
  if (!simulation_.Load(snapshot)) return false;
  CancelUiGesture();
  interface_.SetOnly(ui::Panel::None);
  interface_.previewYaw=0;
  yaw_=0; pitch_=.45f; distance_=10; time_=0; atmosphereTime_=0; ResetMotion();
  return true;
}

void Game::Update(double seconds,const Input& input,float wheel) {
  const float dt=std::isfinite(seconds)?static_cast<float>(std::clamp(seconds,0.0,.05)):0.f;
  interface_.noticeTime=std::max(0.f,interface_.noticeTime-dt);
  // Engine changes the window after this update. Cancel before dispatching a
  // simultaneous mouse release, or that release would commit a stale drop.
  if (input.Pressed(VK_F11)) { CancelUiGesture(); interface_.InvalidateHitRegions(); }
  if (input.Pressed(VK_F2)) { hudVisible_=!hudVisible_; interface_.InvalidateHitRegions(); }
  if (input.Pressed(VK_F3)) debugOverlay_=!debugOverlay_;
  if (input.Pressed(VK_F5)) SaveProgress();
  if (input.Pressed(VK_F9)) LoadProgress();
  if (input.Pressed(VK_ESCAPE)) interface_.Escape();
  if (input.Pressed('C')) interface_.Toggle(ui::Panel::Character);
  if (input.Pressed('I') || input.Pressed('B')) interface_.Toggle(ui::Panel::Inventory);
  if (input.Pressed('J')) interface_.Toggle(ui::Panel::Journal);
  if (input.Pressed('M')) interface_.Toggle(ui::Panel::Map);
  if (input.Pressed('N')) interface_.Toggle(ui::Panel::Build);
  const bool closingMerchant=interface_.panel==ui::Panel::Merchant && input.Pressed('E');
  if (closingMerchant) interface_.Close(ui::Panel::Merchant);
  ui::Event event;
  const bool uiVisible=hudVisible_ || interface_.HasPanels() || simulation_.Health()<=0;
  if (uiVisible)
    event=interface_.Pointer(input.ClientX(),input.ClientY(),input.Width(),input.Height(),
      input.Pressed(VK_LBUTTON),input.Pressed(VK_RBUTTON),input.Held(VK_LBUTTON),input.Released(VK_LBUTTON));
  rpg::Command command;
  switch (event.action) {
  case ui::Action::MusicVolume: interface_.musicVolume=std::clamp(interface_.musicVolume+event.value,0,100); break;
  case ui::Action::EffectsVolume: interface_.effectsVolume=std::clamp(interface_.effectsVolume+event.value,0,100); break;
  case ui::Action::Close: interface_.Close(static_cast<ui::Panel>(event.value)); break;
  case ui::Action::Menu: interface_.Escape(); break;
  case ui::Action::Character: interface_.Toggle(ui::Panel::Character); break;
  case ui::Action::Inventory: interface_.Toggle(ui::Panel::Inventory); break;
  case ui::Action::Map: interface_.Toggle(ui::Panel::Map); break;
  case ui::Action::Journal: interface_.Toggle(ui::Panel::Journal); break;
  case ui::Action::Build: interface_.Toggle(ui::Panel::Build); break;
  case ui::Action::SelectTalent: interface_.selectedTalent=event.value; break;
  case ui::Action::LearnTalent: command.learnTalent=event.value; break;
  case ui::Action::Save: SaveProgress(); interface_.menuNotice=simulation_.Message(); break;
  case ui::Action::Load: LoadProgress(); interface_.menuNotice=simulation_.Message(); break;
  case ui::Action::Fullscreen: fullscreenRequest_=true; break;
  case ui::Action::Quit: quitRequested_=true; break;
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
    command.nextTarget=input.Pressed(VK_TAB); command.interact=input.Pressed('E') && !closingMerchant;
    command.restart=input.Pressed('R');
    command.usePotion=command.usePotion || input.Pressed('Q'); command.equipWeapon=command.equipWeapon || input.Pressed('G');
    for (int i=0;i<4;++i) if (input.Pressed('1'+i)) { command.ability=i; break; }
  }
  // Recovery stays reachable even if the player opens a central panel after death.
  command.respawn=simulation_.Health()<=0 && input.Pressed(VK_RETURN);
  if (command.restart || command.respawn) {
    CancelUiGesture();
    interface_.SetOnly(ui::Panel::None); yaw_=0; pitch_=.45f; distance_=10;
    interface_.previewYaw=0;
    if (command.restart) interface_.hasWaypoint=false;
  }
  const bool wasAlive=simulation_.Health()>0;
  // Menus capture local commands, but never stop simulation time.
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
  simulation_.Update(dt,command);
  if (dt<=0) return;
  time_+=dt;
  atmosphereTime_=std::fmod(atmosphereTime_+dt,1800.0);
  if (command.restart || (wasDead && simulation_.Health()>0)) { ResetMotion(); return; }
  effects_.Advance(dt);
  for (std::size_t i=0;i<simulation_.VisualEventCount();++i) effects_.Emit(simulation_.VisualEvents()[i]);
  const auto dashPosition=simulation_.Player();
  effects_.TrackDash({dashPosition.x,simulation_.Height(),dashPosition.z},simulation_.Dashing(),dt,
    simulation_.Dashing()?motion::DashSpeed(simulation_.DashPhase()):0);
  if (command.haltMovement || simulation_.Health()<=0) effects_.CancelDash();
  if (simulation_.Health()<=0) effects_.CancelTrails();
  if (wasSparring && !simulation_.Sparring()) effects_.CancelTrail(simulation_.SparringOpponent().id);
  playerPose_.Update(simulation_.Facing(),simulation_.GroundMovementDistance(),simulation_.Grounded() && !simulation_.Dashing(),
    simulation_.Health()>0,simulation_.AttackFlash()*5,dt,simulation_.Dashing() || simulation_.DashedThisUpdate());
  const auto velocity=simulation_.MovementVelocity();
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
    simulation_.Health()>0,simulation_.DamagedThisUpdate(),playerPose_.attack,
    simulation_.LandingSpeed(),simulation_.LandingElapsed(),forwardAcceleration,castStarted);
  playerAnimation_.UpdateDash(dt,simulation_.Dashing(),simulation_.DashPhase(),dashImpact,simulation_.Health()>0,dashFinished,dashEndElapsed);
  if (command.haltMovement) playerAnimation_.CancelDash();
  playerAnimation_.pose.lean*=1-playerAnimation_.pose.dash;
  playerFeet_.Update(dt,simulation_.Player(),playerPose_.facing,simulation_.Grounded() && !simulation_.Dashing(),
    simulation_.Health()>0,playerAnimation_.pose);
  const auto& enemies=simulation_.Enemies();
  if (enemyPoses_.size()!=enemies.size()+1) ResetMotion();
  for (std::size_t i=0;i<enemyPoses_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:simulation_.SparringOpponent();
    if (i==enemies.size() && !wasSparring && simulation_.Sparring()) {
      enemyPoses_[i].Reset(enemy.facing); enemyAnimations_[i].Reset(float(enemy.id)*.37f);
      enemyFeet_[i].Reset(enemy.position,enemy.facing);
    }
    const float strike=EnemyStrike(enemy,simulation_.Health()>0 && (i<enemies.size() || simulation_.Sparring()));
    // Enemy heading is already smoothed by simulation, then frozen for attacks.
    // Use it exactly so the character always faces the displayed damage cone.
    enemyPoses_[i].Update(enemy.facing,enemy.movementDistance,true,enemy.health>0,strike,dt,true);
    enemyAnimations_[i].Update(dt,0,0,enemy.movementDistance,enemy.health>0,enemy.damagedThisUpdate,strike);
    enemyFeet_[i].Update(dt,enemy.position,enemy.facing,true,enemy.health>0,enemyAnimations_[i].pose);
  }
  const auto track=[&](rpg::EntityId id,rpg::Vec2 position,float height,const motion::Actor& motion,
    const animation::Sample& animation,bool shield,bool armed) {
    if (!armed) { effects_.CancelTrail(id); return; }
    if (!effects_.Tracking(id)) return;
    const auto pose=BuildCharacterPose(motion.attack,motion.gait,shield,animation);
    const auto point=[&](float distance) -> DirectX::XMFLOAT3 {
      const auto p=pose.WeaponPoint(distance);
      return {position.x+std::cos(motion.facing)*p.x+std::sin(motion.facing)*p.z,height+p.y,
        position.z-std::sin(motion.facing)*p.x+std::cos(motion.facing)*p.z};
    };
    effects_.TrackWeapon(id,point(.22f),point(.96f));
  };
  track(0,simulation_.Player(),simulation_.Height(),playerPose_,playerAnimation_.pose,
    simulation_.Items().Worn(rpg::GearSlot::Offhand)!=rpg::Item::None,
    simulation_.Health()>0 && simulation_.Items().Worn(rpg::GearSlot::Weapon)!=rpg::Item::None);
  for (std::size_t i=0;i<enemyPoses_.size();++i) {
    const auto& enemy=i<enemies.size()?enemies[i]:simulation_.SparringOpponent();
    track(enemy.id,enemy.position,0,enemyPoses_[i],enemyAnimations_[i].pose,false,
      simulation_.Health()>0 && enemy.health>0 && (i<enemies.size() || simulation_.Sparring()));
  }
  UpdateCamera(dt);
}
void Game::SetCameraOrbit(float yaw,float pitch,float distance) {
  if (!std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(distance)) return;
  yaw_=std::remainder(yaw,6.2831853f);
  pitch_=std::clamp(pitch,-.75f,1.1f); distance_=std::clamp(distance,3.f,18.f);
  // Explicit positioning is used by captures/tools; interactive input sets targets.
  UpdateCamera(0,true);
}
void Game::ResetMotion() {
  ++audioGeneration_;
  effects_.Reset();
  playerPose_.Reset(simulation_.Facing());
  playerAnimation_.Reset();
  playerFeet_.Reset(simulation_.Player(),simulation_.Facing());
  enemyPoses_.resize(simulation_.Enemies().size()+1);
  enemyAnimations_.resize(enemyPoses_.size());
  enemyFeet_.resize(enemyPoses_.size());
  for (std::size_t i=0;i<enemyPoses_.size();++i) {
    const auto& enemy=i<simulation_.Enemies().size()?simulation_.Enemies()[i]:simulation_.SparringOpponent();
    enemyPoses_[i].Reset(enemy.facing);
    enemyAnimations_[i].Reset(float(enemy.id)*.37f);
    enemyFeet_[i].Reset(enemy.position,enemy.facing);
    enemyPoses_[i].attack=EnemyStrike(enemy,simulation_.Health()>0 &&
      (i<simulation_.Enemies().size() || simulation_.Sparring()));
  }
  UpdateCamera(0,true);
}
void Game::UpdateCamera(float dt,bool immediate) {
  // Keep the horizon stable: the actor jumps within the frame instead of
  // dragging the entire camera through the full ballistic arc.
  const float height=simulation_.Height()*.35f+1.4f;
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
  renderer.SetAtmosphereTime(atmosphereTime_);
  if (!renderer.HasTerrainSurface() && !renderer.SetTerrainSurface(BuildTerrainSurface(simulation_.Map()),terrainSurfaceSize)) return;
  using DirectX::XMFLOAT3;
  using DirectX::XMFLOAT4;
  const auto player=simulation_.Player();
  const float y=simulation_.Height();
  const rpg::Vec2 direction{-std::sin(cameraYaw_)*std::cos(cameraPitch_),-std::cos(cameraYaw_)*std::cos(cameraPitch_)};
  const float cameraDistance=cameraBoom_.distance;
  renderer.SetCamera({player.x+direction.x*cameraDistance,cameraHeight_+std::sin(cameraPitch_)*cameraDistance,
    player.z+direction.z*cameraDistance},{player.x,cameraHeight_,player.z});
  DrawWorld(renderer,simulation_.Map(),time_,simulation_.Sparring());
  using M=visual::Mesh;
  using S=visual::Material;
  DrawCharacter(renderer,player,y,playerPose_.facing,simulation_.HurtFlash()>0?XMFLOAT4{1,.48f,.35f,1}:
    XMFLOAT4{.22f,.37f,.54f,1},playerPose_.attack,playerPose_.gait,CharacterOutfit::From(simulation_.Items()),playerAnimation_.pose);
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
    if (enemy.health<=0) {
      if (!enemy.looted) {
        renderer.DrawMesh(M::Rock,{enemy.position.x,.22f,enemy.position.z},{.55f,.44f,.45f},{.51f,.34f,.15f,1},S::Cloth);
        renderer.DrawMesh(M::Crystal,{enemy.position.x,1.f+std::sin(time_*3)*.1f,enemy.position.z},{.18f,.3f,.18f},{1,.82f,.3f,1},S::Glow,0,0,visual::ShadowMode::None);
      }
      continue;
    }
    const bool captain=enemy.kind==rpg::EnemyKind::Captain;
    CharacterOutfit enemyOutfit; enemyOutfit.armor=captain; enemyOutfit.helm=captain;
    const auto& pose=enemyPoses_[i];
    DrawCharacter(renderer,enemy.position,0,pose.facing,
      enemy.hitFlash>0?XMFLOAT4{1,.7f,.55f,1}:enemy.kind==rpg::EnemyKind::Trainer?XMFLOAT4{.55f,.46f,.22f,1}:captain?XMFLOAT4{.36f,.19f,.48f,1}:XMFLOAT4{.65f,.2f,.25f,1},
      pose.attack,pose.gait,enemyOutfit,enemyAnimations_[i].pose);
    if (enemy.windup>0 && simulation_.Health()>0) {
      const float progress=1-enemy.windup/enemy.WindupDuration();
      const XMFLOAT4 warning{1,.55f*(1-progress),.12f,1};
      for (int segment=0;segment<=12;++segment) {
        const float angle=enemy.facing-rpg::Enemy::attackHalfAngle+segment*rpg::Enemy::attackHalfAngle/6;
        renderer.DrawMarkerBox({enemy.position.x+std::sin(angle)*rpg::Enemy::attackReach,.075f,
          enemy.position.z+std::cos(angle)*rpg::Enemy::attackReach},{.42f,.05f,.09f},warning,angle);
      }
      for (float side : {-1.f,1.f}) {
        const float angle=enemy.facing+side*rpg::Enemy::attackHalfAngle;
        renderer.DrawMarkerBox({enemy.position.x+std::sin(angle)*rpg::Enemy::attackReach*.5f,.075f,
          enemy.position.z+std::cos(angle)*rpg::Enemy::attackReach*.5f},
          {.06f,.05f,rpg::Enemy::attackReach},warning,angle);
      }
    }
    if (i==simulation_.Target()) {
      renderer.DrawMarkerBox({enemy.position.x,.04f,enemy.position.z},{1.3f,.06f,1.3f},{1,.72f,.2f,1});
      renderer.DrawMarkerBox({enemy.position.x,2.5f,enemy.position.z},{enemy.health/enemy.MaxHealth(), .1f,.1f},{.8f,.15f,.2f,1});
    }
  }
  effects_.Render(renderer);
  const bool drawInterface=showInterface && (hudVisible_ || interface_.HasPanels() || simulation_.Health()<=0);
  for (std::size_t i=0;drawInterface && i<simulation_.Feedback().size();++i) {
    const auto& feedback=simulation_.Feedback()[i];
    DirectX::XMFLOAT2 screen;
    if (!renderer.Project({feedback.position.x,2.5f+feedback.age*.8f+float(i%3)*.25f,feedback.position.z},screen)) continue;
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
    }
    renderer.DrawText(screen.x,screen.y,text,color,2);
  }
  if (drawInterface) interface_.Render(renderer,simulation_,time_,debugOverlay_);
}

std::wstring Game::WindowTitle() const {
  return L"Mediterranean - Astral Frontier | Native prototype";
}

void Game::SaveProgress() {
  try {
    const std::filesystem::path folder=RPG_SAVE_DIRECTORY;
    std::filesystem::create_directories(folder);
    const auto temporary=folder / "patrol.tmp";
    const auto destination=folder / "patrol.save";
    std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
    output << simulation_.Save();
    output.close();
    if (!output || !MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
      simulation_.Notify("Save failed. Previous save retained."); return;
    }
    simulation_.Notify("Progress saved.");
  } catch (const std::exception&) { simulation_.Notify("Save failed. Check project folder access."); }
}

void Game::LoadProgress() {
  try {
    const auto file=std::filesystem::path(RPG_SAVE_DIRECTORY) / "patrol.save";
    std::error_code error;
    const auto length=std::filesystem::file_size(file,error);
    if (error || length>4096) { simulation_.Notify("No valid save found."); return; }
    std::ifstream input(file,std::ios::binary);
    std::ostringstream content; content << input.rdbuf();
    if (!input || !RestoreSnapshot(content.str())) { simulation_.Notify("Save rejected. Current progress retained."); return; }
  } catch (const std::exception&) { simulation_.Notify("Load failed. Current progress retained."); }
}
