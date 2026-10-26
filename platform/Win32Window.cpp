#include "platform/LocalizedText.h"
#include "platform/Win32Window.h"
#include "platform/GameCursor.h"
#include <windowsx.h>
#include <algorithm>

bool Win32Window::SetMouseLook(bool enabled) {
  if (!enabled) { EndMouseLook(); return true; }
  if (GetForegroundWindow()!=hwnd_ || IsIconic(hwnd_) || !IsWindowVisible(hwnd_) ||
      !(GetAsyncKeyState(VK_RBUTTON)&0x8000)) {
    EndMouseLook(true); return false;
  }
  if (mouseLook_) return true;
  if (!rightPress_) return false;
  POINT client=rightAnchor_; RECT bounds{};
  if (!ScreenToClient(hwnd_,&client) || !GetClientRect(hwnd_,&bounds) || !PtInRect(&bounds,client)) return false;
  const RAWINPUTDEVICE mouse{0x01,0x02,0,hwnd_}; // Foreground only; retain UI button messages.
  if (!RegisterRawInputDevices(&mouse,1,sizeof(mouse))) return false;
  mouseMotion_.Reset();
  mouseLook_=true;
  SetCapture(hwnd_);
  const RECT pin{rightAnchor_.x,rightAnchor_.y,rightAnchor_.x+1,rightAnchor_.y+1};
  if (GetCapture()!=hwnd_ || !ClipCursor(&pin)) { EndMouseLook(true); return false; }
  SetCursor(nullptr);
  return true;
}

void Win32Window::EndMouseLook(bool cancelGesture) {
  rightPress_=false;
  mouseMotion_.Reset();
  // UI gestures also need cancellation when no camera capture is active.
  // Keep the notification latched if focus leaves and returns in one pump.
  inputCancelled_|=cancelGesture;
  if (cancelGesture) wheel_=0;
  if (!mouseLook_) return;
  mouseLook_=false; // ReleaseCapture sends WM_CAPTURECHANGED synchronously.
  ClipCursor(nullptr);
  if (GetCapture()==hwnd_) ReleaseCapture();
  const RAWINPUTDEVICE mouse{0x01,0x02,RIDEV_REMOVE,nullptr};
  RegisterRawInputDevices(&mouse,1,sizeof(mouse));
  SetCursor(GetForegroundWindow()==hwnd_ && cursor_?cursor_:LoadCursor(nullptr,IDC_ARROW));
}

void Win32Window::RefreshCursor(UINT dpi) {
  if (auto next=CreateGameCursor(dpi)) {
    const auto previous=cursor_; cursor_=next;
    if (previous && GetCursor()==previous) SetCursor(next);
    if (previous) DestroyCursor(previous);
  }
}

LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCT*>(lparam);
    window = static_cast<Win32Window*>(create->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
  }
  switch (msg) {
  case WM_PAINT:
    if(window && window->loading_) {
      PAINTSTRUCT paint{};const HDC dc=BeginPaint(hwnd,&paint);RECT bounds{};GetClientRect(hwnd,&bounds);
      const auto background=CreateSolidBrush(RGB(20,17,13));FillRect(dc,&bounds,background);DeleteObject(background);
      const int margin=std::max(24,int(bounds.right-bounds.left)/12);
      RECT frame{margin,margin,bounds.right-margin,bounds.bottom-margin};
      const auto bronze=CreateSolidBrush(RGB(132,101,54));FrameRect(dc,&frame,bronze);
      InflateRect(&frame,-5,-5);FrameRect(dc,&frame,bronze);DeleteObject(bronze);
      SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(233,208,151));
      const auto font=CreateFontW(-std::max(24,int(bounds.bottom)/18),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_ROMAN,L"Georgia");
      const auto previous=SelectObject(dc,font);
      RECT title{margin,bounds.bottom/2-60,bounds.right-margin,bounds.bottom/2};
      DrawTextW(dc,localization::Wide(L"MEDITERRANEAN").c_str(),-1,&title,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
      SelectObject(dc,previous);DeleteObject(font);
      title.top=bounds.bottom/2+12;title.bottom=title.top+36;
      DrawTextW(dc,localization::Wide(L"Opening the gates of Dawnwatch...").c_str(),-1,&title,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
      EndPaint(hwnd,&paint);return 0;
    }
    break;
  case WM_CHAR:
    if(window && GetForegroundWindow()==hwnd && wparam>=32 && wparam<0xd800 && window->text_.size()<256) {
      const wchar_t character=static_cast<wchar_t>(wparam);char bytes[4];
      const int count=WideCharToMultiByte(CP_UTF8,0,&character,1,bytes,4,nullptr,nullptr);
      if(count>0) window->text_.append(bytes,count);
    }
    return 0;
  case WM_RBUTTONDOWN:
    if (window) {
      window->rightAnchor_={GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam)};
      window->rightPress_=ClientToScreen(hwnd,&window->rightAnchor_)!=FALSE;
    }
    return 0;
  case WM_RBUTTONUP:
    if (window) window->EndMouseLook();
    return 0;
  case WM_INPUT: {
    if (window && window->mouseLook_ && GetForegroundWindow()==hwnd) {
      RAWINPUT raw{}; UINT size=sizeof(raw);
      const UINT read=GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam),RID_INPUT,&raw,&size,sizeof(RAWINPUTHEADER));
      if (read!=UINT(-1) && read>=sizeof(RAWINPUTHEADER)+sizeof(RAWMOUSE) && raw.header.dwType==RIM_TYPEMOUSE) {
        const bool multi=(raw.data.mouse.usFlags&MOUSE_VIRTUAL_DESKTOP)!=0;
        window->mouseMotion_.Add(raw.data.mouse,raw.header.hDevice,
          GetSystemMetrics(multi?SM_CXVIRTUALSCREEN:SM_CXSCREEN),GetSystemMetrics(multi?SM_CYVIRTUALSCREEN:SM_CYSCREEN));
      }
    }
    // Foreground WM_INPUT requires default processing for system cleanup.
    return DefWindowProc(hwnd,msg,wparam,lparam);
  }
  case WM_ACTIVATEAPP:
    if (window && !wparam) window->EndMouseLook(true);
    break;
  case WM_CAPTURECHANGED:
    // Our normal ReleaseCapture must not synthesize an interruption after
    // EndMouseLook has already cleared mouseLook_.
    if (window && window->mouseLook_ && reinterpret_cast<HWND>(lparam)!=hwnd) window->EndMouseLook(true);
    break;
  case WM_KILLFOCUS:
    if(window) window->text_.clear();
  case WM_CANCELMODE:
  case WM_ENTERSIZEMOVE:
  case WM_ENTERMENULOOP:
  case WM_SIZE:
  case WM_MOVE:
    if (window) window->EndMouseLook(true);
    break;
  case WM_SETCURSOR:
    if (window && window->mouseLook_) { SetCursor(nullptr); return TRUE; }
    if (window && LOWORD(lparam)==HTCLIENT) {
      SetCursor(window->cursor_?window->cursor_:LoadCursor(nullptr,IDC_ARROW));
      return TRUE;
    }
    return DefWindowProc(hwnd,msg,wparam,lparam);
  case WM_DPICHANGED: {
    if (window) window->EndMouseLook(true);
    if (window) window->RefreshCursor(HIWORD(wparam));
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
    if (window) window->EndMouseLook(true);
    PostQuitMessage(0);
    return 0;
  case WM_CLOSE:
    if (window) window->EndMouseLook(true);
    DestroyWindow(hwnd);
    return 0;
  default:
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
  return DefWindowProc(hwnd,msg,wparam,lparam);
}

bool Win32Window::Create(HINSTANCE instance, int width, int height, const wchar_t* title, bool visible, bool fullscreen) {
  if(width<=0 || height<=0) return false;
  visible_=visible;
  const wchar_t* className = L"RPGNativeWindow";

  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpfnWndProc = Win32Window::WndProc;
  wc.hInstance = instance;
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

  if (!RegisterClassEx(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) {
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

  // Fit the requested client size to the work area, retaining its aspect ratio.
  // This keeps windowed launch options reachable on smaller/scaled monitors.
  MONITORINFO monitor{sizeof(monitor)};
  RECT frame{0,0,width,height};
  if(!GetMonitorInfoW(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&monitor) ||
    !AdjustWindowRectExForDpi(&frame,WS_OVERLAPPEDWINDOW,FALSE,0,GetDpiForWindow(hwnd_))) return false;
  const int borderX=frame.right-frame.left-width,borderY=frame.bottom-frame.top-height;
  const int availableWidth=monitor.rcWork.right-monitor.rcWork.left,availableHeight=monitor.rcWork.bottom-monitor.rcWork.top;
  const double fit=std::min({1.,double(availableWidth-borderX)/width,double(availableHeight-borderY)/height});
  if(fit<=0) return false;
  const int outerWidth=int(width*fit)+borderX,outerHeight=int(height*fit)+borderY;
  if(!SetWindowPos(hwnd_,nullptr,monitor.rcWork.left+(availableWidth-outerWidth)/2,monitor.rcWork.top+(availableHeight-outerHeight)/2,
    outerWidth,outerHeight,SWP_NOZORDER|SWP_NOACTIVATE)) return false;

  RefreshCursor(GetDpiForWindow(hwnd_));

  if (fullscreen && !SetFullscreen(true)) return false;
  if (visible) {
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
  }
  return true;
}

bool Win32Window::SetFullscreen(bool enabled) {
  if (enabled==fullscreen_) return true;
  EndMouseLook(true);
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
