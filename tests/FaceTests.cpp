#include "game/Game.h"
#include "game/CharacterPose.h"
#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <wincodec.h>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
std::vector<unsigned char> Pixels(const wchar_t* path) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IWICImagingFactory> factory; ComPtr<IWICBitmapDecoder> decoder;
  ComPtr<IWICBitmapFrameDecode> frame; ComPtr<IWICFormatConverter> converter;
  Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
    SUCCEEDED(factory->CreateDecoderFromFilename(path,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)) &&
    SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
    SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)),"Face diagnostic decode");
  UINT width=0,height=0;
  Check(SUCCEEDED(converter->GetSize(&width,&height)) && width==960 && height==720,"Face diagnostic size");
  std::vector<unsigned char> pixels(width*height*4);
  Check(SUCCEEDED(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())),"Face diagnostic pixels"); return pixels;
}
void Controller() {
  const auto right=facial::LookAt(3,1,3,0),left=facial::LookAt(-3,1,3,0);
  Check(right.yaw>0 && left.yaw<0 && right.pitch<0 && right.eyeYaw>0,"Attention must face the target and share gaze with the eyes");
  Check(facial::LookAt(.0001f,0,-3,0).yaw==0 && facial::LookAt(-.0001f,0,-3,0).yaw==0,"Rear seam must release attention instead of flipping");
  Check(facial::LookAt(30,0,30,0).yaw==0 && facial::LookAt(.01f,0,.01f,0).yaw==0,"Unavailable and overlapping targets must release attention");
  facial::Pose reference;
  for(int hz : {22,30,60,144,1000}) {
    facial::State state; state.Reset(17);
    for(double t=0;t<1.7-1e-8;) { const float dt=float(std::min(1.0/hz,1.7-t)); state.Update(dt,right,.8f,.6f,.3f); t+=dt; }
    if(hz!=22) Check(std::abs(state.pose.yaw-reference.yaw)<.00001f && std::abs(state.pose.blink-reference.blink)<.00001f &&
      std::abs(state.pose.focus-reference.focus)<.00001f,"Equal-time constant attention must agree at different update rates");
    reference=state.pose;
    for(int i=0;i<hz*2;++i) state.Update(1.f/hz,{},0,0,0);
    Check(std::abs(state.pose.yaw)<.00001f && state.pose.focus<.00001f,"Lost attention must settle to neutral");
    state.Update(.01f,right,1,1,1,false);
    Check(state.pose.yaw==0 && state.pose.focus==0 && state.pose.pain==0,"Defeat must clear facial reactions");
  }
  float maximumBlinkStep=0;
  for(unsigned id=0;id<10;++id) {
    unsigned closed=0; const double period=4.1+.13*(id%7);
    for(int i=0;i<10000;++i) {
      const double t=i*.001; const float blink=facial::Blink(t,id);
      closed+=blink>.95f;
      maximumBlinkStep=std::max(maximumBlinkStep,std::abs(blink-facial::Blink(t-.001,id)));
      Check(blink>=0 && blink<=1 && std::abs(blink-facial::Blink(t+period,id))<.00001f,"Blink must stay bounded and continuous at clock wrap");
    }
    Check(closed>10 && closed<200,"Each actor must blink briefly, not keep its eyes shut");
  }
  Check(maximumBlinkStep<.024f && facial::Blink(1.365,0)>.999f,"Blink closure must reach full closure smoothly");
  Check(facial::Blink(1.365,0)!=facial::Blink(1.365,3),"Actors must not all blink in synchrony");
  const float nan=std::numeric_limits<float>::quiet_NaN();
  facial::State invalid; invalid.Update(nan,right,1,1,1); invalid.Update(.01f,{nan,nan,nan,nan},nan,nan,nan);
  Check(invalid.pose.yaw==0 && invalid.pose.focus==0 && facial::Blink(nan)==0,"Invalid diagnostics must not poison facial state");
  std::cout<<"Maximum 1 ms blink change: "<<maximumBlinkStep<<'\n';
}
void Attachments() {
  animation::Sample sample; sample.bank=.6f; sample.hit=.3f; sample.attackSeconds=.25f; sample.heavyAttack=true;
  const auto neutral=BuildCharacterPose(0,.7f,true,sample);
  for(float yaw : {-.62f,0.f,.62f}) for(float pitch : {-.245f,0.f,.245f}) {
    sample.face.yaw=yaw; sample.face.pitch=pitch;
    const auto pose=BuildCharacterPose(0,.7f,true,sample);
    Check(Distance(pose.HeadPoint({0,1.61f,0}),pose.UpperPoint({0,1.61f,0}))<.000001f,"Head rotation must pin the neck attachment");
    Check(std::abs(Distance(pose.HeadPoint({0,1.76f,.012f}),pose.HeadPoint({0,1.86f,.012f}))-.1f)<.000001f,
      "Head/helmet attachments must retain their relative dimensions");
    Check(Distance(pose.WeaponPoint(.96f),neutral.WeaponPoint(.96f))<.000001f &&
      Distance(pose.arms[0].end,neutral.arms[0].end)<.000001f,"Looking at a target must never move the weapon or solved hands");
    if(yaw==0 && pitch==0) Check(Distance(pose.HeadPoint({.1f,1.9f,.05f}),pose.UpperPoint({.1f,1.9f,.05f}))<.000001f,"Neutral head must match the established upper-body transform");
  }
}
void Integration() {
  std::ifstream file(std::string(RPG_TEST_FIXTURES)+"/combat-preview.save");
  std::string snapshot((std::istreambuf_iterator<char>(file)),{});
  Game game; game.Initialize(); Check(game.RestoreSnapshot(snapshot),"Combat attention fixture");
  rpg::Command select; select.nextTarget=true; select.steerFacing=true; select.facingYaw=-.6f; game.ExecuteCommand(select,.01f);
  for(int i=0;i<30;++i) game.ExecuteCommand({},.01f);
  Check(game.PlayerAnimation().face.focus>.2f && game.PlayerAnimation().face.yaw>.3f,
    "Selected side opponent must drive player attention and focus expression");
  Check(game.RestoreSnapshot(snapshot) && game.PlayerAnimation().face.yaw==0 && game.PlayerAnimation().face.focus==0,
    "Load must discard old attention and transient expressions");
  game.Initialize(); Check(game.PlayerAnimation().face.focus==0,"Fresh expedition clears facial state");
}
}
int main() {
  Controller(); Attachments(); Integration();
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Face COM initialization");
  Win32Window window; Check(window.Create(GetModuleHandle(nullptr),960,720,L"Facial attention checks",false),"Hidden face window");
  Renderer renderer; Check(renderer.Initialize(window.GetHandle(),960,720),"Face renderer");
  const auto head=visual::BuildMesh(visual::Mesh::Head),feature=visual::BuildMesh(visual::Mesh::Feature);
  std::cout<<"Head vertices/triangles: "<<head.vertices.size()<<" / "<<head.indices.size()/3
    <<"; feature: "<<feature.vertices.size()<<" / "<<feature.indices.size()/3<<"; total mesh bytes: "<<renderer.MeshBytes()<<'\n';
  const wchar_t* names[]={L"neutral",L"blink",L"focus",L"hurt",L"welcome",L"look-left",L"look-up",L"helmet",L"profile"};
  for(int i=0;i<9;++i) {
    animation::Sample sample;
    sample.face.blink=i==1?1.f:0; sample.face.focus=i==2?1.f:0; sample.face.pain=i==3?1.f:0; sample.face.smile=i==4?1.f:0;
    sample.face.yaw=i==5?-.55f:i==7?.45f:0; sample.face.eyeYaw=i==5?-.18f:0;
    sample.face.pitch=i==6?-.20f:0; sample.face.eyePitch=i==6?-.08f:0;
    CharacterOutfit outfit; outfit.helm=i==7;
    renderer.BeginFrame(); renderer.SetCamera(i==8?XMFLOAT3{.85f,1.79f,.16f}:XMFLOAT3{0,1.79f,.85f},{0,1.76f,0});
    DrawCharacter(renderer,{0,0},0,0,{.22f,.37f,.54f,1},0,0,outfit,sample);
    Check(renderer.Capture(L"face-"+std::wstring(names[i])+L".png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Face color/shadow capture");
  }
  const auto opened=Pixels(L"face-neutral.png"),closed=Pixels(L"face-blink.png");
  unsigned changed=0,outside=0;
  for(int y=0;y<720;++y) for(int x=0;x<960;++x) {
    const auto at=(y*960+x)*4;
    if(opened[at]!=closed[at] || opened[at+1]!=closed[at+1] || opened[at+2]!=closed[at+2]) {
      ++changed; outside+=x<380 || x>580 || y<320 || y>355;
    }
  }
  Check(changed>500 && changed<2500 && outside==0,"Blink must visibly close both eyes without changing the rest of the character or its shadows");
  float cheek[2]{};
  for(int occluded=0;occluded<2;++occluded) {
    renderer.BeginFrame(); renderer.SetCamera({0,1.79f,-.85f},{0,1.76f,0});
    DrawCharacter(renderer,{0,0},0,motion::tau*.5f,{.22f,.37f,.54f,1},0,0);
    if(occluded) renderer.DrawMesh(visual::Mesh::Box,{-.5f,2.61f,-.4f},{1,.08f,1},{.4f,.4f,.4f,1},visual::Material::Stone);
    XMFLOAT2 probe; Check(renderer.Project({-.065f,1.74f,-.105f},probe),"Lit cheek projection");
    const auto path=occluded?L"face-shadowed.png":L"face-sunlit.png";
    Check(renderer.Capture(path) && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Facial shadow receiver capture");
    const auto pixels=Pixels(path); const int cx=int(probe.x*960),cy=int(probe.y*720);
    for(int y=cy-4;y<=cy+4;++y) for(int x=cx-4;x<=cx+4;++x) {
      const int at=(y*960+x)*4; cheek[occluded]+=(pixels[at]+pixels[at+1]+pixels[at+2])/(81.f*3);
    }
  }
  std::cout<<"Blink changed pixels: "<<changed<<"; cheek light / occlusion: "<<cheek[0]<<" / "<<cheek[1]<<'\n';
  Check(cheek[0]-cheek[1]>10,"Curved-face filtering must retain shadows from external occluders");
  renderer.BeginFrame(); renderer.SetCamera({0,2,6},{0,1,0});
  animation::Sample gaze; gaze.face.yaw=.55f; gaze.face.focus=1;
  DrawCharacter(renderer,{-1,0},0,0,{.2f,.4f,.6f,1},0,0,{},gaze,1);
  Check(renderer.BeginPortrait(.7f,.05f,.28f,.8f),"Face portrait after world");
  DrawCharacter(renderer,{0,0},0,0,{.2f,.4f,.6f,1},0,0,{},animation::Idle(1.365f),1);
  Check(renderer.EndPortrait() && renderer.Capture(L"face-world-portrait.png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Face portrait isolation");
  std::cout<<"PASS: facial timing, targeting, attachment invariants, lifecycle and GPU views\n";
  CoUninitialize();
}
