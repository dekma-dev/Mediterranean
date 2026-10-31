#include "tools/BodyAssetAdapter.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <wincodec.h>

using namespace DirectX;
void Check(bool ok,const char* message) {if(!ok) {std::cerr<<message<<'\n';std::exit(1);}}
template<class F> void Reject(F action) {bool failed=false;try {action();} catch(const std::runtime_error&) {failed=true;}Check(failed,"Invalid preview input accepted");}
std::vector<unsigned char> Pixels(const std::wstring& path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"PNG decode");
  UINT w=0,h=0;Check(SUCCEEDED(converter->GetSize(&w,&h)) && w==960 && h==720,"Capture size");
  std::vector<unsigned char> pixels(w*h*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,w*4,UINT(pixels.size()),pixels.data())),"Capture pixels");return pixels;
}
void Frame(Renderer& r,const visual::BodyPalette& palette,const std::wstring& path) {
  r.BeginFrame();r.SetCamera({3,3,-9},{0,2,-2});r.DrawBody({0,0,0},0,palette,1);
  Check(r.Capture(path) && r.EndFrame(false) && r.ValidateDebugMessages(),"Imported body GPU draw");
}
int main() {
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  std::ifstream input(RPG_MESH_FIXTURE,std::ios::binary);std::ostringstream bytes;bytes<<input.rdbuf();
  const auto asset=assets::ImportGlbAsset(bytes.str());const auto geometry=assets::BodyGeometry(asset);
  auto bad=geometry;bad.influences[0].bones[0]=visual::bodyBones;Reject([&]{visual::ValidateBodyGeometry(bad);});
  bad=geometry;bad.mesh.indices[0]=65535;Reject([&]{visual::ValidateBodyGeometry(bad);});
  bad=geometry;bad.influences[0].weights.x=2;Reject([&]{visual::ValidateBodyGeometry(bad);});
  bad=geometry;bad.mesh.vertices[0].normal={0,0,0};Reject([&]{visual::ValidateBodyGeometry(bad);});
  bad=geometry;bad.mesh.vertices[0].color.w=float(visual::bodyRegions);Reject([&]{visual::ValidateBodyGeometry(bad);});
  auto wrong=asset.skeleton.RestPose();wrong.skin.pop_back();Reject([&]{assets::BodyPose(asset,wrong);});
  std::vector<animation::Joint> joints(11);for(int i=0;i<11;++i) {joints[i].name="joint"+std::to_string(i);joints[i].parent=i-1;}
  assets::RigAsset large{animation::Skeleton(joints),{},asset.mesh};Reject([&]{assets::BodyGeometry(large);});
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Imported skin checks",false),"Window");
  Renderer gpu;Check(gpu.Initialize(window.GetHandle(),960,720,&geometry),"Imported immutable buffers");
  unsigned changed=0;std::vector<unsigned char> first;
  for(int step=0;step<4;++step) {
    const float seconds=step==3?1.f:step*.25f;
    const auto pose=asset.skeleton.Sample(asset.clips[0].clip,seconds,false);
    // Plain material isolates deformation/normals: the production shader keeps
    // cloth coordinates in bind space, unlike the baked CPU reference.
    const auto palette=assets::BodyPose(asset,pose,{.45f,.62f,.75f,float(visual::Material::Plain)});
    const auto deformed=assets::DeformMesh(asset.mesh,pose);
    auto reference=geometry;
    for(std::size_t i=0;i<deformed.size();++i) {
      const auto cpu=visual::SkinVertex(geometry.mesh.vertices[i],geometry.influences[i],palette);
      Check(XMVectorGetX(XMVector3Length(XMLoadFloat3(&cpu.position)-XMLoadFloat3(&deformed[i].position)))<.00002f,"Adapter changed joint order or weights");
      reference.mesh.vertices[i].position=deformed[i].position;reference.mesh.vertices[i].normal=deformed[i].normal;
      reference.influences[i]={};reference.influences[i].weights={1,0,0,0};
    }
    Win32Window refWindow;Check(refWindow.Create(GetModuleHandleW(nullptr),960,720,L"CPU reference",false),"Reference window");
    Renderer cpu;Check(cpu.Initialize(refWindow.GetHandle(),960,720,&reference),"Reference buffers");
    auto identity=palette;for(unsigned i=0;i<visual::bodyBones;++i) {XMStoreFloat4x4(&identity.joints[i],XMMatrixIdentity());XMStoreFloat4x4(&identity.normals[i],XMMatrixIdentity());}
    const auto name=L"imported-"+std::to_wstring(step);
    Frame(gpu,palette,name+L"-gpu.png");Frame(cpu,identity,name+L"-cpu.png");
    const auto a=Pixels(name+L"-gpu.png"),b=Pixels(name+L"-cpu.png");
    unsigned pixels=0;
    for(std::size_t p=0;p<a.size();p+=4) {
      bool difference=false;for(unsigned c=0;c<3;++c) difference=difference || std::abs(int(a[p+c])-int(b[p+c]))>3;
      pixels+=difference;
      if(step && a[p]!=first[p]) ++changed;
    }
    std::cout<<"Pose "<<seconds<<": "<<pixels<<" CPU/GPU differing pixels\n";
    Check(pixels<120,"CPU/GPU deformation or normals disagree");
    if(!step) first=a;
  }
  Check(changed>1000,"Imported clip did not animate visible geometry");
  std::cout<<"PASS: imported skin order, weight conversion, rejection, animated GPU/CPU render parity\n";
}
