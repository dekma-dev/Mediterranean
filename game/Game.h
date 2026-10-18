#pragma once
#include "engine/Input.h"
#include "game/Simulation.h"
#include "game/Interface.h"
class Renderer;

class Game {
public:
  void Initialize();
  void Update(double deltaSeconds, const Input& input, float wheel);
  void Render(Renderer& renderer,bool showInterface = true);
  void SetCameraOrbit(float yaw,float pitch,float distance);
  bool RestoreSnapshot(const std::string& snapshot);
  std::wstring WindowTitle() const;
  bool TakeFullscreenRequest() { const bool value=fullscreenRequest_; fullscreenRequest_=false; return value; }
  bool QuitRequested() const { return quitRequested_; }
  void SetFullscreenState(bool enabled) { interface_.fullscreen=enabled; }
  void OpenPanel(ui::Panel panel) { interface_.SetOnly(panel); }
  void ShowPanel(ui::Panel panel) { interface_.Show(panel); }
  void CancelUiGesture() { interface_.CancelDrag(); orbiting_=false; }
  void ExecuteCommand(const rpg::Command& command,float seconds=.01f);
  const rpg::Simulation& State() const { return simulation_; }
private:
  rpg::Simulation simulation_;
  float yaw_ = 0, pitch_ = .45f, distance_ = 10, time_ = 0;
  float walkPhase_ = 0, walkBlend_ = 0;
  bool debugOverlay_ = false;
  bool hudVisible_ = true;
  bool orbiting_ = false;
  ui::Interface interface_;
  bool fullscreenRequest_ = false,quitRequested_ = false;
  void SaveProgress();
  void LoadProgress();
};
