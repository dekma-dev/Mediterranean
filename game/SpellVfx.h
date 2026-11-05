#pragma once
#include "game/Simulation.h"
#include <DirectXMath.h>
class Renderer;
// Original texture layers over accepted combat clocks; never owns gameplay.
namespace spellvfx {
float ContactLifetime(const rpg::CombatVisualEvent& event);
float ArrowProgress(int ability,int ordinal,float age,float flight);
DirectX::XMFLOAT3 ProjectilePosition(const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float progress);
void Contact(Renderer&,const rpg::CombatVisualEvent&,float age);
void Flight(Renderer&,const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float age);
void EnemyContact(Renderer&,const rpg::CombatVisualEvent&,float age,bool fullDetail=true);
}
