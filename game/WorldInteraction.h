#pragma once
#include "game/Simulation.h"
#include <algorithm>
#include <cmath>

namespace ui {
enum class WorldAction { None,Supply,Seal,Anchor,Beacon };
struct WorldInteraction {
  WorldAction kind=WorldAction::None;
  int site=-1;
  float progress=0,duration=0;
  const char* name="";
  bool Active() const {return kind!=WorldAction::None;}
};
template<class State> bool SupplyRecovered(const State& s,int site) {
  return site>=0 && site<3 && (s.FrontierCompleted()>13 ||
    (s.FrontierCompleted()==13 && (s.ExpeditionMask()&(1u<<site))));
}
inline WorldInteraction ReadWorldInteraction(const rpg::Simulation& s) {
  if(s.Health()<=0)return {};
  if(s.SupplyChannelSite()>=0)return {WorldAction::Supply,s.SupplyChannelSite(),s.SupplyChannelProgress(),3,"Recovering supplies"};
  if(s.CourtChannelSite()>=0)return {WorldAction::Seal,s.CourtChannelSite(),s.CourtChannelProgress(),2.5f,"Breaking the seal"};
  if(s.Channeling())return {WorldAction::Anchor,0,s.ChannelProgress(),rpg::Simulation::channelDuration,"Stabilizing the anchor"};
  if(s.RitualActive() && s.RitualCharge()>0 && s.RitualStage()<4)
    return {WorldAction::Beacon,s.RitualStage(),s.RitualCharge(),4,"Securing the beacon"};
  return {};
}
struct WorldInteractionReceipt {
  WorldInteraction action{};
  bool active=false,completed=false;
  float remaining=0;
  void Reset(){*this={};}
  float Opacity() const {return active?1.f:std::clamp(remaining/.35f,0.f,1.f);}
  template<class State> void Update(float dt,const State& s) {
    if(!std::isfinite(dt) || dt<=0 || dt>.25f)return;
    if(s.Health()<=0){Reset();return;}
    const auto now=ReadWorldInteraction(s);
    if(now.Active()){action=now;active=true;completed=false;remaining=0;return;}
    if(active) {
      completed=action.kind==WorldAction::Supply?SupplyRecovered(s,action.site):
        action.kind==WorldAction::Seal?(s.CourtSeals()&(1u<<action.site))!=0:
        action.kind==WorldAction::Anchor?s.AnchorState()>=rpg::AnchorQuest::Return:
        action.kind==WorldAction::Beacon?s.RitualStage()>action.site:false;
      active=false;remaining=1.2f;
      if(completed)action.progress=1;
    } else remaining=std::max(0.f,remaining-dt);
  }
};
}
