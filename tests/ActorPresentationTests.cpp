#include "game/ActorPresentation.h"
#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void Check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
rpg::Enemy Creature(rpg::EnemyKind kind,rpg::EntityId id=400) {
 rpg::Enemy e;e.id=id;e.kind=kind;e.position=e.home={0,-10};e.health=e.MaxHealth();return e;
}
void Binding() {
 auto wolf=Creature(rpg::EnemyKind::Wolf),boar=Creature(rpg::EnemyKind::Boar,401);
 auto trainer=Creature(rpg::EnemyKind::Trainer,200);
 std::vector<rpg::Enemy> enemies{wolf,boar};std::vector<presentation::Actor> actors;
 presentation::Bind(actors,enemies,trainer);Check(actors.size()==3,"Initial actor binding");
 actors[0].motion.phase=2.3f;actors[0].animation.pose.hit=.7f;actors[0].beast.jaw=.8f;
 actors[1].motion.phase=1.2f;actors[2].animation.enemyReleaseAge=.123f;
 std::swap(enemies[0],enemies[1]);presentation::Bind(actors,enemies,trainer);
 Check(actors[0].motion.phase==1.2f && actors[1].motion.phase==2.3f && actors[1].beast.jaw==.8f,
   "Reordering transferred animation to another entity");
 enemies.insert(enemies.begin(),Creature(rpg::EnemyKind::Wolf,402));presentation::Bind(actors,enemies,trainer);
 Check(actors[0].motion.phase==0 && actors[2].animation.pose.hit==.7f && actors.back().animation.enemyReleaseAge==.123f,
   "Roster growth reset surviving actors");
 enemies[2].health=0;presentation::Bind(actors,enemies,trainer);
 Check(actors[2].motion.phase==2.3f,"Death erased the outgoing pose before collapse");
 enemies[2].health=80;presentation::Bind(actors,enemies,trainer);
 Check(actors[2].motion.phase==0 && actors[2].beast.jaw==0 && actors[2].animation.pose.hit==0 && actors[2].defeat.age<0,
   "Respawn inherited an old hit, bite or corpse pose");
 actors[1].motion.phase=2;enemies[1].kind=rpg::EnemyKind::Captain;presentation::Bind(actors,enemies,trainer);
 Check(actors[1].motion.phase==0,"Reused ID with different rig retained stale animation");
 actors[1].motion.phase=2;enemies[1].home.x+=5;presentation::Bind(actors,enemies,trainer);
 Check(actors[1].motion.phase==0,"Reused campaign ID retained another spawn's animation");
 actors[1].motion.phase=2;++enemies[1].rank;presentation::Bind(actors,enemies,trainer);
 Check(actors[1].motion.phase==0,"Reused campaign ID retained the previous rank's pose");
 actors[1].motion.phase=2;enemies[1].position.x+=10;presentation::Bind(actors,enemies,trainer);
 Check(actors[1].motion.phase==0 && actors[1].movement.x==enemies[1].position.x,"Teleport carried stale foot/movement history");
 const auto* storage=actors.data();for(int i=0;i<100;++i)presentation::Bind(actors,enemies,trainer);
 Check(storage==actors.data(),"Stable roster allocated every frame");
 wolf.windup=.15f;presentation::Actor resumed;resumed.Reset(wolf);
 Check(resumed.motion.attack>.8f && resumed.beast.jaw>.8f,"Loading a bite flashed the idle pose");
}
void Locomotion() {
 const auto& clips=authored::wolf::Collection().clips;
 for(unsigned i=1;i<4;++i) {
   const float travel=authored::wolf::TravelAtSpeed(clips[i].distance/clips[i].duration);
   const auto gait=authored::wolf::Blend(travel);
   Check(std::abs(gait.stride-clips[i].distance)<1e-5f,"Wolf stride ignores authored clip distance");
 }
 for(auto kind:{rpg::EnemyKind::Wolf,rpg::EnemyKind::Boar}) {
   float reference=0;
   for(int hz:{1000,240,144,60,30,22}) {
     presentation::BeastLocomotion b;b.Reset(400);auto e=Creature(kind);motion::MovementSample m;m.forward=1;
     // Fixed physical path, including starts, a fast run, slow travel and a stop.
     for(float speed:{1.1f,4.2f,.65f,0.f}) {
       const int steps=hz;for(int i=0;i<steps;++i) {
         e.movementDistance=speed/hz;e.position.z+=e.movementDistance;
         b.Update(1.f/hz,e,m,{},1,true);
       }
     }
     if(hz==1000)reference=b.phase;
     else Check(std::abs(motion::AngleDelta(reference,b.phase))<.012f,"Gait phase depends on frame rate");
     Check(b.travel<1e-4f && b.speed<1e-4f,"Stopped animal kept cycling its legs");
     const auto phase=b.phase;e.movementDistance=0;
     b.Update(.05f,e,m,{},1,true);Check(phase==b.phase,"Stationary animal advanced stride");
   }
   auto e=Creature(kind);presentation::BeastLocomotion b;b.Reset();motion::MovementSample m;m.forward=1;
   b.speed=2;e.movementDistance=.02f;b.Update(.01f,e,m,{},1,true);const float forward=b.phase;
   m.forward=-1;b.Update(.01f,e,m,{},1,true);
   Check(forward>0 && b.phase<forward,"Backward navigation still played a forward stride");
 }
}
void SupportAndCombat() {
 const auto plane=[](rpg::Vec2 p){return .2f*p.z+.1f*p.x;};
 const auto support=presentation::BeastLocomotion::Support({0,0},0,true,plane);
 Check(support.supportPitch<-.19f && support.supportRoll>.09f && std::abs(support.supportLift)<1e-6f,
   "Terrain support signs or height are wrong");
 const auto reverse=presentation::BeastLocomotion::Support({0,0},DirectX::XM_PI,true,plane);
 Check(reverse.supportPitch>.19f && reverse.supportRoll<-.09f,"Support did not rotate with actor heading");
 const auto cliff=presentation::BeastLocomotion::Support({0,0},0,true,[](rpg::Vec2 p){return p.z*10+p.x*10;});
 Check(std::abs(cliff.supportPitch)<=.38f && std::abs(cliff.supportRoll)<=.30f,"Terrain over-rotates creature");
 for(auto kind:{rpg::EnemyKind::Wolf,rpg::EnemyKind::Boar}) {
   auto e=Creature(kind);presentation::BeastLocomotion b;b.Reset();motion::MovementSample m;
   animation::Sample hit;hit.hit=.8f;hit.hitSide=.7f;hit.bank=.5f;
   b.Update(.03f,e,m,hit,1,true);
   const auto asset=kind==rpg::EnemyKind::Wolf?authored::Asset::Wolf:authored::Asset::Boar;
   const auto rest=authored::Pose(asset,0),impact=authored::Pose(asset,0,0,0,0,b.gesture);
   const unsigned body=kind==rpg::EnemyKind::Wolf?3:0;
   Check(std::abs(impact.joints[body]._23-rest.joints[body]._23)>.03f,"Damage does not reach authored body");
   if(kind==rpg::EnemyKind::Wolf) {
     authored::Gesture bite;bite.jaw=1;const auto open=authored::Pose(asset,0,0,0,0,bite);
     // An existing muzzle point cannot separate far from its rigid head-bound
     // nose even at maximum gape. This guards the accepted model's mixed weights.
     const auto point=DirectX::XMVectorSet(0,.86f,1.10f,1);
     const auto head=DirectX::XMVector3TransformCoord(point,DirectX::XMLoadFloat4x4(&open.joints[5]));
     const auto jaw=DirectX::XMVector3TransformCoord(point,DirectX::XMLoadFloat4x4(&open.joints[6]));
     Check(DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(head,jaw)))<.05f,"Bite tears the muzzle away from the nose");
   }
   // Root support moves joints and normals together; the original bind remains exact.
   const auto tilted=authored::Pose(asset,0,0,0,0,support);
   DirectX::XMFLOAT3 up;DirectX::XMStoreFloat3(&up,DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0,1,0,0),DirectX::XMLoadFloat4x4(&tilted.normals[body])));
   Check(up.x<-.09f && up.z<-.19f,"Normal palette did not follow slope support");
   auto invalid=b.gesture;invalid.supportPitch=std::numeric_limits<float>::quiet_NaN();invalid.roll=std::numeric_limits<float>::infinity();
   const auto finite=authored::Pose(asset,1,1,.5f,.5f,invalid);
   for(unsigned bone=0;bone<(kind==rpg::EnemyKind::Wolf?30u:10u);++bone)
     Check(std::isfinite(finite.joints[bone]._11+finite.joints[bone]._42),"Invalid gesture contaminated palette");
 }
 auto e=Creature(rpg::EnemyKind::Wolf);presentation::BeastLocomotion b;b.Reset();motion::MovementSample m;
 for(int i=0;i<40;++i){e.windup=.1f;b.Update(.01f,e,m,{},1,true);}
 Check(b.jaw>.8f,"Bite anticipation missing");
 const float jaw=b.jaw;e.windup=0;b.Update(.01f,e,m,{},0,true);
 Check(b.jaw>.8f && std::abs(b.jaw-jaw)<.08f,"Bite contact snapped jaw");
 for(int i=0;i<50;++i)b.Update(.01f,e,m,{},1,false);
 Check(b.jaw<.001f,"Cancelled bite left jaw open");
 b.gesture.jaw=.8f;b.gesture.supportPitch=-.15f;EnemyDefeat defeat;defeat.Reset(true);
 defeat.UpdatePose(0,false,.7f,1.2f,.6f,b.gesture,2.3f);
 Check(defeat.time==2.3f && defeat.gait==1.2f && defeat.Gesture().jaw==.8f,"Death lost the visible clip/jaw clock");
 defeat.UpdatePose(.5f,false,0,0,0,{},0);
 Check(defeat.Gesture().supportPitch==-.15f && defeat.time==2.3f,"Corpse lost its terrain support or frozen clock");
}
void Residents() {
 WorldView world;world.ResetResidents();
 for(unsigned i=0;i<rpg::WorldMap::residents.size();++i) {
   const auto p=rpg::WorldMap::residents[i].position;
   for(int tick=0;tick<120;++tick)world.UpdateResidents(1.f/120,{p.x+.8f,p.z-2},true,true,p);
   Check(world.ResidentPose(i).yaw<-.25f && world.ResidentPose(i).smile>.45f,"Resident does not acknowledge nearby dialogue");
 }
 for(int tick=0;tick<180;++tick)world.UpdateResidents(1.f/120,{0,0},false,false,{});
 for(unsigned i=0;i<rpg::WorldMap::residents.size();++i)Check(std::abs(world.ResidentPose(i).yaw)<.001f,"Resident retained attention after player death");
}
void Captures() {
 Win32Window window;Renderer renderer;
 Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Model binding checks",false),"Hidden review window");
 Check(renderer.Initialize(window.GetHandle(),960,720),"Renderer initialization");
 const auto dir=std::filesystem::current_path()/"model-binding-check";std::filesystem::create_directories(dir);
 for(auto kind:{rpg::EnemyKind::Wolf,rpg::EnemyKind::Boar}) {
   auto e=Creature(kind);presentation::Actor actor;actor.Reset(e);
   const auto asset=kind==rpg::EnemyKind::Wolf?authored::Asset::Wolf:authored::Asset::Boar;
   for(int frame=0;frame<=180;++frame) {
     const float dt=1.f/60;e.movementDistance=frame<60?2.5f*dt:0;
     e.position.z+=e.movementDistance;actor.movement.Update(e.position.x,e.position.z,0,dt,true);
     e.windup=frame>=60&&frame<108?float(108-frame)/60:0;
     const float release=frame<108?1:float(frame-108)/60;
     animation::Sample pose;if(frame>=140&&frame<158){pose.hit=.8f;pose.hitSide=.8f;}
     actor.beast.Update(dt,e,actor.movement,pose,release,true);
     if(frame%15)continue;
     auto gesture=actor.beast.gesture;
     if(frame>=150) {gesture.supportPitch=-.20f;gesture.supportRoll=.10f;gesture.supportLift=0;}
     const float strike=e.windup>0?combat::Smooth(1-e.windup/e.WindupDuration()):1-combat::Smooth(release/.4f);
     renderer.BeginFrame();renderer.SetCamera({3,1.7f,3.5f},{0,.65f,0});
     renderer.DrawMesh(visual::Mesh::Box,{0,-.08f,0},{7,.12f,7},{.25f,.28f,.24f,1},visual::Material::Stone,0,gesture.supportPitch,visual::ShadowMode::Cast,gesture.supportRoll);
     renderer.DrawAnimatedAsset(asset,{0,0,0},0,float(actor.beast.clock),actor.beast.phase,strike,actor.beast.travel,0,{1,1,1,1},gesture);
     renderer.DrawText(.02f,.025f,std::string(authored::Name(asset))+" / "+std::to_string(frame),{1,.86f,.6f,1},1.8f);
     Check(renderer.Capture((dir/(std::to_string(unsigned(asset))+"-"+std::to_string(frame)+".png")).wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Creature color/shadow capture");
   }
 }
 // Actual game update/render integration; drawing again must not change playback.
 Game game;game.Initialize();for(int i=0;i<120;++i)game.ExecuteCommand({},1.f/60);
 const float phase=game.EnemyPose(3).phase,travel=game.EnemyAnimation(3).travel;const auto generation=game.AudioGeneration();
 for(int i=0;i<2;++i){renderer.BeginFrame();game.Render(renderer);Check(renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Game render integration");}
 Check(game.EnemyPose(3).phase==phase && game.EnemyAnimation(3).travel==travel && game.AudioGeneration()==generation,"Rendering advanced actor state");
}
}
int main() {try {
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Binding();Locomotion();SupportAndCombat();Residents();Captures();
 std::cout<<"Actor binding, stride timing, support, contact/death, residents and native renders passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
