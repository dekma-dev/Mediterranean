#pragma once
#include "engine/PlayerClips.h"
#include "game/AttackMotion.h"
#include "game/CombatWeight.h"
#include <algorithm>
#include <cmath>

namespace animation {
// Player presentation clock. Contacts/releases still belong to Simulation.
// Recovery may finish after the attack timer; interruption freezes the outgoing
// pose instead of advancing through a hit which the simulation cancelled.
struct AuthoredWeaponController {
  playerclips::Frame frame{},outgoing{};
  float weight=0,phase=0,last=-1,clock=-1,blendAge=1,cancelAge=1,outgoingWeight=0,rate=1;
  float ready=0,restAge=4;
  bool heavy=false,staff=false,tail=false,bow=false;
  CombatWeight accent{},outgoingAccent{};
  int ability=-1,style=-1;
  void Update(float dt,float seconds,bool isHeavy,bool isStaff,bool enabled,float playbackRate=1,bool isBow=false,int currentAbility=-1,int currentStyle=-1) {
    if(!enabled) {*this={};return;}
    if(!std::isfinite(dt)||dt<=0)return;
    dt=std::min(dt,.05f);
    const bool active=std::isfinite(seconds)&&seconds>=0;
    restAge=active?0:std::min(4.f,restAge+dt);
    const float targetReady=1-combat::Smooth((restAge-1.6f)/1.2f);
    ready+=(targetReady-ready)*(1-std::exp(-dt/.18f));
    const bool begin=active&&(last<0||seconds<last-.001f||isHeavy!=heavy||isStaff!=staff||isBow!=bow||currentAbility!=ability||currentStyle!=style);
    if(begin) {outgoing=frame;outgoingAccent=accent;outgoingWeight=weight;blendAge=0;heavy=isHeavy;staff=isStaff;bow=isBow;ability=currentAbility;style=currentStyle;}
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
      const float start=contact*.40f,end=contact+(heavy?.95f:.75f);
      phase=playerclips::AttackPhase(clip,clock,start,contact,end);
      auto target=playerclips::Sample(clip,phase);
      if(style==0 && ability==12) {
        // The backhand actually crosses in the opposite direction. Keep the
        // lower-body load/accent clock forward and reverse only the weapon arm.
        const auto reverse=playerclips::Sample(clip,ReturnCutPhase(phase,playerclips::Collection()[unsigned(clip)].contact));
        target.hands[1]=reverse.hands[1];target.elbows[1]=reverse.elbows[1];target.weapon=reverse.weapon;
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
      const float w=combat::Smooth(clock/std::min(.22f,contact))*(1-combat::Smooth((clock-contact-.10f)/(end-contact-.10f)));
      blendAge=std::min(.10f,blendAge+dt);
      // Fade an interrupted stroke once. Multiplying two entrance envelopes
      // compressed a fresh attack's preparation into its last few frames.
      const float blend=outgoingWeight>0?combat::Smooth(blendAge/.10f):1.f;
      weight=outgoingWeight*(1-blend)+w*blend;
      // Blend weighted displacements, so a nearly invisible incoming guard does
      // not pull the still-visible outgoing hand to an unrelated position.
      frame=playerclips::Blend(outgoing,target,weight>.00001f?w*blend/weight:1);
      const float accentBlend=weight>.00001f?w*blend/weight:1;
      accent=outgoingAccent*(1-accentBlend)+AbilityAccent(style,ability,phase,playerclips::Collection()[unsigned(clip)].contact)*accentBlend;
      if(clock>=end) {tail=false;clock=-1;weight=0;}
    } else if(cancelAge<.16f) {
      cancelAge=std::min(.16f,cancelAge+dt);frame=outgoing;
      weight=outgoingWeight*(1-combat::Smooth(cancelAge/.16f));
    } else weight=0;
    last=active?seconds:-1;
  }
};
}
