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
  // Expansion props stay outside the legacy playable rectangle, preserving saves.
  obstacles_.push_back({{-24,6},2,2,3.4f,PropKind::Building,BuildingRole::WatchHall});
  obstacles_.push_back({{25,-1},2,2,3.1f,PropKind::Building,BuildingRole::Storehouse});
  obstacles_.push_back({{12,22},1.5f,1.5f,3.5f,PropKind::Building,BuildingRole::Homestead});
  obstacles_.push_back({{0,29},2,1,2.8f,PropKind::Ruin});
  for(const auto p:{Vec2{-26,-10},Vec2{-23,-15},Vec2{-25,11},Vec2{-17,24},
      Vec2{24,-13},Vec2{19,-17},Vec2{26,9},Vec2{18,18},Vec2{-11,26}})
    obstacles_.push_back({p,.35f,.35f,4.4f,PropKind::Tree});
  for(const auto p:surveySites) obstacles_.push_back({p,.35f,.25f,.7f,PropKind::SurveyStone});
  trails_={{{0,-17},{0,-10},1.3f,true},{{0,-10},{0,-3},1.25f,true},
    {{-4.7f,-11},{4.7f,-11},1.f,true},{{0,-3},{-1,3},1.1f,false},{{-1,3},{0,10.8f},1.1f,false},
    {{0,10.8f},astralAnchor,1.f,false},{{-.5f,0},{-8.8f,0},.85f,false},{{-8.8f,0},{-12.5f,-1},.85f,false},
    {{-.5f,0},{6,-2},.85f,false},{{6,-2},trainingPost,.9f,false},
    {{-12.5f,-1},{-20,-1},.9f,false},{{-20,-1},{-21,2},.9f,false},{{-21,2},{-20,12},.85f,false},
    {{-20,12},{-21,23},.85f,false},{{12,-2},{12.5f,0},.9f,false},{{12.5f,0},{19,0},.9f,false},{{19,0},{21,3},.9f,false},
    {{21,3},{20.5f,11},.85f,false},{{20.5f,11},{23,20},.85f,false},{{23,20},{16,23.5f},.85f,false},
    {{0,12.8f},{4,12.8f},.9f,false},{{4,12.8f},{8,16},.9f,false},{{8,16},{10,18},.9f,false},{{8,16},{3,22},.85f,false},
    {{3,22},{0,27},.9f,false},
    {{0,-17},{0,-22},.85f,false},{{0,-22},{-10,-25},.8f,false},
    {{-10,-25},{-19,-21},.8f,false},{{-19,-21},{-19,-16},.8f,false},
    {{0,-22},{10,-25},.8f,false},{{10,-25},{16,-22},.8f,false},
    {{16,-22},{16,-16},.8f,false}};
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
  // A separately bounded scatter keeps the original village cover intact.
  for(int z=-27;z<=27;z+=2) for(int x=-25;x<=25;x+=2) {
    if(std::abs(x)<17 && std::abs(z)<19) continue;
    const float seed=hash(x+91,z-53);if(seed<.48f || cover_.size()>=maxCover) continue;
    const Vec2 p{float(x)+(seed-.5f)*.5f,float(z)};
    bool service=false;
    for(const auto& npc:residents) service|=std::hypot(p.x-npc.position.x,p.z-npc.position.z)<2;
    for(const auto site:surveySites) service|=std::hypot(p.x-site.x,p.z-site.z)<1.5f;
    if(service || !CanStand(p,.8f) || SurfaceAt(p).dirt+SurfaceAt(p).paving>.06f) continue;
    const bool shrub=seed>.94f && shrubs<maxShrubs;if(shrub) ++shrubs;
    cover_.push_back({p,.7f+seed*.4f,seed*6.2831853f,shrub});
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

float WorldMap::GroundTraction(Vec2 p) const {
  if(!std::isfinite(p.x) || !std::isfinite(p.z)) return 1.f;
  const auto surface=SurfaceAt(p);
  const float ground=.80f+.10f*surface.dirt+.20f*surface.paving;
  // Blend the deck edge over a boot width so response cannot jump on a seam.
  const float outside=std::max({dockMin.x-p.x,p.x-dockMax.x,dockMin.z-p.z,p.z-dockMax.z});
  const float blend=std::clamp((.2f-outside)/.4f,0.f,1.f);
  const float weight=blend*blend*(3-2*blend);
  return ground+(.95f-ground)*weight;
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

Vec2 WorldMap::MoveAndSlide(Vec2 from,Vec2 displacement,float radius) const {
  if (!CanStand(from,radius) || !std::isfinite(displacement.x) || !std::isfinite(displacement.z)) return from;
  // Sweep a circular actor against rectangle faces and rounded corners. The
  // residual vector loses only its inward normal component: no axis priority.
  for (int iteration=0;iteration<4;++iteration) {
    const float lengthSq=displacement.x*displacement.x+displacement.z*displacement.z;
    if (!std::isfinite(lengthSq) || lengthSq<1e-12f) break;
    float hit=1; Vec2 normal{}; bool collided=false;
    const auto accept=[&](float t,Vec2 n) {
      if (t>=0 && t<=hit && displacement.x*n.x+displacement.z*n.z<-1e-7f) {
        hit=t; normal=n; collided=true;
      }
    };
    for (float side : {-1.f,1.f}) {
      if (side*displacement.x>0) accept((side*(halfWidth-radius)-from.x)/displacement.x,{-side,0});
      if (side*displacement.z>0) accept((side*(halfDepth-radius)-from.z)/displacement.z,{0,-side});
    }
    for (const auto& o : obstacles_) {
      const Vec2 end{from.x+displacement.x,from.z+displacement.z};
      if (std::max(from.x,end.x)<o.center.x-o.halfX-radius || std::min(from.x,end.x)>o.center.x+o.halfX+radius ||
          std::max(from.z,end.z)<o.center.z-o.halfZ-radius || std::min(from.z,end.z)>o.center.z+o.halfZ+radius) continue;
      for (float side : {-1.f,1.f}) {
        if (side*displacement.x<0) {
          const float t=(o.center.x+side*(o.halfX+radius)-from.x)/displacement.x;
          if (std::abs(from.z+displacement.z*t-o.center.z)<=o.halfZ) accept(t,{side,0});
        }
        if (side*displacement.z<0) {
          const float t=(o.center.z+side*(o.halfZ+radius)-from.z)/displacement.z;
          if (std::abs(from.x+displacement.x*t-o.center.x)<=o.halfX) accept(t,{0,side});
        }
      }
      for (float sx : {-1.f,1.f}) for (float sz : {-1.f,1.f}) {
        const Vec2 offset{from.x-o.center.x-sx*o.halfX,from.z-o.center.z-sz*o.halfZ};
        // Double precision protects the near root for long dash sweeps.
        const double b=double(offset.x)*displacement.x+double(offset.z)*displacement.z;
        const double c=double(offset.x)*offset.x+double(offset.z)*offset.z-double(radius)*radius;
        const double a=double(displacement.x)*displacement.x+double(displacement.z)*displacement.z;
        const double determinant=b*b-a*c;
        if (b>=0 || determinant<0) continue;
        const float t=float(c/(-b+std::sqrt(determinant)));
        if (t<0 || t>hit) continue;
        const Vec2 contact{offset.x+displacement.x*t,offset.z+displacement.z*t};
        if (contact.x*sx>=0 && contact.z*sz>=0) {
          const float length=std::hypot(contact.x,contact.z);
          if (length>0) accept(t,{contact.x/length,contact.z/length});
        }
      }
    }
    Vec2 next{from.x+displacement.x*hit,from.z+displacement.z*hit};
    if (collided) { next.x+=normal.x*.0001f; next.z+=normal.z*.0001f; }
    // Never publish a penetrating result after floating-point rounding or a
    // simultaneous contact. Unconsumed motion is dropped after four contacts.
    if (!CanStand(next,radius)) break;
    from=next;
    if (!collided) break;
    displacement.x*=1-hit; displacement.z*=1-hit;
    const float inward=std::min(0.f,displacement.x*normal.x+displacement.z*normal.z);
    displacement.x-=normal.x*inward; displacement.z-=normal.z*inward;
  }
  return from;
}

bool WorldMap::CanTravel(Vec2 from,Vec2 to,float radius) const {
  if (!CanStand(from,radius) || !CanStand(to,radius)) return false;
  const float distance=std::hypot(to.x-from.x,to.z-from.z);
  const int steps=std::max(1,static_cast<int>(std::ceil(distance/.1f)));
  // The boundary is convex and both endpoints are valid. Only props near the
  // segment can block an interior sample. Preserve the original sample lattice
  // and rounded-rectangle clearance; this is rejection, not coarser collision.
  const float minX=std::min(from.x,to.x),maxX=std::max(from.x,to.x);
  const float minZ=std::min(from.z,to.z),maxZ=std::max(from.z,to.z);
  for(const auto& o:obstacles_) {
    const float margin=radius+.0001f;
    if(maxX<o.center.x-o.halfX-margin || minX>o.center.x+o.halfX+margin ||
       maxZ<o.center.z-o.halfZ-margin || minZ>o.center.z+o.halfZ+margin) continue;
    for (int i=1;i<steps;++i) {
      const float t=static_cast<float>(i)/steps;
      const Vec2 p{from.x+(to.x-from.x)*t,from.z+(to.z-from.z)*t};
      const float dx=p.x-std::clamp(p.x,o.center.x-o.halfX,o.center.x+o.halfX);
      const float dz=p.z-std::clamp(p.z,o.center.z-o.halfZ,o.center.z+o.halfZ);
      if(dx*dx+dz*dz<radius*radius) return false;
    }
  }
  return true;
}

std::vector<Vec2> WorldMap::FindPath(Vec2 from,Vec2 to,float radius) const {
  if (!CanStand(from,radius) || !CanStand(to,radius)) return {};
  if (CanTravel(from,to,radius)) return {to};
  constexpr int columns=int(halfWidth)*2-1,rows=int(halfDepth)*2-1,total=columns*rows;
  const auto point=[=](int index) { return Vec2{float(index%columns)-(halfWidth-1),float(index/columns)-(halfDepth-1)}; };
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
  if(p.z<-19) return "THE WINDWORN MEADOW";
  if(p.x<-17) return "WESTWARD REACH";
  if(p.x>17) return "THE SURVEY COAST";
  if(p.z>19) return "THE SUNDERED BASTION";
  if (p.z<-4) return "DAWNWATCH VILLAGE";
  if (p.z>5) return "BROKEN WATCH CAMP";
  if (p.x<-5) return "ASTRAL DOCK";
  if (p.x>5) return "TRAINING GROUNDS";
  return "FRONTIER ROAD";
}
}
