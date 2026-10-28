#include "game/CharacterPose.h"
#include "engine/PlayerClips.h"
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
  const float driveSign=dynamics.retreat?-1.f:1.f;
  // The drive/bracing pose takes priority over an earlier attack or cast pose.
  const float burst=std::max(dash,brake);
  const bool carried=dynamics.attackSeconds<0 && dynamics.carriedAttackWeight>0;
  const bool heavy=carried?dynamics.carriedHeavyAttack:dynamics.heavyAttack;
  auto stroke=combat::Sample(carried?dynamics.carriedAttackSeconds:dynamics.attackSeconds,heavy);
  const bool timed=dynamics.attackSeconds>=0 || carried;
  const bool authoredAttack=timed&&(form==rpg::WeaponForm::Blade||form==rpg::WeaponForm::Mace||form==rpg::WeaponForm::Cleaver);
  const auto attackClip=authoredAttack?playerclips::Sample(heavy?playerclips::Clip::Overhead:playerclips::Clip::Slash,
    playerclips::AttackPhase(carried?dynamics.carriedAttackSeconds:dynamics.attackSeconds,combat::StrokeStart(heavy),combat::Contact(heavy),combat::Duration(heavy))):playerclips::Frame{};
  const float authoredAir=air*(1-animation::Unit(dynamics.departure))*(1-burst);
  const auto jumpClip=authoredAir>0?playerclips::Sample(playerclips::Clip::Jump,.23f+.53f*animation::Unit((1-rise)*.5f)):playerclips::Frame{};
  const bool authoredLanding=dynamics.landingSeconds>=0&&dynamics.landingSeconds<.42f&&landing>0;
  const auto landClip=authoredLanding?playerclips::Sample(playerclips::Clip::Land,dynamics.landingSeconds/.42f):playerclips::Frame{};
  if(carried) {stroke.weight*=animation::Unit(dynamics.carriedAttackWeight);stroke.coil*=animation::Unit(dynamics.carriedAttackWeight);}
  if (timed) swing=stroke.weight;
  swing*=1-burst;
  const float cast=animation::Unit(dynamics.cast)*(1-swing)*(1-hit)*(1-burst);
  CharacterPose pose;
  pose.handCast=cast;
  pose.headYaw=animation::Signed(dynamics.face.yaw/.62f)*.62f;
  pose.headPitch=animation::Signed(dynamics.face.pitch/.245f)*.245f;
  pose.bodyLift=gait*gait*.015f*(1-air)+breath*.006f*(1-air)-landing*.14f;
  pose.bodyLift+=jumpClip.hip*authoredAir;
  if(authoredLanding)pose.bodyLift+=(landClip.hip+.14f)*landing;
  pose.bodyLift-=dash*.09f+brake*.055f+dashImpact*.04f+animation::Unit(dynamics.dashLoad)*.035f;
  pose.upperPitch=travel*.09f+animation::Signed(dynamics.lean)*.10f+landing*.12f+air*std::max(0.f,rise)*.06f-hit*.12f+sway*.015f*(1-air);
  pose.upperPitch+=jumpClip.torso.x*authoredAir;
  pose.upperPitch+=(dash*.42f-brake*.16f-dashImpact*.14f)*driveSign;
  pose.upperPitch-=cast*.08f;
  // Clip body motion is layered before foot/hand IK. Committed combat keeps
  // priority over locomotion, while contacts still compensate pelvis height.
  const float clipWeight=(1-swing)*(1-burst)*(1-hit);
  const float authoredWeight=dynamics.authoredGait?clipWeight*(1-air)*(1-cast):0;
  float gaitAmount=0;for(float w:dynamics.gaitWeights)gaitAmount+=animation::Unit(w);
  gaitAmount=animation::Unit(gaitAmount);
  const auto& gaitClip=dynamics.authoredMotion;
  pose.bodyLift+=gaitClip.hip*authoredWeight;
  pose.upperPitch+=gaitClip.torso.x*authoredWeight;
  pose.bodyLift+=dynamics.motion.hip*clipWeight;
  pose.upperPitch+=dynamics.motion.pitch*clipWeight;
  pose.upperYaw=animation::Signed(dynamics.bank)*.08f;
  pose.upperYaw+=dynamics.motion.yaw*clipWeight;
  pose.upperYaw+=gaitClip.torso.y*authoredWeight;
  const float carrying=(1-air)*(1-burst)*(1-swing)*(1-hit)*(1-animation::Unit(dynamics.action.open));
  if(rpg::PhysicalLauncher(form)) {pose.upperYaw-=.045f*carrying;pose.upperPitch+=.02f*carrying;}
  else if(rpg::StaffWeapon(form)) {pose.upperYaw+=.025f*carrying;pose.upperPitch-=.018f*carrying;}
  else if(form==rpg::WeaponForm::Cleaver) {pose.bodyLift-=.012f*carrying;pose.upperPitch+=.025f*carrying;}
  const float departure=animation::Unit(dynamics.departure);
  const float contactWeight=dynamics.feetActive?1.f:departure;
  const auto& sourceFeet=dynamics.feetActive?dynamics.feet:dynamics.departureFeet;
  const float support=(animation::Signed(dynamics.support)*(1-departure)+
    animation::Signed(dynamics.departureSupport)*departure)*(1-air)*(1-burst);
  const float bank=animation::Signed(dynamics.bank)*(1-air)*(1-burst);
  pose.bodyShift=support*.032f+bank*.018f;
  // Load the rear side during anticipation and transfer weight through contact.
  // Feet remain solved against their planted targets below.
  if(timed)pose.bodyShift+=stroke.weight*(stroke.cut*2-1)*(heavy?.035f:.022f)*(1-air)*(1-burst);
  pose.upperRoll=-support*.035f-bank*.08f+dynamics.motion.roll*clipWeight;
  pose.upperRoll+=gaitClip.torso.z*authoredWeight;
  // Lateral contacts roll away from the blow and twist the shoulders slightly.
  // This remains above the pelvis; the existing limb IK retains foot support.
  pose.upperRoll-=animation::Signed(dynamics.hitSide)*.10f;
  pose.upperYaw+=animation::Signed(dynamics.hitSide)*.07f;
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
    if(authoredAttack) {
      const float w=stroke.weight*(1-burst);
      pose.upperYaw+=attackClip.torso.y*w;pose.upperPitch+=attackClip.torso.x*w;pose.upperRoll+=attackClip.torso.z*w;
      // Source shoulder twist is small; retain the player's planted weight
      // transfer as an additive layer around the imported hand trajectory.
      pose.upperYaw+=w*((heavy?-.46f:-.36f)+stroke.cut*.82f);
      pose.bodyLift+=attackClip.hip*w*(1-air);
    } else {
      pose.upperYaw+=stroke.weight*((heavy?-.46f:-.36f)+stroke.cut*.82f)*(1-burst);
      pose.upperPitch+=stroke.weight*(-.10f+stroke.cut*.22f)*(1-burst);
      pose.bodyLift-=stroke.coil*(heavy?.065f:.035f)*(1-air);
    }
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
    const float side=i==0?-1.f:1.f,stride=side*(gait*(1-burst)+(dash*.95f*drive-brake*.65f)*driveSign)*(1-air);
    strides[i]=stride;
    const float positiveRise=std::max(0.f,rise),step=std::max(0.f,stride);
    const float descent=animation::SmoothStep(-rise);
    const float lift=air*(.17f*(1-.85f*descent)+positiveRise*positiveRise*.15f+(i==0?.045f:0)*(1-descent))*
      (1-animation::Unit(dynamics.takeoff)*(i==0?.45f:.9f));
    auto& target=targets[i];
    target={side*(.18f+air*.015f),.17f+step*step*.055f+lift,stride*.22f+air*(i==0?.13f:-.10f)*(1-descent)};
    if(authoredAir>0) {
      const auto delta=jumpClip.feet[i];const XMFLOAT3 authored{side*.18f+delta.x,.17f+delta.y,delta.z};
      target={target.x+(authored.x-target.x)*authoredAir,target.y+(authored.y-target.y)*authoredAir,target.z+(authored.z-target.z)*authoredAir};
    }
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
    hand.x+=gaitClip.hands[i].x*authoredWeight;
    hand.y+=gaitClip.hands[i].y*authoredWeight;
    hand.z+=(gaitClip.hands[i].z+armStride*.13f*gaitAmount)*authoredWeight;
    // Reach forward on descent to brace for impact; rising arms trail the push.
    hand.z+=air*(.18f*animation::SmoothStep(-rise)-.10f*std::max(0.f,rise))*(1-swing);
    if(authoredAir>0) {
      const auto delta=jumpClip.hands[i];const XMFLOAT3 authored{side*.40f+delta.x,1.02f+delta.y,.10f+delta.z};
      const float w=authoredAir*(1-swing)*(1-cast);
      hand={hand.x+(authored.x-hand.x)*w,hand.y+(authored.y-hand.y)*w,hand.z+(authored.z-hand.z)*w};
    }
    if(authoredLanding) {hand.y+=landClip.hands[i].y*landing;hand.z+=landClip.hands[i].z*landing;}
    if (i==0 && shield) hand={-.39f,1.13f+air*.08f+landing*.08f,.23f-armStride*.04f+dash*.09f};
    if(i==0 && shield) {
      hand.y+=gaitClip.hands[i].y*authoredWeight*.25f;
      hand.z+=gaitClip.hands[i].z*authoredWeight*.25f;
    }
    if (timed) {
      const float cut=stroke.cut,weight=stroke.weight*(1-burst);
      // Coil beside the shoulder, cut across the body, then return to guard.
      XMFLOAT3 attackHand=i==1?
        XMFLOAT3{.46f-cut*.34f,(heavy?1.66f:1.48f)-cut*.38f,-.13f+cut*.59f}:
        XMFLOAT3{-.34f,1.25f,.23f-cut*.10f};
      if(authoredAttack) {const auto delta=attackClip.hands[i];attackHand={side*.40f+delta.x*.65f,1.02f+delta.y*.65f,.10f+delta.z*.65f};}
      if(i==1 && form==rpg::WeaponForm::Mace) {
        // A compact raised hammer path, distinct from the sword's cross-cut.
        attackHand.x+=.035f*cut;attackHand.y+=.055f*cut*(1-cut);
        attackHand.z-=.025f*cut;
      } else if(i==1 && form==rpg::WeaponForm::Cleaver) {
        // Forward-weighted blade follows a wider, lower diagonal.
        attackHand.x+=.065f*(1-cut)-.035f*cut;
        attackHand.y-=.035f*cut;attackHand.z+=.04f*cut;
      }
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
    XMFLOAT3 cut{.22f-stroke.cut*.38f,1.05f-stroke.cut*1.25f,.05f+stroke.cut*1.05f};
    if(authoredAttack)cut=attackClip.weapon;
    if(form==rpg::WeaponForm::Mace) {const float x=cut.x;cut.x=x*std::cos(.12f)-cut.y*std::sin(.12f);cut.y=x*std::sin(.12f)+cut.y*std::cos(.12f);cut.z-=.025f*stroke.cut;}
    else if(form==rpg::WeaponForm::Cleaver) {cut.x+=.10f*(1-2*stroke.cut);cut.z+=.06f*stroke.cut;}
    // Blend out to the current movement layer, not a fixed standing blade.
    // Otherwise the first/last timed-attack sample snaps during flight or a
    // landing, and a suppressed attack overrides the dash's weapon clearance.
    const XMFLOAT3 rest{.12f,-.76f+landing*.25f+air*.20f+dash*.32f+brake*.16f,.63f};
    // Keep the retargeted blade in the forward weapon hemisphere. This avoids
    // a shortest-arc reversal behind the smaller player shoulder/shield rig.
    if(authoredAttack)cut.z=std::max(.65f,cut.z);
    const auto a=XMVector3Normalize(XMLoadFloat3(&rest)),b=XMVector3Normalize(XMLoadFloat3(&cut));
    const float blend=weight;
    const float angle=std::acos(std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-.9999f,1.f));
    // Interpolate orientation on the unit sphere: linear direction blending
    // accelerated sharply as the blade crossed from hanging down to raised.
    const auto blended=angle>.0001f?
      (a*std::sin((1-blend)*angle)+b*std::sin(blend*angle))/std::sin(angle):a;
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
  // Retargeted wrist heights differ from the source actor. Preserve a small
  // floor clearance for the longest held blade without moving its grip.
  const float minimumY=std::clamp((.085f-pose.arms[1].end.y)/.96f,-1.f,1.f);
  if(pose.weaponDirection.y<minimumY) {
    const float horizontal=std::hypot(pose.weaponDirection.x,pose.weaponDirection.z);
    const float scale=std::sqrt(1-minimumY*minimumY)/std::max(horizontal,.00001f);
    pose.weaponDirection.x*=scale;pose.weaponDirection.z*=scale;pose.weaponDirection.y=minimumY;
  }
  return pose;
}
