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
  const float x=p.x*std::cos(upperRoll)-y*std::sin(upperRoll);
  return {x*std::cos(upperYaw)+z*std::sin(upperYaw),p.x*std::sin(upperRoll)+y*std::cos(upperRoll),
    -x*std::sin(upperYaw)+z*std::cos(upperYaw)};
}
DirectX::XMFLOAT3 CharacterPose::UpperPoint(DirectX::XMFLOAT3 p) const {
  p.y-=.95f; p=UpperDirection(p); p.y+=.95f+bodyLift; p.x+=bodyShift; return p;
}
DirectX::XMFLOAT3 CharacterPose::WeaponPoint(float distance) const {
  const auto hand=arms[1].end;
  return {hand.x+weaponDirection.x*distance,hand.y+weaponDirection.y*distance,hand.z+weaponDirection.z*distance};
}
DirectX::XMMATRIX CharacterPose::HeadTransform() const {
  using namespace DirectX;
  return XMMatrixTranslation(0,-1.61f,0)*XMMatrixRotationX(headPitch)*XMMatrixRotationY(headYaw)*
    XMMatrixTranslation(0,1.61f-.95f,0)*XMMatrixRotationX(upperPitch)*XMMatrixRotationZ(upperRoll)*
    XMMatrixRotationY(upperYaw)*XMMatrixTranslation(bodyShift,.95f+bodyLift,0);
}
DirectX::XMFLOAT3 CharacterPose::HeadPoint(DirectX::XMFLOAT3 point) const {
  DirectX::XMFLOAT3 result;
  DirectX::XMStoreFloat3(&result,DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&point),HeadTransform()));
  return result;
}
CharacterPose BuildCharacterPose(float swing,float gait,bool shield,animation::Sample dynamics,rpg::WeaponForm form) {
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
  pose.handCast=cast;
  pose.headYaw=animation::Signed(dynamics.face.yaw/.62f)*.62f;
  pose.headPitch=animation::Signed(dynamics.face.pitch/.245f)*.245f;
  pose.bodyLift=gait*gait*.015f*(1-air)+breath*.006f*(1-air)-landing*.14f;
  pose.bodyLift-=dash*.09f+brake*.055f+dashImpact*.04f+animation::Unit(dynamics.dashLoad)*.035f;
  pose.upperPitch=travel*.09f+animation::Signed(dynamics.lean)*.10f+landing*.12f+air*std::max(0.f,rise)*.06f-hit*.12f+sway*.015f*(1-air);
  pose.upperPitch+=dash*.42f-brake*.16f-dashImpact*.14f;
  pose.upperPitch-=cast*.08f;
  // Clip body motion is layered before foot/hand IK. Committed combat keeps
  // priority over locomotion, while contacts still compensate pelvis height.
  const float clipWeight=(1-swing)*(1-burst)*(1-hit);
  pose.bodyLift+=dynamics.motion.hip*clipWeight;
  pose.upperPitch+=dynamics.motion.pitch*clipWeight;
  pose.upperYaw=animation::Signed(dynamics.bank)*.08f;
  pose.upperYaw+=dynamics.motion.yaw*clipWeight;
  const float departure=animation::Unit(dynamics.departure);
  const float contactWeight=dynamics.feetActive?1.f:departure;
  const auto& sourceFeet=dynamics.feetActive?dynamics.feet:dynamics.departureFeet;
  const float support=(animation::Signed(dynamics.support)*(1-departure)+
    animation::Signed(dynamics.departureSupport)*departure)*(1-air)*(1-burst);
  const float bank=animation::Signed(dynamics.bank)*(1-air)*(1-burst);
  pose.bodyShift=support*.032f+bank*.018f;
  pose.upperRoll=-support*.035f-bank*.08f+dynamics.motion.roll*clipWeight;
  // The shoulders counter the planted-leg transfer; descending opens the
  // chest and lowers the hips before the existing impact compression.
  pose.upperYaw+=animation::Signed((sourceFeet[0].z-sourceFeet[1].z)/.6f)*.072f*(1-swing)*contactWeight;
  const float actionWeight=(1-burst)*(1-hit);
  const auto action=dynamics.action*actionWeight;
  pose.upperPitch+=action.pitch; pose.upperYaw+=action.yaw; pose.upperRoll+=action.roll;
  pose.bodyLift+=action.hip*(1-air);
  pose.handCast=std::max(pose.handCast,animation::Unit(action.open));
  pose.upperPitch-=air*animation::SmoothStep(-rise)*.08f;
  pose.bodyLift-=air*animation::SmoothStep(-rise)*.025f;
  if (timed) {
    pose.upperYaw+=stroke.weight*((dynamics.heavyAttack?-.46f:-.36f)+stroke.cut*.82f)*(1-burst);
    pose.upperPitch+=stroke.weight*(-.10f+stroke.cut*.22f)*(1-burst);
    pose.bodyLift-=stroke.coil*(dynamics.heavyAttack?.065f:.035f)*(1-air);
  }
  std::array<XMFLOAT3,2> contacts{};
  if (contactWeight>0) for (int i=0;i<2;++i) {
    const auto& foot=sourceFeet[i];
    const float hip=(i==0?-.17f:.17f)+pose.bodyShift;
    // Use the same bounded contact for standing and departure, including its
    // heading. Losing the heading at release visibly twists a turning boot.
    float x=std::isfinite(foot.x)?foot.x-hip:0,z=std::isfinite(foot.z)?foot.z:0;
    const float distance=std::hypot(x,z),scale=distance>.46f?.46f/distance:1.f;
    x*=scale; z*=scale;
    const float y=std::isfinite(foot.y)?std::clamp(foot.y,.17f,.32f):.17f;
    contacts[i]={hip+x,y,z};
    pose.footYaw[i]=std::isfinite(foot.yaw)?std::remainder(foot.yaw,motion::tau)*contactWeight:0;
  }
  // The push leg sweeps back, then the opposite leg reaches into the catch.
  const float drive=1-2*animation::SmoothStep((animation::Unit(dynamics.dashPhase)-.20f)/.50f);
  std::array<XMFLOAT3,2> targets{};
  std::array<float,2> strides{};
  for (int i=0;i<2;++i) {
    const float side=i==0?-1.f:1.f,stride=side*(gait*(1-burst)+dash*.95f*drive-brake*.65f)*(1-air);
    strides[i]=stride;
    const float positiveRise=std::max(0.f,rise),step=std::max(0.f,stride);
    const float lift=air*(.17f+positiveRise*positiveRise*.15f+(i==0?.045f:0))*
      (1-animation::Unit(dynamics.takeoff)*(i==0?.45f:.9f));
    auto& target=targets[i];
    target={side*(.18f+air*.015f),.17f+step*step*.055f+lift,stride*.22f+air*(i==0?.13f:-.10f)};
    if (contactWeight>0) {
      target={target.x+(contacts[i].x-target.x)*contactWeight,
        target.y+(contacts[i].y-target.y)*contactWeight,target.z+(contacts[i].z-target.z)*contactWeight};
      // Compute both blended targets before solving either leg. Keep the
      // pelvis compensation while releasing a long stride; otherwise the
      // hips jump upward and IK retracts the still-departing ankle.
      const float x=target.x-(side*.17f+pose.bodyShift);
      constexpr float reach=CharacterPose::thigh+CharacterPose::shin-.002f;
      pose.bodyLift=std::min(pose.bodyLift,
        target.y+std::sqrt(std::max(0.f,reach*reach-x*x-target.z*target.z))-.93f);
    }
  }
  for (int i=0;i<2;++i) {
    const float side=i==0?-1.f:1.f;
    pose.legs[i]=Solve({side*.17f+pose.bodyShift,.93f+pose.bodyLift,0},targets[i],{0,0,1},CharacterPose::thigh,CharacterPose::shin);
    const float armStride=strides[i]+(std::clamp(contacts[i].z/.30f,-1.f,1.f)-strides[i])*contactWeight;
    const float attack=i==1?swing:0;
    XMFLOAT3 hand{side*(.40f+air*.035f),1.02f+attack*.52f+air*.12f+landing*.08f+hit*.08f+dash*.12f+brake*.07f,
      .10f-armStride*.13f+attack*.21f+sway*side*.014f-dash*.20f+brake*.12f};
    hand.z+=dynamics.motion.reach*clipWeight;
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
    auto offset=i==0?action.left:action.right;
    if(i==0 && shield && rpg::PhysicalLauncher(form)) offset=offset*.15f;
    hand.x+=offset.x; hand.y+=offset.y; hand.z+=offset.z;
    if(i==1 && rpg::PhysicalLauncher(form)) {
      hand.y+=.20f*animation::Unit(action.aim);hand.z+=.20f*animation::Unit(action.aim);
      if(form==rpg::WeaponForm::Handbow) hand.x-=.12f*animation::Unit(action.aim);
    }
    pose.arms[i]=Solve(pose.UpperPoint({side*.305f,1.43f,0}),pose.UpperPoint(hand),
      pose.UpperDirection({side*.8f,-.2f,-1}),CharacterPose::upperArm,CharacterPose::forearm);
  }
  XMFLOAT3 weapon{.12f,-.76f+swing*1.72f+landing*.25f+air*.20f+dash*.32f+brake*.16f,.63f-swing*.45f};
  if (timed) {
    const float weight=stroke.weight*(1-burst);
    const XMFLOAT3 cut{.22f-stroke.cut*.38f,1.05f-stroke.cut*1.25f,.05f+stroke.cut*1.05f};
    // Blend out to the current movement layer, not a fixed standing blade.
    // Otherwise the first/last timed-attack sample snaps during flight or a
    // landing, and a suppressed attack overrides the dash's weapon clearance.
    const XMFLOAT3 rest{.12f,-.76f+landing*.25f+air*.20f+dash*.32f+brake*.16f,.63f};
    const auto a=XMVector3Normalize(XMLoadFloat3(&rest)),b=XMVector3Normalize(XMLoadFloat3(&cut));
    const float angle=std::acos(std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-.9999f,1.f));
    // Interpolate orientation on the unit sphere: linear direction blending
    // accelerated sharply as the blade crossed from hanging down to raised.
    const auto blended=angle>.0001f?
      (a*std::sin((1-weight)*angle)+b*std::sin(weight*angle))/std::sin(angle):a;
    XMStoreFloat3(&weapon,blended);
  }
  if(rpg::StaffWeapon(form)) weapon={.10f,.99f,.04f+dash*.18f};
  else if(rpg::PhysicalLauncher(form)) {
    const float aim=animation::Unit(action.aim);
    weapon={0,.75f+.25f*aim,.66f*(1-aim)};
  } else if(form==rpg::WeaponForm::Scepter || form==rpg::WeaponForm::Focus || form==rpg::WeaponForm::Baton)
    weapon={.08f,.93f,.32f+dash*.15f};
  const auto direction=pose.UpperDirection(weapon);
  XMStoreFloat3(&pose.weaponDirection,XMVector3Normalize(XMLoadFloat3(&direction)));
  return pose;
}
