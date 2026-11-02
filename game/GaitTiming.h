#pragma once
#include <algorithm>
#include <cmath>

namespace animation {
struct GaitTiming { float stride,stance,period,swing,reach,run; };
inline GaitTiming TravelGait(float speed) {
  speed=std::isfinite(speed)?std::max(0.f,speed):0;
  const float t=std::clamp((speed-1.8f)/3.4f,0.f,1.f);
  const float blend=t*t*(3-2*t);
  // Full left/right cycle, in metres. At 1.4 m/s: ~116 steps/min;
  // at 7 m/s: ~247. Shortening the same cycle to 1.57 m caused frantic feet.
  const float stride=1.45f+1.95f*blend;
  const float stance=.62f-.36f*blend;
  const float period=stride/std::max(.7f,speed);
  return {stride,stance,period,std::clamp(period*(1-stance),.19f,.56f),stride*stance*.5f,blend};
}
}
