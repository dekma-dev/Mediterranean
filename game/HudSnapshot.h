#pragma once
#include "game/SceneSnapshot.h"
#include "game/CommandExecution.h"
#include "game/CombatHud.h"
#include "game/QuestTracking.h"
#include "game/WorldInteraction.h"
#include "game/BuildRead.h"
#include <memory>

namespace ui {
// Evaluated actor data; no AI paths, authority pointers or borrowed nicknames.
struct HudActor : presentation::ActorFrame {
  HudActor()=default;
  HudActor(const rpg::Simulation&,const rpg::Enemy&);
  float MaxHealth()const{return maximum;}
  bool Enraged()const{return enraged;}
  bool Interruptible()const{return interruptible;}
  const char* Name()const{return name;}
  const char* AttackName()const{return attackName;}
  const char* Counterplay()const{return counterplay;}
  float maximum=0;
  bool enraged=false,interruptible=false,visibleCast=false,protectedActor=false,finisher=false,elite=false;
  // Enemy descriptions refer only to static catalog literals.
  const char* name="",*attackName="",*counterplay="";
};

// Read model shared by drawing and feedback. Capturing evaluates authority once;
// a retained copy survives later simulation mutation and destruction.
class HudSnapshot {
public:
  void Capture(const rpg::Simulation&,std::uint64_t sequence,const rpg::StepEvents* events=nullptr);
  std::uint64_t Sequence()const{return sequence_;}
  std::uint64_t Revision()const{return revision_;}
  std::size_t Bytes()const;
  std::uint64_t StorageGrowths()const{return growths_;}
#define HUD_VALUE(Name) const std::decay_t<decltype(std::declval<const rpg::Simulation&>().Name())>& Name()const{return Name##_;}
#define HUD_TEXT(Name) const char* Name()const{return Name##_.c_str();}
#include "game/HudValues.inc"
#undef HUD_VALUE
#undef HUD_TEXT
  const rpg::ClassInfo& ClassInfo()const{return rpg::DescribeClass(Class());}
  const rpg::CombatStyle& CombatStyleInfo()const{return rpg::ClassCombat(Class());}
  rpg::TalentInfo TalentDefinition(int i)const{return rpg::ClassTalent(Class(),i);}
  rpg::ItemInfo ItemDefinition(rpg::Item i)const{return rpg::DescribeClassItem(i,Class());}
  bool HasTalent(rpg::Talent i)const{return rpg::HasTalent(BuildMask(),i);}
  float BuildBonus(unsigned mask,float rpg::TalentInfo::*stat)const{return rpg::ClassBuildBonus(Class(),mask,stat);}
  rpg::BuildStats PreviewBuild(unsigned mask)const{return rpg::ReadBuildPreview(*this,mask);}
  const char* LoadoutStatus(const rpg::BuildCommand& c)const{return rpg::ReadLoadoutStatus(*this,c);}
  const char* TradeStatus(const rpg::TradeCommand& c)const{return rpg::ReadTradeStatus(*this,c);}
  int Loadout(int i)const{return i>=0 && i<3?loadouts_[i]:-1;}
  int BuybackCount(rpg::Item i)const{return rpg::ValidItem(i)?buyback_[int(i)]:0;}
  int ShopStock(int i)const{return i>=0 && i<int(shops_.size())?shops_[i].stock:0;}
  const char* PurchaseStatus(int i)const{return i>=0 && i<int(shops_.size())?shops_[i].status.c_str():"Unknown item";}
  const char* TalentStatus(rpg::Talent i)const{return rpg::ValidTalent(i)?talents_[int(i)].c_str():"Unknown talent";}
  rpg::SpellInfo Spell(int i)const{return rpg::ValidSpell(i)?abilities_[i].spell:rpg::ClassSpell(Class(),i);}
  float Cooldown(int i)const{return rpg::ValidSpell(i)?abilities_[i].cooldown:0;}
  float AbilityCooldown(int i)const{return rpg::ValidSpell(i)?abilities_[i].duration:0;}
  const char* AbilityStatus(int i)const{return rpg::ValidSpell(i)?abilities_[i].status.c_str():"DISABLED";}
  bool CanBufferAttack(int i)const{return rpg::ValidSpell(i) && abilities_[i].buffer;}
  bool NormallySteadyAttack(int i)const{return rpg::ValidSpell(i) && abilities_[i].normallySteady;}
  bool SteadyAttack(int i)const{return rpg::ValidSpell(i) && abilities_[i].steady;}
  bool DisciplineReady(int i)const{return rpg::ValidSpell(i) && abilities_[i].discipline;}
  bool ChainReady(int i)const{return rpg::ValidSpell(i) && abilities_[i].chain;}
  int HuntState(int i)const{return i>=0 && i<6?hunts_[i].state:0;}
  unsigned HuntMarks(int i)const{return i>=0 && i<6?hunts_[i].marks:0;}
  int HuntProgress(int i)const{return i>=0 && i<6?hunts_[i].progress:0;}
  bool HuntReady(int i)const{return i>=0 && i<6 && hunts_[i].ready;}
  bool ReplayAvailable(rpg::QuestId i)const{return int(i)>=0 && int(i)<int(replays_.size()) && replays_[int(i)];}
  rpg::Vec2 ExpeditionSite(int i)const{return i>=0 && i<3?sites_[i]:rpg::Vec2{};}
  const auto& Combatants()const{return actors_;}
  const auto& Dummies()const{return dummies_;}
  const HudActor& SparringOpponent()const{return trainer_;}
  const HudActor* TargetActor()const{return target_?&*target_:nullptr;}
  bool VisibleEnemyCast(const HudActor& a)const{return a.visibleCast;}
  bool CourtProtected(const HudActor& a)const{return a.protectedActor;}
  bool FinisherSetup(const HudActor& a)const{return a.finisher;}
  const rpg::WorldMap& Map()const{return *map_;}
  bool CanStand(rpg::Vec2 p)const{return Map().CanStand(p,.4f);}
  const auto& VisualEvents()const{return events_.values;}
  std::size_t VisualEventCount()const{return events_.count;}
  const QuestTracking& Quests()const{return quests_;}
  const OpportunityView& Opportunity()const{return opportunity_;}
  const PlayerCastView& Cast()const{return cast_;}
  const WorldInteraction& Interaction()const{return interaction_;}
  struct Warning {HudActor actor;bool threatens=false;};
  struct Warnings {std::array<Warning,3> rows{};unsigned count=0;};
  const Warnings& Incoming()const{return incoming_;}
private:
#define HUD_VALUE(Name) std::decay_t<decltype(std::declval<const rpg::Simulation&>().Name())> Name##_{};
#define HUD_TEXT(Name) std::string Name##_;
#include "game/HudValues.inc"
#undef HUD_VALUE
#undef HUD_TEXT
  struct Ability {rpg::SpellInfo spell{};float cooldown=0,duration=0;std::string status;bool buffer=false,normallySteady=false,steady=false,discipline=false,chain=false;};
  struct Shop {int stock=0;std::string status;};
  struct Hunt {int state=0,progress=0;unsigned marks=0;bool ready=false;};
  std::array<Ability,rpg::spells.size()> abilities_{};
  std::array<Shop,rpg::shopOffers.size()> shops_{};
  std::array<std::string,int(rpg::Talent::Count)> talents_{};
  std::array<int,3> loadouts_{};
  std::array<int,int(rpg::Item::Count)> buyback_{};
  std::array<Hunt,6> hunts_{};
  std::array<bool,int(rpg::QuestId::Hunt6)+1> replays_{};
  std::array<rpg::Vec2,3> sites_{};
  std::vector<HudActor> actors_;
  std::array<HudActor,3> dummies_{};
  HudActor trainer_;
  std::optional<HudActor> target_;
  // Current world geometry is immutable. Copy once, share read-only ownership
  // with retained packets; never point into a replaceable Simulation object.
  std::shared_ptr<const rpg::WorldMap> map_;
  rpg::StepEvents events_;
  QuestTracking quests_;
  OpportunityView opportunity_;
  PlayerCastView cast_;
  WorldInteraction interaction_;
  Warnings incoming_;
  std::uint64_t sequence_=0,revision_=0,growths_=0;
};
inline const QuestTracking& ReadQuestTracking(const HudSnapshot& s){return s.Quests();}
inline OpportunityView ReadOpportunity(const HudSnapshot& s){return s.Opportunity();}
inline PlayerCastView ReadPlayerCast(const HudSnapshot& s){return s.Cast();}
inline WorldInteraction ReadWorldInteraction(const HudSnapshot& s){return s.Interaction();}
}
