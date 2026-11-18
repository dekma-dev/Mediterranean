#include "game/CombatEffects.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
using namespace particles;
void Require(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 using C=rpg::ClassId;using K=rpg::CombatVisualKind;using M=impactfx::Material;
 const auto& world=impactfx::Environment();
 Contact wall;
 Require(world.Trace({4.9f,1,-8},{5.5f,1,-8},.02f,wall) && wall.normal.x<-.9f && !wall.ground,
   "Presentation broad phase must collide with the watch hall's solid wall");
 Contact above;
 Require(!world.Trace({4.9f,10,-8},{5.5f,10,-8},.02f,above),"Scenery bounds must end at the solid prop height");
 const float floor=rpg::WorldMap::Height({-52,36});Contact slope;
 Require(world.Trace({-52,floor+.1f,36},{-52,floor-.1f,36},.02f,slope) && slope.ground && slope.normal.y>.4f,
   "Terrain collision must follow the expanded map heightfield");
 struct Case {C school;int ability;M material;};
 for(const auto& c:{Case{C::Warrior,0,M::Steel},Case{C::Scout,1,M::Wood},Case{C::Mage,0,M::Ember},
   Case{C::Mage,8,M::Frost},Case{C::Mage,13,M::Arcane},Case{C::Priest,1,M::Holy}}){
  CombatEffects effects;rpg::CombatVisualEvent event{K::Hit,{-4,8},1.25f,.4f,100,.017f};event.school=c.school;event.ability=c.ability;
  auto release=event;release.kind=K::SpellCast;effects.Emit(release);
  Require(effects.ImpactParticles().Count()==0,"A release cannot fabricate physical hit debris");
  auto evaded=event;evaded.evaded=true;effects.Emit(evaded);
  Require(effects.ImpactParticles().Count()==0,"An evaded hit cannot fabricate fragments");
  effects.Emit(event);Require(effects.ImpactParticles().Count()==16,"Normal confirmed hits must emit one bounded contact fan");
  const auto& first=effects.ImpactParticles().Items()[0];
  Require(first.style==std::uint8_t(c.material) && first.ticks==2,"Element or event frame-tail timing was lost");
  auto secondary=event;secondary.primary=false;effects.Emit(secondary);
  Require(effects.ImpactParticles().Count()==19,"Secondary area victims must use reduced debris");
  Require(effects.ImpactParticles().Items()[16].style==std::uint8_t(c.material),"Secondary contacts must retain the accepted spell's element");
  for(int i=0;i<40;++i)effects.Advance(.05f);
  Require(effects.ImpactParticles().Count()==0,"A finished contact left live debris");
  effects.Emit(event);effects.Reset();Require(effects.ImpactParticles().Count()==0,"Load/death/menu reset must clear physical debris");
 }
 impactfx::System thermal;rpg::CombatVisualEvent mixed{K::HeavyHit,{-4,8},1.25f,0,100};mixed.school=C::Mage;mixed.ability=14;
 thermal.Emit(mixed,17);unsigned ice=0,embers=0;
 for(const auto& p:thermal.State().Items())if(p.Alive()){ice+=p.style==std::uint8_t(M::Frost);embers+=p.style==std::uint8_t(M::Ember);}
 Require(ice==12 && embers==12,"Thermal rupture must preserve both accepted elements");
 // Stress the real terrain/grid, not a flat test plane. Report CPU time as a
 // local measurement only; no machine-dependent FPS assertion.
 impactfx::System stress;rpg::CombatVisualEvent event{K::HeavyHit,{-52,36},floor+1.2f,0,100};event.school=C::Mage;event.ability=0;
 const auto start=std::chrono::steady_clock::now();unsigned peak=0,contacts=0;
 for(unsigned frame=0;frame<600;++frame){if(frame%6==0)stress.Emit(event,frame+1);stress.Advance(1.f/60);
  peak=std::max(peak,stress.State().Count());contacts=stress.State().CollisionCount();
  for(const auto& p:stress.State().Items())if(p.Alive())Require(p.position.y>=rpg::WorldMap::Height({p.position.x,p.position.z})+p.radius-.003f,
    "Physical impact debris penetrated the live terrain");
 }
 const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 Require(peak==Pool::capacity && contacts>0,"Saturated impacts must stay bounded and still collide");
 Require(sizeof(CombatEffects)<=32768,"Physical effects exceeded the declared CPU state budget");
 std::cout<<"PASS: real scenery/terrain, six material responses, event ownership, area caps and lifecycle; peak="<<peak
   <<", contacts="<<contacts<<", state bytes="<<sizeof(CombatEffects)<<", stress incl. terrain assertions="<<ms/600<<" ms/update\n";
}
