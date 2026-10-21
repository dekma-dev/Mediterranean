#include "launcher/LaunchSupport.h"
#include "engine/Engine.h"
#include <shellapi.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <thread>

namespace {
void Check(bool result,const char* message) {if(!result) {std::cerr<<message<<'\n';std::exit(1);}}
void Write(const std::filesystem::path& path,const std::string& text) {std::ofstream file(path,std::ios::binary);file<<text;Check(bool(file),"Test fixture write");}
std::string Read(const std::filesystem::path& path) {std::ifstream file(path,std::ios::binary);Check(bool(file),"Test fixture read");return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};}
void Options(const std::filesystem::path& root) {
  std::wstring error;launch::Options parsed;
  for(unsigned size=0;size<launch::resolutions.size();++size) for(bool full:{false,true}) for(bool resume:{false,true}) {
    const launch::Options source{full,resume,size};
    Check(launch::Parse(launch::Arguments(source),parsed,error) && parsed.fullscreen==full && parsed.resume==resume && parsed.resolution==size,"Launch option round-trip");
    Check(launch::SaveSettings(root/L"settings.ini",source),"Save launcher preferences");
    Check(launch::LoadSettings(root/L"settings.ini",parsed) && parsed.fullscreen==full && parsed.resume==resume && parsed.resolution==size,"Preference round-trip");
  }
  for(const std::vector<std::wstring>& args:{std::vector<std::wstring>{L"--fullscreen",L"--windowed"},{L"--resume",L"--resume"},{L"--resolution"},{L"--resolution",L"9999x9999"},{L"--resolution",L"1280x720",L"--resolution",L"1600x900"},{L"--unknown"}})
    Check(!launch::Parse(args,parsed,error) && !error.empty(),"Invalid/duplicate launch options accepted");
  for(const auto& content:{std::string("bad"),std::string("MEDITERRANEAN_LAUNCHER 2 1 0 0"),std::string("MEDITERRANEAN_LAUNCHER 1 2 0 0"),std::string("MEDITERRANEAN_LAUNCHER 1 1 3 0"),std::string("MEDITERRANEAN_LAUNCHER 1 1 0 0 extra"),std::string(129,'x')}) {
    Write(root/L"settings.ini",content);parsed={false,true,2};
    Check(!launch::LoadSettings(root/L"settings.ini",parsed) && !parsed.fullscreen && parsed.resume && parsed.resolution==2,"Invalid preferences changed caller state");
  }
  parsed={false,true,2};Check(launch::LoadSettings(root/L"absent.ini",parsed) && parsed.fullscreen && !parsed.resume && parsed.resolution==0,"Missing settings should use defaults");
  const std::vector<std::wstring> cases{L"",L"plain",L"space path",L"quoted\"value",L"trailing slash\\",L"two\\\\\"quotes",L"\u0425\u043e\u0437\u044f\u0438\u043d",L"&literal $(data)"};
  std::wstring command=L"probe.exe";for(const auto& value:cases) command+=L" "+launch::Quote(value);
  int count=0;auto** tokens=CommandLineToArgvW(command.c_str(),&count);
  Check(tokens && count==int(cases.size())+1,"Quoted command token count");
  for(unsigned i=0;i<cases.size();++i) Check(tokens[i+1]==cases[i],"Windows argument escaping changed a value");
  LocalFree(tokens);
  std::string snapshot="unchanged";Write(root/L"snapshot.save",std::string(4097,'x'));
  Check(!launch::ReadSnapshot(root/L"snapshot.save",snapshot) && snapshot=="unchanged","Oversized save accepted/modified output");
  Write(root/L"snapshot.save","");Check(!launch::ReadSnapshot(root/L"snapshot.save",snapshot),"Empty resume file accepted");
}
void Processes(const std::filesystem::path& root) {
  const auto name=launch::InstanceName(root,L"test");
  Check(!launch::IsRunning(name),"Test instance leaked");
  {launch::Instance first(name);Check(first.Acquired() && launch::IsRunning(name),"Instance did not publish its lifetime");
    launch::Instance second(name);Check(!second.Acquired(),"Duplicate instance accepted");}
  Check(!launch::IsRunning(name),"Instance was not released");
  Check(name==launch::InstanceName(root/L"." ,L"test"),"Instance path normalization");
  const auto directory=root/L"path with spaces \u0418\u0433\u0440\u0430";std::filesystem::create_directories(directory);
  const auto executable=directory/L"process probe.exe";
  std::filesystem::copy_file(launch::Directory()/L"LauncherTests.exe",executable,std::filesystem::copy_options::overwrite_existing);
  const auto report=directory/L"probe report.txt";
  std::wstring error;launch::Process process;
  Check(!process.Start(directory/L"absent.exe",{},error) && !process.Active() && !error.empty(),"Missing executable did not fail cleanly");
  Check(!process.Start(L"relative.exe",{},error),"Relative executable must not use PATH search");
  const std::vector<std::wstring> args{L"--probe",report.wstring(),L"space path",L"quote\"and\\",L"\u0422\u0435\u0441\u0442"};
  Check(process.Start(executable,args,error,true),"Could not start Unicode/spaced executable");
  Check(!process.Start(executable,args,error,true),"Duplicate owned game process accepted");
  DWORD code=0;const auto start=GetTickCount64();
  while(!process.Poll(code) && GetTickCount64()-start<10000) Sleep(10);
  Check(!process.Active() && code==7 && Read(report)=="PASS", "Child argument/cwd/exit-code handling");
  Check(process.Start(executable,args,error,true),"Relaunch after exit failed");
  const auto second=GetTickCount64();while(!process.Poll(code) && GetTickCount64()-second<10000) Sleep(10);
  Check(!process.Active() && code==7,"Relaunch exit was lost");
  const auto detached=directory/L"launcher-closed.txt";
  Write(detached,"pending");
  {launch::Process child;auto request=args;request[1]=detached.wstring();Check(child.Start(executable,request,error,true),"Detached child launch");}
  bool finished=false;const auto wait=GetTickCount64();
  while(!finished && GetTickCount64()-wait<10000) {
    std::ifstream output(detached);std::string text;output>>text;finished=output && text=="PASS";
    if(!finished) Sleep(10);
  }
  Check(finished,"Closing the launcher terminated its child");
  launch::Process game;
  const auto capture=root/L"process-smoke.png";
  Check(game.Start(launch::Directory()/L"RPGNative.exe",{L"--smoke-test",capture.wstring()},error,true),"Launcher service could not start the real game");
  const auto gameStart=GetTickCount64();while(!game.Poll(code) && GetTickCount64()-gameStart<30000) Sleep(10);
  Check(!game.Active() && code==0 && Read(capture.wstring()+L".log").find("PASS:")==0,"Real game launch/render/exit through launcher service");
}
void Startup(const std::filesystem::path& root) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Startup COM");
  const auto snapshot=Read(std::filesystem::path(RPG_TEST_FIXTURES)/"veteran-preview.save");
  for(unsigned size=0;size<launch::resolutions.size();++size) {
    const auto resolution=launch::resolutions[size];const std::string saved=size==1?snapshot:std::string{};
    Engine engine;Check(engine.Initialize(GetModuleHandleW(nullptr),resolution.width,resolution.height,false,false,saved),"Windowed startup initialization");
    const bool valid=engine.StartupTest(false,resolution.width,resolution.height,saved,(root/(L"startup-"+std::to_wstring(size)+L".png")).wstring());
    if(!valid) std::wcerr<<engine.Error()<<L'\n';
    Check(valid,"Windowed startup size/save/render disagreed with launcher options");
  }
  {Engine engine;Check(engine.Initialize(GetModuleHandleW(nullptr),1280,720,false,true,snapshot),"Fullscreen resume initialization");
    Check(engine.StartupTest(true,1280,720,snapshot,(root/L"startup-fullscreen.png").wstring()),"Fullscreen resume startup state/render");}
  {Engine engine;Check(!engine.Initialize(GetModuleHandleW(nullptr),1280,720,false,false,"corrupt") && !engine.Error().empty(),"Corrupt startup save should fail explicitly");}
  CoUninitialize();
}
}
int wmain(int count,wchar_t** arguments) {
  if(count>1) {
    if(count!=6 || std::wstring(arguments[1])!=L"--probe" || std::wstring(arguments[3])!=L"space path" || std::wstring(arguments[4])!=L"quote\"and\\" || std::wstring(arguments[5])!=L"\u0422\u0435\u0441\u0442") return 9;
    if(std::filesystem::current_path()!=launch::Directory()) return 8;
    Sleep(150);Write(arguments[2],"PASS");return 7;
  }
  const auto root=std::filesystem::current_path()/L"launcher-check";std::filesystem::create_directories(root);
  Options(root);Processes(root);Startup(root);
  std::cout<<"PASS: launcher options/settings, quoting, Unicode process paths, duplicate/relaunch/lifetime checks, three window sizes, fullscreen, resume and bad-save rejection\n";
}
