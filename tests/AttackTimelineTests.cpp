#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>

namespace {
void Check(bool value,const char* message) {
  if (!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
const char* duel="MEDITERRANEAN 3\n-4 5.2 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
  "100 -4 7 80 0 0 1 0.65 3.14159\n101 4 9 80 0 0 0 0 0\n102 0 12 120 0 0 0 0 0\n";
void Advance(rpg::Simulation& s,float duration,rpg::Command command={}) {
  for(float t=0;t<duration-1e-7f;) {
    const float dt=std::min(.01f,duration-t); s.Update(dt,command); t+=dt;
  }
}
void EnemyContactRegressions() {
  bool passed=true;
  const auto verify=[&](bool value,const char* message) {
    if (!value) { std::cerr<<message<<'\n'; passed=false; }
  };
  std::string imminent=duel;
  imminent.replace(imminent.find("0.65 3.14159"),4,"0.017");
  for (int hz : {22,30,60,144,1000}) {
    rpg::Simulation s; Check(s.Load(imminent),"Enemy contact fixture");
    int swings=0,hits=0;
    for (float t=0;t<.12f-1e-7f;) {
      const float dt=std::min(1.f/hz,.12f-t); s.Update(dt,{}); t+=dt;
      for (std::size_t i=0;i<s.VisualEventCount();++i) {
        const auto& e=s.VisualEvents()[i];
        if (e.kind==rpg::CombatVisualKind::EnemySwing || e.kind==rpg::CombatVisualKind::Hurt) {
          verify(std::abs(t-e.elapsed-.017f)<.00001f,"Enemy swing and hurt effects must preserve actual contact time across update rates");
          if (e.kind==rpg::CombatVisualKind::EnemySwing) ++swings; else ++hits;
        }
      }
    }
    verify(swings==1 && hits==1,"An enemy contact must emit one swing and one hurt event");
    verify(std::abs(s.Enemies()[0].attackCooldown-(s.Enemies()[0].AttackInterval()-(.12f-.017f)))<.00001f,
      "Enemy recovery must consume the part of the contact frame after impact");
    verify(std::abs(s.HurtFlash()-(.25f-(.12f-.017f)))<.00001f,
      "The hurt flash must age from contact rather than the end of its frame");
  }
  {
    // Enemy contact occurs in the first part of a frame split by our strike.
    std::string split=duel; split.replace(split.find("0.65 3.14159"),4,"0.185");
    rpg::Simulation s; Check(s.Load(split),"Split enemy contact fixture");
    rpg::Command c; c.nextTarget=true; c.ability=0; s.Update(.01f,c);
    Advance(s,.16f); s.Update(.05f,{});
    int swings=0;
    for (std::size_t i=0;i<s.VisualEventCount();++i) {
      const auto& e=s.VisualEvents()[i];
      if (e.kind==rpg::CombatVisualKind::EnemySwing || e.kind==rpg::CombatVisualKind::Hurt) {
        verify(std::abs(e.elapsed-.035f)<.00001f,"Enemy effect age must include the remainder after the player's contact split");
        if (e.kind==rpg::CombatVisualKind::EnemySwing) ++swings;
      }
    }
    verify(swings==1,"The split frame must contain the enemy swing");
  }
  for (int hz : {22,30,60,144,1000}) {
    // Equal-time enemy contact wins, including accumulated float round-off.
    std::string tie=duel;
    tie.replace(tie.find("5.2 100"),7,"5.2 1");
    tie.replace(tie.find("0.65 3.14159"),4,"0.19");
    rpg::Simulation s; Check(s.Load(tie),"Equal-time contact fixture");
    rpg::Command c; c.nextTarget=true; c.ability=0; s.Update(.01f,c);
    for (float t=0;t<.20f-1e-7f;) {
      const float dt=std::min(1.f/hz,.20f-t); s.Update(dt,{}); t+=dt;
    }
    verify(s.Health()==0 && s.Enemies()[0].health==80,
      "An equal-time fatal enemy hit must cancel player contact consistently across update rates");
  }
  {
    // The captain hits first in time but appears last in the actor array.
    const char* crowd="MEDITERRANEAN 3\n-4 5.2 1 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
      "100 -4 7 80 0 0 1 0.04 3.14159\n101 4 9 80 0 0 0 0 0\n102 -4 7 120 0 0 1 0.01 3.14159\n";
    for (float step : {.01f,.05f}) {
      rpg::Simulation s; Check(s.Load(crowd),"Fatal enemy ordering fixture");
      s.Update(step,{});
      int swings=0;
      for (std::size_t i=0;i<s.VisualEventCount();++i)
        if (s.VisualEvents()[i].kind==rpg::CombatVisualKind::EnemySwing) {
          ++swings;
          verify(s.VisualEvents()[i].actor==102,"The earliest fatal enemy contact must win independently of actor order and frame length");
        }
      verify(s.Health()==0 && swings==1,"Later enemy contacts must be cancelled on defeat");
    }
  }
  {
    const char* crowd="MEDITERRANEAN 3\n-4 5.2 100 0 0 0\n2 0 0 0\n0\n0 0 0 0\n"
      "100 -4 7 80 0 0 1 0.04 3.14159\n101 4 9 80 0 0 0 0 0\n102 -4 7 120 0 0 1 0.01 3.14159\n";
    rpg::Simulation s; Check(s.Load(crowd),"Surviving enemy ordering fixture");
    s.Update(.05f,{});
    int swings=0,hits=0;
    for (std::size_t i=0;i<s.VisualEventCount();++i) {
      const auto& e=s.VisualEvents()[i];
      if (e.kind==rpg::CombatVisualKind::EnemySwing) {
        verify(e.actor==(swings==0?102u:100u),"Nonfatal enemy contacts must also emit in time order");
        verify(std::abs(e.elapsed-(swings==0?.04f:.01f))<.00001f,"Each enemy needs its own within-frame contact age");
        ++swings;
      }
      if (e.kind==rpg::CombatVisualKind::Hurt) ++hits;
    }
    verify(swings==2 && hits==2 && s.Health()==100-s.Enemies()[0].Damage()-s.Enemies()[2].Damage(),
      "A later nonfatal hit must still apply exactly once");
    verify(std::abs(s.HurtFlash()-.24f)<.00001f,"The most recent hit must own the hurt flash");
    // A miss still starts recovery at contact, without a hurt effect.
    std::string miss=imminent; miss.replace(miss.find("5.2 100"),3,"3.2");
    Check(s.Load(miss),"Enemy miss timing fixture"); s.Update(.05f,{});
    verify(s.Health()==100 && s.VisualEventCount()==1 &&
      s.VisualEvents()[0].kind==rpg::CombatVisualKind::EnemySwing &&
      std::abs(s.VisualEvents()[0].elapsed-.033f)<.00001f &&
      std::abs(s.Enemies()[0].attackCooldown-(s.Enemies()[0].AttackInterval()-.033f))<.00001f,
      "A missed swing must retain contact timing and cooldown without damaging the player");
  }
  Check(passed,"Enemy contact regressions failed");
}
}
int main() {
  EnemyContactRegressions();
  for (int hz : {22,30,60,144,1000}) for(int ability : {0,1}) {
    rpg::Simulation s; Check(s.Load(duel),"Timeline fixture");
    rpg::Command start; start.nextTarget=true; start.ability=ability;
    s.Update(.01f,start);
    Check(s.Attacking() && s.Enemies()[0].health==80 && s.VisualEventCount()==0,
      "Activation must start wind-up, with no damage, trail or impact");
    const float contact=ability?.30f:.18f;
    int hits=0,swings=0; float impactTime=-1;
    for(float t=0;t<.81f;) {
      const float dt=std::min(1.f/hz,.81f-t);
      // Repeated input may not restart the clock or multiply damage.
      rpg::Command repeated; repeated.ability=ability; s.Update(dt,repeated); t+=dt;
      if(t<contact-1e-5f) Check(s.Enemies()[0].health==80,"Damage before contact");
      for(std::size_t i=0;i<s.VisualEventCount();++i) {
        const auto& e=s.VisualEvents()[i];
        if(e.kind==rpg::CombatVisualKind::Hit || e.kind==rpg::CombatVisualKind::HeavyHit) {
          ++hits; impactTime=t-e.elapsed;
        }
        if(e.kind==rpg::CombatVisualKind::Strike || e.kind==rpg::CombatVisualKind::HeavyStrike) ++swings;
      }
    }
    Check(hits==1 && swings==1 && std::abs(impactTime-contact)<.0001f,"One contact at the same time across update rates");
    Check(s.Enemies()[0].health==(ability?45:60),"Exactly one configured damage amount");
    Check(!s.Attacking() && s.AttackFlash()==0,"Recovery must finish and release the pose");
  }
  for(int cancellation=0;cancellation<4;++cancellation) {
    rpg::Simulation s; Check(s.Load(duel),"Cancel fixture");
    rpg::Command start; start.nextTarget=true; start.ability=1; s.Update(.01f,start);
    Advance(s,.1f);
    if(cancellation==0) s.StopMovement();
    if(cancellation==1) { rpg::Command modal; modal.haltMovement=true; modal.ability=0; s.Update(.01f,modal); }
    if(cancellation==2) Check(s.Load(s.Save()),"Load while winding up");
    if(cancellation==3) { rpg::Command reset; reset.restart=true; s.Update(.01f,reset); }
    Advance(s,.4f);
    Check(!s.Attacking() && s.Enemies()[0].health==80,"Cancelled wind-up must not hit later or replay on load");
  }
  {
    rpg::Simulation s; Check(s.Load(duel),"Target identity fixture");
    rpg::Command c; c.nextTarget=true; c.ability=0; s.Update(.01f,c);
    c.ability=-1; s.Update(.01f,c); const int selected=s.Target();
    Advance(s,.17f);
    Check(selected!=0 && s.Target()==selected && s.Enemies()[0].health==60 && s.Enemies()[selected].health==80,
      "Tab must not redirect an accepted strike to another actor");
  }
  {
    rpg::Simulation s; Check(s.Load(duel),"Retreat fixture");
    rpg::Command c; c.nextTarget=true; c.ability=1; s.Update(.01f,c);
    c={}; c.movement={0,-1}; c.sprint=true; Advance(s,.31f,c);
    Check(s.Enemies()[0].health==80,"Leaving melee range before contact must miss");
  }
  {
    // A counterattack inside the same update but before our contact must win.
    std::string fatal=duel;
    fatal.replace(fatal.find("5.2 100"),7,"5.2 1");
    fatal.replace(fatal.find("0.65 3.14159"),4,"0.165");
    rpg::Simulation s; Check(s.Load(fatal),"Fatal ordering fixture");
    rpg::Command c; c.nextTarget=true; c.ability=0; s.Update(.01f,c);
    Advance(s,.14f); s.Update(.05f,{});
    Check(s.Health()==0 && s.Enemies()[0].health==80 && !s.Attacking(),
      "Earlier fatal enemy contact must cancel the pending hit in the same update");
  }
  std::cout<<"Attack timing, identity, interruption and cancellation checks passed.\n";
}
