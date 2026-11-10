#pragma once
#include "game/Simulation.h"
#include <array>
#include <vector>
class Renderer;
namespace presentation {struct Actor;}

// Presentation-only envelopes. Slots follow actor identity, never roster index.
class SustainedEffects {
public:
  static constexpr std::size_t maxActors=8,maxWarnings=8;
  void Reset();
  void Update(const rpg::Simulation& state,float dt);
  void Render(Renderer&,const rpg::Simulation&,float time,float renderedFacing) const;
  static void Warnings(Renderer&,const rpg::Simulation&,const std::vector<presentation::Actor>* actors=nullptr);
  std::size_t ActorCount() const;
  float Weight(rpg::EntityId actor,int channel) const;
private:
  struct Channel {float weight=0,age=0;bool active=false;void Step(float remaining,float dt);};
  struct Actor {
    rpg::EntityId id=0;rpg::EnemyKind kind=rpg::EnemyKind::Raider;int rank=0;
    rpg::Vec2 home{},position{};float facing=0;
    std::array<Channel,4> channels{};
    bool Matches(const rpg::Enemy&) const;
  };
  std::array<Actor,maxActors> actors_{};
  std::array<Channel,2> player_{};
  rpg::ClassId school_=rpg::ClassId::Count;
};
