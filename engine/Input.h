#pragma once
#include <Windows.h>
#include <array>
class Input {
public:
  void Update(HWND window) {
    const bool focused = GetForegroundWindow() == window;
    for (int key = 0; key < 256; ++key) {
      const bool down = focused && (GetAsyncKeyState(key) & 0x8000) != 0;
      pressed_[key] = wasFocused_ && down && !held_[key];
      held_[key] = down;
    }
    POINT mouse{}; GetCursorPos(&mouse);
    dx_ = focused && wasFocused_ && held_[VK_RBUTTON] && dragging_ ? float(mouse.x - previous_.x) : 0;
    dy_ = focused && wasFocused_ && held_[VK_RBUTTON] && dragging_ ? float(mouse.y - previous_.y) : 0;
    previous_ = mouse; dragging_ = held_[VK_RBUTTON]; wasFocused_ = focused;
    ScreenToClient(window,&mouse); clientX_=float(mouse.x); clientY_=float(mouse.y);
    RECT client{}; GetClientRect(window,&client); width_=float(client.right); height_=float(client.bottom);
  }
  bool Held(int key) const { return held_[key]; }
  bool Pressed(int key) const { return pressed_[key]; }
  float MouseX() const { return dx_; }
  float MouseY() const { return dy_; }
  float ClientX() const { return clientX_; }
  float ClientY() const { return clientY_; }
  float Width() const { return width_; }
  float Height() const { return height_; }
private:
  std::array<bool,256> held_{}, pressed_{};
  POINT previous_{};
  float dx_ = 0, dy_ = 0;
  float clientX_ = -1, clientY_ = -1,width_ = 1280,height_ = 720;
  bool wasFocused_ = false, dragging_ = false;
};
