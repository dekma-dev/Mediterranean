#include "engine/Renderer.h"
#include "engine/Shadow.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <limits>

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
float MaximumDifference(const Image& a,const Image& b) {
  Require(a.width==b.width && a.height==b.height,"Image comparison size changed.");
  int maximum=0;
  for (size_t i=0;i<a.pixels.size();++i) maximum=std::max(maximum,std::abs(int(a.pixels[i])-int(b.pixels[i])));
  return float(maximum);
}
}
int main() {
  const auto origin=visual::ShadowProjections({0,0,0}),tiny=visual::ShadowProjections({.0001f,0,.0001f});
  Check(std::memcmp(origin.data(),tiny.data(),sizeof(origin))==0,"Sub-texel focus movement must leave the light grid unchanged.");
  for (int i=0;i<80;++i) {
    const auto moved=visual::ShadowProjections({i*.07f,i*.013f,-i*.03f});
    Check(std::memcmp(&origin[0],&moved[0],sizeof(origin[0]))==0,"Detail focus must not move the whole-island projection.");
    const float x=(moved[1]._41-origin[1]._41)*visual::shadowSize*.5f;
    const float y=(moved[1]._42-origin[1]._42)*visual::shadowSize*.5f;
    Check(std::abs(x-std::round(x))<.001f && std::abs(y-std::round(y))<.001f,
      "Detail projection must translate by whole texels without changing its extent.");
  }
  Require(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM initialization failed.");
  Win32Window window;
  Require(window.Create(GetModuleHandle(nullptr),960,720,L"shadow checks",false),"Hidden shadow window failed.");
  Renderer renderer;
  const bool initialized=renderer.Initialize(window.GetHandle(),960,720);
  if (!initialized) std::wcerr<<renderer.Error()<<L'\n';
  Require(initialized,"Shadow renderer initialization failed.");
  {
    renderer.BeginFrame();
    const auto initial=Capture(renderer,L"sky-default.png");
    Check(initial.At({.5f,.5f})>80,"The first frame needs a valid sky before the caller sets a camera.");
    const auto sky=[&](XMFLOAT3 eye,XMFLOAT3 target,const wchar_t* path) {
      renderer.BeginFrame(); renderer.SetCamera(eye,target);
      return Capture(renderer,path);
    };
    const auto north=sky({0,0,0},{0,.3f,1},L"sky-north.png");
    const auto east=sky({0,0,0},{1,.3f,0},L"sky-east.png");
    const auto upward=sky({0,0,0},{0,1,1},L"sky-upward.png");
    Check(MaximumDifference(north,east)>20,"Clouds must respond to camera yaw instead of remaining glued to the screen.");
    Check(MaximumDifference(north,upward)>20,"The sky horizon must respond to camera pitch.");
    const auto translated=sky({3,2,4},{3,2.3f,5},L"sky-translated.png");
    Check(MaximumDifference(north,translated)<=2,"Distant sky must not shift when only camera position changes.");
    Check(renderer.LastDrawCalls()==1,"An empty world must render its sky exactly once.");
    const auto returned=sky({0,0,0},{0,.3f,1},L"sky-returned.png");
    Check(MaximumDifference(north,returned)==0,"Returning to a view must restore its sky without frame-lagged camera state.");
    const auto sun=visual::daylightSun;
    const auto sunlight=sky({0,0,0},{sun.x,sun.y,sun.z},L"sky-sun.png");
    Check(sunlight.At({.5f,.5f})>240,"The visible sun must align with the same direction used by shadow projections.");
    renderer.BeginFrame(); renderer.SetCamera({0,0,0},{sun.x,sun.y,sun.z});
    const float length=std::sqrt(sun.x*sun.x+sun.y*sun.y+sun.z*sun.z);
    renderer.DrawMesh(M::Box,{sun.x/length*3,sun.y/length*3,sun.z/length*3},{.5f,.5f,.5f},
      {.02f,.02f,.02f,1},S::Plain,0,0,Shadow::None);
    const auto covered=Capture(renderer,L"sky-sun-occluded.png");
    Check(sunlight.At({.5f,.5f})-covered.At({.5f,.5f})>80,"Opaque geometry must occlude the sun, including its halo.");
    renderer.SetAtmosphereTime(180);
    const auto drifted=sky({0,0,0},{0,.3f,1},L"sky-cloud-drift.png");
    Check(MaximumDifference(north,drifted)>10,"Clouds must drift with simulation time.");
    renderer.SetAtmosphereTime(std::numeric_limits<double>::quiet_NaN());
    const auto invalidTime=sky({0,0,0},{0,.3f,1},L"sky-invalid-time.png");
    Check(MaximumDifference(drifted,invalidTime)==0,"Invalid time must not corrupt atmospheric state.");
    renderer.SetAtmosphereTime(1800);
    const auto wrapped=sky({0,0,0},{0,.3f,1},L"sky-cloud-period.png");
    Check(MaximumDifference(north,wrapped)==0,"Cloud drift must close smoothly without a wrap jump.");
  }
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
  float minimumThin=255,totalThin=0;
  for (int i=0;i<8;++i) {
    const float shift=i*.0125f;
    floor(); renderer.DrawMesh(M::Box,{shift,1,0},{.035f,2,.035f},white,S::Plain,0,0,Shadow::None);
    const auto reference=Capture(renderer,L"shadow-thin-reference.png");
    floor(); renderer.DrawMesh(M::Box,{shift,1,0},{.035f,2,.035f},white,S::Plain);
    const auto thin=Capture(renderer,L"shadow-thin.png");
    const auto probe=screen({shift+30.f/51,0,24.f/51});
    const float drop=reference.At(probe,0)-thin.At(probe,0);
    minimumThin=std::min(minimumThin,drop); totalThin+=drop;
  }
  std::cout<<"Thin post shadow min/mean contrast: "<<minimumThin<<" / "<<totalThin/8<<'\n';
  Check(minimumThin>35 && totalThin/8>50,"Cubic filtering must retain thin-post contrast through sub-texel movement.");
  const auto focusView=[&](float focus,const wchar_t* path) {
    floor(); renderer.SetShadowFocus({focus,0,0});
    renderer.DrawMesh(M::Box,{0,1,0},{.035f,2,.035f},white,S::Plain);
    return Capture(renderer,path);
  };
  auto previousFocus=focusView(0,L"shadow-focus-origin.png");
  float gridChange=0;
  for (int i=1;i<=12;++i) {
    auto current=focusView(i*.02f,L"shadow-focus-step.png");
    gridChange=std::max(gridChange,MaximumDifference(previousFocus,current)); previousFocus=std::move(current);
  }
  std::cout<<"Snapped-focus pixel change: "<<gridChange<<'\n';
  Check(gridChange<=2,"Moving the focused light grid must not make stationary shadow edges swim.");
  previousFocus=focusView(4,L"shadow-blend-start.png");
  float blendChange=0;
  for (int i=1;i<=32;++i) {
    auto current=focusView(4+i*.5f,L"shadow-blend-step.png");
    blendChange=std::max(blendChange,MaximumDifference(previousFocus,current)); previousFocus=std::move(current);
  }
  std::cout<<"Detail/far blend max pixel step: "<<blendChange<<'\n';
  Check(blendChange<25,"Detail/far transitions must blend instead of popping between resolutions.");
  floor(); renderer.SetShadowFocus({40,0,40});
  renderer.DrawMesh(M::Box,{0,1,0},{.5f,2,.5f},white,S::Plain);
  const auto farOnly=Capture(renderer,L"shadow-far-only.png");
  Check(lit.At(postProbe)-farOnly.At(postProbe)>25,"Leaving the detail region must retain whole-island shadows.");
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
  // Translating an object and its camera together must preserve its material:
  // world-projected grain/relief previously slid through moving body parts.
  for (auto material : {S::Stone,S::Wood,S::Roof,S::Cloth,S::Metal}) {
    const auto surface=[&](float shift,const wchar_t* path) {
      renderer.BeginFrame(); renderer.SetCamera({3+shift,2,4+shift*.7f},{shift,0,shift*.7f});
      renderer.DrawMesh(M::Sphere,{shift,0,shift*.7f},{2,1.5f,1.7f},white,material,.37f,.23f,Shadow::None);
      return Capture(renderer,path);
    };
    const auto before=surface(0,L"material-anchored-reference.png");
    const auto after=surface(.73f,L"material-anchored-moved.png");
    double difference=0;
    for (std::size_t i=0;i<before.pixels.size();++i)
      difference+=std::abs(int(before.pixels[i])-int(after.pixels[i]));
    difference/=double(before.pixels.size());
    std::cout<<"Anchored material "<<int(material)<<" mean channel difference: "<<difference<<'\n';
    Check(difference<.15,"Material grain and relief must remain attached during movement.");
  }
  // A fully culled queue is still consumed at a world/portrait boundary.
  const auto culledWorld=[&](bool queued,const wchar_t* path) {
    renderer.BeginFrame(); renderer.SetCamera({0,1,-5},{0,1,-10});
    if (queued) renderer.DrawMesh(M::Box,{0,1,0},{2,2,2},{1,0,1,1},S::Plain,0,0,Shadow::None);
    Require(renderer.BeginPortrait(.65f,.1f,.3f,.6f),"Begin culled-world portrait failed.");
    renderer.DrawMesh(M::Sphere,{0,1,0},{.7f,1,.7f},white,S::Metal);
    Require(renderer.EndPortrait(),"End culled-world portrait failed.");
    return Capture(renderer,path);
  };
  const auto emptyWorld=culledWorld(false,L"shadow-culled-world-reference.png");
  const auto hiddenWorld=culledWorld(true,L"shadow-culled-world.png");
  const float worldLeak=MaximumDifference(emptyWorld,hiddenWorld);
  std::cout<<"Culled world to portrait pixel difference: "<<worldLeak<<'\n';
  Check(worldLeak==0,"Culled world geometry must not leak into the portrait.");
  const auto culledPortrait=[&](bool queued,const wchar_t* path) {
    floor();
    Require(renderer.BeginPortrait(.65f,.1f,.3f,.6f),"Begin culled portrait failed.");
    // This object is behind the preview camera but visible in the world camera.
    if (queued) renderer.DrawMesh(M::Box,{0,1,6},{.5f,.5f,.5f},{1,0,1,1},S::Plain);
    Require(renderer.EndPortrait(),"End culled portrait failed.");
    renderer.SetCamera({3,4,10},{0,0,4});
    return Capture(renderer,path);
  };
  const auto emptyPortrait=culledPortrait(false,L"shadow-culled-portrait-reference.png");
  const auto hiddenPortrait=culledPortrait(true,L"shadow-culled-portrait.png");
  const float portraitLeak=MaximumDifference(emptyPortrait,hiddenPortrait);
  std::cout<<"Culled portrait to world pixel difference: "<<portraitLeak<<'\n';
  Check(portraitLeak==0,"Culled portrait geometry must not return to the world or its shadows.");
  // 1,400 scene objects fitted the previous two-pass upload budget. Adding a
  // detail shadow pass must not turn repeated draws into extra scene objects.
  floor();
  for (int i=0;i<1400;++i)
    renderer.DrawMesh(M::Box,{(i%40-20)*.03f,.5f,(i/40-17)*.03f},{.02f,.5f,.02f},white,S::Plain);
  const bool denseFrame=renderer.EndFrame(false);
  std::cout<<"Dense shadow scene rendered: "<<denseFrame<<'\n';
  Check(denseFrame,"A supported scene must not overflow merely because objects appear in both shadow maps.");
  // Sharing uploaded instances must preserve a hard scene bound, including its exact
  // edge. Keep overflow last: a rejected frame is not submitted for presentation.
  for (int count : {4096,4097}) {
    renderer.BeginFrame(); renderer.SetCamera({3,4,5},{0,0,0});
    for (int i=0;i<count;++i)
      renderer.DrawMesh(M::Box,{(i%64-32)*.025f,.5f,(i/64-32)*.025f},{.02f,.5f,.02f},white,S::Plain);
    const bool rendered=renderer.EndFrame(false);
    Check(rendered==(count==4096),"The scene instance budget must accept its boundary and reject overflow.");
    if (!rendered) Check(renderer.Error()==L"Scene exceeded the 4096 visible-instance budget.",
      "An over-budget scene must report the actual capacity error.");
  }
  Require(renderer.ValidateDebugMessages(),"Queue regressions emitted a DirectX warning/error.");
  std::cout<<(failures?"FAILED: ":"PASS: ")<<"GPU shadow contact, materials, slopes, moving/offscreen casters, clearing, resize and preview isolation.\n";
  CoUninitialize(); return failures?1:0;
}
