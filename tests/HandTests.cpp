#include "game/HandPose.h"
#include "game/CharacterView.h"
#include "game/WeaponGrip.h"
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
void FittedSwordGrip() {
  animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;
  sample.authoredEquipment=playerclips::Equipment::Sword;
  const auto pose=BuildCharacterPose(0,0,false,sample,rpg::WeaponForm::Blade);
  Check(pose.fittedBladeGrip,"Authored sword did not select its fitted palm socket");
  const auto hand=BuildHandPose(pose,1,true,false,0);
  const auto inverse=XMMatrixInverse(nullptr,WeaponFrame(pose));
  const auto gap=[](float x,float z) {
    float closest=1;
    for(int k=0;k<720;++k) {
      const float angle=k*DirectX::XM_2PI/720;
      closest=std::min(closest,std::hypot(x-weapongrip::swordAcross*std::cos(angle),
        z-weapongrip::swordDepth*std::sin(angle)));
    }
    return (x*x/(weapongrip::swordAcross*weapongrip::swordAcross)+
      z*z/(weapongrip::swordDepth*weapongrip::swordDepth)<1?-1:1)*closest;
  };
  for(unsigned f=0;f<4;++f) {
    const auto tip=Transform(hand.Point(hand.fingers[f].joints[3]),inverse);
    const float pad=(f==3?.0085f:.0097f)*.8f;
    Check(std::abs(gap(tip.x,tip.z)-pad)<.003f,"Fingertip pad floats away from the actual oval sword handle");
    Check(tip.y>-.055f&&tip.y<.101f,"Fingers close outside the usable hilt");
  }
  Check(Distance(hand.Point({0,-.055f,-.043f}),pose.arms[1].end)<1e-5f,"Fitted palm detached from arm IK");
  const auto palette=BuildHandRig(hand);const auto& mesh=visual::CharacterHand(0,hand.mirrored);
  float penetration=0;
  for(std::size_t i=0;i<mesh.mesh.vertices.size();++i) {
    const auto vertex=visual::SkinHandVertex(mesh.mesh.vertices[i],mesh.influences[i],palette);
    const auto p=Transform(hand.Point(vertex.position),inverse);
    if(p.y>-.052f&&p.y<.097f)penetration=std::max(penetration,-gap(p.x,p.z));
  }
  std::cout<<"Fitted sword maximum skin/handle penetration: "<<penetration<<'\n';
  Check(penetration<.0025f,"Skinned palm or finger intersects the fitted sword handle");
}
void FittedRangedGrips() {
  for(bool bow:{false,true})for(unsigned side=0;side<2;++side) {
    if(bow&&!side)continue;
    animation::Sample sample;sample.authoredGait=true;sample.authoredBlend=1;sample.authoredReady=1;
    sample.authoredEquipment=bow?playerclips::Equipment::Bow:playerclips::Equipment::Staff;
    const auto pose=BuildCharacterPose(0,0,false,sample,bow?rpg::WeaponForm::Handbow:rpg::WeaponForm::AstralStaff);
    const auto hand=BuildHandPose(pose,side,true,false,0);const auto inverse=XMMatrixInverse(nullptr,WeaponFrame(pose));
    Check(hand.mirrored==(side==1),"Ranged grip changed anatomical handedness");
    Check(Distance(hand.Point(weapongrip::rangedSocket),pose.WeaponPoint(side?0:.26f))<.0001f,"Ranged palm detached from its real handle");
    Check(Distance(hand.Point({0,-.055f,-.043f}),pose.arms[side].end)<.0001f,"Ranged wrist detached from forearm");
    const auto gap=[bow](float x,float z) {
      const float a=.023f,b=bow?.017f:.023f;float d=1;
      for(int k=0;k<180;++k){const float angle=k*XM_2PI/180;d=std::min(d,std::hypot(x-a*std::cos(angle),z-b*std::sin(angle)));}
      return (x*x/(a*a)+z*z/(b*b)<1?-1:1)*d;
    };
    float padError=0,penetration=0;
    for(unsigned f=0;f<4;++f){const auto tip=Transform(hand.Point(hand.fingers[f].joints[3]),inverse);
      padError=std::max(padError,std::abs(gap(tip.x,tip.z)-(f==3?.0085f:.0097f)*.8f));}
    const auto palette=BuildHandRig(hand);const auto& mesh=visual::CharacterHand(0,hand.mirrored);
    for(std::size_t i=0;i<mesh.mesh.vertices.size();++i){
      const auto v=visual::SkinHandVertex(mesh.mesh.vertices[i],mesh.influences[i],palette);
      const auto p=Transform(hand.Point(v.position),inverse);
      if(std::abs(p.y-(side?0:.26f))<.065f)penetration=std::max(penetration,-gap(p.x,p.z));
    }
    std::cout<<(bow?"Bow":"Staff")<<" hand "<<side<<" pad error "<<padError<<" skin penetration "<<penetration<<'\n';
    Check(padError<.003f,"Ranged fingertips float away from the authored handle");
    Check(penetration<.0025f,"Ranged skinned fingers penetrate the handle");
  }
  CharacterPose pose;pose.fittedBowGrip=true;pose.hasGripSocket=true;pose.bowDraw=pose.bowStringGrip=1;
  pose.weaponDirection={0,1,0};pose.weaponRight={1,0,0};
  const auto hand=BuildHandPose(pose,0,true,false,0);
  for(unsigned f=0;f<3;++f){const auto p=hand.fingers[f].joints[3];
    Check(std::abs(p.y-.053f)<.00001f&&std::abs(p.z+.008f)<.00001f,"Archery fingers do not hook one string line");
    Check(f==0?p.x>weapongrip::bowStringContact.x:p.x<weapongrip::bowStringContact.x,"Archery nock is not between index and middle fingers");}
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
  FittedSwordGrip();
  FittedRangedGrips();
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
