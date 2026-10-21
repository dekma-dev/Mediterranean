#pragma once
#include <Windows.h>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace launch {
struct Resolution { int width,height; const wchar_t* label; };
inline constexpr std::array<Resolution,3> resolutions{{{1280,720,L"1280 x 720"},{1600,900,L"1600 x 900"},{1920,1080,L"1920 x 1080"}}};
struct Options { bool fullscreen=true,resume=false; unsigned resolution=0; };
bool Parse(const std::vector<std::wstring>& arguments,Options& options,std::wstring& error);
std::vector<std::wstring> Arguments(const Options& options);
std::wstring Quote(const std::wstring& argument);
std::filesystem::path Directory();
std::filesystem::path SavePath();
std::filesystem::path GuidePath();
bool ReadSnapshot(const std::filesystem::path& path,std::string& content);
bool LoadSettings(const std::filesystem::path& path,Options& options);
bool SaveSettings(const std::filesystem::path& path,const Options& options);
std::wstring InstanceName(const std::filesystem::path& directory,const wchar_t* role);
class Instance {
public:
  explicit Instance(const std::wstring& name);
  ~Instance();
  Instance(const Instance&)=delete;
  Instance& operator=(const Instance&)=delete;
  bool Acquired() const { return acquired_; }
private:
  HANDLE handle_=nullptr;
  bool acquired_=false;
};
bool IsRunning(const std::wstring& name);
class Process {
public:
  ~Process();
  Process()=default;
  Process(const Process&)=delete;
  Process& operator=(const Process&)=delete;
  bool Start(const std::filesystem::path& executable,const std::vector<std::wstring>& arguments,std::wstring& error,bool hidden=false);
  // Returns true exactly once when the owned child exits. Never terminates it.
  bool Poll(DWORD& exitCode);
  bool Active() const { return handle_!=nullptr; }
private:
  HANDLE handle_=nullptr;
};
}
