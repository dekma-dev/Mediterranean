#pragma once
#include "game/Inventory.h"
namespace rpg {
enum class QuestId { None,Patrol,Anchor };
enum class QuestStage { Offer,Progress,TurnIn,Complete };
enum class QuestAction { None,Close,AcceptPatrol,CompletePatrol,AcceptAnchor,CompleteAnchor };
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
}
