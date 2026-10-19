#include "game/Simulation.h"
#include "tests/QuestFixture.h"
#include <cstdlib>
#include <iostream>
#include <string>
namespace {
using namespace rpg;
void Require(bool ok,const char* text) { if (!ok) { std::cerr<<"FAIL: "<<text<<'\n'; std::exit(1); } }
void Open(Simulation& s) { Command c; c.interact=true; s.Update(.01f,c); }
void Choose(Simulation& s,QuestAction action) { Command c; c.questAction=action; s.Update(.01f,c); }
}
int main() {
  using namespace rpg;
  for (auto id : {QuestId::Patrol,QuestId::Anchor}) {
    const auto& q=DescribeQuest(id);
    Require(q.gold>0 && q.experience>0 && ValidItem(q.item) && std::string(q.title).size()>8,
      "Every quest needs a title and concrete rewards");
    for (auto stage : {QuestStage::Offer,QuestStage::Progress,QuestStage::TurnIn,QuestStage::Complete})
      Require(std::string(QuestStory(id,stage)).size()>150,"Every dialogue stage needs its authored story");
  }
  Simulation s;
  Choose(s,QuestAction::AcceptPatrol);
  Require(s.QuestState()==Quest::Available,"Confirmation without a conversation must not accept");
  const auto untouched=s.Save(); Open(s);
  Require(s.Save()==untouched && s.QuestDialogue()==QuestId::Patrol && s.DialogueStage()==QuestStage::Offer,
    "Opening the offer must not mutate progress or rewards");
  Choose(s,QuestAction::CompletePatrol);
  Require(s.Save()==untouched,"A stale or forged completion cannot bypass acceptance");
  Choose(s,QuestAction::Close);
  Require(s.Save()==untouched && s.QuestDialogue()==QuestId::None,"Declining must leave the offer available");
  Open(s); Choose(s,QuestAction::AcceptPatrol);
  Require(s.QuestState()==Quest::Active && s.DialogueStage()==QuestStage::Progress && s.Gold()==0,
    "Explicit acceptance must start the quest without granting rewards");
  const auto accepted=s.Save(); Choose(s,QuestAction::AcceptPatrol); Choose(s,QuestAction::CompletePatrol);
  Require(s.Save()==accepted,"Repeated acceptance and incomplete hand-in must do nothing");
  Require(s.Load(accepted) && s.QuestDialogue()==QuestId::None,"Loading must close transient dialogue");
  Open(s); Require(s.DialogueStage()==QuestStage::Progress && s.QuestPrimaryAction()==QuestAction::None,
    "An unfinished quest must display a reminder, not a reward button");

  Require(s.Load(test::patrolReturn),"Patrol completion fixture");
  const auto beforeReward=s.Save(); Open(s);
  Require(s.Save()==beforeReward && s.DialogueStage()==QuestStage::TurnIn,"Turn-in must preview rewards first");
  Choose(s,QuestAction::CompleteAnchor);
  Require(s.Save()==beforeReward,"A different quest's completion action must be rejected");
  const int oldXp=s.Experience(),oldGold=s.Gold();
  Choose(s,QuestAction::CompletePatrol);
  const auto& patrol=DescribeQuest(QuestId::Patrol);
  Require(s.QuestState()==Quest::Complete && s.Gold()==oldGold+patrol.gold &&
    s.Experience()==oldXp+patrol.experience && s.Items().Count(patrol.item)==1 &&
    s.Items().Worn(GearSlot::Weapon)==Item::Starter && s.Health()==s.MaxHealth() &&
    s.DialogueStage()==QuestStage::Complete,"Patrol receipt must match the actual item, gold, XP and healing");
  const auto rewarded=s.Save(); Choose(s,QuestAction::CompletePatrol);
  Require(s.Save()==rewarded,"Repeated completion must never duplicate rewards");
  Choose(s,QuestAction::Close); Open(s);
  Require(s.QuestDialogue()==QuestId::Anchor && s.DialogueStage()==QuestStage::Offer,
    "The next conversation must offer the unlocked follow-up");
  Choose(s,QuestAction::AcceptAnchor);
  Require(s.AnchorState()==AnchorQuest::Active && s.DialogueStage()==QuestStage::Progress,
    "The follow-up must require its own acceptance");
  Choose(s,QuestAction::CompleteAnchor);
  Require(s.AnchorState()==AnchorQuest::Active && s.Gold()==patrol.gold,"Unfinished anchor must not pay");

  Require(s.Load(test::anchorReturn),"Anchor completion fixture"); Open(s);
  const auto previousChest=s.Items().Worn(GearSlot::Chest);
  const int anchorXp=s.Experience(),anchorGold=s.Gold(); Choose(s,QuestAction::CompleteAnchor);
  const auto& anchor=DescribeQuest(QuestId::Anchor);
  Require(s.AnchorState()==AnchorQuest::Complete && s.Gold()==anchorGold+anchor.gold &&
    s.Experience()==anchorXp+anchor.experience && s.Items().Worn(GearSlot::Chest)==anchor.item &&
    s.Items().Find(previousChest)>=0 && s.Health()==s.MaxHealth() && s.DialogueStage()==QuestStage::Complete,
    "Anchor receipt must deliver and equip the advertised armor while retaining the old chest item");
  const auto finished=s.Save(); Choose(s,QuestAction::CompleteAnchor); Open(s); Choose(s,QuestAction::CompleteAnchor);
  Require(s.Save()==finished,"Reopening completed dialogue must not award anything twice");

  {
    Simulation early;
    std::string cleared=test::patrolReturn;
    const auto at=cleared.find("60 0 2 0"); cleared.replace(at,8,"60 0 0 0");
    Require(early.Load(cleared),"Pre-cleared patrol fixture"); Open(early); Choose(early,QuestAction::AcceptPatrol);
    Require(early.QuestState()==Quest::Return && early.Gold()==0,"Earlier kills should count, with a separate hand-in");
    Choose(early,QuestAction::AcceptPatrol);
    Require(early.Gold()==0,"A repeated accept cannot become an implicit hand-in");
  }
  for (bool jump : {false,true}) {
    Simulation interrupted; Open(interrupted); Command c; c.questAction=QuestAction::AcceptPatrol;
    if (jump) c.jump=true; else c.movement={1,0}; // Spawn is at the edge of the keeper's interaction range.
    interrupted.Update(.05f,c);
    Require(interrupted.QuestState()==Quest::Available && interrupted.QuestDialogue()==QuestId::None,
      "Leaving range or taking off must invalidate a pending confirmation");
  }
  {
    Simulation read; Open(read); const auto saved=read.Save();
    read.Update(0,{}); Require(read.Save()==saved,"Invalid time must not confirm a dialogue");
    Command reset; reset.restart=true; read.Update(.01f,reset);
    Require(read.QuestDialogue()==QuestId::None && read.QuestState()==Quest::Available,"Reset must discard dialogue state");
  }
  std::cout<<"PASS: quest offer/acceptance, reward previews/receipts, stale actions, lifecycle and catalog consistency.\n";
}
