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
  }
  bool Held(int key) const { return held_[key]; }
  bool Pressed(int key) const { return pressed_[key]; }
  float MouseX() const { return dx_; }
  float MouseY() const { return dy_; }
private:
  std::array<bool,256> held_{}, pressed_{};
  POINT previous_{};
  float dx_ = 0, dy_ = 0;
  bool wasFocused_ = false, dragging_ = false;
};
