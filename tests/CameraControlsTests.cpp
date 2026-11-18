#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <limits>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Driver {
 std::unique_ptr<Game> game=std::make_unique<Game>();Input input;Input::Frame frame;
 Driver(){game->Initialize();frame.focused=true;frame.client={630,300};frame.screen=frame.client;frame.relativeMouse=true;Step();}
 void Step(float dt=1.f/60,float wheel=0){input.Update(frame);game->Update(dt,input,wheel);}
 void Settle(){frame.mouseX=frame.mouseY=0;for(int n=0;n<60;++n)Step();}
 void Button(int key,bool held){frame.down[key]=held;Step();}
 void Point(LONG x,LONG y){frame.client={x,y};frame.screen=frame.client;}
};
void FreeLook(){
 for(float dt:{1.f/30,1.f/60,1.f/144}){
  Driver d;const float facing=d.game->State().Facing();const auto standing=d.game->State().Player();
  d.Button(VK_LBUTTON,true);Check(d.game->MouseLookButtons()==MK_LBUTTON,"LMB world press did not own camera capture");
  d.frame.mouseX=100;d.frame.mouseY=20;d.Step(dt);d.Settle();
  Check(d.game->CameraYaw()>.59f && d.game->CameraPitch()>.52f,"LMB did not rotate both view axes");
  Check(d.game->State().Facing()==facing && d.game->State().Player().x==standing.x && d.game->State().Player().z==standing.z,
    "Free look turned or moved the planted champion");
  d.frame.down['W']=true;for(int n=0;n<12;++n)d.Step(dt);
  Check(std::abs(d.game->State().Player().x-standing.x)<.0001f,"Free look redirected forward travel");
  d.Button(VK_LBUTTON,false);for(int n=0;n<6;++n)d.Step(dt);
  Check(!d.game->WantsMouseLook() && std::abs(d.game->State().Player().x-standing.x)<.0001f,
    "Releasing LMB snapped travel toward the free camera heading");
  d.frame.down['W']=false;d.Button(VK_RBUTTON,true);d.Settle();
  Check(std::abs(motion::AngleDelta(d.game->State().Facing(),d.game->CameraYaw()))<.001f,"RMB did not restore body steering");
 }
}
void ButtonHandoffs(){
 for(int first:{VK_LBUTTON,VK_RBUTTON}){
  Driver d;const int second=first==VK_LBUTTON?VK_RBUTTON:VK_LBUTTON;
  d.Button(first,true);d.Button(second,true);
  Check(d.game->MouseLookButtons()==(MK_LBUTTON|MK_RBUTTON),"Two world buttons must share one capture");
  d.Button(first,false);Check(d.game->MouseLookButtons()==unsigned(second==VK_LBUTTON?MK_LBUTTON:MK_RBUTTON),"First release cancelled the still-held button");
  const float facing=d.game->State().Facing();d.frame.mouseX=80;d.Step();d.Settle();
  if(second==VK_LBUTTON)Check(d.game->State().Facing()==facing,"RMB-to-LMB handoff kept steering the body");
  else Check(d.game->State().Facing()>.4f,"LMB-to-RMB handoff lost steering");
  d.Button(second,false);Check(!d.game->WantsMouseLook(),"Last release kept capture alive");
 }
 // A button held before the fresh gesture cannot silently become its owner.
 for(int blocked:{VK_LBUTTON,VK_RBUTTON}){
  Driver d;d.game->OpenPanel(ui::Panel::Menu);d.Button(blocked,true);
  d.game->OpenPanel(ui::Panel::None);d.Step();const float yaw=d.game->CameraYaw();d.frame.mouseX=200;
  const int fresh=blocked==VK_LBUTTON?VK_RBUTTON:VK_LBUTTON;d.Button(fresh,true);
  Check(d.game->CameraYaw()==yaw,"Fresh world gesture inherited movement from a held UI button");
  d.frame.mouseX=0;d.Button(fresh,false);
  Check(!d.game->WantsMouseLook(),"An old blocked press inherited camera capture");
 }
}
void Cancellations(){
 for(int key:{VK_LBUTTON,VK_RBUTTON})for(int cancel=0;cancel<5;++cancel){
  Driver d;d.Button(key,true);d.frame.mouseX=30;d.Step();d.frame.mouseX=0;
  if(cancel==0)d.frame.focused=false;
  if(cancel==1)d.frame.down[VK_ESCAPE]=true;
  if(cancel==2)d.frame.down[VK_F11]=true;
  if(cancel==3){d.game->CancelUiGesture();d.input.CancelGestures();}
  if(cancel==4)Check(d.game->RestoreSnapshot(d.game->State().Save()),"Camera load fixture");
  d.Step();Check(!d.game->WantsMouseLook(),"Interruption retained camera ownership");
  const float yaw=d.game->CameraYaw();d.frame.focused=true;d.frame.down[VK_ESCAPE]=d.frame.down[VK_F11]=false;
  if(cancel==1)d.game->OpenPanel(ui::Panel::None);
  d.frame.mouseX=200;d.Step();d.Step();
  Check(!d.game->WantsMouseLook() && d.game->CameraYaw()==yaw,"Held input resumed a cancelled camera gesture");
  d.frame.mouseX=0;d.Button(key,false);d.Button(key,true);Check(d.game->WantsMouseLook(),"Fresh gesture failed after cancellation");
 }
 Driver d;d.Button(VK_LBUTTON,true);d.frame.mouseX=std::numeric_limits<float>::quiet_NaN();d.Step();
 Check(std::isfinite(d.game->CameraYaw()),"Invalid mouse motion poisoned the camera");
}
void ObstructedTravel(){
 Driver d;
 const std::string fixture="MEDITERRANEAN 3\n-10 -14 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 0 0 0 0 0\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
 Check(d.game->RestoreSnapshot(fixture),"Tree route fixture");
 d.game->SetCameraOrbit(1.5707963f,.1f,6);
 Check(d.game->CameraRange()==6,"Tree occlusion changed explicit camera distance");
 d.frame.down['D']=true;
 for(int n=0;n<35;++n){d.Step();Check(std::abs(d.game->CameraRange()-6)<.001f,"Passing a tree changed camera distance");}
 d.frame.down['D']=false;d.Step(1.f/60,1);d.Settle();
 Check(std::abs(d.game->CameraRange()-5)<.001f,"Manual wheel zoom stopped working beside an obstacle");
}
void NativePicking(Renderer& renderer,const std::filesystem::path& output){
 Driver d;
 const std::string fixture="MEDITERRANEAN 3\n12 -6 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 10 -2 80 0 0 0 0 0\n101 18 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
 Check(d.game->RestoreSnapshot(fixture),"Native target fixture");
 const auto render=[&]{renderer.BeginFrame();d.game->Render(renderer);Check(renderer.EndFrame(false),"Camera input native render");};
 render();
 // Existing dummy, projected through the same native camera used by HUD picking.
 const auto& dummy=d.game->State().Dummies()[0];
 DirectX::XMFLOAT2 point;Check(renderer.Project({dummy.position.x,rpg::WorldMap::Height(dummy.position)+1.1f,dummy.position.z},point),"Dummy projection");
 d.Point(LONG(point.x*1280),LONG(point.y*720));d.Button(VK_LBUTTON,true);
 Check(!d.game->State().TargetActor(),"Target press committed before click/drag disambiguation");
 d.Button(VK_LBUTTON,false);
 Check(d.game->State().TargetActor() && d.game->State().TargetActor()->id==dummy.id,"Stationary world click lost target selection");
 rpg::Command target;target.selectActor=100;d.game->ExecuteCommand(target,.001f);render();
 Check(d.game->State().TargetActor() && d.game->State().TargetActor()->id==100,"Alternate target fixture");
 d.Button(VK_LBUTTON,true);d.frame.mouseX=60;d.Step();d.frame.mouseX=0;d.Button(VK_LBUTTON,false);
 Check(d.game->State().TargetActor() && d.game->State().TargetActor()->id==100,"Orbiting from a body changed the combat target");
 // UI-origin gestures never turn into free look or native cursor capture.
 d.game->OpenPanel(ui::Panel::Inventory);render();d.Point(1030,275);d.Button(VK_LBUTTON,true);
 Check(!d.game->WantsMouseLook(),"Inventory click acquired world capture");
 d.Point(630,300);d.Step();Check(!d.game->WantsMouseLook(),"UI drag became a camera gesture after leaving its panel");
 d.Button(VK_LBUTTON,false);d.game->OpenPanel(ui::Panel::None);render();
 d.Point(630,300);d.Button(VK_LBUTTON,true);d.frame.mouseX=60;d.Step();d.frame.mouseX=0;
 renderer.BeginFrame();d.game->Render(renderer);Check(renderer.Capture((output/"left-orbit.png").wstring()) && renderer.EndFrame(false),"LMB camera capture");
 Check(renderer.ValidateDebugMessages(),"Native camera validation warnings");
}
}
int main(int argc,char** argv){try{
 FreeLook();ButtonHandoffs();Cancellations();ObstructedTravel();
 const auto output=std::filesystem::absolute(argc>1?argv[1]:"camera-controls-review");std::filesystem::create_directories(output);
 Win32Window window;Renderer renderer;
 Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Camera controls review",false) && renderer.Initialize(window.GetHandle(),1280,720),"Native camera test startup");
 NativePicking(renderer,output);
 std::cout<<"PASS: fixed prop distance, free look, independent movement, button handoffs, cancellation and UI/target ownership\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
