#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <set>
#include <cstring>
#include <limits>
#include <wincodec.h>

namespace {
void Check(bool ok,const char* message) {if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
std::uint64_t Fingerprint(const visual::Geometry& g) {
  std::uint64_t h=14695981039346656037ull;
  for(const auto& v:g.vertices) for(float component:{v.position.x,v.position.y,v.position.z}) {
    std::uint32_t bits;std::memcpy(&bits,&component,sizeof bits);
    h=(h^bits)*1099511628211ull;
  }
  return h;
}
std::vector<unsigned char> Pixels(const std::filesystem::path& path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Decode facial capture");
  UINT width=0,height=0;Check(SUCCEEDED(converter->GetSize(&width,&height)) && width==960 && height==720,"Facial capture dimensions");
  std::vector<unsigned char> pixels(width*height*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())),"Read facial capture");return pixels;
}
}
int main() {
  using namespace DirectX;using M=visual::Mesh;
  Check(FaceHead(HumanFace(999))==M::Head && FaceHair(HumanFace(999))==M::Hair,"Invalid identity must use the player face");
  std::set<std::uint64_t> shapes;
  for(unsigned id=0;id<unsigned(HumanFace::Count);++id) {
    const auto mesh=FaceHead(HumanFace(id)),hair=FaceHair(HumanFace(id));
    std::size_t previous=std::numeric_limits<std::size_t>::max(),bytes=0,triangles=0;
    for(unsigned lod=0;lod<3;++lod) {
      const auto kind=visual::AttachmentMesh(mesh,lod);const auto g=visual::BuildMesh(kind);
      Check(visual::IsHeadMesh(kind),"Identity LOD lost facial deformation");
      Check(g.indices.size()<previous,"Each facial LOD must reduce topology");previous=g.indices.size();
      Check(g.indices.size()/3<=(lod==0?16000u:lod==1?3000u:480u),"Per-identity face budget exceeded");
      bytes+=g.vertices.size()*sizeof(visual::Vertex)+g.indices.size()*4;triangles+=g.indices.size()/3;
      for(const auto& v:g.vertices) {
        const auto p=v.position,n=v.normal;
        Check(std::isfinite(p.x+p.y+p.z+n.x+n.y+n.z) && std::abs(p.x)<=.5001f && std::abs(p.y)<=.5001f && std::abs(p.z)<=.5001f,"Face escaped fitted attachment envelope");
        Check(std::abs(n.x*n.x+n.y*n.y+n.z*n.z-1)<.005f,"Facial normal is invalid");
      }
      if(!lod) {
        shapes.insert(Fingerprint(g));
        // This regresses the old export bug: the socket fit pulled the chin
        // down into a cone. A real chin must project in front of the upper neck.
        float chin=-1,neck=-1;
        for(const auto& v:g.vertices) {
          float x=v.position.x*.265f,y=v.position.y*.42f+1.71f,z=v.position.z*.26f+.012f;
          if(std::abs(x)<.018f && y>1.640f && y<1.677f)chin=std::max(chin,z);
          if(std::abs(x)<.018f && y>1.580f && y<1.620f)neck=std::max(neck,z);
        }
        Check(chin>neck+.020f,"Mandible/chin must remain in front of the neck socket");
      }
    }
    for(unsigned lod=0;lod<2;++lod) {
      const auto kind=visual::AttachmentMesh(hair,lod);const auto g=visual::BuildMesh(kind);
      Check(visual::IsHairMesh(kind),"Identity hair remapped into another part");
      Check(g.indices.size()/3<=(lod?400u:5000u),"Hair exceeded its shared budget");
      bytes+=g.vertices.size()*sizeof(visual::Vertex)+g.indices.size()*4;triangles+=g.indices.size()/3;
    }
    Check(bytes<1024*1024 && triangles<26000,"Individual identity exceeded 1 MiB / 26000 triangles");
  }
  Check(shapes.size()==unsigned(HumanFace::Count),"NPC faces must contain distinct geometry, not just different tint");
  Check(residentAppearances[1].face==HumanFace::Young && residentAppearances[9].face==HumanFace::Mature &&
    residentAppearances[13].face==HumanFace::Elder,"Named residents lost their stable anatomical identity");
  for(const auto& resident:residentAppearances)Check(resident.colors.Valid(),"Invalid resident appearance palette");
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Face review COM");
  const auto folder=std::filesystem::current_path()/"human-face-check";std::filesystem::create_directories(folder);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Anatomical face validation",false),"Face review window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,720),"Face review renderer");
  for(unsigned id=0;id<unsigned(HumanFace::Count);++id) for(unsigned view=0;view<6;++view) {
    auto outfit=CharacterOutfit::From(rpg::Inventory{},view==4?rpg::ClassId::Mage:rpg::ClassId::Warrior);
    outfit.face=HumanFace(id);outfit.appearance={1,id==2 || id==5?4:0,0};outfit.weapon=false;outfit.helm=view==3;
    animation::Sample pose;if(view==2)pose.face.blink=1;
    renderer.BeginFrame();renderer.SetCamera(view==1?XMFLOAT3{-.66f,1.77f,-.02f}:XMFLOAT3{-.18f,1.77f,-.66f},{0,1.70f,0});
    if(view==5)Check(renderer.BeginPortrait(0,0,1,1,.80f),"Lit face portrait");
    DrawCharacter(renderer,{0,0},0,view==5?.25f:XM_PI,{.38f,.36f,.31f,1},0,0,outfit,pose,1);
    if(view==5)Check(renderer.EndPortrait(),"Finish lit portrait");
    Check(renderer.Capture((folder/("face-"+std::to_string(id)+"-view-"+std::to_string(view)+".png")).wstring()) && renderer.EndFrame(false),"Native identity / profile / blink / helmet capture");
    if(view<5)Check(renderer.AttachmentLodCounts()==std::array<unsigned,2>{0,0},"Close portraits unexpectedly selected distant face LODs");
  }
  for(unsigned id=0;id<unsigned(HumanFace::Count);++id) {
    const auto prefix="face-"+std::to_string(id);
    const auto open=Pixels(folder/(prefix+"-view-0.png")),closed=Pixels(folder/(prefix+"-view-2.png"));
    unsigned changed=0,outside=0;
    for(unsigned y=0;y<720;++y)for(unsigned x=0;x<960;++x) {
      const auto p=(y*960+x)*4;
      const bool difference=std::abs(int(open[p])-int(closed[p]))+std::abs(int(open[p+1])-int(closed[p+1]))+std::abs(int(open[p+2])-int(closed[p+2]))>18;
      // This fixed close-up frames every identity's complete head, hair and
      // neck inside these bounds. Blinking also moves the eyelid's cast shadow
      // on the right shoulder; that is not deformation of facial anatomy.
      const bool headAndNeck=x>=315 && x<=610 && y>=130 && y<=575;
      changed+=difference && headAndNeck;
      outside+=difference && headAndNeck && (x<330 || x>650 || y<285 || y>410);
    }
    Check(changed>100 && changed<12000,"Blink must visibly close the eyes");
    Check(outside<300,"Blink displaced forehead, hairline, jaw or neck outside the ocular region");
  }
  for(auto asset:{authored::Asset::Mage,authored::Asset::Orc}) for(unsigned view=0;view<2;++view) {
    const float height=asset==authored::Asset::Orc?2.10f:2.05f;
    renderer.BeginFrame();renderer.SetCamera(view?XMFLOAT3{.72f,height+.04f,0}:XMFLOAT3{-.20f,height+.04f,-.72f},{0,height,0});
    renderer.DrawAuthoredAsset(asset,{0,0,0},{1,1,1},XM_PI,0);
    Check(renderer.Capture((folder/("collection-"+std::to_string(unsigned(asset))+"-view-"+std::to_string(view)+".png")).wstring()) && renderer.EndFrame(false),"Authored collection face closeup");
  }
  std::cout<<"Six distinct anatomical faces; chin/neck separation, bounded LODs, resident identities and 40 native views passed\n";
  CoUninitialize();
}
