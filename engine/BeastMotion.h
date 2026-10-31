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
 const float jawAngle=unit(gesture.jaw)*.6f*(1-collapse);
 const auto jawRotation=XMMatrixRotationX(jawAngle);
 const auto jaw=XMMatrixTranslation(0,-.83f,-.63f)*jawRotation*XMMatrixTranslation(0,.83f,.63f);
 for(unsigned bone=0;bone<visual::paletteBones;++bone) {
  auto transform=XMLoadFloat4x4(&palette.joints[bone]);
  auto rotation=XMLoadFloat4x4(&palette.normals[bone]);
  if(wolf && bone==6) {transform*=jaw;rotation*=jawRotation;}
  if((wolf && bone>=4 && bone<=8) || (!wolf && bone==1)) {transform*=head;rotation*=headRotation;}
  if(collapse>0) {transform*=fall;rotation*=fallRotation;}
  XMStoreFloat4x4(&palette.joints[bone],transform);
  XMStoreFloat4x4(&palette.normals[bone],rotation);
 }
}
}
