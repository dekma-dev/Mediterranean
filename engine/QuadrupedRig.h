#pragma once
#include "engine/AuthoredAssets.h"
#include <cmath>

namespace authored::quadruped {
// Runtime sockets match the preserved Frostfang export. They are not a new rig
// or weights: existing clip matrices remain the input to these bounded solvers.
struct Leg {
 unsigned upper,lower,ankle,toe;
 DirectX::XMFLOAT3 hip,knee,wrist,paw;
};
inline Leg Limb(Asset asset,unsigned side) {
 const float x=(side%2==0?1.f:-1.f);
 if(asset==Asset::Wolf) {
   if(side<2) {const unsigned u=side==0?10:15;return {u,u+1,u+2,u+3,
     {x*.17f,.69f,.27f},{x*.17f,.47f,.22f},{x*.165f,.15f,.30f},{x*.165f,.065f,.34f}};}
   const unsigned u=side==2?22:26;return {u,u+1,u+2,u+3,
     {x*.17f,.67f,-.40f},{x*.17f,.43f,-.24f},{x*.165f,.23f,-.51f},{x*.165f,.065f,-.33f}};
 }
 const unsigned bone=side<2?(side==0?3:2):(side==2?5:4);const float z=side<2?.5f:-.5f;
 return {bone,bone,bone,bone,{x*.26f,.8f,z},{x*.26f,.4f,z},{x*.26f,.08f,z},{x*.26f,.08f,z}};
}
inline DirectX::XMFLOAT3 Point(const visual::BodyPalette& palette,unsigned bone,DirectX::XMFLOAT3 bind) {
 DirectX::XMFLOAT3 result;DirectX::XMStoreFloat3(&result,DirectX::XMVector3TransformCoord(
   DirectX::XMLoadFloat3(&bind),DirectX::XMLoadFloat4x4(&palette.joints[bone])));return result;
}
inline DirectX::XMFLOAT3 Paw(const visual::BodyPalette& palette,Asset asset,unsigned leg) {
 const auto bind=Limb(asset,leg);return Point(palette,bind.toe,bind.paw);
}
inline DirectX::XMMATRIX Align(DirectX::FXMVECTOR from,DirectX::FXMVECTOR to) {
 using namespace DirectX;
 const auto a=XMVector3Normalize(from),b=XMVector3Normalize(to);
 const float dot=std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-1.f,1.f);
 if(dot>.999999f)return XMMatrixIdentity();
 auto axis=XMVector3Cross(a,b);
 if(XMVectorGetX(XMVector3LengthSq(axis))<1e-8f) {
   axis=XMVector3Cross(a,XMVectorSet(1,0,0,0));
   if(XMVectorGetX(XMVector3LengthSq(axis))<1e-8f)axis=XMVector3Cross(a,XMVectorSet(0,0,1,0));
 }
 return XMMatrixRotationAxis(XMVector3Normalize(axis),std::acos(dot));
}
inline void Apply(visual::BodyPalette& palette,Asset asset,const Gesture& gesture) {
 using namespace DirectX;
 if(asset!=Asset::Wolf && asset!=Asset::Boar)return;
 // A corpse keeps its captured corrections while collapse fades them away.
 const float alive=1-std::clamp(std::isfinite(gesture.collapse)?gesture.collapse:0.f,0.f,1.f);
 for(unsigned i=0;i<4;++i) {
   const auto& foot=gesture.paws[i];
   const float weight=std::clamp(std::isfinite(foot.weight)?foot.weight:0.f,0.f,1.f)*alive;
   if(weight<=0 || !std::isfinite(foot.target.x+foot.target.y+foot.target.z))continue;
   const auto leg=Limb(asset,i);
   const auto point=[&](unsigned bone,XMFLOAT3 p){auto value=Point(palette,bone,p);return XMLoadFloat3(&value);};
   const auto hip=point(leg.upper,leg.hip),paw=point(leg.toe,leg.paw);
   auto delta=XMVectorScale(XMVectorSubtract(XMLoadFloat3(&foot.target),paw),weight);
   const float shift=XMVectorGetX(XMVector3Length(delta));
   const float limit=asset==Asset::Wolf?.24f:.12f;
   if(shift>limit)delta=XMVectorScale(delta,limit/shift);
   const auto target=XMVectorAdd(paw,delta);
   const auto transform=[&](unsigned bone,FXMVECTOR oldRoot,FXMVECTOR newRoot,FXMMATRIX rotation) {
     const auto correction=XMMatrixTranslationFromVector(XMVectorNegate(oldRoot))*rotation*XMMatrixTranslationFromVector(newRoot);
     XMStoreFloat4x4(&palette.joints[bone],XMLoadFloat4x4(&palette.joints[bone])*correction);
     XMStoreFloat4x4(&palette.normals[bone],XMLoadFloat4x4(&palette.normals[bone])*rotation);
   };
   if(asset==Asset::Boar) {
     // This accepted rig has one bone per leg. Swing it about the shoulder and
     // allow only a small vertical reach adjustment; do not invent knee weights.
     const auto rotation=Align(XMVectorSubtract(paw,hip),XMVectorSubtract(target,hip));
     const auto rotated=XMVectorAdd(hip,XMVector3TransformNormal(XMVectorSubtract(paw,hip),rotation));
     const float lift=std::clamp(XMVectorGetY(XMVectorSubtract(target,rotated)),-.055f,.055f);
     transform(leg.upper,hip,XMVectorAdd(hip,XMVectorSet(0,lift,0,0)),rotation);continue;
   }
   const auto knee=point(leg.lower,leg.knee),wrist=point(leg.ankle,leg.wrist);
   const float upper=XMVectorGetX(XMVector3Length(XMVectorSubtract(knee,hip)));
   const float lower=XMVectorGetX(XMVector3Length(XMVectorSubtract(wrist,knee)));
   if(upper<.01f || lower<.01f)continue;
   auto wanted=XMVectorAdd(wrist,delta),direction=XMVectorSubtract(wanted,hip);
   const float length=XMVectorGetX(XMVector3Length(direction));if(length<1e-5f)continue;
   direction=XMVectorScale(direction,1/length);
   const float reach=std::clamp(length,std::abs(upper-lower)+.005f,(upper+lower)*.995f);
   wanted=XMVectorAdd(hip,XMVectorScale(direction,reach));
   auto bend=XMVectorSubtract(knee,hip);
   bend=XMVectorSubtract(bend,XMVectorMultiply(direction,XMVector3Dot(bend,direction)));
   if(XMVectorGetX(XMVector3LengthSq(bend))<1e-7f) {
     bend=XMVectorSet(0,0,i<2?-1.f:1.f,0);
     bend=XMVectorSubtract(bend,XMVectorMultiply(direction,XMVector3Dot(bend,direction)));
   }
   bend=XMVector3Normalize(bend);
   const float along=(upper*upper-lower*lower+reach*reach)/(2*reach);
   const auto solved=XMVectorAdd(XMVectorAdd(hip,XMVectorScale(direction,along)),XMVectorScale(bend,std::sqrt(std::max(0.f,upper*upper-along*along))));
   const auto upperRotation=Align(XMVectorSubtract(knee,hip),XMVectorSubtract(solved,hip));
   const auto lowerRotation=Align(XMVectorSubtract(wrist,knee),XMVectorSubtract(wanted,solved));
   transform(leg.upper,hip,hip,upperRotation);
   transform(leg.lower,knee,solved,lowerRotation);
   // The distal lever and toe retain their authored orientation, following the
   // solved wrist as a unit. Skin normals use the same rotation as positions.
   transform(leg.ankle,wrist,wanted,XMMatrixIdentity());
   transform(leg.toe,wrist,wanted,XMMatrixIdentity());
 }
}
}
