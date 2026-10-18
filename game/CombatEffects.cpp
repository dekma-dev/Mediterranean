#include "game/CombatEffects.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace {
using K=rpg::CombatVisualKind;
using namespace DirectX;
constexpr float trailLife=.16f,burstLife=.48f,sweepLife=.32f;
bool Healing(K kind) { return kind==K::Mend || kind==K::Potion; }
float BurstLife(K kind) { return Healing(kind)?.8f:burstLife; }
float Unit(float x) { return std::clamp(x,0.f,1.f); }
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
// A tapered, curved slash envelope. Its two ribbons share the normal effect
// batch: a colored halo around a thin bright edge, without a bloom render target.
void Crescent(Renderer& renderer,XMFLOAT3 origin,float facing,float age,bool heavy,bool enemy,bool echo) {
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
void CombatEffects::Reset() { bursts_={}; trails_={}; serial_=0; }
void CombatEffects::Advance(float dt) {
  if (!std::isfinite(dt) || dt<=0) return;
  dt=std::min(dt,.05f);
  for (auto& burst : bursts_) burst.age=std::min(1.f,burst.age+dt);
  for (auto& trail : trails_) {
    trail.age=std::min(1.f,trail.age+dt);
    for (std::size_t i=0;i<trail.count;++i) trail.samples[i].age+=dt;
    const auto end=std::remove_if(trail.samples.begin(),trail.samples.begin()+trail.count,
      [](const Sample& sample) { return sample.age>=trailLife; });
    trail.count=std::size_t(end-trail.samples.begin());
  }
}
void CombatEffects::Emit(const rpg::CombatVisualEvent& event) {
  if (event.kind==K::Strike || event.kind==K::HeavyStrike || event.kind==K::EnemySwing) {
    auto slot=std::find_if(trails_.begin(),trails_.end(),[&](const Trail& trail) { return trail.actor==event.actor && trail.age<.4f; });
    if (slot==trails_.end()) slot=std::max_element(trails_.begin(),trails_.end(),[](const Trail& a,const Trail& b) { return a.age<b.age; });
    *slot={}; slot->actor=event.actor; slot->kind=event.kind; slot->age=0;
    slot->origin={event.position.x,event.height,event.position.z}; slot->facing=event.facing;
    return;
  }
  auto slot=std::max_element(bursts_.begin(),bursts_.end(),[](const Burst& a,const Burst& b) {
    return a.age/BurstLife(a.event.kind)<b.age/BurstLife(b.event.kind);
  });
  *slot={event,0,++serial_};
}
bool CombatEffects::Tracking(rpg::EntityId actor) const {
  for (const auto& trail : trails_) if (trail.actor==actor && trail.age<.24f) return true;
  return false;
}
void CombatEffects::TrackWeapon(rpg::EntityId actor,XMFLOAT3 root,XMFLOAT3 tip) {
  for (auto& trail : trails_) if (trail.actor==actor && trail.age<.24f) {
    // A dash/teleport must not stretch a blade trail across the map.
    if (trail.count) {
      const auto delta=XMLoadFloat3(&tip)-XMLoadFloat3(&trail.samples[trail.count-1].tip);
      if (XMVectorGetX(XMVector3LengthSq(delta))>4) { trail=Trail{}; return; }
    }
    // Retain a live endpoint but space committed samples by time, not frames.
    // Otherwise >200 Hz updates fill all 32 slots before the 160 ms trail ends.
    constexpr float sampleInterval=1.f/120;
    if (trail.count>=2 && trail.samples[trail.count-2].age<sampleInterval) {
      trail.samples[trail.count-1]={root,tip,0};
      return;
    }
    if (trail.count==trail.samples.size()) {
      std::move(trail.samples.begin()+1,trail.samples.end(),trail.samples.begin()); --trail.count;
    }
    trail.samples[trail.count++]={root,tip,0};
    return;
  }
}
void CombatEffects::CancelTrail(rpg::EntityId actor) {
  for (auto& trail : trails_) if (trail.actor==actor) trail=Trail{};
}
void CombatEffects::CancelTrails() { trails_={}; }
std::size_t CombatEffects::BurstCount() const {
  return std::count_if(bursts_.begin(),bursts_.end(),[](const Burst& burst) { return burst.age<BurstLife(burst.event.kind); });
}
std::size_t CombatEffects::TrailCount() const {
  return std::count_if(trails_.begin(),trails_.end(),[](const Trail& trail) { return trail.age<sweepLife || trail.count>1; });
}
void CombatEffects::Render(Renderer& renderer) const {
  for (const auto& trail : trails_) {
    if (trail.age<sweepLife) {
      Crescent(renderer,trail.origin,trail.facing,trail.age,trail.kind==K::HeavyStrike,trail.kind==K::EnemySwing,false);
      if (trail.kind==K::HeavyStrike) Crescent(renderer,trail.origin,trail.facing,trail.age,true,false,true);
    }
    auto color=Tint(trail.kind); color.w=trail.kind==K::HeavyStrike?.8f:.48f;
    for (std::size_t i=1;i<trail.count;++i) {
      const auto& a=trail.samples[i-1]; const auto& b=trail.samples[i];
      renderer.DrawEffectRibbon(a.root,a.tip,b.tip,b.root,color,
        std::pow(Unit(1-a.age/trailLife),2.f),std::pow(Unit(1-b.age/trailLife),2.f));
    }
  }
  for (const auto& burst : bursts_) {
    if (burst.age>=BurstLife(burst.event.kind)) continue;
    const auto& e=burst.event;
    if (Healing(e.kind)) { HealingPulse(renderer,e,burst.age); continue; }
    const bool heavy=e.kind==K::HeavyHit,interrupt=e.kind==K::Interrupt,hurt=e.kind==K::Hurt;
    const float t=burst.age,fade=std::pow(Unit(1-t/burstLife),2.f);
    // Move the impact onto the struck surface, facing the attacker.
    const XMFLOAT3 origin{e.position.x-std::sin(e.facing)*.34f,e.height,e.position.z-std::cos(e.facing)*.34f};
    auto color=Tint(e.kind);
    const float flash=std::pow(Unit(1-t/.20f),2.f);
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
    const int count=interrupt?10:heavy?12:hurt?6:8;
    for (int i=0;i<count;++i) {
      const float angle=6.2831853f*(float(i)/count+Noise(burst.seed*71+i*11)*.10f);
      const float speed=(heavy?3.3f:2.2f)*(.65f+Noise(burst.seed*23+i*17)*.6f);
      const float lateral=std::cos(angle)*speed;
      const float outward=speed*(.35f+.45f*Noise(burst.seed*59+i*7));
      const float sx=std::sin(e.facing),cx=std::cos(e.facing);
      const XMFLOAT3 velocity=interrupt?
        XMFLOAT3{std::cos(angle)*speed,2.6f+Noise(burst.seed*37+i*13),std::sin(angle)*speed}:
        XMFLOAT3{cx*lateral-sx*outward,.7f+Noise(burst.seed*37+i*13)*1.5f,-sx*lateral-cx*outward};
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
}
