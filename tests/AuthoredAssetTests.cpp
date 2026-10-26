#include "engine/AuthoredAssets.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <iostream>
#include <limits>
#include <cstring>
#include <stdexcept>

void Check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
template<class T> void Put(std::string& bytes,std::size_t offset,T value) {std::memcpy(bytes.data()+offset,&value,sizeof(value));}
void Reject(const std::string& bytes) {bool rejected=false;try{authored::Decode(bytes);}catch(const std::runtime_error&){rejected=true;}Check(rejected,"Malformed authored archive accepted");}
int main() {
 try {
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  const auto source=std::string(authored::EmbeddedBytes());
  const auto& library=authored::Collection();std::size_t triangles=0,bytes=0;
  for(const auto& m:library) {triangles+=m.geometry.indices.size()/3;bytes+=m.geometry.vertices.size()*40+m.geometry.indices.size()*4;}
  Check(bytes<authored::archiveBudget,"Authored geometry budget");
  for(auto size:{std::size_t(0),std::size_t(15),source.size()-1})Reject(source.substr(0,size));
  auto bad=source;Put(bad,0,0u);Reject(bad);
  bad=source;Put(bad,4,2u);Reject(bad);
  bad=source;Put(bad,16,0xffffffffu);Reject(bad);
  bad=source;Put(bad,48,std::numeric_limits<float>::quiet_NaN());Reject(bad);
  bad=source;Put(bad,60,0.f);Put(bad,64,0.f);Put(bad,68,0.f);Reject(bad);
  bad=source;Put(bad,84,99.f);Reject(bad);
  bad=source;Put(bad,48+library[0].geometry.vertices.size()*40,0xffffffffu);Reject(bad);
  bad=source;bad.push_back('x');Reject(bad);
  Check(authored::SelectLod(300)==0&&authored::SelectLod(150)==1&&authored::SelectLod(30)==2,"Distance LOD thresholds");
  Win32Window window;Renderer renderer;
  Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Authored asset checks",false),"Hidden window");
  Check(renderer.Initialize(window.GetHandle(),960,720),"Embedded GPU buffers");
  visual::StaticScene cached;
  unsigned nearTriangles=0;
  for(unsigned frame=0;frame<3;++frame) {
    renderer.BeginFrame();renderer.SetCamera(frame==1?DirectX::XMFLOAT3{0,4,-100}:DirectX::XMFLOAT3{3,3,-6},{0,1.5f,0});
    renderer.DrawStaticScene(cached,[&] {renderer.DrawAuthoredAsset(authored::Asset::Cottage,{0,0,0},{1,1,1},DirectX::XM_PI);});
    Check(cached.Instances()==1,"Authored asset did not batch into one cached instance");
    Check(renderer.Capture(L"authored-lod-"+std::to_wstring(frame)+L".png")&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Authored render/shadow validation");
    const auto geometryTriangles=renderer.Triangles()-renderer.CloudLightUpdates();
    if(!frame)nearTriangles=geometryTriangles;
    else if(frame==1)Check(geometryTriangles<nearTriangles/2,"Cached scenery failed to reduce LOD after camera movement");
    else Check(geometryTriangles==nearTriangles,"Cached scenery failed to restore near LOD");
  }
  std::cout<<"PASS: 13 assets, 39 bounded meshes, "<<triangles<<" triangles, "<<bytes<<" bytes; corrupt archive rejection, GPU/shadows and cached-scene LOD\n";
  return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
