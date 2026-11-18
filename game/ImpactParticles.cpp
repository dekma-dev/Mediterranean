#include "game/ImpactParticles.h"
#include "game/SpellVfx.h"
#include "engine/Renderer.h"
#include <unordered_map>
#include <vector>

namespace impactfx {
namespace {
using namespace particles;
constexpr float cell=4;
std::uint64_t Key(int x,int z){return (std::uint64_t(std::uint32_t(x))<<32)|std::uint32_t(z);}
int Cell(float value){return int(std::floor(value/cell));}
float Ground(Vec3 p){return rpg::WorldMap::Height({p.x,p.z});}
class Scenery final:public CollisionWorld {
 rpg::WorldMap map_;
 std::vector<Box> boxes_;
 std::unordered_map<std::uint64_t,std::vector<unsigned>> cells_;
public:
 Scenery(){
  boxes_.reserve(map_.Obstacles().size());
  for(const auto& prop:map_.Obstacles()){
   const float ground=rpg::WorldMap::Height(prop.center);
   const Box box{{prop.center.x-prop.halfX,ground,prop.center.z-prop.halfZ},
     {prop.center.x+prop.halfX,ground+prop.height,prop.center.z+prop.halfZ}};
   const auto index=unsigned(boxes_.size());boxes_.push_back(box);
   for(int z=Cell(box.low.z);z<=Cell(box.high.z);++z)for(int x=Cell(box.low.x);x<=Cell(box.high.x);++x)
    cells_[Key(x,z)].push_back(index);
  }
 }
 bool Trace(Vec3 from,Vec3 to,float radius,Contact& hit)const override{
  if(!Finite(from)||!Finite(to)||std::abs(from.x)>10000||std::abs(from.z)>10000||
    std::abs(to.x)>10000||std::abs(to.z)>10000)return false;
  bool found=false;
  const auto clearance=[&](Vec3 p){return p.y-radius-Ground(p);};
  // Short 120 Hz steps include a midpoint terrain check rather than testing
  // only the destination. Bisection finds the first sampled surface crossing.
  float previous=0;
  for(int part=0;part<=2;++part){const float t=part*.5f;const Vec3 p=from+(to-from)*t;
   if(clearance(p)<=0){
    float low=previous,high=t;
    for(int i=0;i<9 && high>low;++i){const float mid=(low+high)*.5f;
     if(clearance(from+(to-from)*mid)>0)low=mid;else high=mid;}
    const Vec3 at=from+(to-from)*high;
    const Vec3 normal=Normal({Ground(at+Vec3{-.06f,0,0})-Ground(at+Vec3{.06f,0,0}),.12f,
      Ground(at+Vec3{0,0,-.06f})-Ground(at+Vec3{0,0,.06f})});
    const auto surface=map_.SurfaceAt({at.x,at.z});const float paving=std::clamp(surface.paving,0.f,1.f);
    hit={high,{at.x,Ground(at)+radius,at.z},normal,.28f+paving*.45f,.72f-paving*.40f,true};found=true;break;
   }
   previous=t;
  }
  // Only nearby bins are queried. Vegetation canopy/roof silhouettes do not
  // become invisible collision walls: use the established solid prop bounds.
  for(int z=Cell(std::min(from.z,to.z)-radius);z<=Cell(std::max(from.z,to.z)+radius);++z)
   for(int x=Cell(std::min(from.x,to.x)-radius);x<=Cell(std::max(from.x,to.x)+radius);++x){
    const auto bin=cells_.find(Key(x,z));if(bin==cells_.end())continue;
    for(const auto index:bin->second)if(SweepBox(from,to,radius,boxes_[index],hit))found=true;
   }
  return found;
 }
};
float Random(unsigned& seed){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return float(seed&0xffffff)/float(0x1000000);}
DirectX::XMFLOAT3 Point(Vec3 p){return {p.x,p.y,p.z};}
}
const particles::CollisionWorld& Environment(){static const Scenery world;return world;}
void System::Emit(const rpg::CombatVisualEvent& e,unsigned seed){
 using K=rpg::CombatVisualKind;using C=rpg::ClassId;
 if((e.kind!=K::Hit && e.kind!=K::HeavyHit) || e.evaded)return;
 Material material;bool thermal=false;
 if(e.school==C::Warrior)material=Material::Steel;
 else if(e.school==C::Scout)material=Material::Wood;
 else if(e.school==C::Priest)material=Material::Holy;
 else if(e.school==C::Mage){auto classification=e;classification.primary=true;
  const auto residue=spellvfx::MakeResidue(classification);
  thermal=residue.kind==spellvfx::ResidueKind::Thermal;
  material=residue.kind==spellvfx::ResidueKind::Frost?Material::Frost:
    residue.kind==spellvfx::ResidueKind::Arcane?Material::Arcane:Material::Ember;
 }else return;
 if(!std::isfinite(e.facing))return;
 unsigned random=seed*747796405u+e.actor*2891336453u+(unsigned(e.ability)+2u)*277803737u+1u;
 if(!random)random=1;
 const bool major=e.kind==K::HeavyHit || e.signature || e.chain>=2;
 const unsigned count=e.primary?(major?24:16):3;
 const float sn=std::sin(e.facing),cs=std::cos(e.facing);
 for(unsigned i=0;i<count;++i){
  const auto kind=thermal && i%2==0?Material::Frost:material;
  Particle p;p.position={e.position.x,e.height,e.position.z};p.style=std::uint8_t(kind);p.seed=random;
  const float angle=Random(random)*6.2831853f,speed=(major?2.3f:1.4f)+Random(random)*2.6f;
  float lateral=std::cos(angle)*speed,forward=std::sin(angle)*speed;
  if(kind==Material::Wood){lateral*=.45f;forward=.8f+std::abs(forward);}
  p.velocity={cs*lateral+sn*forward,.25f+Random(random)*(major?3.5f:2.4f),-sn*lateral+cs*forward};
  p.lifetime=1.05f+Random(random)*.45f;p.radius=.018f+Random(random)*.012f;
  p.drag=.45f;p.bounce=.85f;
  if(kind==Material::Wood){p.drag=1.3f;p.bounce=.3f;}
  if(kind==Material::Ember){p.drag=.65f;p.gravity=7;p.lifetime+=.2f;}
  if(kind==Material::Frost){p.bounce=.95f;p.radius=.08f;}
  if(kind==Material::Holy || kind==Material::Arcane){p.gravity=5.5f;p.drag=1.1f;p.bounce=.55f;}
  if(!pool_.Spawn(p,e.elapsed,Environment()))break;
 }
}
void System::Render(Renderer& r)const{
 using Art=Renderer::EffectArt;
 // One bounded batch allowance, after warnings and main contacts. No new GPU
 // buffers, render targets, lights or per-frame allocations.
 const auto limit=std::min(Renderer::maxEffectVertices,r.EffectVertices()+960u);
 for(const auto& p:pool_.Items()){
  if(!p.Alive() || r.EffectVertices()+24>limit)continue;
  const float age=float(p.age),fade=std::clamp(age/.018f,0.f,1.f)*std::pow(std::clamp(1-age/p.lifetime,0.f,1.f),.75f);
  const auto material=Material(p.style);auto at=p.Position();
  DirectX::XMFLOAT4 color{1,.75f,.38f,fade};Art art=Art::Sparks;float size=.037f;
  if(material==Material::Wood){color={.55f,.42f,.24f,fade};art=Art::Dust;size=.05f;}
  if(material==Material::Ember){color={1,.48f,.13f,fade};art=Art::Flame;size=.08f;}
  if(material==Material::Frost){color={.52f,.84f,1,fade};art=Art::Ice;size=.07f;}
  if(material==Material::Arcane){color={.64f,.47f,1,fade};art=Art::Arcane;size=.06f;}
  if(material==Material::Holy){color={1,.87f,.48f,fade};art=Art::Sparks;size=.06f;}
  // Short motion tails fold at the contact; settled fragments stop streaking.
  if(!p.resting){const auto tail=age-p.contactAge<.05f?p.previous:at-p.velocity*.022f;
   r.DrawEffectStreak(Point(tail),Point(at),material==Material::Wood?.012f:.022f,color);}
  const float rotation=float(p.seed%19)+(p.resting?p.contactAge:age)*7;
  if(material==Material::Frost){
   // Two crossed fractured surfaces keep thickness from side views; on
   // settling, lay the chip against the actual contact plane.
   const float length=.12f+float(p.seed%9)*.008f,w=length*.30f;
   const Vec3 horizontal{std::cos(rotation),0,std::sin(rotation)};
   const Vec3 up=p.resting?p.contactNormal:Normal(Vec3{-std::sin(rotation)*.5f,1,std::cos(rotation)*.5f});
   const Vec3 side=Normal({up.y*horizontal.z-up.z*horizontal.y,up.z*horizontal.x-up.x*horizontal.z,up.x*horizontal.y-up.y*horizontal.x});
   const Vec3 along=p.resting?Normal(Vec3{up.y*side.z-up.z*side.y,up.z*side.x-up.x*side.z,up.x*side.y-up.y*side.x}):up;
   for(int plane=0;plane<(p.resting?1:2);++plane){const Vec3 across=plane?horizontal:side;
    const auto vertex=[&](float x,float y){return Point(at+across*x+along*y);};
    r.DrawEffectSurface(Art::Ice,vertex(-w,-length*.5f),vertex(-w*.35f,length),vertex(w*.30f,length*.75f),vertex(w,-length*.6f),color);
   }
  }else r.DrawEffectSprite(art,Point(at),size,size*(material==Material::Wood?1.8f:1.3f),rotation,color,material==Material::Wood);
  const float dustAge=age-p.contactAge;
  if(p.groundContact && p.seed%4==0 && dustAge>=0 && dustAge<.30f){
   const auto dust=p.contactPoint+Vec3{0,.055f+dustAge*.16f,0};
   r.DrawEffectSprite(Art::Dust,Point(dust),.06f+dustAge*.48f,.04f+dustAge*.24f,float(p.seed%7),
     {.48f,.43f,.34f,(1-dustAge/.30f)*.36f},true);
  }
 }
}
}
