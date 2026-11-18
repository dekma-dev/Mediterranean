#pragma once
#include "game/Simulation.h"
#include <DirectXMath.h>
class Renderer;
namespace impactfx {
struct Pressure {
 DirectX::XMFLOAT3 center{},radii{};
 DirectX::XMFLOAT4 tint{};
 float strength=0,age=0,facing=0,groundRadius=0,groundOpacity=0;
 unsigned style=0;
};
Pressure PressureAt(const rpg::CombatVisualEvent&,float age);
void DrawPressure(Renderer&,const Pressure&,bool ground=true);
}
