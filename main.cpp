#include "game/Localization.h"
#include <Windows.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "engine/Engine.h"
#include "launcher/LaunchSupport.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  localization::Initialize(launch::Directory());
  // Process-local DPI awareness keeps fullscreen and UI pixels crisp. This does
  // not change the monitor resolution, scaling preference or any system setting.
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  int count=0;
  wchar_t** args=CommandLineToArgvW(GetCommandLineW(),&count);
  const bool smoke=args && count>=2 && std::wstring(args[1])==L"--smoke-test";
  const bool benchmark=args && count>=2 && std::wstring(args[1])==L"--benchmark";
  const bool fullHd=args && count>=2 && std::wstring(args[1])==L"--benchmark-suite-1080p";
  const bool suite=fullHd || (args && count>=2 && std::wstring(args[1])==L"--benchmark-suite");
  const std::wstring report=(benchmark || suite) && count==3?args[2]:L"";
  const std::wstring capture=smoke && (count==3 || count==4)?args[2]:L"";
  const std::wstring snapshotPath=smoke && count==4?args[3]:L"";
  std::vector<std::wstring> playArguments;
  if(args) for(int i=1;i<count;++i) playArguments.emplace_back(args[i]);
  if (args) LocalFree(args);
  if (smoke && capture.empty()) return 2;
  if ((benchmark || suite) && report.empty()) return 2;
  launch::Options options;std::wstring optionError;
  const bool diagnostic=smoke || benchmark || suite;
  if(!diagnostic && !launch::Parse(playArguments,options,optionError)) {
    MessageBoxW(nullptr,optionError.c_str(),L"Mediterranean launch options",MB_OK|MB_ICONERROR);return 2;
  }
  std::unique_ptr<launch::Instance> instanceLock;
  if(!diagnostic) {
    instanceLock=std::make_unique<launch::Instance>(launch::InstanceName(launch::Directory(),L"game"));
    if(!instanceLock->Acquired()) {MessageBoxW(nullptr,L"The game is already running from this folder.",L"Mediterranean",MB_OK|MB_ICONINFORMATION);return 3;}
  }
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
  const auto resolution=diagnostic?launch::resolutions[0]:launch::resolutions[options.resolution];
  if (!engine.Initialize(instance, resolution.width, resolution.height, !diagnostic,!diagnostic && options.fullscreen)) {
    if (smoke) { std::wofstream log(std::filesystem::path(capture+L".log")); log << engine.Error(); }
    else if (benchmark || suite) { std::wofstream log(std::filesystem::path(report+L".log")); log << engine.Error(); }
    else if (!benchmark) MessageBox(nullptr, engine.Error().c_str(), L"RPG Native", MB_OK | MB_ICONERROR);
    return -1;
  }
  if (benchmark || suite) {
    if (suite?engine.BenchmarkSuite(report,fullHd):engine.Benchmark(report)) return 0;
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
