#pragma once
#include <cstdint>
#include <vector>

namespace rpg {
using EntityId = std::uint32_t;
struct Vec2 { float x = 0, z = 0; };
struct Trail { Vec2 from,to; float halfWidth; bool paved; };
struct GroundSurface { float dirt=0,paving=0; };
struct GroundCover { Vec2 position; float scale,yaw; bool shrub; };
enum class PropKind { Building, Tree, Ruin,Counter };
enum class BuildingRole { Homestead,WatchHall,Storehouse,TrainingHall };
struct Obstacle {
  Vec2 center;
  float halfX, halfZ, height;
  PropKind kind = PropKind::Building;
  BuildingRole role = BuildingRole::Homestead;
  float RoofHeight() const { return halfX*.75f; }
  // Camera bounds include the art silhouette; actors still collide with trunks
  // and wall footprints so they can walk underneath foliage and roof overhangs.
  float CameraHalfX() const { return kind==PropKind::Tree?2.f:halfX+.35f; }
  float CameraHalfZ() const { return kind==PropKind::Tree?2.f:halfZ+(kind==PropKind::Building?.45f:.35f); }
  float CameraHeight() const { return kind==PropKind::Counter?3.1f:
    height+(kind==PropKind::Building?RoofHeight()+1.f:kind==PropKind::Tree?.5f:1.f); }
};

// Shared by simulation, navigation, rendering and map UI. No platform dependencies.
class WorldMap {
public:
  WorldMap();
  static constexpr float halfWidth = 17, halfDepth = 19;
  static constexpr Vec2 spawn{0,-10}, questGiver{-3,-10};
  static constexpr Vec2 astralAnchor{0,12.8f};
  static constexpr Vec2 trainingPost{12,-2};
  static constexpr Vec2 quartermaster{3,-12};
  static constexpr Vec2 dockMin{-13.35f,-3.2f},dockMax{-9.75f,1.25f};
  const std::vector<Obstacle>& Obstacles() const { return obstacles_; }
  const std::vector<Trail>& Trails() const { return trails_; }
  const std::vector<GroundCover>& Cover() const { return cover_; }
  GroundSurface SurfaceAt(Vec2 position) const;
  bool CanStand(Vec2 position, float radius) const;
  bool CanTravel(Vec2 from, Vec2 to, float radius) const;
  std::vector<Vec2> FindPath(Vec2 from, Vec2 to, float radius = .45f) const;
  const char* AreaName(Vec2 position) const;
private:
  std::vector<Obstacle> obstacles_;
  std::vector<Trail> trails_;
  std::vector<GroundCover> cover_;
};
}
