#include "launcher/LaunchSupport.h"
#include "game/UiSkin.h"
#include <gdiplus.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace {
using namespace Gdiplus;
constexpr int Play=1001,Display=1002,Size=1003,Guide=1005,Saves=1006;
constexpr float canvasWidth=940,canvasHeight=590;
const Color background(255,24,24,18),panel(255,29,25,22),gold(255,211,179,111),ink(255,242,228,201),muted(255,191,177,153);
void Text(Graphics& g,const wchar_t* text,float x,float y,float width,float height,float size,Color color,const wchar_t* family=L"Segoe UI",INT style=FontStyleRegular) {
  Font font(family,size,style,UnitPixel);SolidBrush brush(color);StringFormat format;
  format.SetTrimming(StringTrimmingEllipsisCharacter);
  g.DrawString(text,-1,&font,RectF(x,y,width,height),&format,&brush);
}
class Launcher {
public:
  ~Launcher() {if(window_ && IsWindow(window_)) DestroyWindow(window_);if(font_) DeleteObject(font_);if(brush_) DeleteObject(brush_);}
  bool Create(HINSTANCE instance,bool visible) {
    directory_=launch::Directory();if(directory_.empty()) return false;
    game_=directory_/L"RPGNative.exe";settings_=directory_/L"launcher-settings.ini";
    gameName_=launch::InstanceName(directory_,L"game");
    if(!launch::LoadSettings(settings_,options_)) status_=L"Saved launch settings could not be read. Defaults are selected.";
    brush_=CreateSolidBrush(RGB(29,25,22));
    WNDCLASSEXW wc{sizeof(wc)};wc.lpfnWndProc=Procedure;wc.hInstance=instance;wc.lpszClassName=L"MediterraneanLauncherWindow";
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);
    if(!RegisterClassExW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;
    window_=CreateWindowExW(WS_EX_CONTROLPARENT,wc.lpszClassName,L"Mediterranean Launcher",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
      CW_USEDEFAULT,CW_USEDEFAULT,940,590,nullptr,nullptr,instance,this);
    if(!window_) return false;
    const auto make=[&](const wchar_t* kind,const wchar_t* label,int id,DWORD style) {
      const auto control=CreateWindowExW(0,kind,label,WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,0,0,1,1,window_,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);
      return control;
    };
    display_=make(L"COMBOBOX",L"Display mode",Display,CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL);
    size_=make(L"COMBOBOX",L"Window size",Size,CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL);
    play_=make(L"BUTTON",L"&Play",Play,BS_OWNERDRAW);
    guide_=make(L"BUTTON",L"Game &guide",Guide,BS_OWNERDRAW);
    saves_=make(L"BUTTON",L"&Saves folder",Saves,BS_OWNERDRAW);
    if(!display_ || !size_ || !play_ || !guide_ || !saves_) return false;
    for(auto control:{display_,size_}) {
      SetWindowTheme(control,L"",L"");
      if(!SetWindowSubclass(control,ComboProcedure,1,reinterpret_cast<DWORD_PTR>(this))) return false;
    }
    for(const wchar_t* item:{L"Borderless fullscreen",L"Windowed"}) SendMessageW(display_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item));
    for(const auto& resolution:launch::resolutions) SendMessageW(size_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(resolution.label));
    SendMessageW(display_,CB_SETCURSEL,options_.fullscreen?0:1,0);SendMessageW(size_,CB_SETCURSEL,options_.resolution,0);
    options_.resume=false; // Character selection owns the saved-journey choice.
    const auto load=[&](unsigned id,auto& stream,auto& bitmap) {
      const auto resource=FindResourceW(instance,MAKEINTRESOURCEW(id),RT_RCDATA);
      const auto data=resource?LoadResource(instance,resource):nullptr;
      if(!data) return false;
      stream.reset(SHCreateMemStream(static_cast<const BYTE*>(LockResource(data)),SizeofResource(instance,resource)));
      if(!stream) return false;
      bitmap=std::make_unique<Bitmap>(stream.get());return bitmap->GetLastStatus()==Ok;
    };
    if(!load(202,stream_,art_) || !load(201,frameStream_,frame_)) return false;
    MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window_,MONITOR_DEFAULTTONEAREST),&monitor);
    const float scale=std::min({GetDpiForWindow(window_)/96.f,(monitor.rcWork.right-monitor.rcWork.left-24)/canvasWidth,(monitor.rcWork.bottom-monitor.rcWork.top-64)/canvasHeight});
    Resize(std::max(.65f,scale),true);Refresh();SetTimer(window_,1,500,nullptr);SetTimer(window_,2,33,nullptr);
    if(visible) {ShowWindow(window_,SW_SHOW);UpdateWindow(window_);SetFocus(play_);}
    return true;
  }
  int Run() {
    MSG message{};BOOL result;
    while((result=GetMessageW(&message,nullptr,0,0))>0) {
      if(!IsDialogMessageW(window_,&message)) {TranslateMessage(&message);DispatchMessageW(&message);}
    }
    return result==-1?1:int(message.wParam);
  }
  bool SelfTest(const std::filesystem::path& output) {
    if(!art_) return false;
    std::error_code error;std::filesystem::create_directories(output,error);if(error) return false;
    for(unsigned n=0;n<3;++n) {
      Resize(1+n*.5f,false);
      SendMessageW(display_,CB_SETCURSEL,n==1?1:0,0);
      SendMessageW(window_,WM_COMMAND,MAKEWPARAM(Display,CBN_SELCHANGE),reinterpret_cast<LPARAM>(display_));
      if(bool(IsWindowEnabled(size_))!=(n==1)) return false;
      if(SendMessageW(display_,CB_GETCOUNT,0,0)!=2 || SendMessageW(size_,CB_GETCOUNT,0,0)!=3) return false;
      if(n==1 && GetNextDlgTabItem(window_,display_,FALSE)!=size_) return false;
      if(LOWORD(SendMessageW(window_,DM_GETDEFID,0,0))!=Play) return false;
      if(!Capture(output/(L"launcher-"+std::to_wstring(100+n*50)+L".png"))) return false;
      for(auto control:{display_,size_,play_,guide_,saves_}) {
        RECT bounds{};GetWindowRect(control,&bounds);MapWindowPoints(nullptr,window_,reinterpret_cast<POINT*>(&bounds),2);
        RECT client{};GetClientRect(window_,&client);
        if(bounds.left<0 || bounds.top<0 || bounds.right>client.right || bounds.bottom>client.bottom) return false;
      }
    }
    Resize(1,false);const auto original=game_;game_=directory_/L"missing-game-for-launcher-check.exe";Refresh();
    if(IsWindowEnabled(play_) || !Capture(output/L"launcher-missing.png")) return false;
    game_=original;Refresh();return true;
  }
private:
  struct ReleaseStream {void operator()(IStream* value) const {if(value) value->Release();}};
  HWND window_=nullptr,display_=nullptr,size_=nullptr,play_=nullptr,guide_=nullptr,saves_=nullptr;
  HFONT font_=nullptr;HBRUSH brush_=nullptr;
  std::unique_ptr<IStream,ReleaseStream> stream_,frameStream_;std::unique_ptr<Bitmap> art_,frame_;
  std::filesystem::path directory_,game_,settings_;
  launch::Options options_;launch::Process process_;
  std::wstring gameName_,status_=L"Ready to enter Dawnwatch.";
  bool busy_=false,available_=false,saveAvailable_=false;float scale_=1;
  std::array<float,3> highlights_{};
  void AnimateControls() {
    if(!IsWindowVisible(window_) || IsIconic(window_)) return;
    POINT cursor{};GetCursorPos(&cursor);
    const HWND controls[]{play_,guide_,saves_};
    for(int i=0;i<3;++i) {
      RECT rect{};GetWindowRect(controls[i],&rect);
      const bool active=GetForegroundWindow()==window_ && IsWindowEnabled(controls[i]) &&
        (PtInRect(&rect,cursor) || GetFocus()==controls[i]);
      const float next=highlights_[i]+((active?1.f:0.f)-highlights_[i])*.35f;
      if(std::abs(next-highlights_[i])>.001f) {highlights_[i]=next;InvalidateRect(controls[i],nullptr,FALSE);}
    }
  }
  void Resize(float scale,bool centre) {
    RECT bounds{0,0,LONG(std::lround(canvasWidth*scale)),LONG(std::lround(canvasHeight*scale))};
    AdjustWindowRectExForDpi(&bounds,DWORD(GetWindowLongPtrW(window_,GWL_STYLE)),FALSE,WS_EX_CONTROLPARENT,GetDpiForWindow(window_));
    int x=0,y=0;UINT flags=SWP_NOZORDER|SWP_NOACTIVATE;
    if(centre) {MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window_,MONITOR_DEFAULTTONEAREST),&monitor);
      x=monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-(bounds.right-bounds.left))/2;
      y=monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-(bounds.bottom-bounds.top))/2;
    } else flags|=SWP_NOMOVE;
    SetWindowPos(window_,nullptr,x,y,bounds.right-bounds.left,bounds.bottom-bounds.top,flags);
    Layout();
  }
  void Layout() {
    RECT client{};GetClientRect(window_,&client);scale_=client.right/canvasWidth;
    if(!display_) return;
    const auto next=CreateFontW(-int(std::lround(15*scale_)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    for(auto control:{display_,size_,play_,guide_,saves_}) SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(next),FALSE);
    for(auto control:{display_,size_}) {SendMessageW(control,CB_SETITEMHEIGHT,WPARAM(-1),LPARAM(26*scale_));SendMessageW(control,CB_SETITEMHEIGHT,0,LPARAM(28*scale_));}
    if(font_) DeleteObject(font_);font_=next;
    const auto place=[&](HWND control,int x,int y,int w,int h) {MoveWindow(control,int(x*scale_),int(y*scale_),int(w*scale_),int(h*scale_),TRUE);};
    place(display_,636,210,252,180);place(size_,636,283,252,180);
    place(play_,636,410,252,58);place(guide_,636,480,122,32);place(saves_,766,480,122,32);
    InvalidateRect(window_,nullptr,FALSE);
  }
  void Changed() {
    options_.fullscreen=SendMessageW(display_,CB_GETCURSEL,0,0)==0;
    const auto size=SendMessageW(size_,CB_GETCURSEL,0,0);if(size>=0 && size<int(launch::resolutions.size())) options_.resolution=unsigned(size);
    EnableWindow(size_,!busy_ && !options_.fullscreen);InvalidateRect(window_,nullptr,FALSE);
  }
  void Refresh() {
    DWORD exitCode=0;
    const bool exited=process_.Poll(exitCode);
    if(exited) {
      status_=exitCode==0?L"Journey ended. Ready when you are.":L"The game exited with an error ("+std::to_wstring(exitCode)+L"). See the game guide for troubleshooting.";
      if(IsIconic(window_)) ShowWindow(window_,SW_SHOWNOACTIVATE);
    }
    const bool wasBusy=busy_,wasAvailable=available_,hadSave=saveAvailable_;
    busy_=process_.Active() || launch::IsRunning(gameName_);
    if(!wasBusy && busy_ && !process_.Active()) status_=L"The game is already open from this folder. Return here after it closes.";
    if(wasBusy && !busy_ && !exited) status_=L"Journey ended. Ready when you are.";
    std::error_code error;available_=std::filesystem::is_regular_file(game_,error);
    saveAvailable_=std::filesystem::is_regular_file(launch::SavePath(),error);
    EnableWindow(play_,available_ && !busy_);EnableWindow(display_,!busy_);EnableWindow(size_,!busy_ && !options_.fullscreen);
    if(wasBusy!=busy_ || wasAvailable!=available_ || hadSave!=saveAvailable_ || exitCode) InvalidateRect(window_,nullptr,FALSE);
  }
  void Start() {
    Refresh();if(!available_ || busy_) return;Changed();
    const bool saved=launch::SaveSettings(settings_,options_);
    std::wstring error;
    if(!process_.Start(game_,launch::Arguments(options_),error)) {status_=error;InvalidateRect(window_,nullptr,FALSE);return;}
    status_=saved?L"Game started. You can leave this launcher open or close it.":L"Game started, but launch preferences could not be saved.";
    Refresh();InvalidateRect(window_,nullptr,FALSE);ShowWindow(window_,SW_MINIMIZE);
  }
  void Open(const std::filesystem::path& path,bool folder) {
    std::error_code error;if(folder) std::filesystem::create_directories(path,error);
    if(error || reinterpret_cast<INT_PTR>(ShellExecuteW(window_,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32) {
      status_=folder?L"Could not open the saves folder.":L"Could not open the guide. Read START_AND_DEBUG.md in the project folder.";InvalidateRect(window_,nullptr,FALSE);
    }
  }
  void Skin(Graphics& g,RectF box,float corner,bool center=true) {
    if(!frame_) return;
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    ImageAttributes attributes;attributes.SetWrapMode(WrapModeTileFlipXY);
    ui::skin::Panel(box.X,box.Y,box.Width,box.Height,corner,[&](float x,float y,float w,float h,float u,float v,float uu,float vv) {
      g.DrawImage(frame_.get(),RectF(x,y,w,h),u*frame_->GetWidth(),v*frame_->GetHeight(),
        (uu-u)*frame_->GetWidth(),(vv-v)*frame_->GetHeight(),UnitPixel,&attributes);
    },center);
  }
  void Paint(HDC dc) {
    Graphics g(dc);g.ScaleTransform(scale_,scale_);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    SolidBrush base(background);g.FillRectangle(&base,0.f,0.f,canvasWidth,canvasHeight);
    if(art_) {g.SetInterpolationMode(InterpolationModeHighQualityBicubic);g.DrawImage(art_.get(),RectF(10,10,920,570));}
    LinearGradientBrush shade(RectF(10,10,920,150),Color(240,10,9,8),Color(0,10,9,8),LinearGradientModeVertical);
    shade.SetWrapMode(WrapModeTileFlipXY);g.FillRectangle(&shade,10,10,920,150);
    Skin(g,RectF(9,9,922,572),42,false);
    Text(g,L"MEDITERRANEAN",45,29,650,45,34,gold,L"Georgia");
    Text(g,L"T H E   D A W N W A T C H   C H R O N I C L E",47,76,530,22,11,ink,L"Georgia");
    Skin(g,RectF(610,118,298,406),30);
    Skin(g,RectF(636,102,247,42),19);
    Text(g,L"Prepare your journey",654,113,218,27,17,ink,L"Georgia");
    Text(g,L"I.   DISPLAY",636,180,250,24,12,gold,L"Georgia");
    Text(g,L"II.  WINDOW SIZE",636,256,250,24,12,gold,L"Georgia");
    Text(g,L"THE HERO'S CHRONICLE",636,337,255,22,12,gold,L"Georgia");
    Text(g,saveAvailable_?L"Your saved heroes await in the hall.\nChoose a character after launching.":L"Eleven callings await in the hall.\nCreate a hero after launching.",636,369,249,35,12,ink);
    Skin(g,RectF(42,390,540,132),28);
    Text(g,L"DAWNWATCH",64,408,480,42,29,gold,L"Georgia");
    Text(g,L"At the edge of the known world, your chronicle begins.",66,453,490,26,15,ink,L"Georgia");
    Text(g,L"Eleven classes  /  Twelve assignments  /  Ten levels",66,489,490,24,12,muted);
    Skin(g,RectF(29,537,882,33),12);
    const wchar_t* state=!available_?L"BUILD MISSING":busy_?L"GAME RUNNING":L"READY TO PLAY";
    const auto stateColor=!available_?Color(255,246,157,117):busy_?gold:Color(255,179,210,156);
    Text(g,state,45,547,158,19,11,stateColor,L"Segoe UI",FontStyleBold);
    const auto status=!available_?L"Build RPGNative.exe beside MediterraneanLauncher.exe, then try again.":status_.c_str();
    Text(g,status,216,546,672,21,11,ink);
    Text(g,L"LOCAL CAMPAIGN",735,57,165,22,11,gold,L"Georgia");
  }
  void ComboChrome(HWND control,HDC dc) {
    COMBOBOXINFO info{sizeof(info)};RECT client{};
    if(!GetComboBoxInfo(control,&info) || !GetClientRect(control,&client)) return;
    Graphics g(dc);g.SetSmoothingMode(SmoothingModeAntiAlias);
    const bool enabled=IsWindowEnabled(control)!=FALSE;
    Pen border(enabled?gold:muted,std::max(1.f,scale_));
    g.DrawRectangle(&border,scale_*.5f,scale_*.5f,client.right-scale_,client.bottom-scale_);
    const RectF arrow(float(info.rcButton.left),float(info.rcButton.top),float(info.rcButton.right-info.rcButton.left),float(info.rcButton.bottom-info.rcButton.top));
    Skin(g,arrow,6*scale_);
    const float x=arrow.X+arrow.Width*.5f,y=arrow.Y+arrow.Height*.5f;
    PointF points[]={{x-4*scale_,y-2*scale_},{x+4*scale_,y-2*scale_},{x,y+3*scale_}};
    SolidBrush goldBrush(enabled?gold:muted);g.FillPolygon(&goldBrush,points,3);
  }
  static LRESULT CALLBACK ComboProcedure(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data) {
    auto* self=reinterpret_cast<Launcher*>(data);
    const auto result=DefSubclassProc(control,message,wparam,lparam);
    if(message==WM_PAINT) {
      const auto dc=GetDC(control);if(dc){self->ComboChrome(control,dc);ReleaseDC(control,dc);}
    } else if(message==WM_PRINT || message==WM_PRINTCLIENT) self->ComboChrome(control,reinterpret_cast<HDC>(wparam));
    else if(message==WM_NCDESTROY) RemoveWindowSubclass(control,ComboProcedure,id);
    return result;
  }
  void DrawButton(const DRAWITEMSTRUCT& item) {
    Graphics g(item.hDC);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    if(item.CtlType==ODT_COMBOBOX) {
      SolidBrush fill(item.itemState&ODS_SELECTED?Color(255,85,55,31):panel);
      g.FillRectangle(&fill,item.rcItem.left,item.rcItem.top,item.rcItem.right-item.rcItem.left,item.rcItem.bottom-item.rcItem.top);
      wchar_t label[96]{};
      if(item.itemID!=UINT(-1)) SendMessageW(item.hwndItem,CB_GETLBTEXT,item.itemID,reinterpret_cast<LPARAM>(label));
      Font font(L"Segoe UI",14*scale_,FontStyleRegular,UnitPixel);SolidBrush foreground(IsWindowEnabled(item.hwndItem)?ink:muted);
      StringFormat format;format.SetLineAlignment(StringAlignmentCenter);format.SetTrimming(StringTrimmingEllipsisCharacter);
      g.DrawString(label,-1,&font,RectF(item.rcItem.left+7*scale_,float(item.rcItem.top),item.rcItem.right-item.rcItem.left-10*scale_,float(item.rcItem.bottom-item.rcItem.top)),&format,&foreground);
      if(item.itemState&ODS_FOCUS) {RECT focus=item.rcItem;InflateRect(&focus,-2,-2);DrawFocusRect(item.hDC,&focus);}
      return;
    }
    const bool primary=item.CtlID==Play,disabled=(item.itemState&ODS_DISABLED)!=0,pressed=(item.itemState&ODS_SELECTED)!=0;
    const RectF box(float(item.rcItem.left),float(item.rcItem.top),float(item.rcItem.right-item.rcItem.left),float(item.rcItem.bottom-item.rcItem.top));
    Skin(g,box,std::min(18.f*scale_,box.Height*.32f));
    SolidBrush wash(disabled?Color(150,18,17,16):primary?Color(pressed?180:155,111,12,19):Color(pressed?125:65,47,54,47));
    g.FillRectangle(&wash,box.X+5*scale_,box.Y+5*scale_,box.Width-10*scale_,box.Height-10*scale_);
    const float hover=highlights_[primary?0:item.CtlID==Guide?1:2];
    SolidBrush glint(Color(BYTE(disabled?0:42*hover),245,201,117));
    g.FillRectangle(&glint,box.X+5*scale_,box.Y+5*scale_,box.Width-10*scale_,box.Height-10*scale_);
    const wchar_t* label=primary?(busy_?L"GAME RUNNING":L"PLAY"):item.CtlID==Guide?L"Game guide":L"Saves folder";
    Font font(primary?L"Georgia":L"Segoe UI",(primary?22:12)*scale_,FontStyleRegular,UnitPixel);SolidBrush foreground(disabled?muted:ink);StringFormat format;
    format.SetAlignment(StringAlignmentCenter);format.SetLineAlignment(StringAlignmentCenter);g.DrawString(label,-1,&font,box,&format,&foreground);
    if(item.itemState&ODS_FOCUS) {RECT focus=item.rcItem;InflateRect(&focus,-4,-4);DrawFocusRect(item.hDC,&focus);}
  }
  bool Capture(const std::filesystem::path& path) {
    RECT client{};GetClientRect(window_,&client);HDC dc=GetDC(window_),memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,client.right,client.bottom);
    if(!memory || !bitmap) {if(memory) DeleteDC(memory);if(bitmap) DeleteObject(bitmap);ReleaseDC(window_,dc);return false;}
    const auto previous=SelectObject(memory,bitmap);Paint(memory);
    for(auto control:{display_,size_,play_,guide_,saves_}) {
      RECT bounds{};GetWindowRect(control,&bounds);MapWindowPoints(nullptr,window_,reinterpret_cast<POINT*>(&bounds),2);
      const int saved=SaveDC(memory);SetViewportOrgEx(memory,bounds.left,bounds.top,nullptr);
      IntersectClipRect(memory,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
      SendMessageW(control,WM_PRINT,reinterpret_cast<WPARAM>(memory),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
      if(control==display_ || control==size_) {
        // Hidden combo boxes omit their owner-drawn selection in WM_PRINT.
        // Render the same selection callback used by the visible control.
        COMBOBOXINFO info{sizeof(info)};
        if(!GetComboBoxInfo(control,&info)) {RestoreDC(memory,saved);continue;}
        DRAWITEMSTRUCT selection{};selection.CtlType=ODT_COMBOBOX;selection.CtlID=GetDlgCtrlID(control);
        selection.itemID=UINT(SendMessageW(control,CB_GETCURSEL,0,0));selection.itemAction=ODA_DRAWENTIRE;
        selection.itemState=IsWindowEnabled(control)?0:ODS_DISABLED;selection.hwndItem=control;selection.hDC=memory;selection.rcItem=info.rcItem;
        DrawButton(selection);
      }
      RestoreDC(memory,saved);
    }
    const bool painted=GetPixel(memory,2,client.bottom-2)==RGB(background.GetR(),background.GetG(),background.GetB());
    const CLSID png{0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
    bool saved=false;
    {Bitmap image(bitmap,nullptr);saved=painted && image.Save(path.c_str(),&png)==Ok;}
    SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window_,dc);return saved;
  }
  static LRESULT CALLBACK Procedure(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    auto* self=reinterpret_cast<Launcher*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE) {self=static_cast<Launcher*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);self->window_=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(!self) return DefWindowProcW(window,message,wparam,lparam);
    switch(message) {
      case WM_PAINT: {PAINTSTRUCT paint{};const auto dc=BeginPaint(window,&paint);self->Paint(dc);EndPaint(window,&paint);return 0;}
      case WM_PRINTCLIENT: self->Paint(reinterpret_cast<HDC>(wparam));return 0;
      case WM_ERASEBKGND: return 1;
      case WM_SIZE: if(wparam!=SIZE_MINIMIZED) self->Layout();return 0;
      case WM_DPICHANGED: {const auto* rect=reinterpret_cast<RECT*>(lparam);SetWindowPos(window,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
      case WM_TIMER: if(wparam==2) self->AnimateControls();else self->Refresh();return 0;
      case DM_GETDEFID: return MAKELRESULT(Play,DC_HASDEFID);
      case WM_MEASUREITEM: reinterpret_cast<MEASUREITEMSTRUCT*>(lparam)->itemHeight=UINT(28*self->scale_);return TRUE;
      case WM_DRAWITEM: self->DrawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lparam));return TRUE;
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORBTN: SetTextColor(reinterpret_cast<HDC>(wparam),RGB(242,228,201));SetBkColor(reinterpret_cast<HDC>(wparam),RGB(29,25,22));return reinterpret_cast<LRESULT>(self->brush_);
      case WM_COMMAND:
        switch(LOWORD(wparam)) {
          case Play: case IDOK: self->Start();break;
          case IDCANCEL: SendMessageW(window,WM_CLOSE,0,0);break;
          case Display: case Size: self->Changed();break;
          case Guide: self->Open(launch::GuidePath(),false);break;
          case Saves: self->Open(launch::SavePath().parent_path(),true);break;
        } return 0;
      case WM_CLOSE: self->Changed();launch::SaveSettings(self->settings_,self->options_);DestroyWindow(window);return 0;
      case WM_DESTROY: KillTimer(window,1);KillTimer(window,2);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,wparam,lparam);
  }
};
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  int count=0;auto** arguments=CommandLineToArgvW(GetCommandLineW(),&count);
  const bool test=arguments && count==3 && std::wstring(arguments[1])==L"--self-test";
  const std::filesystem::path output=test?arguments[2]:L"";
  if(arguments) LocalFree(arguments);
  if(count!=1 && !test) return 2;
  launch::Instance owner(launch::InstanceName(launch::Directory(),L"launcher"));
  if(!owner.Acquired()) {if(!test) MessageBoxW(nullptr,L"The launcher is already open for this game folder.",L"Mediterranean",MB_OK|MB_ICONINFORMATION);return 3;}
  GdiplusStartupInput startup;ULONG_PTR token=0;
  if(GdiplusStartup(&token,&startup,nullptr)!=Ok) return 1;
  int result=1;
  try {Launcher launcher;if(launcher.Create(instance,!test)) result=test?(launcher.SelfTest(output)?0:1):launcher.Run();}
  catch(const std::exception&) {if(!test) MessageBoxW(nullptr,L"The launcher could not start. Check access to the game folder.",L"Mediterranean",MB_OK|MB_ICONERROR);}
  GdiplusShutdown(token);return result;
}
