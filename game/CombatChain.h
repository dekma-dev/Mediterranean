#pragma once
#include <algorithm>
#include <cstdint>

namespace rpg {
// Transient, identity-bound contact history. Repeating one button cannot stack
// a chain; missed/cancelled attacks never call Contact. No saving mid-combo.
struct CombatChain {
  std::uint32_t target=0;
  int steps=0,last=-1;
  float remaining=0;
  void Clear() { *this={}; }
  // Retain sub-frame history so contact just before expiry is identical at
  // 30 and 144 Hz. elapsed is the contact-to-frame-end time, not future time.
  void Advance(float dt) { remaining=std::max(-1.f,remaining-dt); }
  int Steps(std::uint32_t id,float elapsed=0) const { return remaining+elapsed>0 && target==id?steps:0; }
  static bool Finisher(int ability) { return ability==1 || ability==5 || ability==6 || ability==9 || ability==11 || ability==14; }
  int Contact(std::uint32_t id,int ability,float elapsed) {
    const int before=Steps(id,elapsed);
    if(Finisher(ability)) {if(before>=2) {Clear();return before;} return 0;}
    if(ability!=0 && ability!=12 && ability!=13 && ability!=7) return 0;
    const bool alternate=(last==0 && ability==12) || (last==12 && ability==0);
    const bool link=(ability==13 || ability==7) && (last==0 || last==12);
    steps=before>0 && (alternate || link)?std::min(3,before+1):1;
    target=id;last=ability;remaining=std::max(0.f,5.f-elapsed);return 0;
  }
};
}
