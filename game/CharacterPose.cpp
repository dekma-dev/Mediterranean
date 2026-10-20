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
  const float y=p.y*std::cos(upperPitch)-p.z*std::sin(upperPitch);
  const float z=p.y*std::sin(upperPitch)+p.z*std::cos(upperPitch);
  return {p.x*std::cos(upperYaw)+z*std::sin(upperYaw),y,-p.x*std::sin(upperYaw)+z*std::cos(upperYaw)};
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
  const float dash=animation::Unit(dynamics.dash),brake=animation::Unit(dynamics.dashBrake);
  const float dashImpact=animation::Unit(dynamics.dashImpact);
  // The drive/bracing pose takes priority over an earlier attack or cast pose.
  const float burst=std::max(dash,brake);
  const auto stroke=combat::Sample(dynamics.attackSeconds,dynamics.heavyAttack);
  const bool timed=dynamics.attackSeconds>=0;
  if (timed) swing=stroke.weight;
  swing*=1-burst;
  const float cast=animation::Unit(dynamics.cast)*(1-swing)*(1-hit)*(1-burst);
  CharacterPose pose;
  pose.bodyLift=gait*gait*.015f*(1-air)+breath*.006f*(1-air)-landing*.14f;
  pose.bodyLift-=dash*.09f+brake*.055f+dashImpact*.04f+animation::Unit(dynamics.dashLoad)*.035f;
  pose.upperPitch=travel*.09f+animation::Signed(dynamics.lean)*.10f+landing*.12f+air*std::max(0.f,rise)*.06f-hit*.12f+sway*.015f*(1-air);
  pose.upperPitch+=dash*.42f-brake*.16f-dashImpact*.14f;
  pose.upperPitch-=cast*.08f;
  pose.upperYaw=animation::Signed(dynamics.bank)*.08f;
  if (timed) {
    pose.upperYaw+=stroke.weight*((dynamics.heavyAttack?-.46f:-.36f)+stroke.cut*.82f)*(1-burst);
    pose.upperPitch+=stroke.weight*(-.10f+stroke.cut*.22f)*(1-burst);
    pose.bodyLift-=stroke.coil*(dynamics.heavyAttack?.065f:.035f)*(1-air);
  }
  std::array<XMFLOAT3,2> contacts{};
  if (dynamics.feetActive) for (int i=0;i<2;++i) {
    const auto& foot=dynamics.feet[i];
    const float hip=i==0?-.17f:.17f;
    // Bound malformed/extreme targets before solving. Lower the pelvis to keep
    // normal planted ankles exact rather than stretching a leg or lifting a boot.
    float x=std::isfinite(foot.x)?foot.x-hip:0,z=std::isfinite(foot.z)?foot.z:0;
    const float distance=std::hypot(x,z),scale=distance>.46f?.46f/distance:1.f;
    x*=scale; z*=scale;
    const float y=std::isfinite(foot.y)?std::clamp(foot.y,.17f,.32f):.17f;
    contacts[i]={hip+x,y,z};
    pose.footYaw[i]=std::isfinite(foot.yaw)?std::remainder(foot.yaw,motion::tau):0;
    constexpr float reach=CharacterPose::thigh+CharacterPose::shin-.002f;
    pose.bodyLift=std::min(pose.bodyLift,y+std::sqrt(reach*reach-x*x-z*z)-.93f);
  }
  // The push leg sweeps back, then the opposite leg reaches into the catch.
  const float drive=1-2*animation::SmoothStep((animation::Unit(dynamics.dashPhase)-.20f)/.50f);
  for (int i=0;i<2;++i) {
    const float side=i==0?-1.f:1.f,stride=side*(gait*(1-burst)+dash*.95f*drive-brake*.65f)*(1-air);
    const float positiveRise=std::max(0.f,rise),step=std::max(0.f,stride);
    const float lift=air*(.17f+positiveRise*positiveRise*.15f+(i==0?.045f:0))*
      (1-animation::Unit(dynamics.takeoff)*(i==0?.45f:.9f));
    pose.legs[i]=Solve({side*.17f,.93f+pose.bodyLift,0},{side*(.18f+air*.015f),.17f+step*step*.055f+lift,
      stride*.22f+air*(i==0?.13f:-.10f)},
      {0,0,1},CharacterPose::thigh,CharacterPose::shin);
    if (dynamics.feetActive)
      pose.legs[i]=Solve({side*.17f,.93f+pose.bodyLift,0},contacts[i],{0,0,1},CharacterPose::thigh,CharacterPose::shin);
    const float armStride=dynamics.feetActive?std::clamp(contacts[i].z/.30f,-1.f,1.f):stride;
    const float attack=i==1?swing:0;
    XMFLOAT3 hand{side*(.40f+air*.035f),1.02f+attack*.52f+air*.12f+landing*.08f+hit*.08f+dash*.12f+brake*.07f,
      .10f-armStride*.13f+attack*.21f+sway*side*.014f-dash*.20f+brake*.12f};
    if (i==0 && shield) hand={-.39f,1.13f+air*.08f+landing*.08f,.23f-armStride*.04f+dash*.09f};
    if (timed) {
      const float cut=stroke.cut,weight=stroke.weight*(1-burst);
      // Coil beside the shoulder, cut across the body, then return to guard.
      const XMFLOAT3 attackHand=i==1?
        XMFLOAT3{.46f-cut*.34f,(dynamics.heavyAttack?1.66f:1.48f)-cut*.38f,-.13f+cut*.59f}:
        XMFLOAT3{-.34f,1.25f,.23f-cut*.10f};
      hand.x+=(attackHand.x-hand.x)*weight;
      hand.y+=(attackHand.y-hand.y)*weight;
      hand.z+=(attackHand.z-hand.z)*weight;
    }
    // The free hand opens forward while the weapon arm lifts clear of the cast.
    const XMFLOAT3 spellHand{i==0?-.29f:.43f,i==0?1.39f:1.19f,i==0?.40f:.23f};
    hand.x+=(spellHand.x-hand.x)*cast; hand.y+=(spellHand.y-hand.y)*cast; hand.z+=(spellHand.z-hand.z)*cast;
    pose.arms[i]=Solve(pose.UpperPoint({side*.305f,1.43f,0}),pose.UpperPoint(hand),
      pose.UpperDirection({side*.8f,-.2f,-1}),CharacterPose::upperArm,CharacterPose::forearm);
  }
  XMFLOAT3 weapon{.12f,-.76f+swing*1.72f+landing*.25f+air*.20f+dash*.32f+brake*.16f,.63f-swing*.45f};
  if (timed) {
    const float weight=stroke.weight*(1-burst);
    const XMFLOAT3 cut{.22f-stroke.cut*.38f,1.05f-stroke.cut*1.25f,.05f+stroke.cut*1.05f};
    const XMFLOAT3 rest{.12f,-.76f,.63f};
    const auto a=XMVector3Normalize(XMLoadFloat3(&rest)),b=XMVector3Normalize(XMLoadFloat3(&cut));
    const float angle=std::acos(std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-.9999f,1.f));
    // Interpolate orientation on the unit sphere: linear direction blending
    // accelerated sharply as the blade crossed from hanging down to raised.
    const auto blended=angle>.0001f?
      (a*std::sin((1-weight)*angle)+b*std::sin(weight*angle))/std::sin(angle):a;
    XMStoreFloat3(&weapon,blended);
  }
  const auto direction=pose.UpperDirection(weapon);
  XMStoreFloat3(&pose.weaponDirection,XMVector3Normalize(XMLoadFloat3(&direction)));
  return pose;
}
