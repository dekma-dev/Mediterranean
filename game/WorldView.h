#pragma once
#include "engine/StaticScene.h"
#include "game/FaceMotion.h"
#include "game/Wildlife.h"
class Renderer;
namespace rpg { class WorldMap; }
struct TrainingPropPose {
  float pitch=0,roll=0;
  static TrainingPropPose FromRecoil(float forward,float side) { return {-forward*.045f,-side*.045f}; }
};
class WorldView {
public:
  void Reset() { scenery_.Invalidate(); ResetWildlife(); }
  void ResetWildlife() { wildlife_.Reset(); wildlifePreview_=false; }
  void UpdateWildlife(float dt,const rpg::WorldMap& map,rpg::Vec2 player,bool alive) { wildlifePreview_=true; wildlife_.Update(dt,map,player,alive); }
  void ResetResidents() { for(unsigned i=0;i<residents_.size();++i)residents_[i].Reset(4+i); }
  void UpdateResidents(float dt,rpg::Vec2 player,bool alive,bool talking,rpg::Vec2 speaker);
  const facial::Pose& ResidentPose(unsigned i) const { return residents_.at(i).pose; }
  void Draw(Renderer& renderer,const rpg::WorldMap& map,float time,bool sparring=false,
    facial::Pose merchant={},facial::Pose keeper={},rpg::Vec2 observer=rpg::WorldMap::spawn,
    const std::array<TrainingPropPose,3>& training={});
  std::size_t StaticInstances() const { return scenery_.Instances(); }
  std::size_t StaticBytes() const { return scenery_.Bytes(); }
private:
  visual::StaticScene scenery_;
  ambient::Wildlife wildlife_;
  bool wildlifePreview_=false;
  std::array<facial::State,rpg::WorldMap::residents.size()> residents_{};
};
