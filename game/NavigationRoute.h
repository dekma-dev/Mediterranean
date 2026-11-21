#pragma once
#include "game/Simulation.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace ui {
// Read-only walking guidance. Never issues movement commands or changes a quest.
class NavigationRoute {
public:
  enum class State { None, Unreachable, Travelling, Arrived };
  void Reset() { *this={}; }
  State Status() const { return state_; }
  const std::vector<rpg::Vec2>& Points() const { return points_; }
  rpg::Vec2 Destination() const { return destination_; }
  rpg::Vec2 Endpoint() const { return endpoint_; }
  float Distance() const { return distance_; }
  unsigned Searches() const { return searches_; }
  static float Separation(rpg::Vec2 a,rpg::Vec2 b) { return std::hypot(a.x-b.x,a.z-b.z); }
  static bool HasObjective(const rpg::Simulation& s) {
    return s.TrackedHunt()>=0 || s.ReplayActive() || s.AnchorState()!=rpg::AnchorQuest::Complete ||
      s.FrontierCompleted()<rpg::frontierQuestCount;
  }
  void Update(float dt,const rpg::Simulation& s,bool marked,rpg::Vec2 pin) {
    Update(dt,s.Map(),s.Player(),marked?pin:s.ObjectivePosition(),
      s.Health()>0 && (marked || HasObjective(s)),marked);
  }
  void Update(float dt,const rpg::WorldMap& map,rpg::Vec2 from,rpg::Vec2 goal,bool enabled,bool marked=false) {
    if(!enabled) { Reset(); return; }
    if(!std::isfinite(dt) || dt<0 || !Finite(from) || !Finite(goal)) { Reset(); return; }
    elapsed_+=std::min(dt,.25f);
    const bool changed=state_==State::None || marked!=marked_ || Separation(goal,destination_)>(marked?.0001f:.5f);
    if(!changed && elapsed_<.25f)return;
    elapsed_=0;
    const bool moved=Separation(from,origin_)>.05f;
    if(!changed && !moved)return;
    const bool teleported=Separation(from,origin_)>8;
    origin_=from;destination_=goal;marked_=marked;
    const bool solidGoal=!map.CanStand(goal,.45f);
    if(changed || teleported) {
      endpoint_=goal;
      if(solidGoal && !marked) {
        // Reach a solid objective's near face inside its interaction radius.
        float best=std::numeric_limits<float>::max();
        for(float radius:{1.25f,1.8f})for(int i=0;i<16;++i) {
          const float angle=i*6.2831853f/16;
          const rpg::Vec2 offset{std::sin(angle),std::cos(angle)};
          const rpg::Vec2 candidate{goal.x+offset.x*radius,goal.z+offset.z*radius};
          const rpg::Vec2 face{goal.x+offset.x*.55f,goal.z+offset.z*.55f};
          const float score=Separation(from,candidate);
          if(score<best && map.CanStand(candidate,.45f) && map.CanTravel(candidate,face,.05f)) {
            best=score;endpoint_=candidate;
          }
        }
      }
    } else if(!solidGoal)endpoint_=goal;
    if(!map.CanStand(from,.45f) || !map.CanStand(endpoint_,.45f)) {
      points_.clear();distance_=0;state_=State::Unreachable;return;
    }
    if(Separation(from,endpoint_)<=(solidGoal?.35f:1.6f) && map.CanTravel(from,endpoint_,.45f)) {
      points_.clear();distance_=0;state_=State::Arrived;return;
    }
    // Retain a valid route while walking it; searches are for changed endpoints,
    // teleports or departure behind scenery, not for every rendered frame.
    if(changed || teleported || points_.empty() || !map.CanTravel(from,points_.front(),.45f)) {
      points_=map.FindPath(from,endpoint_,.45f);++searches_;
    } else if(Separation(points_.back(),endpoint_)>.01f) {
      const auto previous=points_.size()>1?points_[points_.size()-2]:from;
      if(map.CanTravel(previous,endpoint_,.45f))points_.back()=endpoint_;
      else {points_=map.FindPath(from,endpoint_,.45f);++searches_;}
    }
    if(points_.empty()) { state_=State::Unreachable;distance_=0;return; }
    // Skip passed corners only when the replacement segment has actor clearance.
    std::size_t first=0;
    for(std::size_t i=1;i<points_.size();++i)if(map.CanTravel(from,points_[i],.45f))first=i;
    if(first)points_.erase(points_.begin(),points_.begin()+first);
    distance_=0;auto previous=from;
    for(const auto point:points_) {
      // Follow hills along the segment instead of measuring its endpoint chord.
      const int steps=std::max(1,int(std::ceil(Separation(previous,point))));
      const auto start=previous;
      for(int i=1;i<=steps;++i) {
        const float t=float(i)/steps;
        const rpg::Vec2 next{start.x+(point.x-start.x)*t,start.z+(point.z-start.z)*t};
        distance_+=std::hypot(Separation(previous,next),rpg::WorldMap::Height(next)-rpg::WorldMap::Height(previous));
        previous=next;
      }
    }
    state_=State::Travelling;
  }
private:
  static bool Finite(rpg::Vec2 p){return std::isfinite(p.x) && std::isfinite(p.z);}
  State state_=State::None;
  rpg::Vec2 origin_{},destination_{},endpoint_{};
  bool marked_=false;
  float elapsed_=0,distance_=0;
  unsigned searches_=0;
  std::vector<rpg::Vec2> points_;
};
}
