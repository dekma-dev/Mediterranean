#include "game/HandPose.h"
#include "game/WeaponGrip.h"
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
  const auto heldRight=XMLoadFloat3(&pose.weaponRight);
  const auto right=XMVectorGetX(XMVector3LengthSq(heldRight))>.5f?heldRight:
    XMVector3Normalize(lateral-up*XMVector3Dot(lateral,up));
  const auto origin=pose.WeaponPoint(0);
  return {right,up,XMVector3Cross(right,up),XMVectorSetW(XMLoadFloat3(&origin),1)};
}
XMMATRIX ShieldFrame(const CharacterPose& pose) {
  static const XMFLOAT3 reference=[] {
    const auto neutral=BuildCharacterPose(0,0,true,{});
    XMFLOAT3 value;XMStoreFloat3(&value,XMVector3Normalize(XMLoadFloat3(&neutral.arms[0].end)-XMLoadFloat3(&neutral.arms[0].joint)));
    return value;
  }();
  const auto torso=XMMatrixRotationX(pose.upperPitch)*XMMatrixRotationZ(pose.upperRoll)*XMMatrixRotationY(pose.upperYaw);
  const auto forearm=XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&pose.arms[0].end)-XMLoadFloat3(&pose.arms[0].joint),XMMatrixTranspose(torso)));
  const auto from=XMLoadFloat3(&reference);const float dot=XMVectorGetX(XMVector3Dot(from,forearm));
  auto q=XMVectorSetW(XMVector3Cross(from,forearm),1+dot);
  if(dot<-.9999f)q=XMQuaternionRotationAxis(XMVector3Normalize(XMVector3Cross(from,XMVectorSet(0,1,0,0))),XM_PI);
  else q=XMQuaternionNormalize(q);
  auto result=XMMatrixRotationQuaternion(q)*torso;
  result.r[3]=XMVectorSetW(XMLoadFloat3(&pose.arms[0].end),1);
  return result;
}
HandPose BuildHandPose(const CharacterPose& pose,unsigned side,bool weapon,bool shield,float cast) {
  side=std::min(side,1u);
  const float staffGrip=side||shield||!weapon?0:animation::Unit(pose.staffGripWeight);
  const float bowGrip=!side&&weapon&&!shield?(pose.fittedBowGrip?pose.bowStringGrip:pose.bowDraw*(1.1f-.1f*pose.bowDraw)):0;
  const bool gripping=side?weapon:shield||staffGrip>0||bowGrip>0;
  const bool fitted=side&&weapon&&pose.fittedBladeGrip;
  const bool ranged=weapon&&(pose.fittedStaffGrip||pose.fittedBowGrip);
  const bool rangedHolding=ranged&&side;
  const auto& arm=pose.arms[side];
  const auto forearm=XMVector3Normalize(XMLoadFloat3(&arm.end)-XMLoadFloat3(&arm.joint));
  XMMATRIX frame;
  if(side && weapon) {
    const auto blade=WeaponFrame(pose);
    frame={blade.r[1],blade.r[0],-blade.r[2],blade.r[3]};
    if(fitted)frame.r[3]+=frame.r[0]*weapongrip::swordAlong+frame.r[2]*weapongrip::swordPalmInset;
    if(ranged) {
      frame=weapongrip::RangedPalm(blade.r[1],blade.r[0],pose.fittedBowGrip,side);
      frame.r[3]=blade.r[3]+frame.r[2]*weapongrip::rangedPalmInset;
    }
  } else if(!side && shield) {
    const auto plate=ShieldFrame(pose);
    frame={plate.r[1],-plate.r[0],plate.r[2],plate.r[3]};
  } else {
    const auto rightLocal=pose.UpperDirection({1,0,0}),upLocal=pose.UpperDirection({0,1,0}),forwardLocal=pose.UpperDirection({0,0,1});
    const auto right=XMLoadFloat3(&rightLocal),up=XMLoadFloat3(&upLocal),forward=XMLoadFloat3(&forwardLocal);
    const float reach=animation::SmoothStep(cast)*(side?.45f:1.f);
    // Lift the actual forearm direction toward the open casting pose in
    // vector space. Scaling a wrapped elevation angle made the wrist spin
    // at both vertical and lateral reaches. A small forward arc gives the
    // palm a natural path as it opens from a hanging arm.
    auto lifted=forearm*(1-reach)+up*reach+forward*(2.f*reach*(1-reach));
    if(XMVectorGetX(XMVector3LengthSq(lifted))<.0001f)lifted=forward;
    const auto fingerAxis=XMVector3Normalize(lifted);
    auto radialAxis=right-fingerAxis*XMVector3Dot(right,fingerAxis);
    if(XMVectorGetX(XMVector3LengthSq(radialAxis))<.0001f)
      radialAxis=XMVector3Cross(fingerAxis,forward);
    radialAxis=XMVector3Normalize(radialAxis);
    const auto normal=XMVector3Cross(radialAxis,fingerAxis);
    // Construct the roll in this orthonormal frame. Projecting a blended
    // palm reference onto fingerAxis's plane flips when both nearly align.
    // Relaxed palms face inward; casting untwists them without that pole.
    const float twist=(side?-1.f:1.f)*(1-reach)*1.57079633f;
    const auto across=radialAxis*std::cos(twist)-normal*std::sin(twist);
    const auto palm=normal*std::cos(twist)+radialAxis*std::sin(twist);
    frame={across,fingerAxis,palm,XMVectorSet(0,0,0,1)};
    // The wrist is fixed to the solved arm even while the casting palm turns.
    const auto wrist=XMVector3TransformNormal(XMVectorSet(0,-.055f,-.043f,0),frame);
    frame.r[3]=XMVectorSetW(XMLoadFloat3(&arm.end)-wrist,1);
  }
  if(staffGrip>0 || bowGrip>0) {
    const auto shaft=WeaponFrame(pose);
    auto held=ranged?weapongrip::RangedPalm(shaft.r[1],shaft.r[0],pose.fittedBowGrip,side):
      XMMATRIX(shaft.r[1],shaft.r[0],-shaft.r[2],XMVectorSet(0,0,0,1));
    held.r[3]=XMVectorSetW(XMLoadFloat3(&arm.end),1);
    const float grip=std::max(staffGrip,bowGrip);
    const auto position=XMVectorLerp(frame.r[3],held.r[3],grip);
    frame=XMMatrixRotationQuaternion(XMQuaternionSlerp(XMQuaternionRotationMatrix(frame),XMQuaternionRotationMatrix(held),grip));
    frame.r[3]=position;
  }
  if(pose.hasGripSocket && !side) {
    // Reanchor after orientation blending, so the palm cannot pull the wrist
    // away from the forearm during a change between free and supporting hands.
    const auto wrist=XMVector3TransformNormal(XMVectorSet(0,-.055f,-.043f,0),frame);
    frame.r[3]=XMVectorSetW(XMLoadFloat3(&arm.end)-wrist,1);
  }
  HandPose result;
  XMStoreFloat4x4(&result.frame,frame);
  const float relaxed=std::clamp(pose.freeHandCurl,.2f,.85f)*(1-animation::SmoothStep(cast));
  result.curl=side?weapon?1.f:relaxed:
    shield?1.f:staffGrip+(1-staffGrip)*relaxed;
  if(!side && weapon && !shield)result.curl=std::max(result.curl,.85f*pose.bowDraw);
  constexpr float lengths[4][3]={{.041f,.026f,.021f},{.045f,.028f,.023f},{.042f,.026f,.022f},{.033f,.022f,.019f}};
  // Free palms face forward when casting: thumbs point toward the body's
  // centre. Grip frames turn their X axis along the held item's shaft.
  const float mirror=ranged?(side?-1.f:1.f):gripping?1.f:side?-1.f:1.f;
  result.mirrored=mirror<0;
  for(unsigned finger=0;finger<4;++finger) {
    auto& chain=result.fingers[finger];
    chain.width=finger==3?.020f:.023f;
    // Index is next to the thumb; the little finger has a lower knuckle.
    chain.joints[0]={mirror*(.033f-finger*.022f),finger==3?.014f:.024f,-.045f};
    const float splay=(1-result.curl)*mirror*(float(finger)-1.2f)*.10f*(ranged&&!side&&pose.fittedBowGrip?1-bowGrip:1);
    constexpr float closedAngles[3]={.70f,1.66f,2.79f};
    for(unsigned bone=0;bone<3;++bone) {
      constexpr float littleAngles[3]={.70f,1.30f,1.90f};
      const float closed=fitted?weapongrip::swordCurl[finger][bone]:rangedHolding?
        (pose.fittedBowGrip?weapongrip::bowCurl:weapongrip::staffCurl)[finger][bone]:finger==3?littleAngles[bone]:closedAngles[bone];
      float angle=.07f+result.curl*(closed-.07f);
      if(ranged&&!side&&pose.fittedStaffGrip) {
        const float resting=.07f+relaxed*(closed-.07f);
        angle=resting+(weapongrip::staffCurl[finger][bone]-resting)*staffGrip;
      }
      if(ranged&&!side&&pose.fittedBowGrip&&finger<3) {
        const float resting=.07f+relaxed*(closed-.07f);
        angle=resting+(weapongrip::StringHook(finger,lengths[finger])[bone]-resting)*bowGrip;
      }
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
  const XMFLOAT3 fittedThumb[]={{mirror*.35f,.05f,.935f},{-mirror*.60f,.40f,.69f},{-mirror*.96f,.28f,-.04f}};
  constexpr float thumbLengths[]={.035f,.030f,.024f};
  for(unsigned bone=0;bone<3;++bone) {
    const float thumbCurl=ranged&&!side&&pose.fittedBowGrip?relaxed*(1-bowGrip)+.2f*bowGrip:result.curl;
    const auto direction=XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&opened[bone]),XMLoadFloat3(&(fitted?fittedThumb:closed)[bone]),thumbCurl));
    XMStoreFloat3(&thumb.joints[bone+1],XMLoadFloat3(&thumb.joints[bone])+direction*thumbLengths[bone]);
  }
  return result;
}
XMFLOAT3 SpellReleasePoint(const CharacterPose& pose,bool shield) {
  if(!shield) return BuildHandPose(pose,0,false,false,pose.handCast).Point({0,.07f,.035f});
  XMFLOAT3 offset;XMStoreFloat3(&offset,XMVector3TransformNormal(XMVectorSet(-.035f,0,.27f,0),ShieldFrame(pose)));
  const auto wrist=BuildHandPose(pose,0,false,true,pose.handCast).Point({0,0,0});
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
  const DirectX::XMFLOAT3 muzzle{0,form==rpg::WeaponForm::Pistol?.16f:.035f,form==rpg::WeaponForm::Pistol?.40f:.78f};
  DirectX::XMFLOAT3 result;
  DirectX::XMStoreFloat3(&result,DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&muzzle),WeaponFrame(pose)));
  return result;
}
