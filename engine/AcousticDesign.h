#pragma once
#include <cstdint>
#include <vector>
namespace sound {
enum class Cue;
bool HasAcousticDesign(Cue);
std::vector<std::int16_t> AcousticEffect(Cue);
std::vector<std::int16_t> ComposeScore(bool menu);
}
