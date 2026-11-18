#pragma once
#include "game/WorldMap.h"
#include "engine/ParticlePhysics.h"
#include <unordered_map>
#include <vector>

namespace sound {
using AcousticPoint=particles::Vec3;
// Immutable broad phase for sound, separate from foliage/camera silhouettes.
// Heights and boxes are world-space; no render thread or actor mutation.
class AcousticWorld {
public:
  using HeightFunction=float(*)(rpg::Vec2);
  explicit AcousticWorld(const rpg::WorldMap& map);
  AcousticWorld(std::vector<particles::Box> walls,HeightFunction height);
  float Obstruction(AcousticPoint source,AcousticPoint listener) const;
  float Enclosure(AcousticPoint listener) const;
  size_t WallCount() const { return walls_.size(); }
private:
  std::vector<particles::Box> walls_;
  std::unordered_map<std::uint64_t,std::vector<unsigned>> bins_;
  HeightFunction height_=nullptr;
  void Index();
  float WallDistance(AcousticPoint from,AcousticPoint to) const;
  bool Blocked(AcousticPoint from,AcousticPoint to) const;
};
struct AcousticResponse {
  float obstruction=0,enclosure=0;
  void Update(float blocked,float room,float seconds);
  float DirectGain() const;
  float FilterCoefficient() const;
  float WetSend() const;
};
}
