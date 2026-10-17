#include "game/WorldMap.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

void Require(bool value,const char* message) {
  if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
void CheckRoute(const rpg::WorldMap& map,rpg::Vec2 start,rpg::Vec2 goal) {
  const auto route=map.FindPath(start,goal);
  Require(!route.empty(),"Expected a navigable route");
  rpg::Vec2 previous=start;
  float length=0;
  for (const auto& waypoint : route) {
    Require(map.CanTravel(previous,waypoint,.45f),"Route must preserve actor clearance on every segment");
    length+=std::hypot(waypoint.x-previous.x,waypoint.z-previous.z);
    previous=waypoint;
  }
  Require(std::hypot(previous.x-goal.x,previous.z-goal.z)<.001f,"Route must reach requested goal");
  Require(length<100,"Route must not wander indefinitely");
}
int main() {
  rpg::WorldMap map;
  Require(!map.CanTravel({-12,-8},{-4,-8},.45f),"Direct route through house must fail");
  CheckRoute(map,{-12,-8},{-4,-8});
  CheckRoute(map,{4,-8},{12,-8});
  CheckRoute(map,{0,12},{0,18});
  CheckRoute(map,{0,-10},{-15,3});
  CheckRoute(map,{-13,10},{15,10});
  Require(map.FindPath({-8,-8},{0,0}).empty(),"Blocked starting point must not create a route");
  Require(map.FindPath({0,0},{40,0}).empty(),"Outside goal must not create a route");
  Require(!map.CanStand({std::numeric_limits<float>::quiet_NaN(),0},.4f),"NaN coordinates must be rejected");
  Require(!map.CanStand({14,-2},.4f),"Visible tree trunk must be solid");
  std::cout << "Navigation checks passed.\n";
}
