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
visual::SkinInfluence Influence(DirectX::XMFLOAT3 position);
visual::BodyPalette Pose(float time,float gait,float travel);
}
