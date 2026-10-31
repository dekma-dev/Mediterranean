#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <wincodec.h>
#include <limits>

namespace {
void Check(bool ok,const char* message) { if(!ok) {std::cerr<<message<<'\n';std::exit(1);} }
std::vector<unsigned char> Pixels(const std::filesystem::path& path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Decode art capture");
  UINT width=0,height=0;Check(SUCCEEDED(converter->GetSize(&width,&height)),"Read art dimensions");
  std::vector<unsigned char> result(width*height*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,width*4,UINT(result.size()),result.data())),"Read art pixels");return result;
}
}
int main() {
  using namespace DirectX;
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Art COM initialization");
  const auto root=std::filesystem::current_path()/"character-art-check";
  std::filesystem::create_directories(root);
  Win32Window window;
  Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Character art validation",false),"Hidden art window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,720),"Art renderer");
  for(unsigned view=0;view<12;++view) {
    auto outfit=CharacterOutfit::From(rpg::Inventory{},rpg::ClassId::Warrior);
    outfit.helm=view==3;outfit.appearance.skin=view==4?2:0;outfit.appearance.hair=view==5?2:0;
    animation::Sample motion;
    if(view==6) motion.face.blink=1;
    if(view==7) {motion.face.pain=.8f;motion.face.eyeYaw=.18f;}
    if(view==8) {motion.face.yaw=.62f;motion.face.pitch=.245f;}
    if(view==9) {motion.face.yaw=-.62f;motion.face.pitch=-.245f;}
    if(view==10) motion.face.smile=1;
    const XMFLOAT3 eye=view==1?XMFLOAT3{.78f,1.83f,.06f}:view==2?XMFLOAT3{.55f,1.84f,-.7f}:XMFLOAT3{.30f,1.82f,.90f};
    renderer.BeginFrame();renderer.SetCamera(eye,{0,1.73f,0});
    DrawCharacter(renderer,{0,0},0,0,{.40f,.45f,.50f,1},0,0,outfit,motion,1);
    Check(renderer.Capture((root/("portrait-"+std::to_string(view)+".png")).wstring()) && renderer.EndFrame(false),"Portrait color/shadow capture");
  }
  const auto open=Pixels(root/"portrait-0.png"),closed=Pixels(root/"portrait-6.png");
  unsigned changed=0;
  for(std::size_t p=0;p<open.size();p+=4)
    changed+=std::abs(int(open[p])-int(closed[p]))+std::abs(int(open[p+1])-int(closed[p+1]))+std::abs(int(open[p+2])-int(closed[p+2]))>12;
  Check(changed>100 && changed<12000,"Blink must visibly deform the eyelids without moving the whole actor");
  Check(open==Pixels(root/"portrait-11.png"),"Facial state leaked between actors/frames");
  for(unsigned invalid=0;invalid<2;++invalid) {
    renderer.BeginFrame();renderer.SetCamera({0,0,2},{0,0,0});
    const float bad=invalid?std::numeric_limits<float>::quiet_NaN():0;
    renderer.DrawHead(XMMatrixIdentity(),{.72f,.51f,.35f,1},bad,bad);
    Check(renderer.Capture((root/(invalid?"invalid.png":"neutral.png")).wstring()) && renderer.EndFrame(false),"Sanitized face capture");
  }
  Check(Pixels(root/"neutral.png")==Pixels(root/"invalid.png"),"Invalid facial channels must render the neutral head");
  std::cout<<"Blink changed "<<changed<<" pixels; recovery and invalid channels are pixel-identical\n";
  std::cout<<"Native portrait, gear, complexion, hair and expression captures passed\n";
  CoUninitialize();
}
