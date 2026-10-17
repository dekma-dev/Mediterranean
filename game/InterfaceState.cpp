#include "game/Interface.h"
#include <algorithm>
namespace ui {
void Interface::Show(Panel next) {
  if (next==Panel::Character) { characterOpen=true; panel=Panel::None; }
  else if (next==Panel::Inventory) { inventoryOpen=true; panel=Panel::None; }
  else panel=next;
}
void Interface::SetOnly(Panel next) {
  characterOpen=inventoryOpen=false; panel=Panel::None; Show(next);
}
void Interface::Toggle(Panel next) {
  if (IsOpen(next)) Close(next); else Show(next);
}
void Interface::Close(Panel next) {
  if (next==Panel::Character) characterOpen=false;
  else if (next==Panel::Inventory) inventoryOpen=false;
  else panel=Panel::None;
}
bool Interface::IsOpen(Panel next) const {
  return next==Panel::Character?characterOpen:next==Panel::Inventory?inventoryOpen:panel==next;
}
bool Interface::PointerOverWindow() const {
  return panel!=Panel::None || (characterOpen && Rect{12,116,438,500}.Contains(mouseX_,mouseY_)) ||
    (inventoryOpen && Rect{994,180,276,468}.Contains(mouseX_,mouseY_));
}
Layout::Layout(float width,float height) {
  scale=std::max(.01f,std::min(width/1280.f,height/720.f)); x=(width-1280*scale)*.5f; y=(height-720*scale)*.5f;
}
const ItemInfo& Describe(Item item) {
  static const ItemInfo items[]={
    {Item::Starter,"Frontier sword","Main hand / Common","A serviceable iron blade carried by the\nDawnwatch militia. Reliable at close range."},
    {Item::Watchguard,"Watchguard blade","Main hand / Uncommon","A brass-hilted sword awarded for the\nfrontier patrol. Sharper and well balanced."},
    {Item::Potion,"Healing draught","Consumable","Restores up to 40 health.\nThree-second shared potion cooldown."},
    {Item::Tunic,"Dawnwatch tunic","Chest / Travel clothes","A blue wool tunic and a leather belt.\nWorn beneath the ward armor when earned."},
    {Item::Trousers,"Traveller's trousers","Legs / Travel clothes","Durable cloth trousers for long patrols.\nPart of your starting outfit."},
    {Item::Boots,"Weathered boots","Feet / Travel clothes","Leather boots with reinforced soles.\nPart of your starting outfit."},
    {Item::Ward,"Astral ward armor","Chest / Uncommon","Plate and pauldrons awarded for securing\nthe anchor. Automatically worn over your tunic."}};
  const auto index=static_cast<unsigned>(item); return items[index<7?index:0];
}
bool Owned(const rpg::Simulation& s,Item item) {
  return item==Item::Watchguard?s.HasRewardWeapon():item==Item::Ward?s.HasWardArmor():item==Item::Potion?s.Potions()>0:true;
}
bool Equipped(const rpg::Simulation& s,Item item) {
  return item==Item::Starter?!s.RewardWeaponEquipped():item==Item::Watchguard?s.RewardWeaponEquipped():
    item==Item::Ward?s.HasWardArmor():item==Item::Tunic?!s.HasWardArmor():item==Item::Trousers || item==Item::Boots;
}
bool CanUse(const rpg::Simulation& s,Item item) {
  if (!Owned(s,item) || s.Health()<=0) return false;
  if (item==Item::Potion) return s.PotionCooldown()==0 && s.Health()<s.MaxHealth();
  return (item==Item::Starter || item==Item::Watchguard) && !Equipped(s,item);
}
void ApplyItem(rpg::Command& command,const rpg::Simulation& s,Item item) {
  if (!CanUse(s,item)) return;
  if (item==Item::Potion) command.usePotion=true;
  else command.equipWeapon=true;
}
Event Interface::Pointer(float x,float y,float width,float height,bool clicked,bool rightClicked) {
  const Layout layout(width,height); mouseX_=layout.LocalX(x); mouseY_=layout.LocalY(y);
  if (panel==Panel::Map && rightClicked) {
    for (const auto& region : regions_) if (region.event.action==Action::PlaceWaypoint && region.rect.Contains(mouseX_,mouseY_)) hasWaypoint=false;
    return {};
  }
  if (!clicked) return {};
  for (auto it=regions_.rbegin();it!=regions_.rend();++it) if (it->rect.Contains(mouseX_,mouseY_)) {
    if (!it->enabled) return {};
    if (it->event.action==Action::PlaceWaypoint) {
      const float spanX=46*it->rect.w/it->rect.h;
      pendingWaypoint={(mouseX_-it->rect.x)/it->rect.w*spanX-spanX*.5f,23-(mouseY_-it->rect.y)/it->rect.h*46};
    }
    return it->event;
  }
  return {};
}
}

