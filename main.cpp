#include <Windows.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "engine/Engine.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  // Process-local DPI awareness keeps fullscreen and UI pixels crisp. This does
  // not change the monitor resolution, scaling preference or any system setting.
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  int count=0;
  wchar_t** args=CommandLineToArgvW(GetCommandLineW(),&count);
  const bool smoke=args && count>=2 && std::wstring(args[1])==L"--smoke-test";
  const bool benchmark=args && count>=2 && std::wstring(args[1])==L"--benchmark";
  const std::wstring report=benchmark && count==3?args[2]:L"";
  const std::wstring capture=smoke && (count==3 || count==4)?args[2]:L"";
  const std::wstring snapshotPath=smoke && count==4?args[3]:L"";
  if (args) LocalFree(args);
  if (smoke && capture.empty()) return 2;
  if (benchmark && report.empty()) return 2;
  std::string snapshot;
  if (!snapshotPath.empty()) {
    std::error_code error;
    const auto length=std::filesystem::file_size(snapshotPath,error);
    std::ifstream input{std::filesystem::path(snapshotPath),std::ios::binary};
    if (error || length==0 || length>4096 || !input) {
      std::wofstream log(std::filesystem::path(capture+L".log")); log << L"Invalid smoke-test snapshot file."; return 2;
    }
    std::ostringstream data; data << input.rdbuf(); snapshot=data.str();
    if (!input) return 2;
  }
  Engine engine;
  if (!engine.Initialize(instance, 1280, 720, !smoke && !benchmark)) {
    if (smoke) { std::wofstream log(std::filesystem::path(capture+L".log")); log << engine.Error(); }
    else if (benchmark) { std::wofstream log(std::filesystem::path(report+L".log")); log << engine.Error(); }
    else if (!benchmark) MessageBox(nullptr, engine.Error().c_str(), L"RPG Native", MB_OK | MB_ICONERROR);
    return -1;
  }
  if (benchmark) {
    if (engine.Benchmark(report)) return 0;
    std::wofstream log(std::filesystem::path(report+L".log")); log << engine.Error(); return 1;
  }
  if (smoke) {
    const bool passed=engine.SmokeTest(capture,snapshot);
    std::wofstream log(std::filesystem::path(capture+L".log"));
    log << (passed?L"PASS: hidden fullscreen restoration, device, shaders, resize, world/UI rendering, PNG capture.":engine.Error());
    return passed?0:1;
  }
  return engine.Run();
}
