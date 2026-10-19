#pragma once
#include <Windows.h>

// Raw motion survives a pinned desktop cursor. Absolute devices need a baseline.
class MouseMotion {
public:
  struct Delta { float x=0,y=0; };
  void Reset() { *this=MouseMotion{}; }
  void Add(const RAWMOUSE& mouse,HANDLE device,int desktopWidth,int desktopHeight) {
    if (mouse.usFlags&MOUSE_MOVE_ABSOLUTE) {
      const bool virtualDesktop=(mouse.usFlags&MOUSE_VIRTUAL_DESKTOP)!=0;
      if (absolute_ && device==device_ && virtualDesktop==virtualDesktop_ &&
          desktopWidth==width_ && desktopHeight==height_) {
        delta_.x+=float(mouse.lLastX-previous_.x)*float(desktopWidth)/65535.f;
        delta_.y+=float(mouse.lLastY-previous_.y)*float(desktopHeight)/65535.f;
      }
      previous_={mouse.lLastX,mouse.lLastY}; device_=device;
      width_=desktopWidth; height_=desktopHeight; virtualDesktop_=virtualDesktop; absolute_=true;
    } else {
      delta_.x+=float(mouse.lLastX); delta_.y+=float(mouse.lLastY); absolute_=false;
    }
  }
  Delta Consume() { const auto result=delta_; delta_={}; return result; }
private:
  Delta delta_;
  POINT previous_{};
  HANDLE device_=nullptr;
  int width_=0,height_=0;
  bool absolute_=false,virtualDesktop_=false;
};
