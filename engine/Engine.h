#pragma once

#include <cstdint>
#include <memory>
#include "engine/Input.h"
#include "engine/GraphicsRecovery.h"
#include <string>

class Win32Window;
class Renderer;
class Game;
class Audio;

class Engine {
public:
  Engine();
  ~Engine();

  bool Initialize(HINSTANCE instance, int width, int height, bool visible = true, bool fullscreen = true, const std::string& snapshot = {});
  bool StartupTest(bool fullscreen,int width,int height,const std::string& snapshot,const std::wstring& capturePath);
  bool SmokeTest(const std::wstring& capturePath,const std::string& snapshot = {});
  bool Benchmark(const std::wstring& reportPath);
  bool BenchmarkSuite(const std::wstring& reportPath,bool fullHd=false);
  int Run();
  const std::wstring& Error() const { return error_; }

private:
  friend struct GraphicsLifecycleTestAccess;
  void Tick(double deltaSeconds);
  bool TryRecoverGraphics();
  void HandleGraphicsFailure();
  graphics::RecoveryBudget graphicsRecovery_;
  bool resetFrameClock_=false;
  unsigned graphicsRecoveries_=0;

  std::unique_ptr<Win32Window> window_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Game> game_;
  std::unique_ptr<Audio> audio_;
  bool running_ = false;
  Input input_;
  int exitCode_ = 0;
  std::wstring error_;
};
