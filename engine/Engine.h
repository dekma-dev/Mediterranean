#pragma once

#include <cstdint>
#include <memory>

struct HINSTANCE__;
using HINSTANCE = HINSTANCE__*;

class Win32Window;
class Renderer;
class Game;

class Engine {
public:
  Engine();
  ~Engine();

  bool Initialize(HINSTANCE instance, int width, int height);
  int Run();

private:
  void Tick(double deltaSeconds);

  std::unique_ptr<Win32Window> window_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Game> game_;
  bool running_ = false;
};
