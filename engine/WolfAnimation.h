#pragma once
#include "engine/Skinning.h"
#include <string_view>
namespace authored::wolf {
struct Sample { DirectX::XMFLOAT4 rotation; DirectX::XMFLOAT3 translation; };
struct Clip { unsigned frames=0; float duration=0,distance=0; std::vector<Sample> samples; };
struct Data { unsigned bones=0; std::array<std::vector<visual::SkinInfluence>,3> weights; std::array<Clip,4> clips; };
Data Decode(std::string_view bytes);
std::string_view EmbeddedBytes();
const Data& Collection();
// Read-only locomotion metadata shared by the controller and palette sampler.
struct GaitBlend { unsigned first=1,second=2;float blend=0,moving=0,stride=1; };
GaitBlend Blend(float travel);
float TravelAtSpeed(float metresPerSecond);
visual::SkinInfluence Influence(DirectX::XMFLOAT3 position);
visual::BodyPalette Pose(float time,float gait,float travel);
}
