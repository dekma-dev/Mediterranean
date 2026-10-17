#pragma once
#include "game/Inventory.h"
#include <array>

namespace rpg {
struct ShopOffer { Item item; int price,stock; bool patrolRequired; };
inline constexpr std::array<ShopOffer,3> shopOffers{{
  {Item::Potion,6,6,false}, {Item::Helm,20,1,true}, {Item::Shield,35,1,true}
}};
}
