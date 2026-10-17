#pragma once
#include <Windows.h>
#include <array>
class Input {
public:
  // Sample the platform once; transitions are also testable without desktop input.
  struct Frame {
    bool focused=false;
    std::array<bool,256> down{};
    POINT screen{},client{};
    int width=1280,height=720;
  };
  void Update(HWND window) {
    Frame frame; frame.focused=GetForegroundWindow()==window;
    for (int key=0;key<256;++key) frame.down[key]=(GetAsyncKeyState(key)&0x8000)!=0;
    GetCursorPos(&frame.screen); frame.client=frame.screen; ScreenToClient(window,&frame.client);
    RECT client{}; GetClientRect(window,&client); frame.width=client.right; frame.height=client.bottom;
    Update(frame);
  }
  void Update(const Frame& frame) {
    const bool focused=frame.focused;
    for (int key = 0; key < 256; ++key) {
      const bool down = focused && frame.down[key];
      pressed_[key] = wasFocused_ && down && !held_[key];
      released_[key] = focused && wasFocused_ && !down && held_[key];
      held_[key] = down;
    }
    const POINT mouse=frame.screen;
    dx_ = focused && wasFocused_ && held_[VK_RBUTTON] && dragging_ ? float(mouse.x - previous_.x) : 0;
    dy_ = focused && wasFocused_ && held_[VK_RBUTTON] && dragging_ ? float(mouse.y - previous_.y) : 0;
    previous_ = mouse; dragging_ = held_[VK_RBUTTON]; wasFocused_ = focused;
    clientX_=float(frame.client.x); clientY_=float(frame.client.y);
    width_=float(frame.width); height_=float(frame.height);
  }
  bool Held(int key) const { return held_[key]; }
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
  bool wasFocused_ = false, dragging_ = false;
};
