#pragma once
#include "engine/StaticScene.h"
#include "game/FaceMotion.h"
class Renderer;
namespace rpg { class WorldMap; }
class WorldView {
public:
  void Reset() { scenery_.Invalidate(); }
  void Draw(Renderer& renderer,const rpg::WorldMap& map,float time,bool sparring=false,
    facial::Pose merchant={},facial::Pose keeper={});
  std::size_t StaticInstances() const { return scenery_.Instances(); }
  std::size_t StaticBytes() const { return scenery_.Bytes(); }
private:
  visual::StaticScene scenery_;
  bool sparring_=false;
};
