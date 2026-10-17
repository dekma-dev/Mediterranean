#pragma once
#include "engine/Input.h"
#include "game/Simulation.h"
class Renderer;

class Game {
public:
  void Initialize();
  void Update(double deltaSeconds, const Input& input, float wheel);
  void Render(Renderer& renderer);
  std::wstring WindowTitle() const;
private:
  rpg::Simulation simulation_;
  float yaw_ = 0, pitch_ = .45f, distance_ = 10, time_ = 0;
  float walkPhase_ = 0, walkBlend_ = 0;
  bool paused_ = false;
  bool debugOverlay_ = false;
  enum class Panel { None, Character, Inventory, Quest, Map };
  Panel panel_ = Panel::None;
  void SaveProgress();
  void LoadProgress();
};
