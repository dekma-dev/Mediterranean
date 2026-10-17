#include "engine/Engine.h"

#include <Windows.h>
#include <algorithm>

#include "engine/Renderer.h"
#include "game/Game.h"
#include "platform/Win32Window.h"

Engine::Engine() = default;

Engine::~Engine() = default;

bool Engine::Initialize(HINSTANCE instance, int width, int height, bool visible) {
  window_ = std::make_unique<Win32Window>();
  if (!window_->Create(instance, width, height, L"RPG Native Prototype", visible)) {
    error_ = L"Could not create the application window.";
    return false;
  }

  renderer_ = std::make_unique<Renderer>();
  if (!renderer_->Initialize(window_->GetHandle(), width, height)) {
    error_ = renderer_->Error();
    return false;
  }

  game_ = std::make_unique<Game>();
  game_->Initialize();
  SetWindowTextW(window_->GetHandle(), game_->WindowTitle().c_str());

  running_ = true;
  return true;
}

bool Engine::SmokeTest(const std::wstring& capturePath) {
  // Exercise real device/shader creation, resized targets, rendering and GPU readback.
  // This does not synthesize input or claim to replace interactive playtesting.
  for (const auto dimensions : {std::pair<int,int>{960,600},{1600,900},{1280,720}}) {
    if (!renderer_->Resize(dimensions.first,dimensions.second)) { error_=renderer_->Error(); return false; }
    game_->Update(1.0/60.0,input_,0);
    renderer_->BeginFrame(); game_->Render(*renderer_);
    if (dimensions.first==1280 && !renderer_->Capture(capturePath)) { error_=renderer_->Error(); return false; }
    if (!renderer_->EndFrame()) { error_=renderer_->Error(); return false; }
  }
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

    if (!running_) break;

    input_.Update(window_->GetHandle());
    if (IsIconic(window_->GetHandle()) || GetForegroundWindow() != window_->GetHandle()) {
      window_->ConsumeWheel();
      WaitMessage();
      QueryPerformanceCounter(&last);
      continue;
    }

    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    const double deltaSeconds = static_cast<double>(now.QuadPart - last.QuadPart)
      / static_cast<double>(frequency.QuadPart);
    last = now;

    Tick(std::min(deltaSeconds, 0.05));
  }

  return exitCode_;
}

void Engine::Tick(double deltaSeconds) {
  if (!renderer_) return;

  RECT client{};
  GetClientRect(window_->GetHandle(), &client);
  if (!renderer_->Resize(client.right, client.bottom)) {
    MessageBoxW(window_->GetHandle(), renderer_->Error().c_str(), L"Renderer error", MB_OK | MB_ICONERROR);
    exitCode_ = 1;
    running_ = false;
    return;
  }
  game_->Update(deltaSeconds, input_, window_->ConsumeWheel());

  renderer_->BeginFrame();
  game_->Render(*renderer_);
  if (!renderer_->EndFrame()) {
    MessageBoxW(window_->GetHandle(), renderer_->Error().c_str(), L"Renderer error", MB_OK | MB_ICONERROR);
    exitCode_ = 1;
    running_ = false;
  }
}
