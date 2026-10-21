#include "game/CharacterRig.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <wincodec.h>

namespace {
using namespace DirectX;
using M=visual::Mesh; using S=visual::Material;
void Check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
std::vector<unsigned char> Pixels(const std::wstring& path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory; ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame; ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Attachment PNG decode");
  UINT width=0,height=0; Check(SUCCEEDED(converter->GetSize(&width,&height)) && width==960 && height==720,"Attachment diagnostic dimensions");
  std::vector<unsigned char> pixels(width*height*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())),"Attachment pixels"); return pixels;
}
void Compare(const std::wstring& prefix,unsigned level) {
  const auto a=Pixels(prefix+L"-auto.png"),b=Pixels(prefix+L"-full.png"),background=Pixels(prefix+L"-background.png");
  std::vector<bool> masks[2]; unsigned difference=0,unmatched=0,covered=0; double colorError=0;
  for(auto& mask : masks) mask.resize(960*720);
  for(std::size_t pixel=0;pixel<masks[0].size();++pixel) {
    for(int channel=0;channel<3;++channel) {
      masks[0][pixel]=masks[0][pixel] || std::abs(int(a[pixel*4+channel])-int(background[pixel*4+channel]))>16;
      masks[1][pixel]=masks[1][pixel] || std::abs(int(b[pixel*4+channel])-int(background[pixel*4+channel]))>16;
    }
    if(masks[0][pixel] || masks[1][pixel]) {
      ++covered;
      for(int channel=0;channel<3;++channel) colorError+=std::abs(int(a[pixel*4+channel])-int(b[pixel*4+channel]));
    }
    difference+=masks[0][pixel]!=masks[1][pixel];
  }
  for(int side=0;side<2;++side) for(int y=2;y<718;++y) for(int x=2;x<958;++x) if(masks[side][y*960+x]) {
    bool nearby=false;
    for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx) nearby=nearby || masks[1-side][(y+dy)*960+x+dx];
    unmatched+=!nearby;
  }
  const double mean=colorError/std::max(1u,covered*3);
  std::wcout<<prefix<<L": silhouette "<<difference<<L" / "<<covered<<L", beyond 2 px "<<unmatched<<L", mean RGB error "<<mean<<'\n';
  Check(covered>100 && unmatched==0 && difference<covered*.08f && mean<12,"Attachment LOD lost the character silhouette or material readability");
  if(!level) Check(a==b,"Near attachments must remain pixel identical");
}
void ScopeAndPortrait(Renderer& renderer) {
  const auto palette=BuildCharacterRig(BuildCharacterPose(0,0,false),{}, {.2f,.4f,.6f,1});
  visual::StaticScene scenery;
  for(int pass=0;pass<2;++pass) {
    renderer.BeginFrame(); renderer.SetCamera({0,1,34},{0,1,0});
    const auto body=renderer.DrawBody({0,0,0},0,palette,12);
    {
      const auto attachments=renderer.AttachToBody(body);
      renderer.DrawMesh(M::Head,{0,1.76f,0},{.265f,.31f,.26f},{.7f,.5f,.3f,1},S::Skin);
      renderer.DrawMesh(M::Feature,{0,1.78f,.13f},{.02f,.01f,.01f},{.1f,.1f,.1f,1},S::Plain,0,0,visual::ShadowMode::None,0,0);
    }
    renderer.DrawStaticScene(scenery,[&] {
      renderer.DrawMesh(M::Head,{2,1,0},{1,1,1},{.4f,.4f,.4f,1},S::Stone);
      renderer.DrawMesh(M::Sphere,{-2,1,0},{1,1,1},{.4f,.4f,.4f,1},S::Stone);
    });
    Check(renderer.EndFrame(false),"Attachment owner/cached scenery frame");
    Check(renderer.AttachmentLodCounts()==std::array<unsigned,2>{1,1},"Detail ownership leaked into scenery or failed to follow its body");
    Check(scenery.Instances()==2 && scenery.Bytes()==360,"LOD metadata enlarged cached scenery or duplicated it");
  }
  renderer.BeginFrame(); renderer.SetCamera({0,1,5},{0,1,0});
  DrawCharacter(renderer,{2,-29},0,0,{.2f,.4f,.6f,1},0,0,{}, {},14);
  Check(renderer.EndFrame(false),"Distant owner reference");
  const auto distant=renderer.AttachmentLodCounts();
  renderer.BeginFrame(); renderer.SetCamera({0,1,5},{0,1,0});
  DrawCharacter(renderer,{-1,0},0,0,{.2f,.4f,.6f,1},0,0,{}, {},13);
  DrawCharacter(renderer,{2,-29},0,0,{.2f,.4f,.6f,1},0,0,{}, {},14);
  Check(renderer.EndFrame(false) && renderer.BodyLodCounts()[0]==1 && renderer.BodyLodCounts()[2]==1 &&
    renderer.AttachmentLodCounts()==distant,"Nearby and distant owners must not share attachment detail");
  // Force a distant owner's attachment into a close diagnostic view. Comparing
  // to the explicit reduced meshes checks both color and real floor shadows.
  for(int explicitMesh=0;explicitMesh<2;++explicitMesh) {
    renderer.BeginFrame(); renderer.SetCamera({0,3,5},{0,.7f,0});
    renderer.DrawMesh(M::Box,{0,-.05f,0},{6,.1f,6},{.6f,.6f,.6f,1},S::Stone);
    const auto body=renderer.DrawBody({100,0,-40},0,palette,15);
    const auto draw=[&](M head,M helmet) {
      renderer.DrawMesh(head,{0,1,0},{.8f,1,.8f},{.7f,.5f,.3f,1},S::Skin);
      renderer.DrawMesh(helmet,{0,1.3f,0},{.9f,.45f,.9f},{.5f,.6f,.6f,1},S::Metal);
    };
    if(explicitMesh) draw(M::HeadFar,M::SphereLow);
    else { const auto owner=renderer.AttachToBody(body); draw(M::Head,M::Sphere); }
    Check(renderer.Capture(explicitMesh?L"attachment-shadow-explicit.png":L"attachment-shadow-auto.png") && renderer.EndFrame(false),"Attachment shadow fixture");
  }
  Check(Pixels(L"attachment-shadow-explicit.png")==Pixels(L"attachment-shadow-auto.png"),"Automatic mesh selection changed color/shadow geometry or head receiver policy");
  for(int full=0;full<2;++full) {
    renderer.SetAttachmentLodEnabled(!full); renderer.BeginFrame(); renderer.SetCamera({0,1,34},{0,1,0});
    DrawCharacter(renderer,{0,0},0,0,{.2f,.4f,.6f,1},0,0,{}, {},12);
    renderer.DrawHudRect(.7f,.05f,.28f,.8f,{.05f,.06f,.04f,1});
    Check(renderer.BeginPortrait(.7f,.05f,.28f,.8f),"Portrait flush after distant attachments");
    const auto stats=renderer.AttachmentLodCounts(); const auto triangles=renderer.Triangles();
    CharacterOutfit outfit; outfit.armor=outfit.shield=outfit.helm=outfit.shoulders=true;
    DrawCharacter(renderer,{0,0},0,0,{.2f,.4f,.6f,1},0,0,outfit,{},12);
    Check(renderer.EndPortrait() && stats==renderer.AttachmentLodCounts(),"Portrait changed world attachment counters");
    static unsigned portraitTriangles=0;
    if(!full) portraitTriangles=renderer.Triangles()-triangles;
    else Check(renderer.Triangles()-triangles==portraitTriangles,"Portrait lost full-detail geometry");
    DrawCharacter(renderer,{-1,0},0,0,{.2f,.4f,.6f,1},0,0,{}, {},12);
    Check(renderer.Capture(full?L"attachment-portrait-full.png":L"attachment-portrait-auto.png") && renderer.EndFrame(false),"Restore world attachment detail after portrait");
    Check(renderer.BodyLodCounts()[2]==2,"Portrait changed the owner's world detail history");
  }
  const auto a=Pixels(L"attachment-portrait-auto.png"),b=Pixels(L"attachment-portrait-full.png");
  for(int y=36;y<612;++y) for(int x=672;x<940;++x) for(int c=0;c<3;++c)
    Check(a[(y*960+x)*4+c]==b[(y*960+x)*4+c],"World LOD changed portrait pixels");
  renderer.SetAttachmentLodEnabled(true);
  Check(renderer.Resize(960,1440),"Attachment resolution change");
  renderer.SetBodyLodEnabled(true); renderer.BeginFrame(); renderer.SetCamera({0,1,16},{0,1,0});
  DrawCharacter(renderer,{0,0},0,0,{.2f,.4f,.6f,1},0,0,{}, {},12);
  Check(renderer.EndFrame(false) && renderer.AttachmentLodCounts()==std::array<unsigned,2>{0,0},"Resolution promotion must restore full attachments");
  Check(renderer.Resize(960,720),"Restore attachment resolution");
  renderer.BeginFrame(); renderer.SetCamera({0,1,-34},{0,1,-35});
  DrawCharacter(renderer,{0,0},0,0,{.2f,.4f,.6f,1},0,0,{}, {},12);
  Check(renderer.EndFrame(false) && renderer.AttachmentLodCounts()[0]>0,"Offscreen shadow casters must share reduced detail");
  renderer.BeginFrame(); renderer.SetCamera({0,1,34},{0,1,0});
  renderer.DrawMesh(M::Head,{0,1,0},{1,1,1},{.4f,.4f,.4f,1},S::Stone);
  Check(renderer.EndFrame(false) && renderer.AttachmentLodCounts()==std::array<unsigned,2>{0,0},"Attachment ownership survived frame reset");
}
}
int main() {
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Attachment COM");
  Win32Window window; Check(window.Create(GetModuleHandle(nullptr),960,720,L"Character attachment LOD",false),"Hidden attachment window");
  Renderer renderer; Check(renderer.Initialize(window.GetHandle(),960,720),"Attachment renderer");
  for(int level=0;level<3;++level) for(int equipped=0;equipped<2;++equipped) for(int pose=0;pose<3;++pose) {
    const float distance=level==0?5.f:level==1?9.6f:22.f;
    const auto prefix=L"attachment-"+std::to_wstring(level)+L"-"+std::to_wstring(equipped)+L"-"+std::to_wstring(pose);
    renderer.BeginFrame(); renderer.SetCamera({0,1,distance},{0,1,0});
    Check(renderer.Capture(prefix+L"-background.png") && renderer.EndFrame(false),"Attachment background");
    unsigned triangles[2]{};
    for(int full=0;full<2;++full) {
      renderer.SetAttachmentLodEnabled(!full); renderer.SetBodyLodEnabled(true);
      renderer.BeginFrame(); renderer.SetCamera({0,1,level==0?5.f:level==1?16.f:34.f},{0,1,0});
      DrawCharacter(renderer,{0,0},0,0,{.22f,.37f,.54f,1},0,0,{}, {},42);
      Check(renderer.EndFrame(false),"Seed attachment history");
      renderer.BeginFrame(); renderer.SetCamera({0,1,5},{0,1,0});
      animation::Sample sample; sample.air=pose==1?1.f:0; sample.rise=.5f;
      sample.attackSeconds=pose==2?.30f:-1; sample.heavyAttack=pose==2;
      sample.face.yaw=.5f; sample.face.focus=.5f;
      CharacterOutfit outfit; outfit.armor=outfit.shoulders=outfit.helm=outfit.cloak=outfit.shield=equipped!=0;
      DrawCharacter(renderer,{0,0},0,pose*.65f,{.22f,.37f,.54f,1},0,pose==0?.5f:0,outfit,sample,42);
      renderer.SetCamera({0,1,distance},{0,1,0});
      Check(renderer.Capture(prefix+(full?L"-full.png":L"-auto.png")) && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Attachment color/shadow capture");
      Check(renderer.BodyLodCounts()[level]==1,"Attachments must use final-camera owner selection");
      const auto stats=renderer.AttachmentLodCounts();
      Check(full || !level?stats==std::array<unsigned,2>{0,0}:stats[0]>0 && stats[1]>0,"Attachment detail diagnostics disagree with selected level");
      triangles[full]=renderer.Triangles();
    }
    Check(!level?triangles[0]==triangles[1]:triangles[0]<triangles[1]*.8f,"Reduced attachments did not meaningfully reduce submitted geometry");
    Compare(prefix,unsigned(level));
  }
  renderer.SetAttachmentLodEnabled(true); ScopeAndPortrait(renderer);
  Check(renderer.ValidateDebugMessages(),"Attachment device diagnostics");
  std::cout<<"PASS: attachment ownership, final camera, silhouette/material limits, shadow and portrait isolation; mesh bytes "<<renderer.MeshBytes()<<'\n';
  CoUninitialize();
}
