#include "game/NavigationRoute.h"
#include <iostream>
#include <stdexcept>
#include <limits>

namespace {
using namespace rpg;
using Route=ui::NavigationRoute;
void Check(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
void Validate(const Route& route,const WorldMap& map,Vec2 from) {
  if(route.Status()!=Route::State::Travelling)std::cerr<<"Route "<<from.x<<','<<from.z<<" -> "<<route.Destination().x<<','<<route.Destination().z<<" status "<<int(route.Status())<<'\n';
  Check(route.Status()==Route::State::Travelling,"Expected a walkable route");
  float flat=0;
  for(auto p:route.Points()) {
    Check(map.CanTravel(from,p,.45f),"Route clips a solid footprint");
    flat+=Route::Separation(from,p);from=p;
  }
  Check(route.Distance()+.01f>=flat,"Walking distance ignores ground distance");
  Check(Route::Separation(from,route.Endpoint())<.01f,"Route does not reach destination approach");
}
void Routing() {
  WorldMap map;Route route;const auto start=WorldMap::spawn;
  for(auto goal:{WorldMap::residents[6].position,WorldMap::residents[7].position,
    WorldMap::residents[8].position,WorldMap::expeditionSites[6],WorldMap::courtSeals[2]}) {
    route.Update(.25f,map,start,goal,true);Validate(route,map,start);
  }
  // Find an actual building crossing, so the detour assertion cannot accidentally
  // pass on an unobstructed line in a changing location layout.
  bool checked=false;
  for(const auto& prop:map.Obstacles())if(prop.kind==PropKind::Building) {
    const Vec2 from{prop.center.x-prop.halfX-1,prop.center.z},to{prop.center.x+prop.halfX+1,prop.center.z};
    if(!map.CanStand(from,.45f) || !map.CanStand(to,.45f))continue;
    route.Reset();route.Update(0,map,from,to,true);if(route.Points().empty())continue;
    Validate(route,map,from);
    Check(route.Points().size()>1 && route.Distance()>Route::Separation(from,to)+.5f,"Building was not routed around");
    const auto searches=route.Searches();for(int i=0;i<600;++i)route.Update(1.f/60,map,from,to,true);
    Check(route.Searches()==searches,"Stationary map redraw repeatedly searches");
    route.Update(.25f,map,to,to,true);Check(route.Status()==Route::State::Arrived && route.Points().empty(),"Arrival retains obsolete trail");
    route.Update(.25f,map,from,prop.center,true,true);Check(route.Status()==Route::State::Unreachable && route.Points().empty(),"Blocked pin retained previous route");
    route.Update(.25f,map,from,to,true);Validate(route,map,from);
    route.Update(.25f,map,from,to,false);Check(route.Status()==Route::State::None,"Disabled guidance remains visible");
    route.Update(.25f,map,from,to,true);
    route.Update(.1f,map,{std::numeric_limits<float>::quiet_NaN(),0},to,true);
    Check(route.Status()==Route::State::None,"Invalid input reaches navigation search");
    checked=true;break;
  }
  Check(checked,"No actual building detour exercised");
  route.Update(0,map,start,{0,-9},true,true);
  route.Update(0,map,start,{0,-8.9f},true,true);
  Check(std::abs(route.Destination().z+8.9f)<.001f,"Small manual pin change was ignored while stationary");
}
void Walking() {
  for(int hz:{30,144}) {
    Simulation s;Route route;const auto destination=WorldMap::residents[6].position;
    const auto before=s.Save();route.Update(0,s,true,destination);
    Check(s.Save()==before,"Guidance modifies game state");
    const float initial=route.Distance();int frame=0;
    for(;frame<hz*120 && route.Status()!=Route::State::Arrived;++frame) {
      Check(!route.Points().empty(),"Walking lost its route");
      const auto next=route.Points().front(),p=s.Player();const float d=Route::Separation(next,p);
      Command command;const float scale=std::min(1.f,d*1.2f)/std::max(d,.001f);
      command.movement={(next.x-p.x)*scale,(next.z-p.z)*scale};
      s.Update(1.f/hz,command);route.Update(1.f/hz,s,true,destination);
    }
    Check(frame<hz*120 && Route::Separation(s.Player(),destination)<1.7f,"Real on-foot caravan approach failed");
    Check(initial>70 && route.Distance()==0,"Distance did not count down to arrival");
    Check(route.Searches()<15,"Walking guidance unnecessarily reruns pathfinding");
    route.Update(.25f,s,false,{});
    Check(Route::Separation(route.Destination(),s.ObjectivePosition())<.01f,"Cleared waypoint does not resume objective");
    const auto saved=s.Save();Check(s.Load(saved),"Walking save rejected");route.Reset();route.Update(0,s,false,{});
    Check(route.Status()!=Route::State::None,"Load reset cannot rebuild guidance");
  }
}
}
int main()try{Routing();Walking();std::cout<<"PASS: navigation clearance, lifecycle, cache and real caravan approach at 30/144 Hz\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
