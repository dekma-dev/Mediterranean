#include "engine/PlayerBody.h"
#include "engine/Renderer.h"
#include "game/CharacterRig.h"
#include "platform/Win32Window.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <wincodec.h>

namespace {
void Check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
void Reject(std::string_view bytes) {
  bool rejected=false;
  try {playerbody::Decode(bytes);} catch(const std::runtime_error&) {rejected=true;}
  Check(rejected,"Corrupt body archive accepted");
}
template<class T> void Corrupt(std::string_view original,std::size_t offset,T value) {
  std::string bytes(original);Check(offset+sizeof(value)<=bytes.size(),"Invalid corruption probe");
  std::memcpy(bytes.data()+offset,&value,sizeof(value));Reject(bytes);
}
void Archive() {
  const auto bytes=playerbody::EmbeddedBytes();
  const auto decoded=playerbody::Decode(bytes);
  std::size_t offset=16+visual::bodyBones*24;
  for(std::size_t count=0;count<=offset;++count) Reject(bytes.substr(0,count));
  Corrupt(bytes,0,0u);Corrupt(bytes,4,2u);Corrupt(bytes,8,visual::bodyBones+1);Corrupt(bytes,12,0u);
  Corrupt(bytes,16,100.f);Corrupt(bytes,20,std::numeric_limits<float>::quiet_NaN());
  for(unsigned lod=0;lod<decoded.size();++lod) {
    const auto& body=decoded[lod];const auto count=body.mesh.vertices.size();
    Check(count==visual::CharacterBody(lod).mesh.vertices.size(),"Runtime body bypassed imported archive");
    Check(&visual::CharacterBody(lod)==&visual::CharacterBody(lod),"Runtime body was regenerated");
    unsigned undershorts=0;
    for(const auto& v:body.mesh.vertices) undershorts+=v.color.w==6;
    Check(undershorts>5,"Reduced body lost its mandatory undergarment");
    Corrupt(bytes,offset,0xffffffffu);Corrupt(bytes,offset+4,2u);
    const auto vertex=offset+8,indices=vertex+count*60;
    for(const auto end:{offset+1,vertex,vertex+59,indices,indices+body.mesh.indices.size()*4-1})
      Reject(bytes.substr(0,end));
    Corrupt(bytes,vertex,std::numeric_limits<float>::infinity());
    Corrupt(bytes,vertex+4,-10.f);Corrupt(bytes,vertex+12,100.f);
    Corrupt(bytes,vertex+24,visual::bodyRegions);Corrupt(bytes,vertex+28,visual::bodyBones);
    Corrupt(bytes,vertex+44,-1.f);Corrupt(bytes,vertex+48,std::numeric_limits<float>::quiet_NaN());
    Corrupt(bytes,indices,unsigned(count));
    Corrupt(bytes,indices+4,body.mesh.indices[0]); // Collapsed triangle.
    offset=indices+body.mesh.indices.size()*4;
  }
  Reject(std::string(bytes)+"unexpected");Reject(std::string(1024*1024+1,'x'));
  Check(offset==bytes.size(),"Archive layout drifted");
  Check(&visual::CharacterBody(999)==&visual::CharacterBody(2),"Invalid detail must clamp to final LOD");
}
void Clothing() {
  using namespace DirectX;
  for(unsigned id=0;id<unsigned(rpg::ClassId::Count);++id) for(bool trousers:{false,true}) {
    CharacterOutfit outfit;outfit.classId=rpg::ClassId(id);outfit.trousers=trousers;
    for(unsigned dye=0;dye<rpg::tunicDyes.size();++dye) {
      outfit.appearance.dye=int(dye);
      const auto rig=BuildCharacterRig(BuildCharacterPose(0,0,false),outfit,{.3f,.4f,.5f,1});
      Check(rig.colors[6].w==float(visual::Material::Cloth)||rig.colors[6].w==float(visual::Material::WornCloth),"Removing trousers exposed bare pelvis");
      if(trousers) Check(std::memcmp(&rig.colors[6],&rig.colors[3],sizeof(XMFLOAT4))==0,
        "Worn trousers have mismatched hip/thigh materials");
      for(const auto c:rig.colors) Check(std::isfinite(c.x+c.y+c.z+c.w),"Invalid body material after gear change");
    }
  }
}
std::vector<unsigned char> Pixels(const std::filesystem::path& path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Decode body materials");
  UINT w=0,h=0;Check(SUCCEEDED(converter->GetSize(&w,&h)) && w==960 && h==720,"Material view dimensions");
  std::vector<unsigned char> result(w*h*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,w*4,UINT(result.size()),result.data())),"Read body materials");return result;
}
void MaterialIsolation(Renderer& renderer,const std::filesystem::path& root) {
  using namespace DirectX;
  visual::BodyPalette palette;
  for(unsigned i=0;i<visual::paletteBones;++i) {
    XMStoreFloat4x4(&palette.joints[i],XMMatrixIdentity());XMStoreFloat4x4(&palette.normals[i],XMMatrixIdentity());
  }
  palette.colors.fill({.4f,.4f,.4f,float(visual::Material::Plain)});
  for(unsigned view=0;view<2;++view) {
    if(view) palette.colors[6]={.05f,.95f,.05f,float(visual::Material::Plain)};
    renderer.BeginFrame();Check(renderer.BeginPortrait(0,0,1,1),"Material inspection lighting");
    renderer.SetCamera({0,1,3.3f},{0,1,0});renderer.DrawBody({0,0,0},0,palette,1);
    Check(renderer.EndPortrait() && renderer.Capture((root/("material-"+std::to_string(view)+".png")).wstring()) && renderer.EndFrame(false),"Material inspection");
  }
  const auto a=Pixels(root/"material-0.png"),b=Pixels(root/"material-1.png");
  unsigned changed=0;
  for(unsigned y=0;y<720;++y) for(unsigned x=0;x<960;++x) {
    const auto i=(y*960+x)*4;
    if(std::abs(int(a[i])-int(b[i]))+std::abs(int(a[i+1])-int(b[i+1]))+std::abs(int(a[i+2])-int(b[i+2]))>12) {
      ++changed;
      // Conservative projection of the pelvis envelope. Forearms pass the
      // same height but must never receive trousers/undergarment materials.
      Check(x>=360 && x<=600 && y>=345 && y<=440,"Undergarment leaked outside the pelvis");
    }
  }
  Check(changed>500,"Regional pixel material was not applied");
}
void Captures() {
  using namespace DirectX;
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Body COM initialization");
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Anatomical body checks",false),"Body window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),960,720),"Body renderer");
  const auto root=std::filesystem::current_path()/"player-body-check";std::filesystem::create_directories(root);
  MaterialIsolation(renderer,root);
  for(unsigned view=0;view<10;++view) {
    CharacterOutfit outfit;outfit.belt=false;outfit.tunic=false;outfit.trousers=false;outfit.boots=false;outfit.weapon=false;
    outfit.classId=view==7?rpg::ClassId::Mage:rpg::ClassId::Warrior;
    animation::Sample sample;
    if(view==4) {sample.air=1;sample.rise=.6f;}
    if(view==5) {sample.landing=.8f;sample.face.pitch=.245f;}
    if(view==6) {sample.attackSeconds=.30f;sample.heavyAttack=true;}
    if(view==7) {sample.cast=1;sample.face.yaw=.62f;}
    if(view==8) {sample.hit=.8f;sample.face.yaw=-.62f;}
    if(view==9) {outfit.tunic=true;outfit.trousers=true;outfit.boots=true;outfit.classId=rpg::ClassId::Scout;}
    renderer.BeginFrame();Check(renderer.BeginPortrait(0,0,1,1),"Body inspection lighting");
    renderer.SetCamera(view==1?XMFLOAT3{0,1.25f,-3.3f}:view==2?XMFLOAT3{3.3f,1.25f,0}:XMFLOAT3{.65f,1.25f,3.25f},{0,1,0});
    DrawCharacter(renderer,{0,0},0,0,{.30f,.38f,.45f,1},0,view==3?.8f:0,outfit,sample,1);
    Check(renderer.EndPortrait() && renderer.Capture((root/("body-"+std::to_string(view)+".png")).wstring()) &&
      renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Anatomical pose/gear capture");
  }
  CoUninitialize();
}
}
int main() {
  Archive();Clothing();Captures();
  std::cout<<"PASS: anatomical archive rejection, cached LODs, undergarment ownership and native body pose views\n";
}
