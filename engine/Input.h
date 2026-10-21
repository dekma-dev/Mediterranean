#pragma once
#include <Windows.h>
#include <array>
class Input {
public:
  // Sample the platform once; transitions are also testable without desktop input.
  struct Frame {
    bool focused=false;
    bool capsLock=false;
    std::array<bool,256> down{};
    POINT screen{},client{};
    int width=1280,height=720;
    bool relativeMouse=false;
    float mouseX=0,mouseY=0;
  };
  void Update(HWND window,bool relativeMouse=false,float mouseX=0,float mouseY=0) {
    Frame frame; frame.focused=GetForegroundWindow()==window;
    frame.capsLock=(GetKeyState(VK_CAPITAL)&1)!=0;
    frame.relativeMouse=relativeMouse; frame.mouseX=mouseX; frame.mouseY=mouseY;
    for (int key=0;key<256;++key) frame.down[key]=(GetAsyncKeyState(key)&0x8000)!=0;
    GetCursorPos(&frame.screen); frame.client=frame.screen; ScreenToClient(window,&frame.client);
    RECT client{}; GetClientRect(window,&client); frame.width=client.right; frame.height=client.bottom;
    Update(frame);
  }
  void Update(const Frame& frame) {
    const bool focused=frame.focused;capsLock_=frame.capsLock;
    for (int key = 0; key < 256; ++key) {
      const bool down = focused && frame.down[key];
      pressed_[key] = wasFocused_ && down && !held_[key];
      released_[key] = focused && wasFocused_ && !down && held_[key];
      held_[key] = down;
    }
    const POINT mouse=frame.screen;
    // A released/restarted native capture can move the desktop cursor between
    // samples. Do not reinterpret that transition as camera movement.
    const bool moving=focused && wasFocused_ && held_[VK_RBUTTON] && dragging_ &&
      (frame.relativeMouse || !wasRelative_);
    dx_ = moving ? (frame.relativeMouse?frame.mouseX:float(mouse.x-previous_.x)) : 0;
    dy_ = moving ? (frame.relativeMouse?frame.mouseY:float(mouse.y-previous_.y)) : 0;
    previous_ = mouse; dragging_ = held_[VK_RBUTTON]; wasFocused_ = focused; wasRelative_=frame.relativeMouse;
    clientX_=float(frame.client.x); clientY_=float(frame.client.y);
    width_=float(frame.width); height_=float(frame.height);
  }
  // An OS cancellation can occur and finish between samples. Resynchronize
  // held buttons/keys on the next sample without generating presses/releases
  // from another window or from a native move/menu loop.
  void CancelGestures() {
    wasFocused_=dragging_=wasRelative_=false;
    held_.fill(false); pressed_.fill(false); released_.fill(false);
    dx_=dy_=0;
  }
  bool Held(int key) const { return held_[key]; }
  bool Focused() const { return wasFocused_; }
  bool CapsLock() const { return capsLock_; }
  bool Pressed(int key) const { return pressed_[key]; }
  bool Released(int key) const { return released_[key]; }
  float MouseX() const { return dx_; }
  float MouseY() const { return dy_; }
  float ClientX() const { return clientX_; }
  float ClientY() const { return clientY_; }
  float Width() const { return width_; }
  float Height() const { return height_; }
private:
  std::array<bool,256> held_{}, pressed_{}, released_{};
  POINT previous_{};
  float dx_ = 0, dy_ = 0;
  float clientX_ = -1, clientY_ = -1,width_ = 1280,height_ = 720;
  bool wasFocused_ = false, dragging_ = false,wasRelative_=false,capsLock_=false;
};
