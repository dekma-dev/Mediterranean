#pragma once
#include <array>

namespace weapongrip {
// Measured from the authored oval leather grip, including its winding.
// Values are hand-local: X follows the shaft; Y/Z surround the handle.
inline constexpr float swordAcross=.020f,swordDepth=.016f;
inline constexpr float swordPalmInset=.011f,swordAlong=.035f;
// Fit fixed-length phalanges independently around the handle surface.
inline constexpr std::array<std::array<float,3>,4> swordCurl{{
  {1.352949f,2.282950f,3.251560f},
  {1.269954f,2.361816f,3.372013f},
  {1.334107f,2.286154f,3.280571f},
  {1.070422f,1.834302f,2.811027f}
}};
}
