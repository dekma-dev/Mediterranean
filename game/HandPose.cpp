#include "game/HandPose.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;
XMFLOAT3 HandPose::Point(XMFLOAT3 local) const {
  XMFLOAT3 result;
  XMStoreFloat3(&result,XMVector3TransformCoord(XMLoadFloat3(&local),XMLoadFloat4x4(&frame)));
  return result;
}
XMMATRIX WeaponFrame(const CharacterPose& pose) {
  const auto up=XMLoadFloat3(&pose.weaponDirection);
  const auto across=pose.UpperDirection({1,0,0});
  // The authored blade cone never approaches the shoulder's lateral axis.
  // Project that axis instead of crossing a forward vector: the old frame
  // inverted when the blade passed through the forward-facing pole.
  const auto lateral=XMLoadFloat3(&across);
  const auto right=XMVector3Normalize(lateral-up*XMVector3Dot(lateral,up));
  return {right,up,XMVector3Cross(right,up),XMVectorSetW(XMLoadFloat3(&pose.arms[1].end),1)};
}
HandPose BuildHandPose(const CharacterPose& pose,unsigned side,bool weapon,bool shield,float cast) {
  side=std::min(side,1u);
  const bool gripping=side?weapon:shield;
  const auto& arm=pose.arms[side];
  const auto forearm=XMVector3Normalize(XMLoadFloat3(&arm.end)-XMLoadFloat3(&arm.joint));
  XMMATRIX frame;
  if(side && weapon) {
    const auto blade=WeaponFrame(pose);
    frame={blade.r[1],blade.r[0],-blade.r[2],blade.r[3]};
  } else if(!side && shield) {
    const auto x=pose.UpperDirection({0,1,0}),y=pose.UpperDirection({-1,0,0}),z=pose.UpperDirection({0,0,1});
    frame={XMLoadFloat3(&x),XMLoadFloat3(&y),XMLoadFloat3(&z),XMVectorSetW(XMLoadFloat3(&arm.end),1)};
  } else {
    const auto rightLocal=pose.UpperDirection({1,0,0}),upLocal=pose.UpperDirection({0,1,0}),forwardLocal=pose.UpperDirection({0,0,1});
    const auto right=XMLoadFloat3(&rightLocal),up=XMLoadFloat3(&upLocal),forward=XMLoadFloat3(&forwardLocal);
    const float reach=animation::SmoothStep(cast)*(side?.45f:1.f);
    // A hanging hand and upright casting palm can be almost 180 degrees
    // apart. Shortest-arc quaternion blending switches sides at that seam.
    // Lift through the forward hemisphere using a bounded elevation instead;
    // the elbow plane cannot introduce a second, independent wrist twist.
    const float vertical=XMVectorGetX(XMVector3Dot(forearm,up));
    const float elevation=std::acos(std::clamp(vertical,-.995f,.995f))*(1-reach);
    const float lateral=std::clamp(XMVectorGetX(XMVector3Dot(forearm,right))*(1-reach),-.98f,.98f);
    const float radial=std::sqrt(1-lateral*lateral);
    const auto fingerAxis=right*lateral+(up*std::cos(elevation)+forward*std::sin(elevation))*radial;
    const auto across=XMVector3Normalize(right-fingerAxis*XMVector3Dot(right,fingerAxis));
    frame={across,fingerAxis,XMVector3Cross(across,fingerAxis),XMVectorSet(0,0,0,1)};
    // The wrist is fixed to the solved arm even while the casting palm turns.
    const auto wrist=XMVector3TransformNormal(XMVectorSet(0,-.055f,-.043f,0),frame);
    frame.r[3]=XMVectorSetW(XMLoadFloat3(&arm.end)-wrist,1);
  }
  HandPose result;
  XMStoreFloat4x4(&result.frame,frame);
  result.curl=gripping?1.f:.32f*(1-animation::SmoothStep(cast));
  constexpr float lengths[4][3]={{.041f,.026f,.021f},{.045f,.028f,.023f},{.042f,.026f,.022f},{.033f,.022f,.019f}};
  // Free palms face forward when casting: thumbs point toward the body's
  // centre. Grip frames turn their X axis along the held item's shaft.
  const float mirror=gripping?1.f:side?-1.f:1.f;
  result.mirrored=mirror<0;
  for(unsigned finger=0;finger<4;++finger) {
    auto& chain=result.fingers[finger];
    chain.width=finger==3?.020f:.023f;
    // Index is next to the thumb; the little finger has a lower knuckle.
    chain.joints[0]={mirror*(.033f-finger*.022f),finger==3?.014f:.024f,-.045f};
    const float splay=(1-result.curl)*mirror*(float(finger)-1.2f)*.10f;
    constexpr float closedAngles[3]={.70f,1.66f,2.79f};
    for(unsigned bone=0;bone<3;++bone) {
      constexpr float littleAngles[3]={.70f,1.30f,1.90f};
      const float angle=.07f+result.curl*((finger==3?littleAngles[bone]:closedAngles[bone])-.07f);
      const auto direction=XMVector3TransformNormal(XMVectorSet(0,std::cos(angle),std::sin(angle),0),XMMatrixRotationZ(splay));
      XMStoreFloat3(&chain.joints[bone+1],XMLoadFloat3(&chain.joints[bone])+direction*lengths[finger][bone]);
    }
  }
  auto& thumb=result.fingers[4]; thumb.width=.023f;
  thumb.joints[0]={mirror*.040f,-.040f,-.045f};
  // Opposition brings the thumb across the other fingers, instead of through
  // the handle. Rotate fixed-length bones to preserve volume while opening.
  const XMFLOAT3 opened[]={{mirror*.76f,.55f,.20f},{mirror*.48f,.84f,.20f},{mirror*.16f,.97f,.18f}};
  const XMFLOAT3 closed[]={{mirror*.35f,-.10f,.93f},{-mirror*.60f,.05f,.80f},{-mirror*.95f,.22f,.19f}};
  constexpr float thumbLengths[]={.035f,.030f,.024f};
  for(unsigned bone=0;bone<3;++bone) {
    const auto direction=XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&opened[bone]),XMLoadFloat3(&closed[bone]),result.curl));
    XMStoreFloat3(&thumb.joints[bone+1],XMLoadFloat3(&thumb.joints[bone])+direction*thumbLengths[bone]);
  }
  return result;
}
XMFLOAT3 SpellReleasePoint(const CharacterPose& pose,bool shield) {
  if(!shield) return BuildHandPose(pose,0,false,false,pose.handCast).Point({0,.07f,.035f});
  const auto offset=pose.UpperDirection({-.035f,0,.27f});
  const auto wrist=pose.arms[0].end;
  return {wrist.x+offset.x,wrist.y+offset.y,wrist.z+offset.z};
}
visual::HandPalette BuildHandRig(const HandPose& pose) {
  static const auto inverseBind=[] {
    std::array<std::array<XMFLOAT4X4,visual::handBones>,2> result{};
    for(unsigned side=0;side<2;++side) for(unsigned bone=1;bone<visual::handBones;++bone) {
      auto from=visual::HandBindPose()[bone].from,to=visual::HandBindPose()[bone].to;
      if(side) { from.x=-from.x; to.x=-to.x; }
      XMStoreFloat4x4(&result[side][bone],XMMatrixInverse(nullptr,visual::HandBoneFrame(from,to,(bone-1)/3,side!=0)));
    }
    return result;
  }();
  visual::HandPalette result; XMStoreFloat4x4(&result.joints[0],XMMatrixIdentity());
  for(unsigned finger=0;finger<5;++finger) for(unsigned bone=0;bone<3;++bone) {
    const unsigned index=1+finger*3+bone; const auto& chain=pose.fingers[finger];
    const auto local=visual::HandBoneFrame(chain.joints[bone],chain.joints[bone+1],finger,pose.mirrored);
    XMStoreFloat4x4(&result.joints[index],XMLoadFloat4x4(&inverseBind[pose.mirrored][index])*local);
  }
  return result;
}

DirectX::XMFLOAT3 WeaponReleasePoint(const CharacterPose& pose,rpg::WeaponForm form) {
  // These sockets match the barrel/bolt rail endpoints in cached weapon geometry.
  const DirectX::XMFLOAT3 muzzle{0,form==rpg::WeaponForm::Pistol?.16f:.191f,form==rpg::WeaponForm::Pistol?.40f:.36f};
  DirectX::XMFLOAT3 result;
  DirectX::XMStoreFloat3(&result,DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&muzzle),WeaponFrame(pose)));
  return result;
}
