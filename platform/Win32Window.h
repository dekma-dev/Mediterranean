#pragma once

#include <Windows.h>
#include "platform/MouseMotion.h"

class Win32Window {
public:
  Win32Window()=default;
  Win32Window(const Win32Window&)=delete;
  Win32Window& operator=(const Win32Window&)=delete;
  ~Win32Window() {
    EndMouseLook();
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
  bool SetMouseLook(bool enabled);
  bool MouseLook() const { return mouseLook_; }
  MouseMotion::Delta ConsumeMouseMotion() { return mouseMotion_.Consume(); }
  bool ConsumeInputCancellation() { const bool result=inputCancelled_; inputCancelled_=false; return result; }

private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  void RefreshCursor(UINT dpi);
  void EndMouseLook(bool cancelGesture=false);

  HWND hwnd_ = nullptr;
  HCURSOR cursor_ = nullptr;
  float wheel_ = 0;
  bool fullscreen_ = false;
  bool visible_ = false;
  bool mouseLook_=false,inputCancelled_=false,rightPress_=false;
  POINT rightAnchor_{};
  MouseMotion mouseMotion_;
  WINDOWPLACEMENT windowed_{sizeof(WINDOWPLACEMENT)};
};
