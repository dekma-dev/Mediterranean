#pragma once

namespace test {
// A stationary, telegraphing target five metres ahead; initial facing is away.
inline constexpr const char* dashFixture=
  "MEDITERRANEAN 3\n-4 2 100 3.14159 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 0 0 1 0.7 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
// Sight clears the storehouse edge, but the champion's body must collide.
inline constexpr const char* dashWallFixture=
  "MEDITERRANEAN 3\n-8.4 0 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -8.4 5 80 0 0 1 0.7 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
}
