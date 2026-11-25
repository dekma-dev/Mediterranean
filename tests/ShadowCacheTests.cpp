#include "engine/Renderer.h"
#include "game/Game.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
using Bytes=std::vector<char>;
Bytes Capture(Renderer& r,const std::filesystem::path& path){
  Check(r.Capture(path.wstring()) && r.EndFrame(false),"Capture shadow cache");
  Check(r.ValidateDebugMessages(),"Shadow cache D3D validation");
  std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};
}
void World(Renderer& r,const std::filesystem::path& out){
  auto game=std::make_unique<Game>();game->Initialize();
  unsigned rebuilt=0;
  for(int pose=0;pose<6;++pose){
    game->ExecuteFrame({},.025f);game->SetCameraOrbit(pose*.65f,.25f+pose*.06f,5.f+pose*3);
    if(pose==4)game->OpenPanel(ui::Panel::Character);
    if(pose==5){game->OpenPanel(ui::Panel::Inventory);Check(r.Resize(1280,720),"Resize cache renderer");}
    r.SetStaticShadowCacheEnabled(true);
    Check(r.BeginFrame(),"World miss begin");game->Render(r);auto miss=Capture(r,out/(std::to_string(pose)+"-miss.png"));
    rebuilt+=r.ShadowCacheStats().rebuilds;
    if(pose==0)Check(r.ShadowCacheStats().rebuilds==1,"World cache did not build");
    Check(r.BeginFrame(),"World hit begin");game->Render(r);auto hit=Capture(r,out/(std::to_string(pose)+"-hit.png"));
    Check(r.ShadowCacheStats().hits==1 && r.ShadowCacheStats().avoidedTriangles>1000000,"World static shadows were not reused");
    Check(r.ShadowCacheStats().keyUpdates==0 && r.ShadowCacheKeyBytes()<=Renderer::maxWorldInstances*68,
      "Stable shadows rewrote their keys or exceeded their storage budget");
    const auto hitTriangles=r.Triangles();Check(miss==hit,"Cache hit changed the world image");
    r.SetStaticShadowCacheEnabled(false);
    Check(r.BeginFrame(),"World reference begin");game->Render(r);auto reference=Capture(r,out/(std::to_string(pose)+"-reference.png"));
    Check(hit==reference && hitTriangles<r.Triangles(),"Cached shadows differ from the original complete pass or save no triangles");
  }
  Check(rebuilt>1,"Camera LOD changes did not refresh static depth");
}
void Invalidation(Renderer& r,const std::filesystem::path& out){
  visual::StaticScene scene;float wall=1,dynamic=0,red=.6f;bool include=true;
  const auto draw=[&](const std::string& name){
    Check(r.BeginFrame(),"Fixture begin");r.SetCamera({5,5,-8},{0,0,0});
    r.DrawStaticScene(scene,[&]{
      r.DrawMesh(visual::Mesh::Box,{0,-.2f,0},{12,.2f,12},{.6f,.6f,.6f,1},visual::Material::Plain);
      if(include)r.DrawMesh(visual::Mesh::Box,{wall,1,0},{1,2,1},{red,.4f,.2f,1},visual::Material::Plain);
    });
    r.DrawMesh(visual::Mesh::Box,{dynamic,.6f,-1.5f},{.6f,1.2f,.6f},{.2f,.3f,.7f,1},visual::Material::Plain);
    return Capture(r,out/(name+".png"));
  };
  r.SetStaticShadowCacheEnabled(true);draw("initial");Check(r.ShadowCacheStats().rebuilds==1,"Initial depth");
  const auto first=draw("stable");Check(r.ShadowCacheStats().hits==1,"Stable depth not reused");
  red=.2f;scene.Invalidate();const auto tinted=draw("tinted");
  Check(r.ShadowCacheStats().hits==1 && r.ShadowCacheStats().keyUpdates==0 && tinted!=first,"Tint invalidated depth or failed to change color");
  r.SetStaticShadowCacheEnabled(false);Check(tinted==draw("tinted-reference"),"Depth key omitted a necessary shader input");
  r.SetStaticShadowCacheEnabled(true);
  dynamic=2;const auto moved=draw("dynamic");Check(r.ShadowCacheStats().hits==1 && first!=moved,"Dynamic caster froze");
  wall=-2;scene.Invalidate();const auto changed=draw("changed");Check(r.ShadowCacheStats().rebuilds==1,"Moved static geometry retained old depth");
  r.SetStaticShadowCacheEnabled(false);Check(changed==draw("changed-reference"),"Rebuilt depth changed pixels");
  r.SetStaticShadowCacheEnabled(true);draw("restore");include=false;scene.Invalidate();const auto removed=draw("removed");
  Check(r.ShadowCacheStats().rebuilds==1,"Removed static caster retained depth");
  r.SetStaticShadowCacheEnabled(false);Check(removed==draw("removed-reference"),"Removed caster left ghost shadow");
}
}
int main()try{
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"COM");
  const auto out=std::filesystem::current_path()/"shadow-cache-review";std::filesystem::create_directories(out);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),960,600,L"Shadow cache validation",false),"Window");
  auto r=std::make_unique<Renderer>();Check(r->Initialize(window.GetHandle(),960,600),"Renderer");
  World(*r,out);Invalidation(*r,out);
  r.reset();r=std::make_unique<Renderer>();Check(r->Initialize(window.GetHandle(),960,600),"Reconstructed renderer");
  Invalidation(*r,out);
  std::cout<<"PASS: exact native depth reuse, live casters, static move/removal, camera/LOD, portraits, resize and reconstruction\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
