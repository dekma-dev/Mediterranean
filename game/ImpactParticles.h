#pragma once
#include "engine/ParticlePhysics.h"
#include "game/Simulation.h"
class Renderer;

namespace impactfx {
// Immutable scenery broad phase shared by all presentation instances.
const particles::CollisionWorld& Environment();
enum class Material : std::uint8_t {Steel,Wood,Ember,Frost,Arcane,Holy};
class System {
public:
 void Reset(){pool_.Reset();}
 void Advance(float dt){if(pool_.Count())pool_.Advance(dt,Environment());}
 void Emit(const rpg::CombatVisualEvent&,unsigned seed);
 void Render(Renderer&)const;
 const particles::Pool& State()const{return pool_;}
private:
 particles::Pool pool_;
};
}
