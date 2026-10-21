#pragma once
#include "engine/Input.h"
#include "game/Simulation.h"
#include "game/Interface.h"
#include "game/Motion.h"
#include "game/Animation.h"
#include "game/CombatEffects.h"
#include "game/FootPlacement.h"
#include "game/WorldView.h"
#include "game/CharacterSelection.h"
class Renderer;

class Game {
public:
  void Initialize();
  void ChooseCharacter(const std::filesystem::path& save);
  bool ChoosingCharacter() const { return choosingCharacter_; }
  const ui::CharacterSelection& Selection() const { return selection_; }
  void Update(double deltaSeconds, const Input& input, float wheel);
  void Render(Renderer& renderer,bool showInterface = true);
  void SetCameraOrbit(float yaw,float pitch,float distance);
  bool RestoreSnapshot(const std::string& snapshot);
  std::wstring WindowTitle() const;
  bool TakeFullscreenRequest() { const bool value=fullscreenRequest_; fullscreenRequest_=false; return value; }
  bool QuitRequested() const { return quitRequested_; }
  bool WantsMouseLook() const { return orbiting_; }
  void SetFullscreenState(bool enabled) { interface_.fullscreen=enabled; }
  void OpenPanel(ui::Panel panel) { interface_.SetOnly(panel); }
  void ShowPanel(ui::Panel panel) { interface_.Show(panel); }
  void CancelUiGesture() {
    selection_.CancelGesture();
    interface_.CancelDrag(); interface_.CancelBuildConfirmation(); interface_.InvalidateHitRegions(); CancelOrbit();
  }
  void SuspendInput() { CancelUiGesture(); simulation_.StopMovement(); effects_.CancelDash(); playerAnimation_.CancelDash();
    effects_.CancelTrail(0); playerAnimation_.pose.attackSeconds=-1; playerPose_.attack=playerPose_.attackVelocity=0;
    simulation_.CloseQuestDialogue(); if (interface_.panel==ui::Panel::Quest) interface_.Close(ui::Panel::Quest); }
  void ExecuteCommand(const rpg::Command& command,float seconds=.01f);
  const rpg::Simulation& State() const { return simulation_; }
  const motion::Actor& PlayerPose() const { return playerPose_; }
  const motion::Actor& EnemyPose(std::size_t index) const { return enemyPoses_.at(index); }
  const animation::Sample& PlayerAnimation() const { return playerAnimation_.pose; }
  const animation::Sample& EnemyAnimation(std::size_t index) const { return enemyAnimations_.at(index).pose; }
  float CameraRange() const { return cameraBoom_.distance; }
  float CameraYaw() const { return cameraYaw_; }
  float CameraPitch() const { return cameraPitch_; }
  float CameraHeight() const { return cameraHeight_; }
  const CombatEffects& Effects() const { return effects_; }
  std::size_t StaticSceneInstances() const { return worldView_.StaticInstances(); }
  std::size_t StaticSceneBytes() const { return worldView_.StaticBytes(); }
  unsigned AudioGeneration() const { return audioGeneration_; }
  float MusicVolume() const { return interface_.musicVolume/100.f; }
  float EffectsVolume() const { return interface_.effectsVolume/100.f; }
  void SetAudioAvailable(bool value) { interface_.audioAvailable=value; }
private:
  std::filesystem::path saveFile_;
  ui::CharacterSelection selection_;
  bool choosingCharacter_=false,inputBarrier_=false;
  rpg::Simulation simulation_;
  WorldView worldView_;
  unsigned audioGeneration_=0;
  float yaw_ = 0, pitch_ = .45f, distance_ = 10, time_ = 0;
  double atmosphereTime_=0;
  motion::Actor playerPose_;
  std::vector<motion::Actor> enemyPoses_;
  animation::State playerAnimation_;
  std::vector<animation::State> enemyAnimations_;
  facial::State playerFace_,keeperFace_,merchantFace_;
  std::vector<facial::State> enemyFaces_;
  animation::FootPlacement playerFeet_;
  std::vector<animation::FootPlacement> enemyFeet_;
  CombatEffects effects_;
  motion::CameraBoom cameraBoom_;
  float cameraYaw_=0,cameraPitch_=.45f,cameraHeight_=1.4f;
  bool debugOverlay_ = false;
  bool hudVisible_ = true;
  bool orbiting_ = false;
  ui::Interface interface_;
  bool fullscreenRequest_ = false,quitRequested_ = false;
  void SaveProgress();
  void LoadProgress();
  void ResetMotion();
  void UpdateFaces(float dt);
  void UpdateCamera(float dt,bool immediate=false);
  void CancelOrbit() { orbiting_=false; yaw_=cameraYaw_; pitch_=cameraPitch_; }
};
