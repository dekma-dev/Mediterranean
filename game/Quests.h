#pragma once
#include "game/Inventory.h"
#include "game/WorldMap.h"
namespace rpg {
enum class QuestId { None,Patrol,Anchor,Watch1,Watch2,Watch3,Watch4,Watch5,Watch6,Watch7,Watch8,Watch9,Watch10,Watch11,Watch12,Watch13,Watch14,Watch15,Watch16,Hunt1,Hunt2,Hunt3,Hunt4,Hunt5,Hunt6 };
inline constexpr int frontierQuestCount=16;
inline constexpr bool HuntingQuest(QuestId id) {return id>=QuestId::Hunt1 && id<=QuestId::Hunt6;}
inline constexpr int HuntIndex(QuestId id) {return int(id)-int(QuestId::Hunt1);}
inline constexpr int HuntRequired(int i) {return i==0||i==1?4:3;}
inline constexpr bool HuntGather(int i) {return i==2||i==5;}
enum class QuestStage { Offer,Progress,TurnIn,Complete };
enum class QuestAction { None,Close,AcceptPatrol,CompletePatrol,AcceptAnchor,CompleteAnchor,AcceptFrontier,CompleteFrontier,BeginReplay,FinishReplay,AcceptHunt,CompleteHunt };
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
