#include "game/WorldMap.h"
#include "game/Population.h"
#include "game/Grove.h"
#include <algorithm>
#include <cmath>

namespace rpg {
namespace {
float Hash(int x,int z) {
  std::uint32_t h=std::uint32_t(x)*374761393u+std::uint32_t(z)*668265263u;
  h=(h^(h>>13))*1274126177u;return float(h^(h>>16))/4294967295.f;
}
float Distance(Vec2 a,Vec2 b) {return std::hypot(a.x-b.x,a.z-b.z);}
// Open encounter clearings and service approaches are intentional negative
// space. Forests frame their edges; cover must not hide a warning or a speaker.
bool BusyGround(Vec2 p,float margin) {
  for(const auto& n:WorldMap::residents)if(Distance(p,n.position)<3+margin)return true;
  for(auto q:{WorldMap::questGiver,WorldMap::quartermaster,WorldMap::astralAnchor})if(Distance(p,q)<3+margin)return true;
  if(Distance(p,{12,1})<6+margin || Distance(p,WorldMap::trainingPost)<4.3f+std::max(0.f,margin) ||
    Distance(p,WorldMap::ritualSite)<12+margin)return true;
  for(const auto& q:WorldMap::surveySites)if(Distance(p,q)<2+margin)return true;
  for(const auto& q:WorldMap::expeditionSites)if(Distance(p,q)<4+margin)return true;
  for(const auto& q:WorldMap::courtSeals)if(Distance(p,q)<4+margin)return true;
  for(const auto& d:population::districts)if(Distance(p,d.habitat)<10+margin)return true;
  constexpr float arenas[][4]={{0,15,10,17},{-22,19,8,10},{-45,42,16,17},
    {45,50,17,17},{-2,66,24,11},{-38,-102,23,15},{87,-108,30,18}};
  for(const auto& a:arenas)if(std::abs(p.x-a[0])<a[2]+margin && std::abs(p.z-a[1])<a[3]+margin)return true;
  return p.x>WorldMap::dockMin.x-1-margin && p.x<WorldMap::dockMax.x+1+margin &&
    p.z>WorldMap::dockMin.z-1-margin && p.z<WorldMap::dockMax.z+1+margin;
}
float RoadDistance(const WorldMap& map,Vec2 p) {
  float distance=1000;
  for(const auto& t:map.Trails()) {
    const float x=t.to.x-t.from.x,z=t.to.z-t.from.z;
    const float f=std::clamp(((p.x-t.from.x)*x+(p.z-t.from.z)*z)/std::max(.001f,x*x+z*z),0.f,1.f);
    distance=std::min(distance,Distance(p,{t.from.x+x*f,t.from.z+z*f})-t.halfWidth);
  }
  return distance;
}
}
void WorldMap::BuildLocation() {
  // Existing source meshes are instanced; no Blender source/export is changed.
  // Preserve old trunk collision while giving established trees real crowns.
  for(auto& o:obstacles_)if(o.kind==PropKind::Tree) {o.height*=1.28f;o.canopyRadius=2.35f;}
  constexpr Vec2 groves[]={{-17,8},{-14,23},{-31,10},{30,13},{-25,-24},{24,-24},
    {-56,7},{-66,30},{-38,63},{40,13},{64,40},{-25,81},{30,77},{-12,40},{20,45},{-28,48},
    {-105,-86},{-65,-112},{55,-83},{106,-66},{-105,86},{74,105}};
  unsigned trees=0;
  for(int n=0;n<18 && trees<160;++n)for(unsigned grove=0;grove<std::size(groves) && trees<160;++grove) {
    const float seed=Hash(int(grove)+81,n+37),angle=n*2.399963f+grove;
    const float distance=1.8f+std::sqrt(float(n))*1.6f;
    const Vec2 p{groves[grove].x+std::sin(angle)*distance,groves[grove].z+std::cos(angle)*distance};
    if(BusyGround(p,1.6f) || RoadDistance(*this,p)<2.3f || !CanStand(p,2.0f))continue;
    obstacles_.push_back({p,.38f,.38f,6.6f+seed*3.8f,PropKind::Tree,BuildingRole::Homestead,sceneryVersion,3.1f+seed*.8f});
    ++trees;
  }
  for(auto& o:obstacles_)if(o.kind==PropKind::Tree)grove::FitCamera(o);
  const auto place=[&](LandscapeKind kind,Vec2 p,float scale,float yaw) {
    Vec2 half{.39f,.39f};float height=1.02f;
    switch(kind) {
    case LandscapeKind::Boulder:half={.9f,.7f};height=.85f;break;
    // Conservative export bounds include cart handles, well roof and all LODs.
    case LandscapeKind::Crate:half={.47f,.44f};height=.75f;break;
    case LandscapeKind::Cart:half={.96f,2.03f};height=1.2f;break;
    case LandscapeKind::Well:half={1.20f,.80f};height=2.6f;break;
    case LandscapeKind::Bench:half={.93f,.33f};height=.52f;break;
    case LandscapeKind::Logs:half={.61f,.61f};height=.67f;break;
    case LandscapeKind::Fence:half={1.11f,.12f};height=1.21f;break;
    case LandscapeKind::Sacks:half={.54f,.41f};height=.70f;break;
    case LandscapeKind::Stump:half={.63f,.56f};height=.61f;break;
    case LandscapeKind::Lantern:half={};break;
    default:break;
    }
    const float c=std::abs(std::cos(yaw)),s=std::abs(std::sin(yaw));
    const Vec2 bounds{(half.x*c+half.z*s)*scale,(half.x*s+half.z*c)*scale};
    const float radius=std::hypot(bounds.x,bounds.z);
    if(BusyGround(p,.2f) || RoadDistance(*this,p)<radius+.5f || !CanStand(p,radius+.3f))return;
    // Avoid perching rigid authored props on a hillside with a floating base.
    if(kind!=LandscapeKind::Boulder && terrain::Slope(p.x,p.z)>.18f)return;
    details_.push_back({kind,p,scale,yaw});
    if(radius>0)obstacles_.push_back({p,bounds.x,bounds.z,height*scale,PropKind::Dressing,BuildingRole::Homestead,sceneryVersion});
  };
  // Store yards, woodcutters' stacks and small resting places flank the village.
  // Each grouping uses actual authored objects, with open road-side entrances.
  constexpr Vec2 yards[]={{-8,-17},{8,-18},{-19,-10},{20,-9},{-27,1},{29,-5},
    {-48,12},{50,24},{-17,50},{21,-66},{-44,-87},{-80,-73},{-7,-21},{7,-23},{-30,3},{28,-11}};
  for(unsigned i=0;i<std::size(yards);++i) {
    const auto p=yards[i];const float yaw=float(i%4)*1.570796f;
    place(i%3==0?LandscapeKind::Cart:i%3==1?LandscapeKind::Logs:LandscapeKind::Bench,p,.85f,yaw);
    place(LandscapeKind::Barrel,{p.x+2,p.z},.9f,.2f+i);
    place(LandscapeKind::Crate,{p.x+2.3f,p.z-1.1f},.85f,.13f*i);
    place(LandscapeKind::Sacks,{p.x-1.7f,p.z-1.1f},.8f,yaw);
    place(LandscapeKind::Lantern,{p.x-1.5f,p.z+1.8f},.8f,yaw);
    place(LandscapeKind::Fence,{p.x,p.z-2.8f},.85f,.07f);
    place(LandscapeKind::Fence,{p.x+1.8f,p.z-2.8f},.85f,-.06f);
  }
  place(LandscapeKind::Well,{-17,-6},.9f,0);
  // Weathered stone shelves sit along the banks, never as a regular dotted
  // border on every road. Each substantial stone has the same physical bounds.
  for(unsigned grove=0;grove<std::size(groves);++grove)for(int n=0;n<5;++n) {
    const float seed=Hash(int(grove)+17,n+32),a=n*2.399963f;
    const Vec2 p{groves[grove].x+std::sin(a)*(4+n),groves[grove].z+std::cos(a)*(4+n)};
    place(LandscapeKind::Boulder,p,.8f+seed*.65f,a);
    if(n==2)place(LandscapeKind::Stump,{p.x+1.6f,p.z-1},.85f,a);
  }
}
void WorldMap::BuildGroundCover() {
  cover_.clear();unsigned shrubs=0;
  const auto accept=[&](Vec2 p,float seed,bool woodland) {
    if(cover_.size()>=maxCover || !CanStand(p,.8f) || BusyGround(p,-1.f))return;
    const auto surface=SurfaceAt(p);
    if(surface.dirt+surface.paving>.04f || terrain::Slope(p.x,p.z)>.65f)return;
    const bool shrub=woodland && seed>.46f && shrubs<maxShrubs;
    shrubs+=shrub;
    cover_.push_back({p,.55f+seed*.65f,seed*6.2831853f,shrub});
  };
  // Prioritize the near village and forest understory before distant heath.
  // Clusters share coarse density, with stable jitter instead of a visible grid.
  for(int z=-34;z<=34;++z)for(int x=-34;x<=34;++x) {
    const float seed=Hash(x+13,z-21),patch=Hash(int(std::floor(x/5.f)),int(std::floor(z/5.f)));
    if(seed<.32f+patch*.45f)continue;
    accept({x+(seed-.5f)*.85f,z+(Hash(z,x)-.5f)*.85f},seed,patch>.5f);
  }
  for(const auto& o:obstacles_)if(o.kind==PropKind::Tree)for(int n=0;n<4;++n) {
    const float seed=Hash(int(o.center.x*9)+n,int(o.center.z*7)-n),a=seed*6.2831853f;
    accept({o.center.x+std::cos(a)*(1.1f+n*.4f),o.center.z+std::sin(a)*(1.1f+n*.4f)},seed,true);
  }
  for(int z=-124;z<=124;z+=4)for(int x=-116;x<=116;x+=4) {
    if(std::abs(x)<35 && std::abs(z)<35)continue;
    const float seed=Hash(x+91,z-53);if(seed<.6f)continue;
    accept({x+(seed-.5f)*2,z+(Hash(z,x)-.5f)*2},seed,false);
  }
}
void WorldMap::RecoverOldSceneryPosition(Vec2& position,float radius,int version) const {
  if(!std::isfinite(position.x) || !std::isfinite(position.z) || CanStand(position,radius))return;
  // Relocate only a valid old position newly covered by scenery. An old wall
  // intersection, invalid coordinate or corrupt current save is still rejected.
  bool changed=false;
  for(const auto& o:obstacles_) {
    const Vec2 nearest{std::clamp(position.x,o.center.x-o.halfX,o.center.x+o.halfX),std::clamp(position.z,o.center.z-o.halfZ,o.center.z+o.halfZ)};
    if(Distance(position,nearest)>=radius)continue;
    if(o.introducedVersion<=version && !(version<5 && o.kind==PropKind::Counter))return;
    changed=true;
  }
  if(!changed)return;
  const Vec2 origin=position;
  for(int ring=1;ring<=40;++ring)for(int direction=0;direction<32;++direction) {
    const float a=direction*6.2831853f/32,r=ring*.15f;
    const Vec2 p{origin.x+std::cos(a)*r,origin.z+std::sin(a)*r};
    if(CanStand(p,radius)){position=p;return;}
  }
}
}
