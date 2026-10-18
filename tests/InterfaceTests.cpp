#include "game/Game.h"
#include "game/Interface.h"
#include "game/TerrainSurface.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include "platform/GameCursor.h"
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
  for (UINT dpi : {96u,144u,192u,384u}) {
    const auto cursor=CreateGameCursor(dpi);
    Require(cursor!=nullptr,"Native game cursor creation failed");
    ICONINFO info{}; Require(GetIconInfo(cursor,&info)!=FALSE,"Cursor inspection failed");
    BITMAP bitmap{}; Require(GetObject(info.hbmColor,sizeof(bitmap),&bitmap)!=0,"Cursor bitmap missing");
    Require(!info.fIcon && info.xHotspot==MulDiv(3,int(dpi),96) && info.yHotspot==MulDiv(2,int(dpi),96),
      "DPI-scaled cursor must keep its click hotspot at the arrow tip");
    Require(bitmap.bmWidth==MulDiv(32,int(dpi),96) && bitmap.bmHeight==bitmap.bmWidth && bitmap.bmBitsPixel==32,
      "Cursor must retain a DPI-scaled color/alpha bitmap");
    BITMAPINFO format{}; format.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    format.bmiHeader.biWidth=bitmap.bmWidth; format.bmiHeader.biHeight=-bitmap.bmHeight;
    format.bmiHeader.biPlanes=1; format.bmiHeader.biBitCount=32;
    std::vector<std::uint32_t> pixels(size_t(bitmap.bmWidth)*bitmap.bmHeight);
    const auto dc=GetDC(nullptr);
    const int rows=GetDIBits(dc,info.hbmColor,0,bitmap.bmHeight,pixels.data(),&format,DIB_RGB_COLORS);
    ReleaseDC(nullptr,dc); Require(rows==bitmap.bmHeight,"Cursor pixels unavailable");
    bool clear=false,solid=false,edge=false;
    for (auto pixel : pixels) { const auto alpha=pixel>>24; clear|=alpha==0; solid|=alpha==255; edge|=alpha>0 && alpha<255; }
    Require(clear && solid && edge,"Cursor needs transparent surroundings, solid fill and antialiased edges");
    if (dpi==384) {
      // Enlarged diagnostic on a dark neutral background; actual cursor retains alpha.
      for (auto& pixel : pixels) {
        const auto background=40*(255-(pixel>>24))/255;
        pixel=0xff000000u|((((pixel>>16)&255)+background)<<16)|((((pixel>>8)&255)+background)<<8)|((pixel&255)+background);
      }
      BITMAPFILEHEADER file{}; file.bfType=0x4d42; file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);
      file.bfSize=file.bfOffBits+DWORD(pixels.size()*4);
      std::ofstream preview("cursor-preview.bmp",std::ios::binary);
      preview.write(reinterpret_cast<const char*>(&file),sizeof(file));
      preview.write(reinterpret_cast<const char*>(&format.bmiHeader),sizeof(BITMAPINFOHEADER));
      preview.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*4));
      Require(bool(preview),"Cursor diagnostic write failed");
    }
    DeleteObject(info.hbmColor); DeleteObject(info.hbmMask); DestroyCursor(cursor);
  }
  {
    const rpg::WorldMap map;
    const auto pixels=BuildTerrainSurface(map);
    Require(pixels.size()==terrainSurfaceSize*terrainSurfaceSize*4,"Invalid ground weight texture dimensions.");
    for (std::size_t i=0;i<pixels.size();i+=4)
      Require(unsigned(pixels[i])+pixels[i+1]<=255 && pixels[i+2]==0 && pixels[i+3]==255,
        "Ground weights must survive quantization without negative grass weights or transparency.");
    // A vertically inverted upload would put the paved village at the enemy camp.
    const auto sample=[&](float x,float z,int channel) {
      const auto col=unsigned((x+21)/42*terrainSurfaceSize),row=unsigned((23-z)/46*terrainSurfaceSize);
      return pixels[(row*terrainSurfaceSize+col)*4+channel];
    };
    Require(sample(0,-10,1)==255 && sample(0,10,1)==0 && sample(0,10,0)>245 && sample(12,-2,0)==255 &&
      sample(-15,16,0)==0 && sample(-15,16,1)==0,"Ground texture orientation must match world and atlas coordinates.");
  }
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
  const auto upgrade=ui::SummarizeItem(simulation,ui::Item::Watchguard);
  Require(upgrade.difference==7 && upgrade.comparison.find("+10")!=std::string::npos,
    "Hover comparison must use actual worn weapon stats");
  Require(ui::SummarizeItem(simulation,ui::Item::Starter).comparison=="Currently equipped",
    "Worn item tooltip must identify current equipment");
  Require(ui::SummarizeItem(simulation,ui::Item::Helm).difference==1,
    "Shop equipment must compare against an empty gear slot");
  Require(ui::SummarizeItem(simulation,ui::Item::Potion).comparison=="Shared cooldown: 3 seconds",
    "Potion tooltip must describe its shared cooldown");
  ui::ApplyItem(command,simulation,ui::Item::Watchguard); Require(command.inventory.action==rpg::InventoryAction::None,"Unowned gear must not equip.");
  ui::ApplyItem(command,simulation,ui::Item::Potion); Require(!command.usePotion,"Full health must disable potion use.");
  Require(simulation.Load(Fixture("veteran-preview.save")),"Veteran fixture must load.");
  Require(ui::SummarizeItem(simulation,ui::Item::Starter).difference==-7,
    "Downgrade tooltip must compare against the currently worn reward weapon");
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
  Renderer renderer;
  const bool initialized=renderer.Initialize(window.GetHandle(),1280,720);
  if (!initialized) std::wcerr<<renderer.Error()<<L'\n';
  Require(initialized,"screen renderer initialization failed.");
  {
    // Older saves can contain a pending swing while the player is already dead.
    // Render immediately after load, before the first Update.
    auto snapshot=Fixture("combat-preview.save");
    const auto health=snapshot.find("5.2 100"); Require(health!=std::string::npos,"Expected combat health field.");
    snapshot.replace(health,7,"5.2 0");
    Game game; Require(game.RestoreSnapshot(snapshot),"Defeated windup fixture must load.");
    Require(game.EnemyPose(0).attack==0,"A defeated save must load with a neutral enemy attack pose.");
    renderer.BeginFrame(); game.Render(renderer,false);
    Require(renderer.EndFrame(false),"Defeated windup render failed.");
    const auto triangles=renderer.Triangles();
    const auto windup=snapshot.find("1 0.5 3.14159"); Require(windup!=std::string::npos,"Expected pending enemy swing.");
    snapshot.replace(windup,std::string("1 0.5 3.14159").size(),"1 0 3.14159");
    Require(game.RestoreSnapshot(snapshot),"Defeated neutral fixture must load.");
    renderer.BeginFrame(); game.Render(renderer,false);
    Require(renderer.EndFrame(false),"Defeated neutral render failed.");
    Require(renderer.Triangles()==triangles,"A defeated save must not render an obsolete attack warning cone.");
  }
  {
    Game game; game.Initialize(); rpg::Command jump; jump.jump=true; game.ExecuteCommand(jump,.05f);
    game.OpenPanel(ui::Panel::Character); game.ShowPanel(ui::Panel::Inventory);
    const auto pose=game.PlayerAnimation(); const auto snapshot=game.State().Save();
    for (int i=0;i<3;++i) {
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Animation portrait render failed.");
    }
    Require(game.PlayerAnimation().air==pose.air && game.PlayerAnimation().rise==pose.rise &&
      game.PlayerAnimation().breath==pose.breath && game.State().Save()==snapshot,
      "Rendering the world and equipment preview must not advance animation or simulation.");
  }
  {
    Game game; Require(game.RestoreSnapshot(Fixture("veteran-preview.save")),"Fullscreen drag fixture must load.");
    game.OpenPanel(ui::Panel::Character); game.ShowPanel(ui::Panel::Inventory);
    Input input; Input::Frame frame; frame.focused=true; frame.client={1068,274}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.05,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Fullscreen drag render failed.");
    };
    step(); frame.down[VK_LBUTTON]=true; step();
    frame.client={277,210}; frame.screen=frame.client; step();
    const auto before=game.State().Save();
    frame.down[VK_LBUTTON]=false; frame.down[VK_F11]=true; step();
    Require(game.State().Save()==before,
      "F11 on the drop frame must cancel the drag before equipment changes, not afterward in Engine.");
    frame.down[VK_F11]=false; frame.client={1068,274}; frame.screen=frame.client; step();
    frame.down[VK_LBUTTON]=true; step(); frame.client={277,210}; frame.screen=frame.client; step();
    frame.down[VK_LBUTTON]=false; step();
    Require(game.State().Items().Worn(rpg::GearSlot::Weapon)==rpg::Item::Starter,
      "A fresh drag after display cancellation must still equip normally.");
  }
  for (auto panel : {ui::Panel::Menu,ui::Panel::Map,ui::Panel::Journal}) {
    auto lowHealth=Fixture("combat-preview.save");
    const auto health=lowHealth.find("5.2 100"); Require(health!=std::string::npos,"Expected combat health field.");
    lowHealth.replace(health,7,"5.2 1");
    Game game; Require(game.RestoreSnapshot(lowHealth),"Low-health combat fixture must load.");
    game.OpenPanel(panel); Input input; Input::Frame frame; frame.focused=true;
    input.Update(frame);
    for (int i=0;i<20;++i) game.Update(.05,input,0);
    Require(game.State().Health()==0,"Combat must be able to defeat the player behind a central panel.");
    if (panel==ui::Panel::Menu) {
      renderer.BeginFrame(); game.Render(renderer);
      Require(renderer.Capture(L"defeat-recovery.png") && renderer.EndFrame(false),"Defeat recovery capture failed.");
    }
    // Even explicitly reopening a panel while defeated must not swallow recovery.
    game.OpenPanel(panel); frame.down[VK_RETURN]=true; input.Update(frame); game.Update(.05,input,0);
    Require(game.State().Health()==game.State().MaxHealth() && game.State().Player().z==rpg::WorldMap::spawn.z,
      "Enter must recover a defeated player even with the menu, atlas or journal open.");
    Require(game.State().QuestState()==rpg::Quest::Active && game.State().Gold()==0,
      "Panel recovery must retain campaign progress, not restart the expedition.");
  }
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
    frame.focused=true; frame.client={1240,685}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.05,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Menu movement capture render failed");
    };
    step(); frame.down['W']=true; for(int i=0;i<6;++i) step();
    const auto before=game.State().Player(); frame.down[VK_LBUTTON]=true; step();
    Require(game.State().Player().x==before.x && game.State().Player().z==before.z,
      "Clicking the HUD menu while running must stop movement on the same frame");
    frame.down[VK_LBUTTON]=false; step(); frame.client={630,216}; frame.screen=frame.client;
    frame.down[VK_LBUTTON]=true; step();
    Require(game.State().Player().z>before.z,"Clicking Return to game must restore movement on the same frame");
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
    Input input; Input::Frame frame; frame.focused=true; frame.client={900,293}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.05,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Merchant input render failed.");
    };
    step(); frame.down['E']=true; step(); frame.down['E']=false; step();
    const auto before=game.State().Player(); frame.down['W']=true; step(); frame.down['W']=false;
    Require(game.State().Player().x==before.x && game.State().Player().z==before.z,"Trade must capture movement without pausing simulation.");
    frame.down[VK_LBUTTON]=true; step(); frame.down[VK_LBUTTON]=false; step();
    Require(game.State().Items().Count(rpg::Item::Helm)==1 && game.State().Gold()==70,"Merchant Buy must reach the authoritative purchase command.");
    const auto clickAt=[&](LONG x,LONG y) {
      frame.client={x,y}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step(); frame.down[VK_LBUTTON]=false; step();
    };
    const int helm=game.State().Items().Find(rpg::Item::Helm);
    clickAt(1028+helm%6*40,274+helm/6*42); clickAt(1070,587);
    Require(game.State().BuybackCount(rpg::Item::Helm)==1 && game.State().Gold()==80,"Bag Sell must reach the authoritative trade command.");
    clickAt(511+int(rpg::Item::Helm)*51,469);
    Require(game.State().BuybackCount(rpg::Item::Helm)==0 && game.State().Gold()==70 && game.State().Items().Count(rpg::Item::Helm)==1,
      "Buyback click must reclaim into the bag, charging exactly the sale price.");
    clickAt(1028,274); clickAt(1195,587);
    Require(game.State().Items().Bag()[0].count==3 && game.State().Potions()==5,"Game input must split the selected odd stack.");
    int second=-1;
    for (int i=1;i<rpg::Inventory::capacity;++i) if (game.State().Items().Bag()[i].item==rpg::Item::Potion) second=i;
    Require(second>=0,"Split must create a second selectable stack.");
    clickAt(1028+second%6*40,274+second/6*42); clickAt(1070,587);
    Require(game.State().Items().Bag()[0].count==3 && game.State().Items().Bag()[second].count==1 && game.State().Gold()==72,
      "Sale must consume from the selected split stack, not the first matching cell.");
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
    Require(click(640,343).action==ui::Action::Fullscreen,"Fullscreen button must issue the display command.");
    Require(click(727,439).action==ui::Action::MusicVolume && click(727,439).value==-10,
      "Music decrease must remain clickable at every UI scale");
    Require(click(767,473).action==ui::Action::EffectsVolume && click(767,473).value==10,
      "Effects increase must remain clickable at every UI scale");
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
    Require(click(640,522).action==ui::Action::None,"Closing the menu must not activate its invisible Exit button before redraw.");
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
    auto purchase=click(900,293); Require(purchase.action==ui::Action::Purchase && purchase.value==1,"Scaled helm purchase must select the correct offer.");
    command={}; command.purchase=purchase.value; simulation.Update(.01f,command); render();
    Require(click(900,293).action==ui::Action::None,"Sold-out Buy must be disabled.");
    const int helmCell=simulation.Items().Find(rpg::Item::Helm);
    apply(drag(1028+float(helmCell%6)*40,274+float(helmCell/6)*42,277,410));
    Require(simulation.Items().Worn(rpg::GearSlot::Head)==rpg::Item::Helm && screen.panel==ui::Panel::Merchant,
      "Bought helm must drag into Head while trade remains open.");
    render(); purchase=click(900,363); command={}; command.purchase=purchase.value; simulation.Update(.01f,command); render();
    const int shieldCell=simulation.Items().Find(rpg::Item::Shield); Require(shieldCell>=0,"Shield purchase must reach the bag.");
    apply(drag(1028+float(shieldCell%6)*40,274+float(shieldCell/6)*42,277,310));
    Require(simulation.Items().Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield && simulation.DamageReduction()==5,"Off-hand drag must update actual defense.");
    render(); Require(click(550,681).action==ui::Action::None,"Trade must block the underlying action bar.");
    const auto rotate=click(147,593); Require(rotate.action==ui::Action::RotatePreview && rotate.value==1,"Portrait turn control must be clickable at every scale.");
    screen.selected=ui::Item::Shield; screen.selectedBag=-1; render();
    Require(click(1070,587).action==ui::Action::None,"Worn gear cannot be sold directly from the detail panel.");
    screen.selected=ui::Item::Potion; screen.selectedBag=0; render();
    const auto split=click(1195,587);
    Require(split.action==ui::Action::InventoryChange && split.inventory.action==rpg::InventoryAction::Split && split.inventory.amount==2,
      "Split half must target the selected stack at every scale.");
    apply(split); render();
    int second=-1;
    for (int i=1;i<rpg::Inventory::capacity;++i) if (simulation.Items().Bag()[i].item==rpg::Item::Potion) second=i;
    Require(second>=0,"Scaled split creates a second stack.");
    const auto inspect=click(1028+float(second%6)*40,274+float(second/6)*42);
    Require(inspect.action==ui::Action::SelectItem && inspect.inventory.source==second,"Selection retains the exact bag cell.");
    screen.selected=ui::Item(inspect.value); screen.selectedBag=inspect.inventory.source; render();
    const auto sell=click(1070,587);
    Require(sell.action==ui::Action::Sell && sell.inventory.source==second,"Scaled sale targets the selected split cell.");
    command={}; command.trade={rpg::TradeAction::Sell,rpg::Item(sell.value),sell.inventory.source}; simulation.Update(.01f,command); render();
    Require(simulation.Items().Bag()[0].count==3 && simulation.Items().Bag()[second].count==1,"Selling one leaves the other potion stack intact.");
    const auto reclaim=click(613,469);
    Require(reclaim.action==ui::Action::Buyback && reclaim.value==int(rpg::Item::Potion),"Scaled buyback shelf must route to the correct item.");
    command={}; command.trade={rpg::TradeAction::Buyback,rpg::Item(reclaim.value)}; simulation.Update(.01f,command); render();
    Require(simulation.Potions()==5 && simulation.BuybackCount(rpg::Item::Potion)==0 && click(613,469).action==ui::Action::None,
      "Reclaimed items must disappear from the buyback shelf without leaving a clickable stale entry.");
    auto injuredSnapshot=simulation.Save(); const auto health=injuredSnapshot.find("140"); injuredSnapshot.replace(health,3,"90");
    Require(simulation.Load(injuredSnapshot),"Prepare selected-stack right-click fixture."); render();
    const auto drink=pointer(1028+float(second%6)*40,274+float(second/6)*42,false,false,false,true);
    Require(drink.action==ui::Action::UseItem && drink.inventory.source==second,"Right-click must preserve the clicked potion cell.");
    command={}; ui::ApplyItem(command,simulation,ui::Item(drink.value),drink.inventory.source); simulation.Update(.01f,command);
    Require(simulation.Health()==130 && simulation.Items().Bag()[second].item==rpg::Item::None && simulation.Items().Bag()[0].count==4,
      "Right-click must drink from the second stack after a buyback merged into the first.");
  }
  for (const auto size : {std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(renderer.Resize(size.first,size.second),"Tooltip resize failed");
    ui::Interface tips; rpg::Simulation state; tips.Show(ui::Panel::Inventory);
    const ui::Layout layout(float(size.first),float(size.second));
    for (bool shop : {false,true}) {
      if (shop) tips.Show(ui::Panel::Merchant);
      renderer.BeginFrame(); tips.Render(renderer,state,0,false); Require(renderer.EndFrame(false),"Tooltip layout failed");
      const float x=shop?900.f:1028.f,y=shop?278.f:274.f;
      tips.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),false,false);
      const auto before=state.Save();
      renderer.BeginFrame(); tips.Render(renderer,state,0,false);
      const auto path=std::wstring(shop?L"tooltip-shop-":L"tooltip-bag-")+std::to_wstring(size.first)+L".png";
      Require(renderer.Capture(path) && renderer.EndFrame(false),"Tooltip capture failed");
      Require(state.Save()==before,"Hovering must not change gameplay or purchase items");
    }
  }
  Require(renderer.ValidateDebugMessages(),"DirectX reported an screen validation issue.");
  for (const auto size : {std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(renderer.Resize(size.first,size.second),"Combat HUD resize failed");
    rpg::Simulation state; Require(state.Load(Fixture("combat-preview.save")),"Combat HUD fixture failed");
    rpg::Command attack; attack.nextTarget=true; attack.ability=0; state.Update(.01f,attack);
    Require(state.TargetActor() && state.TargetActor()->windup>0 && state.Cooldown(0)>0,
      "HUD diagnostic must contain an attack warning and a cooling ability");
    ui::Interface hud; const ui::Layout layout(float(size.first),float(size.second));
    renderer.BeginFrame(); hud.Render(renderer,state,0,false); Require(renderer.EndFrame(false),"Combat HUD layout failed");
    const auto before=state.Save();
    const auto event=hud.Pointer(layout.x+466*layout.scale,layout.y+681*layout.scale,
      float(size.first),float(size.second),true,false);
    Require(event.action==ui::Action::None,"Cooling ability must remain disabled despite new progress indicators");
    renderer.BeginFrame(); hud.Render(renderer,state,0,false);
    Require(renderer.Capture(L"combat-hud-"+std::to_wstring(size.first)+L".png") && renderer.EndFrame(false),
      "Combat HUD diagnostic capture failed");
    Require(state.Save()==before,"HUD rendering and disabled clicks must not change combat state");
  }
  Require(renderer.ValidateDebugMessages(),"Combat HUD emitted a DirectX warning/error");
  std::cout<<"PASS: live menus, item actions, persistence, scaled pointer routing and UI rendering.\n";
}
