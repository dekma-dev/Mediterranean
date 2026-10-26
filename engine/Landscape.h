#pragma once
#include <algorithm>
#include <cmath>

namespace visual {
// Decorative ridge profile in unit coordinates. The navigable island remains
// unchanged; scenery uses the same height function to ground its authored trees.
inline float RidgeHeight(float x,float z) {
  const float r2=x*x+z*z;
  const float edge=std::max(0.f,1-r2);
  return edge*edge*(.86f+.14f*std::cos(x*4+z*3));
}
}
