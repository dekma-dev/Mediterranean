#include "engine/FrameClock.h"
#include "engine/Input.h"
#include "engine/Engine.h"
#include "engine/Renderer.h"
#include "platform/MessagePump.h"
#include "platform/Win32Window.h"
#include "game/Game.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <sstream>

struct ExecutionTestAccess {
  static HWND Window(Engine& engine){return engine.window_->GetHandle();}
  static std::uint64_t HudRevision(Engine& engine){return engine.game_->Hud().Revision();}
  static std::uint64_t GpuFrames(Engine& engine){return engine.renderer_->GpuFrame();}
};
namespace {
void Check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void Close(double a,double b,const char* why){Check(std::abs(a-b)<1e-8,why);}
void ClockAccounting(){
  for(int hz:{20,30,60,144,1000}){
    execution::FrameClock clock(1000000000);Check(clock.Sample(0,true).resynchronized,"Initial baseline");
    for(int n=1;n<=hz*2;++n)clock.Sample(std::llround(double(n)*1000000000/hz),true);
    Close(clock.Stats().gameSeconds,2,"Ordinary frame time changed");
    Check(clock.Stats().frames==unsigned(hz*2) && !clock.Stats().clampedFrames,"Normal frames were capped");
  }
  execution::FrameClock clock(1000);clock.Sample(0,true);
  Close(clock.Sample(10,true).seconds,.01,"Initial running step");
  Close(clock.Sample(510,true).seconds,.05,"Existing stall ceiling changed");
  Close(clock.Sample(520,true).seconds,.01,"Clipped stall created catch-up debt");
  Check(clock.Sample(5520,false).seconds==0 && clock.Sample(10520,true).resynchronized,"Inactive interval leaked into simulation");
  Close(clock.Sample(10530,true).seconds,.01,"Resume retained elapsed pause time");
  Check(clock.Sample(20530,true,true).resynchronized,"Modal loop did not reset the frame clock");
  Close(clock.Sample(20540,true).seconds,.01,"Modal return retained time debt");
  Check(clock.Sample(20539,true).seconds==0 && clock.Sample(20550,true).resynchronized,"Backward counter admitted negative or duplicate time");
  const auto& s=clock.Stats();
  Close(s.wallSeconds,s.gameSeconds+s.excludedSeconds+s.clampedSeconds,"Wall-time accounting lost time");
  Check(s.invalidSamples==1 && s.clampedFrames==1,"Clock diagnostics lost incidents");
  execution::FrameClock invalid(0);Check(invalid.Sample(1,true).seconds==0 && invalid.Stats().invalidSamples==1,"Invalid frequency accepted");
}
void Keyboard(){
  Input input;Input::Frame frame;frame.focused=true;frame.nativeKeys=true;input.Update(frame);
  KeyTransitions keys;keys.Add('1',true,false,KeyTransitions::Control);keys.Add('1',false,false);
  frame.keys=keys.Consume();input.Update(frame);
  Check(input.Pressed('1') && input.Released('1') && !input.Held('1'),"Between-frame tap was lost");
  Check(input.Modifiers('1')==KeyTransitions::Control,"Released Ctrl changed the shortcut's meaning");
  frame.keys=keys.Consume();input.Update(frame);Check(!input.Pressed('1') && !input.Released('1'),"Tap replayed next frame");
  frame.down['1']=true;input.Update(frame);Check(input.Held('1') && !input.Pressed('1'),"Polling bypassed queued native keyboard edges");
  for(int n=0;n<100000;++n)keys.Add('1',true,true);
  frame.keys=keys.Consume();input.Update(frame);Check(!input.Pressed('1'),"OS autorepeat became repeated game actions");
  keys.Add('1',true,false,KeyTransitions::Shift);keys.Add('1',true,false,KeyTransitions::Control);
  frame.keys=keys.Consume();input.Update(frame);Check(input.Modifiers('1')==KeyTransitions::Shift,"Coalesced tap replaced first modifiers");
  input.CancelGestures();input.Update(frame);Check(!input.Pressed('1'),"Cancellation replayed a queued press");
  frame.focused=false;input.Update(frame);frame.focused=true;input.Update(frame);Check(!input.Pressed('1'),"Refocus manufactured a press");
  keys.Add(999,true,false);keys.Clear();frame.keys=keys.Consume();frame.down['1']=false;input.Update(frame);
  Check(!input.Released('1'),"Physical snapshot invented a native release");
  frame.down[VK_LBUTTON]=true;input.Update(frame);Check(input.Pressed(VK_LBUTTON),"Keyboard handoff changed pointer sampling");
}
unsigned dispatched=0,repost=0;
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM w,LPARAM l){
  if(message==WM_APP+1){++dispatched;if(repost){--repost;PostMessageW(window,message,0,0);}return 0;}
  if(message==WM_APP+2){Sleep(4);++dispatched;return 0;}
  return DefWindowProcW(window,message,w,l);
}
void NativePump(){
  WNDCLASSW type{};type.lpfnWndProc=WindowProc;type.hInstance=GetModuleHandleW(nullptr);type.lpszClassName=L"ExecutionPumpTests";
  Check(RegisterClassW(&type)!=0,"Register hidden pump fixture");
  HWND window=CreateWindowW(type.lpszClassName,L"Execution fixture",0,0,0,0,0,HWND_MESSAGE,nullptr,type.hInstance,nullptr);
  Check(window!=nullptr,"Hidden message fixture");platform::PumpMessages(1024,1);
  for(int n=0;n<300;++n)Check(PostMessageW(window,WM_APP+1,0,0)!=FALSE,"Post bounded workload");
  auto result=platform::PumpMessages(32,1);Check(result.messages==32 && result.budgetReached && dispatched==32,"Message burst starved frame boundary");
  unsigned frames=1;while(dispatched<300){platform::PumpMessages(32,1);++frames;}Check(frames==10,"Message burst dropped or duplicated work");
  repost=100;PostMessageW(window,WM_APP+1,0,0);const auto before=dispatched;
  result=platform::PumpMessages(16,1);Check(dispatched-before==16 && result.budgetReached,"Self-reposting work monopolized the loop");
  repost=0;platform::PumpMessages(1024,1);
  PostMessageW(window,WM_APP+2,0,0);PostMessageW(window,WM_APP+1,0,0);
  result=platform::PumpMessages(256,.001);Check(result.messages==1 && result.budgetReached,"Dispatch time budget ignored a slow handler");
  platform::PumpMessages(1024,1);
  PostMessageW(window,WM_APP+1,0,0);MSG message{};
  Check(PeekMessageW(&message,window,WM_APP+1,WM_APP+1,PM_NOREMOVE)!=FALSE,"Inspect pending message");
  Check(platform::WaitForMessages(0)==WAIT_OBJECT_0,"Already inspected input failed to wake idle loop");platform::PumpMessages(1024,1);
  for(int n=0;n<300;++n)PostMessageW(window,WM_APP+1,0,0);
  PostQuitMessage(7);unsigned quitFrames=0;
  do{result=platform::PumpMessages(16,1);++quitFrames;}while(!result.quit && quitFrames<22);
  Check(result.quit && result.exitCode==7 && quitFrames<=20,"Generated quit was lost across bounded pumps");
  // Unlike PostQuitMessage's on-demand flag, this is an already posted quit.
  for(int n=0;n<300;++n)PostMessageW(window,WM_APP+1,0,0);
  PostThreadMessageW(GetCurrentThreadId(),WM_QUIT,8,0);const auto prior=dispatched;result=platform::PumpMessages(16,1);
  Check(result.quit && result.exitCode==8 && dispatched==prior,"Available quit was trapped behind queued work");platform::PumpMessages(1024,1);
  DestroyWindow(window);UnregisterClassW(type.lpszClassName,type.hInstance);
  Win32Window gameWindow;Check(gameWindow.Create(type.hInstance,960,600,L"Interruption fixture",false),"Hidden game window");
  gameWindow.ConsumeClockInterruption();gameWindow.ConsumeInputCancellation();
  for(auto event:{WM_ENTERSIZEMOVE,WM_EXITSIZEMOVE,WM_ENTERMENULOOP,WM_EXITMENULOOP,WM_KILLFOCUS}){
    SendMessageW(gameWindow.GetHandle(),event,0,0);
    Check(gameWindow.ConsumeClockInterruption() && !gameWindow.ConsumeClockInterruption(),"Native modal/focus interruption was not latched once");
    Check(gameWindow.ConsumeInputCancellation(),"Modal clock reset retained gestures");
  }
}
void GameEdgesAndTime(){
  auto game=std::make_unique<Game>();game->Initialize();
  Input input;Input::Frame frame;frame.focused=true;frame.nativeKeys=true;frame.client={600,300};input.Update(frame);
  KeyTransitions keys;keys.Add(VK_SPACE,true,false);keys.Add(VK_SPACE,false,false);frame.keys=keys.Consume();input.Update(frame);
  game->Update(.01,input,0);Check(game->State().Height()>0,"Short native-style Space tap did not reach actual jump mechanics");
  frame.keys=keys.Consume();frame.down['W']=true;input.Update(frame);
  execution::FrameClock clock(1000);clock.Sample(0,true);
  for(int n=1;n<=20;++n)game->Update(clock.Sample(n*10,true).seconds,input,0);
  const auto save=game->State().Save();const auto height=game->State().Height();
  const auto suspended=clock.Sample(10000,false),resumed=clock.Sample(20000,true);
  Check(suspended.seconds==0 && resumed.seconds==0 && save==game->State().Save() && height==game->State().Height(),"Paused clock changed the game");
  const auto p=game->State().Player();game->Update(clock.Sample(20010,true).seconds,input,0);
  Check(std::hypot(game->State().Player().x-p.x,game->State().Player().z-p.z)<.1f,"Resume produced a catch-up movement");
}
void InactiveEngineLoop(){
  // Exercise the real Run/wait path with a hidden owned window. No desktop
  // input, foreground changes, player saves or audio endpoint are involved.
  platform::PumpMessages(1024,1); // Drain the preceding fixture's WM_QUIT.
  Engine engine;Check(engine.Initialize(GetModuleHandleW(nullptr),960,540,false,false),"Hidden execution engine");
  const auto revision=ExecutionTestAccess::HudRevision(engine),frames=ExecutionTestAccess::GpuFrames(engine);
  const DWORD threadId=GetCurrentThreadId();
  for(int n=0;n<600;++n)Check(PostThreadMessageW(threadId,WM_APP+3,0,0)!=FALSE,"Queue engine work");
  std::thread wake([threadId]{Sleep(60);PostThreadMessageW(threadId,WM_QUIT,19,0);});
  const int result=engine.Run();wake.join();
  Check(result==19,"Native inactive engine failed to wake and preserve quit code");
  Check(ExecutionTestAccess::GpuFrames(engine)==frames,"Background messages caused GPU rendering");
  Check(ExecutionTestAccess::HudRevision(engine)==revision+1,"Background messages repeatedly rebuilt cancelled HUD state");
  const auto report=engine.ExecutionDiagnostics();
  Check(report.find("Admitted frames / clock resets / capped frames / invalid samples: 0 /")!=std::string::npos,"Inactive engine advanced game time");
  const std::string label="Wall / admitted / excluded / capped seconds: ";const auto at=report.find(label);
  Check(at!=std::string::npos,"Clock accounting missing from report");double wall=0;
  std::istringstream(report.substr(at+label.size()))>>wall;Check(wall>=.04,"Final inactive wait was missing from clock accounting");
  std::cout<<report;
}
void ClosedEngineLoop(){
  platform::PumpMessages(1024,1);
  Engine engine;Check(engine.Initialize(GetModuleHandleW(nullptr),960,540,false,false),"Hidden close-loop engine");
  const auto window=ExecutionTestAccess::Window(engine);
  Check(PostMessageW(window,WM_CLOSE,0,0)!=FALSE,"Queue game window close");
  for(int n=0;n<600;++n)PostThreadMessageW(GetCurrentThreadId(),WM_APP+3,0,0);
  Check(engine.Run()==0 && !IsWindow(window),"Closed engine kept waiting for an on-demand quit");
  MSG pending{};Check(PeekMessageW(&pending,nullptr,WM_APP+3,WM_APP+3,PM_NOREMOVE)!=FALSE,"Close loop drained all posted work before stopping");
  platform::PumpMessages(1024,1);
}
}
int main()try {
  ClockAccounting();Keyboard();GameEdgesAndTime();NativePump();InactiveEngineLoop();ClosedEngineLoop();
  std::cout<<"PASS: accounted frame time, pause/stall policies, short key edges, modifiers, repeat bounds, actual jump, native pump fairness/quit/wakeup and modal interruption\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
