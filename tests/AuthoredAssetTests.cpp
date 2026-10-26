#include "engine/AuthoredAssets.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include "game/AuthoredActorMotion.h"
#include "game/EnemyDefeat.h"
#include "game/Game.h"
#include "tests/DashFixture.h"
#include <filesystem>
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
  for(unsigned a=0;a<unsigned(authored::Asset::Oak);++a) {
    const auto asset=authored::Asset(a);
    const auto rest=authored::Pose(asset,0),moving=authored::Pose(asset,1,.8f,.6f,1);
    float displacement=0;
    for(unsigned lod=0;lod<authored::lodCount;++lod) {
      for(const auto& vertex:library[a*authored::lodCount+lod].geometry.vertices) {
        const auto weights=authored::Influence(asset,vertex.position);
        Check(weights.bones[0]<visual::bodyBones && weights.weights.x>=0 && weights.weights.y>=0 && std::abs(weights.weights.x+weights.weights.y-1)<1e-6f,"Invalid animation influence");
        const auto bind=visual::SkinVertex(vertex,weights,rest),posed=visual::SkinVertex(vertex,weights,moving);
        Check(std::abs(bind.position.x-vertex.position.x)+std::abs(bind.position.y-vertex.position.y)+std::abs(bind.position.z-vertex.position.z)<1e-5f,"Bind pose changed mesh");
        Check(std::isfinite(posed.position.x)&&std::isfinite(posed.position.y)&&std::isfinite(posed.position.z),"Nonfinite animated vertex");
        displacement+=std::abs(posed.position.y-bind.position.y)+std::abs(posed.position.z-bind.position.z);
      }
      renderer.BeginFrame();renderer.SetCamera({3,2,-5},{0,1,0});
      renderer.DrawAnimatedAsset(asset,{0,0,0},DirectX::XM_PI,1,.8f,.6f,1,lod);
      Check(renderer.Capture(L"authored-motion-"+std::to_wstring(a)+L"-"+std::to_wstring(lod)+L".png")&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Animated color/shadow/LOD render failed");
    }
    Check(displacement>1,"Actor remained static");
  }
  visual::StaticScene cached;
  for(int hz:{30,60,144,240}) {
    EnemyDefeat death;death.Reset(true);animation::Sample pose;pose.travel=.7f;pose.action.pitch=.12f;
    death.Update(0,false,.4f,1,pose);
    Check(death.Gesture().collapse==0&&death.attack==.4f,"Death onset lost pose");
    float last=0;
    for(int i=0;i<hz*2;++i) {
      death.Update(1.f/hz,false,0,0,{});const float collapse=death.Gesture().collapse;
      Check(collapse>=last&&collapse<=1,"Collapse is non-monotonic");last=collapse;
    }
    Check(death.Gesture().collapse==1&&death.Remaining()==0,"Death does not settle");
    death.Reset(false);Check(death.Gesture().collapse==1,"Loaded corpse replays death");
    death.Update(.01f,true,0,0,{});Check(death.age<0&&death.Gesture().collapse==0,"Revival retained collapse");
  }
  const auto captures=std::filesystem::current_path()/"authored-combat-check";
  std::filesystem::create_directories(captures);
  for(const auto asset:{authored::Asset::Skeleton,authored::Asset::Orc,authored::Asset::Revenant}) {
    const auto id=asset==authored::Asset::Revenant?rpg::ClassId::Mage:asset==authored::Asset::Orc?rpg::ClassId::Demonologist:rpg::ClassId::Warrior;
    const auto clip=asset==authored::Asset::Skeleton?animation::ActionClip::Basic:animation::ActionClip::Heavy;
    const auto rest=authored::Pose(asset,0);
    for(unsigned stage=0;stage<5;++stage) {
      animation::Sample sample;
      if(stage>0 && stage<4)sample.action=animation::SampleAction(id,clip,animation::ActionDuration(clip)*(.15f+.25f*stage));
      if(stage==4){sample.hit=.8f;sample.hitSide=.7f;}
      const auto gesture=AuthoredActorGesture(sample);
      const auto posed=authored::Pose(asset,0,0,0,0,gesture);
      float moved=0;
      for(unsigned lod=0;lod<authored::lodCount;++lod) {
        for(const auto& v:library[unsigned(asset)*authored::lodCount+lod].geometry.vertices) {
          const auto influence=authored::Influence(asset,v.position);
          const auto p=visual::SkinVertex(v,influence,posed);
          const float d=std::abs(p.position.x-v.position.x)+std::abs(p.position.y-v.position.y)+std::abs(p.position.z-v.position.z);
          Check(std::isfinite(d)&&d<2,"Combat gesture exploded mesh");
          if(v.position.y<.7f && std::abs(v.position.x)<.3f)Check(d<1e-5f,"Upper-body gesture displaced planted legs");
          moved+=d;
        }
      }
      Check(stage?moved>.01f:moved<.01f,"Gesture missing or neutral pose changed");
      renderer.BeginFrame();renderer.SetCamera({3,2,-5},{0,1,0});
      renderer.DrawMesh(visual::Mesh::Box,{0,-.06f,0},{8,.12f,8},{.22f,.25f,.23f,1},visual::Material::Stone);
      renderer.DrawAnimatedAsset(asset,{0,0,0},DirectX::XM_PI,0,0,0,0,0,{1,1,1,1},gesture);
      renderer.DrawText(.02f,.025f,std::string(authored::Name(asset))+" / "+(stage==0?"NEUTRAL":stage==4?"HIT RESPONSE":"ACTION "+std::to_string(stage)),{1,.86f,.6f,1},1.8f);
      Check(renderer.Capture((captures/(std::to_string(unsigned(asset))+"-"+std::to_string(stage)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Combat gesture render/shadows");
    }
    authored::Gesture invalid;invalid.pitch=std::numeric_limits<float>::quiet_NaN();invalid.leftLift=std::numeric_limits<float>::infinity();
    const auto sanitized=authored::Pose(asset,0,0,0,0,invalid);
    Check(std::memcmp(&sanitized,&rest,sizeof(rest))==0,"Invalid gesture was not sanitized to neutral");
    for(unsigned stage=0;stage<5;++stage) {
      EnemyDefeat defeat;defeat.Reset(true);defeat.Update(EnemyDefeat::duration*stage/4,false,0,0,{});
      const auto gesture=defeat.Gesture();const auto palette=authored::Pose(asset,0,0,0,0,gesture);
      for(unsigned lod=0;lod<authored::lodCount;++lod)for(const auto& v:library[unsigned(asset)*authored::lodCount+lod].geometry.vertices) {
        const auto p=visual::SkinVertex(v,authored::Influence(asset,v.position),palette);
        Check(std::isfinite(p.position.y),"Death pose nonfinite");
        if(stage==4)Check(p.position.y>=-.12f&&p.position.y<1.3f,"Corpse is underground or standing");
      }
      renderer.BeginFrame();renderer.SetCamera({3,2,-5},{0,.65f,.6f});
      renderer.DrawMesh(visual::Mesh::Box,{0,-.06f,0},{8,.12f,8},{.22f,.25f,.23f,1},visual::Material::Stone);
      renderer.DrawAnimatedAsset(asset,{0,0,0},DirectX::XM_PI,0,0,0,0,0,{1,1,1,1},gesture);
      Check(renderer.Capture((captures/("death-"+std::to_string(unsigned(asset))+"-"+std::to_string(stage)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Death color/shadow capture");
    }
  }
  unsigned nearTriangles=0;
  {
    std::string fixture=test::dashFixture;
    fixture.replace(fixture.find("-4 2 100"),std::strlen("-4 2 100"),"-4 5.5 100");
    fixture.replace(fixture.find("100 -4 7 80"),11,"100 -4 7 1");
    Game game;game.Initialize();Check(game.RestoreSnapshot(fixture),"Death integration fixture");
    rpg::Command target;target.nextTarget=true;game.ExecuteCommand(target,1.f/120);
    rpg::Command strike;strike.ability=0;game.ExecuteCommand(strike,1.f/120);
    for(unsigned i=0;i<240;++i)game.ExecuteCommand({},1.f/120);
    Check(game.State().Enemies()[0].health<=0&&!game.State().Enemies()[0].looted,"Death test failed to defeat enemy");
    const auto saved=game.State().Save();
    game.SetCameraOrbit(.4f,.6f,5);
    renderer.BeginFrame();game.Render(renderer,false);
    Check(renderer.Capture((captures/"corpse-world.png").wstring())&&renderer.EndFrame(false),"World corpse render");
    Check(game.RestoreSnapshot(saved)&&game.State().Save()==saved,"Corpse load changed save");
    renderer.BeginFrame();game.Render(renderer,false);Check(renderer.EndFrame(false),"Loaded corpse render");
    rpg::Command loot;loot.interact=true;game.ExecuteCommand(loot,1.f/120);
    Check(game.State().Enemies()[0].looted,"Corpse blocked existing loot interaction");
  }
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
