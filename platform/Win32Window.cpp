#include "platform/Win32Window.h"

LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCT*>(lparam);
    window = static_cast<Win32Window*>(create->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
  }
  switch (msg) {
  case WM_DPICHANGED: {
    const auto* suggested=reinterpret_cast<const RECT*>(lparam);
    RECT rect=*suggested;
    if (window && window->fullscreen_) {
      MONITORINFO monitor{sizeof(MONITORINFO)};
      if (GetMonitorInfo(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor)) rect=monitor.rcMonitor;
    }
    SetWindowPos(hwnd,nullptr,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,SWP_NOZORDER|SWP_NOACTIVATE);
    return 0;
  }
  case WM_MOUSEWHEEL:
    if (window) window->wheel_ += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
    return 0;
  case WM_GETMINMAXINFO: {
    auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
    info->ptMinTrackSize = {960, 600};
    return 0;
  }
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

bool Win32Window::Create(HINSTANCE instance, int width, int height, const wchar_t* title, bool visible) {
  visible_=visible;
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
    this
  );

  if (!hwnd_) {
    return false;
  }

  if (visible) {
    if (!SetFullscreen(true)) return false;
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
  }
  return true;
}

bool Win32Window::SetFullscreen(bool enabled) {
  if (enabled==fullscreen_) return true;
  if (enabled) {
    MONITORINFO monitor{sizeof(MONITORINFO)};
    if (!GetWindowPlacement(hwnd_,&windowed_) || !GetMonitorInfo(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&monitor)) return false;
    if (!visible_) windowed_.showCmd=SW_HIDE;
    if (visible_ && windowed_.showCmd==SW_HIDE) windowed_.showCmd=SW_SHOWNORMAL;
    SetLastError(0);
    const auto style=(GetWindowLongPtr(hwnd_,GWL_STYLE)&~WS_OVERLAPPEDWINDOW)|WS_POPUP;
    if (!SetWindowLongPtr(hwnd_,GWL_STYLE,style) && GetLastError()) return false;
    if (!SetWindowPos(hwnd_,HWND_NOTOPMOST,monitor.rcMonitor.left,monitor.rcMonitor.top,
      monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top,
      SWP_FRAMECHANGED|SWP_NOACTIVATE)) return false;
  } else {
    SetLastError(0);
    const auto style=(GetWindowLongPtr(hwnd_,GWL_STYLE)&~WS_POPUP)|WS_OVERLAPPEDWINDOW;
    if (!SetWindowLongPtr(hwnd_,GWL_STYLE,style) && GetLastError()) return false;
    if (!SetWindowPlacement(hwnd_,&windowed_) || !SetWindowPos(hwnd_,nullptr,0,0,0,0,
      SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED)) return false;
  }
  fullscreen_=enabled;
  return true;
}
