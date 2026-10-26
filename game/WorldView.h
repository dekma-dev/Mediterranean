#pragma once
#include "engine/StaticScene.h"
#include "game/FaceMotion.h"
#include "game/Wildlife.h"
class Renderer;
namespace rpg { class WorldMap; }
class WorldView {
public:
  void Reset() { scenery_.Invalidate(); wildlife_.Reset(); }
  void ResetWildlife() { wildlife_.Reset(); }
  void UpdateWildlife(float dt,const rpg::WorldMap& map,rpg::Vec2 player,bool alive) { wildlife_.Update(dt,map,player,alive); }
  void Draw(Renderer& renderer,const rpg::WorldMap& map,float time,bool sparring=false,
    facial::Pose merchant={},facial::Pose keeper={});
  std::size_t StaticInstances() const { return scenery_.Instances(); }
  std::size_t StaticBytes() const { return scenery_.Bytes(); }
private:
  visual::StaticScene scenery_;
  bool sparring_=false;
  ambient::Wildlife wildlife_;
};
