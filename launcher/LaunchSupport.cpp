#include "launcher/LaunchSupport.h"
#include <cwctype>
#include <fstream>
#include <sstream>

namespace launch {
bool Parse(const std::vector<std::wstring>& arguments,Options& options,std::wstring& error) {
  Options candidate; bool display=false,size=false,resume=false;
  for(std::size_t i=0;i<arguments.size();++i) {
    const auto& value=arguments[i];
    if(value==L"--fullscreen" || value==L"--windowed") {
      if(display) {error=L"Choose one display mode.";return false;}
      display=true;candidate.fullscreen=value==L"--fullscreen";
    } else if(value==L"--resume") {
      if(resume) {error=L"Duplicate resume option.";return false;}
      resume=true;candidate.resume=true;
    } else if(value==L"--resolution") {
      if(size || ++i==arguments.size()) {error=L"Choose one window size.";return false;}
      size=true;bool found=false;
      for(unsigned n=0;n<resolutions.size();++n)
        if(arguments[i]==std::to_wstring(resolutions[n].width)+L"x"+std::to_wstring(resolutions[n].height)) {candidate.resolution=n;found=true;break;}
      if(!found) {error=L"Supported window sizes: 1280x720, 1600x900, 1920x1080.";return false;}
    } else {error=L"Unknown launch option: "+value;return false;}
  }
  options=candidate;error.clear();return true;
}
std::vector<std::wstring> Arguments(const Options& options) {
  if(options.resolution>=resolutions.size()) return {};
  const auto& size=resolutions[options.resolution];
  std::vector<std::wstring> result{options.fullscreen?L"--fullscreen":L"--windowed",L"--resolution",std::to_wstring(size.width)+L"x"+std::to_wstring(size.height)};
  if(options.resume) result.push_back(L"--resume");
  return result;
}
std::wstring Quote(const std::wstring& argument) {
  std::wstring result=L"\"";std::size_t slashes=0;
  for(wchar_t c:argument) {
    if(c==L'\\') {++slashes;continue;}
    result.append(c==L'"'?slashes*2+1:slashes,L'\\');slashes=0;result+=c;
  }
  result.append(slashes*2,L'\\');return result+L'"';
}
std::filesystem::path Directory() {
  std::wstring path(32768,L'\0');
  const auto size=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
  if(!size || size>=path.size()) return {};
  path.resize(size);return std::filesystem::path(path).parent_path();
}
std::filesystem::path SavePath() { return std::filesystem::path(RPG_SAVE_DIRECTORY)/"patrol.save"; }
std::filesystem::path GuidePath() { return std::filesystem::path(RPG_PROJECT_DIRECTORY)/"START_AND_DEBUG.md"; }
bool ReadSnapshot(const std::filesystem::path& path,std::string& content) {
  std::error_code error;const auto size=std::filesystem::file_size(path,error);
  if(error || !size || size>4096) return false;
  std::ifstream input(path,std::ios::binary);std::string candidate(std::size_t(size),'\0');
  if(!input.read(candidate.data(),std::streamsize(size)) || input.peek()!=std::char_traits<char>::eof()) return false;
  content=std::move(candidate);return true;
}
bool LoadSettings(const std::filesystem::path& path,Options& options) {
  std::error_code error;
  if(!std::filesystem::exists(path,error) && !error) {options={};return true;}
  const auto size=std::filesystem::file_size(path,error);
  if(error || size>128) return false;
  std::ifstream input(path);std::string magic,extra;int version=-1,fullscreen=-1,resume=-1,resolution=-1;
  if(!(input>>magic>>version>>fullscreen>>resolution>>resume) || magic!="MEDITERRANEAN_LAUNCHER" || version!=1 ||
    fullscreen<0 || fullscreen>1 || resume<0 || resume>1 || resolution<0 || resolution>=int(resolutions.size()) || input>>extra) return false;
  options={fullscreen!=0,resume!=0,unsigned(resolution)};return true;
}
bool SaveSettings(const std::filesystem::path& path,const Options& options) {
  if(options.resolution>=resolutions.size()) return false;
  const auto temporary=std::filesystem::path(path.wstring()+L".tmp");
  std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
  output<<"MEDITERRANEAN_LAUNCHER 1\n"<<int(options.fullscreen)<<' '<<options.resolution<<' '<<int(options.resume)<<'\n';
  output.close();
  return output && MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
std::wstring InstanceName(const std::filesystem::path& directory,const wchar_t* role) {
  // Kernel-object name only; no registry or machine-wide configuration changes.
  std::error_code error;auto canonical=std::filesystem::weakly_canonical(directory,error);
  if(error) canonical=directory.lexically_normal();
  unsigned long long hash=14695981039346656037ull;
  for(wchar_t c:canonical.wstring()) {hash^=static_cast<unsigned long long>(std::towlower(c));hash*=1099511628211ull;}
  return L"Local\\Mediterranean-"+std::wstring(role)+L"-"+std::to_wstring(hash);
}
Instance::Instance(const std::wstring& name) {
  handle_=CreateMutexW(nullptr,FALSE,name.c_str());acquired_=handle_ && GetLastError()!=ERROR_ALREADY_EXISTS;
}
Instance::~Instance() {if(handle_) CloseHandle(handle_);}
bool IsRunning(const std::wstring& name) {
  const auto handle=OpenMutexW(SYNCHRONIZE,FALSE,name.c_str());
  if(handle) {CloseHandle(handle);return true;}
  return GetLastError()==ERROR_ACCESS_DENIED;
}
Process::~Process() {if(handle_) CloseHandle(handle_);}
bool Process::Start(const std::filesystem::path& executable,const std::vector<std::wstring>& arguments,std::wstring& error,bool hidden) {
  if(handle_) {error=L"The game is already running.";return false;}
  std::error_code fileError;
  if(!executable.is_absolute() || !std::filesystem::is_regular_file(executable,fileError)) {
    error=L"Game executable not found. Build RPGNative beside this launcher.";return false;
  }
  auto command=Quote(executable.wstring());
  for(const auto& argument:arguments) command+=L' '+Quote(argument);
  if(command.size()>=32767 || command.find(L'\0')!=command.npos) {error=L"Launch arguments are too long or invalid.";return false;}
  STARTUPINFOW startup{sizeof(startup)};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=hidden?SW_HIDE:SW_SHOWNORMAL;
  PROCESS_INFORMATION process{};
  if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,executable.parent_path().c_str(),&startup,&process)) {
    const DWORD code=GetLastError();wchar_t description[512]{};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,0,description,512,nullptr);
    error=L"Cannot start the game ("+std::to_wstring(code)+L"): "+description;return false;
  }
  CloseHandle(process.hThread);handle_=process.hProcess;error.clear();return true;
}
bool Process::Poll(DWORD& exitCode) {
  if(!handle_ || WaitForSingleObject(handle_,0)!=WAIT_OBJECT_0) return false;
  if(!GetExitCodeProcess(handle_,&exitCode)) exitCode=1;
  CloseHandle(handle_);handle_=nullptr;return true;
}
}
