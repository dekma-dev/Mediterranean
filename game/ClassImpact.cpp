#include "game/SpellVfx.h"
#include "game/SpellFlow.h"
#include <algorithm>
#include <cmath>

namespace spellvfx {
namespace {
using namespace DirectX;
using C=rpg::ClassId;
using Art=Renderer::EffectArt;
constexpr float pi=3.141592654f,tau=pi*2;
float Unit(float v){return std::clamp(v,0.f,1.f);}
float Envelope(float t,float life,float rise=.018f){return Unit(t/rise)*std::pow(Unit(1-t/life),1.3f);}
struct Frame {
 XMFLOAT3 p;float sn,cs;
 Frame(XMFLOAT3 at,float yaw):p(at),sn(std::sin(yaw)),cs(std::cos(yaw)){}
 XMFLOAT3 At(float x,float y,float z=0)const{return {p.x+cs*x+sn*z,p.y+y,p.z-sn*x+cs*z};}
 XMFLOAT3 Normal(float x,float z)const{return {cs*x+sn*z,0,-sn*x+cs*z};}
};
XMFLOAT3 Floor(rpg::Vec2 p,float up=.08f){return {p.x,rpg::WorldMap::Height(p)+up,p.z};}
void Debris(Renderer& r,const Frame& f,float age,float strength,bool metal){
 const float fade=Envelope(age,.64f,.006f);
 for(int n=0;n<10;++n){const float a=n*2.39996f,v=.8f+(n%4)*.55f;
  const auto at=[&](float t){return f.At(std::cos(a)*t*v,(.8f+std::sin(a)*1.4f)*t-3.6f*t*t,t*(.6f+(n%3)*.5f));};
  r.DrawEffectStreak(at(std::max(0.f,age-.035f)),at(age),metal?.014f:.010f,
    metal?XMFLOAT4{1,.69f,.28f,fade*strength}:XMFLOAT4{.73f,.62f,.40f,fade*strength});
 }
}
void Steel(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 const Frame hit({e.position.x,e.height,e.position.z},e.facing);
 const bool breaker=e.ability==9,cross=e.ability==14,cyclone=e.ability==11;
 // Centre the sweep on the accepted caster, not on a target that might move
 // later. The material sheet occupies the real melee reach, never a distant
 // projectile. The tracked weapon trail remains independent and exact.
 const float distance=std::hypot(e.position.x-e.source.x,e.position.z-e.source.z);
 const bool sourceOk=std::isfinite(e.sourceHeight)&&std::isfinite(distance)&&distance<5.5f;
 const Frame swing(sourceOk?XMFLOAT3{e.source.x,e.sourceHeight+1.10f,e.source.z}:
   hit.At(0,-.16f,-1.8f),e.facing);
 const float fade=Envelope(age,.40f,.008f),turn=combat::Smooth(age/.25f);
 const float reach=cyclone?3.25f:sourceOk?std::clamp(distance,1.4f,3.f):2.f;
 for(int layer=0;layer<(cross||cyclone?2:1);++layer){
  const auto path=[&](float u){
   const float a=(u-.5f)*(cyclone?tau*.86f:2.35f)+(turn-.4f)*.35f+layer*(cyclone?pi:0);
   const float x=std::sin(a)*reach,z=std::cos(a)*reach;
   const float slope=sourceOk?(e.height-.16f-swing.p.y)*std::max(0.f,std::cos(a)):0;
   const float y=slope+(breaker?(u-.5f)*2.4f:cross?(layer?1.f:-1.f)*x*.47f:std::sin(a)*.16f+layer*.4f);
   return breaker?swing.At(x*.32f,y,z):swing.At(x,y,z);
  };
  FilamentStream(r,path,breaker?swing.Normal(1,0):XMFLOAT3{swing.sn*.35f,.82f,swing.cs*.35f},breaker?.28f:.40f,
    reach*2.35f,age,{.74f,.86f,.96f,fade*.86f},20);
 }
 // A fast pressure cut at the victim ties the wide swing to the hit frame.
 FilamentStream(r,[&](float u){return hit.At((u-.5f)*1.45f,(u-.5f)*(breaker?2.1f:.48f),-.12f);},
   hit.Normal(0,1),.16f,1.8f,age,{1,.89f,.64f,Envelope(age,.18f,.004f)},8);
 Debris(r,hit,age,.9f,true);
 for(int n=0;n<3;++n){const auto p=Floor({hit.p.x+std::cos(n*2.1f+e.facing)*age*2,
   hit.p.z+std::sin(n*2.1f+e.facing)*age*2},.10f+age*.22f);
  r.DrawEffectSprite(Art::Dust,p,.24f+age*.8f,.16f+age*.4f,float(n),{.48f,.43f,.33f,Envelope(age,.58f,.04f)*.42f},true);}
}
void Pierce(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 const Frame f({e.position.x,e.height,e.position.z},e.facing);
 const float fade=Envelope(age,.40f,.006f),push=combat::Smooth(age/.25f);
 // Forward air tears, not a radial magic explosion. No extension is drawn
 // through a wall: this compact fan stays around the confirmed victim.
 for(int n=0;n<4;++n){const float a=n*tau/4+pi*.25f;
  FilamentStream(r,[&](float u){const float spread=std::sin(u*pi)*(.20f+push*.50f);
    return f.At(std::cos(a)*spread,std::sin(a)*spread,-.8f+u*(1.25f+push*.35f));},
    f.Normal(std::sin(a),std::cos(a)),.10f,1.8f,age,{.76f,.88f,.73f,fade*.65f},10);
 }
 r.DrawEffectSprite(Art::Sparks,f.p,.34f,.34f,0,{1,.86f,.56f,Envelope(age,.12f,.004f)});
 Debris(r,f,age,.85f,false);
 r.DrawEffectSprite(Art::Dust,f.At(0,-.08f,age*.5f),.20f+age*.7f,.17f+age*.5f,e.facing,
   {.43f,.40f,.31f,Envelope(age,.42f)*.50f},true);
}
void Meteor(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 const bool meteor=e.ability==9;
 const Frame f(meteor?Floor(e.position):XMFLOAT3{e.position.x,e.height-.22f,e.position.z},e.facing);
 const float rise=combat::Smooth(age/.10f),expand=combat::Smooth(age/.40f),fade=Envelope(age,.85f,.012f);
 // The descending mass compresses first, then throws six curved tongues
 // outwards. Broad sheets and rising volumes leave a visible central gap.
 const float mass=meteor?1.f:.65f;
 r.DrawSpellVolume(Renderer::SpellMedium::Fire,f.At(0,.35f*mass+age*.45f),{0,1,0},
   {(.68f+expand*.65f)*mass,(.68f+expand*.65f)*mass,(.42f+rise*.50f)*mass},age,{1,1,1,Envelope(age,.38f,.005f)},e.actor+19);
 const int tongues=meteor?6:4;
 for(int n=0;n<tongues;++n){const float a=n*2.39996f,ca=std::cos(a),sa=std::sin(a);
  const float radial=(.30f+expand*2.25f)*mass*(.83f+.20f*std::sin(n*3.7f));
  const float height=(.85f+(n%3)*.32f)*rise*mass*(1-age*.45f);
  const auto path=[&](float u){const float rr=.10f+radial*u,theta=a+std::sin(u*4.2f+age*7+n)*.20f*u;
   return f.At(std::cos(theta)*rr,height*std::sin(u*pi)*(.85f+.15f*std::sin(u*7+n))+.12f+age*.30f,std::sin(theta)*rr);};
  FlameStream(r,path,f.Normal(-sa,ca),.52f*mass,radial*1.8f,age+n*.17f,{1,1,1,fade*.86f},12);
  if(n%2==0){const auto p=path(.65f);
   r.DrawSpellVolume(Renderer::SpellMedium::Fire,p,{ca*.3f,1,sa*.3f},
    {(.48f+age*.20f)*mass,(.48f+age*.20f)*mass,.80f*mass},age+n*.13f,{1,.86f,.62f,fade*.72f},e.actor+n*31);}
  const auto smoke=f.At(ca*radial,.25f+age*.75f,sa*radial);
  r.DrawEffectSprite(Art::Smoke,smoke,.24f+age*.85f,.28f+age*.80f,a,{.29f,.26f,.22f,Envelope(age-.05f,.78f,.07f)*.5f},true);
 }
 for(int n=0;n<14;++n){const float a=n*2.39996f,speed=1.8f+(n%4)*.6f;
  const auto at=[&](float t){return f.At(std::cos(a)*t*speed,.20f+(2.8f+(n%3)*.65f)*t-3.8f*t*t,std::sin(a)*t*speed);};
  r.DrawEffectStreak(at(std::max(0.f,age-.045f)),at(age),.018f,{1,.53f,.09f,fade*.92f});
 }
}
void Sanctuary(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 const Frame f(Floor(e.position),e.facing);
 const float open=combat::Smooth(age/.28f),fade=Envelope(age,.88f,.016f);
 // Paired spreading vanes, then an open vault. Each rib has a broad flowing
 // body and a bright inner filament; the target remains visible between them.
 for(int side:{-1,1})for(int rib=0;rib<4;++rib){
  const float delay=rib*.018f,t=age-delay;if(t<=0)continue;
  const float reach=(1.25f+rib*.32f)*(.45f+open*.55f);
  const auto path=[&](float u){return f.At(side*(.12f+u*reach),
    .35f+u*(2.25f-rib*.40f)+std::sin(u*pi)*(.60f+open*.4f),
    -.12f-rib*.15f+std::sin(u*pi)*.18f);};
  FilamentStream(r,path,f.Normal(0,1),.10f+rib*.010f,reach*2,age+rib*.12f,
    {1,.81f-rib*.04f,.40f-rib*.035f,Envelope(t,.80f-delay,.025f)*.60f},10);
 }
 for(int n=0;n<3;++n){const float t=age-n*.035f;if(t<=0)continue;
  const float x=(n-1)*.58f,lower=.18f+(1-combat::Smooth(t/.10f))*2.5f;
  FilamentStream(r,[&](float u){return f.At(x,lower+u*(4.2f-lower)-age*.45f,.22f);},
    f.Normal(0,1),n==1?.15f:.09f,4.2f,age,{1,.97f,.76f,Envelope(t,.53f,.012f)*.88f},6);
 }
 r.DrawSpellVolume(Renderer::SpellMedium::Holy,f.At(0,1.5f),{0,1,0},{.48f,.48f,1.5f},age,
   {1,1,1,fade*.58f},e.actor+71);
 // A short ground seal states the ability's holy identity, without implying
 // an ongoing area heal or an invented new combat radius.
 const float radius=.6f+open*.55f;
 r.DrawEffectSurface(Art::Seal,f.At(-radius,0,-radius),f.At(-radius,0,radius),f.At(radius,0,radius),f.At(radius,0,-radius),
   {1,.77f,.31f,fade*.42f});
 for(int n=0;n<8;++n){const float a=n*tau/8;const auto p=f.At(std::cos(a)*(.3f+open*.6f),.3f+age*(1.2f+(n%3)*.35f),std::sin(a)*(.3f+open*.6f));
  r.DrawEffectSprite(Art::Slash,p,.035f,.17f,age,{1,.95f,.68f,fade*.7f});}
}
}
bool HasClassImpact(const rpg::CombatVisualEvent& e){
 if(!e.primary)return false;
 return (e.school==C::Warrior && (e.ability==1 || e.ability==8 || e.ability==9 || e.ability==11 || e.ability==14)) ||
   (e.school==C::Scout && (e.ability==1 || e.ability==6 || e.ability==9 || e.ability==11 || e.ability==14)) ||
   (e.school==C::Mage && (e.ability==1 || e.ability==9)) || (e.school==C::Priest && e.ability==6);
}
void ClassImpact(Renderer& r,const rpg::CombatVisualEvent& e,float age){
 if(!HasClassImpact(e) || !std::isfinite(age) || age<0 || age>=ContactLifetime(e) ||
   !std::isfinite(e.position.x)||!std::isfinite(e.position.z)||!std::isfinite(e.height)||!std::isfinite(e.facing) ||
   (e.kind!=rpg::CombatVisualKind::Hit && e.kind!=rpg::CombatVisualKind::HeavyHit))return;
 if(e.school==C::Warrior)Steel(r,e,age);
 else if(e.school==C::Scout)Pierce(r,e,age);
 else if(e.school==C::Mage)Meteor(r,e,age);
 else Sanctuary(r,e,age);
}
}
