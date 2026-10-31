#pragma once
#include "engine/PlayerClips.h"
#include "game/AttackMotion.h"
#include <algorithm>
#include <cmath>

namespace animation {
// Player presentation clock. Contacts/releases still belong to Simulation.
// Recovery may finish after the attack timer; interruption freezes the outgoing
// pose instead of advancing through a hit which the simulation cancelled.
struct AuthoredWeaponController {
  playerclips::Frame frame{},outgoing{};
  float weight=0,phase=0,last=-1,clock=-1,blendAge=1,cancelAge=1,outgoingWeight=0,rate=1;
  bool heavy=false,staff=false,tail=false,bow=false;
  void Update(float dt,float seconds,bool isHeavy,bool isStaff,bool enabled,float playbackRate=1,bool isBow=false) {
    if(!enabled) {*this={};return;}
    if(!std::isfinite(dt)||dt<=0)return;
    dt=std::min(dt,.05f);
    const bool active=std::isfinite(seconds)&&seconds>=0;
    const bool begin=active&&(last<0||seconds<last-.001f||isHeavy!=heavy||isStaff!=staff||isBow!=bow);
    if(begin) {outgoing=frame;outgoingWeight=weight;blendAge=0;heavy=isHeavy;staff=isStaff;bow=isBow;}
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
      const auto target=playerclips::Sample(clip,phase);
      const float w=combat::Smooth(clock/std::min(.10f,start))*(1-combat::Smooth((clock-contact-.10f)/(end-contact-.10f)));
      blendAge=std::min(.10f,blendAge+dt);const float blend=combat::Smooth(blendAge/.10f);
      weight=outgoingWeight*(1-blend)+w*blend;
      // Blend weighted displacements, so a nearly invisible incoming guard does
      // not pull the still-visible outgoing hand to an unrelated position.
      frame=playerclips::Blend(outgoing,target,weight>.00001f?w*blend/weight:1);
      if(clock>=end) {tail=false;clock=-1;weight=0;}
    } else if(cancelAge<.16f) {
      cancelAge=std::min(.16f,cancelAge+dt);frame=outgoing;
      weight=outgoingWeight*(1-combat::Smooth(cancelAge/.16f));
    } else weight=0;
    last=active?seconds:-1;
  }
};
}
