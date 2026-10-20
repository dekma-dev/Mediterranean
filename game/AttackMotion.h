#pragma once
#include <algorithm>
#include <cmath>

// Shared clock for authoritative melee contact and the displayed weapon pose.
namespace combat {
inline float Contact(bool heavy) { return heavy?.30f:.18f; }
inline float StrokeStart(bool heavy) { return Contact(heavy)-.09f; }
inline float Duration(bool heavy) { return heavy?.76f:.52f; }
inline float Smooth(float t) { t=std::clamp(t,0.f,1.f); return t*t*(3-2*t); }
struct Pose { float weight=0,coil=0,cut=0,settle=0; };
inline Pose Sample(float seconds,bool heavy) {
  if (!std::isfinite(seconds) || seconds<0 || seconds>=Duration(heavy)) return {};
  // Most of the cut happens before contact; the final 30 ms is follow-through.
  const float start=StrokeStart(heavy),contact=Contact(heavy),follow=contact+.03f;
  Pose p;
  p.weight=seconds<start?Smooth(seconds/start):1-Smooth((seconds-follow)/(Duration(heavy)-follow));
  p.coil=p.weight*(1-Smooth((seconds-start)/(contact-start)));
  p.cut=Smooth((seconds-start)/(follow-start));
  p.settle=Smooth((seconds-follow)/(Duration(heavy)-follow));
  return p;
}
}
