#include "engine/FramePacing.h"
#include "engine/FrameClock.h"
#include "platform/FrameWait.h"
#include "platform/MessagePump.h"
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void Check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void Policy(){
  for(unsigned hz:{30u,60u}){
    execution::FramePacing pacer(1000000);execution::FrameClock clock(1000000);
    clock.Sample(0,true);unsigned frames=0;
    for(std::int64_t t=0;t<2000000;t+=100){
      if(pacer.Remaining(t,hz)>0)continue;
      clock.Sample(t,true);pacer.Admit(t);++frames;
    }
    Check(frames==hz*2,"Messages bypassed the FPS ceiling or phase drifted");
    Check(std::abs(clock.Stats().gameSeconds-(2.0-1.0/hz))<.0002,"Pacing lost elapsed simulation time");
    Check(pacer.Remaining(9000000,hz)==0,"Long stall retained old deadline");pacer.Admit(9000000);
    Check(pacer.Remaining(9000001,hz)>.9/hz,"Long stall produced a catch-up render burst");
    pacer.Reset();Check(pacer.Remaining(20000000,hz)==0,"Focus resume retained waiting debt");
    pacer.Admit(20000000);Check(pacer.Remaining(20000001,hz==30?60:30)==0,"Changing screen rate retained old deadline");
    Check(pacer.Remaining(1,hz)==0,"Backward clock retained a future deadline");
  }
}
void Native(){
  LARGE_INTEGER frequency{},start{},now{};QueryPerformanceFrequency(&frequency);
  platform::FrameWait wait;
  MSG message{};PeekMessageW(&message,nullptr,0,0,PM_NOREMOVE);
  const DWORD owner=GetCurrentThreadId();
  std::thread wake([owner]{Sleep(15);PostThreadMessageW(owner,WM_APP+9,0,0);});
  QueryPerformanceCounter(&start);const auto result=wait.Wait(.5);QueryPerformanceCounter(&now);wake.join();
  Check(result!=WAIT_FAILED && double(now.QuadPart-start.QuadPart)/frequency.QuadPart<.4,"Frame timer blocked window messages");
  Check(PeekMessageW(&message,nullptr,WM_APP+9,WM_APP+9,PM_REMOVE)!=FALSE,"Frame wait consumed input");
  for(unsigned hz:{60u,30u}){
    execution::FramePacing pacer(frequency.QuadPart);QueryPerformanceCounter(&start);
    unsigned frames=0,waits=0;double elapsed=0;
    while(true){
      platform::PumpMessages();QueryPerformanceCounter(&now);elapsed=double(now.QuadPart-start.QuadPart)/frequency.QuadPart;
      if(elapsed>=1.2)break;
      const double remaining=pacer.Remaining(now.QuadPart,hz);
      if(remaining>0){Check(wait.Wait(remaining)!=WAIT_FAILED,"Native timer wait");++waits;continue;}
      pacer.Admit(now.QuadPart);++frames;
    }
    Check(frames<=unsigned(std::ceil(elapsed*hz))+1 && frames>=hz/2 && waits>=frames/2,"Native pacing unbounded or did not sleep");
    std::cout<<"Native limit "<<hz<<" Hz: "<<frames<<" frames / "<<elapsed<<" seconds / "<<waits<<" waits\n";
  }
}
}
int main()try{Policy();Native();std::cout<<"PASS: bounded cadence, input wake, no catch-up burst and preserved wall-time admission\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
