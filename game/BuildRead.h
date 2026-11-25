#pragma once
#include "game/Classes.h"
#include "game/Trade.h"

namespace rpg {
// Shared read-only rules. Authority and published HUD data use the same
// calculation; only Simulation may commit the resulting transaction.
inline float ClassBuildBonus(ClassId id,unsigned mask,float TalentInfo::*stat) {
  float value=0;for(int i=0;i<int(Talent::Count);++i)
    if(HasTalent(mask,Talent(i)))value+=ClassTalent(id,i).*stat;
  return value;
}
template<class State> BuildStats ReadBuildPreview(const State& s,unsigned mask) {
  if(!ValidBuild(mask,levelCap))mask=s.BuildMask();
  const auto delta=[&](float TalentInfo::*stat){return s.BuildBonus(mask,stat)-s.BuildBonus(s.BuildMask(),stat);};
  return {s.MaxHealth()+delta(&TalentInfo::health),s.StrikeDamage()+delta(&TalentInfo::strike),
    s.HeavyDamage()+delta(&TalentInfo::heavy),s.DamageReduction()+delta(&TalentInfo::armor),
    s.MendHealing()+delta(&TalentInfo::mend),s.AbilityCooldown(2)-delta(&TalentInfo::dashCooldown),
    s.AbilityCooldown(3)-delta(&TalentInfo::mendCooldown),s.AbilityCooldown(0)-delta(&TalentInfo::basicCooldown),
    s.AbilityCooldown(1)-delta(&TalentInfo::heavyCooldown),ClassRecoveryCast(s.Class())/(1+s.BuildBonus(mask,&TalentInfo::castSpeed))};
}
template<class State> const char* ReadLoadoutStatus(const State& s,const BuildCommand& command) {
  if(command.action!=BuildAction::Apply && command.action!=BuildAction::Store)return "Unknown build action";
  if(command.action==BuildAction::Store && (command.slot<0 || command.slot>=3))return "Choose a loadout slot";
  if(command.mask<0)return "Choose a build to preview";
  if(!ValidBuild(unsigned(command.mask),levelCap))return "Invalid talents or prerequisites";
  if(!ValidBuild(unsigned(command.mask),s.Level()))return "Earn more levels to use this build";
  if(s.Health()<=0)return "Recover before managing builds";
  if(command.action==BuildAction::Store)return s.Loadout(command.slot)==command.mask?"Already saved in this slot":"Ready to store";
  if(unsigned(command.mask)==s.BuildMask())return "Already your active build";
  const auto blocked=s.BuildChangeStatus();return *blocked?blocked:"Ready to apply";
}
template<class State> const char* ReadTradeStatus(const State& s,const TradeCommand& command) {
  if(!ValidItem(command.item))return "Unknown item";
  if(!s.CanTrade())return "Return to the quartermaster";
  if(command.action==TradeAction::Sell){
    if(command.source<0 || command.source>=Inventory::capacity || s.Items().Bag()[command.source].item!=command.item)
      return "Select an item in your bag";
    return "Available";
  }
  if(command.action!=TradeAction::Buyback)return "Unknown transaction";
  if(s.BuybackCount(command.item)==0)return "No item to reclaim";
  if(s.Gold()<SellPrice(command.item))return "Not enough gold";
  auto candidate=s.Items();if(!candidate.Add(command.item))return "Make room in your bag";
  return "Available";
}
}
