#include "game/CharacterPose.h"
#include "engine/PlayerClips.h"
#include "game/CombatWeight.h"
#include "game/WeaponGrip.h"
#include <algorithm>
#include <cmath>

namespace {
using namespace DirectX;
LimbPose Solve(XMFLOAT3 root,XMFLOAT3 target,XMFLOAT3 bend,float upper,float lower,float margin=.001f) {
  const auto origin=XMLoadFloat3(&root),delta=XMLoadFloat3(&target)-origin;
  const float requested=XMVectorGetX(XMVector3Length(delta));
  const auto axis=requested>.0001f?delta/requested:XMVectorSet(0,-1,0,0);
  const float distance=std::clamp(requested,std::abs(upper-lower)+.001f,upper+lower-margin);
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
LimbPose SolveArm(XMFLOAT3 root,XMFLOAT3 target,XMFLOAT3 authoredBend) {
  const auto delta=XMLoadFloat3(&target)-XMLoadFloat3(&root);
  const auto axis=XMVector3Normalize(delta);
  // Transport from an outward/downward reference. Its opposite lies inward
  // and above the shoulder, outside the authored reach. Transporting from
  // straight down put the singularity directly in the overhead sword arc.
  const auto reference=XMVectorSet(root.x>=0?.70710678f:-.70710678f,-.70710678f,0,0);
  auto q=XMVectorSetW(XMVector3Cross(reference,axis),1+XMVectorGetX(XMVector3Dot(reference,axis)));
  const auto base=XMVectorGetX(XMVector4LengthSq(q))>.000001f?
    XMVector3Rotate(XMVectorSet(0,0,-1,0),XMQuaternionNormalize(q)):XMVectorSet(0,0,-1,0);
  auto desired=XMLoadFloat3(&authoredBend);desired-=axis*XMVector3Dot(desired,axis);
  const float length=XMVectorGetX(XMVector3Length(desired));
  auto bend=base;
  if(length>.00001f) {
    desired/=length;
    const float agreement=std::max(0.f,XMVectorGetX(XMVector3Dot(base,desired)));
    // Retain the authored swivel within the anatomical hemisphere. Fade it
    // out continuously when grip retargeting would invert the bend plane.
    bend=XMVector3Normalize(XMVectorLerp(base,desired,agreement*.85f));
  }
  XMFLOAT3 pole;XMStoreFloat3(&pole,bend);
  return Solve(root,target,pole,CharacterPose::upperArm,CharacterPose::forearm,.006f);
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
  DirectX::XMStoreFloat3(&p,DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&p),UpperTransform()));return p;
}
DirectX::XMMATRIX CharacterPose::SpineTransform(unsigned segment) const {
  using namespace DirectX;
  const auto rotation=[](XMFLOAT3 r){return XMMatrixRotationX(r.x)*XMMatrixRotationZ(r.z)*XMMatrixRotationY(r.y);};
  const auto chest=rotation({upperPitch,upperYaw,upperRoll});
  if(spineWeight<=0)return XMMatrixTranslation(0,-.95f,0)*chest*XMMatrixTranslation(bodyShift,.95f+bodyLift,bodyForward);
  const auto lower=rotation(spineRotation[0]),upper=rotation(spineRotation[1]);
  const auto a=PelvisPoint({0,.95f,0});
  auto center=XMLoadFloat3(&a);
  if(segment>=1)center+=XMVector3TransformNormal(XMVectorSet(0,.15f,0,0),lower);
  if(segment>=2)center+=XMVector3TransformNormal(XMVectorSet(0,.17f,0,0),upper);
  const float y=segment==0?.95f:segment==1?1.10f:1.27f;
  const auto legacyCenter=XMVector3TransformNormal(XMVectorSet(0,y-.95f,0,0),chest)+XMVectorSet(bodyShift,.95f+bodyLift,bodyForward,0);
  center=XMVectorLerp(legacyCenter,center,spineWeight);
  const auto desired=segment==0?lower:segment==1?upper:chest;
  const auto q=XMQuaternionSlerp(XMQuaternionRotationMatrix(chest),XMQuaternionRotationMatrix(desired),spineWeight);
  auto matrix=XMMatrixTranslation(0,-y,0)*XMMatrixRotationQuaternion(q);
  matrix.r[3]+=XMVectorSetW(center,0);return matrix;
}
DirectX::XMMATRIX CharacterPose::UpperTransform() const {return SpineTransform(2);}
DirectX::XMMATRIX CharacterPose::PelvisTransform() const {
  using namespace DirectX;
  return XMMatrixTranslation(0,-.93f,0)*XMMatrixRotationX(pelvisRotation.x)*
    XMMatrixRotationZ(pelvisRotation.z)*XMMatrixRotationY(pelvisRotation.y)*
    XMMatrixTranslation(bodyShift,.93f+bodyLift,bodyForward);
}
DirectX::XMFLOAT3 CharacterPose::PelvisPoint(DirectX::XMFLOAT3 p) const {
  DirectX::XMStoreFloat3(&p,DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&p),PelvisTransform()));return p;
}
DirectX::XMFLOAT3 CharacterPose::WeaponPoint(float distance) const {
  const auto hand=hasGripSocket?weaponGrip:arms[1].end;
  return {hand.x+weaponDirection.x*distance,hand.y+weaponDirection.y*distance,hand.z+weaponDirection.z*distance};
}
DirectX::XMFLOAT3 CharacterPose::FootPoint(unsigned side,DirectX::XMFLOAT3 local) const {
  using namespace DirectX;
  const auto& ankle=legs[side].end;
  XMStoreFloat3(&local,XMVector3TransformCoord(XMLoadFloat3(&local),XMMatrixRotationX(footPitch[side])*
    XMMatrixRotationY(footYaw[side])*XMMatrixTranslation(ankle.x,ankle.y,ankle.z)));return local;
}
DirectX::XMMATRIX CharacterPose::HeadTransform() const {
  using namespace DirectX;
  return XMMatrixTranslation(0,-1.61f,0)*XMMatrixRotationX(headPitch)*XMMatrixRotationY(headYaw)*
    XMMatrixTranslation(0,1.61f,0)*UpperTransform();
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
  const bool newStrike=dynamics.authoredGait&&dynamics.authoredStrikeWeight>0;
  const bool bow=dynamics.authoredGait&&dynamics.authoredEquipment==playerclips::Equipment::Bow;
  if(bow)shield=false;
  const bool heavy=newStrike?dynamics.authoredStrikeHeavy:carried?dynamics.carriedHeavyAttack:dynamics.heavyAttack;
  auto stroke=combat::Sample(carried?dynamics.carriedAttackSeconds:dynamics.attackSeconds,heavy);
  const bool timed=dynamics.attackSeconds>=0 || carried || newStrike;
  const bool authoredAttack=timed&&(newStrike||form==rpg::WeaponForm::Blade||form==rpg::WeaponForm::Mace||form==rpg::WeaponForm::Cleaver);
  const auto selectedClip=heavy?playerclips::Clip::LegacyOverhead:playerclips::Clip::LegacySlash;
  const auto attackClip=newStrike?dynamics.authoredStrike:authoredAttack?playerclips::Sample(selectedClip,
    playerclips::AttackPhase(selectedClip,carried?dynamics.carriedAttackSeconds:dynamics.attackSeconds,combat::StrokeStart(heavy),combat::Contact(heavy),combat::Duration(heavy))):playerclips::Frame{};
  const float authoredAir=air*(1-animation::Unit(dynamics.departure))*(1-burst);
  const float flight=dynamics.flightPhase>=0?animation::Unit(dynamics.flightPhase):animation::Unit((1-rise)*.5f);
  auto jumpClip=authoredAir>0?playerclips::Sample(dynamics.authoredGait?playerclips::Clip::Jump:playerclips::Clip::LegacyJump,
    dynamics.authoredGait?playerclips::FlightPhase(flight):.23f+.53f*flight):playerclips::Frame{};
  if(dynamics.authoredGait&&authoredAir>0)jumpClip=playerclips::Blend(jumpClip,
    playerclips::Sample(playerclips::Clip::Fall,.5f),animation::Unit(dynamics.extendedFall));
  const bool authoredLanding=dynamics.landingSeconds>=0&&dynamics.landingSeconds<.42f&&landing>0;
  const auto& landingTrack=playerclips::Collection()[unsigned(playerclips::Clip::Land)];
  const auto landClip=authoredLanding?playerclips::Sample(dynamics.authoredGait?playerclips::Clip::Land:playerclips::Clip::LegacyLand,
    dynamics.authoredGait?landingTrack.contact+(1-landingTrack.contact)*dynamics.landingSeconds/.42f:dynamics.landingSeconds/.42f):playerclips::Frame{};
  const bool equippedFlight=dynamics.authoredGait&&dynamics.authoredEquipment!=playerclips::Equipment::None;
  const auto equippedJump=dynamics.authoredEquipment==playerclips::Equipment::Staff?playerclips::Clip::StaffJump:
    dynamics.authoredEquipment==playerclips::Equipment::Bow?playerclips::Clip::BowJump:playerclips::Clip::SwordJump;
  const bool trackedArms=dynamics.authoredGait&&dynamics.flightArms>=0;
  const float armAir=trackedArms?animation::Unit(dynamics.flightArms)*(1-burst):authoredAir;
  const bool armLanding=trackedArms?dynamics.armLandingSeconds>=0&&dynamics.armLandingSeconds<.42f:authoredLanding;
  const float armAge=trackedArms?dynamics.armLandingSeconds:dynamics.landingSeconds;
  const float armLandingWeight=trackedArms?armAir:landing;
  const auto flightArms=trackedArms?animation::AuthoredArmFrame(dynamics):armAir>0&&dynamics.authoredGait?
    playerclips::Sample(equippedFlight?equippedJump:playerclips::Clip::Jump,playerclips::FlightPhase(flight)):jumpClip;
  const auto& jumpTrack=playerclips::Collection()[unsigned(playerclips::Clip::Jump)];
  const auto landingArms=trackedArms?flightArms:armLanding&&dynamics.authoredGait?playerclips::Sample(equippedFlight?equippedJump:playerclips::Clip::Land,
    equippedFlight?jumpTrack.landing+(1-jumpTrack.landing)*armAge/.42f:
    landingTrack.contact+(1-landingTrack.contact)*armAge/.42f):landClip;
  if(carried) {stroke.weight*=animation::Unit(dynamics.carriedAttackWeight);stroke.coil*=animation::Unit(dynamics.carriedAttackWeight);}
  if(newStrike) {stroke.weight=animation::Unit(dynamics.authoredStrikeWeight);stroke.cut=animation::SmoothStep(dynamics.authoredStrikePhase);}
  if (timed) swing=stroke.weight;
  swing*=1-burst;
  const float cast=animation::Unit(dynamics.cast)*(1-swing)*(1-hit)*(1-burst);
  CharacterPose pose;
  // A relaxed carrying hand holds the bow alone. The arrow is nocked during
  // the actual draw and stays absent through recovery and locomotion.
  if(bow)pose.bowArrow=0;
  if(bow&&newStrike) {
    const auto& track=playerclips::Collection()[unsigned(heavy?playerclips::Clip::BowPowerRelease:playerclips::Clip::BowRelease)];
    const float p=dynamics.authoredStrikePhase;
    pose.bowDraw=stroke.weight*animation::SmoothStep(p/track.contact)*(1-animation::SmoothStep((p-track.contact)/.18f));
    pose.bowArrow=p<=track.contact?stroke.weight*animation::SmoothStep(p/.035f):0;
  }
  pose.handCast=cast;
  // A running hand is loosely closed, rather than the flat, splayed palm of
  // a casting gesture. Occupied grips remain controlled by their equipment.
  pose.freeHandCurl=.32f+.28f*animation::Unit(dynamics.runBlend)*travel+.10f*air;
  pose.headYaw=animation::Signed(dynamics.face.yaw/.62f)*.62f;
  pose.headPitch=animation::Signed(dynamics.face.pitch/.245f)*.245f;
  pose.bodyLift=gait*gait*.015f*(1-air)+breath*.006f*(1-air)-landing*.14f;
  pose.bodyLift+=jumpClip.hip*authoredAir;
  if(authoredLanding)pose.bodyLift+=(landClip.hip+.14f)*landing;
  pose.bodyLift-=dash*.09f+brake*.055f+dashImpact*.04f+animation::Unit(dynamics.dashLoad)*.035f;
  pose.upperPitch=travel*.09f+animation::Signed(dynamics.lean)*.10f+landing*.12f+air*std::max(0.f,rise)*.06f-animation::ForwardRecoil(dynamics)*.12f+sway*.015f*(1-air);
  pose.upperPitch+=jumpClip.torso.x*authoredAir;
  pose.upperPitch+=(dash*.42f-brake*.16f-dashImpact*.14f)*driveSign;
  pose.upperPitch-=cast*.08f;
  // Clip body motion is layered before foot/hand IK. Committed combat keeps
  // priority over locomotion, while contacts still compensate pelvis height.
  const float clipWeight=(1-swing)*(1-burst)*(1-hit);
  const float authoredBlend=dynamics.authoredGait?animation::Unit(dynamics.authoredBlend):0;
  pose.upperPitch-=travel*.09f*authoredBlend;
  const auto& gesture=dynamics.action;
  const float gestureWeight=animation::SmoothStep(std::max(std::hypot(gesture.left.x,gesture.left.y,gesture.left.z),
    std::hypot(gesture.right.x,gesture.right.y,gesture.right.z))/.25f);
  const float castLayer=std::max(cast,std::max(gestureWeight,animation::Unit(gesture.open))*(1-burst)*(1-hit));
  const float authoredWeight=authoredBlend*clipWeight*(1-air)*(1-castLayer);
  float gaitAmount=0;for(float w:dynamics.gaitWeights)gaitAmount+=animation::Unit(w);
  gaitAmount=animation::Unit(gaitAmount);
  auto gaitClip=dynamics.authoredMotion;
  const auto guardClip=playerclips::Sample(dynamics.authoredEquipment==playerclips::Equipment::Staff?playerclips::Clip::StaffGuard:
    dynamics.authoredEquipment==playerclips::Equipment::Sword?playerclips::Clip::SwordGuard:bow?playerclips::Clip::BowGuard:playerclips::Clip::Idle,dynamics.authoredClock/4);
  const auto carryClip=playerclips::Sample(dynamics.authoredEquipment==playerclips::Equipment::Staff?playerclips::Clip::StaffCarry:
    dynamics.authoredEquipment==playerclips::Equipment::Sword?playerclips::Clip::SwordCarry:bow?playerclips::Clip::BowCarry:playerclips::Clip::Idle,dynamics.authoredClock/4);
  float ready=animation::Unit(dynamics.authoredReady);
  if(newStrike&&dynamics.authoredEquipment==playerclips::Equipment::Sword&&dynamics.authoredEntryReady>=0) {
    // The wind-up departs from the actual incoming carry. Raising its base
    // toward guard at the same time doubled the first attack's acceleration.
    // Bring that base to combat guard only through the recovery after contact.
    const float contact=playerclips::Collection()[unsigned(heavy?playerclips::Clip::Overhead:playerclips::Clip::Slash)].contact;
    const float recovery=animation::SmoothStep((dynamics.authoredStrikePhase-contact)/(1-contact));
    const float entry=animation::Unit(dynamics.authoredEntryReady);
    ready=entry+(ready-entry)*recovery;
  }
  auto idleClip=playerclips::Blend(carryClip,guardClip,ready);
  const auto blendDirection=[](XMFLOAT3 from,XMFLOAT3 to,float amount) {
    const auto a=XMVector3Normalize(XMLoadFloat3(&from)),b=XMVector3Normalize(XMLoadFloat3(&to));
    const float angle=std::acos(std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-.9999f,1.f));
    XMFLOAT3 result;XMStoreFloat3(&result,angle>.0001f?
      (a*std::sin((1-amount)*angle)+b*std::sin(amount*angle))/std::sin(angle):a);
    return result;
  };
  idleClip.weapon=blendDirection(carryClip.weapon,guardClip.weapon,ready);
  // Retain the authored directional swing while bringing the carry towards
  // guard. The blend is presentation state, never a second combat timer.
  const float readyGait=ready*gaitAmount;
  for(unsigned i=0;i<2;++i) {
    gaitClip.hands[i].x+=(guardClip.hands[i].x-carryClip.hands[i].x)*readyGait;
    gaitClip.hands[i].y+=(guardClip.hands[i].y-carryClip.hands[i].y)*readyGait;
    gaitClip.hands[i].z+=(guardClip.hands[i].z-carryClip.hands[i].z)*readyGait;
  }
  if(gaitAmount>.00001f) {
    const auto direction=blendDirection(gaitClip.weapon,guardClip.weapon,ready);
    gaitClip.weapon={direction.x*gaitAmount,direction.y*gaitAmount,direction.z*gaitAmount};
  }
  const float idleWeight=authoredBlend*(1-gaitAmount)*clipWeight*(1-air)*(1-castLayer);
  // Hands are interpolated to the strike later; fading their carry here too
  // would pull the wrists towards the unarmed rest pose halfway through a cut.
  // Flight interpolates from this complete carry below. Fading it here as
  // well applies (1-air) twice and pulls the wrists through the rest pose.
  const float handAuthoredWeight=authoredBlend*(1-burst)*(1-hit)*(1-castLayer);
  const float handIdleWeight=handAuthoredWeight*(1-gaitAmount);
  pose.bodyLift+=gaitClip.hip*authoredWeight+idleClip.hip*idleWeight;
  pose.upperPitch+=gaitClip.torso.x*authoredWeight+idleClip.torso.x*idleWeight;
  pose.bodyLift+=dynamics.motion.hip*clipWeight;
  pose.upperPitch+=dynamics.motion.pitch*clipWeight;
  pose.upperYaw=animation::Signed(dynamics.bank)*.08f;
  pose.upperYaw+=dynamics.motion.yaw*clipWeight;
  pose.upperYaw+=gaitClip.torso.y*authoredWeight+idleClip.torso.y*idleWeight;
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
  if(timed&&!newStrike)pose.bodyShift+=stroke.weight*(stroke.cut*2-1)*(heavy?.035f:.022f)*(1-air)*(1-burst);
  pose.upperRoll=-support*.035f-bank*.08f+dynamics.motion.roll*clipWeight;
  pose.upperRoll+=gaitClip.torso.z*authoredWeight+idleClip.torso.z*idleWeight;
  // Lateral contacts roll away from the blow and twist the shoulders slightly.
  // This remains above the pelvis; the existing limb IK retains foot support.
  pose.upperRoll-=animation::Signed(dynamics.hitSide)*.10f;
  pose.upperYaw+=animation::Signed(dynamics.hitSide)*.07f;
  // The shoulders counter the planted-leg transfer; descending opens the
  // chest and lowers the hips before the existing impact compression.
  pose.upperYaw+=animation::Signed((sourceFeet[0].z-sourceFeet[1].z)/.6f)*.072f*(1-swing)*contactWeight*(1-authoredBlend);
  const float actionWeight=(1-burst)*(1-hit);
  auto action=dynamics.action*(actionWeight*(newStrike?1-stroke.weight:1));
  if(newStrike && !shield && (rpg::StaffWeapon(form)||form==rpg::WeaponForm::Scepter)) {
    // Authored attacks own the weapon hand, but an invocation still needs
    // its free-hand path. Suppressing every additive channel at full stroke
    // weight erased the spell gesture and pinned the supporting staff hand.
    const float bodyWeight=actionWeight*(1-.5f*stroke.weight);
    // The clip already provides the broad arm travel; retain a half-strength
    // additive path at its peak instead of stacking two full-sized reaches.
    action.left=dynamics.action.left*bodyWeight;
    action.open=dynamics.action.open*actionWeight;
    action.pitch=dynamics.action.pitch*bodyWeight;action.yaw=dynamics.action.yaw*bodyWeight;
    action.roll=dynamics.action.roll*bodyWeight;action.hip=dynamics.action.hip*bodyWeight;
  }
  pose.upperPitch+=action.pitch; pose.upperYaw+=action.yaw; pose.upperRoll+=action.roll;
  pose.bodyLift+=action.hip*(1-air);
  pose.handCast=std::max(pose.handCast,animation::Unit(action.open));
  pose.upperPitch-=air*animation::SmoothStep(-rise)*.08f;
  pose.bodyLift-=air*animation::SmoothStep(-rise)*.025f;
  if (timed) {
    if(authoredAttack) {
      const float w=stroke.weight*(1-burst);
      pose.upperYaw+=attackClip.torso.y*w;pose.upperPitch+=attackClip.torso.x*w;pose.upperRoll+=attackClip.torso.z*w;
      if(!newStrike)pose.upperYaw+=w*((heavy?-.46f:-.36f)+stroke.cut*.82f);
      pose.bodyLift+=attackClip.hip*w*(1-air);
    } else {
      pose.upperYaw+=stroke.weight*((heavy?-.46f:-.36f)+stroke.cut*.82f)*(1-burst);
      pose.upperPitch+=stroke.weight*(-.10f+stroke.cut*.22f)*(1-burst);
      pose.bodyLift-=stroke.coil*(heavy?.065f:.035f)*(1-air);
    }
  }
  // Full-body source channels share the same layer weights as the chest.
  // Feet retain world anchors; the pelvis moves over them and IK compensates.
  XMFLOAT3 sourceChest{};
  const auto addBody=[&](const playerclips::Frame& f,float weight) {
    pose.pelvisRotation.x+=f.pelvis.x*weight;pose.pelvisRotation.y+=f.pelvis.y*weight;pose.pelvisRotation.z+=f.pelvis.z*weight;
    pose.bodyShift+=f.shift.x*weight;pose.bodyForward+=f.shift.y*weight;
    const float w=animation::Unit(f.articulated)*weight;pose.spineWeight+=w;
    if(f.articulated>0){sourceChest.x+=f.torso.x*weight;sourceChest.y+=f.torso.y*weight;sourceChest.z+=f.torso.z*weight;}
    for(unsigned i=0;i<2;++i){
      XMStoreFloat3(&pose.spineRotation[i],XMLoadFloat3(&pose.spineRotation[i])+XMLoadFloat3(&f.spine[i])*weight);
      XMStoreFloat3(&pose.shoulderOffset[i],XMLoadFloat3(&pose.shoulderOffset[i])+XMLoadFloat3(&f.shoulders[i])*weight);
    }
  };
  addBody(gaitClip,authoredWeight);addBody(idleClip,idleWeight);
  addBody(jumpClip,authoredAir*(1-swing));
  if(authoredLanding)addBody(landClip,landing*(1-swing));
  if(newStrike)addBody(attackClip,stroke.weight*(1-air)*(1-burst));
  if(newStrike && (dynamics.authoredEquipment==playerclips::Equipment::Sword || bow)) {
    const auto clip=bow?(heavy?playerclips::Clip::BowPowerRelease:playerclips::Clip::BowRelease):(heavy?playerclips::Clip::Overhead:playerclips::Clip::Slash);
    const float contact=playerclips::Collection()[unsigned(clip)].contact;
    const auto motion=dynamics.authoredMechanicsTracked?dynamics.authoredMechanics:
      animation::AttackWeight(bow,heavy,dynamics.authoredStrikePhase,contact);
    const float weight=stroke.weight*(1-air)*(1-burst)*(1-hit);
    pose.upperPitch+=motion.pitch*weight;pose.upperYaw+=motion.yaw*weight;pose.upperRoll+=motion.roll*weight;
    pose.bodyLift+=motion.lift*weight;pose.bodyShift+=motion.side*weight;pose.bodyForward+=motion.forward*weight;
    pose.pelvisRotation.x+=motion.pelvisPitch*weight;pose.pelvisRotation.y+=motion.pelvisYaw*weight;pose.pelvisRotation.z+=motion.pelvisRoll*weight;
  }
  std::array<XMFLOAT3,2> elbowPoles{};
  if(newStrike){
    const auto motion=dynamics.authoredAccent*(stroke.weight*(1-air)*(1-burst)*(1-hit));
    pose.upperPitch+=motion.pitch;pose.upperYaw+=motion.yaw;pose.upperRoll+=motion.roll;
    pose.bodyLift+=motion.lift;pose.bodyShift+=motion.side;pose.bodyForward+=motion.forward;
    pose.pelvisRotation.x+=motion.pelvisPitch;pose.pelvisRotation.y+=motion.pelvisYaw;pose.pelvisRotation.z+=motion.pelvisRoll;
  }
  pose.spineWeight=animation::Unit(pose.spineWeight);
  const auto finalChest=XMVectorSet(pose.upperPitch,pose.upperYaw,pose.upperRoll,0);
  const auto residual=finalChest*pose.spineWeight-XMLoadFloat3(&sourceChest);
  for(unsigned i=0;i<2;++i)XMStoreFloat3(&pose.spineRotation[i],XMLoadFloat3(&pose.spineRotation[i])+
    finalChest*(1-pose.spineWeight)+residual*(i?.60f:.25f));
  for(int i=0;i<2;++i) {
    const XMFLOAT3 fallback{i==0?-.8f:.8f,-.2f,-1};
    auto pole=XMLoadFloat3(&fallback);
    const auto addPole=[&](const playerclips::Frame& f,float w) {
      const auto p=XMLoadFloat3(&f.elbows[i]);
      if(XMVectorGetX(XMVector3LengthSq(p))>.0001f)pole=XMVectorLerp(pole,XMVector3Normalize(p),std::clamp(w,0.f,1.f));
    };
    addPole(idleClip,authoredBlend*(1-gaitAmount));addPole(gaitClip,authoredBlend*gaitAmount);
    if(armLanding)addPole(landingArms,armLandingWeight);else addPole(flightArms,armAir);
    if(newStrike)addPole(attackClip,stroke.weight*(1-burst)*(1-hit));
    XMStoreFloat3(&elbowPoles[i],XMVector3Normalize(pole));
    elbowPoles[i]=pose.UpperDirection(elbowPoles[i]);
  }
  std::array<XMFLOAT3,2> contacts{};
  if (contactWeight>0) for (int i=0;i<2;++i) {
    const auto& foot=sourceFeet[i];
    const float hip=pose.PelvisPoint({i==0?-.17f:.17f,.93f,0}).x;
    // Use the same bounded contact for standing and departure, including its
    // heading. Losing the heading at release visibly twists a turning boot.
    float x=std::isfinite(foot.x)?foot.x-hip:0,z=std::isfinite(foot.z)?foot.z:0;
    const float distance=std::hypot(x,z),scale=distance>.58f?.58f/distance:1.f;
    x*=scale; z*=scale;
    const float y=std::isfinite(foot.y) && foot.y>=-.2f && foot.y<=1.f?std::clamp(foot.y,-.16f,.76f):.17f;
    contacts[i]={hip+x,y,z};
    pose.footYaw[i]=std::isfinite(foot.yaw)?std::remainder(foot.yaw,motion::tau)*contactWeight:0;
    pose.footPitch[i]=std::isfinite(foot.pitch)?std::clamp(foot.pitch,-.3f,1.3f)*contactWeight:0;
  }
  // The push leg sweeps back, then the opposite leg reaches into the catch.
  const float drive=1-2*animation::SmoothStep((animation::Unit(dynamics.dashPhase)-.20f)/.50f);
  std::array<XMFLOAT3,2> targets{};
  std::array<float,2> strides{};
  pose.unconstrainedLift=pose.bodyLift;
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
      pose.footPitch[i]+=jumpClip.ankles[i].x*authoredAir*(1-contactWeight);
      pose.footYaw[i]+=jumpClip.ankles[i].y*authoredAir*(1-contactWeight);
      if(dynamics.flightPhase>=0){
        // A moving takeoff keeps its leading leg. The old fixed tuck brought
        // both legs together regardless of the stride that launched the jump.
        // Fade back to the authored landing extension before ground contact.
        const float lead=animation::Signed((dynamics.departureFeet[0].z-dynamics.departureFeet[1].z)/.45f);
        const float leg=i==0?1.f:-1.f,envelope=std::sin(flight*motion::tau*.5f)*authoredAir;
        target.z+=leg*lead*.07f*envelope;
        target.y=std::max(.17f,target.y+leg*(lead*.045f-std::abs(lead)*.015f)*envelope);
      }
    }
    if (contactWeight>0) {
      target={target.x+(contacts[i].x-target.x)*contactWeight,
        target.y+(contacts[i].y-target.y)*contactWeight,target.z+(contacts[i].z-target.z)*contactWeight};
    }
    // Fit both targets before solving either leg, including fallback/preview
    // poses without a world contact controller. With anatomical shin length,
    // even breathing can otherwise lift a standing ankle beyond its reach.
    // Retaining this fit through departure also prevents a takeoff hip snap.
    const auto hip=pose.PelvisPoint({side*.17f,.93f,0});
    const float x=target.x-hip.x,z=target.z-hip.z;
    // Keep a small non-singular bend without reintroducing a visible idle
    // crouch. The solver itself reserves 0.05 mm of extension.
    constexpr float reach=CharacterPose::thigh+CharacterPose::shin-.0002f;
    // A planted leg constrains the body; a recovering leg is free to shorten
    // its reach. Fitting the pelvis to both ankle targets dragged the entire
    // runner down every time the trailing foot left its reach sphere.
    const float fit=dynamics.authoredGait&&contactWeight>0?animation::Unit(
      dynamics.feetActive?dynamics.feetReachWeight[i]:dynamics.departureReachWeight[i]):1.f;
    const float limit=target.y+std::sqrt(std::max(0.f,reach*reach-x*x-z*z))-(hip.y-pose.bodyLift);
    pose.bodyLift+=std::min(0.f,limit-pose.bodyLift)*fit;
  }
  if(dynamics.pelvisStabilized&&std::isfinite(dynamics.pelvisLimit))
    pose.bodyLift=std::min(pose.bodyLift,std::clamp(dynamics.pelvisLimit,-.45f,.30f));
  // Release the planted pelvis constraint with the departing foot arrangement.
  // Dropping it in one frame pops the body upward during a repeat jump or a
  // takeoff from a long stride. This affects the pose, never the ballistic root.
  if(departure>0 && dynamics.departurePelvisStabilized && std::isfinite(dynamics.departurePelvisLimit)) {
    const float carried=std::min(pose.bodyLift,std::clamp(dynamics.departurePelvisLimit,-.45f,.30f));
    pose.bodyLift+=(carried-pose.bodyLift)*departure;
  }
  for (int i=0;i<2;++i) {
    const float side=i==0?-1.f:1.f;
    XMFLOAT3 knee{std::sin(pose.pelvisRotation.y)*.35f,0,std::cos(pose.pelvisRotation.y)};
    auto bend=XMLoadFloat3(&knee);
    const auto addKnee=[&](const playerclips::Frame& f,float w){
      auto authored=XMLoadFloat3(&f.knees[i]);
      if(XMVectorGetX(XMVector3LengthSq(authored))>.0001f){
        authored=XMVector3Normalize(XMVector3TransformNormal(authored,pose.PelvisTransform()));
        bend=XMVectorLerp(bend,authored,animation::Unit(w));
      }
    };
    addKnee(gaitClip,authoredWeight);addKnee(jumpClip,authoredAir);
    XMStoreFloat3(&knee,XMVector3Normalize(bend));
    pose.legs[i]=Solve(pose.PelvisPoint({side*.17f,.93f,0}),targets[i],knee,CharacterPose::thigh,CharacterPose::shin,.00005f);
    const float armStride=strides[i]+(std::clamp(contacts[i].z/.30f,-1.f,1.f)-strides[i])*contactWeight;
    const float attack=i==1&&!newStrike?swing:0;
    XMFLOAT3 hand{side*(.40f+air*.035f),1.02f+attack*.52f+air*.12f+landing*.08f+hit*.08f+dash*.12f+brake*.07f,
      .10f-armStride*.13f+attack*.21f+sway*side*.014f-dash*.20f+brake*.12f};
    hand.z+=dynamics.motion.reach*clipWeight;
    hand.x+=gaitClip.hands[i].x*handAuthoredWeight+idleClip.hands[i].x*handIdleWeight;
    hand.y+=gaitClip.hands[i].y*handAuthoredWeight+idleClip.hands[i].y*handIdleWeight;
    // The imported gait plus idle blend already supplies the entire arm path.
    // Cancel the legacy contact swing throughout that blend, including stops.
    hand.z+=(gaitClip.hands[i].z+armStride*.13f)*handAuthoredWeight+idleClip.hands[i].z*handIdleWeight;
    // Reach forward on descent to brace for impact; rising arms trail the push.
    hand.z+=air*(.18f*animation::SmoothStep(-rise)-.10f*std::max(0.f,rise))*(1-swing);
    if(armAir>0&&!armLanding) {
      const auto delta=flightArms.hands[i];const XMFLOAT3 authored{side*.40f+delta.x,1.02f+delta.y,.10f+delta.z};
      const float w=armAir*(1-swing)*(1-cast);
      hand={hand.x+(authored.x-hand.x)*w,hand.y+(authored.y-hand.y)*w,hand.z+(authored.z-hand.z)*w};
    }
    if(armLanding) {
      if(dynamics.authoredGait) {
        const auto d=landingArms.hands[i];const float w=armLandingWeight*(1-swing)*(1-cast);
        hand={hand.x+(side*.40f+d.x-hand.x)*w,hand.y+(1.02f+d.y-hand.y)*w,hand.z+(.10f+d.z-hand.z)*w};
      } else {hand.y+=landClip.hands[i].y*landing;hand.z+=landClip.hands[i].z*landing;}
    }
    if (i==0 && shield) hand={-.39f,1.13f+air*.08f+landing*.08f,.23f-armStride*.04f+dash*.09f};
    if(i==0 && shield) {
      hand.y+=gaitClip.hands[i].y*authoredWeight*.25f;
      hand.z+=gaitClip.hands[i].z*authoredWeight*.25f;
    }
    if (timed) {
      const float cut=stroke.cut;
      float weight=stroke.weight*(1-burst);
      if(i==0&&newStrike&&(bow||rpg::StaffWeapon(form))) {
        const auto clip=bow?(heavy?playerclips::Clip::BowPowerRelease:playerclips::Clip::BowRelease):heavy?playerclips::Clip::StaffSweep:playerclips::Clip::StaffThrust;
        weight*=animation::SmoothStep(dynamics.authoredStrikePhase/playerclips::Collection()[unsigned(clip)].contact);
      }
      // Coil beside the shoulder, cut across the body, then return to guard.
      XMFLOAT3 attackHand=i==1?
        XMFLOAT3{.46f-cut*.34f,(heavy?1.66f:1.48f)-cut*.38f,-.13f+cut*.59f}:
        XMFLOAT3{-.34f,1.25f,.23f-cut*.10f};
      if(authoredAttack) {const auto delta=attackClip.hands[i];const float scale=newStrike?1.f:.65f;
        attackHand={side*.40f+delta.x*scale,1.02f+delta.y*scale,.10f+delta.z*scale};}
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
    if(i==1 && rpg::PhysicalLauncher(form)&&!bow) {
      hand.y+=.20f*animation::Unit(action.aim);hand.z+=.20f*animation::Unit(action.aim);
      if(form==rpg::WeaponForm::Handbow) hand.x-=.12f*animation::Unit(action.aim);
    }
    const auto offsetShoulder=pose.shoulderOffset[i];
    hand.x+=offsetShoulder.x;hand.y+=offsetShoulder.y;hand.z+=offsetShoulder.z;
    const auto shoulder=pose.UpperPoint({side*.305f+offsetShoulder.x,1.43f+offsetShoulder.y,offsetShoulder.z}),wrist=pose.UpperPoint(hand);
    // Legacy procedural gestures have no authored elbow channel. Retain their
    // established bend plane; the transported solver belongs to authored clips.
    pose.arms[i]=dynamics.authoredGait || newStrike?SolveArm(shoulder,wrist,elbowPoles[i]):
      Solve(shoulder,wrist,elbowPoles[i],CharacterPose::upperArm,CharacterPose::forearm);
  }
  XMFLOAT3 weapon{.12f,-.76f+swing*1.72f+landing*.25f+air*.20f+dash*.32f+brake*.16f,.63f-swing*.45f};
  if(rpg::StaffWeapon(form))weapon={.10f,.99f,.04f+dash*.18f};
  // The archery layer fades in from an upright bow, not the sword's lowered
  // blade. Otherwise spawning or enabling authored locomotion flips the bow.
  if(bow)weapon=carryClip.weapon;
  if(dynamics.authoredGait&&dynamics.authoredEquipment==playerclips::Equipment::Sword)
    weapon=carryClip.weapon;
  if(dynamics.authoredGait&&dynamics.authoredEquipment!=playerclips::Equipment::None) {
    const XMFLOAT3 carry{idleClip.weapon.x*(1-gaitAmount)+gaitClip.weapon.x,
      idleClip.weapon.y*(1-gaitAmount)+gaitClip.weapon.y,idleClip.weapon.z*(1-gaitAmount)+gaitClip.weapon.z};
    const float w=(1-burst)*(1-hit)*(1-animation::Unit(dynamics.action.open))*authoredBlend;
    XMStoreFloat3(&weapon,XMVectorLerp(XMLoadFloat3(&weapon),XMLoadFloat3(&carry),w));
  }
  if(equippedFlight&&armAir>0&&!armLanding)
    weapon=blendDirection(weapon,flightArms.weapon,armAir*(1-burst)*(1-hit));
  if(equippedFlight&&armLanding)
    weapon=blendDirection(weapon,landingArms.weapon,armLandingWeight*(1-burst)*(1-hit));
  if (timed) {
    const float weight=stroke.weight*(1-burst);
    XMFLOAT3 cut{.22f-stroke.cut*.38f,1.05f-stroke.cut*1.25f,.05f+stroke.cut*1.05f};
    if(authoredAttack)cut=attackClip.weapon;
    if(form==rpg::WeaponForm::Mace) {const float x=cut.x;cut.x=x*std::cos(.12f)-cut.y*std::sin(.12f);cut.y=x*std::sin(.12f)+cut.y*std::cos(.12f);cut.z-=.025f*stroke.cut;}
    else if(form==rpg::WeaponForm::Cleaver) {cut.x+=.10f*(1-2*stroke.cut);cut.z+=.06f*stroke.cut;}
    // Blend out to the current movement layer, not a fixed standing blade.
    // Otherwise the first/last timed-attack sample snaps during flight or a
    // landing, and a suppressed attack overrides the dash's weapon clearance.
    const XMFLOAT3 rest=newStrike?weapon:rpg::StaffWeapon(form)?XMFLOAT3{.10f,.99f,.04f+dash*.18f}:
      XMFLOAT3{.12f,-.76f+landing*.25f+air*.20f+dash*.32f+brake*.16f,.63f};
    // Keep the retargeted blade in the forward weapon hemisphere. This avoids
    // a shortest-arc reversal behind the smaller player shoulder/shield rig.
    if(authoredAttack&&!newStrike)cut.z=std::max(.65f,cut.z);
    const auto a=XMVector3Normalize(XMLoadFloat3(&rest)),b=XMVector3Normalize(XMLoadFloat3(&cut));
    const float blend=weight;
    const float angle=std::acos(std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-.9999f,1.f));
    // Interpolate orientation on the unit sphere: linear direction blending
    // accelerated sharply as the blade crossed from hanging down to raised.
    const auto blended=angle>.0001f?
      (a*std::sin((1-blend)*angle)+b*std::sin(blend*angle))/std::sin(angle):a;
    XMStoreFloat3(&weapon,blended);
  }
  if(rpg::StaffWeapon(form)&&!newStrike&&dynamics.authoredEquipment!=playerclips::Equipment::Staff)weapon={.10f,.99f,.04f+dash*.18f};
  if(rpg::PhysicalLauncher(form)&&!bow) {
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
  pose.hasGripSocket=dynamics.authoredGait&&dynamics.authoredEquipment!=playerclips::Equipment::None;
  // The hand's bind wrist is (0,-.055,-.043) in its gripping frame. Solve
  // the forearm to that point, keeping the authored handle path as a socket.
  const auto shaftUp=XMLoadFloat3(&pose.weaponDirection);
  const auto across=pose.UpperDirection({1,0,0});const auto lateral=XMLoadFloat3(&across);
  auto shaftRight=XMVector3Normalize(lateral-shaftUp*XMVector3Dot(lateral,shaftUp));
  auto shaftForward=XMVector3Cross(shaftRight,shaftUp);
  auto wristOffset=-shaftRight*.055f+shaftForward*.043f;
  const bool bladeGrip=pose.hasGripSocket &&
    (form==rpg::WeaponForm::Blade || form==rpg::WeaponForm::Mace || form==rpg::WeaponForm::Cleaver);
  pose.fittedBladeGrip=pose.hasGripSocket&&form==rpg::WeaponForm::Blade;
  if(bladeGrip) {
    // A cutting grip closes across the handle while the knuckles continue
    // the forearm. Fixing this axis to torso +X bent the wrist sideways and
    // presented the broad blade edge-on, like a fencing foil.
    const auto socket=XMLoadFloat3(&pose.arms[1].end);
    // Parallel-transport the grip from forward instead of projecting +X.
    // Carry-to-guard can sweep across +X; projection collapses at that point
    // and otherwise turns a continuous blade path into a wrist flip.
    const auto frontPoint=pose.UpperDirection({0,0,1});
    const auto front=XMLoadFloat3(&frontPoint);
    const auto transport=XMVectorSetW(XMVector3Cross(front,shaftUp),1+XMVectorGetX(XMVector3Dot(front,shaftUp)));
    if(XMVectorGetX(XMVector4LengthSq(transport))>.000001f)
      shaftRight=XMVector3Rotate(lateral,XMQuaternionNormalize(transport));
    shaftForward=XMVector3Cross(shaftRight,shaftUp);
    const auto forearm=XMVector3Normalize(socket-XMLoadFloat3(&pose.arms[1].joint));
    // Follow the forearm's roll without exchanging sides of the handle when
    // its projection becomes parallel or crosses the blade's local axis.
    const auto radial=forearm-shaftUp*XMVector3Dot(forearm,shaftUp);
    // Stabilize only near an axial forearm. A global positive-axis clamp
    // reverses the relaxed down/back grip and bends the wrist sideways.
    const float radialLength=XMVectorGetX(XMVector3Length(radial));
    const float gripSupport=.12f*(1-animation::SmoothStep((radialLength-.05f)/.25f));
    const auto held=radial+shaftRight*gripSupport;
    if(XMVectorGetX(XMVector3LengthSq(held))>.000001f)
      shaftRight=XMVector3Normalize(held);
    shaftForward=XMVector3Cross(shaftRight,shaftUp);
    wristOffset=-shaftRight*.055f+shaftForward*.043f;
  }
  XMStoreFloat3(&pose.weaponRight,shaftRight);
  if(pose.fittedBladeGrip)
    wristOffset+=shaftUp*weapongrip::swordAlong-shaftForward*weapongrip::swordPalmInset;
  if(pose.hasGripSocket) {
    XMFLOAT3 wrist;XMStoreFloat3(&wrist,XMLoadFloat3(&pose.arms[1].end)+wristOffset);
    pose.arms[1]=SolveArm(pose.arms[1].root,wrist,elbowPoles[1]);
    XMStoreFloat3(&pose.weaponGrip,XMLoadFloat3(&pose.arms[1].end)-wristOffset);
  }
  if(rpg::StaffWeapon(form)&&!shield&&dynamics.authoredGait&&dynamics.authoredEquipment==playerclips::Equipment::Staff) {
    const auto& strike=playerclips::Collection()[unsigned(heavy?playerclips::Clip::StaffSweep:playerclips::Clip::StaffThrust)];
    const float loaded=newStrike?animation::SmoothStep(dynamics.authoredStrikePhase/strike.contact):1.f;
    pose.staffGripWeight=authoredBlend*(1-burst)*(1-hit)*(1-cast)*
      std::max(ready,stroke.weight*loaded)*(1-animation::SmoothStep(action.open/.70f));
    // A spell's open-hand gesture releases the supporting grip throughout
    // preparation as well as recovery. Full stroke weight must not repin the
    // casting palm to the shaft and erase its ability-specific choreography.
    // Retarget the two-handed shaft onto the shorter, wider native skeleton.
    // Project the shared grip into BOTH reach spheres before solving either arm.
    auto grip=XMLoadFloat3(&pose.weaponGrip),axis=shaftUp;
    const auto right=XMLoadFloat3(&pose.arms[1].root),left=XMLoadFloat3(&pose.arms[0].root);
    const float reach=CharacterPose::upperArm+CharacterPose::forearm-.007f;
    for(int iteration=0;iteration<12;++iteration)for(auto centre:{left-wristOffset-axis*.26f,right-wristOffset}) {
      const auto d=grip-centre;const float len=XMVectorGetX(XMVector3Length(d));
      if(len>reach)grip=centre+d*(reach/len);
    }
    XMFLOAT3 r,l;XMStoreFloat3(&r,XMVectorLerp(XMLoadFloat3(&pose.arms[1].end),grip+wristOffset,pose.staffGripWeight));
    XMStoreFloat3(&l,XMVectorLerp(XMLoadFloat3(&pose.arms[0].end),grip+axis*.26f+wristOffset,pose.staffGripWeight));
    pose.arms[1]=SolveArm(pose.arms[1].root,r,elbowPoles[1]);
    pose.arms[0]=SolveArm(pose.arms[0].root,l,elbowPoles[0]);
    XMStoreFloat3(&pose.weaponGrip,XMLoadFloat3(&pose.arms[1].end)-wristOffset);
  }
  if(bow && pose.bowDraw>0) {
    // Native arm lengths differ from the Blender actor. Keep the string hand
    // attached to the rendered nock through the draw, then release it smoothly.
    const auto up=XMLoadFloat3(&pose.weaponDirection);
    const auto localRight=pose.UpperDirection({1,0,0});const auto lateral=XMLoadFloat3(&localRight);
    const auto right=XMVector3Normalize(lateral-up*XMVector3Dot(lateral,up));
    const auto forward=XMVector3Cross(right,up);
    const auto nockOffset=up*.035f-forward*(.07f+.40f*pose.bowDraw);
    auto sharedGrip=XMLoadFloat3(&pose.weaponGrip);
    const auto leftRoot=XMLoadFloat3(&pose.arms[0].root),rightRoot=XMLoadFloat3(&pose.arms[1].root);
    constexpr float reach=CharacterPose::upperArm+CharacterPose::forearm-.007f;
    for(int iteration=0;iteration<12;++iteration)for(auto centre:{leftRoot-wristOffset-nockOffset,rightRoot-wristOffset}) {
      const auto d=sharedGrip-centre;const float length=XMVectorGetX(XMVector3Length(d));
      if(length>reach)sharedGrip=centre+d*(reach/length);
    }
    sharedGrip=XMVectorLerp(XMLoadFloat3(&pose.weaponGrip),sharedGrip,animation::SmoothStep(pose.bowDraw/.6f));
    XMFLOAT3 holdingWrist;XMStoreFloat3(&holdingWrist,sharedGrip+wristOffset);
    pose.arms[1]=SolveArm(pose.arms[1].root,holdingWrist,elbowPoles[1]);
    XMStoreFloat3(&pose.weaponGrip,XMLoadFloat3(&pose.arms[1].end)-wristOffset);
    const auto nock=XMLoadFloat3(&pose.weaponGrip)+nockOffset;
    const float grip=animation::SmoothStep(pose.bowDraw/.8f);
    XMFLOAT3 target;XMStoreFloat3(&target,XMVectorLerp(XMLoadFloat3(&pose.arms[0].end),nock+wristOffset,grip));
    pose.arms[0]=SolveArm(pose.arms[0].root,target,elbowPoles[0]);
  }
  return pose;
}
