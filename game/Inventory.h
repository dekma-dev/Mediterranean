#pragma once
#include <array>
#include <iosfwd>

namespace rpg {
enum class Item { None=-1,Starter,Watchguard,Potion,Tunic,Trousers,Boots,Ward,Helm,Shield,Shoulders,Gloves,Belt,Cloak,Count };
enum class GearSlot { None=-1,Weapon,Chest,Legs,Feet,Head,Offhand,Shoulders,Hands,Waist,Back,Count };
struct ItemInfo {
  Item id;
  const char* name;
  const char* type;
  const char* description;
  GearSlot slot;
  int stackLimit;
  float strike=0,heavy=0,armor=0;
};
const ItemInfo& DescribeItem(Item item);
bool ValidItem(Item item);
struct ItemStack { Item item=Item::None; int count=0; };
enum class InventoryAction { None,Move,Equip,Unequip,Sort,Split };
struct InventoryCommand {
  InventoryAction action=InventoryAction::None;
  int source=-1,destination=-1;
  Item expected=Item::None;
  int amount=0;
};
class Inventory {
public:
  static constexpr int capacity=24,gearCount=int(GearSlot::Count);
  Inventory();
  const std::array<ItemStack,capacity>& Bag() const { return bag_; }
  Item Worn(GearSlot slot) const;
  int Count(Item item) const;
  int Find(Item item) const;
  int UsedSlots() const;
  bool Add(Item item,int count=1);
  bool Consume(Item item,int source=-1);
  bool Remove(int source,Item expected);
  bool Equip(Item item);
  bool Apply(const InventoryCommand& command);
  void Sort();
  void Write(std::ostream& out) const;
  bool Read(std::istream& input,int savedGearCount=gearCount);
  bool MatchesCampaign(bool rewardWeapon,bool wardArmor,int potions,bool helm=false,bool shield=false,
    const std::array<int,int(Item::Count)>& sold={}) const;
  static Inventory Migrate(int potions,bool rewardWeapon,bool equipped,bool wardArmor);
private:
  std::array<ItemStack,capacity> bag_{};
  std::array<Item,gearCount> gear_{};
};
}
