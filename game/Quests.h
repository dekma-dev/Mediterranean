#pragma once
#include "game/Inventory.h"
#include "game/WorldMap.h"
namespace rpg {
enum class QuestId { None,Patrol,Anchor,Watch1,Watch2,Watch3,Watch4,Watch5,Watch6,Watch7,Watch8,Watch9,Watch10,Watch11,Watch12,Watch13,Watch14,Watch15,Watch16 };
inline constexpr int frontierQuestCount=16;
enum class QuestStage { Offer,Progress,TurnIn,Complete };
enum class QuestAction { None,Close,AcceptPatrol,CompletePatrol,AcceptAnchor,CompleteAnchor,AcceptFrontier,CompleteFrontier };
struct QuestDefinition {
  const char* title;
  const char* speaker;
  const char* objective;
  const char* offer;
  const char* progress;
  const char* turnIn;
  const char* complete;
  int gold,experience;
  Item item;
  bool autoEquip;
};
const QuestDefinition& DescribeQuest(QuestId id);
const char* QuestStory(QuestId id,QuestStage stage);
Vec2 QuestLocation(QuestId id);
}
