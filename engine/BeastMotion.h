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
 const float headAngle=(wolf?-.22f:.30f)*strength;
 const auto headRotation=XMMatrixRotationX(headAngle);
 const float neckHeight=wolf?.80f:1.f,neckForward=wolf?.29f:.45f;
 const auto head=XMMatrixTranslation(0,-neckHeight,-neckForward)*headRotation*
   XMMatrixTranslation(0,neckHeight,neckForward+(wolf?.10f:0.f)*strength);
 const float roll=collapse*XM_PIDIV2;
 const auto fallRotation=XMMatrixRotationZ(roll);
 const auto& bounds=Collection()[unsigned(asset)*lodCount].bounds;
 // The side of the authored bounding box rests on the floor at full collapse.
 const auto fall=fallRotation*XMMatrixTranslation(0,-bounds.minimum.x*std::sin(roll)+.015f*collapse,0);
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
  if((wolf && bone>=4 && bone<=8) || (!wolf && bone==1)) {transform*=head;rotation*=headRotation;}
  // Recoil/banking affects the trunk and head; planted legs keep their clip pose.
  if((wolf && ((bone>=1 && bone<=8) || (bone>=19 && bone<=21))) || (!wolf && bone<=1)) {
    transform*=body;rotation*=bodyRotation;
  }
  if(collapse>0) {transform*=fall;rotation*=fallRotation;}
  transform*=support;rotation*=supportRotation;
  XMStoreFloat4x4(&palette.joints[bone],transform);
  XMStoreFloat4x4(&palette.normals[bone],rotation);
 }
}
}
