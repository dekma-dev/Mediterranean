#pragma once
#include "engine/PlayerClips.h"
#include "game/AttackMotion.h"
#include "game/CombatWeight.h"
#include <algorithm>
#include <cmath>

namespace animation {
inline DirectX::XMFLOAT3 BlendWeaponAim(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,float weight);
inline DirectX::XMFLOAT3 BlendSwordReadyAim(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,float weight){
  using namespace DirectX;
  if(weight<=0)return from;if(weight>=1)return to;
  const auto a=XMVector3Normalize(XMLoadFloat3(&from)),b=XMVector3Normalize(XMLoadFloat3(&to));
  float separation=std::clamp(1-XMVectorGetX(XMVector3Dot(a,b)),0.f,1.f);
  separation=separation*separation*(3-2*separation);
  // An outboard spherical arc raises the sword around the right arm. Unlike
  // adding a direction vector, this cannot cancel a leftward follow-through.
  // Nearby poses retain their direct interpolation and identical poses stay put.
  const DirectX::XMFLOAT3 outside{.90630779f,0,.42261826f};
  const auto first=BlendWeaponAim(from,outside,weight),second=BlendWeaponAim(outside,to,weight);
  const auto curved=BlendWeaponAim(first,second,weight);
  if(separation>=1)return curved;
  return BlendWeaponAim(BlendWeaponAim(from,to,weight),curved,separation);
}
inline DirectX::XMFLOAT3 BlendWeaponAim(DirectX::XMFLOAT3 from,DirectX::XMFLOAT3 to,float weight){
  using namespace DirectX;
  if(weight<=0)return from;if(weight>=1)return to;
  auto a=XMLoadFloat3(&from),b=XMLoadFloat3(&to);
  if(XMVectorGetX(XMVector3LengthSq(a))<1e-8f)return to;
  if(XMVectorGetX(XMVector3LengthSq(b))<1e-8f)return from;
  a=XMVector3Normalize(a);b=XMVector3Normalize(b);
  const float dot=std::clamp(XMVectorGetX(XMVector3Dot(a,b)),-1.f,1.f);
  XMVECTOR value;
  if(dot>.9995f)value=XMVector3Normalize(XMVectorLerp(a,b,weight));
  else {
    auto tangent=XMVectorSubtract(b,XMVectorScale(a,dot));
    if(XMVectorGetX(XMVector3LengthSq(tangent))<1e-7f){
      const auto guide=std::abs(XMVectorGetZ(a))<.9f?XMVectorSet(0,0,1,0):XMVectorSet(0,1,0,0);
      tangent=XMVectorSubtract(guide,XMVectorMultiply(a,XMVector3Dot(guide,a)));
    }
    const float angle=std::acos(dot)*weight;
    value=XMVectorAdd(XMVectorScale(a,std::cos(angle)),XMVectorScale(XMVector3Normalize(tangent),std::sin(angle)));
  }
  XMFLOAT3 result;XMStoreFloat3(&result,value);return result;
}
// Player presentation clock. Contacts/releases still belong to Simulation.
// Recovery may finish after the attack timer; interruption freezes the outgoing
// pose instead of advancing through a hit which the simulation cancelled.
struct AuthoredWeaponController {
  static float LoadEnvelope(float t) {
    t=std::clamp(t,0.f,1.f);
    // Smooth acceleration/deceleration with a longer constant-speed middle.
    // A single smoothstep concentrated the carry-to-guard turn into a short
    // peak even when all source keys were continuous.
    constexpr float ramp=.15f,pi=3.14159265f;
    const auto edge=[=](float u){return (u-ramp/pi*std::sin(pi*u/ramp))*.5f/(1-ramp);};
    return t<ramp?edge(t):t>1-ramp?1-edge(1-t):(t-ramp*.5f)/(1-ramp);
  }
  playerclips::Frame frame{},outgoing{};
  float weight=0,phase=0,last=-1,clock=-1,blendAge=1,cancelAge=1,outgoingWeight=0,rate=1;
  float ready=0,entryReady=0,restAge=4;
  bool heavy=false,staff=false,tail=false,bow=false;
  CombatWeight accent{},outgoingAccent{},mechanics{},outgoingMechanics{};
  int ability=-1,style=-1;
  static constexpr float cancelDuration=.20f;
  void Update(float dt,float seconds,bool isHeavy,bool isStaff,bool enabled,float playbackRate=1,bool isBow=false,int currentAbility=-1,int currentStyle=-1) {
    if(!enabled) {*this={};return;}
    if(!std::isfinite(dt)||dt<=0)return;
    dt=std::min(dt,.05f);
    const float previousReady=ready;
    const bool active=std::isfinite(seconds)&&seconds>=0;
    restAge=active?0:std::min(4.f,restAge+dt);
    const float targetReady=1-combat::Smooth((restAge-1.6f)/1.2f);
    const float guardTime=isStaff||isBow||targetReady<ready?.18f:.32f;
    ready+=(targetReady-ready)*(1-std::exp(-dt/guardTime));
    const bool begin=active&&(last<0||seconds<last-.001f||isHeavy!=heavy||isStaff!=staff||isBow!=bow||currentAbility!=ability||currentStyle!=style);
    if(begin) {entryReady=previousReady;outgoing=frame;outgoingAccent=accent;outgoingMechanics=mechanics;outgoingWeight=weight;blendAge=0;heavy=isHeavy;staff=isStaff;bow=isBow;ability=currentAbility;style=currentStyle;}
    if(active) {clock=seconds;tail=false;cancelAge=1;rate=std::clamp(std::isfinite(playbackRate)?playbackRate:1.f,.1f,4.f);}
    else if(last>=0) {
      tail=last>=combat::Duration(heavy)-.06f;
      if(!tail) {outgoing=frame;outgoingWeight=weight;cancelAge=0;blendAge=1;}
    }
    if(!active&&tail)clock+=dt*rate;
    if(active||tail) {
      const auto clip=bow?(heavy?playerclips::Clip::BowPowerRelease:playerclips::Clip::BowRelease):staff?(heavy?playerclips::Clip::StaffSweep:playerclips::Clip::StaffThrust):
        (heavy?playerclips::Clip::Overhead:playerclips::Clip::Slash);
      const float contact=staff||bow?combat::StrokeStart(heavy):combat::Contact(heavy);
      // Let the loaded arm decelerate after contact before returning to guard.
      // This is presentation recovery; damage/release and cooldown stay on
      // the simulation clock, and a new attack can still interrupt the tail.
      // Blade recovery carries the returning arm's momentum for longer than
      // a release gesture. Contact/queue responsiveness is unchanged; a new
      // accepted stroke can blend out this tail immediately.
      const float recovery=staff||bow?(heavy?1.15f:.90f):(heavy?1.35f:1.15f);
      const float start=contact*.40f,end=contact+recovery;
      phase=playerclips::AttackPhase(clip,clock,start,contact,end);
      auto target=playerclips::Sample(clip,phase);
      if(!staff&&!bow&&(style<0||style==0)) {
        // A relaxed sword cannot reach a high guard and cut back down inside
        // the first contact interval. The source has a separate low-entry cut.
        const auto entry=heavy?playerclips::Clip::SwordCarrySweep:playerclips::Clip::SwordCarryRising;
        const auto carry=playerclips::Sample(entry,playerclips::AttackPhase(entry,clock,start,contact,end));
        // Match the carry/guard blend used by CharacterPose at attack entry.
        // Easing this a second time changes the starting pose at partial ready.
        const float initial=std::clamp(entryReady,0.f,1.f);
        // After contact, finish one coherent cut. Continuing to mix opposite
        // rising/downward recoveries can align the shaft with the forearm.
        const float destination=initial>=.5f?1.f:0.f;
        const float prepared=initial+(destination-initial)*combat::Smooth((clock-contact)/.24f);
        const bool authoredGrip=playerclips::HasWeaponFrame(carry)&&playerclips::HasWeaponFrame(target);
        const auto aim=BlendSwordReadyAim(carry.weapon,target.weapon,prepared);
        target=playerclips::Blend(carry,target,prepared);
        if(!authoredGrip)target.weapon=aim;
      }
      if(style==0 && (ability==8 || ability==11 || ability==14)) {
        // Wide techniques cross the body instead of reusing the overhead chop.
        // Re-time the imported slash to this action's authoritative contact.
        const auto sweep=playerclips::Sample(playerclips::Clip::Slash,
          playerclips::AttackPhase(playerclips::Clip::Slash,clock,start,contact,end));
        target.hands[1]=sweep.hands[1];target.elbows[1]=sweep.elbows[1];target.weapon=sweep.weapon;target.weaponRight=sweep.weaponRight;
        target.torso.y=sweep.torso.y;target.pelvis.y=sweep.pelvis.y;
      }
      if(style==1) {
        const float strike=playerclips::Collection()[unsigned(clip)].contact;
        const float after=(phase-strike)/(1-strike);
        const float follow=after<.20f?combat::Smooth(after/.20f):1-combat::Smooth((after-.20f)/.80f);
        // The string hand follows back past the cheek as the bow arm stays
        // extended, then both settle. The existing nock solve owns the draw.
        target.hands[0].z-=.085f*follow;target.hands[0].x-=.025f*follow;
        target.hands[0].y+=.025f*follow;target.elbows[0].z-=.08f*follow;
        target.hands[1].z+=.018f*follow;
      }
      if(style==0 && ability==12) {
        // The backhand actually crosses in the opposite direction. Keep the
        // lower-body load/accent clock forward and reverse only the weapon arm.
        const auto reverse=playerclips::Sample(clip,ReturnCutPhase(phase,playerclips::Collection()[unsigned(clip)].contact));
        target.hands[1]=reverse.hands[1];target.elbows[1]=reverse.elbows[1];target.weapon=reverse.weapon;target.weaponRight=reverse.weaponRight;
      }
      if(style==3) {
        // A one-handed reliquary gathers toward the chest and sends the free
        // palm forward. Reuse the contact clock, not the staff's two-hand grip.
        const float strike=playerclips::Collection()[unsigned(clip)].contact;
        const auto pulse=[&](float begin,float peak,float end){return phase<peak?combat::Smooth((phase-begin)/(peak-begin)):1-combat::Smooth((phase-peak)/(end-peak));};
        const float gather=pulse(0,strike*.70f,strike+.10f),send=pulse(strike*.55f,strike+(1-strike)*.18f,1);
        const float solemn=ability==5 || ability==6?1.f:.72f;
        target={};
        target.hands[0]={.16f*gather+.09f*send,(.28f*gather+.23f*send)*solemn,.06f*gather+.31f*send};
        target.hands[1]={-.10f*gather-.035f*send,(.25f*gather+.12f*send)*solemn,.07f*gather+.08f*send};
        target.weapon={.08f,.93f,.32f};
      }
      // Load the guarded source action from the incoming carry throughout
      // preparation, arriving at full action weight on the contact clock.
      const float entrance=staff||bow?std::min(.22f,contact):contact;
      const float guardedLoad=staff||bow?combat::Smooth(clock/entrance):LoadEnvelope(clock/entrance);
      // Low-entry source poses already depart from carry. Applying another
      // contact-length fade squeezed their authored arc into the last frames.
      const bool lowEntry=!staff&&!bow&&(style<0||style==0)&&
        ability!=8&&ability!=11&&ability!=12&&ability!=14;
      const float load=lowEntry?combat::Smooth(clock/.035f):guardedLoad;
      // Complete the authored follow-through before blending to guard. An
      // early second fade folded the blade toward the forearm in mid-cut.
      const float release=contact+(staff||bow?.10f:heavy?.50f:.40f);
      const float w=load*(1-combat::Smooth((clock-release)/(end-release)));
      blendAge=std::min(.10f,blendAge+dt);
      // Fade an interrupted stroke once. Multiplying two entrance envelopes
      // compressed a fresh attack's preparation into its last few frames.
      const float blend=outgoingWeight>0?combat::Smooth(blendAge/.10f):1.f;
      weight=outgoingWeight*(1-blend)+w*blend;
      // Blend weighted displacements, so a nearly invisible incoming guard does
      // not pull the still-visible outgoing hand to an unrelated position.
      const float frameBlend=weight>.00001f?w*blend/weight:1;
      frame=playerclips::Blend(outgoing,target,frameBlend);
      // A weighted linear direction can collapse between opposing cuts. Its
      // later normalization whips the attached wrist through the zero vector.
      // Positions keep their weighted blend; orientation follows a unit arc.
      if(!playerclips::HasWeaponFrame(outgoing)||!playerclips::HasWeaponFrame(target))
        frame.weapon=BlendWeaponAim(outgoing.weapon,target.weapon,frameBlend);
      const float accentBlend=weight>.00001f?w*blend/weight:1;
      accent=outgoingAccent*(1-accentBlend)+AbilityAccent(style,ability,phase,playerclips::Collection()[unsigned(clip)].contact)*accentBlend;
      // The support/brace follows the outgoing strike too. Recomputing it from
      // the new phase in the pose builder reset the chest and pelvis in one
      // frame when an accepted attack interrupted the previous recovery.
      const auto body=staff?CombatWeight{}:AttackWeight(bow,heavy,phase,playerclips::Collection()[unsigned(clip)].contact);
      mechanics=outgoingMechanics*(1-accentBlend)+body*accentBlend;
      if(clock>=end) {tail=false;clock=-1;weight=0;}
    } else if(cancelAge<cancelDuration) {
      const float previousCancel=combat::Smooth(cancelAge/cancelDuration);
      cancelAge=std::min(cancelDuration,cancelAge+dt);frame=outgoing;
      // The frozen wind-up carries an incoming stance as well as a weapon
      // frame. Return that stance continuously on cancellation; otherwise
      // its last nonzero sample drops straight from carry into combat guard.
      const float nextCancel=combat::Smooth(cancelAge/cancelDuration);
      const float release=(nextCancel-previousCancel)/std::max(.000001f,1-previousCancel);
      entryReady+=(ready-entryReady)*release;
      weight=outgoingWeight*(1-nextCancel);
    } else weight=0;
    last=active?seconds:-1;
  }
};
}
