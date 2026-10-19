#pragma once
#include "game/Inventory.h"
#include <array>

namespace rpg {
enum class TradeAction { None,Sell,Buyback };
struct TradeCommand { TradeAction action=TradeAction::None; Item item=Item::None; int source=-1; };
inline int SellPrice(Item item) {
  constexpr std::array<int,int(Item::Count)> prices{4,10,2,3,2,2,15,10,17,3,2,3,4};
  return ValidItem(item)?prices[int(item)]:0;
}
struct ShopOffer { Item item; int price,stock; bool patrolRequired; };
inline constexpr std::array<ShopOffer,3> shopOffers{{
  {Item::Potion,6,6,false}, {Item::Helm,20,1,true}, {Item::Shield,35,1,true}
}};
}
