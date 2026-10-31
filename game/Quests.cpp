#include "game/Quests.h"
#include "QuestText.generated.h"
#include "game/Campaign.h"
#include "game/Hunts.h"
namespace rpg {
Vec2 QuestLocation(QuestId id) {
  if(HuntingQuest(id))return WorldMap::residents[9+HuntIndex(id)].position;
  return id>=QuestId::Watch8 && id<=QuestId::Watch16?WorldMap::residents[int(id)-int(QuestId::Watch8)].position:WorldMap::questGiver;
}
const QuestDefinition& DescribeQuest(QuestId id) {
  static const QuestDefinition empty{"","","","","","","",0,0,Item::None,false};
  if(HuntingQuest(id))return huntQuests[HuntIndex(id)];
  if(id>=QuestId::Watch1 && id<=QuestId::Watch16) return frontierQuests[int(id)-int(QuestId::Watch1)];
  return id==QuestId::Patrol?quest_text::patrol:id==QuestId::Anchor?quest_text::anchor:empty;
}
const char* QuestStory(QuestId id,QuestStage stage) {
  const auto& quest=DescribeQuest(id);
  switch (stage) {
  case QuestStage::Offer: return quest.offer;
  case QuestStage::Progress: return quest.progress;
  case QuestStage::TurnIn: return quest.turnIn;
  case QuestStage::Complete: return quest.complete;
  }
  return "";
}
}
