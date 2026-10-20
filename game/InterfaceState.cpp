#include "game/Interface.h"
#include <algorithm>
#include <cmath>
namespace ui {
void Interface::Show(Panel next) {
  CancelBuildConfirmation();
  if (next==Panel::Build && panel!=Panel::Build) loadoutsPage=false;
  const auto before=StateKey();
  if (next==Panel::Character) { characterOpen=true; if (panel!=Panel::Merchant) panel=Panel::None; }
  else if (next==Panel::Inventory) { inventoryOpen=true; if (panel!=Panel::Merchant) panel=Panel::None; }
  else { CancelDrag(); panel=next; }
  if (next==Panel::Merchant) inventoryOpen=true;
  if (before!=StateKey()) InvalidateHitRegions();
}
void Interface::SetOnly(Panel next) {
  selectedSpell=-1; CancelBuildConfirmation(); loadoutsPage=false; previewBuild=-1; selectedLoadout=0;
  CancelDrag(); InvalidateHitRegions(); characterOpen=inventoryOpen=false; panel=Panel::None; Show(next);
}
void Interface::Toggle(Panel next) {
  if (panel!=Panel::None && panel!=Panel::Merchant && (next==Panel::Character || next==Panel::Inventory)) { Show(next); return; }
  if (IsOpen(next)) Close(next); else Show(next);
}
void Interface::Close(Panel next) {
  CancelBuildConfirmation();
  CancelDrag(); InvalidateHitRegions();
  if (next==Panel::Character) characterOpen=false;
  else if (next==Panel::Inventory) inventoryOpen=false;
  else panel=Panel::None;
}
bool Interface::IsOpen(Panel next) const {
  return next==Panel::Character?characterOpen:next==Panel::Inventory?inventoryOpen:panel==next;
}
bool Interface::PointerOverWindow() const {
  return panel!=Panel::None || (characterOpen && Rect{12,106,438,510}.Contains(mouseX_,mouseY_)) ||
    (inventoryOpen && Rect{994,186,276,462}.Contains(mouseX_,mouseY_));
}
bool Interface::PointerOverUi() const {
  return PointerOverWindow() || std::any_of(regions_.begin(),regions_.end(),[&](const Region& region) {
    return region.rect.Contains(mouseX_,mouseY_);
  });
}
Layout::Layout(float width,float height) {
  scale=std::max(.01f,std::min(width/1280.f,height/720.f)); x=(width-1280*scale)*.5f; y=(height-720*scale)*.5f;
}
const ItemInfo& Describe(Item item) { return rpg::DescribeItem(item); }
bool Owned(const rpg::Simulation& s,Item item) { return s.Items().Count(item)>0; }
bool Equipped(const rpg::Simulation& s,Item item) {
  return rpg::ValidItem(item) && Describe(item).slot!=rpg::GearSlot::None && s.Items().Worn(Describe(item).slot)==item;
}
bool CanUse(const rpg::Simulation& s,Item item) {
  if (!Owned(s,item) || s.Health()<=0) return false;
  return item==Item::Potion?s.PotionCooldown()==0 && s.Health()<s.MaxHealth():Describe(item).slot!=rpg::GearSlot::None;
}
ItemSummary SummarizeItem(const rpg::Simulation& s,Item item) {
  if (!rpg::ValidItem(item)) return {};
  const auto& info=Describe(item);
  const auto signedValue=[](float value) { return (value>=0?std::string("+"):std::string())+std::to_string(int(value)); };
  if (item==Item::Potion) return {"Restores up to 40 health","Shared cooldown: 3 seconds",0};
  const auto worn=s.Items().Worn(info.slot); const auto& current=Describe(worn);
  ItemSummary result;
  if (info.slot==rpg::GearSlot::Weapon) {
    result.stats="Strike "+std::to_string(int(info.strike+s.StrikeBonus()))+"  /  Heavy "+std::to_string(int(info.heavy+s.HeavyBonus()));
    result.difference=info.strike-current.strike;
    result.comparison="Strike "+signedValue(result.difference)+"  /  Heavy "+signedValue(info.heavy-current.heavy);
  } else {
    result.stats="Damage reduction: "+std::to_string(int(info.armor));
    result.difference=info.armor-current.armor;
    result.comparison="Protection "+signedValue(result.difference);
  }
  result.comparison=worn==item?"Currently equipped":result.comparison+(worn==Item::None?" vs empty slot":" vs worn gear");
  return result;
}
void ApplyItem(rpg::Command& command,const rpg::Simulation& s,Item item,int bagSlot) {
  if (!CanUse(s,item)) return;
  if (bagSlot!=-1 && (bagSlot<0 || bagSlot>=rpg::Inventory::capacity || s.Items().Bag()[bagSlot].item!=item)) return;
  if (item==Item::Potion) { command.usePotion=true; command.potionSource=bagSlot; }
  else if (Equipped(s,item)) command.inventory={rpg::InventoryAction::Unequip,int(Describe(item).slot),-1,item};
  else command.inventory={rpg::InventoryAction::Equip,bagSlot==-1?s.Items().Find(item):bagSlot,int(Describe(item).slot),item};
}
Event Interface::Pointer(float x,float y,float width,float height,bool clicked,bool rightClicked,bool held,bool released) {
  const Layout layout(width,height); mouseX_=layout.LocalX(x); mouseY_=layout.LocalY(y);
  // Hit regions describe the last drawn layout, not a panel opened this frame.
  if (renderedState_!=StateKey() || width!=renderedWidth_ || height!=renderedHeight_) {
    CancelDrag(); return {};
  }
  const Region* hit=nullptr;
  for (auto it=regions_.rbegin();it!=regions_.rend();++it) if (it->rect.Contains(mouseX_,mouseY_)) { hit=&*it; break; }
  if (panel==Panel::Map && rightClicked) {
    if (hit && hit->event.action==Action::PlaceWaypoint) hasWaypoint=false;
    CancelDrag(); return {};
  }
  if (rightClicked) {
    CancelDrag();
    if (hit && hit->enabled && hit->event.action==Action::SelectItem && rpg::ValidItem(Item(hit->event.value))) {
      auto result=hit->event; result.action=Action::UseItem; return result;
    }
    return {};
  }
  if (dragItem_!=Item::None && (held || released) && std::hypot(mouseX_-pressX_,mouseY_-pressY_)>6) dragging_=true;
  if (released) {
    Event result;
    if (dragging_ && hit && (panel==Panel::None || panel==Panel::Merchant)) {
      if (hit->gear>=0 && dragBag_>=0) result={Action::InventoryChange,0,{rpg::InventoryAction::Equip,dragBag_,hit->gear,dragItem_}};
      else if (hit->bag>=0 && dragGear_>=0) result={Action::InventoryChange,0,{rpg::InventoryAction::Unequip,dragGear_,hit->bag,dragItem_}};
      else if (hit->bag>=0 && dragBag_>=0) result={Action::InventoryChange,0,{rpg::InventoryAction::Move,dragBag_,hit->bag,dragItem_}};
    }
    CancelDrag(); return result;
  }
  if (!held && !clicked) CancelDrag();
  if (!clicked) return {};
  CancelDrag();
  if (!hit || !hit->enabled) return {};
  if (hit->event.action==Action::SelectItem && (hit->bag>=0 || hit->gear>=0)) {
    dragBag_=hit->bag; dragGear_=hit->gear; dragItem_=Item(hit->event.value); pressX_=mouseX_; pressY_=mouseY_;
  }
  if (hit->event.action==Action::PlaceWaypoint) {
    const float spanX=46*hit->rect.w/hit->rect.h;
    pendingWaypoint={(mouseX_-hit->rect.x)/hit->rect.w*spanX-spanX*.5f,23-(mouseY_-hit->rect.y)/hit->rect.h*46};
  }
  return hit->event;
}
}
