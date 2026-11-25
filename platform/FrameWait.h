#pragma once
#include <Windows.h>
#include <algorithm>
#include <cmath>

namespace platform {
// A process-owned timer. No timer-resolution, driver or system setting change.
// Input/close/focus messages wake promptly; Engine rechecks the deadline.
class FrameWait {
public:
  FrameWait(){
    timer_=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_MODIFY_STATE|SYNCHRONIZE);
    if(!timer_)timer_=CreateWaitableTimerExW(nullptr,nullptr,0,TIMER_MODIFY_STATE|SYNCHRONIZE);
  }
  ~FrameWait(){if(timer_)CloseHandle(timer_);}
  FrameWait(const FrameWait&)=delete;
  FrameWait& operator=(const FrameWait&)=delete;
  DWORD Wait(double seconds){
    if(!std::isfinite(seconds) || seconds<=0)return WAIT_TIMEOUT;
    seconds=std::min(seconds,1.0);
    LARGE_INTEGER due{};due.QuadPart=-std::max<LONGLONG>(1,LONGLONG(std::ceil(seconds*10000000)));
    if(timer_ && SetWaitableTimer(timer_,&due,0,nullptr,nullptr,FALSE))
      return MsgWaitForMultipleObjectsEx(1,&timer_,INFINITE,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
    return MsgWaitForMultipleObjectsEx(0,nullptr,DWORD(std::ceil(seconds*1000)),QS_ALLINPUT,MWMO_INPUTAVAILABLE);
  }
private:
  HANDLE timer_=nullptr;
};
}
