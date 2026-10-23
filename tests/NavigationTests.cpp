#include "game/Simulation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <chrono>
#include <string>
#include <array>
#include <algorithm>
#include <vector>

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
// Original sampled implementation retained as an independent equivalence oracle.
bool ReferenceTravel(const rpg::WorldMap& map,rpg::Vec2 from,rpg::Vec2 to,float radius) {
  if(!map.CanStand(from,radius) || !map.CanStand(to,radius)) return false;
  const int steps=std::max(1,int(std::ceil(std::hypot(to.x-from.x,to.z-from.z)/.1f)));
  for(int i=1;i<steps;++i) {
    const float t=float(i)/steps;
    if(!map.CanStand({from.x+(to.x-from.x)*t,from.z+(to.z-from.z)*t},radius)) return false;
  }
  return true;
}
void TravelChecks(const rpg::WorldMap& map,bool benchmark) {
  struct Query {rpg::Vec2 from,to;float radius;};std::vector<Query> queries;
  unsigned seed=12345;
  const auto coordinate=[&]() {seed=seed*1664525u+1013904223u;return float((seed>>8)%6200)/100-31;};
  for(int i=0;i<4096;++i) queries.push_back({{coordinate(),coordinate()},{coordinate(),coordinate()},std::array<float,4>{.05f,.4f,.45f,1.f}[i%4]});
  if(!benchmark) {
    for(const auto& q:queries) Require(map.CanTravel(q.from,q.to,q.radius)==ReferenceTravel(map,q.from,q.to,q.radius),"Travel optimization changed clearance");
    for(const auto& o:map.Obstacles()) for(float radius:{.05f,.4f,.45f,1.f}) for(float epsilon:{-.0001f,0.f,.0001f}) {
      const float x=o.center.x-o.halfX-radius+epsilon;
      const rpg::Vec2 from{x,o.center.z-o.halfZ-1},to{x,o.center.z+o.halfZ+1};
      Require(map.CanTravel(from,to,radius)==ReferenceTravel(map,from,to,radius),"Broad-phase rejection changed tangential clearance");
      Require(map.CanTravel(to,from,radius)==ReferenceTravel(map,to,from,radius),"Reverse travel changed tangential clearance");
    }
    return;
  }
  unsigned checksum=0;const auto start=std::chrono::steady_clock::now();
  for(int repeat=0;repeat<8;++repeat) for(const auto& q:queries) checksum+=map.CanTravel(q.from,q.to,q.radius);
  const auto travel=std::chrono::steady_clock::now();
  for(int repeat=0;repeat<100;++repeat) for(const auto goal:{rpg::Vec2{-21,2},rpg::Vec2{21,3},rpg::Vec2{10,18},rpg::Vec2{0,27}})
    checksum+=unsigned(map.FindPath({0,-10},goal).size());
  const auto end=std::chrono::steady_clock::now();
  std::cout<<"32768 travel queries ms: "<<std::chrono::duration<double,std::milli>(travel-start).count()
    <<"; 400 paths ms: "<<std::chrono::duration<double,std::milli>(end-travel).count()<<"; checksum: "<<checksum<<'\n';
}
int main(int argc,char** argv) {
  rpg::WorldMap map;
  if(argc==2 && std::string(argv[1])=="--benchmark") {TravelChecks(map,true);return 0;}
  TravelChecks(map,false);
  Require(std::string(map.AreaName({-21,23}))=="WESTWARD REACH" &&
    std::string(map.AreaName({26,22}))=="THE SURVEY COAST" &&
    std::string(map.AreaName({0,27}))=="THE SUNDERED BASTION","Expanded encounters must retain their region names.");
  const auto wall=map.MoveAndSlide({4,-8},{3,1},.4f);
  Require(std::abs(wall.x-4.8f)<.0002f && std::abs(wall.z+7)<.0002f,
    "Swept collision must reach the wall and preserve tangential movement");
  const auto dash=map.MoveAndSlide({0,-8},{15,0},.4f);
  Require(std::abs(dash.x-4.8f)<.0002f,"Long displacement must not tunnel through a building");
  const auto trunk=map.MoveAndSlide({12,-2},{4,0},.4f);
  Require(std::abs(trunk.x-13.25f)<.0002f,"Long displacement must not tunnel through a thin trunk");
  const auto edge=map.MoveAndSlide({rpg::WorldMap::halfWidth-1,rpg::WorldMap::halfDepth-2},{4,4},.4f);
  Require(map.CanStand(edge,.4f) && std::abs(edge.x-(rpg::WorldMap::halfWidth-.4f))<.0002f && std::abs(edge.z-(rpg::WorldMap::halfDepth-.4f))<.0002f,
    "Simultaneous map-edge contacts must slide safely into the corner");
  for (int hz : {30,60,144}) {
    rpg::Vec2 p{4,-8};
    for (int i=0;i<hz;++i) { p=map.MoveAndSlide(p,{3.f/hz,1.f/hz},.4f); Require(map.CanStand(p,.4f),"Wall slide penetrated"); }
    Require(std::hypot(p.x-wall.x,p.z-wall.z)<.001f,"Wall contact and sliding must agree across update rates");
  }
  for (float offset : {-.2f,0.f,.2f}) {
    const auto right=map.MoveAndSlide({4,-11.5f+offset},{2,2},.4f);
    const auto left=map.MoveAndSlide({-4,-11.5f+offset},{-2,2},.4f);
    Require(map.CanStand(right,.4f) && map.CanStand(left,.4f),"Rounded corner sweep penetrated");
    Require(std::abs(right.x+left.x)<.001f && std::abs(right.z-left.z)<.001f,
      "Corner response must not favor the X direction or one side of the map");
  }
  const auto away=map.MoveAndSlide(wall,{-1,0},.4f);
  Require(away.x<wall.x-.99f,"Moving away from contact must not stick");
  const auto invalid=map.MoveAndSlide({0,-10},{std::numeric_limits<float>::quiet_NaN(),1},.4f);
  Require(invalid.x==0 && invalid.z==-10,"Invalid displacement must leave the actor in place");
  unsigned sweeps=0;
  for (int x=1-int(rpg::WorldMap::halfWidth);x<rpg::WorldMap::halfWidth;x+=2)
    for (int z=1-int(rpg::WorldMap::halfDepth);z<rpg::WorldMap::halfDepth;z+=2) {
    const rpg::Vec2 start{float(x),float(z)};
    if (!map.CanStand(start,.4f)) continue;
    for (int angle=0;angle<16;++angle) {
      const rpg::Vec2 delta{std::cos(angle*.39269908f)*3.5f,std::sin(angle*.39269908f)*3.5f};
      const auto end=map.MoveAndSlide(start,delta,.4f);
      Require(map.CanStand(end,.4f),"Swept movement must always finish outside static obstacles");
      Require(std::hypot(end.x-start.x,end.z-start.z)<=3.5005f,"Sliding must not add movement energy");
      ++sweeps;
    }
  }
  std::cout<<"Swept collision safety cases: "<<sweeps<<'\n';
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
  Require(!map.CanTravel({3,-15},{3,-11},.45f),"The visible shop counter must block a direct route.");
  CheckRoute(map,{3,-15},{3,-11});

  for (const auto& trail : map.Trails()) {
    if(!map.CanTravel(trail.from,trail.to,.45f)) {
      std::cerr<<"Blocked trail: "<<trail.from.x<<','<<trail.from.z<<" -> "<<trail.to.x<<','<<trail.to.z<<'\n';
      Require(false,"A visible route must be traversable by the player.");
    }
  }
  Require(std::string(map.AreaName({0,-24}))=="THE WINDWORN MEADOW","Southern landscape lost its region label");
  CheckRoute(map,rpg::WorldMap::spawn,{-19,-21});
  CheckRoute(map,rpg::WorldMap::spawn,{16,-22});
  Require(map.SurfaceAt({0,-22}).dirt>.9f,"Southern route does not reach the landscape surface");
  for (float z=rpg::WorldMap::dockMin.z;z<=rpg::WorldMap::dockMax.z;z+=.1f)
    for (float x=rpg::WorldMap::dockMin.x;x<=rpg::WorldMap::dockMax.x;x+=.1f)
      Require(map.CanStand({x,z},.1f),"The dock must not overlap a tree trunk or building footprint.");
  Require(map.SurfaceAt({0,-10}).paving>.99f && map.SurfaceAt({0,-5}).paving>.99f,
    "The plaza and village street must retain their paved surface.");
  for (auto p : {rpg::Vec2{-11,-.65f},rpg::WorldMap::trainingPost,rpg::WorldMap::astralAnchor})
    Require(map.SurfaceAt(p).dirt>.99f,"Dock, training and anchor routes must be visible on the ground.");
  Require(map.SurfaceAt({-15,16}).dirt+map.SurfaceAt({-15,16}).paving==0,"The outer meadow must remain grass.");
  for (int z=-38;z<=38;++z) for (int x=-34;x<=34;++x) {
    const auto surface=map.SurfaceAt({x*.5f,z*.5f});
    Require(std::isfinite(surface.dirt) && std::isfinite(surface.paving) && surface.dirt>=0 && surface.paving>=0 &&
      surface.dirt+surface.paving<=1.00001f,"Ground weights must remain finite, nonnegative and normalized.");
  }
  for (float invalid : {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
    const auto surface=map.SurfaceAt({invalid,0});
    Require(surface.dirt==0 && surface.paving==0,"Invalid positions must not poison the ground texture.");
  }
  const rpg::WorldMap repeat;
  Require(!map.Cover().empty() && map.Cover().size()<=rpg::WorldMap::maxCover && map.Cover().size()==repeat.Cover().size(),
    "Ground cover must have a bounded, repeatable allocation.");
  unsigned shrubs=0;
  for (std::size_t i=0;i<map.Cover().size();++i) {
    const auto& cover=map.Cover()[i]; const auto& same=repeat.Cover()[i];
    Require(cover.position.x==same.position.x && cover.position.z==same.position.z && cover.scale==same.scale &&
      cover.yaw==same.yaw && cover.shrub==same.shrub,"Rebuilding a world must not move vegetation.");
    Require(map.CanStand(cover.position,.8f),"Ground cover must clear solid props and the island boundary.");
    const auto surface=map.SurfaceAt(cover.position);
    Require(surface.dirt+surface.paving<=.06f,"Ground cover must stay off visible roads.");
    for (auto point : {rpg::WorldMap::questGiver,rpg::WorldMap::quartermaster,rpg::WorldMap::astralAnchor})
      Require(std::hypot(cover.position.x-point.x,cover.position.z-point.z)>2,
        "Ground cover must not obscure an interaction point.");
    Require(std::hypot(cover.position.x-rpg::WorldMap::trainingPost.x,cover.position.z-rpg::WorldMap::trainingPost.z)>4,
      "The sparring arena must stay clear.");
    if (cover.shrub) ++shrubs;
  }
  Require(shrubs>0 && shrubs<=rpg::WorldMap::maxShrubs,"Shrub population must stay inside the environment budget.");
  std::cout << "Ground cover: " << map.Cover().size()-shrubs << " grass clumps, " << shrubs << " shrubs.\n";

  rpg::Simulation simulation;
  for (float pitch : {-.15f,-.4f,-.75f}) {
    const float range=simulation.CameraDistance({0,-10},1.4f,{0,-std::cos(pitch)},std::sin(pitch),18);
    Require(range>0 && range<18 && 1.4f+std::sin(pitch)*range>=.2999f,
      "Upward-looking cameras must retract above the floor before rendering.");
  }
  Require(simulation.CameraDistance({3,-15},2.65f,{0,1},0,3)<2,
    "An orbiting camera must stop before entering the supply canopy.");
  Require(simulation.CameraDistance({3,-15},.5f,{0,1},0,3)<2,
    "The stall counter must still stop a low camera.");
  Require(simulation.CameraDistance({3,-15},1.6f,{0,1},0,3)==3,
    "The open space above the counter must not collapse the camera distance.");
  Require(simulation.CameraDistance({1.96f,-15},1.6f,{0,1},0,3)<2,
    "The stall uprights must stop the camera even in the open bay.");
  Require(simulation.CameraDistance({3,-15},2.95f,{0,1},0,3)==3,
    "A camera above the cloth must not collide with an empty bounding box.");
  Require(simulation.CameraDistance({3,-10},1.4f,{0,-std::cos(.45f)},std::sin(.45f),10)==10,
    "The normal shop camera must clear the awning instead of collapsing into the player.");
  Require(simulation.CameraDistance({8,-13},5.2f,{0,1},0,4)<2.5f,
    "The watch hall's decorated gable must remain inside the camera envelope.");
  const std::string returning="MEDITERRANEAN 3\n0 -10 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 5 80 0 0 2 0 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(simulation.Load(returning),"Returning enemy fixture must load");
  const float initialFacing=simulation.Enemies()[0].facing;
  simulation.Update(.05f,{});
  Require(std::abs(std::remainder(simulation.Enemies()[0].facing-initialFacing,6.2831853f))<=.5001f &&
    simulation.Enemies()[0].movementDistance<.001f,"A returning enemy must turn before walking backward");
  for (int i=0;i<7;++i) simulation.Update(.05f,{});
  const auto before=simulation.Enemies()[0].position;
  simulation.Update(.05f,{});
  const auto& enemy=simulation.Enemies()[0];
  const float dx=enemy.position.x-before.x,dz=enemy.position.z-before.z;
  const float distance=std::hypot(dx,dz);
  Require(distance>.01f && enemy.state==rpg::EnemyState::Returning,"Return fixture must actually move home");
  Require((std::sin(enemy.facing)*dx+std::cos(enemy.facing)*dz)/distance>.99f,
    "Returning enemies must face their travel direction instead of walking backward");
  Require(std::abs(enemy.movementDistance-distance)<.0001f,"Movement feedback must match actual navigation travel");
  const auto movingSave=simulation.Save();
  rpg::Simulation loaded;
  Require(loaded.Load(movingSave) && loaded.Save()==movingSave && loaded.Enemies()[0].movementDistance==0,
    "Loading must retain durable state and reset transient movement feedback");
  for (int i=0;i<60;++i) simulation.Update(.05f,{});
  Require(simulation.Enemies()[0].state==rpg::EnemyState::Idle && simulation.Enemies()[0].movementDistance==0,
    "Arriving home must stop the walking pose");

  const std::string recovering="MEDITERRANEAN 3\n-4 5.5 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
    "100 -4 7 80 1 0 1 0 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
  Require(simulation.Load(recovering),"Stationary recovery fixture must load");
  simulation.Update(.05f,{});
  Require(!simulation.Enemies()[0].path.empty() && simulation.Enemies()[0].movementDistance==0,
    "A retained navigation path must not imply walking while the enemy is standing in melee range");
  for (int i=0;i<20;++i) simulation.Update(.05f,{});
  Require(simulation.Enemies()[0].windup>0 && simulation.Enemies()[0].movementDistance==0,
    "A committed windup must keep the feet stationary");
  std::cout << "Navigation checks passed.\n";
}
