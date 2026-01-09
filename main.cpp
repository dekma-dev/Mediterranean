#include <Windows.h>

#include "engine/Engine.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  Engine engine;
  if (!engine.Initialize(instance, 1280, 720)) {
    MessageBox(nullptr, L"Failed to initialize engine.", L"RPG Native", MB_OK | MB_ICONERROR);
    return -1;
  }
  return engine.Run();
}