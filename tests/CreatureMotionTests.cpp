#include "game/BeastLocomotion.h"
#include "game/CombatEffects.h"
#include "game/Game.h"
#include "engine/Audio.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <wincodec.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
using namespace DirectX;
using A=authored::Asset;
void Check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
float Distance(XMFLOAT3 a,XMFLOAT3 b) {return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
void Finite(const visual::BodyPalette& p) {
 for(unsigned i=0;i<visual::paletteBones;++i) for(const auto* matrix:{&p.joints[i],&p.normals[i]})
   for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)Check(std::isfinite(matrix->m[r][c]),"Non-finite creature palette");
}
void Solvers() {
 for(auto asset:{A::Wolf,A::Boar}) {
   const auto base=authored::Pose(asset,0);auto unchanged=base;
   authored::quadruped::Apply(unchanged,asset,{});
   Check(std::memcmp(&base,&unchanged,sizeof(base))==0,"Neutral foot constraints changed accepted bind");
   for(unsigned i=0;i<4;++i) {
     const auto paw=authored::quadruped::Paw(base,asset,i);
     auto target=paw;target.y+=.045f;target.z+=.015f;
     authored::Gesture g;g.paws[i]={target,1};auto solved=base;authored::quadruped::Apply(solved,asset,g);Finite(solved);
     const auto actual=authored::quadruped::Paw(solved,asset,i);
     Check(Distance(actual,target)<.007f,"Reachable paw target not solved");
     for(unsigned other=0;other<4;++other)if(other!=i)
       Check(Distance(authored::quadruped::Paw(solved,asset,other),authored::quadruped::Paw(base,asset,other))<1e-6f,"One leg moved another leg");
     if(asset==A::Wolf) {
       const auto leg=authored::quadruped::Limb(asset,i);
       const auto point=[&](const auto& p,unsigned bone,XMFLOAT3 bind){return authored::quadruped::Point(p,bone,bind);};
       const float before=Distance(point(base,leg.lower,leg.knee),point(base,leg.upper,leg.hip));
       const float after=Distance(point(solved,leg.lower,leg.knee),point(solved,leg.upper,leg.hip));
       Check(std::abs(after-before)<1e-5f,"Wolf IK stretches upper limb");
       Check(Distance(point(solved,leg.upper,leg.knee),point(solved,leg.lower,leg.knee))<.0001f,"Wolf knee joint disconnected");
       Check(Distance(point(solved,leg.lower,leg.wrist),point(solved,leg.ankle,leg.wrist))<.0001f,"Wolf wrist joint disconnected");
     }
     g.paws[i].target={100,-100,100};auto distant=base;authored::quadruped::Apply(distant,asset,g);Finite(distant);
     Check(Distance(paw,authored::quadruped::Paw(distant,asset,i))<.30f,"Unreachable foot tears limb");
     g.paws[i].target.x=std::numeric_limits<float>::quiet_NaN();auto invalid=base;
     authored::quadruped::Apply(invalid,asset,g);Finite(invalid);
     Check(std::memcmp(&base,&invalid,sizeof(base))==0,"Invalid paw target did not retain pose");
   }
 }
}
void Terrain() {
 const auto ground=[](rpg::Vec2 p){return .12f*p.x+.22f*p.z+.035f*std::sin(p.x*8)*std::cos(p.z*5);};
 for(auto asset:{A::Wolf,A::Boar})for(float yaw:{0.f,1.1f,3.1f}) {
   authored::Gesture gesture=presentation::BeastLocomotion::Support({0,0},yaw,asset==A::Wolf,ground);
   presentation::QuadrupedContacts feet;
   for(int frame=0;frame<120;++frame) {
     gesture.paws={};const auto base=authored::Pose(asset,0,0,0,0,gesture);
     feet.Update(1.f/120,asset,400,{0,0},yaw,0,0,0,base,gesture,ground);
     Check(feet.count==0,"Standing/spawn emitted phantom footsteps");
   }
   const auto corrected=authored::Pose(asset,0,0,0,0,gesture);Finite(corrected);
   for(unsigned i=0;i<4;++i) {
     const auto paw=authored::quadruped::Paw(corrected,asset,i);
     const rpg::Vec2 world{std::cos(yaw)*paw.x+std::sin(yaw)*paw.z,-std::sin(yaw)*paw.x+std::cos(yaw)*paw.z};
     const float error=std::abs(paw.y-ground(world)-(asset==A::Wolf?.065f:.08f));
     Check(error<.025f,"Standing paw failed rough slope contact");
   }
   gesture.paws={};auto base=authored::Pose(asset,0);
   feet.Update(.02f,asset,400,{20,20},yaw,0,.5f,2,base,gesture,ground);
   Check(feet.count==0,"Teleport emitted footsteps");
   feet.Reset();Check(feet.count==0 && !feet.Planted(0),"Contact reset retained anchors");
 }
}
void RatesAndLifecycle() {
 const auto flat=[](rpg::Vec2){return 0.f;};
 for(auto asset:{A::Wolf,A::Boar}) {
   int reference=-1;
   for(int hz:{22,30,60,144,1000}) {
     presentation::QuadrupedContacts feet;authored::Gesture gesture;unsigned events=0;
     const float speed=1.4f,travel=asset==A::Wolf?authored::wolf::TravelAtSpeed(speed):speed/3.2f;
     const float stride=asset==A::Wolf?authored::wolf::Blend(travel).stride:1.15f;
     for(int n=0;n<hz*3;++n) {
       const float t=float(n)/hz,phase=t*speed/stride*motion::tau;
       gesture.paws={};const auto base=authored::Pose(asset,t,phase,0,travel,gesture);
       feet.Update(1.f/hz,asset,400,{0,t*speed},0,phase,travel,speed,base,gesture,flat);
       events+=feet.count;
       for(unsigned i=0;i<feet.count;++i)Check(std::abs(feet.events[i].position.y)<1e-6f && feet.events[i].strength>0,"Foot event not on actual terrain");
       const auto pose=authored::Pose(asset,t,phase,0,travel,gesture);Finite(pose);
     }
     if(reference>=0)Check(std::abs(reference-int(events))<=2,"Touchdown count varies with refresh rate");
     Check(events>8 && events<30,"Missing or excessive touchdown events");reference=int(events);
   }
   rpg::Enemy enemy;enemy.id=400;enemy.kind=asset==A::Wolf?rpg::EnemyKind::Wolf:rpg::EnemyKind::Boar;
   enemy.health=enemy.MaxHealth();enemy.position={0,-10};enemy.state=rpg::EnemyState::Idle;
   presentation::BeastLocomotion beast;beast.Reset(enemy.id);motion::MovementSample movement;
   for(int n=0;n<240;++n)beast.Update(1.f/120,enemy,movement,{},1,false);
   Check(std::abs(beast.gesture.headYaw)>.01f && beast.feet.count==0,"Idle animal lacks quiet motion or emits steps");
   const float idle=beast.idleBlend;enemy.windup=.2f;enemy.state=rpg::EnemyState::Pursuing;
   for(int n=0;n<60;++n)beast.Update(1.f/120,enemy,movement,{},1,true);
   Check(beast.idleBlend<idle*.05f,"Quiet gestures did not yield to combat");
   beast.feet.count=2;enemy.health=0;beast.Update(.02f,enemy,movement,{},1,true);
   Check(beast.feet.count==0,"Dead creature replayed last footsteps");
   beast.Reset();Check(beast.gesture.paws[0].weight==0 && beast.feet.count==0,"Respawn retained paw targets");
   enemy.health=enemy.MaxHealth();beast.Update(.02f,enemy,movement,{},1,false,false);
   Check(beast.gesture.paws[0].weight==0,"Distant animal retained detailed IK work");
   beast.Update(.02f,enemy,movement,{},1,false,true);
   Check(beast.feet.count==0,"Entering detailed range emitted stale contact sounds");
   enemy.windup=0;enemy.state=rpg::EnemyState::Idle;beast.idleBlend=1;beast.idleVelocity=0;
   beast.clock=double(motion::tau)*20-.001;beast.Update(.0005f,enemy,movement,{},1,false);
   const auto old=beast.gesture;beast.Update(.001f,enemy,movement,{},1,false);
   Check(std::abs(old.headYaw-beast.gesture.headYaw)<.001f && std::abs(old.tail-beast.gesture.tail)<.001f,"Idle clock wrap snapped a gesture");
 }
}
void EventsAndEffects() {
 presentation::CreatureContacts contacts;
 for(unsigned i=0;i<30;++i)contacts.Add({400+i,{17-float(i)*.5f,0,0},0,1,i%2==0},{0,0});
 Check(contacts.count==16,"Contact pool overflow");
 for(unsigned i=0;i<contacts.count;++i)Check(contacts.events[i].position.x<=10,"Nearby contacts displaced by distant ones");
 contacts.Add({999,{100,0,0},0,1,false},{0,0});Check(contacts.count==16,"Out-of-range contact accepted");
 contacts.Reset();rpg::Simulation state;sound::Events audio;const auto player=state.Player();
 for(unsigned i=0;i<8;++i)contacts.Add({400+i,{player.x+8-i,0,player.z},0,1,i%2==0},player);
 const auto save=state.Save();audio.Update(state,&contacts);Check(audio.count==3,"Ground sound burst must select at most three nearest contacts");
 for(unsigned i=0;i<audio.count;++i)Check(audio.spatial[i] && audio.origins[i].x-player.x<=3.01f,"Ground sound lost spatial position/nearest priority");
 Check(save==state.Save(),"Presentation changed durable gameplay state");
 contacts.Reset();audio.Update(state,&contacts);Check(audio.count==0,"Empty contact frame replayed a sound");
 contacts.Add({999,{player.x,0,player.z},std::numeric_limits<float>::quiet_NaN(),1,false},player);
 Check(contacts.count==0,"Invalid heading entered contact effects");
 contacts.Add({400,{player.x+1,0,player.z},0,.65f/4,true},player);audio.Update(state,&contacts);
 Check(audio.count==1 && audio.cues[0]>=sound::Cue::HoofGrass && audio.cues[0]<=sound::Cue::HoofWood,"Slow roaming boar steps became silent");
 for(bool hoof:{false,true})for(int z=-100;z<100;z+=5)for(int x=-100;x<100;x+=5) {
   const auto expected=sound::GroundCue(state.Map(),{float(x),float(z)});
   const auto cue=sound::CreatureGroundCue(state.Map(),{float(x),float(z)},hoof);
   Check(sound::IsStep(cue) && cue!=expected,"Species terrain selection fell back to player steps");
 }
 CombatEffects effects;rpg::Vec2 soil{};bool found=false;
 for(int x=-10;x<=10&&!found;++x)for(int z=-20;z<=0&&!found;++z) {
   rpg::Vec2 p{float(x),float(z)};if(state.Map().SurfaceAt(p).paving<.1f){soil=p;found=true;}
 }
 Check(found,"Soil fixture absent");
 for(unsigned n=0;n<100;++n)effects.EmitFootstep({400,{soil.x,0,soil.z},0,1,false},state.Map());
 Check(effects.FootprintCount()==64,"Footprint pool exceeded capacity");
 for(int n=0;n<60;++n)effects.Advance(.05f);
 Check(effects.FootprintCount()==0,"Footprints failed to expire");
 effects.EmitFootstep({400,{soil.x,0,soil.z},0,1,true},state.Map());effects.Reset();
 Check(effects.FootprintCount()==0,"Reset retained footprints");
 bool stone=false;
 for(int x=-10;x<=10&&!stone;++x)for(int z=-10;z<=10&&!stone;++z) {
   const rpg::Vec2 p{float(x),float(z)};
   if(state.Map().SurfaceAt(p).paving>.5f) {
     effects.EmitFootstep({400,{p.x,0,p.z},0,1,true},state.Map());stone=true;
   }
 }
 Check(stone&&effects.FootprintCount()==0,"Stone should sound but not retain soil prints");
}
void Workload() {
 // Measure only presentation math after resource warmup, not an asserted FPS.
 std::array<presentation::BeastLocomotion,36> controllers;
 std::array<rpg::Enemy,36> enemies;
 for(unsigned i=0;i<enemies.size();++i) {
   auto& e=enemies[i];e.id=400+i;e.kind=i%2?rpg::EnemyKind::Wolf:rpg::EnemyKind::Boar;
   e.health=e.MaxHealth();e.position={float(i%6)*2,-10.f-float(i/6)*2};controllers[i].Reset(e.id);
 }
 motion::MovementSample move;move.forward=1;
 const auto start=std::chrono::steady_clock::now();
 for(unsigned frame=0;frame<120;++frame)for(unsigned i=0;i<enemies.size();++i) {
   auto& e=enemies[i];e.movementDistance=.015f;e.position.z+=e.movementDistance;
   controllers[i].Update(1.f/60,e,move,{},1,false);
 }
 const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/120;
 std::cout<<"36 near creatures, presentation CPU average: "<<ms<<" ms/update (not whole-game FPS)\n";
}
void GameTransitions() {
 rpg::Simulation reference;const auto home=reference.Beasts()[0].home;rpg::Vec2 p{};bool found=false;
 for(unsigned i=0;i<16&&!found;++i) {
   const float angle=i*motion::tau/16;p={home.x+std::sin(angle)*5,home.z+std::cos(angle)*5};
   found=reference.CanStand(p)&&reference.Map().CanTravel(p,home,.45f);
 }
 Check(found,"No contact integration approach");
 auto snapshot=reference.Save();const auto at=snapshot.find('\n',snapshot.find("PROFILE "))+1;
 std::ostringstream row;row<<p.x<<' '<<p.z<<" 100 0 0 0";
 snapshot.replace(at,snapshot.find('\n',at)-at,row.str());
 Game game;game.Initialize();Check(reference.Load(snapshot)&&game.RestoreSnapshot(snapshot),"Creature integration fixture");
 bool contacted=false;
 for(unsigned n=0;n<240&&!contacted;++n) {
   reference.Update(1.f/60,{});game.ExecuteCommand({},1.f/60);
   Check(reference.Save()==game.State().Save(),"Contacts changed real navigation/combat/save state");
   contacted=game.CreatureSteps().count>0;
 }
 Check(contacted,"Live creatures did not feed shared foot contacts");
 game.SuspendInput();Check(game.CreatureSteps().count==0,"Focus suspension retained footsteps");
 game.ExecuteCommand({},0);Check(game.CreatureSteps().count==0,"Invalid game update retained footsteps");
 Check(game.RestoreSnapshot(game.State().Save())&&game.CreatureSteps().count==0&&game.Effects().FootprintCount()==0,"Reload retained contact history");
}
std::vector<unsigned char> Capture(Renderer& renderer,const std::filesystem::path& path) {
 Check(renderer.Capture(path.wstring())&&renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Native creature/ground pass capture");
 using Microsoft::WRL::ComPtr;ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;
 ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
 Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))),"WIC factory");
 Check(SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
   SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
   SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Capture decoding");
 UINT w=0,h=0;converter->GetSize(&w,&h);std::vector<unsigned char> pixels(size_t(w)*h*4);
 Check(SUCCEEDED(converter->CopyPixels(nullptr,w*4,UINT(pixels.size()),pixels.data())),"Capture pixels");return pixels;
}
void NativeCaptures() {
 Win32Window window;Renderer renderer;
 Check(window.Create(GetModuleHandleW(nullptr),960,720,L"Creature motion checks",false)&&renderer.Initialize(window.GetHandle(),960,720),"Native renderer");
 const auto dir=std::filesystem::current_path()/"creature-motion-check";std::filesystem::create_directories(dir);
 for(auto asset:{A::Wolf,A::Boar})for(int view=0;view<3;++view) {
   const float yaw=float(view)*1.7f;
   const auto height=[](rpg::Vec2 p){return p.z*.20f+p.x*.10f;};
   presentation::QuadrupedContacts feet;authored::Gesture gesture;
   for(int n=0;n<120;++n) {
     gesture=presentation::BeastLocomotion::Support({0,0},yaw,asset==A::Wolf,height);
     const auto base=authored::Pose(asset,0,0,0,0,gesture);
     feet.Update(1.f/120,asset,400,{0,0},yaw,0,0,0,base,gesture,height);
   }
   renderer.BeginFrame();renderer.SetCamera({3,1.7f,3.5f},{0,.6f,0});
   const auto support=presentation::BeastLocomotion::Support({0,0},0,true,height);
   renderer.DrawMesh(visual::Mesh::Box,{0,-.08f,0},{7,.15f,7},{.33f,.32f,.27f,1},visual::Material::Stone,0,support.supportPitch,visual::ShadowMode::Cast,support.supportRoll);
   renderer.DrawAnimatedAsset(asset,{0,0,0},yaw,0,0,0,0,0,{1,1,1,1},gesture);
   Capture(renderer,dir/(std::to_string(unsigned(asset))+"-slope-"+std::to_string(view)+".png"));
 }
 std::vector<unsigned char> plain,marked;
 for(bool mark:{false,true}) {
   renderer.BeginFrame();renderer.SetCamera({0,3,3},{0,0,0});
   renderer.DrawMesh(visual::Mesh::Box,{0,-.10f,0},{6,.20f,6},{.6f,.55f,.45f,1},visual::Material::Stone);
   if(mark)renderer.DrawGroundMark({-.5f,.015f,-.5f},{-.5f,.015f,.5f},{.5f,.015f,.5f},{.5f,.015f,-.5f},{.08f,.06f,.04f,.65f});
   Check(renderer.EffectVertices()==(mark?6u:0u),"Ground vertices missing from shared budget");
   auto pixels=Capture(renderer,dir/(mark?"ground-mark.png":"ground-plain.png"));
   (mark?marked:plain)=std::move(pixels);
 }
 unsigned darker=0,brighter=0;
 for(size_t i=0;i<plain.size();i+=4) {
   const int before=int(plain[i])+plain[i+1]+plain[i+2],after=int(marked[i])+marked[i+1]+marked[i+2];
   darker+=after<before-6;brighter+=after>before+6;
 }
 Check(darker>300 && brighter<10,"Ground marks glow or do not draw with normal alpha");
 renderer.BeginFrame();for(unsigned i=0;i<2000;++i)renderer.DrawGroundMark({0,0,0},{0,0,1},{1,0,1},{1,0,0},{0,0,0,1});
 for(unsigned i=0;i<2000;++i)renderer.DrawEffectGlow({0,1,0},1,{1,1,1,1});
 Check(renderer.EffectVertices()<=Renderer::maxEffectVertices,"Ground plus spell effects exceeded shared vertex budget");
 Check(renderer.EndFrame(false)&&renderer.ValidateDebugMessages(),"Bounded ground/effect flush");
}
}
int main() {try {
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Solvers();Terrain();RatesAndLifecycle();EventsAndEffects();Workload();GameTransitions();NativeCaptures();
 std::cout<<"PASS: creature joints, terrain, refresh rates, lifecycle, sound/footprint bounds and native color/shadow passes\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
