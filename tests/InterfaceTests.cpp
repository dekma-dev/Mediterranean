#include "game/Game.h"
#include "game/Interface.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

void Require(bool value,const char* message) { if (!value) { std::cerr<<message<<'\n'; std::exit(1); } }
std::string Fixture(const char* name) {
  std::ifstream input(std::string(RPG_TEST_FIXTURES)+"/"+name); std::ostringstream data; data<<input.rdbuf();
  Require(bool(input),"Missing test fixture."); return data.str();
}
int main() {
  ui::Interface screen;
  screen.Escape(); Require(screen.panel==ui::Panel::Menu && screen.CapturesMovement(),"Escape must open the menu and capture commands.");
  screen.Escape(); Require(screen.panel==ui::Panel::None,"Escape must close the menu.");
  screen.Toggle(ui::Panel::Inventory); screen.Escape();
  Require(screen.panel==ui::Panel::Menu,"Escape from another panel must open the menu.");
  for (auto panel : {ui::Panel::Menu,ui::Panel::Inventory,ui::Panel::Character,ui::Panel::Map,ui::Panel::Journal}) {
    Game game; game.Initialize(); Require(game.RestoreSnapshot(Fixture("combat-preview.save")),"Combat fixture must load.");
    game.OpenPanel(panel); const float before=game.State().Health(); const Input idle;
    for (int frame=0;frame<20;++frame) game.Update(.05,idle,0);
    Require(game.State().Health()<before,"Enemy windup/damage must continue while a menu or panel is open.");
  }
  rpg::Simulation simulation; rpg::Command command;
  ui::ApplyItem(command,simulation,ui::Item::Watchguard); Require(!command.equipWeapon,"Unowned gear must not equip.");
  ui::ApplyItem(command,simulation,ui::Item::Potion); Require(!command.usePotion,"Full health must disable potion use.");
  Require(simulation.Load(Fixture("veteran-preview.save")),"Veteran fixture must load.");
  ui::ApplyItem(command,simulation,ui::Item::Starter); Require(command.equipWeapon,"Unequipped starter weapon must be selectable.");
  simulation.Update(.01f,command); Require(!simulation.RewardWeaponEquipped(),"Inventory command must change actual equipment.");
  command={}; ui::ApplyItem(command,simulation,ui::Item::Watchguard); simulation.Update(.01f,command);
  Require(simulation.RewardWeaponEquipped(),"Reward weapon must re-equip without duplicating ownership.");
  rpg::Simulation restored; Require(restored.Load(simulation.Save()) && restored.RewardWeaponEquipped(),"Equipment changes must survive save/load.");
  rpg::Simulation injured; Require(injured.Load(Fixture("combat-preview.save")),"Potion fixture must load.");
  for (int i=0;i<12;++i) injured.Update(.05f,{});
  const float injuredHealth=injured.Health(); const int potions=injured.Potions(); command={};
  ui::ApplyItem(command,injured,ui::Item::Potion); Require(command.usePotion,"An available potion must be usable while injured.");
  injured.Update(.01f,command);
  Require(injured.Health()>injuredHealth && injured.Potions()==potions-1,"Bag potion action must heal and consume exactly one item.");

  // A hidden real renderer validates hit regions at multiple scales against the
  // same layout that draws the screen. No desktop input is synthesized.
  Win32Window window; Require(window.Create(GetModuleHandle(nullptr),1280,720,L"screen checks",false),"Hidden test window failed.");
  Renderer renderer; Require(renderer.Initialize(window.GetHandle(),1280,720),"screen renderer initialization failed.");
  for (const auto size : {std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(renderer.Resize(size.first,size.second),"screen resize failed.");
    const ui::Layout layout(float(size.first),float(size.second));
    const auto click=[&](float x,float y) { return screen.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),true,false); };
    screen.panel=ui::Panel::Inventory; screen.selected=ui::Item::Starter;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Inventory render failed.");
    Require(click(238,240).action==ui::Action::SelectItem,"Item slot did not receive a scaled click.");
    const auto equip=click(810,434);
    Require(equip.action==ui::Action::UseItem && equip.value==int(ui::Item::Starter),"Equip button did not target the inspected item.");
    screen.panel=ui::Panel::Menu;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Menu render failed.");
    Require(click(640,375).action==ui::Action::Fullscreen,"Fullscreen button must issue the display command.");
    Require(click(640,220).action==ui::Action::Close,"Return button must close the menu.");
    Require(click(1000,680).action==ui::Action::None,"Menu must prevent clicks reaching the underlying HUD.");
    screen.panel=ui::Panel::Map;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Atlas render failed.");
    Require(click(477.5f,366).action==ui::Action::PlaceWaypoint,"Atlas must accept a map click.");
    Require(std::abs(screen.pendingWaypoint.x)<.001f && std::abs(screen.pendingWaypoint.z)<.001f,"Waypoint coordinates must match the world map.");
    screen.hasWaypoint=true; screen.waypoint={0,0}; click(160,366);
    Require(!simulation.CanStand(screen.pendingWaypoint) && screen.waypoint.x==0 && screen.waypoint.z==0,"Invalid map candidate must preserve the existing waypoint.");
    screen.Pointer(layout.x+477.5f*layout.scale,layout.y+366*layout.scale,float(size.first),float(size.second),false,true);
    Require(!screen.hasWaypoint,"Right-clicking the atlas must clear a waypoint.");
  }
  Require(renderer.ValidateDebugMessages(),"DirectX reported an screen validation issue.");
  std::cout<<"PASS: live menus, item actions, persistence, scaled pointer routing and UI rendering.\n";
}
