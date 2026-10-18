#pragma once

#include <Windows.h>

class Win32Window {
public:
  Win32Window()=default;
  Win32Window(const Win32Window&)=delete;
  Win32Window& operator=(const Win32Window&)=delete;
  ~Win32Window() {
    if (hwnd_ && IsWindow(hwnd_)) DestroyWindow(hwnd_);
    if (cursor_) {
      if (GetCursor()==cursor_) SetCursor(LoadCursor(nullptr,IDC_ARROW));
      DestroyCursor(cursor_);
    }
  }
  bool Create(HINSTANCE instance, int width, int height, const wchar_t* title, bool visible = true);
  HWND GetHandle() const { return hwnd_; }
  bool SetFullscreen(bool enabled);
  bool Fullscreen() const { return fullscreen_; }
  float ConsumeWheel() { const float result = wheel_; wheel_ = 0; return result; }

private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  void RefreshCursor(UINT dpi);

  HWND hwnd_ = nullptr;
  HCURSOR cursor_ = nullptr;
  float wheel_ = 0;
  bool fullscreen_ = false;
  bool visible_ = false;
  WINDOWPLACEMENT windowed_{sizeof(WINDOWPLACEMENT)};
};
