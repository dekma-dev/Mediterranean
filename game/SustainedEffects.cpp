#include "game/SustainedEffects.h"
#include "game/ClassEffects.h"
#include "game/CastEffects.h"
#include "game/ActorPresentation.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>

namespace {
using namespace DirectX;using C=rpg::ClassId;using Art=Renderer::EffectArt;
constexpr float pi=3.14159265f,tau=2*pi;
float Unit(float x){return std::clamp(x,0.f,1.f);}
XMFLOAT3 Add(XMFLOAT3 a,XMFLOAT3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
XMFLOAT3 Ground(rpg::Vec2 p,float lift=.07f){return {p.x,rpg::WorldMap::Height(p)+lift,p.z};}
XMFLOAT3 Polar(rpg::Vec2 p,float a,float radius,float lift=.07f){return Ground({p.x+std::sin(a)*radius,p.z+std::cos(a)*radius},lift);}
void Ring(Renderer& r,rpg::Vec2 p,float radius,float start,float span,float width,XMFLOAT4 c,int segments=24){
  for(int i=0;i<segments;++i){const float a=start+span*i/segments,b=start+span*(i+1)/segments;
    r.DrawEffectRibbon(Polar(p,a,radius-width),Polar(p,a,radius+width),Polar(p,b,radius+width),Polar(p,b,radius-width),c);}
}
void Seal(Renderer& r,rpg::Vec2 p,float radius,float angle,XMFLOAT4 c){
  const auto at=[&](float x,float z){return Ground({p.x+(std::cos(angle)*x-std::sin(angle)*z)*radius,p.z+(std::sin(angle)*x+std::cos(angle)*z)*radius},.055f);};
  r.DrawEffectSurface(Art::Seal,at(-1,-1),at(-1,1),at(1,1),at(1,-1),c);
}
void Barrier(Renderer& r,const rpg::Simulation& s,float weight,float age,float facing){
  if(weight<=0)return;
  const auto p=s.Player();const XMFLOAT3 root{p.x,s.WorldHeight(),p.z};
  const bool physical=s.Class()==C::Warrior || s.Class()==C::Engineer;
  const bool frost=s.Class()==C::Mage;
  const float pulse=.85f+.15f*std::sin(age*2.3f),strength=Unit(s.Barrier()/12.f);
  auto color=frost?XMFLOAT4{.45f,.78f,1,weight*.66f*pulse}:classfx::Color(s.Class(),weight*.44f*pulse);
  const int panels=physical?3:5;
  for(int n=0;n<panels;++n){
    const float a=facing+(physical?(n-1)*.37f:n*tau/panels+age*.16f),radius=physical?.55f:.66f;
    const XMFLOAT3 center=Add(root,{std::sin(a)*radius,physical?1.03f:.86f+std::sin(age*1.4f+n)*.04f,std::cos(a)*radius});
    const auto at=[&](float side,float up){return Add(center,{std::cos(a)*side,up,-std::sin(a)*side});};
    // Actual angled panels rather than camera-facing circles around the body.
    const float half=physical?.14f:.21f,h=physical?.29f:.43f;
    auto surface=color;surface.w*=frost?.65f+.30f*strength:.32f+.30f*strength;
    r.DrawEffectSurface(physical?Art::Slash:frost?Art::Ice:Art::Seal,at(-half,-h),at(-half,h),at(half,h),at(half,-h),surface);
    for(float side:{-1.f,1.f}){
      r.DrawEffectStreak(at(0,h),at(side*half,0),.012f,color);
      r.DrawEffectStreak(at(side*half,0),at(0,-h),.012f,color);
    }
  }
  if(!physical){auto c=color;c.w*=.62f;Seal(r,p,.85f,-age*.08f,c);}
  if(frost)for(int n=0;n<3;++n){const float a=age*.35f+n*tau/3;
    r.DrawEffectSprite(Art::Smoke,Add(root,{std::sin(a)*.45f,.22f,std::cos(a)*.45f}),.36f,.22f,a,
      {.43f,.63f,.78f,weight*.18f},true);
  }
}
void Evasion(Renderer& r,const rpg::Simulation& s,float weight,float age,float facing){
  if(weight<=0)return;
  const auto p=s.Player();const float y=s.WorldHeight();
  for(int band=0;band<3;++band)for(int n=0;n<10;++n){
    const auto at=[&](float u){const float a=facing+pi+u*1.5f+band*.6f;
      const float radius=.46f+.06f*std::sin(age*3+band);
      return XMFLOAT3{p.x+std::sin(a)*radius,y+.24f+band*.29f+u*.08f,p.z+std::cos(a)*radius};};
    r.DrawEffectStreak(at(n/10.f),at((n+1)/10.f),.018f,{.58f,.77f,.68f,weight*.27f*std::sin((n+.5f)*pi/10)});
  }
}
void Field(Renderer& r,const rpg::Simulation& s,float time){
  if(s.FrostFieldSeconds()<=0 || s.Health()<=0)return;
  const auto p=s.FrostFieldPosition();const float age=5-s.FrostFieldSeconds();
  const float fade=Unit(age/.15f)*Unit(s.FrostFieldSeconds()/.45f);
  Ring(r,p,rpg::Simulation::frostFieldRadius-.025f,0,tau,.025f,{.34f,.67f,.91f,.38f*fade},64);
  // The fixed three-metre boundary matches simulation; only interior wisps move.
  for(int n=0;n<8;++n){
    const float a=n*2.39996f,phase=std::fmod(time*.27f+n*.173f,1.f);
    const auto at=Polar(p,a,.55f+n*.27f,.12f+phase*.30f);
    r.DrawEffectSprite(Art::Smoke,at,.56f+phase*.32f,.23f+phase*.12f,a,
      {.52f,.72f,.86f,fade*std::sin(phase*pi)*.27f},true);
    const auto crystal=Polar(p,a,2.45f,.17f);
    r.DrawEffectSprite(Art::Ice,crystal,.10f,.31f,a*.12f,{.56f,.80f,1,fade*.66f});
    for(int j=0;j<3;++j)r.DrawEffectStreak(Polar(p,a+.035f*std::sin(j*3.f),.45f+j*.63f),
      Polar(p,a+.035f*std::sin((j+1)*3.f),1.08f+j*.63f),.018f,{.48f,.72f,.91f,fade*.32f});
  }
  Seal(r,p,1.55f,.2f,{.45f,.72f,.94f,fade*.28f});
}
}

void SustainedEffects::Channel::Step(float remaining,float dt){
  const bool now=std::isfinite(remaining) && remaining>0;
  if(now && !active)age=0;
  age+=dt;active=now;
  const float target=now?Unit(remaining/.16f):0.f,step=dt/(now?.12f:.18f);
  weight+=std::clamp(target-weight,-step,step);
}
bool SustainedEffects::Actor::Matches(const rpg::Enemy& e) const {
  return id==e.id && kind==e.kind && rank==e.rank && home.x==e.home.x && home.z==e.home.z;
}
void SustainedEffects::Reset(){actors_={};player_={};school_=C::Count;}
std::size_t SustainedEffects::ActorCount() const {return std::count_if(actors_.begin(),actors_.end(),[](const Actor& a){return a.id!=0;});}
float SustainedEffects::Weight(rpg::EntityId id,int channel) const {
  if(channel<0 || channel>=4)return 0;
  if(id==0)return channel<2?player_[channel].weight:0;
  for(const auto& a:actors_)if(a.id==id)return a.channels[channel].weight;return 0;
}
void SustainedEffects::Update(const rpg::Simulation& s,float dt){
  if(!std::isfinite(dt) || dt<=0)return;dt=std::min(dt,.05f);
  if(s.Class()!=school_){Reset();school_=s.Class();}
  if(s.Health()<=0){Reset();return;}
  player_[0].Step(s.Barrier()>0?1.f:0.f,dt);player_[1].Step(s.EvasionSeconds(),dt);
  struct Candidate{const rpg::Enemy* enemy=nullptr;float distance=0;bool target=false;};
  std::array<Candidate,maxActors> selected{};std::size_t count=0;
  const auto before=[](const Candidate& a,const Candidate& b){return a.target!=b.target?a.target:a.distance!=b.distance?a.distance<b.distance:a.enemy->id<b.enemy->id;};
  const auto add=[&](const rpg::Enemy& e){
    if(e.health<=0 || e.state==rpg::EnemyState::Returning)return;
    const float distance=std::hypot(e.position.x-s.Player().x,e.position.z-s.Player().z);if(distance>30)return;
    bool old=false;for(const auto& a:actors_)if(a.Matches(e))for(const auto& c:a.channels)old|=c.weight>0;
    if(!old && e.snared<=0 && e.rooted<=0 && e.stunned<=0 && e.exposed<=0)return;
    Candidate c{&e,distance,s.TargetActor() && s.TargetActor()->id==e.id};std::size_t slot=0;
    while(slot<count && before(selected[slot],c))++slot;if(slot>=maxActors)return;
    for(auto i=std::min(count,maxActors-1);i>slot;--i)selected[i]=selected[i-1];selected[slot]=c;count=std::min(count+1,maxActors);
  };
  for(const auto& e:s.Combatants())add(e);
  for(const auto& e:s.Dummies())add(e);
  if(s.Sparring())add(s.SparringOpponent());
  std::array<Actor,maxActors> next{};
  for(std::size_t i=0;i<count;++i){const auto& e=*selected[i].enemy;auto& a=next[i];
    for(const auto& old:actors_)if(old.Matches(e) && std::hypot(e.position.x-old.position.x,e.position.z-old.position.z)<1.f){a=old;break;}
    a.id=e.id;a.kind=e.kind;a.rank=e.rank;a.home=e.home;a.position=e.position;a.facing=e.facing;
    const float timers[]={e.rooted,e.snared,e.stunned,e.exposed};bool visible=false;
    for(int c=0;c<4;++c){a.channels[c].Step(timers[c],dt);visible|=a.channels[c].weight>0;}
    if(!visible)a={};
  }
  actors_=next;
}
void SustainedEffects::Render(Renderer& r,const rpg::Simulation& s,float time,float facing) const {
  if(s.Health()<=0 || s.Class()!=school_)return;
  const auto start=r.EffectVertices();
  Barrier(r,s,player_[0].weight,player_[0].age,facing);Evasion(r,s,player_[1].weight,player_[1].age,facing);Field(r,s,time);
  for(const auto& a:actors_){
    if(!a.id || r.EffectVertices()-start>1700)continue;
    const bool beast=a.kind==rpg::EnemyKind::Boar || a.kind==rpg::EnemyKind::Wolf;
    const auto p=Ground(a.position);const float height=beast?1.05f:2.15f;
    const float root=a.channels[0].weight,snare=a.channels[1].weight,stun=a.channels[2].weight,exposed=a.channels[3].weight;
    if(root>0 || snare>0){
      const float weight=std::max(root,snare*.55f),age=root>0?a.channels[0].age:a.channels[1].age;
      if(s.Class()==C::Mage){
        for(int n=0;n<6;++n){const float angle=n*tau/6+a.id*.17f;
          const auto at=Add(p,{std::sin(angle)*.38f,.19f+root*.12f,std::cos(angle)*.38f});
          r.DrawEffectSprite(Art::Ice,at,.065f+root*.018f,.18f+root*.17f,std::sin(angle)*.25f,{.46f,.75f,1,weight*.63f});
        }
        const float cycle=std::fmod(age*.45f,1.f);r.DrawEffectSprite(Art::Smoke,Add(p,{0,.16f+cycle*.12f,0}),.52f,.20f,age*.12f,
          {.48f,.69f,.83f,weight*.18f*std::sin(cycle*pi)},true);
      }else{
        const auto color=s.Class()==C::Scout?XMFLOAT4{.64f,.57f,.32f,weight*.55f}:classfx::Color(s.Class(),weight*.43f);
        // Scout snares cross around the ankles; nature roots climb in a spiral.
        for(int strand=0;strand<3;++strand)for(int n=0;n<8;++n){
          const auto at=[&](float u){const float angle=strand*tau/3+u*(s.Class()==C::Scout?4.f:2.4f);
            const float radius=.43f-u*.12f;return Add(p,{std::sin(angle)*radius,u*(root>0?.67f:.23f),std::cos(angle)*radius});};
          r.DrawEffectStreak(at(n/8.f),at((n+1)/8.f),.018f,color);
        }
      }
    }
    if(stun>0)for(int n=0;n<3;++n){const float angle=a.channels[2].age*2.2f+n*tau/3;
      r.DrawEffectSprite(Art::Sparks,Add(p,{std::sin(angle)*.27f,height,std::cos(angle)*.27f}),.08f,.075f,angle,
        {1,.72f,.27f,stun*.57f});}
    if(exposed>0){const float pulse=.8f+.2f*std::sin(a.channels[3].age*4);
      for(int n=0;n<4;++n){const float angle=n*pi*.5f+.35f;
        const auto at=Add(p,{std::cos(angle)*.35f,height*.53f+std::sin(angle)*.35f,0});
        r.DrawEffectSprite(Art::Slash,at,.13f,.045f,-angle,{1,.45f,.19f,exposed*.35f*pulse});}
    }
  }
}
void SustainedEffects::Warnings(Renderer& r,const rpg::Simulation& s,const std::vector<presentation::Actor>* actors){
  if(s.Health()<=0)return;
  struct Warning {const rpg::Enemy* e=nullptr;float distance=0;bool threatens=false;};
  std::array<Warning,maxWarnings> chosen{};std::size_t count=0;
  const auto before=[](const Warning& a,const Warning& b){return a.threatens!=b.threatens?a.threatens:
    a.e->windup!=b.e->windup?a.e->windup<b.e->windup:a.distance!=b.distance?a.distance<b.distance:a.e->id<b.e->id;};
  const auto add=[&](const rpg::Enemy& e){
    if(!s.VisibleEnemyCast(e))return;
    const float dx=s.Player().x-e.position.x,dz=s.Player().z-e.position.z,d=std::hypot(dx,dz);
    const float dot=d>.001f?(std::sin(e.facing)*dx+std::cos(e.facing)*dz)/d:1;
    Warning w{&e,d,e.Hits(d,dot) || (e.kind==rpg::EnemyKind::Boar && d<=e.WarningReach() && dot>=std::cos(e.HalfAngle()))};
    std::size_t slot=0;while(slot<count && before(chosen[slot],w))++slot;if(slot>=maxWarnings)return;
    for(auto n=std::min(count,maxWarnings-1);n>slot;--n)chosen[n]=chosen[n-1];chosen[slot]=w;count=std::min(count+1,maxWarnings);
  };
  for(const auto& e:s.Combatants())add(e);if(s.Sparring())add(s.SparringOpponent());
  // Submit essential borders before scenery, status ornaments and hit cosmetics.
  // Eight simultaneous quake+cone borders cost at most 3,168 vertices.
  const int circleSegments=count<=2?64:count<=4?40:24,coneSegments=count<=2?32:count<=4?24:16;
  for(std::size_t i=0;i<count;++i){const auto& e=*chosen[i].e;const float progress=Unit(1-e.windup/e.WindupDuration());
    const auto color=e.Interruptible()?XMFLOAT4{1,.47f-.25f*progress,.08f,.72f}:XMFLOAT4{.77f,.36f,1,.78f};
    const bool disc=e.Boss() && e.bossAttack==1;const float span=e.HalfAngle()*2,start=e.facing-e.HalfAngle();
    Ring(r,e.position,e.WarningReach(),start,span,.028f,color,disc?circleSegments:coneSegments);
    if(!disc)for(float side:{-1.f,1.f})r.DrawEffectStreak(Polar(e.position,e.facing+side*e.HalfAngle(),.08f),
      Polar(e.position,e.facing+side*e.HalfAngle(),e.WarningReach()),.023f,color);
    if(e.Boss() && e.bossAttack==0)for(float radius:{rpg::Enemy::shockInner,rpg::Enemy::shockOuter})Ring(r,e.position,radius,0,tau,.028f,color,circleSegments);
  }
  const unsigned detailLimit=3600;
  const int fillSegments=count<=2?48:count<=4?24:12;
  for(std::size_t i=0;i<count && r.EffectVertices()+fillSegments*6+36<=detailLimit;++i){const auto& e=*chosen[i].e;
    const float progress=Unit(1-e.windup/e.WindupDuration());const bool quake=e.Boss() && e.bossAttack==0;
    const float span=quake?tau:e.HalfAngle()*2,start=quake?0:e.facing-e.HalfAngle();
    const auto c=e.Interruptible()?XMFLOAT4{1,.30f,.08f,.08f+progress*.09f}:XMFLOAT4{.72f,.30f,1,.08f+progress*.09f};
    for(int n=0;n<fillSegments;++n){const float a=start+span*n/fillSegments,b=start+span*(n+1)/fillSegments;
      const float inner=quake?rpg::Enemy::shockInner:.08f,outer=quake?rpg::Enemy::shockOuter:e.WarningReach();
      r.DrawEffectRibbon(Polar(e.position,a,inner),Polar(e.position,a,outer),Polar(e.position,b,outer),Polar(e.position,b,inner),c);
    }
    if(actors && (e.kind==rpg::EnemyKind::Wraith || e.Boss())){
      const auto actor=std::find_if(actors->begin(),actors->end(),[&](const presentation::Actor& a){return a.id==e.id;});
      if(actor!=actors->end())castfx::PrepareEnemy(r,e,castfx::FromEnemy(e,*actor));
    }
  }
}
