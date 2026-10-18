#include "game/CharacterPose.h"
#include <algorithm>
#include <cmath>

namespace {
using namespace DirectX;
LimbPose Solve(XMFLOAT3 root,XMFLOAT3 target,XMFLOAT3 bend,float upper,float lower) {
  const auto origin=XMLoadFloat3(&root),delta=XMLoadFloat3(&target)-origin;
  const float requested=XMVectorGetX(XMVector3Length(delta));
  const auto axis=requested>.0001f?delta/requested:XMVectorSet(0,-1,0,0);
  const float distance=std::clamp(requested,std::abs(upper-lower)+.001f,upper+lower-.001f);
  auto plane=XMLoadFloat3(&bend); plane-=axis*XMVector3Dot(plane,axis);
  if (XMVectorGetX(XMVector3LengthSq(plane))<.0001f) plane=XMVector3Cross(axis,XMVectorSet(1,0,0,0));
  plane=XMVector3Normalize(plane);
  const float along=(upper*upper-lower*lower+distance*distance)/(2*distance);
  const float height=std::sqrt(std::max(0.f,upper*upper-along*along));
  LimbPose result; result.root=root;
  XMStoreFloat3(&result.joint,origin+axis*along+plane*height);
  XMStoreFloat3(&result.end,origin+axis*distance);
  return result;
}
}
CharacterPose BuildCharacterPose(float swing,float gait,bool shield) {
  using namespace DirectX;
  swing=std::isfinite(swing)?std::clamp(swing,0.f,1.f):0;
  gait=std::isfinite(gait)?std::clamp(gait,-1.f,1.f):0;
  CharacterPose pose; pose.bodyLift=gait*gait*.015f;
  for (int i=0;i<2;++i) {
    const float side=i==0?-1.f:1.f,stride=side*gait;
    pose.legs[i]=Solve({side*.17f,.93f+pose.bodyLift,0},{side*.18f,.17f+std::max(0.f,stride)*.055f,stride*.22f},
      {0,0,1},CharacterPose::thigh,CharacterPose::shin);
    const float attack=i==1?swing:0;
    XMFLOAT3 hand{side*.40f,1.02f+pose.bodyLift+attack*.52f,.10f-stride*.13f+attack*.21f};
    if (i==0 && shield) hand={-.39f,1.13f+pose.bodyLift,.23f-stride*.04f};
    pose.arms[i]=Solve({side*.305f,1.43f+pose.bodyLift,0},hand,{side*.8f,-.2f,-1},CharacterPose::upperArm,CharacterPose::forearm);
  }
  const XMFLOAT3 direction{.12f,-.76f+swing*1.72f,.63f-swing*.45f};
  XMStoreFloat3(&pose.weaponDirection,XMVector3Normalize(XMLoadFloat3(&direction)));
  return pose;
}
