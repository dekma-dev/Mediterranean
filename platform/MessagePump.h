#pragma once
#include <Windows.h>
#include <cstdint>

namespace platform {
struct PumpResult {
  unsigned messages=0;
  bool quit=false,budgetReached=false;
  int exitCode=0;
  double seconds=0;
};
// A handler/OS modal loop cannot be preempted. Limits apply between dispatches.
inline PumpResult PumpMessages(unsigned limit=256,double budgetSeconds=.002) {
  PumpResult result;MSG message{};
  LARGE_INTEGER start{},now{},frequency{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
  const auto quit=[&]{result.quit=true;result.exitCode=int(message.wParam);};
  // Give an available quit priority. PostQuitMessage itself is generated only
  // once posted work drains; Engine also checks destruction of its own window.
  if(PeekMessageW(&message,nullptr,WM_QUIT,WM_QUIT,PM_REMOVE)){quit();return result;}
  while(result.messages<limit && PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
    if(message.message==WM_QUIT){quit();break;}
    TranslateMessage(&message);DispatchMessageW(&message);++result.messages;
    QueryPerformanceCounter(&now);
    if(frequency.QuadPart>0 && double(now.QuadPart-start.QuadPart)/double(frequency.QuadPart)>=budgetSeconds){result.budgetReached=true;break;}
  }
  result.budgetReached|=result.messages==limit;
  QueryPerformanceCounter(&now);
  if(frequency.QuadPart>0)result.seconds=double(now.QuadPart-start.QuadPart)/double(frequency.QuadPart);
  return result;
}
// Unlike WaitMessage, already-inspected queued input also wakes this wait.
inline DWORD WaitForMessages(DWORD milliseconds=INFINITE) {
  return MsgWaitForMultipleObjectsEx(0,nullptr,milliseconds,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
}
}
