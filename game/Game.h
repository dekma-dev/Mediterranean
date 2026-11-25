#pragma once
#include "engine/Input.h"
#include <optional>
#include "game/Simulation.h"
#include "game/CommandExecution.h"
#include "game/Interface.h"
#include "game/Motion.h"
#include "game/Animation.h"
#include "game/CombatEffects.h"
#include "game/SustainedEffects.h"
#include "game/FootPlacement.h"
#include "game/WorldView.h"
#include "game/CharacterSelection.h"
#include "game/ActorPresentation.h"
#include "game/SceneSnapshot.h"
#include "game/CommandSchedule.h"
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
  bool WantsMouseLook() const { return orbitButtons_!=0; }
  unsigned MouseLookButtons() const { return orbitButtons_; }
  void SetFullscreenState(bool enabled) { interface_.fullscreen=enabled; }
  void OpenPanel(ui::Panel panel) { interface_.SetOnly(panel); }
  void ShowPanel(ui::Panel panel) { interface_.Show(panel); }
  void CancelUiGesture() {
    selection_.SuspendPreview();
    interface_.CancelDrag(); interface_.CancelBuildConfirmation(); interface_.InvalidateHitRegions(); CancelOrbit();
  }
  void SuspendInput() { execution_.InvalidateTrace(); soundDelivery_.Reset();soundEvents_.Reset();CancelUiGesture(); creatureSteps_.Reset(); simulation_.StopMovement(); effects_.CancelDash(); playerAnimation_.CancelDash();
    effects_.CancelTrail(0); playerAnimation_.pose.attackSeconds=-1; playerPose_.attack=playerPose_.attackVelocity=0;
    playerAnimation_.authoredWeapon={};playerAnimation_.pose.authoredStrikeWeight=0;
    simulation_.CloseQuestDialogue(); if (interface_.panel==ui::Panel::Quest) interface_.Close(ui::Panel::Quest);
    scene_.Capture(simulation_,scene_.Sequence()); }
  rpg::StepReceipt ExecuteCommand(const rpg::Command& command,float seconds=.01f);
  struct FrameResult {
    bool tradeRequested=false,boundary=false;
    unsigned steps=0,events=0,droppedEvents=0;
    std::uint64_t eventKinds=0;
    std::array<float,64> firstEventOffsets{};
    std::string notice;
    bool Has(rpg::CombatVisualKind kind) const {
      const auto index=static_cast<unsigned>(kind);return index<64 && (eventKinds&(std::uint64_t(1)<<index))!=0;
    }
    float FirstOffset(rpg::CombatVisualKind kind) const {return Has(kind)?firstEventOffsets[static_cast<unsigned>(kind)]:-1.f;}
  };
  FrameResult ExecuteFrame(const rpg::Command& command,float seconds);
  const rpg::CommandSchedule::Statistics& ScheduleStats() const {return schedule_.Stats();}
  sound::Events TakeSoundEvents(){return soundDelivery_.Take();}
  const sound::DeliveryQueue::Statistics& SoundDeliveryStats()const{return soundDelivery_.Stats();}
  const rpg::ExecutionStats& StepStats()const{return execution_.Stats();}
  void BeginCommandTrace(std::size_t capacity=90){execution_.BeginTrace(simulation_,capacity);}
  rpg::ReplayResult VerifyCommandTrace()const{return execution_.VerifyTrace();}
  std::string StepDiagnostics()const;
  const presentation::SceneSnapshot& Scene() const {return scene_;}
  const rpg::Simulation& State() const { return simulation_; }
  const motion::Actor& PlayerPose() const { return playerPose_; }
  const motion::Actor& EnemyPose(std::size_t index) const { return enemyActors_.at(index).motion; }
  const animation::Sample& PlayerAnimation() const { return playerAnimation_.pose; }
  const animation::Sample& EnemyAnimation(std::size_t index) const { return enemyActors_.at(index).animation.pose; }
  const animation::Sample& DummyAnimation(std::size_t index) const { return dummyAnimations_.at(index).pose; }
  float CameraRange() const { return cameraBoom_.distance; }
  float CameraYaw() const { return cameraYaw_; }
  float CameraPitch() const { return cameraPitch_; }
  float CameraHeight() const { return cameraHeight_; }
  const CombatEffects& Effects() const { return effects_; }
  const SustainedEffects& Sustained() const { return sustained_; }
  const presentation::CreatureContacts& CreatureSteps() const { return creatureSteps_; }
  std::size_t StaticSceneInstances() const { return worldView_.StaticInstances(); }
  std::size_t StaticSceneBytes() const { return worldView_.StaticBytes(); }
  unsigned AudioGeneration() const { return audioGeneration_; }
  float MusicVolume() const { return interface_.musicVolume/100.f; }
  float EffectsVolume() const { return interface_.effectsVolume/100.f; }
  void SetAudioAvailable(bool value) { interface_.audioAvailable=value; }
private:
  std::filesystem::path saveFile_,rosterFile_;
  std::optional<std::string> savedSnapshot_,savedHotbars_;
  void LoadHotbarSettings();
  bool SaveHotbarSettings();
  ui::CharacterSelection selection_;
  bool choosingCharacter_=false,inputBarrier_=false;
  rpg::Simulation simulation_;
  rpg::CommandExecution execution_;
  rpg::CommandSchedule schedule_;
  presentation::SceneSnapshot scene_;
  sound::Events soundEvents_;
  sound::DeliveryQueue soundDelivery_;
  void PresentStep(const rpg::StepReceipt& step);
  void Notify(const std::string& message){execution_.InvalidateTrace();simulation_.Notify(message);}
  WorldView worldView_;
  unsigned audioGeneration_=0;
  float yaw_ = 0, pitch_ = .45f, distance_ = 10, time_ = 0;
  double atmosphereTime_=0;
  motion::Actor playerPose_;
  std::vector<presentation::Actor> enemyActors_;
  animation::State playerAnimation_;
  std::array<animation::State,3> dummyAnimations_{};
  facial::State playerFace_,keeperFace_,merchantFace_;
  animation::FootPlacement playerFeet_;
  CombatEffects effects_;
  SustainedEffects sustained_;
  presentation::CreatureContacts creatureSteps_;
  motion::CameraBoom cameraBoom_;
  float cameraYaw_=0,cameraPitch_=.45f,cameraHeight_=1.4f;
  bool debugOverlay_ = false;
  bool hudVisible_ = true;
  unsigned orbitButtons_=0;
  bool freeLook_=false,leftOrbitDragged_=false;
  float movementYaw_=0,leftOrbitTravel_=0;
  rpg::EntityId pendingWorldTarget_=0;
  ui::Interface interface_;
  bool fullscreenRequest_ = false,quitRequested_ = false;
  bool SaveProgress();
  void LoadProgress();
  void ResetMotion();
  void UpdateFaces(float dt);
  void UpdateCamera(float dt,bool immediate=false);
  void CancelOrbit() { orbitButtons_=0;pendingWorldTarget_=0;leftOrbitTravel_=0;leftOrbitDragged_=false; yaw_=cameraYaw_; pitch_=cameraPitch_; }
};
