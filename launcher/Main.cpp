#include "launcher/LaunchSupport.h"
#include <gdiplus.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace {
using namespace Gdiplus;
constexpr int Play=1001,Display=1002,Size=1003,Guide=1005,Saves=1006;
constexpr float canvasWidth=940,canvasHeight=590;
const Color background(255,24,24,18),panel(255,32,38,29),gold(255,211,179,111),ink(255,235,236,225),muted(255,159,178,181);
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
    brush_=CreateSolidBrush(RGB(23,33,39));
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
    for(const wchar_t* item:{L"Borderless fullscreen",L"Windowed"}) SendMessageW(display_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item));
    for(const auto& resolution:launch::resolutions) SendMessageW(size_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(resolution.label));
    SendMessageW(display_,CB_SETCURSEL,options_.fullscreen?0:1,0);SendMessageW(size_,CB_SETCURSEL,options_.resolution,0);
    options_.resume=false; // Character selection owns the saved-journey choice.
    const auto resource=FindResourceW(instance,MAKEINTRESOURCEW(101),RT_RCDATA);
    if(resource) {
      const auto data=LoadResource(instance,resource);
      stream_.reset(SHCreateMemStream(static_cast<const BYTE*>(LockResource(data)),SizeofResource(instance,resource)));
      if(stream_) {art_=std::make_unique<Bitmap>(stream_.get());if(art_->GetLastStatus()!=Ok) art_.reset();}
    }
    MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window_,MONITOR_DEFAULTTONEAREST),&monitor);
    const float scale=std::min({GetDpiForWindow(window_)/96.f,(monitor.rcWork.right-monitor.rcWork.left-24)/canvasWidth,(monitor.rcWork.bottom-monitor.rcWork.top-64)/canvasHeight});
    Resize(std::max(.65f,scale),true);Refresh();SetTimer(window_,1,500,nullptr);
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
  std::unique_ptr<IStream,ReleaseStream> stream_;std::unique_ptr<Bitmap> art_;
  std::filesystem::path directory_,game_,settings_;
  launch::Options options_;launch::Process process_;
  std::wstring gameName_,status_=L"Ready to enter Dawnwatch.";
  bool busy_=false,available_=false,saveAvailable_=false;float scale_=1;
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
  void Paint(HDC dc) {
    Graphics g(dc);g.ScaleTransform(scale_,scale_);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    SolidBrush base(background),card(panel),goldBrush(gold);g.FillRectangle(&base,0.f,0.f,canvasWidth,canvasHeight);
    Pen trim(Color(255,75,79,63),1);g.DrawLine(&trim,24.f,88.f,916.f,88.f);
    // Incised bronze borders and leather grain drawn at logical DPI scale.
    const auto frame=[&](float x,float y,float w,float h) {
      Pen shadow(Color(255,8,12,9),5),metal(Color(255,117,96,53),2),light(Color(255,199,173,105),.7f);
      g.DrawRectangle(&shadow,x,y,w,h);g.DrawRectangle(&metal,x,y,w,h);g.DrawRectangle(&light,x+4,y+4,w-8,h-8);
      for(int n=0;n<42;++n) {
        const float px=x+8+(w-16)*n/42,grain=std::fmod(n*1.731f,1.f);
        g.DrawLine(&light,px,y+2,px+2+grain*4,y+3);g.DrawLine(&light,px,y+h-3,px+2,y+h-2);
      }
      for(float side:{0.f,1.f}) for(float bottom:{0.f,1.f}) {
        const float cx=x+side*w,cy=y+bottom*h,sx=side?-1.f:1.f,sy=bottom?-1.f:1.f;
        for(int n=0;n<3;++n) {
          const float d=10+n*6.f;g.DrawLine(&metal,cx+sx*3,cy+sy*d,cx+sx*d,cy+sy*3);
          g.DrawArc(&light,cx+(side?-25.f:1.f),cy+(bottom?-25.f:1.f),24.f,24.f,side?90.f:0.f,220.f);
        }
        SolidBrush stud(Color(255,154,120,63));g.FillEllipse(&stud,cx-3,cy-3,6.f,6.f);
      }
    };
    for(int n=0;n<220;++n) {
      const float x=std::fmod(n*71.7f,canvasWidth),y=std::fmod(n*43.3f,canvasHeight);
      Pen grain(Color(18,175,145,92),.6f);g.DrawLine(&grain,x,y,x+3+float(n%7),y+1);
    }
    frame(9,9,922,572);
    Text(g,L"MEDITERRANEAN",30,20,650,40,31,gold,L"Georgia");Text(g,L"A S T R A L   F R O N T I E R",32,61,570,19,11,muted);
    Text(g,L"OFFLINE  /  SINGLE PLAYER",696,42,220,20,11,muted);
    g.FillRectangle(&card,24.f,108.f,558.f,404.f);g.FillRectangle(&card,610.f,108.f,306.f,404.f);
    if(art_) {g.SetInterpolationMode(InterpolationModeHighQualityBicubic);g.DrawImage(art_.get(),RectF(24,108,558,314));}
    LinearGradientBrush shade(Point(0,245),Point(0,423),Color(0,10,20,26),Color(245,10,20,26));g.FillRectangle(&shade,24,245,558,178);
    frame(24,108,558,404);frame(610,108,306,404);
    Text(g,L"DAWNWATCH",45,331,510,46,32,ink,L"Georgia");Text(g,L"Your journey begins on the astral frontier.",47,378,495,27,15,ink);
    Text(g,L"ELEVEN CALLINGS. ONE FRONTIER.",43,441,515,24,12,gold,L"Segoe UI",FontStyleBold);
    Text(g,L"Choose your class, master its disciplines and defend Dawnwatch.\nNine assignments. Ten levels. Your own chronicle.",43,472,519,38,13,muted);
    Text(g,L"START YOUR JOURNEY",635,131,255,30,18,ink,L"Georgia");
    Text(g,L"Display mode",636,185,250,22,13,muted);Text(g,L"Window size (fits desktop)",636,258,250,22,13,muted);
    Text(g,L"CHOOSE YOUR CHARACTER",636,337,255,22,12,gold);
    Text(g,saveAvailable_?L"Review your saved hero in the game,\nthen select Enter World.":L"Meet your first hero in the game,\nthen select Enter World.",636,369,255,34,12,muted);
    g.DrawLine(&trim,24.f,532.f,916.f,532.f);
    const wchar_t* state=!available_?L"GAME BUILD MISSING":busy_?L"GAME RUNNING":L"READY";
    const auto stateColor=!available_?Color(255,236,164,136):busy_?gold:Color(255,151,211,175);
    Text(g,state,31,548,175,21,12,stateColor,L"Segoe UI",FontStyleBold);
    const auto status=!available_?L"Build RPGNative.exe beside MediterraneanLauncher.exe, then try again.":status_.c_str();
    Text(g,status,216,545,686,34,12,muted);
  }
  void DrawButton(const DRAWITEMSTRUCT& item) {
    Graphics g(item.hDC);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    if(item.CtlType==ODT_COMBOBOX) {
      SolidBrush fill(item.itemState&ODS_SELECTED?Color(255,54,70,75):panel);
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
    const Color first=disabled?Color(255,48,57,59):primary?Color(255,116,99,54):Color(255,39,52,57);
    const Color last=disabled?Color(255,37,44,47):primary?Color(255,64,62,36):Color(255,26,38,44);
    LinearGradientBrush fill(box,pressed?last:first,pressed?first:last,LinearGradientModeVertical);g.FillRectangle(&fill,box);
    Pen border(disabled?Color(255,72,81,81):primary?gold:Color(255,82,102,105),1);g.DrawRectangle(&border,box.X+.5f,box.Y+.5f,box.Width-1,box.Height-1);
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
      case WM_TIMER: self->Refresh();return 0;
      case DM_GETDEFID: return MAKELRESULT(Play,DC_HASDEFID);
      case WM_MEASUREITEM: reinterpret_cast<MEASUREITEMSTRUCT*>(lparam)->itemHeight=UINT(28*self->scale_);return TRUE;
      case WM_DRAWITEM: self->DrawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lparam));return TRUE;
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORBTN: SetTextColor(reinterpret_cast<HDC>(wparam),RGB(235,236,225));SetBkColor(reinterpret_cast<HDC>(wparam),RGB(23,33,39));return reinterpret_cast<LRESULT>(self->brush_);
      case WM_COMMAND:
        switch(LOWORD(wparam)) {
          case Play: case IDOK: self->Start();break;
          case IDCANCEL: SendMessageW(window,WM_CLOSE,0,0);break;
          case Display: case Size: self->Changed();break;
          case Guide: self->Open(launch::GuidePath(),false);break;
          case Saves: self->Open(launch::SavePath().parent_path(),true);break;
        } return 0;
      case WM_CLOSE: self->Changed();launch::SaveSettings(self->settings_,self->options_);DestroyWindow(window);return 0;
      case WM_DESTROY: KillTimer(window,1);PostQuitMessage(0);return 0;
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
