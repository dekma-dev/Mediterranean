#pragma once
#include <cstdint>
#include <vector>

namespace rpg {
using EntityId = std::uint32_t;
struct Vec2 { float x = 0, z = 0; };
enum class PropKind { Building, Tree, Ruin };
struct Obstacle {
  Vec2 center;
  float halfX, halfZ, height;
  PropKind kind = PropKind::Building;
};

// Shared by simulation, navigation, rendering and map UI. No platform dependencies.
class WorldMap {
public:
  WorldMap();
  static constexpr float halfWidth = 17, halfDepth = 19;
  static constexpr Vec2 spawn{0,-10}, questGiver{-3,-10};
  const std::vector<Obstacle>& Obstacles() const { return obstacles_; }
  bool CanStand(Vec2 position, float radius) const;
  bool CanTravel(Vec2 from, Vec2 to, float radius) const;
  std::vector<Vec2> FindPath(Vec2 from, Vec2 to, float radius = .45f) const;
  const char* AreaName(Vec2 position) const;
private:
  std::vector<Obstacle> obstacles_;
};
}
