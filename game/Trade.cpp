#include "game/Simulation.h"
#include <cmath>

namespace rpg {
bool Simulation::NearQuartermaster() const {
  const auto position=WorldMap::quartermaster;
  return std::hypot(player_.x-position.x,player_.z-position.z)<=2.75f && HasLineOfSight(player_,position);
}
bool Simulation::CanTrade() const { return health_>0 && height_==0 && !sparring_ && NearQuartermaster(); }
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
}
