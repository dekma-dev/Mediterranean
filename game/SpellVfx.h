#pragma once
#include "game/Simulation.h"
#include <DirectXMath.h>
class Renderer;
// Original texture layers over accepted combat clocks; never owns gameplay.
namespace spellvfx {
class FlightPath;
enum class ResidueKind {None,Ember,Frost,Arcane,Holy,Thermal};
struct Residue {
  rpg::Vec2 position{};
  ResidueKind kind=ResidueKind::None;
  float age=10,facing=0,size=1;
  unsigned seed=0;
  unsigned order=0;
};
Residue MakeResidue(const rpg::CombatVisualEvent&);
float ResidueLifetime(ResidueKind);
void Aftermath(Renderer&,const Residue&);
void GroundField(Renderer&,rpg::Vec2,float radius,float age,ResidueKind,float opacity,unsigned seed=0);
void FrostField(Renderer&,rpg::Vec2,float age,float remaining);
float ContactLifetime(const rpg::CombatVisualEvent& event);
float ArrowProgress(int ability,int ordinal,float age,float flight);
DirectX::XMFLOAT3 ProjectilePosition(const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float progress);
void Contact(Renderer&,const rpg::CombatVisualEvent&,float age);
bool HasClassImpact(const rpg::CombatVisualEvent&);
void ClassImpact(Renderer&,const rpg::CombatVisualEvent&,float age);
void Interrupt(Renderer&,const rpg::CombatVisualEvent&,float age);
void Flight(Renderer&,const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float age,const FlightPath* path=nullptr);
void MagicFlight(Renderer&,const rpg::CombatVisualEvent&,DirectX::XMFLOAT3 origin,float age,const FlightPath* path=nullptr);
void EnemyContact(Renderer&,const rpg::CombatVisualEvent&,float age,bool fullDetail=true,const DirectX::XMFLOAT3* origin=nullptr);
}
