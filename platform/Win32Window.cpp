#include "platform/Win32Window.h"

LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    return 0;
  default:
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

bool Win32Window::Create(HINSTANCE instance, int width, int height, const wchar_t* title) {
  const wchar_t* className = L"RPGNativeWindow";

  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpfnWndProc = Win32Window::WndProc;
  wc.hInstance = instance;
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

  if (!RegisterClassEx(&wc)) {
    return false;
  }

  RECT rect = { 0, 0, width, height };
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  hwnd_ = CreateWindowEx(
    0,
    className,
    title,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    rect.right - rect.left,
    rect.bottom - rect.top,
    nullptr,
    nullptr,
    instance,
    nullptr
  );

  if (!hwnd_) {
    return false;
  }

  ShowWindow(hwnd_, SW_SHOW);
  UpdateWindow(hwnd_);
  return true;
}