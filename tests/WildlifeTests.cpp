#include "game/Wildlife.h"
#include "game/WorldView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <limits>
#include <cstdlib>
namespace {
void Check(bool value,const char* message) {if(!value){std::cerr<<message<<'\n';std::exit(1);}}
float Distance(rpg::Vec2 a,rpg::Vec2 b) {return std::hypot(a.x-b.x,a.z-b.z);}
void Advance(ambient::Wildlife& wildlife,const rpg::WorldMap& map,float seconds,rpg::Vec2 player) {
 for(int i=0;i<int(seconds*120);++i) wildlife.Update(1.f/120,map,player);
}
}
int main() {
 rpg::WorldMap map;ambient::Wildlife wildlife;
 const auto initial=wildlife.Animals();
 for(const auto& a:initial)Check(map.CanStand(a.position,.65f),"Wildlife spawned in obstacle");
 Advance(wildlife,map,5,{0,0});
 Check(Distance(initial[0].position,wildlife.Animals()[0].position)>.2f,"Wolf never roams");
 Check(Distance(initial[1].position,wildlife.Animals()[1].position)>.1f,"Boar never roams");
 for(unsigned species=0;species<2;++species) {
   wildlife.Reset();auto p=initial[species].position;p.z-=4;
   Advance(wildlife,map,.5f,p);
   Check(wildlife.Animals()[species].behaviour==ambient::Behaviour::Alert,"Animal ignores nearby player");
   p=initial[species].position;p.z-=1.5f;
   Advance(wildlife,map,2,p);
   Check(wildlife.Animals()[species].behaviour==ambient::Behaviour::Flee,"Animal does not flee");
   Check(Distance(wildlife.Animals()[species].position,p)>2,"Flee failed to increase distance");
   Advance(wildlife,map,25,{0,0});
   Check(wildlife.Animals()[species].behaviour!=ambient::Behaviour::Flee,"Animal remains permanently frightened");
   Check(Distance(wildlife.Animals()[species].position,initial[species].home)<8,"Animal fails to return to habitat");
 }
 // Continuous pursuit stresses obstacle probes and the map boundary.
 wildlife.Reset();
 for(int i=0;i<18000;++i) {
   const auto before=wildlife.Animals();auto p=before[0].position;p.z-=1;
   wildlife.Update(1.f/120,map,p);
   for(unsigned n=0;n<2;++n) {
     const auto& a=wildlife.Animals()[n];
     Check(map.CanStand(a.position,.65f)&&map.CanTravel(before[n].position,a.position,.65f),"Wildlife crossed collision boundary");
     Check(Distance(a.position,before[n].position)<.031f,"Wildlife teleported");
     Check(std::abs(std::remainder(a.yaw-before[n].yaw,6.2831853f))<.026f,"Wildlife snapped facing");
     Check(std::isfinite(a.gait)&&a.travel>=0&&a.travel<=1,"Invalid gait");
   }
 }
 ambient::Wildlife reference;Advance(reference,map,30,{0,0});
 for(int hz:{30,60,144,240}) {
   ambient::Wildlife sample;for(int i=0;i<30*hz;++i)sample.Update(1.f/hz,map,{0,0});
   for(unsigned i=0;i<2;++i)Check(Distance(sample.Animals()[i].position,reference.Animals()[i].position)<.04f,"Frame-rate dependent wildlife");
 }
 const auto before=wildlife.Animals();
 wildlife.Update(std::numeric_limits<float>::quiet_NaN(),map,{0,0});wildlife.Update(-1,map,{0,0});
 Check(Distance(before[0].position,wildlife.Animals()[0].position)==0,"Invalid time advanced wildlife");
 wildlife.Reset();Check(Distance(initial[0].position,wildlife.Animals()[0].position)==0&&wildlife.Animals()[0].gait==0,"Reset leaked transient state");
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);Win32Window window;Renderer renderer;
 Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Wildlife checks",false)&&renderer.Initialize(window.GetHandle(),1280,720),"Native wildlife renderer");
 const auto root=std::filesystem::current_path()/"wildlife-check";std::filesystem::create_directories(root);
 for(unsigned species=0;species<2;++species)for(unsigned stage=0;stage<3;++stage) {
   wildlife.Reset();
   auto player=initial[species].position;player.z-=1.5f;
   if(stage)Advance(wildlife,map,stage==1?4.8f:.8f,stage==1?rpg::Vec2{0,0}:player);
   const auto& a=wildlife.Animals()[species];
   renderer.BeginFrame();renderer.SetCamera({3,2.2f,-4},{0,.7f,0});
   renderer.DrawMesh(visual::Mesh::Box,{0,-.08f,0},{12,.16f,12},{.3f,.35f,.2f,1},visual::Material::Ground);
   renderer.DrawAnimatedAsset(species==0?authored::Asset::Wolf:authored::Asset::Boar,{0,0,0},3.5f,float(stage),a.gait,0,a.travel);
   renderer.DrawText(.03f,.04f,std::string(species==0?"FROSTFANG":"THORNBACK")+" / "+(stage==0?"REST":stage==1?"ROAM":"FLEE"),{1,.85f,.5f,1},2);
   Check(renderer.Capture((root/(std::to_string(species)+"-"+std::to_string(stage)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Wildlife capture/shadows");
 }
 WorldView world;
 for(unsigned stage=0;stage<3;++stage) {
   world.Reset();
   const float seconds=stage==0?0:stage==1?5.f:1.5f;
   for(int i=0;i<int(seconds*120);++i)world.UpdateWildlife(1.f/120,map,stage==2?rpg::Vec2{-18,15.5f}:rpg::Vec2{0,0},true);
   renderer.BeginFrame();renderer.SetCamera({-12,5,10},{-18,1,17});world.Draw(renderer,map,seconds);
   Check(renderer.Capture((root/("habitat-"+std::to_string(stage)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Live habitat render");
 }
 std::cout<<"PASS: roaming, alert/flee/return, swept clearance, bounded speed/turn, reset, frame rates, native pose/shadow and habitat captures\n";
}
