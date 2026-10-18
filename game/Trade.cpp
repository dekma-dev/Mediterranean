#include "game/Simulation.h"
#include <cmath>

namespace rpg {
bool Simulation::NearQuartermaster() const {
  const auto position=WorldMap::quartermaster;
  return std::hypot(player_.x-position.x,player_.z-position.z)<=2.75f && HasLineOfSight(player_,position);
}
bool Simulation::CanTrade() const { return health_>0 && Grounded() && !Dashing() && !sparring_ && NearQuartermaster(); }
int Simulation::ShopStock(int offer) const {
  return offer>=0 && offer<int(shopOffers.size())?shopOffers[offer].stock-purchases_[offer]:0;
}
int Simulation::GoldSpent() const {
  int result=0; for (int i=0;i<int(shopOffers.size());++i) result+=purchases_[i]*shopOffers[i].price;
  return result;
}
const char* Simulation::PurchaseStatus(int offer) const {
  if (offer<0 || offer>=int(shopOffers.size())) return "Unknown item";
  if (!CanTrade()) return "Return to the quartermaster";
  const auto& entry=shopOffers[offer];
  if (entry.patrolRequired && quest_!=Quest::Complete) return "Complete the frontier patrol";
  if (ShopStock(offer)==0) return "Sold out";
  if (gold_<entry.price) return "Not enough gold";
  auto candidate=inventory_;
  if (!candidate.Add(entry.item)) return "Make room in your bag";
  return "Available";
}
void Simulation::Buy(int offer) {
  if (std::string(PurchaseStatus(offer))!="Available") { message_=PurchaseStatus(offer); return; }
  const auto& entry=shopOffers[offer];
  // Item, stock and gold commit together; no UI path can bypass these checks.
  if (!inventory_.Add(entry.item)) return;
  gold_-=entry.price; ++purchases_[offer];
  message_=std::string("Bought ")+DescribeItem(entry.item).name+" for "+std::to_string(entry.price)+" gold. Stored in your bag.";
}
int Simulation::BuybackValue() const {
  int result=0; for (int i=0;i<int(Item::Count);++i) result+=buyback_[i]*SellPrice(Item(i));
  return result;
}
const char* Simulation::TradeStatus(const TradeCommand& command) const {
  if (!ValidItem(command.item)) return "Unknown item";
  if (!CanTrade()) return "Return to the quartermaster";
  if (command.action==TradeAction::Sell) {
    if (command.source<0 || command.source>=Inventory::capacity || inventory_.Bag()[command.source].item!=command.item)
      return "Select an item in your bag";
    return "Available";
  }
  if (command.action!=TradeAction::Buyback) return "Unknown transaction";
  if (BuybackCount(command.item)==0) return "No item to reclaim";
  if (gold_<SellPrice(command.item)) return "Not enough gold";
  auto candidate=inventory_;
  if (!candidate.Add(command.item)) return "Make room in your bag";
  return "Available";
}
void Simulation::Trade(const TradeCommand& command) {
  const auto status=TradeStatus(command);
  if (std::string(status)!="Available") { message_=status; return; }
  const int price=SellPrice(command.item);
  if (command.action==TradeAction::Sell) {
    if (!inventory_.Remove(command.source,command.item)) return;
    gold_+=price; ++buyback_[int(command.item)];
    message_=std::string("Sold one ")+DescribeItem(command.item).name+" for "+std::to_string(price)+" gold. Available in buyback.";
  } else {
    if (!inventory_.Add(command.item)) return;
    gold_-=price; --buyback_[int(command.item)];
    message_=std::string("Reclaimed ")+DescribeItem(command.item).name+" for "+std::to_string(price)+" gold. Stored in your bag.";
  }
}
}
