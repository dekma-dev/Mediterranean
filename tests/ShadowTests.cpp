#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using M=visual::Mesh;
using S=visual::Material;
using Shadow=visual::ShadowMode;
namespace {
int failures=0;
void Require(bool ok,const char* message) {
  if (!ok) { std::cerr<<message<<'\n'; std::exit(1); }
}
void Check(bool ok,const char* message) {
  if (!ok) { std::cerr<<"FAIL: "<<message<<'\n'; ++failures; }
}
struct Image {
  UINT width=0,height=0;
  std::vector<unsigned char> pixels;
  float At(XMFLOAT2 p,int radius=1) const {
    const int cx=int(std::round(p.x*width)),cy=int(std::round(p.y*height));
    Require(cx>radius && cy>radius && cx+radius<int(width) && cy+radius<int(height),"Sample outside image.");
    float value=0;
    for (int y=cy-radius;y<=cy+radius;++y) for (int x=cx-radius;x<=cx+radius;++x) {
      const auto i=(size_t(y)*width+x)*4;
      value+=(pixels[i]+pixels[i+1]+pixels[i+2])/3.f;
    }
    return value/float((radius*2+1)*(radius*2+1));
  }
};
Image Capture(Renderer& renderer,const wchar_t* path,bool present=true) {
  Require(renderer.Capture(path),"Shadow image capture failed.");
  if (present) Require(renderer.EndFrame(false),"Shadow frame presentation failed.");
  Require(renderer.ValidateDebugMessages(),"Shadow rendering emitted a DirectX warning/error.");
  ComPtr<IWICImagingFactory> factory;
  ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame;
  ComPtr<IWICFormatConverter> converter;
  Require(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))),"Image decoder failed.");
  Require(SUCCEEDED(factory->CreateDecoderFromFilename(path,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"PNG read failed.");
  Image image;
  Require(SUCCEEDED(converter->GetSize(&image.width,&image.height)),"PNG dimensions failed.");
  image.pixels.resize(size_t(image.width)*image.height*4);
  Require(SUCCEEDED(converter->CopyPixels(nullptr,image.width*4,UINT(image.pixels.size()),image.pixels.data())),"PNG pixels failed.");
  return image;
}
}
int main() {
  Require(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization failed.");
  Win32Window window;
  Require(window.Create(GetModuleHandle(nullptr),960,720,L"shadow checks",false),"Hidden shadow window failed.");
  Renderer renderer;
  Require(renderer.Initialize(window.GetHandle(),960,720),"Shadow renderer initialization failed.");
  const XMFLOAT4 white{.8f,.8f,.8f,1};
  const auto floor=[&]() {
    renderer.BeginFrame(); renderer.SetCamera({3,4,5},{0,0,0});
    renderer.DrawMesh(M::Box,{0,-.1f,0},{12,.2f,12},white,S::Plain);
  };
  const auto screen=[&](XMFLOAT3 p) {
    XMFLOAT2 result; Require(renderer.Project(p,result),"Shadow probe projection failed."); return result;
  };
  floor();
  const auto postProbe=screen({.95f,0,.76f}),stepProbe=screen({.55f,0,.54f});
  const auto lit=Capture(renderer,L"shadow-lit.png");
  floor(); renderer.DrawMesh(M::Box,{0,1,0},{.5f,2,.5f},white,S::Stone);
  const auto stone=Capture(renderer,L"shadow-stone.png");
  floor(); renderer.DrawMesh(M::Box,{0,1,0},{.5f,2,.5f},white,S::Plain);
  const auto plain=Capture(renderer,L"shadow-plain.png");
  const float stoneDrop=lit.At(postProbe)-stone.At(postProbe),plainDrop=lit.At(postProbe)-plain.At(postProbe);
  std::cout<<"Post shadow, stone/plain: "<<stoneDrop<<" / "<<plainDrop<<'\n';
  Check(stoneDrop>25,"Opaque post must cast a visible shadow.");
  Check(plainDrop>25 && std::abs(plainDrop-stoneDrop)<2,"Changing an opaque object's material to Plain must retain its shadow.");
  floor(); renderer.DrawMesh(M::Box,{0,.1f,0},{1,.2f,1},white,S::Stone);
  const auto step=Capture(renderer,L"shadow-contact.png");
  const float stepDrop=lit.At(stepProbe)-step.At(stepProbe);
  std::cout<<"Low step contact shadow: "<<stepDrop<<'\n';
  Check(stepDrop>20,"A low step must cast a contact shadow instead of losing it to depth bias.");
  floor();
  const auto cleared=Capture(renderer,L"shadow-cleared.png");
  Check(cleared.pixels==lit.pixels,"Removing a caster must clear its previous frame's shadow.");
  floor(); renderer.DrawMarkerBox({0,1,0},{.5f,2,.5f},white);
  const auto marker=Capture(renderer,L"shadow-marker.png");
  Check(std::abs(marker.At(postProbe)-lit.At(postProbe))<2,"Combat markers must not cast a solid shadow.");
  for (float height : {.05f,.1f,.2f}) {
    floor(); renderer.DrawMesh(M::Box,{0,height*.5f,0},{1,height,1},white,S::Stone,0,0,Shadow::None);
    const auto reference=Capture(renderer,L"shadow-low-reference.png");
    floor(); renderer.DrawMesh(M::Box,{0,height*.5f,0},{1,height,1},white,S::Stone);
    const auto contact=Capture(renderer,L"shadow-low-contact.png");
    const auto probe=screen({.51f,0,0});
    const float drop=reference.At(probe,0)-contact.At(probe,0);
    std::cout<<"Low object "<<height<<" shadow contrast: "<<drop<<'\n';
    Check(drop>10,"Low opaque geometry must retain its ground contact shadow.");
  }
  for (float pitch : {0.f,.6f,-.6f,1.1f,XM_PIDIV2}) for (float yaw : {0.f,.7f,2.4f}) {
    const auto transform=XMMatrixRotationX(pitch)*XMMatrixRotationY(yaw);
    XMFLOAT3 eye; XMStoreFloat3(&eye,XMVector3TransformCoord(XMVectorSet(2,7,2,1),transform));
    const auto plane=[&](Shadow casting) {
      renderer.BeginFrame(); renderer.SetCamera(eye,{0,0,0});
      renderer.DrawMesh(M::Box,{0,0,0},{6,.02f,6},white,S::Plain,yaw,pitch,casting);
    };
    plane(Shadow::None); const auto reference=Capture(renderer,L"shadow-slope-reference.png");
    plane(Shadow::Cast); const auto self=Capture(renderer,L"shadow-slope.png");
    float maxDifference=0;
    for (float x : {-1.f,0.f,1.f}) for (float z : {-1.f,0.f,1.f}) {
      XMFLOAT3 p; XMStoreFloat3(&p,XMVector3TransformCoord(XMVectorSet(x,.01f,z,1),transform));
      const auto probe=screen(p);
      maxDifference=std::max(maxDifference,std::abs(reference.At(probe)-self.At(probe)));
    }
    std::cout<<"Sloped receiver "<<pitch<<", "<<yaw<<" self-shadow difference: "<<maxDifference<<'\n';
    Check(maxDifference<3,"An isolated lit plane must not acquire self-shadow stripes.");
  }
  // A high caster lies entirely above the camera frustum, but its shadow lands
  // in front of the camera. Camera culling must not cull the light pass.
  const auto offscreenFloor=[&]() {
    renderer.BeginFrame(); renderer.SetCamera({0,2,-5},{.7f,0,.7f});
    renderer.DrawMesh(M::Box,{0,-.1f,0},{12,.2f,12},white,S::Plain,0,0,Shadow::None);
  };
  offscreenFloor(); const auto offscreenProbe=screen({.7f,0,.76f});
  const auto offscreenLit=Capture(renderer,L"shadow-offscreen-lit.png");
  offscreenFloor();
  for (float x : {-4.5f,-3.5f}) for (float y : {7.5f,8.5f}) for (float z : {-3.5f,-2.5f}) {
    XMFLOAT2 unused;
    Require(!renderer.Project({x,y,z},unused),"Offscreen fixture caster became visible.");
  }
  renderer.DrawMesh(M::Box,{-4,8,-3},{1,1,1},white,S::Stone);
  const auto offscreen=Capture(renderer,L"shadow-offscreen.png");
  Check(offscreenLit.At(offscreenProbe)-offscreen.At(offscreenProbe)>25,"An offscreen caster must still shade a visible receiver.");
  offscreenFloor(); const auto noCasters=Capture(renderer,L"shadow-no-casters.png");
  Check(noCasters.pixels==offscreenLit.pixels,"A frame containing only non-casters must clear all old shadows.");

  // Known physical shadow centers follow the same sun direction as the shader.
  for (const auto size : {std::pair<int,int>{960,720},{1280,720},{960,600}}) {
    Require(renderer.Resize(size.first,size.second),"Shadow resize failed.");
    for (float shift : {-.3f,0.f,.3f}) {
      floor(); const auto probe=screen({shift+.95f,0,.76f});
      const auto reference=Capture(renderer,L"shadow-moving-lit.png");
      floor(); renderer.DrawMesh(M::Box,{shift,1,0},{.5f,2,.5f},white,S::Stone);
      const auto moving=Capture(renderer,L"shadow-moving.png");
      Check(reference.At(probe)-moving.At(probe)>25,"Moving casters must retain aligned shadows after resize.");
    }
  }
  Require(renderer.Resize(960,720),"Restore shadow size failed.");
  // Opening a portrait must preserve world pixels and use its own lighting;
  // the shadowed world origin must not darken the equipment preview.
  const auto portrait=[&](bool caster,const wchar_t* path) {
    floor();
    if (caster) renderer.DrawMesh(M::Box,{-1.18f,2,-.94f},{1,1,1},white,S::Stone);
    const auto before=Capture(renderer,L"shadow-before-portrait.png",false);
    Require(renderer.BeginPortrait(.65f,.1f,.3f,.6f),"Begin shadow portrait failed.");
    renderer.DrawMesh(M::Sphere,{0,1,0},{1,1.5f,1},white,S::Metal);
    Require(renderer.EndPortrait(),"End shadow portrait failed.");
    const auto after=Capture(renderer,path);
    for (UINT y=0;y<after.height;++y) for (UINT x=0;x<after.width;++x) {
      if (x>=624 && x<912 && y>=72 && y<504) continue;
      const auto i=(size_t(y)*after.width+x)*4;
      Require(std::equal(after.pixels.begin()+i,after.pixels.begin()+i+4,before.pixels.begin()+i),"Portrait altered the world outside its viewport.");
    }
    return after;
  };
  const auto portraitLit=portrait(false,L"shadow-portrait-lit.png");
  const auto portraitShadow=portrait(true,L"shadow-portrait.png");
  Check(std::abs(portraitLit.At({.8f,.4f})-portraitShadow.At({.8f,.4f}))<2,"Portrait lighting must be independent of world shadows.");
  floor(); const auto restored=Capture(renderer,L"shadow-restored.png");
  Check(restored.pixels==lit.pixels,"Closing the preview must restore world shadow/viewport state.");
  std::cout<<(failures?"FAILED: ":"PASS: ")<<"GPU shadow contact, materials, slopes, moving/offscreen casters, clearing, resize and preview isolation.\n";
  CoUninitialize(); return failures?1:0;
}
