#include "game/ClassEffects.h"
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
         bool ground=false,float tilt=0) {
  constexpr int segments=20;
  const auto point=[&](float a,float offset) -> XMFLOAT3 {
    const float x=std::cos(a)*(radius+offset),v=std::sin(a)*(radius+offset);
    return Add(p,ground?XMFLOAT3{x,v*tilt,v}:XMFLOAT3{x,v,v*tilt});
  };
  for(int i=0;i<segments;++i) {
    const float a=start+span*float(i)/segments,b=start+span*float(i+1)/segments;
    r.DrawEffectRibbon(point(a,-width),point(a,width),point(b,width),point(b,-width),color,1,1);
  }
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
  const bool physical=e.school==C::Scout || e.school==C::Engineer;
  const float flight=std::max(.01f,e.flight),t=Unit(age/flight);
  const float fade=1-Unit((age-flight)/(physical?.045f:.10f));
  if(fade<=0) return;
  const float power=e.heavy?1.3f:1,arc=e.school==C::Mage?.28f:physical?0.f:.12f;
  const auto point=[&](float u) { return CombatEffects::SpellTracePoint(origin,e.destination,Unit(u),arc); };
  auto c=Color(e.school,fade*.72f);
  const auto head=point(t);
  if(e.technique && e.school==C::Mage) {
    c={.35f,.78f,1.f,fade*.8f};
    r.DrawEffectStreak(point(t-.17f),head,.035f,c);
    for(int n=0;n<3;++n) r.DrawEffectStreak(Add(head,{(n-1)*.05f,.06f,0}),point(t-.07f),.012f,c);
    return;
  }
  if(physical) {
    const auto tail=point(t-.08f);
    r.DrawEffectStreak(tail,head,.012f*power,{1,.87f,.64f,fade});
    if(e.school==C::Scout) {
      // A solid, narrow bolt and paired fletching; no spell orb on the handbow.
      for(float side:{-1.f,1.f}) r.DrawEffectStreak(Add(tail,{side*.055f,.02f,0}),point(t-.045f),.009f,c);
      if(e.heavy) r.DrawEffectStreak(point(t-.14f),head,.025f,Color(e.school,fade*.23f));
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
  if(e.empowered) Arc(r,head,.23f,age*12,pi*1.5f,.014f,Color(e.school,fade*.55f));
}
void Impact(Renderer& r,const rpg::CombatVisualEvent& e,float age) {
  const float phase=Unit(age/.48f),fade=(1-phase)*(1-phase);
  const float flash=std::pow(1-Unit(age/.095f),2.f);
  const float size=(e.heavy?1.1f:.72f)*(e.empowered?1.2f:1);
  const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.34f,e.height,e.position.z-std::cos(e.facing)*.34f};
  auto c=Color(e.school,fade*.8f);
  if(e.technique) {
    if(e.school==C::Mage) c={.35f,.78f,1.f,fade*.8f};
    const XMFLOAT3 ground{e.position.x,.055f,e.position.z};
    // A short broken seal identifies the tactical contact without hiding feet.
    for(int n=0;n<3;++n) Arc(r,ground,.42f+phase*.30f,n*tau/3+phase,.65f,.012f,c,true);
  }
  if(e.technique && e.school==C::Mage) {
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
      Arc(r,{e.position.x,.055f,e.position.z},(.25f+phase*.85f)*size,0,tau,.025f,Color(e.school,fade*.42f),true);
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
    Arc(r,{e.position.x,.06f,e.position.z},(.3f+delayed*.9f)*size,phase*2,pi*1.7f,.018f,
      Color(e.school,std::sin(delayed*pi)*.4f),true);
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
    Arc(r,{p.x,e.height+.06f,p.z},.5f+t*.2f,0,tau,.018f,color,true);
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
      Arc(r,{p.x,e.height+.08f,p.z},.25f+beat*.65f,0,tau,.018f,c,true);
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
void Prepare(Renderer& r,const rpg::Simulation& state,XMFLOAT3 p) {
  if(state.Health()<=0 || (!state.CastingRecovery() && (!state.Attacking() || state.ClassInfo().range<=3))) return;
  const float progress=state.CastingRecovery()?state.RecoveryProgress():state.AttackSeconds()/combat::StrokeStart(state.HeavyAttack());
  if(progress<=0 || progress>=1) return;
  const float alpha=std::sin(progress*pi*.5f)*.55f;
  const auto id=state.Class();const auto c=state.TechniqueAttack() && id==C::Mage?XMFLOAT4{.35f,.78f,1.f,alpha}:Color(id,alpha);
  if((id==C::Scout || id==C::Engineer) && !state.CastingRecovery()) {
    if(!state.HeavyAttack()) return;
    // Compact sight brackets at the weapon; no premature muzzle flash.
    for(float side:{-1.f,1.f}) r.DrawEffectStreak(Add(p,{side*(.10f-.04f*progress),-.05f,0}),
      Add(p,{side*(.10f-.04f*progress),.05f,0}),.008f,c);
    return;
  }
  if(state.CastingRecovery()) {
    const XMFLOAT3 base{state.Player().x,state.Height()+.06f,state.Player().z};
    Arc(r,base,.45f+progress*.12f,progress*4,pi*1.7f,.012f,c,true);
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
  const float t=Unit(age/.38f),alpha=(1-t)*(1-t);
  const XMFLOAT3 p{e.position.x-std::sin(e.facing)*.45f,e.height,e.position.z-std::cos(e.facing)*.45f};
  auto color=e.school==C::Mage?XMFLOAT4{.35f,.72f,1,alpha*.8f}:Color(e.school,alpha*.8f);
  Arc(r,p,.17f+t*.45f,0,tau,.025f,color);
  color.w=alpha*.7f;
  for(int i=0;i<6;++i) Ray(r,p,i*tau/6,.12f+t*.2f,.23f+t*.4f,.013f,color);
}
void States(Renderer& r,const rpg::Simulation& state,float time) {
  if(state.Health()<=0) return;
  const auto player=state.Player();
  if(state.Barrier()>0) {
    const float a=std::min(1.f,state.Barrier()/12.f)*(.28f+.045f*std::sin(time*3));
    const XMFLOAT3 p{player.x,state.Height()+1,player.z};
    const auto color=state.Class()==C::Mage?XMFLOAT4{.35f,.72f,1,a}:Color(state.Class(),a);
    const bool physical=state.Class()==C::Warrior || state.Class()==C::Engineer;
    if(physical) {
      // Guard plates face with the champion instead of orbiting like a spell.
      const float facing=state.Facing(),sx=std::sin(facing),cx=std::cos(facing);
      for(int plate=-1;plate<=1;++plate) {
        const float side=plate*.21f;
        const auto center=Add(p,{cx*side+sx*.46f,.04f,cx*.46f-sx*side});
        const auto top=Add(center,{0,.23f,0}),bottom=Add(center,{0,-.26f,0});
        r.DrawEffectStreak(top,Add(center,{cx*.09f,0,-sx*.09f}),.014f,color);
        r.DrawEffectStreak(Add(center,{cx*.09f,0,-sx*.09f}),bottom,.014f,color);
        r.DrawEffectStreak(bottom,Add(center,{-cx*.09f,0,sx*.09f}),.014f,color);
        r.DrawEffectStreak(Add(center,{-cx*.09f,0,sx*.09f}),top,.014f,color);
      }
    } else {
      for(int i=0;i<3;++i) Arc(r,p,.64f,i*tau/3+time*.18f,1.3f,.013f,color,false,.35f);
      Arc(r,{player.x,state.Height()+.08f,player.z},.57f,0,tau,.014f,color,true);
    }
  }
  if(state.EvasionSeconds()>0) {
    const float alpha=std::min(1.f,state.EvasionSeconds()/.3f)*.3f;
    for(int i=0;i<3;++i) Arc(r,{player.x,state.Height()+.15f+i*.23f,player.z},.43f,
      state.Facing()+time*.5f+i*.8f,.8f,.012f,Color(C::Scout,alpha),true,.15f);
  }
  unsigned shown=0;
  const auto status=[&](const rpg::Enemy& enemy) {
    if(enemy.health<=0 || shown>=8 || (enemy.snared<=0 && enemy.rooted<=0 && enemy.stunned<=0 && enemy.exposed<=0)) return;
    if(std::hypot(enemy.position.x-player.x,enemy.position.z-player.z)>30) return;
    ++shown;const XMFLOAT3 ground{enemy.position.x,.08f,enemy.position.z};
    if(enemy.rooted>0) {
      const float alpha=Unit(enemy.rooted/.25f)*.55f;
      for(int j=0;j<3;++j) for(int i=0;i<10;++i) {
        const auto point=[&](float u) {const float a=j*tau/3+u*2.6f;return Add(ground,
          {std::cos(a)*(.45f-u*.17f),u*.75f,std::sin(a)*(.45f-u*.17f)});};
        r.DrawEffectStreak(point(float(i)/10),point(float(i+1)/10),.018f,{.26f,.71f,.28f,alpha});
      }
    } else if(enemy.snared>0) {
      const auto c=Color(C::Scout,Unit(enemy.snared/.25f)*.48f);
      Arc(r,ground,.42f,0,tau,.014f,c,true);
      for(int i=0;i<4;++i) {
        const float a=i*pi*.5f;
        r.DrawEffectStreak(Add(ground,{std::cos(a)*.42f,0,std::sin(a)*.42f}),
          Add(ground,{std::cos(a+.8f)*.42f,.20f,std::sin(a+.8f)*.42f}),.012f,c);
      }
    }
    if(enemy.stunned>0) {
      const auto c=Color(C::Psionic,Unit(enemy.stunned/.25f)*.6f);
      for(int i=0;i<3;++i) {
        const float a=time*2+i*tau/3;
        const auto p=Add(ground,{std::cos(a)*.27f,2.18f,std::sin(a)*.27f});
        Ray(r,p,0,.01f,.065f,.018f,c);Ray(r,p,pi*.5f,-.065f,.065f,.018f,c);
      }
    }
    if(enemy.exposed>0) {
      const auto c=Color(C::Engineer,Unit(enemy.exposed/.25f)*.55f);
      const auto p=Add(ground,{0,1.15f,0});
      for(int i=0;i<4;++i) Arc(r,p,.48f,i*pi*.5f+.12f,.42f,.016f,c);
    }
  };
  for(const auto& enemy:state.Enemies()) status(enemy);
  if(state.Sparring()) status(state.SparringOpponent());
}
}
