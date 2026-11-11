#include "game/WorldView.h"
#include "game/WorldMap.h"
#include "game/Simulation.h"
#include "game/TerrainSurface.h"
#include "game/Grove.h"
#include "game/LocationView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
struct View {const char* name;DirectX::XMFLOAT3 eye,target;};
// Absolute cameras, exposure and time are identical in before/after captures.
constexpr View views[]{
  {"village",{0,5,-18},{0,1,3}},
  {"village-west",{-18,7,-18},{-6,1,6}},
  {"north-road",{4,10,19},{-4,6,46}},
  {"western-wood",{-37,11,18},{-59,8,20}},
  {"farm",{-19,10,-50},{-33,2,-35}},
  {"mist-ridge",{-61,14,-78},{-84,14,-92}},
  {"province",{0,155,-180},{0,0,0}}
};
std::string PlaceRecord(std::string data,const std::string& key,rpg::Vec2 point) {
  auto start=data.find(key);Check(start!=data.npos,"Missing save fixture record");start+=key.size();
  const auto end=data.find(' ',data.find(' ',start)+1);
  data.replace(start,end-start,std::to_string(point.x)+" "+std::to_string(point.z));return data;
}
void LayoutChecks() {
  using namespace rpg;
  WorldMap map;
  Check(map.Height(WorldMap::spawn)==0,"Market floor moved");
  Check(map.Height({-16,8})>.7f && map.Height({-10,24})>1.5f && map.Height({28,13})>1.4f,"Missing village banks");
  const auto& heights=terrain::Heights();
  for(int z=0;z<terrain::rows-1;++z)for(int x=0;x<terrain::columns-1;++x) {
    const auto i=z*terrain::columns+x;
    for(int corner:{0,1}) {
      const float a=corner?heights[i+terrain::columns+1]-heights[i+terrain::columns]:heights[i+1]-heights[i];
      const float b=corner?heights[i+terrain::columns+1]-heights[i+1]:heights[i+terrain::columns]-heights[i];
      Check(std::hypot(a,b)/terrain::cell<.68f,"Unwalkable terrain triangle");
    }
  }
  for(const auto& n:WorldMap::residents) {
    Check(map.CanStand(n.position,.45f),"NPC footprint blocked");
    Check(!map.FindPath(WorldMap::spawn,n.position).empty(),"NPC inaccessible");
  }
  for(const auto& t:map.Trails())Check(map.CanTravel(t.from,t.to,.6f),"Route blocked by new scenery");
  unsigned trees=0,details=0,shrubs=0;
  Vec2 covered{};bool found=false;
  for(const auto& o:map.Obstacles())if(o.introducedVersion==WorldMap::sceneryVersion) {
    trees+=o.kind==PropKind::Tree;details+=o.kind==PropKind::Dressing;
    Vec2 old=o.center;map.RecoverOldSceneryPosition(old,.45f,23);
    Check(map.CanStand(old,.45f) && std::hypot(old.x-o.center.x,old.z-o.center.z)<=6.01f,"Old actor cannot escape new scenery");
    Vec2 current=o.center;map.RecoverOldSceneryPosition(current,.45f,24);
    Check(current.x==o.center.x && current.z==o.center.z,"Current save inside scenery silently accepted");
    if(!found){covered=o.center;found=true;}
  }
  for(const auto& c:map.Cover()) {
    shrubs+=c.shrub;const auto s=map.SurfaceAt(c.position);
    Check(s.dirt+s.paving<=.04f && map.CanStand(c.position,.35f),"Vegetation blocks a road/solid prop");
  }
  Check(trees>=100 && details>=40,"Missing forest/inhabited landscape groups");
  Check(map.Cover().size()<=WorldMap::maxCover && shrubs<=WorldMap::maxShrubs,"Ground cover budget exceeded");
  Simulation simulation;
  const auto original=simulation.Save();auto old=original;old.replace(old.find("24"),2,"23");
  const auto start=old.find('\n',old.find("PROFILE "))+1;
  const auto end=old.find(' ',old.find(' ',start)+1);
  old.replace(start,end-start,std::to_string(covered.x)+" "+std::to_string(covered.z));
  Check(simulation.Load(old) && map.CanStand(simulation.Player(),.4f),"v23 hero migration failed");
  Simulation roundtrip;Check(roundtrip.Load(simulation.Save()) && roundtrip.Save()==simulation.Save(),"Migrated save is not stable");
  old.replace(old.find("23"),2,"24");Check(!roundtrip.Load(old),"Malformed current-version hero accepted");
  Check(roundtrip.Save()==simulation.Save(),"Rejected load changed live state");
  // Wildlife positions have their own parser: exercise migration inside a new
  // footprint within the original leash, including current-version rejection.
  bool beastChecked=false;
  for(const auto& b:simulation.Beasts())for(const auto& o:map.Obstacles()) {
    if(beastChecked || o.introducedVersion!=24 || std::hypot(b.home.x-o.center.x,b.home.z-o.center.z)>20)continue;
    auto fixture=PlaceRecord(original,"\n"+std::to_string(b.id)+" ",o.center);fixture.replace(fixture.find("24"),2,"23");
    Check(roundtrip.Load(fixture),"v23 wildlife migration failed");
    fixture.replace(fixture.find("23"),2,"24");Check(!roundtrip.Load(fixture),"Malformed current wildlife position accepted");beastChecked=true;
  }
  Check(beastChecked,"Missing wildlife migration fixture");
  const auto weights=BuildTerrainSurface(map);unsigned litter=0,open=0;
  for(std::size_t i=0;i<weights.size();i+=4) {
    Check(unsigned(weights[i])+weights[i+1]<=255 && weights[i+2]<=90,"Terrain data channels invalid");
    litter+=weights[i+3]>40;open+=weights[i+3]==0;
  }
  Check(litter>500 && open>20000,"Canopy mask lost forest/open-ground distinction");
  std::cout<<"Layout: "<<trees<<" added trees, "<<details<<" solid details, "<<map.Cover().size()<<" cover patches, "<<shrubs<<" shrubs\n";
}
void ExportBounds(const rpg::WorldMap& map) {
  using namespace rpg;
  for(const auto& o:map.Obstacles())if(o.kind==PropKind::Tree) {
    const auto profile=grove::Tree(o);const auto scale=grove::Size(o);
    for(unsigned lod=0;lod<3;++lod) {
      const auto& bounds=authored::Collection()[(unsigned(authored::Asset::Oak)+profile.species)*3+lod].bounds;
      const float radius=std::hypot(std::max(std::abs(bounds.minimum.x),std::abs(bounds.maximum.x)),std::max(std::abs(bounds.minimum.z),std::abs(bounds.maximum.z)));
      Check(radius*scale.width<=o.canopyRadius && bounds.maximum.y*scale.height<=o.CameraHeight(),"Crown escapes camera envelope");
    }
  }
  for(const auto& detail:map.Details()) {
    if(detail.kind==LandscapeKind::Boulder || detail.kind==LandscapeKind::Lantern)continue;
    const auto o=std::find_if(map.Obstacles().begin(),map.Obstacles().end(),[&](const Obstacle& o){return o.kind==PropKind::Dressing && o.center.x==detail.position.x && o.center.z==detail.position.z;});
    Check(o!=map.Obstacles().end(),"Authored prop lacks collision");
    for(unsigned lod=0;lod<3;++lod) {
      const auto& bounds=authored::Collection()[unsigned(locationview::DetailAsset(detail.kind))*3+lod].bounds;
      for(float x:{bounds.minimum.x,bounds.maximum.x})for(float z:{bounds.minimum.z,bounds.maximum.z}) {
        const float cx=std::cos(detail.yaw),sx=std::sin(detail.yaw);
        Check(std::abs(x*cx+z*sx)*detail.scale<=o->halfX && std::abs(z*cx-x*sx)*detail.scale<=o->halfZ,"Authored prop extends beyond collision");
      }
      Check(bounds.maximum.y*detail.scale<=o->CameraHeight(),"Prop extends above camera envelope");
    }
  }
}
}
int main(int argc,char** argv) try {
  LayoutChecks();
  const auto root=argc>1?std::filesystem::path(argv[1]):std::filesystem::current_path()/"location-check";
  std::filesystem::create_directories(root);
  rpg::WorldMap map;WorldView world;
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Location checks",false),"Hidden window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer initialization");
  ExportBounds(map);
  Check(renderer.SetTerrainSurface(BuildTerrainSurface(map),terrainSurfaceSize),"Terrain data upload");
  std::ofstream report(root/"metrics.txt");report<<renderer.AdapterDiagnostics();
  report<<"\nObstacles "<<map.Obstacles().size()<<"; cover "<<map.Cover().size()<<'\n';
  for(const auto& view:views) {
    std::vector<double> timings;unsigned triangles=0;
    const auto firstFrame=renderer.GpuFrame()+1,firstMeasured=firstFrame+8;
    for(int frame=0;frame<64;++frame) {
      renderer.BeginFrame();renderer.SetCamera(view.eye,view.target);
      renderer.SetShadowFocus(view.target);renderer.SetAtmosphereTime(120);
      world.Draw(renderer,map,0,false,{}, {},{view.target.x,view.target.z});
      Check(renderer.EffectVertices()<=192 && renderer.CombatLightCount()<=4,"Environmental effects exceeded bounded budget");
      // Hidden swap chains can defer timestamp queries until a readback. Drain
      // every eighth frame, and exclude those frames from reported GPU timing.
      if(frame%8==7)Check(renderer.Capture((root/(frame==63?std::string(view.name)+".png":"render-sync.png")).wstring()),"Location capture");
      if(!renderer.EndFrame(false)) {std::wcerr<<renderer.Error()<<'\n';Check(false,"Location exceeded render capacity");}
      triangles=renderer.Triangles();
      for(unsigned i=0;i<renderer.GpuReadyCount();++i) {
        const auto sample=renderer.GpuReady()[i];
        if(sample.frame>=firstMeasured && (sample.frame-firstFrame)%8!=7)timings.push_back(sample.milliseconds);
      }
    }
    std::sort(timings.begin(),timings.end());
    report<<view.name<<": "<<triangles<<" triangles, GPU median "<<(timings.empty()?0:timings[timings.size()/2])<<" ms\n";
  }
  report<<"Static instances "<<world.StaticInstances()<<"; bytes "<<world.StaticBytes()<<'\n';
  Check(world.StaticInstances()<7000 && world.StaticBytes()<1400000,"Retained scenery exceeds content budget");
  Check(renderer.ValidateDebugMessages(),"D3D debug errors in location views");
  std::cout<<"PASS: seven fixed location cameras, instance capacity and native D3D captures\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
