#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace sound {
inline unsigned TailPriority(unsigned priority,float remainingPeak) {
  // Quiet tails yield to real releases/contacts, but not to steps or UI ticks.
  return remainingPeak>=0 && remainingPeak<.025f?std::min(priority,2u):priority;
}
// A conservative upper bound for every sample still to play. Unlike a short
// look-ahead window, a suffix maximum stays safe if the game thread stalls.
struct EffectEnvelope {
  static constexpr unsigned block = 256;
  std::vector<float> remaining;
  void Build(const std::vector<std::int16_t>& pcm) {
    remaining.assign((pcm.size()+block-1)/block,0);
    float peak=0;
    for(size_t n=pcm.size();n>0;--n) {
      peak=std::max(peak,float(std::abs(int(pcm[n-1])))/32767.f);
      remaining[(n-1)/block]=peak;
    }
  }
  float Peak(std::uint64_t played) const {
    // Keep one completed block for the resampler's interpolation history.
    const auto index=played>block?(played-block)/block:0;
    return index<remaining.size()?remaining[size_t(index)]:0;
  }
};
}
