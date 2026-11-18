#include "engine/WorldAcoustics.h"
#include <utility>

namespace sound {
namespace {
constexpr float cell=8.f;
std::uint64_t Key(int x,int z){return (std::uint64_t(std::uint32_t(x))<<32)|std::uint32_t(z);}
bool Valid(AcousticPoint p){return particles::Finite(p)&&std::abs(p.x)<10000&&std::abs(p.y)<10000&&std::abs(p.z)<10000;}
float Unit(float v){return std::isfinite(v)?std::clamp(v,0.f,1.f):0;}
float Length(AcousticPoint p){return std::sqrt(particles::Dot(p,p));}
}
AcousticWorld::AcousticWorld(const rpg::WorldMap& map):height_(&rpg::WorldMap::Height){
  for(const auto& p:map.Obstacles()){
    // Canopies, banners and dressing are not sealed acoustic walls. Use actual
    // wall bodies rather than the larger camera/roof/foliage envelope.
    if(p.kind!=rpg::PropKind::Building&&p.kind!=rpg::PropKind::Ruin&&p.kind!=rpg::PropKind::SurveyStone)continue;
    const float y=map.Height(p.center);
    walls_.push_back({{p.center.x-p.halfX,y,p.center.z-p.halfZ},{p.center.x+p.halfX,y+p.height,p.center.z+p.halfZ}});
  }
  Index();
}
AcousticWorld::AcousticWorld(std::vector<particles::Box> walls,HeightFunction height):walls_(std::move(walls)),height_(height){Index();}
void AcousticWorld::Index(){
  walls_.erase(std::remove_if(walls_.begin(),walls_.end(),[](const auto& b){return !Valid(b.low)||!Valid(b.high)||
    b.low.x>=b.high.x||b.low.y>=b.high.y||b.low.z>=b.high.z||b.high.x-b.low.x>128||b.high.z-b.low.z>128;}),walls_.end());
  for(unsigned i=0;i<walls_.size();++i){const auto& b=walls_[i];
    for(int z=int(std::floor(b.low.z/cell));z<=int(std::floor(b.high.z/cell));++z)
      for(int x=int(std::floor(b.low.x/cell));x<=int(std::floor(b.high.x/cell));++x)bins_[Key(x,z)].push_back(i);
  }
}
float AcousticWorld::WallDistance(AcousticPoint from,AcousticPoint to) const {
  particles::Contact contact;
  // Queries are at most the audible radius, so at most 25 cells are visited.
  for(int z=int(std::floor(std::min(from.z,to.z)/cell));z<=int(std::floor(std::max(from.z,to.z)/cell));++z)
    for(int x=int(std::floor(std::min(from.x,to.x)/cell));x<=int(std::floor(std::max(from.x,to.x)/cell));++x){
      const auto bin=bins_.find(Key(x,z));if(bin==bins_.end())continue;
      for(auto i:bin->second)particles::SweepBox(from,to,0,walls_[i],contact);
    }
  return contact.fraction;
}
bool AcousticWorld::Blocked(AcousticPoint from,AcousticPoint to) const {
  if(WallDistance(from,to)<.999f)return true;
  if(!height_)return false;
  const int steps=std::clamp(int(std::ceil(Length(to-from)/.35f)),1,72);
  // Frequent height samples catch intervening ridges, not just endpoints or
  // one midpoint. A small tolerance prevents a ground contact muffling itself.
  for(int i=1;i<steps;++i){const auto p=from+(to-from)*(float(i)/steps);
    if(p.y<height_({p.x,p.z})-.04f)return true;
  }
  return false;
}
float AcousticWorld::Obstruction(AcousticPoint source,AcousticPoint listener) const {
  if(!Valid(source)||!Valid(listener))return 0;
  const auto delta=listener-source;const float distance=Length(delta);
  if(distance<.5f||distance>24)return 0;
  const auto side=particles::Normal({-delta.z,0,delta.x})*.23f;
  const AcousticPoint offsets[]={{},side,side*-1,{0,.22f,0},{0,-.22f,0}};
  float blocked=0;for(const auto offset:offsets)blocked+=Blocked(source,listener+offset)?1.f:0.f;
  return blocked/5;
}
float AcousticWorld::Enclosure(AcousticPoint listener) const {
  if(!Valid(listener))return 0;
  float nearby=0;
  for(int i=0;i<8;++i){const float a=float(i)*.7853981634f;
    nearby+=1-WallDistance(listener,listener+AcousticPoint{std::cos(a)*8,0,std::sin(a)*8});
  }
  return std::clamp(nearby/6,0.f,1.f);
}
void AcousticResponse::Update(float blocked,float room,float seconds){
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.1f):0;
  obstruction=Unit(obstruction)+(Unit(blocked)-Unit(obstruction))*-std::expm1(-dt/.09f);
  enclosure=Unit(enclosure)+(Unit(room)-Unit(enclosure))*-std::expm1(-dt/.24f);
}
float AcousticResponse::DirectGain() const{return std::pow(.22f,Unit(obstruction));}
float AcousticResponse::FilterCoefficient() const{
  // XAudio2's one-pole coefficient at the 22.05 kHz source rate. 1 bypasses
  // filtering completely; a fully occluded source rolls off around 850 Hz.
  const float cutoff=11025.f*std::pow(850.f/11025.f,Unit(obstruction));
  const float t=1-2*cutoff/22050.f;return 1-t*t;
}
float AcousticResponse::WetSend() const{return .018f+.48f*Unit(enclosure);}
}
