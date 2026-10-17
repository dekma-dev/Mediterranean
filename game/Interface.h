#pragma once
#include "game/Simulation.h"
#include <vector>
class Renderer;
namespace ui {
enum class Panel { None,Menu,Character,Inventory,Map,Journal };
enum class Action { None,Close,Menu,Character,Inventory,Map,Journal,Save,Load,Fullscreen,Quit,SelectItem,UseItem,Ability,ClearWaypoint,PlaceWaypoint };
enum class Item { Starter,Watchguard,Potion,Tunic,Trousers,Boots,Ward };
struct Rect { float x,y,w,h; bool Contains(float px,float py) const { return px>=x && py>=y && px<x+w && py<y+h; } };
struct Layout {
  float scale,x,y;
  Layout(float width,float height);
  float LocalX(float px) const { return (px-x)/scale; }
  float LocalY(float py) const { return (py-y)/scale; }
};
struct Event { Action action=Action::None; int value=0; };
struct Region { Rect rect; Event event; bool enabled; };
struct ItemInfo { Item id; const char* name; const char* type; const char* description; };
const ItemInfo& Describe(Item item);
bool Owned(const rpg::Simulation& simulation,Item item);
bool Equipped(const rpg::Simulation& simulation,Item item);
bool CanUse(const rpg::Simulation& simulation,Item item);
void ApplyItem(rpg::Command& command,const rpg::Simulation& simulation,Item item);
class Interface {
public:
  void Escape() { panel=panel==Panel::Menu?Panel::None:Panel::Menu; }
  void Toggle(Panel next);
  void Show(Panel next);
  void SetOnly(Panel next);
  void Close(Panel next);
  bool IsOpen(Panel next) const;
  bool HasPanels() const { return panel!=Panel::None || characterOpen || inventoryOpen; }
  unsigned StateKey() const { return unsigned(panel)*4+unsigned(characterOpen)*2+unsigned(inventoryOpen); }
  bool PointerOverWindow() const;
  Event Pointer(float x,float y,float width,float height,bool clicked,bool rightClicked);
  void Render(Renderer& renderer,const rpg::Simulation& simulation,float time,bool debug);
  bool CapturesMovement() const { return panel!=Panel::None; }
  // Central overlays are modal; character and bag are independent side windows.
  Panel panel=Panel::None;
  bool characterOpen=false,inventoryOpen=false;
  Item selected=Item::Starter;
  bool fullscreen=true,hasWaypoint=false;
  std::string menuNotice;
  rpg::Vec2 waypoint{},pendingWaypoint{};
private:
  std::vector<Region> regions_;
  float mouseX_=-100,mouseY_=-100;
};
}
