#pragma once
#include "engine/AuthoredAssets.h"
#include <cmath>

namespace authored {
// Additive combat over the accepted wolf clips and the boar presentation rig.
// Bone indices follow the preserved Frostfang 30-joint export (neck 4,
// head 5, jaw 6, ears 7/8). No source mesh, clip, weight or bind pose is rebuilt.
inline void ApplyBeastCombat(visual::BodyPalette& palette,Asset asset,float attack,Gesture gesture) {
 using namespace DirectX;
 const auto unit=[](float value){return std::isfinite(value)?std::clamp(value,0.f,1.f):0.f;};
 const float collapse=unit(gesture.collapse),strength=unit(attack)*(1-collapse);
 const auto bounded=[](float v,float limit){return std::isfinite(v)?std::clamp(v,-limit,limit):0.f;};
 const bool wolf=asset==Asset::Wolf;
 const float headAngle=(wolf?-.22f:.30f)*strength+bounded(gesture.headPitch,.18f)*(1-collapse);
 const auto headRotation=XMMatrixRotationX(headAngle)*XMMatrixRotationY(bounded(gesture.headYaw,.20f)*(1-collapse));
 const float neckHeight=wolf?.80f:1.f,neckForward=wolf?.29f:.45f;
 const auto head=XMMatrixTranslation(0,-neckHeight,-neckForward)*headRotation*
   XMMatrixTranslation(0,neckHeight,neckForward+(wolf?.10f:0.f)*strength);
 // The accepted muzzle has mixed head/jaw weights while the nose is rigidly
 // head-bound. A broad gape separates them; keep this overlay inside the rig's
 // existing articulation range instead of rewriting the user's skin weights.
 const float jawAngle=unit(gesture.jaw)*.10f*(1-collapse);
 const auto jawRotation=XMMatrixRotationX(jawAngle);
 const auto jaw=XMMatrixTranslation(0,-.83f,-.63f)*jawRotation*XMMatrixTranslation(0,.83f,.63f);
 const auto bodyRotation=XMMatrixRotationRollPitchYaw(bounded(gesture.pitch,.22f),bounded(gesture.yaw,.18f),bounded(gesture.roll,.18f));
 const auto body=XMMatrixTranslation(0,-.75f,0)*bodyRotation*XMMatrixTranslation(0,.75f+bounded(gesture.hip,.06f),0);
 const auto supportRotation=XMMatrixRotationRollPitchYaw(bounded(gesture.supportPitch,.38f),0,bounded(gesture.supportRoll,.30f));
 const auto support=supportRotation*XMMatrixTranslation(0,bounded(gesture.supportLift,.18f),0);
 for(unsigned bone=0;bone<visual::paletteBones;++bone) {
  auto transform=XMLoadFloat4x4(&palette.joints[bone]);
  auto rotation=XMLoadFloat4x4(&palette.normals[bone]);
  if(wolf && bone==6) {transform*=jaw;rotation*=jawRotation;}
  if(wolf && (bone==7 || bone==8)) {
    const float side=bone==7?1.f:-1.f;
    const auto earRotation=XMMatrixRotationZ(side*bounded(gesture.ears,.15f)*(1-collapse));
    transform*=XMMatrixTranslation(-side*.08f,-1.f,-.60f)*earRotation*XMMatrixTranslation(side*.08f,1.f,.60f);
    rotation*=earRotation;
  }
  if(wolf && bone>=19 && bone<=21) {
    const auto tailRotation=XMMatrixRotationY(bounded(gesture.tail,.15f)*(1-collapse));
    transform*=XMMatrixTranslation(0,-.68f,.56f)*tailRotation*XMMatrixTranslation(0,.68f,-.56f);
    rotation*=tailRotation;
  }
  if((wolf && bone>=4 && bone<=8) || (!wolf && bone==1)) {transform*=head;rotation*=headRotation;}
  // Recoil/banking affects the trunk and head; planted legs keep their clip pose.
  if((wolf && ((bone>=1 && bone<=8) || (bone>=19 && bone<=21))) || (!wolf && bone<=1)) {
    transform*=body;rotation*=bodyRotation;
  }
  transform*=support;rotation*=supportRotation;
  XMStoreFloat4x4(&palette.joints[bone],transform);
  XMStoreFloat4x4(&palette.normals[bone],rotation);
 }
}
// Collapse happens after paw corrections, around the same supported ground
// plane. Otherwise an absolute foot target fights the fall and tears the limbs.
inline void ApplyBeastCollapse(visual::BodyPalette& palette,Asset asset,Gesture gesture) {
 using namespace DirectX;
 const auto bound=[](float v,float limit){return std::isfinite(v)?std::clamp(v,-limit,limit):0.f;};
 const float collapse=std::clamp(bound(gesture.collapse,1),0.f,1.f);if(collapse<=0)return;
 const float roll=collapse*XM_PIDIV2;
 const auto rotation=XMMatrixRotationZ(roll);
 const auto& bounds=Collection()[unsigned(asset)*lodCount].bounds;
 const auto fall=rotation*XMMatrixTranslation(0,-bounds.minimum.x*std::sin(roll)+.015f*collapse,0);
 const auto groundRotation=XMMatrixRotationRollPitchYaw(bound(gesture.supportPitch,.38f),0,bound(gesture.supportRoll,.30f));
 const auto ground=groundRotation*XMMatrixTranslation(0,bound(gesture.supportLift,.18f),0);
 const auto transform=XMMatrixInverse(nullptr,ground)*fall*ground;
 const auto normals=XMMatrixTranspose(groundRotation)*rotation*groundRotation;
 for(unsigned b=0;b<visual::paletteBones;++b) {
   XMStoreFloat4x4(&palette.joints[b],XMLoadFloat4x4(&palette.joints[b])*transform);
   XMStoreFloat4x4(&palette.normals[b],XMLoadFloat4x4(&palette.normals[b])*normals);
 }
}
}
