#pragma once
#include "game/Simulation.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace ui {
struct HudAnchor { float x,y; };
struct CombatHudLayout {
  HudAnchor player,target;
  static constexpr float statusOffset=73,castOffset=109;
};
inline CombatHudLayout CombatHudAnchors(bool panels) {
  return panels?CombatHudLayout{{18,23},{475,23}}:CombatHudLayout{{318,399},{778,405}};
}

// A cast meter describes preparation, not the entire attack/recovery cycle.
// Use the same accepted clock and release boundary as simulation and effects.
struct PlayerCastView {
  int ability=-1;
  float progress=0,remaining=0;
};
inline PlayerCastView ReadPlayerCast(const rpg::Simulation& state) {
  if(state.Health()<=0) return {};
  if(state.CastingRecovery())
    return {3,state.RecoveryProgress(),state.RecoveryCastDuration()*(1-state.RecoveryProgress())};
  if(!state.Attacking() || state.ClassInfo().range<=3) return {};
  const float release=combat::StrokeStart(state.HeavyAttack());
  if(state.AttackSeconds()>=release) return {};
  return {state.AttackAbility(),std::clamp(state.AttackSeconds()/release,0.f,1.f),
    std::max(0.f,(release-state.AttackSeconds())/state.AttackRate())};
}

// A short receipt after a real release/cancellation, never a fabricated cast.
// Driven by simulation updates so repeated renders cannot advance its lifetime.
struct CastReceipt {
  int ability=-1,pending=-1;
  float age=1;
  bool interrupted=false;
  rpg::ClassId school=rpg::ClassId::Warrior;
  void Reset() { *this={}; }
  float Opacity() const {
    const float duration=interrupted?.48f:.28f;
    return ability<0?0:1-std::clamp(age/duration,0.f,1.f);
  }
  void Update(float dt,const rpg::Simulation& state) {
    if(!std::isfinite(dt) || dt<=0 || dt>.25f || state.Health()<=0) {Reset();return;}
    if(school!=state.Class())Reset();school=state.Class();age+=dt;
    const auto cast=ReadPlayerCast(state);bool released=false;int action=-1;float elapsed=0;
    for(std::size_t n=0;n<state.VisualEventCount();++n) {
      const auto& e=state.VisualEvents()[n];if(e.actor!=0)continue;
      if((e.kind==rpg::CombatVisualKind::SpellCast && state.ClassInfo().range>3 && rpg::ValidSpell(e.ability)) ||
         (e.kind==rpg::CombatVisualKind::Mend && pending==3)) {
        released=true;action=e.kind==rpg::CombatVisualKind::Mend?3:e.ability;elapsed=std::clamp(e.elapsed,0.f,dt);
      }
    }
    if(cast.ability>=0) {pending=cast.ability;ability=-1;return;}
    if(released) {ability=action;interrupted=false;age=elapsed;}
    else if(pending>=0) {ability=pending;interrupted=true;age=0;}
    // Cancel events after a completed release must not paint a successful shot
    // red when movement merely ends its follow-through.
    pending=-1;if(Opacity()<=0)ability=-1;
  }
};

// Per ability, not per slot: rearranging icons cannot move the wrong receipt.
// Rendering never advances these clocks or spends a spell.
struct AbilityFeedback {
  std::array<float,rpg::spells.size()> previous{},readyAge{},useAge{};
  rpg::ClassId school=rpg::ClassId::Count;
  bool initialized=false;
  AbilityFeedback(){readyAge.fill(1);useAge.fill(1);}
  void Reset(){*this=AbilityFeedback{};}
  float Ready(int ability) const {return ability<0 || ability>=int(readyAge.size())?0:std::max(0.f,1-readyAge[ability]/.55f);}
  float Used(int ability) const {return ability<0 || ability>=int(useAge.size())?0:std::max(0.f,1-useAge[ability]/.24f);}
  void Update(float dt,const rpg::Simulation& state) {
    if(!std::isfinite(dt) || dt<=0 || dt>.25f || state.Health()<=0){Reset();return;}
    if(school!=state.Class())Reset();school=state.Class();
    for(int i=0;i<int(previous.size());++i){
      readyAge[i]=std::min(1.f,readyAge[i]+dt);useAge[i]=std::min(1.f,useAge[i]+dt);
      const float remaining=i==4?state.PotionCooldown():state.Cooldown(i);
      if(initialized && previous[i]>0 && remaining<=0)readyAge[i]=std::max(0.f,dt-previous[i]);
      // Buffered/instant actions do not necessarily have a preparation bar.
      if(initialized && remaining>previous[i]+.001f){useAge[i]=0;readyAge[i]=1;}
      previous[i]=remaining;
    }
    for(std::size_t n=0;n<state.VisualEventCount();++n){
      const auto& event=state.VisualEvents()[n];if(event.actor!=0)continue;
      using K=rpg::CombatVisualKind;
      const int ability=event.kind==K::CastStart?event.ability:event.kind==K::DashStart?2:event.kind==K::Potion?4:
        event.kind==K::Mend && state.RecoveryCastDuration()<=0?3:-1;
      if(ability>=0 && ability<int(useAge.size())){useAge[ability]=std::clamp(event.elapsed,0.f,dt);readyAge[ability]=1;}
    }
    initialized=true;
  }
};

struct WorldPlate {
  rpg::EntityId id=0;
  float anchorX=0,anchorY=0,width=128,height=30,distance=0;
  int priority=0;
  float x=0,y=0;
  bool visible=false;
};
// Fixed capacity and bounded displacement: selected actors retain their place;
// less important labels never cover a higher-priority label or migrate across
// the screen. Identity breaks ties, independent of simulation roster ordering.
struct WorldPlates {
  static constexpr std::size_t capacity=24;
  std::array<WorldPlate,capacity> rows{};
  std::array<WorldPlate,8> reserved{};
  std::size_t count=0;
  std::size_t reservedCount=0;
  void Reserve(float x,float y,float width,float height) {
    if(reservedCount==reserved.size()) return;
    auto& area=reserved[reservedCount++];area.x=x;area.y=y;area.width=width;area.height=height;
  }
  static bool Before(const WorldPlate& a,const WorldPlate& b) {
    if(a.priority!=b.priority) return a.priority>b.priority;
    if(a.distance!=b.distance) return a.distance<b.distance;
    return a.id<b.id;
  }
  void Add(WorldPlate plate) {
    if(!std::isfinite(plate.anchorX) || !std::isfinite(plate.anchorY) || !std::isfinite(plate.distance)) return;
    std::size_t at=0;
    while(at<count && Before(rows[at],plate)) ++at;
    if(at==capacity) return;
    for(auto i=std::min(count,capacity-1);i>at;--i) rows[i]=rows[i-1];
    rows[at]=plate;count=std::min(count+1,capacity);
  }
  static bool Overlaps(const WorldPlate& a,const WorldPlate& b) {
    return a.x<b.x+b.width+4 && a.x+a.width+4>b.x && a.y<b.y+b.height+4 && a.y+a.height+4>b.y;
  }
  void Arrange() {
    for(std::size_t i=0;i<count;++i) {
      auto& plate=rows[i];plate.visible=false;
      plate.x=std::clamp(plate.anchorX-plate.width*.5f,4.f,1276.f-plate.width);
      const float desired=plate.anchorY-plate.height;
      // Candidate edges move continuously with their actors, avoiding the
      // staircase motion of quantized vertical packing.
      std::array<float,(capacity+8)*2+1> candidates{};std::size_t choices=1;
      candidates[0]=desired;
      for(std::size_t j=0;j<reservedCount;++j) {
        candidates[choices++]=reserved[j].y-plate.height-4;
        candidates[choices++]=reserved[j].y+reserved[j].height+4;
      }
      for(std::size_t j=0;j<i;++j) if(rows[j].visible) {
        candidates[choices++]=rows[j].y-plate.height-4;
        candidates[choices++]=rows[j].y+rows[j].height+4;
      }
      float best=65,bestY=0;
      for(std::size_t attempt=0;attempt<choices;++attempt) {
        plate.y=std::clamp(candidates[attempt],8.f,578.f-plate.height);
        if(std::abs(plate.y-desired)>64) continue;
        bool blocked=false;
        for(std::size_t j=0;j<reservedCount;++j) if(Overlaps(plate,reserved[j])) {blocked=true;break;}
        for(std::size_t j=0;j<i;++j) if(rows[j].visible && Overlaps(plate,rows[j])) {blocked=true;break;}
        if(!blocked && std::abs(plate.y-desired)<best) {
          plate.visible=true;best=std::abs(plate.y-desired);bestY=plate.y;
        }
      }
      if(plate.visible) plate.y=bestY;
    }
  }
};
}
