#include "game/Inventory.h"
#include <algorithm>
#include <istream>
#include <ostream>
#include <string>

namespace rpg {
bool ValidItem(Item item) { return int(item)>=0 && item<Item::Count; }
const ItemInfo& DescribeItem(Item item) {
  static const ItemInfo empty{Item::None,"Empty slot","No equipment","No item equipped.",GearSlot::None,0,6,10,0};
  static const ItemInfo items[]={
    {Item::Starter,"Frontier sword","Main hand / Common","A serviceable iron blade carried by the\nDawnwatch militia.",GearSlot::Weapon,1,20,35},
    {Item::Watchguard,"Watchguard blade","Main hand / Uncommon","A brass-hilted sword awarded for the\nfrontier patrol.",GearSlot::Weapon,1,27,45},
    {Item::Potion,"Healing draught","Consumable","Restores up to 40 health.\nThree-second shared potion cooldown.",GearSlot::None,20},
    {Item::Tunic,"Dawnwatch tunic","Chest / Travel clothes","Blue wool, leather belt and brass buckle.\nYour starting chest piece.",GearSlot::Chest,1},
    {Item::Trousers,"Traveller's trousers","Legs / Travel clothes","Durable cloth trousers for long patrols.",GearSlot::Legs,1},
    {Item::Boots,"Weathered boots","Feet / Travel clothes","Leather boots with reinforced soles.",GearSlot::Feet,1},
    {Item::Ward,"Astral ward armor","Chest / Uncommon","Plate and pauldrons earned at the anchor.\nReduces incoming damage by 2.",GearSlot::Chest,1,0,0,2},
    {Item::Helm,"Watchman's helm","Head / Uncommon","Steel cap with a brass brow and cheek guards.\nReduces incoming damage by 1.",GearSlot::Head,1,0,0,1},
    {Item::Shield,"Dawnwatch shield","Off hand / Uncommon","An oak shield faced with blue cloth and brass.\nReduces incoming damage by 2.",GearSlot::Offhand,1,0,0,2}};
  return ValidItem(item)?items[int(item)]:empty;
}
Inventory::Inventory() {
  gear_={Item::Starter,Item::Tunic,Item::Trousers,Item::Boots,Item::None,Item::None}; bag_[0]={Item::Potion,2};
}
Item Inventory::Worn(GearSlot slot) const { return int(slot)>=0 && int(slot)<gearCount?gear_[int(slot)]:Item::None; }
int Inventory::Count(Item item) const {
  if (!ValidItem(item)) return 0;
  int count=0; for (const auto& stack : bag_) if (stack.item==item) count+=stack.count;
  for (auto worn : gear_) if (worn==item) ++count;
  return count;
}
int Inventory::Find(Item item) const {
  if (!ValidItem(item)) return -1;
  for (int i=0;i<capacity;++i) if (bag_[i].item==item) return i;
  return -1;
}
int Inventory::UsedSlots() const { return int(std::count_if(bag_.begin(),bag_.end(),[](const ItemStack& s) { return s.count>0; })); }
bool Inventory::Add(Item item,int count) {
  if (!ValidItem(item) || count<=0 || count>capacity*DescribeItem(item).stackLimit) return false;
  if (DescribeItem(item).stackLimit==1 && (count!=1 || Count(item)>0)) return false;
  auto next=bag_; const int limit=DescribeItem(item).stackLimit;
  for (auto& stack : next) if (stack.item==item) {
    const int added=std::min(count,limit-stack.count); stack.count+=added; count-=added;
  }
  for (auto& stack : next) if (stack.item==Item::None && count>0) {
    const int added=std::min(count,limit); stack={item,added}; count-=added;
  }
  if (count!=0) return false;
  bag_=next; return true;
}
bool Inventory::Consume(Item item) {
  if (item!=Item::Potion) return false;
  const int at=Find(item); if (at<0) return false;
  if (--bag_[at].count==0) bag_[at]={};
  return true;
}
bool Inventory::Equip(Item item) {
  return Apply({InventoryAction::Equip,Find(item),int(DescribeItem(item).slot),item});
}
bool Inventory::Apply(const InventoryCommand& command) {
  const int from=command.source,to=command.destination;
  if (command.action==InventoryAction::Sort) { Sort(); return true; }
  if (command.action==InventoryAction::Unequip) {
    if (from<0 || from>=gearCount || gear_[from]==Item::None || gear_[from]!=command.expected) return false;
    int target=to;
    if (target==-1) for (int i=0;i<capacity;++i) if (bag_[i].item==Item::None) { target=i; break; }
    if (target<0 || target>=capacity) return false;
    const auto replacement=bag_[target];
    if (replacement.item!=Item::None && (replacement.count!=1 || int(DescribeItem(replacement.item).slot)!=from)) return false;
    bag_[target]={gear_[from],1}; gear_[from]=replacement.item; return true;
  }
  if (from<0 || from>=capacity || bag_[from].item==Item::None || bag_[from].item!=command.expected) return false;
  if (command.action==InventoryAction::Equip) {
    if (to<0 || to>=gearCount || int(DescribeItem(bag_[from].item).slot)!=to || bag_[from].count!=1) return false;
    const auto old=gear_[to]; gear_[to]=bag_[from].item; bag_[from]={old,old==Item::None?0:1}; return true;
  }
  if (command.action!=InventoryAction::Move || to<0 || to>=capacity || from==to) return false;
  if (bag_[to].item==bag_[from].item) {
    const int transferred=std::min(bag_[from].count,DescribeItem(bag_[to].item).stackLimit-bag_[to].count);
    if (transferred==0) return false;
    bag_[to].count+=transferred; bag_[from].count-=transferred;
    if (bag_[from].count==0) bag_[from]={};
  } else std::swap(bag_[from],bag_[to]);
  return true;
}
void Inventory::Sort() {
  std::array<int,int(Item::Count)> totals{};
  for (auto stack : bag_) if (ValidItem(stack.item)) totals[int(stack.item)]+=stack.count;
  bag_.fill({}); int index=0;
  for (int item=0;item<int(Item::Count);++item) while (totals[item]>0) {
    const int count=std::min(totals[item],DescribeItem(Item(item)).stackLimit);
    bag_[index++]={Item(item),count}; totals[item]-=count;
  }
}
void Inventory::Write(std::ostream& out) const {
  out<<"GEAR"; for (auto item : gear_) out<<' '<<int(item);
  out<<"\nBAG"; for (auto stack : bag_) out<<' '<<int(stack.item)<<' '<<stack.count;
  out<<'\n';
}
bool Inventory::Read(std::istream& input,int savedGearCount) {
  if (savedGearCount!=4 && savedGearCount!=gearCount) return false;
  Inventory candidate; std::string tag;
  if (!(input>>tag) || tag!="GEAR") return false;
  for (int i=0;i<savedGearCount;++i) {
    int item=0; if (!(input>>item) || item<-1 || item>=int(Item::Count) ||
      (item!=-1 && int(DescribeItem(Item(item)).slot)!=i)) return false;
    candidate.gear_[i]=Item(item);
  }
  if (!(input>>tag) || tag!="BAG") return false;
  for (auto& stack : candidate.bag_) {
    int item=0,count=0;
    if (!(input>>item>>count) || item<-1 || item>=int(Item::Count) || count<0 ||
      (item==-1 && count!=0) || (item!=-1 && (count==0 || count>DescribeItem(Item(item)).stackLimit))) return false;
    stack={Item(item),count};
  }
  for (int i=0;i<int(Item::Count);++i) if (DescribeItem(Item(i)).stackLimit==1 && candidate.Count(Item(i))>1) return false;
  *this=candidate; return true;
}
bool Inventory::MatchesCampaign(bool rewardWeapon,bool wardArmor,int potions,bool helm,bool shield) const {
  for (auto item : {Item::Starter,Item::Tunic,Item::Trousers,Item::Boots}) if (Count(item)!=1) return false;
  return Count(Item::Watchguard)==int(rewardWeapon) && Count(Item::Ward)==int(wardArmor) && Count(Item::Potion)==potions &&
    Count(Item::Helm)==int(helm) && Count(Item::Shield)==int(shield);
}
Inventory Inventory::Migrate(int potions,bool rewardWeapon,bool equipped,bool wardArmor) {
  Inventory result; result.bag_.fill({}); if (potions>0) result.Add(Item::Potion,potions);
  if (rewardWeapon) { result.Add(Item::Watchguard); if (equipped) result.Equip(Item::Watchguard); }
  if (wardArmor) { result.Add(Item::Ward); result.Equip(Item::Ward); }
  return result;
}
}
