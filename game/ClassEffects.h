#pragma once
#include "game/Simulation.h"
#include <DirectXMath.h>

class Renderer;

// Stateless, depth-tested shapes. No timers, allocations or gameplay writes.
namespace classfx {
DirectX::XMFLOAT4 Color(rpg::ClassId school,float alpha=1);
void Projectile(Renderer& renderer,const rpg::CombatVisualEvent& event,DirectX::XMFLOAT3 origin,float age);
void Impact(Renderer& renderer,const rpg::CombatVisualEvent& event,float age);
void Prepare(Renderer& renderer,const rpg::Simulation& state,DirectX::XMFLOAT3 releasePoint);
void States(Renderer& renderer,const rpg::Simulation& state,float time);
void BarrierContact(Renderer& renderer,const rpg::CombatVisualEvent& event,float age);
void Recovery(Renderer& renderer,const rpg::CombatVisualEvent& event,float age);
}
