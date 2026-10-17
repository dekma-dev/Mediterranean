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
  screen.SetOnly(ui::Panel::Character); screen.Toggle(ui::Panel::Inventory);
  Require(screen.IsOpen(ui::Panel::Character) && screen.IsOpen(ui::Panel::Inventory) && !screen.CapturesMovement(),"Character and bag must coexist without blocking keyboard movement.");
  screen.Escape(); Require(screen.CapturesMovement(),"Menu over side windows must capture movement.");
  screen.Escape(); Require(screen.IsOpen(ui::Panel::Character) && screen.IsOpen(ui::Panel::Inventory),"Esc must preserve both side windows.");
  screen.Close(ui::Panel::Inventory); Require(screen.IsOpen(ui::Panel::Character) && !screen.IsOpen(ui::Panel::Inventory),"Closing the bag must preserve the character window.");
  screen.Escape(); screen.Toggle(ui::Panel::Character);
  Require(screen.panel==ui::Panel::None && screen.characterOpen,"Menu Character must reveal an already open character window.");
  for (auto panel : {ui::Panel::Menu,ui::Panel::Inventory,ui::Panel::Character,ui::Panel::Map,ui::Panel::Journal}) {
    Game game; game.Initialize(); Require(game.RestoreSnapshot(Fixture("combat-preview.save")),"Combat fixture must load.");
    game.OpenPanel(panel); const float before=game.State().Health(); const Input idle;
    for (int frame=0;frame<20;++frame) game.Update(.05,idle,0);
    Require(game.State().Health()<before,"Enemy windup/damage must continue while a menu or panel is open.");
  }
  rpg::Simulation simulation; rpg::Command command;
  ui::ApplyItem(command,simulation,ui::Item::Watchguard); Require(command.inventory.action==rpg::InventoryAction::None,"Unowned gear must not equip.");
  ui::ApplyItem(command,simulation,ui::Item::Potion); Require(!command.usePotion,"Full health must disable potion use.");
  Require(simulation.Load(Fixture("veteran-preview.save")),"Veteran fixture must load.");
  ui::ApplyItem(command,simulation,ui::Item::Starter); Require(command.inventory.action==rpg::InventoryAction::Equip,"Unequipped starter weapon must be selectable.");
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
  {
    Input input; Input::Frame frame; frame.focused=true;
    input.Update(frame); frame.down['E']=true; frame.down[VK_LBUTTON]=true; input.Update(frame);
    Require(input.Pressed('E') && input.Pressed(VK_LBUTTON),"Fresh key/button presses must be delivered.");
    frame.focused=false; input.Update(frame);
    Require(!input.Held('E') && !input.Released(VK_LBUTTON),"Focus loss must clear held input without dropping an item.");
    frame.focused=true; frame.down[VK_RBUTTON]=true; frame.screen={900,500}; input.Update(frame);
    Require(!input.Pressed('E') && !input.Pressed(VK_RBUTTON) && input.MouseX()==0,
      "Focus restoration must not repeat actions or jump the camera.");
    frame.down.fill(false); input.Update(frame); frame.down['E']=true; input.Update(frame);
    Require(input.Pressed('E'),"A fresh press after focus restoration must work.");
  }
  {
    Game game; game.Initialize(); game.OpenPanel(ui::Panel::Inventory);
    Input input; Input::Frame frame; frame.focused=true; frame.client={1068,274}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.05,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Input integration render failed.");
    };
    step(); frame.down[VK_RBUTTON]=true; step();
    frame.client={680,380}; frame.screen.x+=100; step();
    frame.down[VK_RBUTTON]=false; frame.down['W']=true; step();
    Require(std::abs(game.State().Player().x)<.0001f,
      "A right-button gesture begun in the bag must not turn the camera after leaving the window.");
    frame.down['W']=false; step(); frame.down[VK_RBUTTON]=true; step();
    frame.screen.x+=100; step(); frame.down[VK_RBUTTON]=false; frame.down['W']=true; step();
    Require(game.State().Player().x>.01f,"A fresh right-button gesture begun over the world must still orbit.");
  }
  {
    Game game; game.Initialize(); game.OpenPanel(ui::Panel::Character);
    Input input; Input::Frame frame; frame.focused=true; frame.client={40,210}; frame.screen=frame.client;
    input.Update(frame); game.Update(.01,input,0);
    renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Pre-load layout render failed.");
    const auto initial=game.State().Save(); Require(game.RestoreSnapshot(initial),"Snapshot reload must succeed.");
    frame.down[VK_RBUTTON]=true; input.Update(frame); game.Update(.01,input,0);
    Require(game.State().Items().Worn(rpg::GearSlot::Chest)==rpg::Item::Tunic,
      "Loading must invalidate old gear hit regions before input is routed.");
  }
  {
    Game game; game.Initialize(); Input input; Input::Frame frame;
    frame.focused=true; frame.client={1170,94}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.05,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"HUD gesture render failed.");
    };
    step(); frame.down[VK_RBUTTON]=true; step(); frame.client={680,380}; frame.screen.x+=100; step();
    frame.down[VK_RBUTTON]=false; frame.down['W']=true; step();
    Require(std::abs(game.State().Player().x)<.0001f,"A minimap gesture must not leak into camera orbit.");
    frame.down['W']=false; frame.down[VK_RBUTTON]=true; step();
    game.CancelUiGesture(); frame.focused=false; input.Update(frame);
    frame.focused=true; frame.screen.x+=300; step(); frame.screen.x+=100; step();
    frame.down[VK_RBUTTON]=false; frame.down['W']=true; step();
    Require(std::abs(game.State().Player().x)<.0001f,"Returning with RMB still held must not resume a cancelled orbit.");
  }
  {
    Game game; Require(game.RestoreSnapshot(Fixture("quartermaster-preview.save")),"Quartermaster fixture must load.");
    Input input; Input::Frame frame; frame.focused=true; frame.client={900,385}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.05,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Merchant input render failed.");
    };
    step(); frame.down['E']=true; step(); frame.down['E']=false; step();
    const auto before=game.State().Player(); frame.down['W']=true; step(); frame.down['W']=false;
    Require(game.State().Player().x==before.x && game.State().Player().z==before.z,"Trade must capture movement without pausing simulation.");
    frame.down[VK_LBUTTON]=true; step(); frame.down[VK_LBUTTON]=false; step();
    Require(game.State().Items().Count(rpg::Item::Helm)==1 && game.State().Gold()==70,"Merchant Buy must reach the authoritative purchase command.");
    frame.down['E']=true; step(); frame.down['E']=false; frame.down['W']=true; step();
    Require(game.State().Player().z>before.z,"E must close trade without immediately reopening it.");
  }
  for (const auto size : {std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(simulation.Load(Fixture("veteran-preview.save")),"Reset pointer fixture.");
    Require(renderer.Resize(size.first,size.second),"screen resize failed.");
    const ui::Layout layout(float(size.first),float(size.second));
    const auto click=[&](float x,float y) { return screen.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),true,false); };
    screen.SetOnly(ui::Panel::Inventory); screen.selected=ui::Item::Starter;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Inventory render failed.");
    Require(click(1028,274).action==ui::Action::SelectItem,"Item slot did not receive a scaled click.");
    const auto equip=click(1110,620);
    Require(equip.action==ui::Action::UseItem && equip.value==int(ui::Item::Starter),"Equip button did not target the inspected item.");
    screen.Show(ui::Panel::Character);
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Combined equipment/bag render failed.");
    Require(click(40,210).action==ui::Action::SelectItem && screen.inventoryOpen,"Worn gear inspection must leave the bag open.");
    const auto closeBag=click(1250,213);
    Require(closeBag.action==ui::Action::Close && closeBag.value==int(ui::Panel::Inventory),"Bag close must identify only the bag.");
    Require(click(1040,580).action==ui::Action::None && screen.PointerOverWindow(),"Blank bag surface must capture its pointer.");
    Require(click(680,380).action==ui::Action::None && !screen.PointerOverWindow(),"World gap must remain available for camera orbit.");
    Require(click(550,681).action==ui::Action::Ability,"Available Dash must remain usable with both side windows open.");
    screen.panel=ui::Panel::Menu;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Menu render failed.");
    Require(click(640,375).action==ui::Action::Fullscreen,"Fullscreen button must issue the display command.");
    Require(click(640,220).action==ui::Action::Close,"Return button must close the menu.");
    Require(click(1000,680).action==ui::Action::None,"Menu must prevent clicks reaching the underlying HUD.");
    screen.SetOnly(ui::Panel::Map);
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Atlas render failed.");
    Require(click(477.5f,366).action==ui::Action::PlaceWaypoint,"Atlas must accept a map click.");
    Require(std::abs(screen.pendingWaypoint.x)<.001f && std::abs(screen.pendingWaypoint.z)<.001f,"Waypoint coordinates must match the world map.");
    screen.hasWaypoint=true; screen.waypoint={0,0}; click(160,366);
    Require(!simulation.CanStand(screen.pendingWaypoint) && screen.waypoint.x==0 && screen.waypoint.z==0,"Invalid map candidate must preserve the existing waypoint.");
    screen.Pointer(layout.x+477.5f*layout.scale,layout.y+366*layout.scale,float(size.first),float(size.second),false,true);
    Require(!screen.hasWaypoint,"Right-clicking the atlas must clear a waypoint.");

    screen.SetOnly(ui::Panel::Character); screen.Show(ui::Panel::Inventory);
    const auto render=[&]() { renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Drag layout render failed."); };
    const auto pointer=[&](float x,float y,bool pressed,bool held,bool released,bool right=false) {
      return screen.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),pressed,right,held,released);
    };
    bool capturedDrag=false;
    const auto drag=[&](float x,float y,float tx,float ty) {
      pointer(x,y,true,true,false); pointer(tx,ty,false,true,false); Require(screen.Dragging(),"Movement must start an item drag.");
      if (size.first==1280 && !capturedDrag) {
        renderer.BeginFrame(); screen.Render(renderer,simulation,0,false);
        Require(renderer.Capture(L"inventory-drag.png") && renderer.EndFrame(false),"Drag preview capture failed."); capturedDrag=true;
      }
      return pointer(tx,ty,false,false,true);
    };
    const auto apply=[&](ui::Event event) { rpg::Command c; c.inventory=event.inventory; simulation.Update(.01f,c); };
    render(); screen.Escape();
    Require(pointer(1068,274,false,false,false,true).action==ui::Action::None,
      "Opening the menu must reject right-clicks against the previous bag layout before redraw.");
    render(); screen.Escape();
    Require(click(640,500).action==ui::Action::None,"Closing the menu must not activate its invisible Exit button before redraw.");
    render(); screen.Close(ui::Panel::Inventory);
    Require(pointer(1068,274,false,false,false,true).action==ui::Action::None,"A closed bag must not accept stale item actions.");
    screen.Show(ui::Panel::Inventory);
    render(); pointer(1068,274,true,true,false); pointer(1200,400,false,true,false);
    Require(screen.Pointer(1200,400,float(size.first+100),float(size.second),false,false,false,true).action==ui::Action::None && !screen.Dragging(),
      "A resized layout must cancel its stale drop before the next render.");
    render();
    apply(drag(1068,274,1228,400));
    Require(simulation.Items().Bag()[23].item==rpg::Item::Starter && simulation.Items().Count(rpg::Item::Starter)==1,"Dragging to an empty cell must move, not duplicate, a sword.");
    render(); apply(drag(1228,400,277,210));
    Require(simulation.Items().Worn(rpg::GearSlot::Weapon)==rpg::Item::Starter && simulation.Items().Bag()[23].item==rpg::Item::Watchguard,"Dropping onto main hand must swap the previous sword into the source cell.");
    render(); apply(drag(40,210,1188,400));
    Require(!simulation.WardArmorEquipped() && simulation.DamageReduction()==0 && simulation.Level()==3,"Dragging armor off must update protection without removing quest XP.");
    render();
    const auto invalid=drag(1028,274,277,210); const auto before=simulation.Save(); apply(invalid);
    Require(simulation.Save()==before,"Potion dropped on a weapon slot must preserve all inventory state.");
    render(); const auto rightUse=pointer(40,410,false,false,false,true); rpg::Command removeBoots;
    ui::ApplyItem(removeBoots,simulation,ui::Item(rightUse.value)); simulation.Update(.01f,removeBoots);
    Require(rightUse.action==ui::Action::UseItem && simulation.Items().Worn(rpg::GearSlot::Feet)==rpg::Item::None,"Right-clicking worn boots must remove them.");
    render(); pointer(1028,274,true,true,false); pointer(1100,350,false,true,false); screen.Escape();
    Require(!screen.Dragging() && pointer(1200,400,false,false,true).action==ui::Action::None,"Escape must cancel dragging without moving an item.");
    screen.Escape(); render();
    const auto sorted=click(1215,236); Require(sorted.action==ui::Action::InventoryChange,"Sort must issue a real inventory command."); apply(sorted);
    rpg::Simulation copy; Require(copy.Load(simulation.Save()) && copy.Save()==simulation.Save(),"Dragged equipment and bag cells must round-trip in the current save format.");
    Require(simulation.Load(Fixture("quartermaster-preview.save")),"Reset scaled merchant scenario.");
    screen.SetOnly(ui::Panel::Merchant); screen.Show(ui::Panel::Character); render();
    Require(screen.panel==ui::Panel::Merchant && screen.characterOpen && screen.inventoryOpen,"Trade must coexist with both equipment windows.");
    auto purchase=click(900,385); Require(purchase.action==ui::Action::Purchase && purchase.value==1,"Scaled helm purchase must select the correct offer.");
    command={}; command.purchase=purchase.value; simulation.Update(.01f,command); render();
    Require(click(900,385).action==ui::Action::None,"Sold-out Buy must be disabled.");
    const int helmCell=simulation.Items().Find(rpg::Item::Helm);
    apply(drag(1028+float(helmCell%6)*40,274+float(helmCell/6)*42,277,410));
    Require(simulation.Items().Worn(rpg::GearSlot::Head)==rpg::Item::Helm && screen.panel==ui::Panel::Merchant,
      "Bought helm must drag into Head while trade remains open.");
    render(); purchase=click(900,479); command={}; command.purchase=purchase.value; simulation.Update(.01f,command); render();
    const int shieldCell=simulation.Items().Find(rpg::Item::Shield); Require(shieldCell>=0,"Shield purchase must reach the bag.");
    apply(drag(1028+float(shieldCell%6)*40,274+float(shieldCell/6)*42,277,310));
    Require(simulation.Items().Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield && simulation.DamageReduction()==5,"Off-hand drag must update actual defense.");
    render(); Require(click(550,681).action==ui::Action::None,"Trade must block the underlying action bar.");
    const auto rotate=click(147,593); Require(rotate.action==ui::Action::RotatePreview && rotate.value==1,"Portrait turn control must be clickable at every scale.");
  }
  Require(renderer.ValidateDebugMessages(),"DirectX reported an screen validation issue.");
  std::cout<<"PASS: live menus, item actions, persistence, scaled pointer routing and UI rendering.\n";
}
