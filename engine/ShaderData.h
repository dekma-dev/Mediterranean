#pragma once
#include "engine/Shadow.h"
#include <array>
#include <cstddef>
#include <type_traits>

// CPU upload types shared with the build-time shader reflection validator.
// Changes here and in HLSL must agree before bytecode can be published.
namespace shader {
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;
namespace slot {
inline constexpr unsigned scene=0,hands=2,cloud=3,lights=4,volumes=5,glow=6,fields=7,pressure=8;
}
struct Scene {
  XMFLOAT4X4 world,wvp,normalWorld,shadowVP[visual::shadowLayers];
  XMFLOAT4 color,eyeAndHud;
  XMFLOAT4 sun=visual::daylightSun;
  XMFLOAT4 atmosphere{0,.90f,.0032f,.53f};
};
struct Glow {XMFLOAT4 texel,screen,region;};
struct Volume {XMFLOAT4 centerKind,rightRadius,upRadius,axisRadius,clock,tint;};
struct Field {XMFLOAT4 clock,tint,shape,details;};
struct PressureWave {XMFLOAT4 centerStrength,radiiAge,axisStyle;};
using Volumes=std::array<Volume,24>;
using Fields=std::array<Field,12>;
using PressureWaves=std::array<PressureWave,8>;
struct Pressure {XMFLOAT4X4 inverseVP,vp;XMFLOAT4 eye,screen,region;PressureWaves waves;};
static_assert(visual::shadowLayers==2 && sizeof(Scene)==384);
static_assert(sizeof(Glow)==48 && sizeof(Volumes)==2304 && sizeof(Fields)==768 && sizeof(Pressure)==560);
static_assert(std::is_standard_layout_v<Scene> && std::is_standard_layout_v<Pressure>);
}
