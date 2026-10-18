#include "game/WorldMap.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>

namespace rpg {
WorldMap::WorldMap() {
  obstacles_ = {{{-8,-8},2.8f,2.5f,4.f},{{8,-8},2.8f,2.5f,4.f,PropKind::Building,BuildingRole::WatchHall},
    {{-10,3},1.3f,1.3f,3.f,PropKind::Building,BuildingRole::Storehouse},
    {{10,1},1.5f,1.5f,3.4f,PropKind::Building,BuildingRole::TrainingHall},{{0,15},2,1,2,PropKind::Ruin},
    {{3,-13.3f},1.1f,.4f,.85f,PropKind::Counter}};
  for (float x : {-14.f,14.f}) for (float z : {-14.f,-2.f,10.f})
    obstacles_.push_back({{x,z},.35f,.35f,3.9f,PropKind::Tree});
  trails_={{{0,-17},{0,-10},1.3f,true},{{0,-10},{0,-3},1.25f,true},
    {{-4.7f,-11},{4.7f,-11},1.f,true},{{0,-3},{-1,3},1.1f,false},{{-1,3},{0,10.8f},1.1f,false},
    {{0,10.8f},astralAnchor,1.f,false},{{-.5f,0},{-8.8f,0},.85f,false},{{-8.8f,0},{-12.5f,-1},.85f,false},
    {{-.5f,0},{6,-2},.85f,false},{{6,-2},trainingPost,.9f,false}};
  // Stable decoration, generated once. Keep routes, services and the arena clear.
  const auto hash=[](int x,int z) {
    std::uint32_t h=std::uint32_t(x)*374761393u+std::uint32_t(z)*668265263u;
    h=(h^(h>>13))*1274126177u; return float(h^(h>>16))/4294967295.f;
  };
  unsigned shrubs=0;
  for (int z=-14;z<=14;++z) for (int x=-12;x<=12;++x) {
    const float seed=hash(x,z);
    if (seed<.64f || cover_.size()>=180) continue;
    const Vec2 p{x*1.3f+(seed-.5f)*.6f,z*1.25f+(hash(z,x)-.5f)*.6f};
    const auto surface=SurfaceAt(p);
    if (!CanStand(p,.8f) || surface.dirt+surface.paving>.06f ||
      std::hypot(p.x-trainingPost.x,p.z-trainingPost.z)<4.3f ||
      std::hypot(p.x-questGiver.x,p.z-questGiver.z)<2.1f ||
      std::hypot(p.x-quartermaster.x,p.z-quartermaster.z)<2.1f ||
      std::hypot(p.x-astralAnchor.x,p.z-astralAnchor.z)<3.1f ||
      (p.x<dockMax.x+.6f && p.x>dockMin.x-.6f && p.z>dockMin.z-.6f && p.z<dockMax.z+.6f)) continue;
    const bool shrub=seed>.94f && shrubs<18;
    if (shrub) ++shrubs;
    cover_.push_back({p,.7f+hash(x+17,z-3)*.5f,seed*6.2831853f,shrub});
  }
}

GroundSurface WorldMap::SurfaceAt(Vec2 p) const {
  if (!std::isfinite(p.x) || !std::isfinite(p.z)) return {};
  const auto edge=[](float distance) {
    const float t=std::clamp((distance+.15f)/.65f,0.f,1.f); return 1-t*t*(3-2*t);
  };
  float dirt=0,paving=edge((std::hypot(p.x/4.f,(p.z+10)/2.2f)-1)*2.2f);
  for (const auto& trail : trails_) {
    const float dx=trail.to.x-trail.from.x,dz=trail.to.z-trail.from.z;
    const float t=std::clamp(((p.x-trail.from.x)*dx+(p.z-trail.from.z)*dz)/(dx*dx+dz*dz),0.f,1.f);
    const float weight=edge(std::hypot(p.x-trail.from.x-t*dx,p.z-trail.from.z-t*dz)-trail.halfWidth);
    if (trail.paved) paving=std::max(paving,weight); else dirt=std::max(dirt,weight);
  }
  dirt=std::max(dirt,edge(std::hypot(p.x-trainingPost.x,p.z-trainingPost.z)-2.8f));
  dirt=std::max(dirt,edge(std::hypot(p.x-astralAnchor.x,p.z-astralAnchor.z)-2.2f));
  return {dirt*(1-paving),paving};
}

bool WorldMap::CanStand(Vec2 p,float radius) const {
  if (!std::isfinite(p.x) || !std::isfinite(p.z) || !std::isfinite(radius) || radius<=0 ||
    std::abs(p.x)>halfWidth-radius || std::abs(p.z)>halfDepth-radius) return false;
  for (const auto& o : obstacles_) {
    const float dx=p.x-std::clamp(p.x,o.center.x-o.halfX,o.center.x+o.halfX);
    const float dz=p.z-std::clamp(p.z,o.center.z-o.halfZ,o.center.z+o.halfZ);
    if (dx*dx+dz*dz < radius*radius) return false;
  }
  return true;
}

bool WorldMap::CanTravel(Vec2 from,Vec2 to,float radius) const {
  if (!CanStand(from,radius) || !CanStand(to,radius)) return false;
  const float distance=std::hypot(to.x-from.x,to.z-from.z);
  const int steps=std::max(1,static_cast<int>(std::ceil(distance/.1f)));
  for (int i=1;i<steps;++i) {
    const float t=static_cast<float>(i)/steps;
    if (!CanStand({from.x+(to.x-from.x)*t,from.z+(to.z-from.z)*t},radius)) return false;
  }
  return true;
}

std::vector<Vec2> WorldMap::FindPath(Vec2 from,Vec2 to,float radius) const {
  if (!CanStand(from,radius) || !CanStand(to,radius)) return {};
  if (CanTravel(from,to,radius)) return {to};
  constexpr int columns=33, rows=37, total=columns*rows;
  const auto point=[=](int index) { return Vec2{float(index%columns-16),float(index/columns-18)}; };
  // Anchor to a visible free grid point rather than rounding into an obstacle.
  const auto anchor=[&](Vec2 p) {
    int best=-1; float nearest=std::numeric_limits<float>::max();
    for (int i=0;i<total;++i) {
      const Vec2 candidate=point(i);
      const float distance=std::hypot(candidate.x-p.x,candidate.z-p.z);
      if (distance<nearest && CanTravel(p,candidate,radius)) { nearest=distance; best=i; }
    }
    return best;
  };
  const int start=anchor(from), goal=anchor(to);
  if (start<0 || goal<0) return {};
  struct Visit { float score; int index; };
  struct Later { bool operator()(Visit a,Visit b) const {
    return a.score==b.score?a.index>b.index:a.score>b.score;
  }};
  std::priority_queue<Visit,std::vector<Visit>,Later> open;
  std::array<float,total> costs; costs.fill(std::numeric_limits<float>::max());
  std::array<int,total> previous; previous.fill(-1);
  std::array<bool,total> closed{};
  costs[start]=0; open.push({0,start});
  while (!open.empty()) {
    const int current=open.top().index; open.pop();
    if (closed[current]) continue;
    closed[current]=true;
    if (current==goal) break;
    const int x=current%columns,z=current/columns;
    for (int dz=-1;dz<=1;++dz) for (int dx=-1;dx<=1;++dx) {
      if ((dx==0 && dz==0) || x+dx<0 || x+dx>=columns || z+dz<0 || z+dz>=rows) continue;
      const int next=(z+dz)*columns+x+dx;
      if (closed[next] || !CanTravel(point(current),point(next),radius)) continue;
      const float nextCost=costs[current]+(dx!=0 && dz!=0?1.41421356f:1.f);
      if (nextCost>=costs[next]) continue;
      costs[next]=nextCost; previous[next]=current;
      const Vec2 p=point(next), g=point(goal);
      open.push({nextCost+std::hypot(p.x-g.x,p.z-g.z),next});
    }
  }
  if (!closed[goal]) return {};
  std::vector<Vec2> route;
  for (int index=goal;index!=-1;index=previous[index]) route.push_back(point(index));
  std::reverse(route.begin(),route.end()); route.push_back(to);
  // Greedy line-of-sight smoothing; every returned segment retains full clearance.
  std::vector<Vec2> result;
  Vec2 last=from;
  for (std::size_t i=0;i<route.size();) {
    std::size_t furthest=i;
    for (std::size_t j=i;j<route.size();++j)
      if (CanTravel(last,route[j],radius)) furthest=j;
    result.push_back(route[furthest]); last=route[furthest]; i=furthest+1;
  }
  return result;
}

const char* WorldMap::AreaName(Vec2 p) const {
  if (p.z<-4) return "DAWNWATCH VILLAGE";
  if (p.z>5) return "BROKEN WATCH CAMP";
  if (p.x<-5) return "ASTRAL DOCK";
  if (p.x>5) return "TRAINING GROUNDS";
  return "FRONTIER ROAD";
}
}
