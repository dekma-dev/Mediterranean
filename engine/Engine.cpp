#include "engine/Engine.h"

#include <Windows.h>

#include "engine/Renderer.h"
#include "game/Game.h"
#include "platform/Win32Window.h"

Engine::Engine() = default;

Engine::~Engine() = default;

bool Engine::Initialize(HINSTANCE instance, int width, int height) {
  window_ = std::make_unique<Win32Window>();
  if (!window_->Create(instance, width, height, L"RPG Native Prototype")) {
    return false;
  }

  renderer_ = std::make_unique<Renderer>();
  if (!renderer_->Initialize(window_->GetHandle(), width, height)) {
    return false;
  }

  game_ = std::make_unique<Game>();
  game_->Initialize();

  running_ = true;
  return true;
}

int Engine::Run() {
  LARGE_INTEGER frequency = {};
  LARGE_INTEGER last = {};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&last);

  MSG msg = {};
  while (running_) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running_ = false;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    const double deltaSeconds = static_cast<double>(now.QuadPart - last.QuadPart)
      / static_cast<double>(frequency.QuadPart);
    last = now;

    Tick(deltaSeconds);
  }

  return static_cast<int>(msg.wParam);
}

void Engine::Tick(double deltaSeconds) {
  if (!renderer_) return;

  game_->Update(deltaSeconds);

  renderer_->BeginFrame();
  game_->Render(*renderer_);
  renderer_->EndFrame();
}