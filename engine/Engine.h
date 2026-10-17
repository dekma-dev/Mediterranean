#pragma once

#include <cstdint>
#include <memory>
#include "engine/Input.h"
#include <string>

class Win32Window;
class Renderer;
class Game;

class Engine {
public:
  Engine();
  ~Engine();

  bool Initialize(HINSTANCE instance, int width, int height, bool visible = true);
  bool SmokeTest(const std::wstring& capturePath);
  int Run();
  const std::wstring& Error() const { return error_; }

private:
  void Tick(double deltaSeconds);

  std::unique_ptr<Win32Window> window_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Game> game_;
  bool running_ = false;
  Input input_;
  int exitCode_ = 0;
  std::wstring error_;
};
