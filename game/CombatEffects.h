#pragma once
#include "game/Simulation.h"
#include <DirectXMath.h>
#include <array>

class Renderer;

// Bounded cosmetic state, advanced by Game updates rather than render calls.
class CombatEffects {
public:
  static constexpr std::size_t maxBursts=32,maxTrails=8,trailSamples=32;
  void Reset();
  void Advance(float dt);
  void Emit(const rpg::CombatVisualEvent& event);
  void TrackWeapon(rpg::EntityId actor,DirectX::XMFLOAT3 root,DirectX::XMFLOAT3 tip);
  void CancelTrail(rpg::EntityId actor);
  void CancelTrails();
  void TrackDash(DirectX::XMFLOAT3 position,bool active);
  void CancelDash();
  std::size_t DashSampleCount() const { return wake_.count; }
  bool Tracking(rpg::EntityId actor) const;
  std::size_t BurstCount() const;
  std::size_t TrailCount() const;
  void Render(Renderer& renderer) const;
private:
  struct Burst { rpg::CombatVisualEvent event; float age=1; unsigned seed=0; };
  struct Sample { DirectX::XMFLOAT3 root,tip; float age=0; };
  struct Trail {
    rpg::EntityId actor=0;
    rpg::CombatVisualKind kind=rpg::CombatVisualKind::Strike;
    float age=1;
    DirectX::XMFLOAT3 origin{};
    float facing=0;
    std::array<Sample,trailSamples> samples{};
    std::size_t count=0;
  };
  struct WakeSample { DirectX::XMFLOAT3 position; float age=0; };
  struct Wake {
    std::array<WakeSample,trailSamples> samples{};
    std::size_t count=0;
    float facing=0;
    bool active=false;
  } wake_;
  std::array<Burst,maxBursts> bursts_{};
  std::array<Trail,maxTrails> trails_{};
  unsigned serial_=0;
};
