#pragma once

#include <Windows.h>

class Win32Window {
public:
  ~Win32Window() { if (hwnd_ && IsWindow(hwnd_)) DestroyWindow(hwnd_); }
  bool Create(HINSTANCE instance, int width, int height, const wchar_t* title);
  HWND GetHandle() const { return hwnd_; }
  float ConsumeWheel() { const float result = wheel_; wheel_ = 0; return result; }

private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  HWND hwnd_ = nullptr;
  float wheel_ = 0;
};
