#pragma once
#include "game/Simulation.h"
#include <DirectXMath.h>
class Renderer;
// Original texture layers over accepted combat clocks; never owns gameplay.
namespace spellvfx {
float ContactLifetime(const rpg::CombatVisualEvent& event);
void Contact(Renderer&,const rpg::CombatVisualEvent&,float age);
void Flight(Renderer&,const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float age);
void Preparation(Renderer&,const rpg::Simulation&,DirectX::XMFLOAT3 origin,float progress);
void EnemyContact(Renderer&,const rpg::CombatVisualEvent&,float age);
}
