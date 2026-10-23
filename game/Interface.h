#pragma once
#include "game/Simulation.h"
#include <vector>
class Renderer;
namespace ui {
enum class LoadingStage { World,Chronicles,Sound,Entering };
void RenderLoading(Renderer& renderer,LoadingStage stage);
enum class Panel { None,Menu,Character,Inventory,Map,Journal,Merchant,Build,Quest };
enum class Action { None,Close,Menu,Character,Inventory,Map,Journal,Save,Load,Fullscreen,Quit,SelectItem,UseItem,Ability,ClearWaypoint,PlaceWaypoint,InventoryChange,Purchase,Sell,Buyback,RotatePreview,MusicVolume,EffectsVolume,Build,SelectTalent,LearnTalent,QuestConfirm,QuestPage,SelectSpell,ConfirmTalentReset,ResetTalents,CancelTalentReset,BuildPage,SelectLoadout,SelectBuildPreset,RequestBuildApply,RequestBuildStore,ConfirmBuild,CancelBuild };
using Item=rpg::Item;
struct Rect { float x,y,w,h; bool Contains(float px,float py) const { return px>=x && py>=y && px<x+w && py<y+h; } };
struct Layout {
  float scale,x,y;
  Layout(float width,float height);
  float LocalX(float px) const { return (px-x)/scale; }
  float LocalY(float py) const { return (py-y)/scale; }
};
struct Event { Action action=Action::None; int value=0; rpg::InventoryCommand inventory; };
struct Region { Rect rect; Event event; bool enabled; int bag=-1,gear=-1; };
using ItemInfo=rpg::ItemInfo;
const ItemInfo& Describe(Item item);
bool Owned(const rpg::Simulation& simulation,Item item);
bool Equipped(const rpg::Simulation& simulation,Item item);
bool CanUse(const rpg::Simulation& simulation,Item item);
struct ItemSummary { std::string stats,comparison; float difference=0; };
ItemSummary SummarizeItem(const rpg::Simulation& simulation,Item item);
void ApplyItem(rpg::Command& command,const rpg::Simulation& simulation,Item item,int bagSlot=-1);
class Interface {
public:
  void Escape() { CancelDrag(); InvalidateHitRegions(); CancelBuildConfirmation(); panel=panel==Panel::Menu?Panel::None:Panel::Menu; }
  void Toggle(Panel next);
  void Show(Panel next);
  void SetOnly(Panel next);
  void Close(Panel next);
  bool IsOpen(Panel next) const;
  bool HasPanels() const { return panel!=Panel::None || characterOpen || inventoryOpen; }
  unsigned StateKey() const {
    const unsigned view=(unsigned(panel)*4+unsigned(characterOpen)*2+unsigned(inventoryOpen))*16+
      unsigned(selectedSpell+1)*2+unsigned(confirmTalentReset);
    return (((view*2+unsigned(loadoutsPage))*3+unsigned(pendingBuild.action))*513+
      unsigned(previewBuild+1))*4+unsigned(selectedLoadout+1);
  }
  void CancelBuildConfirmation() {
    if (confirmTalentReset || pendingBuild.action!=rpg::BuildAction::None) InvalidateHitRegions();
    confirmTalentReset=false; pendingBuild={};
  }
  bool PointerOverWindow() const;
  bool PointerOverUi() const;
  Event Pointer(float x,float y,float width,float height,bool clicked,bool rightClicked,bool held=false,bool released=false);
  void CancelDrag() { dragBag_=dragGear_=-1; dragItem_=Item::None; dragging_=false; }
  void InvalidateHitRegions() { regions_.clear(); renderedState_=~0u; }
  bool Dragging() const { return dragging_; }
  void Render(Renderer& renderer,const rpg::Simulation& simulation,float time,bool debug);
  bool CapturesMovement() const { return panel!=Panel::None; }
  // Central overlays are modal; character and bag are independent side windows.
  Panel panel=Panel::None;
  bool characterOpen=false,inventoryOpen=false;
  int selectedTalent=0;
  int selectedSpell=-1;
  bool confirmTalentReset=false;
  bool loadoutsPage=false;
  int previewBuild=-1,selectedLoadout=0;
  rpg::BuildCommand pendingBuild;
  int questPage=0;
  Item selected=Item::Starter;
  int selectedBag=-1;
  bool fullscreen=true,hasWaypoint=false;
  std::string menuNotice;
  std::string inventoryNotice;
  std::string merchantNotice;
  float noticeTime=0;
  float previewYaw=0;
  int musicVolume=35,effectsVolume=70;
  bool audioAvailable=false;
  rpg::Vec2 waypoint{},pendingWaypoint{};
private:
  std::vector<Region> regions_;
  unsigned renderedState_=~0u;
  int questTextKey_=-1;
  float renderedWidth_=0,renderedHeight_=0;
  float mouseX_=-100,mouseY_=-100;
  int dragBag_=-1,dragGear_=-1;
  Item dragItem_=Item::None;
  float pressX_=0,pressY_=0;
  bool dragging_=false;
};
}
