#include <Windows.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>

#include "engine/Engine.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  int count=0;
  wchar_t** args=CommandLineToArgvW(GetCommandLineW(),&count);
  const bool smoke=args && count>=2 && std::wstring(args[1])==L"--smoke-test";
  const std::wstring capture=smoke && count==3?args[2]:L"";
  if (args) LocalFree(args);
  if (smoke && capture.empty()) return 2;
  Engine engine;
  if (!engine.Initialize(instance, 1280, 720, !smoke)) {
    if (smoke) { std::wofstream log(std::filesystem::path(capture+L".log")); log << engine.Error(); }
    else MessageBox(nullptr, engine.Error().c_str(), L"RPG Native", MB_OK | MB_ICONERROR);
    return -1;
  }
  if (smoke) {
    const bool passed=engine.SmokeTest(capture);
    std::wofstream log(std::filesystem::path(capture+L".log"));
    log << (passed?L"PASS: device, shaders, resize, frame rendering, PNG capture.":engine.Error());
    return passed?0:1;
  }
  return engine.Run();
}
