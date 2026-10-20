#pragma once
#include "engine/StaticScene.h"
class Renderer;
namespace rpg { class WorldMap; }
class WorldView {
public:
  void Reset() { scenery_.Invalidate(); }
  void Draw(Renderer& renderer,const rpg::WorldMap& map,float time,bool sparring=false);
  std::size_t StaticInstances() const { return scenery_.Instances(); }
  std::size_t StaticBytes() const { return scenery_.Bytes(); }
private:
  visual::StaticScene scenery_;
  bool sparring_=false;
};
