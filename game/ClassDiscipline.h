#pragma once
#include "game/Classes.h"
#include <algorithm>
#include <cstdint>

namespace rpg {
// Transient combat opportunities. Contact times are measured at the end of an
// update; retaining a short negative tail preserves sub-frame expiry checks.
struct ClassDiscipline {
 float guard=-1,ready=-1,elements=-1;
 unsigned mask=0;
 std::uint32_t target=0;
 void Clear(){*this={};}
 void Advance(float dt){guard=std::max(-1.f,guard-dt);ready=std::max(-1.f,ready-dt);elements=std::max(-1.f,elements-dt);}
 void Prime(float seconds,float elapsed=0){ready=seconds-elapsed;}
 bool Take(float elapsed=0){if(ready+elapsed<=0)return false;ready=-1;return true;}
 static unsigned Element(int ability){
  switch(ability){
  case 0:case 1:case 7:case 9:case 11:return 1; // fire
  case 5:case 6:case 8:case 12:return 2; // frost
  case 10:case 13:return 4; // arcane
  default:return 0; // Thermal rupture cannot supply two schools at once.
  }
 }
 bool Contact(std::uint32_t actor,int ability,float elapsed){
  const auto bit=Element(ability);if(!bit || ready+elapsed>0)return false;
  if(target!=actor || elements+elapsed<=0){mask=0;target=actor;}
  // Repeating one element cannot keep an unfinished combination alive forever.
  if(!(mask&bit)){mask|=bit;elements=6.f-elapsed;}
  if(mask!=7)return false;
  mask=0;elements=-1;target=0;Prime(8,elapsed);return true;
 }
};
inline const char* DisciplineName(ClassId id){
 return id==ClassId::Warrior?"Riposte":id==ClassId::Mage?"Elemental confluence":id==ClassId::Priest?"Benediction":"Pursuit shot";
}
inline bool HasDiscipline(ClassId id){return AdvancedClass(id) || id==ClassId::Priest;}
inline const char* DisciplineDescription(ClassId id){
 return id==ClassId::Priest?
  "Complete Greater heal and restore missing health. For six seconds, your next Radiance or Rebuke hit adds 25% basic damage and grants an 18-point ward. Interrupted healing grants nothing.":id==ClassId::Warrior?
  "Absorb a frontal hit within 0.8 seconds of Iron guard. For five seconds, your next Strike or Reverse cut adds 50% basic damage and a 12-point ward. Misses do not consume it.":id==ClassId::Mage?
  "Land fire, frost and arcane on the same target, within six seconds between new elements. Gain eight seconds to commit one normally stationary spell while moving. It is spent on commitment, even if cancelled.":
  "Disengage at least 1.5 metres away from a selected living threat within nine metres. For five seconds, your next Quick shot or Twin shot adds 45% basic damage and slows for two seconds. Misses do not consume it.";
}
}
