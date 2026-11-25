#include "game/HudSnapshot.h"
#include "game/AttackWarning.h"

namespace ui {
HudActor::HudActor(const rpg::Simulation& s,const rpg::Enemy& e):ActorFrame(e),
  maximum(e.MaxHealth()),enraged(e.Enraged()),interruptible(e.Interruptible()),
  visibleCast(s.VisibleEnemyCast(e)),protectedActor(s.CourtProtected(e)),finisher(s.FinisherSetup(e)),elite(e.elite),
  name(e.Name()),attackName(e.AttackName()),counterplay(e.Counterplay()) {}

void HudSnapshot::Capture(const rpg::Simulation& s,std::uint64_t sequence,const rpg::StepEvents* accepted){
  if(!map_)map_=std::make_shared<const rpg::WorldMap>(s.Map());
  if(actors_.capacity()<s.Combatants().size()){actors_.reserve(s.Combatants().size());++growths_;}
#define HUD_VALUE(Name) Name##_=s.Name();
#define HUD_TEXT(Name) Name##_=s.Name();
#include "game/HudValues.inc"
#undef HUD_VALUE
#undef HUD_TEXT
  for(int i=0;i<int(abilities_.size());++i){auto& a=abilities_[i];a.spell=s.Spell(i);a.cooldown=s.Cooldown(i);a.duration=s.AbilityCooldown(i);
    a.status=s.AbilityStatus(i);a.buffer=s.CanBufferAttack(i);a.normallySteady=s.NormallySteadyAttack(i);
    a.steady=s.SteadyAttack(i);a.discipline=s.DisciplineReady(i);a.chain=s.ChainReady(i);}
  for(int i=0;i<int(shops_.size());++i){shops_[i].stock=s.ShopStock(i);shops_[i].status=s.PurchaseStatus(i);}
  for(int i=0;i<int(talents_.size());++i)talents_[i]=s.TalentStatus(rpg::Talent(i));
  for(int i=0;i<3;++i){loadouts_[i]=s.Loadout(i);sites_[i]=s.ExpeditionSite(i);dummies_[i]=HudActor(s,s.Dummies()[i]);}
  for(int i=0;i<int(buyback_.size());++i)buyback_[i]=s.BuybackCount(rpg::Item(i));
  for(int i=0;i<6;++i)hunts_[i]={s.HuntState(i),s.HuntProgress(i),s.HuntMarks(i),s.HuntReady(i)};
  for(int i=0;i<int(replays_.size());++i)replays_[i]=s.ReplayAvailable(rpg::QuestId(i));
  actors_.clear();for(const auto& a:s.Combatants())actors_.emplace_back(s,a);
  trainer_=HudActor(s,s.SparringOpponent());
  if(const auto* a=s.TargetActor())target_=HudActor(s,*a);else target_.reset();
  const auto warnings=rpg::ReadAttackWarnings<3>(s);incoming_.count=warnings.count;
  for(unsigned i=0;i<warnings.count;++i)incoming_.rows[i]={HudActor(s,*warnings.rows[i].enemy),warnings.rows[i].threatens};
  if(accepted)events_=*accepted;else {events_.count=std::min(s.VisualEventCount(),events_.values.size());
    std::copy_n(s.VisualEvents().begin(),events_.count,events_.values.begin());events_.dropped=s.VisualEventsDropped();}
  quests_=ReadQuestTracking(s);opportunity_=ReadOpportunity(s);cast_=ReadPlayerCast(s);interaction_=ReadWorldInteraction(s);
  sequence_=sequence;++revision_;
}
std::size_t HudSnapshot::Bytes()const{
  std::size_t bytes=sizeof(*this)+actors_.capacity()*sizeof(HudActor);
  const auto add=[&](const auto& value){if constexpr(std::is_same_v<std::decay_t<decltype(value)>,std::string>){
    // Small-string storage already belongs to sizeof(*this).
    const auto data=reinterpret_cast<std::uintptr_t>(value.data()),object=reinterpret_cast<std::uintptr_t>(&value);
    if(data<object || data>=object+sizeof(value))bytes+=value.capacity()+1;
  }};
#define HUD_VALUE(Name) add(Name##_);
#define HUD_TEXT(Name) add(Name##_);
#include "game/HudValues.inc"
#undef HUD_VALUE
#undef HUD_TEXT
  for(const auto& a:abilities_)add(a.status);for(const auto& s:shops_)add(s.status);for(const auto& s:talents_)add(s);
  for(const auto& q:quests_.entries){add(q.title);add(q.detail);add(q.summary);}
  return bytes; // Shared immutable geometry is not per-packet storage.
}
}
