#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <cmath>
#include <array>

namespace {
void Frame(Renderer& renderer,unsigned index,float yaw,unsigned lod) {
  const auto asset=authored::Asset(index);
  const auto& b=authored::Collection()[index*authored::lodCount].bounds;
  const float height=b.maximum.y-b.minimum.y;
  const float span=std::max({height,b.maximum.x-b.minimum.x,b.maximum.z-b.minimum.z,1.f});
  renderer.BeginFrame();renderer.SetCamera({span*1.0f,height*.8f,-span*1.8f},{0,height*.48f,0});
  renderer.DrawMesh(visual::Mesh::Box,{0,-.06f,0},{span*1.6f,.12f,span*1.6f},{.22f,.25f,.23f,1},visual::Material::Stone);
  renderer.DrawAuthoredAsset(asset,{0,-b.minimum.y,0},{1,1,1},yaw+DirectX::XM_PI,lod);
  renderer.DrawHudRect(0,0,1,.098f,{.025f,.035f,.05f,.94f});
  renderer.DrawText(.02f,.019f,"BLENDER COLLECTION  /  "+std::to_string(index+1)+" OF 13  /  "+authored::Name(asset),{.9f,.82f,.6f,1},1.8f);
  renderer.DrawText(.02f,.059f,"Left/Right: model   A/D: rotate   L: detail   Space: turntable   Esc: close",{.73f,.79f,.8f,1},1.1f);
  renderer.DrawHudRect(0,.946f,1,.054f,{.025f,.035f,.05f,.94f});
  renderer.DrawText(.02f,.963f,"Native DirectX 11 materials and shadows | Static asset review | LOD: "+(lod==3?std::string("automatic"):std::to_string(lod)),{.75f,.8f,.82f,1},1.2f);
}
}
int wmain(int argc,wchar_t** argv) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  const bool capture=argc==3&&std::wstring(argv[1])==L"--capture";
  if(argc!=1&&!capture) {std::cerr<<"Usage: MediterraneanAssetReview [--capture directory]\n";return 2;}
  try {
    Win32Window window;Renderer renderer;
    if(!window.Create(GetModuleHandleW(nullptr),1280,800,L"Mediterranean - Blender asset review",!capture)||
       !renderer.Initialize(window.GetHandle(),1280,800)) {std::wcerr<<renderer.Error();return 1;}
    if(capture) {
      std::filesystem::create_directories(argv[2]);
      for(unsigned i=0;i<authored::assetCount;++i) {
        Frame(renderer,i,.35f,0);
        const auto path=std::filesystem::path(argv[2])/(std::to_wstring(i+1)+L".png");
        if(!renderer.Capture(path.wstring())||!renderer.EndFrame(false)||!renderer.ValidateDebugMessages()) {std::wcerr<<renderer.Error();return 1;}
        std::cout<<"Rendered "<<authored::Name(authored::Asset(i))<<'\n';
      }
      return 0;
    }
    unsigned index=10,lod=3;float yaw=.35f;bool spin=false;
    std::array<bool,256> previous{};auto last=std::chrono::steady_clock::now();
    for(;;) {
      MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {if(msg.message==WM_QUIT)return 0;TranslateMessage(&msg);DispatchMessageW(&msg);}
      const auto now=std::chrono::steady_clock::now();const float dt=std::min(.05f,std::chrono::duration<float>(now-last).count());last=now;
      if(GetForegroundWindow()==window.GetHandle()) {
        const auto pressed=[&](unsigned key) {const bool down=(GetAsyncKeyState(int(key))&0x8000)!=0;const bool edge=down&&!previous[key];previous[key]=down;return edge;};
        if(pressed(VK_ESCAPE))return 0;
        if(pressed(VK_RIGHT))index=(index+1)%authored::assetCount;
        if(pressed(VK_LEFT))index=(index+authored::assetCount-1)%authored::assetCount;
        if(pressed('L'))lod=(lod+1)%4;
        if(pressed(VK_SPACE))spin=!spin;
        if(GetAsyncKeyState('A')&0x8000)yaw-=dt;
        if(GetAsyncKeyState('D')&0x8000)yaw+=dt;
      } else previous.fill(false);
      if(spin)yaw+=dt*.35f;
      RECT client{};GetClientRect(window.GetHandle(),&client);
      if(client.right<=0||client.bottom<=0) {WaitMessage();continue;}
      if((client.right!=renderer.Width()||client.bottom!=renderer.Height())&&!renderer.Resize(client.right,client.bottom))return 1;
      Frame(renderer,index,yaw,lod);if(!renderer.EndFrame())return 1;
    }
  } catch(const std::exception& error) {std::cerr<<error.what();return 1;}
}
