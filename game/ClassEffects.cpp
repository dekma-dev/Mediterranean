#include "game/ClassEffects.h"
#include "game/SpellVfx.h"
#include "game/SustainedEffects.h"
#include "game/CombatEffects.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace classfx {
using namespace DirectX;
using C=rpg::ClassId;
namespace {
constexpr float pi=3.14159265f,tau=2*pi;
float Unit(float t) { return std::clamp(t,0.f,1.f); }
XMFLOAT3 Add(XMFLOAT3 a,XMFLOAT3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
XMFLOAT3 Lerp(XMFLOAT3 a,XMFLOAT3 b,float t) { return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t}; }
// Open contours, rather than filled discs, preserve silhouettes and telegraphs.
void Arc(Renderer& r,XMFLOAT3 p,float radius,float start,float span,float width,XMFLOAT4 color,
         bool ground=false,float tilt=0,bool conform=false) {
  constexpr int segments=20;
  const auto point=[&](float a,float offset) -> XMFLOAT3 {
    const float x=std::cos(a)*(radius+offset),v=std::sin(a)*(radius+offset);
    auto result=Add(p,ground?XMFLOAT3{x,v*tilt,v}:XMFLOAT3{x,v,v*tilt});
    if(conform) result.y+=rpg::WorldMap::Height({result.x,result.z})-rpg::WorldMap::Height({p.x,p.z});
    return result;
  };
  for(int i=0;i<segments;++i) {
    const float a=start+span*float(i)/segments,b=start+span*float(i+1)/segments;
    r.DrawEffectRibbon(point(a,-width),point(a,width),point(b,width),point(b,-width),color,1,1);
  }
}
XMFLOAT3 Floor(rpg::Vec2 p,float lift=.06f) {return {p.x,rpg::WorldMap::Height(p)+lift,p.z};}
void GroundArc(Renderer& r,XMFLOAT3 p,float radius,float start,float span,float width,XMFLOAT4 color) {
  Arc(r,p,radius,start,span,width,color,true,0,true);
}
void Ray(Renderer& r,XMFLOAT3 p,float angle,float inner,float outer,float width,XMFLOAT4 c) {
  r.DrawEffectStreak(Add(p,{std::cos(angle)*inner,std::sin(angle)*inner,0}),
    Add(p,{std::cos(angle)*outer,std::sin(angle)*outer,0}),width,c);
}
void Note(Renderer& r,XMFLOAT3 p,float size,XMFLOAT4 c) {
  r.DrawEffectRing(p,size*.30f,c);
  r.DrawEffectStreak(Add(p,{size*.22f,0,0}),Add(p,{size*.22f,size,0}),size*.065f,c);
  r.DrawEffectStreak(Add(p,{size*.22f,size,0}),Add(p,{size*.65f,size*.78f,0}),size*.085f,c);
}
void Bolt(Renderer& r,XMFLOAT3 a,XMFLOAT3 b,float spread,float phase,XMFLOAT4 c) {
  XMFLOAT3 previous=a;
  for(int i=1;i<=9;++i) {
    const float u=float(i)/9;
    auto next=Lerp(a,b,u);
    next.x+=std::sin(u*pi)*std::sin(i*2.7f+phase)*spread;
    next.z+=std::sin(u*pi)*std::cos(i*4.1f+phase)*spread;
    r.DrawEffectStreak(previous,next,.022f,c);previous=next;
  }
}
}
XMFLOAT4 Color(C school,float alpha) {
  switch(school) {
    case C::Warrior:return {1,.61f,.22f,alpha};
    case C::Paladin:return {1,.78f,.27f,alpha};
    case C::Scout:return {.65f,.87f,.76f,alpha};
    case C::Priest:return {1,.91f,.63f,alpha};
    case C::Druid:return {.28f,.93f,.62f,alpha};
    case C::Mage:return {1,.30f,.065f,alpha};
    case C::Necromancer:return {.36f,.95f,.68f,alpha};
    case C::Psionic:return {.69f,.42f,1,alpha};
    case C::Bard:return {.35f,.78f,1,alpha};
    case C::Engineer:return {1,.67f,.30f,alpha};
    case C::Demonologist:return {1,.16f,.33f,alpha};
    default:return {1,1,1,alpha};
  }
}
void Projectile(Renderer& r,const rpg::CombatVisualEvent& e,XMFLOAT3 origin,float age) {
  spellvfx::Flight(r,e,origin,age);
  const bool physical=e.school==C::Scout || e.school==C::Engineer;
  const float flight=std::max(.01f,e.flight),t=Unit(age/flight);
  const float fade=1-Unit((age-flight)/(physical?.045f:.10f));
  if(fade<=0) return;
  const float power=e.heavy?1.3f:1,arc=e.school==C::Mage?.28f:physical?0.f:.12f;
  const auto point=[&](float u) { return CombatEffects::SpellTracePoint(origin,e.destination,Unit(u),arc); };
  auto c=Color(e.school,fade*.72f);
  const auto head=point(t);
  if(e.ability>=12 && e.school==C::Mage) {
    if(e.ability==12) {
      for(int n=-1;n<=1;++n) {
        const XMFLOAT3 offset{std::cos(e.facing)*n*.065f,.025f*std::abs(n),-std::sin(e.facing)*n*.065f};
        r.DrawEffectStreak(Add(point(t-.16f),offset),Add(head,offset),.012f,{.6f,.86f,1,fade*.85f});
      }
    } else if(e.ability==13) {
      Arc(r,head,.13f,age*9,tau,.015f,{.7f,.43f,1,fade*.7f});
      Bolt(r,point(t-.24f),head,.07f,age*13,{.84f,.62f,1,fade*.65f});
    } else {
      for(int side:{-1,1}) for(int n=0;n<9;++n) {
        const float a=std::max(0.f,t-.3f+n*.3f/9),b=std::min(t,a+.3f/9);
        const auto spiral=[&](float u) {return Add(point(u),{std::cos(e.facing)*std::sin(u*22)*side*.10f,std::cos(u*22)*side*.10f,-std::sin(e.facing)*std::sin(u*22)*side*.10f});};
        r.DrawEffectStreak(spiral(a),spiral(b),.022f,side<0?XMFLOAT4{1,.35f,.08f,fade*.7f}:XMFLOAT4{.42f,.78f,1,fade*.8f});
      }
      r.DrawEffectGlow(head,.075f,{1,.90f,.8f,fade*.65f});
    }
    return;
  }
  if(e.school==C::Mage && e.ability==10) {
    const XMFLOAT4 violet{.68f,.55f,1,fade*.8f};
    Bolt(r,point(t-.23f),head,.08f,t*13,violet);
    Arc(r,head,.10f,age*9,pi*1.5f,.013f,violet);return;
  }
  if((e.technique || e.signature || e.ability==8) && e.school==C::Mage) {
    c={.35f,.78f,1.f,fade*.8f};
    r.DrawEffectStreak(point(t-.17f),head,.035f,c);
    for(int n=0;n<3;++n) r.DrawEffectStreak(Add(head,{(n-1)*.05f,.06f,0}),point(t-.07f),.012f,c);
    return;
  }
  if(physical) {
    if(e.school==C::Scout && e.ability>=12) {
      const int count=e.ability==12?2:e.ability==14?3:1;
      for(int n=0;n<count;++n) {
        const float spread=(n-(count-1)*.5f)*.16f*(1-t);
        const XMFLOAT3 offset{std::cos(e.facing)*spread,0,-std::sin(e.facing)*spread};
        const auto tip=Add(head,offset),tail=Add(point(t-.12f),offset);
        r.DrawEffectStreak(tail,tip,.012f,{.92f,.85f,.65f,fade});
        for(float side:{-1.f,1.f})r.DrawEffectStreak(Add(tail,{std::cos(e.facing)*side*.04f,.03f,-std::sin(e.facing)*side*.04f}),Lerp(tail,tip,.25f),.009f,c);
      }
      if(e.ability==13) Arc(r,head,.075f,age*8,pi*1.5f,.009f,{.65f,.74f,.34f,fade*.5f});
      return;
    }
    const auto tail=point(t-.08f);
    r.DrawEffectStreak(tail,head,.012f*power,{1,.87f,.64f,fade});
    if(e.school==C::Scout) {
      // A solid, narrow bolt and paired fletching; no spell orb on the handbow.
      for(float side:{-1.f,1.f}) r.DrawEffectStreak(Add(tail,{side*.055f,.02f,0}),point(t-.045f),.009f,c);
      if(e.heavy) r.DrawEffectStreak(point(t-.14f),head,.025f,Color(e.school,fade*.23f));
      if(e.ability==11) r.DrawEffectStreak(point(t-.5f),head,.018f,{.9f,.94f,1,fade*.7f});
    } else {
      const float flash=std::pow(1-Unit(age/.065f),2.f);
      r.DrawEffectGlow(origin,.19f*power,{1,.68f,.26f,flash*.8f});
      for(int i=0;i<5;++i) Ray(r,origin,float(i)*tau/5,.025f,.22f*power,.016f,{1,.78f,.48f,flash});
      r.DrawEffectGlow(head,.035f,c);
    }
    return;
  }
  if(e.school==C::Druid) {
    Bolt(r,point(t-.28f),head,.12f*power,t*9,c);
    r.DrawEffectGlow(head,.065f,c);
  } else if(e.school==C::Priest) {
    r.DrawEffectStreak(point(t-.35f),head,.026f*power,{1,.95f,.78f,fade});
    Arc(r,head,.10f*power,0,tau,.012f,c);
    r.DrawEffectGlow(head,.11f,Color(e.school,fade*.42f));
  } else if(e.school==C::Bard) {
    for(int i=0;i<3;++i) {
      const float u=std::max(0.f,t-i*.095f);
      Arc(r,point(u),(.07f+i*.045f)*power,-.9f,1.8f,.015f,Color(e.school,fade*(.65f-i*.12f)));
    }
    Note(r,head,.18f*power,c);
  } else if(e.school==C::Psionic) {
    for(int side:{-1,1}) {
      const auto lens=Add(head,{side*.085f,0,0});
      Arc(r,lens,.14f*power,t*5+side*.7f,pi*1.4f,.018f,c,false,side*.5f);
    }
    r.DrawEffectStreak(point(t-.23f),head,.025f,Color(e.school,fade*.4f));
  } else {
    // A rising ember tail for fire, twin orbiting soul strands for necromancy.
    const bool soul=e.school==C::Necromancer;
    const int strands=soul?2:1;
    for(int strand=0;strand<strands;++strand) for(int i=0;i<12;++i) {
      const float a=std::max(0.f,t-.38f+.38f*i/12),b=std::max(0.f,t-.38f+.38f*(i+1)/12);
      const auto at=[&](float u) {
        auto p=point(u);const float behind=Unit((t-u)/.38f);
        if(soul) {p.x+=std::sin(u*24+strand*pi)*.12f*behind;p.y+=std::cos(u*24+strand*pi)*.12f*behind;}
        else p.y+=behind*behind*.20f;
        return p;
      };
      auto tail=c;tail.w*=float(i+1)/12;
      r.DrawEffectStreak(at(a),at(b),(.018f+.027f*float(i)/12)*power,tail);
    }
    r.DrawEffectGlow(head,.15f*power,Color(e.school,fade*.55f));
    r.DrawEffectGlow(head,.045f*power,{1,.93f,.66f,fade*.9f});
    if(!soul) for(int i=0;i<4;++i) {
      auto ember=point(t-i*.055f);ember.y+=std::sin(age*22+i)*.065f;
      ember.x+=std::cos(i*2.4f)*.075f;
      r.DrawEffectGlow(ember,.025f,Color(e.school,fade*.7f));
    }
  }
  if(e.empowered || e.signature) Arc(r,head,.23f,age*12,pi*1.5f,.014f,Color(e.school,fade*.55f));
}
void Impact(Renderer& r,const rpg::CombatVisualEvent& e,float age) {
  spellvfx::Contact(r,e,age);
  const float phase=Unit(age/.48f),fade=(1-phase)*(1-phase);
  const float flash=std::pow(1-Unit(age/.095f),2.f);
  const float size=(e.heavy?1.1f:.72f)*(e.empowered?1.2f:1);
  const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.34f,e.height,e.position.z-std::cos(e.facing)*.34f};
  auto c=(e.technique || e.signature || e.ability==8) && e.school==C::Mage?XMFLOAT4{.35f,.78f,1.f,fade*.8f}:Color(e.school,fade*.8f);
  if(e.chain>=2) {
    for(int n=0;n<e.chain;++n) GroundArc(r,Floor(e.position),.38f+phase*.65f,n*tau/e.chain+phase*2,.65f,.018f,{1,.82f,.43f,fade*.5f});
  }
  if(e.ability>=12) {
    const float sn=std::sin(e.facing),cs=std::cos(e.facing);
    const auto local=[&](float x,float y,float z=0.f){return Add(p,{cs*x+sn*z,y,-sn*x+cs*z});};
    if(e.school==C::Warrior) {
      const int cuts=e.ability==14?2:1;
      for(int n=0;n<cuts;++n) {
        const float side=n?-1.f:1.f;
        r.DrawEffectStreak(local(-.42f,-.32f*side),local(.42f,.32f*side),.022f,{.85f,.91f,1,fade*.8f});
      }
      for(int n=0;n<9;++n) {
        const float a=n*2.39996f,speed=.6f+(n%3)*.4f;
        r.DrawEffectStreak(local(std::cos(a)*age*speed,std::sin(a)*age*speed-2*age*age),
          local(std::cos(a)*(age+.045f)*speed,std::sin(a)*(age+.045f)*speed-2*age*age),.010f,{1,.65f,.25f,fade*.8f});
      }
      if(e.ability==13) Arc(r,p,.2f+phase*.3f,pi*.15f,pi*.7f,.024f,{.72f,.83f,.95f,fade*.7f});
    } else if(e.school==C::Scout) {
      for(int n=0;n<(e.ability==14?3:2);++n) {
        const float x=(n-1)*.09f;
        r.DrawEffectStreak(local(x,-.08f,-.30f),local(x,.08f,.12f),.015f,{.85f,.89f,.65f,fade*.85f});
      }
      if(e.ability==13) for(int n=0;n<3;++n) GroundArc(r,Floor(e.position,.14f+n*.15f),.32f+phase*.15f,n*2.1f,pi*1.2f,.013f,{.60f,.68f,.30f,fade*.7f});
    } else {
      const bool prism=e.ability==13,split=e.ability==14;
      if(prism) {
        for(int n=0;n<4;++n) {const float a=n*pi*.5f+phase;Ray(r,p,a,.06f,.20f+phase*.4f,.018f,{.73f,.48f,1,fade*.85f});}
        Arc(r,p,.12f+phase*.5f,phase*4,tau,.014f,{.65f,.40f,1,fade*.7f});
      } else for(int n=0;n<(split?12:6);++n) {
        const float a=n*2.39996f,rad=.12f+phase*(split?1.3f:.6f);
        const auto tip=local(std::cos(a)*rad,std::sin(a)*rad+phase*.25f);
        r.DrawEffectStreak(Lerp(p,tip,.55f),tip,split?.024f:.012f,
          split && n%2?XMFLOAT4{1,.39f,.09f,fade*.8f}:XMFLOAT4{.55f,.84f,1,fade*.85f});
      }
      if(split) {
        // Six curved, counter-rotating plumes have a soft halo and a narrow
        // bright core. Growth and curling use the accepted contact age only.
        for(int petal=0;petal<6;++petal)for(int segment=0;segment<9;++segment) {
          const auto plume=[&](float u,float width) {
            const float angle=petal*tau/6+(petal%2?1:-1)*u*.85f;
            const float radius=(.35f+phase*2.6f)*u;
            return local(std::cos(angle)*radius-std::sin(angle)*width,
              std::sin(angle)*radius+std::cos(angle)*width+phase*.15f,std::sin(u*pi)*.13f);
          };
          const float a=segment/9.f,b=(segment+1)/9.f;
          const auto tint=petal%2?XMFLOAT4{1,.32f,.05f,fade*.72f}:XMFLOAT4{.25f,.65f,1,fade*.78f};
          const auto ribbon=[&](float width,float tip,XMFLOAT4 color) {
            const float wa=width*(1-a)+tip,wb=width*(1-b)+tip;
            r.DrawEffectRibbon(plume(a,-wa),plume(a,wa),plume(b,wb),plume(b,-wb),color,1-a,1-b);
          };
          // A continuous ribbon avoids the dotted gaps caused by restarting
          // a streak's endpoint fade on every short curved segment.
          ribbon(.17f,.025f,tint);
          ribbon(.022f,.004f,petal%2?XMFLOAT4{1,.78f,.4f,fade*.8f}:XMFLOAT4{.8f,.94f,1,fade*.85f});
        }
        r.DrawEffectGlow(p,.27f*(1-phase),{.65f,.77f,1,fade*.26f});
      }
      r.DrawEffectGlow(p,split?.22f:.10f,{.86f,.91f,1,flash*.65f});
    }
    return;
  }
  if(e.ability==10) {
    const auto tint=e.school==C::Mage?XMFLOAT4{.68f,.55f,1,fade*.8f}:XMFLOAT4{.82f,.86f,.74f,fade*.8f};
    // Control contacts stay small, keeping an interrupted foe's pose readable.
    for(int i=0;i<4;++i) Ray(r,p,i*pi*.5f+pi*.25f,.08f+phase*.12f,.18f+phase*.4f,.02f,tint);
    if(e.school==C::Mage) Arc(r,p,.38f*(1-phase)+.08f,phase*3,tau,.018f,tint);
    r.DrawEffectGlow(p,.14f,{1,.95f,.8f,flash*.65f});return;
  }
  if(e.ability==11) {
    if(e.school==C::Mage) {
      const auto floor=Floor(e.position);
      GroundArc(r,floor,.4f+phase*4.1f,0,tau,.035f,{1,.43f,.15f,fade*.65f});
      for(int i=0;i<12;++i) {
        const float a=i*2.39996f+phase,rad=.45f+(i%4)*.85f;
        const auto base=Floor({e.position.x+std::cos(a)*rad,e.position.z+std::sin(a)*rad},.08f);
        const auto tip=Add(base,{std::sin(a+phase*3)*.35f,(.7f+(i%3)*.4f)*(1-phase),std::cos(a)*.22f});
        r.DrawEffectStreak(base,tip,.13f,{.92f,.27f,.06f,fade*.27f});
        r.DrawEffectStreak(base,tip,.027f,{1,.78f,.4f,fade*.8f});
      }
      r.DrawEffectGlow(p,.25f,{1,.77f,.4f,flash*.7f});
    } else if(e.school==C::Warrior) {
      const auto center=Floor(e.source,.85f);
      for(int i=0;i<3;++i) Arc(r,center,1.4f+phase*2.4f,i*tau/3+phase*3,1.2f,.035f,
        {.75f,.84f,.92f,fade*.65f},true,.04f,true);
    } else {
      const XMFLOAT3 from{e.source.x,e.sourceHeight+1.35f,e.source.z};
      const auto through=Lerp(from,p,1.12f);
      r.DrawEffectStreak(Lerp(from,p,.8f),through,.018f,{.87f,.94f,1,fade*.8f});
      for(int i=0;i<6;++i) Ray(r,p,i*tau/6,.1f,.2f+phase*.6f,.012f,{.9f,.84f,.66f,fade*.7f});
    }
    return;
  }
  if(e.ability>=8) {
    const XMFLOAT3 floor{e.position.x,rpg::WorldMap::Height(e.position)+.07f,e.position.z};
    if(e.school==C::Mage) {
      const XMFLOAT4 tint=e.ability==8?XMFLOAT4{.40f,.78f,1,fade*.82f}:XMFLOAT4{1,.36f,.08f,fade*.82f};
      GroundArc(r,floor,.25f+phase*2.75f,0,tau,.055f,tint);
      for(int n=0;n<10;++n) {
        const float a=n*tau/10,rad=.4f+phase*1.9f;
        auto tip=Add(floor,{std::cos(a)*rad,(e.ability==8?.7f:1.7f)*(1-phase),std::sin(a)*rad});
        tip.y+=rpg::WorldMap::Height({tip.x,tip.z})-rpg::WorldMap::Height(e.position);
        r.DrawEffectStreak(Add(tip,{0,e.ability==8?-.4f:1.4f,0}),tip,.045f,tint);
      }
      if(e.ability==8) {
        r.DrawEffectGlow(p,.18f,{.65f,.9f,1,flash*.8f});
        return; // Frost bloom must not inherit the default fire impact layer.
      }
    } else if(e.school==C::Warrior) {
      Arc(r,Add(floor,{0,.85f,0}),.8f+phase*1.5f,e.facing-1.2f,2.4f,.035f,{1,.66f,.3f,fade*.65f});
    } else {
      for(int n=-1;n<=1;++n) {
        const auto tip=Add(p,{std::cos(e.facing)*n*.16f,phase*.25f,-std::sin(e.facing)*n*.16f});
        r.DrawEffectStreak(Add(tip,{-std::sin(e.facing)*.8f,0,-std::cos(e.facing)*.8f}),tip,.032f,{.75f,.9f,.50f,fade*.8f});
      }
    }
  }
  if(e.opening) {
    for(int n=0;n<3;++n) {
      const float angle=n*tau/3+e.facing;
      Ray(r,p,angle,.10f+phase*.2f,.22f+phase*.4f,.012f,c);
    }
  }
  if(e.signature) {
    // Class-colored paired crescents and rising motes mark the spent reserve.
    // Fixed segment counts keep contact cost independent of frame rate.
    const auto ground=Floor(e.position);
    for(int side=0;side<2;++side) GroundArc(r,ground,.35f+phase*.85f,side*pi+phase*2,pi*.7f,.024f,c);
    for(int n=0;n<6;++n) {
      const float angle=n*tau/6;
      const auto mote=Add(p,{std::cos(angle)*phase*.65f,phase*.65f,std::sin(angle)*phase*.65f});
      r.DrawEffectStreak(mote,Add(mote,{0,.12f,0}),.018f,c);
    }
  }
  if(e.technique) {
    if(e.school==C::Mage) c={.35f,.78f,1.f,fade*.8f};
    const auto ground=Floor(e.position,.055f);
    // A short broken seal identifies the tactical contact without hiding feet.
    for(int n=0;n<3;++n) GroundArc(r,ground,.42f+phase*.30f,n*tau/3+phase,.65f,.012f,c);
  }
  if((e.technique || e.signature) && e.school==C::Mage) {
    for(int n=0;n<8;++n) Ray(r,p,n*tau/8,.10f+phase*.18f,.28f+phase*.65f,.016f,c);
    r.DrawEffectGlow(p,.28f,{.6f,.88f,1.f,flash*.65f});return;
  }
  const float bloom=std::sin(phase*pi);
  const int facets=3+int(e.school)%4;
  for(int i=0;i<facets;++i) {
    const float a=i*tau/facets+e.facing+int(e.school)*.31f;
    const float radius=(.15f+phase*.65f)*size;
    const auto tip=Add(p,{std::cos(a)*radius,(.3f+std::sin(a)*.4f)*phase,std::sin(a)*radius});
    auto accent=Color(e.school,bloom*.38f);
    r.DrawEffectStreak(Lerp(p,tip,.64f),tip,.012f+flash*.015f,accent);
  }
  r.DrawEffectGlow(p,.12f*size,{1,.95f,.83f,flash*.9f});
  r.DrawEffectGlow(p,.40f*size,Color(e.school,flash*.32f));
  switch(e.school) {
    case C::Warrior: {
      // Blade contact leaves a short, oblique cut and hot metal fragments.
      r.DrawEffectStreak(Add(p,{-.28f*size,-.20f*size,0}),Add(p,{.28f*size,.20f*size,0}),.025f,c);
      if(e.heavy) r.DrawEffectStreak(Add(p,{-.18f*size,.28f*size,0}),Add(p,{.18f*size,-.28f*size,0}),.020f,c);
      for(int i=0;i<12;++i) {
        const float a=i*2.39996f,speed=(1.2f+float(i%4)*.4f)*size;
        const auto at=[&](float t) {return Add(p,{std::cos(a)*speed*t,(.6f+std::sin(a)*speed)*t-3*t*t,-t*.35f});};
        r.DrawEffectStreak(at(std::max(0.f,age-.05f)),at(age),.018f,c);
      }
      break;
    }
    case C::Paladin:
    case C::Priest: {
      const float radius=(.22f+phase*.65f)*size;
      Arc(r,p,radius,0,tau,.018f,c);
      for(int i=0;i<(e.school==C::Priest?4:8);++i) {
        const float a=i*tau/(e.school==C::Priest?4:8);
        Ray(r,p,a,radius*.65f,radius*1.35f,.018f,c);
      }
      if(e.school==C::Priest) r.DrawEffectStreak(Add(p,{0,-.6f*size,0}),Add(p,{0,.75f*size,0}),.028f,c);
      break;
    }
    case C::Scout:
    case C::Engineer: {
      // Directional sparks and ballistic debris, never a magical explosion.
      const int count=e.school==C::Scout?8:12;
      for(int i=0;i<count;++i) {
        const float a=i*2.39996f,speed=(1.4f+float(i%3)*.8f)*size;
        const auto at=[&](float t) {return Add(p,{std::cos(a)*speed*t,std::sin(a)*speed*t-3*t*t,-t*(.3f+float(i%4)*.13f)});};
        r.DrawEffectStreak(at(std::max(0.f,age-.05f)),at(age),e.school==C::Scout?.014f:.022f,c);
      }
      if(e.school==C::Engineer) Arc(r,p,(.18f+phase*.45f)*size,0,tau,.026f,Color(e.school,fade*.3f));
      else for(int i=0;i<3;++i) {
        const float offset=(i-1)*.07f;
        r.DrawEffectStreak(Add(p,{-.18f*size,offset-.08f,0}),Add(p,{.18f*size,offset+.08f,0}),.012f,c);
      }
      break;
    }
    case C::Druid: {
      for(int i=0;i<5;++i) {
        const float a=i*tau/5;
        const auto tip=Add(p,{std::cos(a)*size*.65f,std::sin(a)*size*.65f,0});
        Bolt(r,p,tip,.13f,phase*6+i,c);
      }
      break;
    }
    case C::Mage: {
      for(int i=0;i<7;++i) {
        const float a=i*2.39996f;
        const auto base=Add(p,{std::cos(a)*(.05f+phase*.48f)*size,-.18f,std::sin(a)*phase*.3f*size});
        const auto tip=Add(base,{std::sin(i*3.f+phase*4)*.13f,(.2f+phase*.95f)*size,0});
        r.DrawEffectStreak(base,tip,.12f*size,Color(e.school,fade*.18f));
        r.DrawEffectStreak(base,tip,.045f*(1-phase),c);
        r.DrawEffectGlow(Lerp(base,tip,.4f),.17f*size,Color(e.school,fade*.24f));
        r.DrawEffectGlow(tip,.045f,Color(e.school,fade*.65f));
      }
      GroundArc(r,Floor(e.position,.055f),(.25f+phase*.85f)*size,0,tau,.025f,Color(e.school,fade*.42f));
      break;
    }
    case C::Necromancer: {
      for(int strand=0;strand<2;++strand) for(int i=0;i<14;++i) {
        const auto point=[&](float u) {const float a=u*tau+phase*6+strand*pi;return Add(p,
          {std::cos(a)*(.12f+phase*.25f)*size,(u-.5f)*size,std::sin(a)*(.12f+phase*.25f)*size});};
        r.DrawEffectStreak(point(float(i)/14),point(float(i+1)/14),.018f,c);
      }
      // Drain returns to the snapshotted caster only after the actual hit.
      if(e.heavy) {
        const XMFLOAT3 caster{e.source.x,e.sourceHeight+1.25f,e.source.z};
        const auto point=[&](float u) {auto p2=Lerp(p,caster,u);p2.y+=std::sin(u*pi)*.35f;return p2;};
        const float t=Unit((age-.04f)/.32f);
        if(t>0 && t<1) {
          r.DrawEffectStreak(point(std::max(0.f,t-.12f)),point(t),.026f,c);
          r.DrawEffectGlow(point(t),.065f,c);
        }
      }
      break;
    }
    case C::Psionic: {
      const float radius=(.62f-phase*.4f)*size;
      for(int i=0;i<4;++i) {
        const float a=i*pi*.5f+phase;
        Arc(r,p,radius,a,.8f,.021f,c,false,.45f);
        Ray(r,p,a,radius*.45f,radius*.75f,.018f,c);
      }
      break;
    }
    case C::Bard: {
      for(int i=0;i<3;++i) Arc(r,p,(.15f+phase*.7f+i*.12f)*size,-.75f+pi,1.5f,.012f,
        Color(e.school,fade*(.65f-i*.12f)));
      for(int i=0;i<3;++i) Note(r,Add(p,{(i-1)*.28f*size,(.05f+phase*.65f+i*.08f)*size,0}),.18f*size,c);
      break;
    }
    case C::Demonologist: {
      for(int i=-1;i<=1;++i) {
        const auto a=Add(p,{i*.19f*size-.23f*size,-.35f*size,0});
        const auto b=Add(p,{i*.19f*size+.23f*size,.35f*size,0});
        r.DrawEffectStreak(a,b,.027f*size,c);
        r.DrawEffectStreak(a,b,.09f*size,Color(e.school,fade*.18f));
      }
      break;
    }
    default:break;
  }
  if(e.empowered) {
    // A second, delayed contour announces the resource payoff without masking contact.
    const float delayed=Unit((age-.035f)/.36f);
    GroundArc(r,Floor(e.position),(.3f+delayed*.9f)*size,phase*2,pi*1.7f,.018f,
      Color(e.school,std::sin(delayed*pi)*.4f));
    for(int i=0;i<4;++i) {
      const float a=i*pi*.5f+phase;
      r.DrawEffectGlow(Add(p,{std::cos(a)*phase*size,phase*size,std::sin(a)*phase*size}),.045f,Color(e.school,fade*.5f));
    }
  }
}
void Recovery(Renderer& r,const rpg::CombatVisualEvent& e,float age) {
  if(!std::isfinite(age) || age<=0 || age>=.8f) return;
  const float t=age/.8f,fade=std::sin(t*pi)*(1-t*.35f);
  const XMFLOAT3 p{e.position.x,e.height+.85f,e.position.z};
  auto color=Color(e.school,fade*.65f);
  switch(e.school) {
  case C::Warrior: // A warm breath and rising flecks, not a caster's seal.
    for(int i=0;i<8;++i) {
      const float a=i*tau/8;
      r.DrawEffectStreak(Add(p,{std::cos(a)*.35f,-.55f+t*.7f,std::sin(a)*.35f}),
        Add(p,{std::cos(a)*.39f,-.40f+t*.7f,std::sin(a)*.39f}),.016f,color);
    }break;
  case C::Scout: // Short linen-like wraps remain close to the body.
    color={.75f,.83f,.63f,fade*.55f};
    for(int i=0;i<3;++i) Arc(r,Add(p,{0,-.25f+i*.25f,0}),.39f,t*2+i,pi*1.3f,.035f,color,true,.22f);
    break;
  case C::Paladin:case C::Priest: {
    const bool priest=e.school==C::Priest;
    GroundArc(r,{p.x,e.height+.06f,p.z},.5f+t*.2f,0,tau,.018f,color);
    for(float side:{-1.f,1.f}) for(int i=0;i<4;++i) {
      const float span=(priest?.58f:.35f)+i*.08f;
      r.DrawEffectStreak(Add(p,{side*.22f,.12f+i*.11f,0}),
        Add(p,{side*span,.28f+i*.16f+t*.15f,0}),.018f,color);
    }
    Ray(r,Add(p,{0,.55f,0}),pi*.5f,-.15f,.3f,.023f,color);
    Ray(r,Add(p,{0,.65f,0}),0,-.16f,.16f,.023f,color);
    break;
  }
  case C::Druid:
    for(int vine=0;vine<3;++vine) for(int i=0;i<12;++i) {
      const auto point=[&](float u) {const float a=vine*tau/3+u*2+t;return Add(p,
        {std::cos(a)*(.55f-u*.15f),-.75f+u*1.6f,std::sin(a)*(.55f-u*.15f)});};
      const float u=i/12.f;const auto leaf=point(u);
      r.DrawEffectStreak(leaf,point((i+1)/12.f),.018f,color);
      if(i%3==0) r.DrawEffectStreak(leaf,Add(leaf,{.09f,.09f,.03f}),.035f,color);
    }break;
  case C::Mage:
    color={.35f,.72f,1,fade*.65f};
    for(int j=0;j<2;++j) Arc(r,Add(p,{0,(j-.5f)*.6f,0}),.52f,t*(j?2:-2),tau,.013f,color,true,.15f);
    for(int i=0;i<6;++i) {const float a=i*tau/6+t;
      const auto shard=Add(p,{std::cos(a)*.52f,.12f,std::sin(a)*.52f});
      r.DrawEffectStreak(Add(shard,{0,-.15f,0}),Add(shard,{0,.15f,0}),.04f,color);
    }break;
  case C::Necromancer:case C::Demonologist:
    for(int i=0;i<6;++i) {
      const float a=i*tau/6+t*1.8f,radius=.85f*(1-t)+.12f;
      const auto head=Add(p,{std::cos(a)*radius,.15f+std::sin(a*2)*.2f,std::sin(a)*radius});
      const auto tail=Add(p,{std::cos(a-.35f)*(radius+.12f),.12f,std::sin(a-.35f)*(radius+.12f)});
      r.DrawEffectStreak(tail,head,e.school==C::Demonologist?.035f:.018f,color);
      r.DrawEffectGlow(head,.05f,color);
    }break;
  case C::Psionic:
    for(float side:{-1.f,1.f}) {
      const auto center=Add(p,{side*(.6f-t*.18f),.1f,0});
      for(int i=0;i<4;++i) {
        const float a=i*pi*.5f,b=(i+1)*pi*.5f;
        r.DrawEffectStreak(Add(center,{std::cos(a)*.17f,std::sin(a)*.28f,0}),
          Add(center,{std::cos(b)*.17f,std::sin(b)*.28f,0}),.018f,color);
      }
    }break;
  case C::Bard:
    for(int i=0;i<3;++i) {
      const float beat=Unit((t-i*.13f)/.65f);if(beat<=0 || beat>=1) continue;
      auto c=color;c.w*=std::sin(beat*pi);
      GroundArc(r,{p.x,e.height+.08f,p.z},.25f+beat*.65f,0,tau,.018f,c);
      Note(r,Add(p,{(i-1)*.38f,.1f+beat*.7f,0}),.19f,c);
    }break;
  case C::Engineer:
    for(int i=0;i<8;++i) {
      const float a=i*tau/8;
      const auto spark=Add(p,{std::cos(a)*(.32f+t*.3f),-.4f+t*.6f,std::sin(a)*(.32f+t*.3f)});
      r.DrawEffectStreak(spark,Add(spark,{.035f,.1f,0}),.018f,color);
    }
    Arc(r,Add(p,{0,.12f,0}),.40f,-t*3,pi*1.5f,.025f,color);break;
  default:break;
  }
  color.w=fade*.1f;r.DrawEffectGlow(p,.4f,color);
}
void Prepare(Renderer& r,const rpg::Simulation& state,XMFLOAT3 p,float renderedFacing) {
  if(state.Health()<=0 || (!state.CastingRecovery() && (!state.Attacking() || state.ClassInfo().range<=3))) return;
  const float progress=state.CastingRecovery()?state.RecoveryProgress():state.AttackSeconds()/combat::StrokeStart(state.HeavyAttack());
  if(progress<=0 || progress>=1) return;
  spellvfx::Preparation(r,state,p,progress);
  const float alpha=std::sin(progress*pi*.5f)*.55f;
  const auto id=state.Class();const auto c=id==C::Mage && (state.AttackAbility()==10 || state.AttackAbility()==13)?XMFLOAT4{.68f,.55f,1,alpha}:
    (state.TechniqueAttack() || state.SignatureAttack() || state.AttackAbility()==8 || state.AttackAbility()==12) && id==C::Mage?XMFLOAT4{.35f,.78f,1.f,alpha}:Color(id,alpha);
  // Preparation follows the actual hand/weapon socket and the caster's facing.
  // World-XY brackets used to turn edge-on or float across the bow when aiming
  // east/west. This local plane remains attached through turns and strafing.
  const float sn=std::sin(renderedFacing),cs=std::cos(renderedFacing);
  const auto focus=[&](float right,float up,float forward=0.f) {
    return Add(p,{cs*right+sn*forward,up,-sn*right+cs*forward});
  };
  if((id==C::Scout || id==C::Engineer) && !state.CastingRecovery()) {
    if(!state.HeavyAttack()) return;
    // Compact sight brackets at the weapon; no premature muzzle flash.
    const float span=.12f-.065f*combat::Smooth(progress);
    for(float side:{-1.f,1.f}) {
      r.DrawEffectStreak(focus(side*span,-.065f),focus(side*span,.065f),.007f,c);
      r.DrawEffectStreak(focus(side*span,.065f),focus(side*(span-.025f),.065f),.007f,c);
      r.DrawEffectStreak(focus(side*span,-.065f),focus(side*(span-.025f),-.065f),.007f,c);
    }
    return;
  }
  if(id==C::Mage && !state.CastingRecovery()) {
    const bool frost=state.TechniqueAttack() || state.SignatureAttack() || state.AttackAbility()==8 || state.AttackAbility()==12;
    const bool arcane=state.AttackAbility()==10 || state.AttackAbility()==13;
    const float power=combat::Smooth(progress),radius=(.075f+.16f*power)*(state.AttackAbility()>=8?1.3f:1.f);
    const float phase=progress*(arcane?-5.f:frost?2.f:7.f);
    // Separate silhouettes: two flowing fire bands, six crystalline frost
    // spokes, or a broken arcane seal. Motes converge before the true release.
    if(frost) {
      for(int i=0;i<6;++i) {
        const float a=phase+i*tau/6,dx=std::cos(a),dy=std::sin(a);
        r.DrawEffectStreak(focus(dx*radius*.42f,dy*radius*.42f),focus(dx*radius,dy*radius),.009f,c);
        for(float side:{-1.f,1.f})
          r.DrawEffectStreak(focus(dx*radius*.72f,dy*radius*.72f),
            focus((dx*.53f-dy*.18f*side)*radius,(dy*.53f+dx*.18f*side)*radius),.007f,c);
      }
    } else for(int band=0;band<2;++band) for(int i=0;i<22;++i) {
      const float a=phase*(band?-1.f:1.f)+i*tau/28+band*pi,b=a+tau/28;
      const float stretch=band?.62f:1.f;
      const float z=band?.05f:-.025f;
      const auto bandColor=state.AttackAbility()==14 && band?XMFLOAT4{.4f,.8f,1,alpha}:c;
      r.DrawEffectStreak(focus(std::cos(a)*radius,std::sin(a)*radius*stretch,z),
        focus(std::cos(b)*radius,std::sin(b)*radius*stretch,z),arcane?.008f:.012f,bandColor);
    }
    for(int i=0;i<6;++i) {
      const float t=std::fmod(progress*1.6f+i/6.f,1.f),a=phase+i*tau/6;
      const float orbit=radius*(1.8f-t),up=(t-.5f)*radius;
      const auto mote=focus(std::cos(a)*orbit,std::sin(a)*orbit+up,-.05f);
      auto tint=c;tint.w*=std::sin(t*pi);
      r.DrawEffectStreak(mote,Lerp(mote,p,.22f+power*.18f),.009f,tint);
    }
    auto core=c;core.w*=.40f;
    r.DrawEffectGlow(p,radius*.5f,core);
    r.DrawEffectGlow(p,.025f+power*.025f,{1,.92f,.75f,alpha*.7f});
    return;
  }
  if(state.CastingRecovery()) {
    const XMFLOAT3 base{state.Player().x,state.WorldHeight()+.06f,state.Player().z};
    GroundArc(r,base,.45f+progress*.12f,progress*4,pi*1.7f,.012f,c);
    for(int i=0;i<4;++i) {
      const float a=i*tau/4+progress*5;
      const auto mote=Add(base,{std::cos(a)*.5f,progress*1.1f,std::sin(a)*.5f});
      r.DrawEffectStreak(mote,Lerp(mote,p,.25f),.014f,c);
    }
  }
  const float radius=.08f+progress*(state.HeavyAttack()?.18f:.08f);
  Arc(r,p,radius,progress*4,pi*1.6f,.013f,c,false,.45f);
  r.DrawEffectGlow(p,radius*.55f,Color(id,alpha*.45f));
  if(id==C::Bard) Note(r,Add(p,{.1f,0,0}),.14f,c);
  else if(id==C::Druid) Bolt(r,Add(p,{-.12f,0,0}),Add(p,{.12f,0,0}),.08f,progress*6,c);
  else for(int i=0;i<3;++i) {
    const float a=progress*6+i*tau/3;
    r.DrawEffectGlow(Add(p,{std::cos(a)*radius,std::sin(a)*radius,0}),.022f,c);
  }
}
void BarrierContact(Renderer& r,const rpg::CombatVisualEvent& e,float age) {
  if(!std::isfinite(age) || age<0 || age>=.38f)return;
  const float t=Unit(age/.38f),alpha=(1-t)*(1-t);
  const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.45f,e.height,e.position.z-std::cos(e.facing)*.45f};
  auto color=e.school==C::Mage?XMFLOAT4{.35f,.72f,1,alpha*.8f}:Color(e.school,alpha*.8f);
  Arc(r,p,.17f+t*.45f,0,tau,.025f,color);
  color.w=alpha*.7f;
  for(int i=0;i<6;++i) Ray(r,p,i*tau/6,.12f+t*.2f,.23f+t*.4f,.013f,color);
  const bool frost=e.school==C::Mage;
  r.DrawEffectSprite(frost?Renderer::EffectArt::Seal:Renderer::EffectArt::Sparks,p,
    .25f+t*.22f,.25f+t*.22f,-t*.8f,color);
  if(frost)for(int n=0;n<4;++n){
    const float a=n*tau/4+.4f;const auto shard=Add(p,{std::cos(a)*age*1.5f,std::sin(a)*age*1.5f-age*age,0});
    r.DrawEffectSprite(Renderer::EffectArt::Ice,shard,.035f,.13f,a,{.48f,.8f,1,alpha*.65f});
  }
}
void States(Renderer& r,const rpg::Simulation& state,float time) {
  // Stateless tool/preview adapter. The live Game owns persistent envelopes.
  SustainedEffects effects;for(int i=0;i<4;++i)effects.Update(state,.05f);
  effects.Render(r,state,time,state.Facing());
}
}
