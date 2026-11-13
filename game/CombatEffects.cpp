#include "game/CombatEffects.h"
#include "game/ClassEffects.h"
#include "game/SpellVfx.h"
#include "game/CastEffects.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace {
using K=rpg::CombatVisualKind;
using namespace DirectX;
constexpr float trailLife=.16f,burstLife=.48f,sweepLife=.32f;
bool Healing(K kind) { return kind==K::Mend || kind==K::Potion; }
bool Dashing(K kind) { return kind==K::DashStart || kind==K::DashStop || kind==K::DashImpact; }
bool HostileRelease(K kind) {return kind==K::EnemyQuake || kind==K::EnemyTempest || kind==K::EnemyExecution || kind==K::EnemyLance;}
float BurstLife(const rpg::CombatVisualEvent& e) {
  if(e.kind==K::SpellCast)return std::clamp((std::isfinite(e.flight)?e.flight:0.f)+.16f,.16f,.95f);
  if(HostileRelease(e.kind))return .72f;
  if(e.kind==K::DisciplineReady || e.kind==K::DisciplineSpent)return .65f;
  return e.kind==K::Hit || e.kind==K::HeavyHit?spellvfx::ContactLifetime(e):Healing(e.kind)?.8f:Dashing(e.kind)?.42f:burstLife;
}
float Unit(float x) { return std::clamp(x,0.f,1.f); }
XMFLOAT3 GroundPoint(float x,float z,float lift) {return {x,rpg::WorldMap::Height({x,z})+lift,z};}
XMFLOAT4 Tint(K kind) {
  if (kind==K::Interrupt) return {.67f,.30f,1,1};
  if (kind==K::HeavyStrike || kind==K::HeavyHit) return {.18f,.65f,1,1};
  if (kind==K::EnemySwing || kind==K::Hurt) return {1,.25f,.055f,1};
  return {1,.62f,.16f,1};
}
float Noise(unsigned value) {
  value^=value>>16; value*=0x7feb352du; value^=value>>15; value*=0x846ca68bu; value^=value>>16;
  return float(value&65535)/65535;
}
// World-anchored cast residue: twin ascending ribbons leave the silhouette
// readable, while a smaller potion pulse remains distinct from Mend.
void HealingPulse(Renderer& renderer,const rpg::CombatVisualEvent& event,float age) {
  const float phase=Unit(age/.8f);
  const float envelope=std::sin(phase*3.14159265f);
  const bool mend=event.kind==K::Mend;
  XMFLOAT4 color=mend?XMFLOAT4{.16f,1,.59f,1}:XMFLOAT4{.9f,.76f,.24f,1};
  const float radius=mend?.48f:.31f;
  constexpr int segments=20;
  const auto point=[&](float u,int strand,float width) -> XMFLOAT3 {
    const float angle=u*5.5f+phase*2.8f+strand*3.14159265f;
    const float r=radius+width;
    return {event.position.x+std::cos(angle)*r,event.height+.12f+u*1.45f+phase*.22f,event.position.z+std::sin(angle)*r};
  };
  for (int strand=0;strand<(mend?2:1);++strand) for (int i=0;i<segments;++i) {
    const float a=float(i)/segments,b=float(i+1)/segments;
    color.w=envelope*.55f;
    renderer.DrawEffectRibbon(point(a,strand,-.035f),point(a,strand,.035f),point(b,strand,.035f),point(b,strand,-.035f),color,
      std::sin(a*3.14159265f),std::sin(b*3.14159265f));
  }
  color.w=envelope*.2f;
  renderer.DrawEffectGlow({event.position.x,event.height+.9f,event.position.z},mend?.72f:.46f,color);
  for (int i=0;i<(mend?10:6);++i) {
    const float angle=i*2.3999632f+phase*1.8f;
    const float rise=std::fmod(i*.137f+phase,1.f);
    const XMFLOAT3 mote{event.position.x+std::cos(angle)*radius,event.height+.2f+rise*1.7f,event.position.z+std::sin(angle)*radius};
    color.w=envelope*std::sin(rise*3.14159265f)*.7f;
    renderer.DrawEffectGlow(mote,.04f,color);
  }
}
// Directional air rings and ballistic motes stay at the actual launch/contact
// position. Ground grit is suppressed in flight; it is never a teleport beam.
void DashPulse(Renderer& renderer,const rpg::CombatVisualEvent& event,float age) {
  const bool start=event.kind==K::DashStart,impact=event.kind==K::DashImpact;
  const float phase=Unit(age/.42f),fade=(1-phase)*(1-phase);
  const float heading=event.facing+(rpg::ClassCombat(event.school).retreatDistance>0?3.14159265f:0);
  const float sx=std::sin(heading),cx=std::cos(heading);
  const auto& tint=rpg::DescribeClass(event.school).color;
  XMFLOAT4 color=impact?XMFLOAT4{1,.64f,.28f,1}:event.school==rpg::ClassId::Warrior?
    XMFLOAT4{.24f,.78f,1,1}:XMFLOAT4{tint[0],tint[1],tint[2],1};
  const float radius=.32f+phase*(start?.85f:.55f);
  const bool grounded=event.height-rpg::WorldMap::Height(event.position)<.08f;
  const bool groundStop=!start && !impact && grounded;
  const auto ringPoint=[&](float a,float width) -> XMFLOAT3 {
    const float r=radius+width,side=std::cos(a)*r;
    const float back=(start?-.35f:.10f)*phase;
    if (groundStop) {
      const float forward=std::sin(a)*r*.65f;
      return GroundPoint(event.position.x+cx*side+sx*forward,event.position.z-sx*side+cx*forward,.055f);
    }
    return {event.position.x+cx*side+sx*back,
      event.height+.85f+std::sin(a)*r*.85f,event.position.z-sx*side+cx*back};
  };
  color.w=fade*(start?.45f:.30f);
  for (int i=0;i<24;++i) {
    const float a=i*motion::tau/24,b=(i+1)*motion::tau/24;
    renderer.DrawEffectRibbon(ringPoint(a,-.022f),ringPoint(a,.022f),
      ringPoint(b,.022f),ringPoint(b,-.022f),color,1,1);
  }
  color.w=fade*.13f;
  renderer.DrawEffectGlow({event.position.x,event.height+.85f,event.position.z},.6f+phase*.3f,color);
  for (int i=0;i<10;++i) {
    const float angle=i*2.3999632f,spread=.6f+Noise(i*37+5)*1.4f;
    const float side=std::cos(angle)*spread,forward=(start?-2.6f:impact?-1.5f:.8f);
    const bool grit=grounded && i<4;
    const auto at=[&](float t) -> XMFLOAT3 {
      const float travel=-std::expm1(-5*t)/5;
      auto p=GroundPoint(event.position.x+(cx*side+sx*forward)*travel,event.position.z+(-sx*side+cx*forward)*travel,.035f);
      p.y=std::max(p.y,event.height+(grit?.07f:.4f+float(i%4)*.3f)+(grit?1.2f:.5f)*t-(grit?4.f:1.f)*t*t);
      return p;
    };
    auto mote=grit?XMFLOAT4{.62f,.49f,.29f,fade*.22f}:color;
    if (!grit) mote.w=fade*.65f;
    const auto head=at(age),tail=at(std::max(0.f,age-.045f));
    renderer.DrawEffectStreak(tail,head,grit?.035f:.018f,mote);
    renderer.DrawEffectGlow(head,grit?.055f:.026f,mote);
  }
}
// A tapered, curved slash envelope. Its two ribbons share the normal effect
// batch: a colored halo around a thin bright edge, without a bloom render target.
void Crescent(Renderer& renderer,XMFLOAT3 origin,float facing,float age,bool heavy,bool enemy,bool echo,rpg::ClassId school) {
  const float delay=echo?.045f:0,phase=(age-delay)/(sweepLife-delay);
  if (phase<=0 || phase>=1) return;
  constexpr int segments=28;
  const float progress=1-std::pow(1-phase,3.f);
  const float head=-1.1f+progress*3.25f,span=heavy?2.45f:enemy?1.45f:1.95f;
  const float radius=(heavy?1.15f:enemy?.80f:.96f)+progress*.12f-(echo?.12f:0);
  const float envelope=std::sin(phase*3.14159265f)*std::min(1.f,phase*12);
  const float width=(heavy?.36f:enemy?.16f:.25f)*(echo?.7f:1);
  const float tilt=echo?-.45f:.4f;
  const float sx=std::sin(facing),cx=std::cos(facing);
  const auto point=[&](float u,float offset) -> XMFLOAT3 {
    const float angle=head-span+span*u;
    const float r=radius+offset*std::pow(std::max(0.f,std::sin(u*3.14159265f)),.7f);
    const float horizontal=std::cos(angle)*r,vertical=std::sin(angle)*r;
    const float x=horizontal*.9f-vertical*tilt,y=horizontal*tilt+vertical*.9f,z=.75f+vertical*.28f;
    return {origin.x+cx*x+sx*z,origin.y+1.1f+y,origin.z-sx*x+cx*z};
  };
  auto color=echo?XMFLOAT4{.62f,.22f,1,1}:Tint(heavy?K::HeavyStrike:enemy?K::EnemySwing:K::Strike);
  if(!enemy && school!=rpg::ClassId::Warrior) color=classfx::Color(school);
  for (int layer=0;layer<2;++layer) {
    const float thickness=width*(layer==0?2.6f:.55f);
    color.w=envelope*(layer==0?.24f:.92f)*(enemy?.65f:1)*(echo?.7f:1);
    for (int segment=0;segment<segments;++segment) {
      const float a=float(segment)/segments,b=float(segment+1)/segments;
      renderer.DrawEffectRibbon(point(a,-thickness),point(a,thickness),point(b,thickness),point(b,-thickness),color,
        std::sin(a*3.14159265f),std::sin(b*3.14159265f));
    }
  }
  // Fine fragments follow the leading edge, leaving a brief colored residue
  // rather than increasing the brightness of the whole slash silhouette.
  if (!enemy && !echo) for (int i=0;i<5;++i) {
    const float u=.55f+i*.09f;
    auto fragment=color; fragment.w=envelope*(1-phase)*.5f;
    renderer.DrawEffectStreak(point(u,-width*.12f),point(std::min(1.f,u+.035f),width*.45f),
      heavy?.018f:.012f,fragment);
  }
}
}
void CombatEffects::Reset() { bursts_={}; trails_={}; wake_={}; serial_=0; flights_={}; footprints_={};footprintCursor_=0;residues_={}; }
void CombatEffects::EmitFootstep(const presentation::CreatureContact& contact,const rpg::WorldMap& map) {
 if(!std::isfinite(contact.position.x+contact.position.y+contact.position.z+contact.strength+contact.facing) || contact.strength<=0)return;
 const auto p=rpg::Vec2{contact.position.x,contact.position.z};const auto surface=map.SurfaceAt(p);
 if(surface.paving>.5f || (p.x>=rpg::WorldMap::dockMin.x && p.x<=rpg::WorldMap::dockMax.x && p.z>=rpg::WorldMap::dockMin.z && p.z<=rpg::WorldMap::dockMax.z))return;
 footprints_[footprintCursor_++%footprints_.size()]={contact,0,surface.dirt};
}
std::size_t CombatEffects::FootprintCount() const {
 return std::count_if(footprints_.begin(),footprints_.end(),[](const auto& p){return p.age<2.8f;});
}
std::size_t CombatEffects::ResidueCount() const {
 return std::count_if(residues_.begin(),residues_.end(),[](const auto& r){return r.age<spellvfx::ResidueLifetime(r.kind);});
}
void CombatEffects::Advance(float dt) {
  if (!std::isfinite(dt) || dt<=0) return;
  dt=std::min(dt,.05f);
  for(auto& residue:residues_)residue.age=std::min(10.f,residue.age+dt);
  for(auto& print:footprints_)print.age=std::min(10.f,print.age+dt);
  for (std::size_t i=0;i<wake_.count;++i) wake_.samples[i].age+=dt;
  const auto wakeEnd=std::remove_if(wake_.samples.begin(),wake_.samples.begin()+wake_.count,
    [](const WakeSample& sample) { return sample.age>=.20f; });
  wake_.count=std::size_t(wakeEnd-wake_.samples.begin());
  for (auto& burst : bursts_) burst.age=std::min(1.f,burst.age+dt);
  for (auto& trail : trails_) {
    trail.age=std::min(1.f,trail.age+dt);
    for (std::size_t i=0;i<trail.count;++i) trail.samples[i].age+=dt;
    const auto end=std::remove_if(trail.samples.begin(),trail.samples.begin()+trail.count,
      [](const Sample& sample) { return sample.age>=trailLife; });
    trail.count=std::size_t(end-trail.samples.begin());
  }
}
XMFLOAT3 CombatEffects::SpellTracePoint(XMFLOAT3 origin,rpg::Vec2 destination,float progress,float arc,float targetHeight) {
  const float u=std::isfinite(progress)?Unit(progress):0;
  arc=std::isfinite(arc)?std::clamp(arc,0.f,.5f):0;
  targetHeight=std::isfinite(targetHeight)?std::clamp(targetHeight,.3f,3.f):1.25f;
  // Current enemy capsules stand on map ground, including when the caster jumps.
  return {origin.x+(destination.x-origin.x)*u,origin.y+(rpg::WorldMap::Height(destination)+targetHeight-origin.y)*u+arc*std::sin(u*3.14159265f),
    origin.z+(destination.z-origin.z)*u};
}
spellvfx::FlightPath* CombatEffects::Path(const Burst& b) {
  for(auto& flight:flights_)if(flight.seed==b.seed && flight.path.Active())return &flight.path;
  return nullptr;
}
const spellvfx::FlightPath* CombatEffects::Path(const Burst& b) const {
  for(const auto& flight:flights_)if(flight.seed==b.seed && flight.path.Active())return &flight.path;
  return nullptr;
}
const spellvfx::FlightPath* CombatEffects::ProjectilePath(rpg::EntityId target) const {
  const Burst* newest=nullptr;
  for(const auto& b:bursts_)if(b.event.kind==K::SpellCast && b.event.target==target && Path(b) && b.age<BurstLife(b.event) &&
    (!newest || b.seed>newest->seed))newest=&b;
  return newest?Path(*newest):nullptr;
}
void CombatEffects::TrackProjectiles(const rpg::Simulation& state) {
  for(auto& b:bursts_) {
    auto* path=Path(b);
    if(b.event.kind!=K::SpellCast || !path || path->Age()>=b.event.flight || b.age>=BurstLife(b.event))continue;
    const rpg::Enemy* target=nullptr;
    for(const auto& e:state.Combatants())if(e.id==b.event.target)target=&e;
    for(const auto& e:state.Dummies())if(e.id==b.event.target)target=&e;
    if(state.Sparring() && state.SparringOpponent().id==b.event.target)target=&state.SparringOpponent();
    // Never acquire the newly selected target, resurrect a vanished actor or
    // manufacture an impact. Missing/dead targets retain the last trajectory.
    auto destination=path->Target();
    if(target && target->health>0 && target->state!=rpg::EnemyState::Returning)
      destination={target->position.x,rpg::WorldMap::Height(target->position)+target->ContactHeight(),target->position.z};
    // If this update crosses arrival without a hit, interpolate to its clock
    // instead of tracking the target into the unused tail of this update.
    if(b.age>b.event.flight && b.age>path->Age()) {
      const float u=Unit((b.event.flight-path->Age())/(b.age-path->Age()));const auto previous=path->Target();
      destination={previous.x+(destination.x-previous.x)*u,previous.y+(destination.y-previous.y)*u,previous.z+(destination.z-previous.z)*u};
    }
    path->Advance(b.event,b.spellOrigin,b.age,destination);
  }
}
void CombatEffects::Emit(const rpg::CombatVisualEvent& incoming,const XMFLOAT3* spellOrigin) {
  auto event=incoming;
  if(event.kind==K::UiClick || event.kind==K::CastStart || event.kind==K::ReserveReady || event.kind==K::CastCancelled ||
    event.kind==K::EnemyPrepare || event.kind==K::BossPrepare) return;
  const float elapsed=std::isfinite(event.elapsed)?std::clamp(event.elapsed,0.f,.05f):0;
  if((event.kind==K::Hit || event.kind==K::HeavyHit) && event.primary) {
    for(auto& b:bursts_)if(b.event.kind==K::SpellCast && Path(b) && b.event.target==event.actor &&
      b.event.ability==event.ability && b.event.school==event.school && std::abs(b.age-b.event.flight-elapsed)<.051f) {
      Path(b)->Advance(b.event,b.spellOrigin,b.event.flight,{event.position.x,event.height,event.position.z});
      const auto tip=Path(b)->Point(1),tail=Path(b)->Point(.98f);
      if(std::hypot(tip.x-tail.x,tip.z-tail.z)>.001f)event.facing=std::atan2(tip.x-tail.x,tip.z-tail.z);
    }
  }
  auto residue=spellvfx::MakeResidue(event);
  residue.order=serial_+1;
  if(residue.kind!=spellvfx::ResidueKind::None) {
    // Repeated contacts can rekindle a small patch, not stack an unlimited
    // pile of decals at a practice target. Existing marks stay on the ground.
    auto slot=std::find_if(residues_.begin(),residues_.end(),[&](const auto& r){
      return r.kind==residue.kind && r.age<spellvfx::ResidueLifetime(r.kind) &&
        std::hypot(r.position.x-residue.position.x,r.position.z-residue.position.z)<.5f;
    });
    if(slot!=residues_.end()) {slot->age=std::min(slot->age,.3f);slot->size=std::max(slot->size,residue.size);}
    else {*std::max_element(residues_.begin(),residues_.end(),[](const auto& a,const auto& b){return a.age==b.age?a.order>b.order:a.age<b.age;})=residue;}
  }
  if (event.kind==K::DashStart) {
    CancelTrail(0);
    wake_={}; wake_.school=event.school;
    wake_.facing=event.facing+(rpg::ClassCombat(event.school).retreatDistance>0?3.14159265f:0); wake_.active=true;
    wake_.previous={event.position.x,event.height,event.position.z};
    wake_.samples[wake_.count++]={wake_.previous,elapsed,0};
  }
  if (event.kind==K::Strike || event.kind==K::HeavyStrike || event.kind==K::EnemySwing) {
    auto slot=std::find_if(trails_.begin(),trails_.end(),[&](const Trail& trail) { return trail.actor==event.actor && trail.age<.4f; });
    if (slot==trails_.end()) slot=std::max_element(trails_.begin(),trails_.end(),[](const Trail& a,const Trail& b) { return a.age<b.age; });
    *slot={}; slot->actor=event.actor; slot->kind=event.kind; slot->age=elapsed;
    slot->origin={event.position.x,event.height,event.position.z}; slot->facing=event.facing;
    slot->school=event.school;
    return;
  }
  auto slot=std::max_element(bursts_.begin(),bursts_.end(),[](const Burst& a,const Burst& b) {
    return a.age/BurstLife(a.event)<b.age/BurstLife(b.event);
  });
  *slot={event,elapsed,++serial_};
  slot->spellOrigin=spellOrigin && std::isfinite(spellOrigin->x) && std::isfinite(spellOrigin->y) && std::isfinite(spellOrigin->z)?
    *spellOrigin:XMFLOAT3{event.position.x,event.height+1.25f,event.position.z};
  if(event.kind==K::SpellCast && event.target && (event.school==rpg::ClassId::Mage || event.school==rpg::ClassId::Scout || event.school==rpg::ClassId::Priest)) {
    auto& tracked=*std::min_element(flights_.begin(),flights_.end(),[](const auto& a,const auto& b){return a.seed<b.seed;});
    tracked.seed=slot->seed;tracked.path.Reset(event,slot->spellOrigin);
  }
}
bool CombatEffects::Tracking(rpg::EntityId actor) const {
  for (const auto& trail : trails_) if (trail.actor==actor && trail.age<.24f) return true;
  return false;
}
void CombatEffects::TrackWeapon(rpg::EntityId actor,XMFLOAT3 root,XMFLOAT3 tip) {
  for (auto& trail : trails_) if (trail.actor==actor && trail.age<.24f) {
    if(!std::isfinite(root.x+root.y+root.z+tip.x+tip.y+tip.z)){trail={};return;}
    const auto blade=XMLoadFloat3(&tip)-XMLoadFloat3(&root);
    const float length=XMVectorGetX(XMVector3Length(blade));
    if(length<.02f || length>3){trail={};return;}
    const auto append=[&](Sample sample){
      if(trail.count==trail.samples.size()){
        std::move(trail.samples.begin()+1,trail.samples.end(),trail.samples.begin());--trail.count;
      }
      trail.samples[trail.count++]=sample;
    };
    const float dt=trail.age-trail.sampledAt;
    if(!trail.tracked || !trail.count || dt>.06f){
      trail.count=0;append({root,tip,0});trail.pending=0;trail.liveEndpoint=false;
    }else{
      const auto oldRoot=XMLoadFloat3(&trail.previousRoot),oldTip=XMLoadFloat3(&trail.previousTip);
      const auto rootDelta=XMLoadFloat3(&root)-oldRoot,tipDelta=XMLoadFloat3(&tip)-oldTip;
      // Both endpoints matter: a malformed grip must not leave a huge ribbon
      // even when the tip happens to stay near the previous sample.
      if(XMVectorGetX(XMVector3LengthSq(rootDelta))>4 || XMVectorGetX(XMVector3LengthSq(tipDelta))>4){trail={};return;}
      if(dt<=1e-7f){trail.samples[trail.count-1]={root,tip,0};trail.previousRoot=root;trail.previousTip=tip;return;}
      if(trail.liveEndpoint && trail.count)--trail.count;
      const auto oldBlade=oldTip-oldRoot;const float oldLength=XMVectorGetX(XMVector3Length(oldBlade));
      if(XMVectorGetX(XMVector3Dot(oldBlade/oldLength,blade/length))<-.95f){trail={};return;}
      // Fixed 120 Hz samples reconstruct the measured sweep. Interpolating
      // direction and length keeps a turning blade from shrinking to a chord.
      constexpr float interval=1.f/120;
      float offset=interval-trail.pending,last=-1;
      for(;offset<=dt+1e-6f;offset+=interval){const float t=Unit(offset/dt);Sample sample;
        const auto at=oldRoot+rootDelta*t;
        const auto axis=XMVector3Normalize(XMVectorLerp(oldBlade/oldLength,blade/length,t));
        XMStoreFloat3(&sample.root,at);XMStoreFloat3(&sample.tip,at+axis*(oldLength+(length-oldLength)*t));
        sample.age=std::max(0.f,dt-offset);append(sample);last=offset;
      }
      trail.pending=std::clamp(interval-(offset-dt),0.f,interval);
      trail.liveEndpoint=last<dt-1e-6f;
      if(trail.liveEndpoint)append({root,tip,0});
    }
    trail.previousRoot=root;trail.previousTip=tip;trail.sampledAt=trail.age;trail.tracked=true;
    return;
  }
}
void CombatEffects::TrackDash(XMFLOAT3 position,bool active,float dt,float strength) {
  if (!wake_.active || !std::isfinite(dt) || dt<=0) return;
  if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) { wake_={}; return; }
  dt=std::min(dt,.05f); strength=std::isfinite(strength)?Unit(strength):0;
  const auto delta=XMLoadFloat3(&position)-XMLoadFloat3(&wake_.previous);
  if (XMVectorGetX(XMVector3LengthSq(delta))>4) { wake_={}; return; }
  const float dx=position.x-wake_.previous.x,dz=position.z-wake_.previous.z;
  if (dx*dx+dz*dz>1e-10f) wake_.facing=std::atan2(dx,dz);
  // Keep committed samples on a 120 Hz clock, interpolating the actual swept
  // segment. A separate live endpoint must not become a frame-dependent sample.
  if (wake_.liveEndpoint && wake_.count) --wake_.count;
  const auto append=[&](WakeSample sample) {
    if (wake_.count==wake_.samples.size()) {
      std::move(wake_.samples.begin()+1,wake_.samples.end(),wake_.samples.begin()); --wake_.count;
    }
    wake_.samples[wake_.count++]=sample;
  };
  constexpr float interval=1.f/120;
  float offset=interval-wake_.pending,last=-1;
  for (;offset<=dt+1e-6f;offset+=interval) {
    const float t=Unit(offset/dt);
    XMFLOAT3 sample;
    XMStoreFloat3(&sample,XMLoadFloat3(&wake_.previous)+delta*t);
    append({sample,std::max(0.f,dt-offset),wake_.strength+(strength-wake_.strength)*t}); last=offset;
  }
  wake_.pending=std::clamp(interval-(offset-dt),0.f,interval);
  wake_.liveEndpoint=last<dt-1e-6f;
  if (wake_.liveEndpoint) append({position,0,strength});
  wake_.previous=position; wake_.strength=strength; wake_.active=active;
}
void CombatEffects::CancelDash() {
  wake_={};
  for (auto& burst : bursts_) if (Dashing(burst.event.kind)) burst=Burst{};
}
void CombatEffects::CancelTrail(rpg::EntityId actor) {
  for (auto& trail : trails_) if (trail.actor==actor) trail=Trail{};
}
void CombatEffects::CancelTrails() { trails_={}; }
std::size_t CombatEffects::BurstCount() const {
  return std::count_if(bursts_.begin(),bursts_.end(),[](const Burst& burst) { return burst.age<BurstLife(burst.event); });
}
std::size_t CombatEffects::TrailCount() const {
  return std::count_if(trails_.begin(),trails_.end(),[](const Trail& trail) { return trail.age<sweepLife || trail.count>1; });
}
std::size_t CombatEffects::WeaponSampleCount(rpg::EntityId actor)const{
  for(const auto& trail:trails_)if(trail.actor==actor && trail.tracked && trail.count)return trail.count;
  return 0;
}
float CombatEffects::WeaponHistorySeconds(rpg::EntityId actor)const{
  for(const auto& trail:trails_)if(trail.actor==actor && trail.tracked && trail.count)return trail.samples[0].age;
  return 0;
}
bool CombatEffects::HasRecovery() const {
  return std::any_of(bursts_.begin(),bursts_.end(),[](const Burst& b){return b.event.kind==K::Mend && b.age<BurstLife(b.event);});
}
bool CombatEffects::NeedsPose() const {
  return HasRecovery() || std::any_of(bursts_.begin(),bursts_.end(),[](const Burst& b){return b.event.actor==0 &&
    (b.event.kind==K::DisciplineReady || b.event.kind==K::DisciplineSpent) && b.age<BurstLife(b.event);});
}
void CombatEffects::Illuminate(Renderer& renderer,const rpg::Simulation& state,const castfx::Attachments& caster) const {
 using C=rpg::ClassId;
 if(!caster.alive || state.Health()<=0)return;
 const auto tint=[](C school,int ability)->XMFLOAT3 {
  if(school==C::Priest)return {1,.78f,.32f};
  if(ability==5 || ability==6 || ability==8 || ability==12)return {.24f,.60f,1};
  if(ability==10 || ability==13)return {.58f,.22f,1};
  return {1,.25f,.055f};
 };
 const auto pulse=[](float age,float lifetime){return combat::Smooth(age/.018f)*std::pow(Unit(1-age/lifetime),2.f);};
 if(state.Class()==C::Mage || state.Class()==C::Priest){
  const bool recovery=state.CastingRecovery();
  const float progress=recovery?state.RecoveryProgress():state.Attacking()?state.AttackSeconds()/combat::StrokeStart(state.HeavyAttack()):-1;
  if(progress>=0 && progress<1){const float power=combat::Smooth(progress);
   renderer.AddCombatLight(caster.focus,2.0f,tint(state.Class(),state.AttackAbility()),power*1.8f);}
 }
 for(const auto& b:bursts_){
  const auto& e=b.event;if(b.age>=BurstLife(e) || (e.school!=C::Mage && e.school!=C::Priest))continue;
  const float power=e.heavy?1.35f:1.f;const auto color=tint(e.school,e.ability);
  if(e.kind==K::SpellCast && e.flight>0 && b.age<e.flight){
   const auto position=Path(b)?Path(b)->Point(Unit(b.age/e.flight)):spellvfx::ProjectilePosition(e,b.spellOrigin,Unit(b.age/e.flight));
   renderer.AddCombatLight(position,2.3f*power,color,2.2f*power);
  }else if(e.kind==K::Hit || e.kind==K::HeavyHit){
   renderer.AddCombatLight({e.position.x,e.height,e.position.z},3.1f*power,color,6.f*power*pulse(b.age,.56f));
  }else if(e.kind==K::Mend){
   renderer.AddCombatLight({caster.root.x,caster.root.y+.9f,caster.root.z},3.2f,color,3.2f*pulse(b.age,.75f));
  }
 }
}
void CombatEffects::Render(Renderer& renderer,const castfx::Attachments* caster) const {
  // Warning contours are already submitted by Game. Contact geometry gets
  // the next budget, before weapon wakes and player cosmetics. Sort without
  // allocation; simultaneous crowded releases retain all their silhouettes.
  std::array<const Burst*,maxBursts> releases{};std::size_t releaseCount=0;
  for(const auto& b:bursts_)if(HostileRelease(b.event.kind) && b.age<BurstLife(b.event))releases[releaseCount++]=&b;
  std::sort(releases.begin(),releases.begin()+releaseCount,[](const Burst* a,const Burst* b){
    if(a->age!=b->age)return a->age<b->age;
    if(a->event.actor!=b->event.actor)return a->event.actor<b->event.actor;
    return a->event.kind<b->event.kind;
  });
  for(std::size_t i=0;i<releaseCount;++i)spellvfx::EnemyContact(renderer,releases[i]->event,releases[i]->age,releaseCount<=2,&releases[i]->spellOrigin);
  for(const auto& print:footprints_) {
    if(print.age>=2.8f)continue;
    const auto& e=print.contact;const float c=std::cos(e.facing),s=std::sin(e.facing);
    const float fade=Unit(1-print.age/2.8f),alpha=fade*(.18f+print.dirt*.22f);
    const auto point=[&](float x,float z) {return GroundPoint(e.position.x+c*x+s*z,e.position.z-s*x+c*z,.012f);};
    const auto mark=[&](float x,float z,float w,float h) {
      renderer.DrawGroundMark(point(x-w,z-h),point(x-w,z+h),point(x+w,z+h),point(x+w,z-h),{.12f,.10f,.07f,alpha});
    };
    if(e.hoof) {mark(-.023f,0,.020f,.055f);mark(.023f,0,.020f,.055f);}
    else {mark(0,-.012f,.041f,.035f);for(int i=0;i<3;++i)mark((i-1)*.028f,.044f,.016f,.018f);}
    // Brief soil flecks at a fast contact; no particles on paving or timber.
    if(print.age<.24f && e.strength>.4f && print.dirt>.25f) {
      for(int i=0;i<2;++i) {
        const float a=e.facing+i*2.4f,x=e.position.x+std::sin(a)*print.age*.35f,z=e.position.z+std::cos(a)*print.age*.35f;
        const float y=rpg::WorldMap::Height({x,z})+.018f+print.age*.65f-2.3f*print.age*print.age;
        renderer.DrawMesh(visual::Mesh::Rock,{x,y,z},{.018f,.012f,.020f},{.23f,.18f,.12f,1},visual::Material::Stone,0,0,visual::ShadowMode::None);
      }
    }
  }
  const float sx=std::sin(wake_.facing),cx=std::cos(wake_.facing);
  const auto& wakeTint=rpg::DescribeClass(wake_.school).color;
  const auto wakeColor=[&](XMFLOAT4 original) {
    if(wake_.school==rpg::ClassId::Warrior) return original;
    return XMFLOAT4{std::min(1.f,wakeTint[0]+.2f),std::min(1.f,wakeTint[1]+.2f),std::min(1.f,wakeTint[2]+.2f),original.w};
  };
  for (std::size_t i=1;i<wake_.count;++i) {
    const auto& a=wake_.samples[i-1]; const auto& b=wake_.samples[i];
    const float dx=b.position.x-a.position.x,dz=b.position.z-a.position.z,length=std::hypot(dx,dz);
    const float pathX=length>.00001f?dx/length:sx,pathZ=length>.00001f?dz/length:cx;
    for (int band=0;band<3;++band) for (int layer=0;layer<2;++layer) {
      const float side=band==0?-.31f:band==1?.31f:0,up=band==2?.38f:1.22f;
      const auto edge=[&](const WakeSample& sample,float sign) -> XMFLOAT3 {
        const float taper=Unit(1-sample.age/.20f)*std::sqrt(Unit(sample.strength));
        const float width=(layer==0?.09f:.018f)*(.12f+.88f*taper)*sign;
        const float drift=side*sample.age*1.3f;
        // Raised side strips remain readable from a low side camera; the lower
        // horizontal strip anchors the speed cue to the feet.
        const float lateral=side+drift+width*(band==2?1.f:.38f);
        return {sample.position.x+pathZ*lateral,sample.position.y+up+width*(band==2?0.f:.92f),
          sample.position.z-pathX*lateral};
      };
      const auto opacity=[](const WakeSample& sample) {
        return std::pow(Unit(1-sample.age/.20f),2.f)*std::sqrt(Unit(sample.strength));
      };
      renderer.DrawEffectRibbon(edge(a,-1),edge(a,1),edge(b,1),edge(b,-1),
        wakeColor(layer==0?XMFLOAT4{.12f,.48f,1,.38f}:XMFLOAT4{.66f,.95f,1,.75f}),opacity(a),opacity(b));
    }
  }
  // A pair of open pressure arcs travels with the chest, leaving the body clear.
  if (wake_.active && wake_.strength>.05f) for (int side : {-1,1}) {
    const auto point=[&](float u,float width) -> XMFLOAT3 {
      const float a=-1.05f+u*2.1f,lateral=side*(.39f+std::cos(a)*(.10f+width));
      const float forward=.25f+std::cos(a)*.20f;
      return {wake_.previous.x+cx*lateral+sx*forward,wake_.previous.y+1+std::sin(a)*.43f,
        wake_.previous.z-sx*lateral+cx*forward};
    };
    for (int i=0;i<14;++i) {
      const float a=float(i)/14,b=float(i+1)/14;
      renderer.DrawEffectRibbon(point(a,-.018f),point(a,.018f),point(b,.018f),point(b,-.018f),
        wakeColor({.42f,.85f,1,wake_.strength*.42f}),std::sin(a*3.14159265f),std::sin(b*3.14159265f));
    }
  }
  for (const auto& trail : trails_) {
    if (!trail.tracked && trail.age<sweepLife) {
      Crescent(renderer,trail.origin,trail.facing,trail.age,trail.kind==K::HeavyStrike,trail.kind==K::EnemySwing,false,trail.school);
      if (trail.kind==K::HeavyStrike) Crescent(renderer,trail.origin,trail.facing,trail.age,true,false,true,trail.school);
    }
    const bool heavy=trail.kind==K::HeavyStrike,hostile=trail.kind==K::EnemySwing;
    auto color=Tint(trail.kind); color.w=heavy?.58f:.38f;
    if(trail.school==rpg::ClassId::Warrior && !hostile)color=heavy?XMFLOAT4{.39f,.67f,.91f,.58f}:XMFLOAT4{.72f,.79f,.83f,.38f};
    if(trail.kind!=K::EnemySwing && trail.school!=rpg::ClassId::Warrior) color=classfx::Color(trail.school,color.w);
    for (std::size_t i=1;i<trail.count;++i) {
      const auto& a=trail.samples[i-1]; const auto& b=trail.samples[i];
      const auto delta=XMLoadFloat3(&b.tip)-XMLoadFloat3(&a.tip);
      const float travel=XMVectorGetX(XMVector3Length(delta)),dt=std::max(.0001f,a.age-b.age);
      if(travel<.0001f)continue;
      const float intensity=Unit(travel/dt/3.5f);
      const auto point=[](const Sample& s,float at){XMFLOAT3 p;XMStoreFloat3(&p,XMVectorLerp(XMLoadFloat3(&s.root),XMLoadFloat3(&s.tip),at));return p;};
      auto body=color;body.w*=intensity;
      // Fade across the blade as well as through history; the pale cutting
      // edge stays on the actual tip while a restrained veil supplies width.
      auto veil=body;veil.w*=.22f;
      renderer.DrawEffectRibbon(point(a,.45f),point(a,1.10f),point(b,1.10f),point(b,.45f),veil,
        std::pow(Unit(1-a.age/trailLife),2.f),std::pow(Unit(1-b.age/trailLife),2.f));
      if(trail.school==rpg::ClassId::Warrior && !hostile)
        renderer.DrawFilamentRibbon(point(a,.18f),a.tip,b.tip,point(b,.18f),(trail.age-a.age)*6,(trail.age-b.age)*6,trail.age,body,
          std::pow(Unit(1-a.age/trailLife),2.f),std::pow(Unit(1-b.age/trailLife),2.f));
      else renderer.DrawEffectRibbon(point(a,.18f),a.tip,b.tip,point(b,.18f),body,
          std::pow(Unit(1-a.age/trailLife),2.f),std::pow(Unit(1-b.age/trailLife),2.f));
      if(trail.school==rpg::ClassId::Warrior && trail.kind!=K::EnemySwing){
        const auto edge=[](const Sample& s){return XMFLOAT3{s.tip.x+(s.root.x-s.tip.x)*.12f,s.tip.y+(s.root.y-s.tip.y)*.12f,s.tip.z+(s.root.z-s.tip.z)*.12f};};
        renderer.DrawEffectRibbon(edge(a),a.tip,b.tip,edge(b),{.82f,.92f,1,.65f*intensity},
          std::pow(Unit(1-a.age/trailLife),3.f),std::pow(Unit(1-b.age/trailLife),3.f));
      }
    }
  }
  for (const auto& burst : bursts_) {
    if (burst.age>=BurstLife(burst.event)) continue;
    const auto& e=burst.event;
    if(HostileRelease(e.kind))continue;
    spellvfx::EnemyContact(renderer,e,burst.age);
    if(e.kind==K::BeastBite || e.kind==K::BeastCharge) {
      const bool charge=e.kind==K::BeastCharge;const float fade=Unit(1-burst.age/.45f);
      for(int i=0;i<(charge?12:5);++i) {
        const float a=e.facing+(Noise(i+burst.seed)*2-1)*1.5f;
        const float reach=.6f+burst.age*(charge?2.2f:1.2f);
        const float x=e.position.x+std::sin(a)*reach,z=e.position.z+std::cos(a)*reach;
        const float y=rpg::WorldMap::Height({x,z})+(charge?.14f:.65f)+burst.age*.6f;
        renderer.DrawEffectGlow({x,y,z},charge?.10f:.045f,charge?XMFLOAT4{.43f,.34f,.22f,fade*.45f}:XMFLOAT4{.76f,.56f,.37f,fade*.55f});
      }
      continue;
    }
    if(e.kind==K::CastStart) continue;
    if(e.kind==K::SpellCast) {
      classfx::Projectile(renderer,e,burst.spellOrigin,burst.age,Path(burst));
      continue;
    }
    if(e.kind==K::DisciplineReady || e.kind==K::DisciplineSpent){castfx::DisciplinePulse(renderer,e,burst.age,caster);continue;}
    if(e.kind==K::BarrierHit) { classfx::BarrierContact(renderer,e,burst.age);continue; }
    if(e.kind==K::Interrupt && e.actor!=0) {spellvfx::Interrupt(renderer,e,burst.age);continue;}
    if(e.kind==K::Hit || e.kind==K::HeavyHit) {
      classfx::Impact(renderer,e,burst.age);continue;
    }
    if (e.kind==K::Mend) {
      if(caster)castfx::Complete(renderer,e,burst.age,*caster);
      else classfx::Recovery(renderer,e,burst.age);
      continue;
    }
    if (e.kind==K::Potion) {
      if(caster){if(caster->alive){auto attached=e;attached.position={caster->root.x,caster->root.z};attached.height=caster->root.y;HealingPulse(renderer,attached,burst.age);}}
      else HealingPulse(renderer,e,burst.age);
      continue;
    }
    if (Dashing(e.kind)) { DashPulse(renderer,e,burst.age); continue; }
    const bool heavy=e.kind==K::HeavyHit,interrupt=e.kind==K::Interrupt,hurt=e.kind==K::Hurt;
    const float t=burst.age,fade=std::pow(Unit(1-t/burstLife),2.f);
    // Move the impact onto the struck surface, facing the attacker.
    const XMFLOAT3 origin{e.position.x-std::sin(e.facing)*.34f,e.height,e.position.z-std::cos(e.facing)*.34f};
    auto color=Tint(e.kind);
    const float flash=std::pow(Unit(1-t/.20f),2.f);
    // A brief pale contact core, followed by the coloured halo and fragments.
    // Timing comes from the weapon contact event, never the initial key press.
    const float core=std::pow(Unit(1-t/.075f),2.f);
    if (!hurt) renderer.DrawEffectGlow(origin,heavy?.27f:.17f,{1,.94f,.77f,core*.9f});
    color.w=(heavy?.78f:.60f)*flash;
    renderer.DrawEffectGlow(origin,(heavy?.52f:hurt?.25f:.35f)+t*.6f,color);
    color.w=(heavy?.32f:.18f)*flash;
    renderer.DrawEffectGlow(origin,(heavy?1.1f:.65f)+t*.4f,color);
    if (!hurt && !interrupt) {
      color.w=(heavy?.82f:.5f)*std::pow(Unit(1-t/.22f),1.5f);
      renderer.DrawEffectRing(origin,(heavy?.28f:.19f)+t*(heavy?3.5f:2.5f),color);
      // A small star at contact provides a crisp hit amid the larger soft halo.
      color.w=flash*.85f;
      const float span=heavy?.48f:.3f;
      renderer.DrawEffectStreak({origin.x-span,origin.y,origin.z},{origin.x+span,origin.y,origin.z},.035f,color);
      renderer.DrawEffectStreak({origin.x,origin.y-span*.7f,origin.z},{origin.x,origin.y+span*.7f,origin.z},.025f,color);
    }
    if (interrupt || heavy) {
      // A delayed violet pulse distinguishes a broken cast from a normal hit.
      const float phase=Unit((t-.04f)/.30f);
      auto pulse=interrupt?XMFLOAT4{.65f,.22f,1,1}:XMFLOAT4{.25f,.45f,1,1};
      pulse.w=std::sin(phase*3.14159265f)*(interrupt?.38f:.18f);
      renderer.DrawEffectRing(origin,.18f+phase*(interrupt?.85f:.65f),pulse);
    }
    if (heavy && e.height-rpg::WorldMap::Height(e.position)<1.7f) {
      // A low pressure skirt in the ground plane anchors the impact to the
      // target. The transparent centre keeps boots and collision cues visible.
      const float radius=.25f+1.3f*std::sqrt(Unit(t/.48f));
      for(int i=0;i<24;++i) {
        const float a=i*motion::tau/24,b=(i+1)*motion::tau/24;
        const auto point=[&](float angle,float r) -> XMFLOAT3 {
          return GroundPoint(e.position.x+std::cos(angle)*r,e.position.z+std::sin(angle)*r,.035f);
        };
        renderer.DrawEffectRibbon(point(a,radius),point(a,radius+.10f),point(b,radius+.10f),point(b,radius),
          {.18f,.46f,.64f,fade*.27f},1,1);
      }
    }
    const int count=interrupt?10:heavy?12:hurt?6:8;
    for (int i=0;i<count;++i) {
      const float angle=6.2831853f*(float(i)/count+Noise(burst.seed*71+i*11)*.10f);
      const float speed=(heavy?3.3f:2.2f)*(.65f+Noise(burst.seed*23+i*17)*.6f);
      const float lateral=std::cos(angle)*speed;
      const float outward=speed*(.35f+.45f*Noise(burst.seed*59+i*7));
      const float impactSin=std::sin(e.facing),impactCos=std::cos(e.facing);
      const XMFLOAT3 velocity=interrupt?
        XMFLOAT3{std::cos(angle)*speed,2.6f+Noise(burst.seed*37+i*13),std::sin(angle)*speed}:
        XMFLOAT3{impactCos*lateral-impactSin*outward,.7f+Noise(burst.seed*37+i*13)*1.5f,-impactSin*lateral-impactCos*outward};
      const auto at=[&](float age) -> XMFLOAT3 {
        const float travel=-std::expm1(-3.f*age)/3.f;
        return {origin.x+velocity.x*travel,origin.y+velocity.y*age-3.2f*age*age,origin.z+velocity.z*travel};
      };
      const auto cooled=heavy?XMFLOAT4{.46f,.24f,1,1}:interrupt?XMFLOAT4{.4f,.12f,.8f,1}:XMFLOAT4{1,.27f,.04f,1};
      const auto hot=Tint(e.kind); const float cooling=Unit(t/burstLife);
      color={hot.x+(cooled.x-hot.x)*cooling,hot.y+(cooled.y-hot.y)*cooling,hot.z+(cooled.z-hot.z)*cooling,1};
      color.w=fade*(heavy?.9f:.7f);
      const float width=(heavy?.038f:.024f)*(1-t/burstLife*.5f);
      renderer.DrawEffectStreak(at(std::max(0.f,t-.065f)),at(t+.016f),width,color);
      color.w=fade*.55f;
      renderer.DrawEffectGlow(at(t),width*2.1f,color);
    }
    // Small rising motes form a soft afterglow; no emitter simulation or texture.
    if (!hurt) for (int i=0;i<(heavy?6:3);++i) {
      const float angle=Noise(burst.seed*43+i*19)*6.2831853f;
      const float radius=.08f+t*(.65f+Noise(burst.seed*13+i)*.5f);
      const XMFLOAT3 mote{origin.x+std::cos(angle)*radius,origin.y+t*(.7f+Noise(burst.seed*31+i)),origin.z+std::sin(angle)*radius};
      auto tint=heavy && i%2?XMFLOAT4{.65f,.25f,1,1}:Tint(e.kind);
      tint.w=std::sin(Unit(t/burstLife)*3.14159265f)*.42f;
      renderer.DrawEffectGlow(mote,.045f+Noise(burst.seed+i*7)*.035f,tint);
    }
  }
  // Last cosmetic priority: never displace current danger, release or hit
  // geometry. At most six recent patches share a 900-vertex allowance.
  std::array<const spellvfx::Residue*,12> residueOrder{};std::size_t residueCount=0;
  for(const auto& residue:residues_)if(residue.age<spellvfx::ResidueLifetime(residue.kind))residueOrder[residueCount++]=&residue;
  std::sort(residueOrder.begin(),residueOrder.begin()+residueCount,[](const auto* a,const auto* b){
    return a->age==b->age?a->order>b->order:a->age<b->age;
  });
  const auto residueLimit=std::min(Renderer::maxEffectVertices,renderer.EffectVertices()+900);
  for(std::size_t i=0;i<std::min<std::size_t>(6,residueCount) && renderer.EffectVertices()+180<=residueLimit;++i)
    spellvfx::Aftermath(renderer,*residueOrder[i]);
}
