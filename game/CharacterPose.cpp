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
DirectX::XMFLOAT3 CharacterPose::UpperDirection(DirectX::XMFLOAT3 p) const {
  return {p.x,p.y*std::cos(upperPitch)-p.z*std::sin(upperPitch),p.y*std::sin(upperPitch)+p.z*std::cos(upperPitch)};
}
DirectX::XMFLOAT3 CharacterPose::UpperPoint(DirectX::XMFLOAT3 p) const {
  p.y-=.95f; p=UpperDirection(p); p.y+=.95f+bodyLift; return p;
}
DirectX::XMFLOAT3 CharacterPose::WeaponPoint(float distance) const {
  const auto hand=arms[1].end;
  return {hand.x+weaponDirection.x*distance,hand.y+weaponDirection.y*distance,hand.z+weaponDirection.z*distance};
}
CharacterPose BuildCharacterPose(float swing,float gait,bool shield,animation::Sample dynamics) {
  using namespace DirectX;
  swing=std::isfinite(swing)?std::clamp(swing,0.f,1.f):0;
  gait=std::isfinite(gait)?std::clamp(gait,-1.f,1.f):0;
  const float air=animation::Unit(dynamics.air),rise=animation::Signed(dynamics.rise);
  const float landing=animation::Unit(dynamics.landing),hit=animation::Unit(dynamics.hit);
  const float breath=animation::Signed(dynamics.breath),sway=animation::Signed(dynamics.sway);
  const float travel=animation::Unit(dynamics.travel);
  CharacterPose pose;
  pose.bodyLift=gait*gait*.015f*(1-air)+breath*.006f*(1-air)-landing*.10f;
  pose.upperPitch=travel*.07f+landing*.08f-hit*.12f+sway*.015f*(1-air);
  for (int i=0;i<2;++i) {
    const float side=i==0?-1.f:1.f,stride=side*gait*(1-air);
    const float positiveRise=std::max(0.f,rise),step=std::max(0.f,stride);
    const float lift=air*(.14f+positiveRise*positiveRise*.12f+(i==0?.035f:0));
    pose.legs[i]=Solve({side*.17f,.93f+pose.bodyLift,0},{side*(.18f+air*.015f),.17f+step*step*.055f+lift,
      stride*.22f+air*(i==0?.13f:-.10f)},
      {0,0,1},CharacterPose::thigh,CharacterPose::shin);
    const float attack=i==1?swing:0;
    XMFLOAT3 hand{side*(.40f+air*.035f),1.02f+attack*.52f+air*.12f+landing*.08f+hit*.08f,
      .10f-stride*.13f+attack*.21f+sway*side*.014f};
    if (i==0 && shield) hand={-.39f,1.13f+air*.08f+landing*.08f,.23f-stride*.04f};
    pose.arms[i]=Solve(pose.UpperPoint({side*.305f,1.43f,0}),pose.UpperPoint(hand),
      pose.UpperDirection({side*.8f,-.2f,-1}),CharacterPose::upperArm,CharacterPose::forearm);
  }
  const auto direction=pose.UpperDirection({.12f,-.76f+swing*1.72f+landing*.25f+air*.20f,.63f-swing*.45f});
  XMStoreFloat3(&pose.weaponDirection,XMVector3Normalize(XMLoadFloat3(&direction)));
  return pose;
}
