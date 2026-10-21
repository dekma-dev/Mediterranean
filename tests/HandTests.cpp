#include "game/HandPose.h"
#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using namespace DirectX;
void Check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
float Distance(XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); }
XMFLOAT3 Transform(XMFLOAT3 p,const XMMATRIX& frame) {
  XMFLOAT3 result; XMStoreFloat3(&result,XMVector3TransformCoord(XMLoadFloat3(&p),frame)); return result;
}
void Structure() {
  float largestTipStep=0,smallestLateral=1;
  for(bool heavy : {false,true}) for(bool shield : {false,true}) for(int layer=0;layer<4;++layer) {
    std::array<HandPose,2> previous{};
    for(int step=0;step<=1100;++step) {
      const float t=step*.001f;
      animation::Sample sample; sample.attackSeconds=t; sample.heavyAttack=heavy;
      sample.air=layer==1?.8f:0; sample.landing=layer==2?.8f:0; sample.dash=layer==3?.8f:0;
      sample.bank=.6f; sample.cast=.7f;
      const auto pose=BuildCharacterPose(0,std::sin(t*6),shield,sample);
      const auto weapon=WeaponFrame(pose);
      Check(std::abs(XMVectorGetX(XMMatrixDeterminant(weapon))-1)<.00001f,"Weapon frame must be orthonormal and preserve winding");
      const auto across=pose.UpperDirection({1,0,0});
      smallestLateral=std::min(smallestLateral,XMVectorGetX(XMVector3Dot(weapon.r[0],XMLoadFloat3(&across))));
      Check(Distance(Transform({0,.96f,0},weapon),pose.WeaponPoint(.96f))<.000001f,"Blade transform must retain the established effect trail tip");
      Check(std::abs(XMVectorGetX(XMVector3Dot(weapon.r[0],weapon.r[1])))<.000001f,"Guard must stay perpendicular to the blade through attacks");
      for(unsigned side=0;side<2;++side) {
        const auto hand=BuildHandPose(pose,side,true,shield,pose.handCast);
        const auto frame=XMLoadFloat4x4(&hand.frame);
        Check(std::abs(XMVectorGetX(XMMatrixDeterminant(frame))-1)<.00001f,"Hand frame must preserve volume and winding");
        if(side || shield) Check(Distance(hand.Point({0,0,0}),pose.arms[side].end)<.000001f,"Held item lost its grip anchor");
        else Check(Distance(hand.Point({0,-.055f,-.043f}),pose.arms[side].end)<.000001f,"Free casting wrist detached from the solved arm");
        for(unsigned f=0;f<5;++f) for(unsigned bone=0;bone<3;++bone) {
          const auto a=hand.Point(hand.fingers[f].joints[bone]),b=hand.Point(hand.fingers[f].joints[bone+1]);
          const float local=Distance(hand.fingers[f].joints[bone],hand.fingers[f].joints[bone+1]);
          Check(local>.018f && local<.046f && std::abs(Distance(a,b)-local)<.000001f,"Finger phalanx stretched or became nonfinite");
        }
        if(step) for(unsigned f=0;f<5;++f) largestTipStep=std::max(largestTipStep,
          Distance(hand.Point(hand.fingers[f].joints[3]),previous[side].Point(previous[side].fingers[f].joints[3])));
        previous[side]=hand;
      }
    }
  }
  Check(smallestLateral>.8f,"Authored weapon cone approached the attachment frame pole");
  Check(largestTipStep<.012f,"Hand frame or fingers snapped between consecutive attack samples");
  const auto pose=BuildCharacterPose(0,0,false);
  const auto open=BuildHandPose(pose,0,false,false,1),relaxed=BuildHandPose(pose,0,false,false,0),grip=BuildHandPose(pose,1,true,false,1);
  Check(open.curl==0 && relaxed.curl>.2f && grip.curl==1,"Casting must open only an unoccupied hand");
  const auto rightOpen=BuildHandPose(pose,1,false,false,1);
  Check(open.fingers[4].joints[3].x>0 && rightOpen.fingers[4].joints[3].x<0,"Open hands must have anatomically opposed thumbs");
  const auto locked=BuildHandPose(pose,0,false,true,1);
  Check(locked.curl==1,"Casting must retain an equipped shield grip");
  for(unsigned f=0;f<4;++f) {
    Check(open.fingers[f].joints[3].y>relaxed.fingers[f].joints[3].y,"Open casting fingers must extend");
    for(unsigned bone=0;bone<3;++bone)
      Check(std::abs(Distance(open.fingers[f].joints[bone],open.fingers[f].joints[bone+1])-
        Distance(grip.fingers[f].joints[bone],grip.fingers[f].joints[bone+1]))<.000001f,"Grip changes must preserve finger length");
  }
  for(unsigned f=0;f<5;++f) {
    // The cylinder axis is hand X. Leave room for the phalanx radius too.
    for(unsigned bone=0;bone<3;++bone) for(int step=0;step<=10;++step) {
      const auto a=grip.fingers[f].joints[bone],b=grip.fingers[f].joints[bone+1];
      const float t=step*.1f,y=a.y+(b.y-a.y)*t,z=a.z+(b.z-a.z)*t;
      Check(std::hypot(y,z)>.026f+grip.fingers[f].width*.35f,"A gripping finger passes through the sword handle");
    }
  }
  for(float invalid : {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-100.f,100.f}) {
    const auto hand=BuildHandPose(pose,0,false,false,invalid);
    Check(std::isfinite(hand.Point(hand.fingers[4].joints[3]).x) && hand.curl>=0 && hand.curl<=1,"Invalid cast poisoned the hand pose");
  }
  std::cout<<"Maximum 1 ms fingertip step: "<<largestTipStep<<"; minimum weapon lateral alignment: "<<smallestLateral<<'\n';
}
}
int main() {
  Structure();
  Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"Hand COM");
  Win32Window window; Check(window.Create(GetModuleHandle(nullptr),960,720,L"Hand and weapon checks",false),"Hidden hand window");
  Renderer renderer; Check(renderer.Initialize(window.GetHandle(),960,720),"Hand renderer");
  const wchar_t* names[]={L"grip-front",L"grip-back",L"grip-side",L"cast",L"relaxed",L"gloves",L"shield",L"strike",L"heavy",L"jump",L"whole",L"portrait"};
  for(int i=0;i<12;++i) {
    animation::Sample sample; sample.cast=i==3?1.f:0;
    sample.attackSeconds=i==7?.18f:i==8?.36f:-1.f; sample.heavyAttack=i==8;
    sample.air=i==9?.8f:0; sample.rise=i==9?.5f:0;
    CharacterOutfit outfit; outfit.weapon=i!=4; outfit.gloves=i==5; outfit.shield=i==6;
    const auto pose=BuildCharacterPose(0,0,outfit.shield,sample);
    const auto center=pose.arms[i==3 || i==4 || i==6?0:1].end;
    renderer.BeginFrame();
    const XMFLOAT3 eye{i==2?.65f:0,.12f,i==1?-.65f:.65f};
    renderer.SetCamera({center.x+eye.x,center.y+eye.y,center.z+eye.z},center);
    if(i>=10) renderer.SetCamera({0,1.9f,4},{0,1.05f,0});
    DrawCharacter(renderer,{0,0},0,0,{.22f,.37f,.54f,1},0,0,outfit,sample,1);
    if(i==11) {
      Check(renderer.BeginPortrait(.7f,.05f,.28f,.8f),"Hand portrait");
      DrawCharacter(renderer,{0,0},0,0,{.22f,.37f,.54f,1},0,0,outfit,sample,1);
      Check(renderer.EndPortrait(),"End hand portrait");
    }
    Check(renderer.Capture(L"hands-"+std::wstring(names[i])+L".png") && renderer.EndFrame(false) && renderer.ValidateDebugMessages(),"Hand render/shadow/portrait capture");
  }
  std::cout<<"PASS: hand articulation, grip/guard continuity, fixed lengths, wrist anchors and 12 GPU views; mesh bytes: "<<renderer.MeshBytes()<<'\n';
  CoUninitialize();
}
