#pragma once

#include <Windows.h>

class Win32Window {
public:
  bool Create(HINSTANCE instance, int width, int height, const wchar_t* title);
  HWND GetHandle() const { return hwnd_; }

private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  HWND hwnd_ = nullptr;
};