#include "game/Game.h"
#include "tests/DashFixture.h"
#include "tests/QuestFixture.h"
#include "game/Interface.h"
#include "game/CharacterView.h"
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
void RelativeMouseRegressions() {
  MouseMotion motion; RAWMOUSE packet{};
  packet.lLastX=3; packet.lLastY=-2;
  for (int i=0;i<1000;++i) motion.Add(packet,nullptr,1920,1080);
  const auto summed=motion.Consume();
  Require(summed.x==3000 && summed.y==-2000,"All raw packets must contribute while the cursor is fixed");
  const auto empty=motion.Consume();
  Require(empty.x==0 && empty.y==0,"Raw movement must be consumed once per frame");
  packet.usFlags=MOUSE_MOVE_ABSOLUTE; packet.lLastX=32768; packet.lLastY=32768;
  motion.Add(packet,nullptr,1920,1080);
  Require(motion.Consume().x==0,"First absolute packet must establish a baseline without jumping");
  packet.lLastX+=1000; packet.lLastY-=1000; motion.Add(packet,nullptr,1920,1080);
  const auto absolute=motion.Consume();
  Require(std::abs(absolute.x-1000.f*1920/65535)<.001f && std::abs(absolute.y+1000.f*1080/65535)<.001f,
    "Absolute motion must convert to signed desktop distances");
  packet.usFlags|=MOUSE_VIRTUAL_DESKTOP; motion.Add(packet,nullptr,3840,1080);
  Require(motion.Consume().x==0,"Monitor mapping changes must reset the absolute baseline");
  motion.Add(packet,reinterpret_cast<HANDLE>(1),3840,1080);
  Require(motion.Consume().x==0,"Switching absolute devices must not jump the camera");
  packet.lLastX+=1000; motion.Add(packet,reinterpret_cast<HANDLE>(1),3840,1080);
  motion.Reset(); const auto reset=motion.Consume();
  Require(reset.x==0 && reset.y==0,"Cancellation must discard queued raw movement");
  motion.Add(packet,nullptr,1920,1080);
  Require(motion.Consume().x==0,"New captures must not reuse an old absolute baseline");

  Input input; Input::Frame frame; frame.focused=true; frame.screen={-500,350}; frame.client={600,300};
  input.Update(frame); frame.down[VK_RBUTTON]=true; frame.relativeMouse=true; frame.mouseX=900; input.Update(frame);
  Require(input.Pressed(VK_RBUTTON) && input.MouseX()==0,"Capture start must ignore motion from before the press");
  frame.mouseX=41; frame.mouseY=-7; input.Update(frame);
  Require(input.MouseX()==41 && input.MouseY()==-7,"Relative movement must work with a fixed cursor on a secondary monitor");
  frame.screen={700,800}; frame.mouseX=0; frame.mouseY=0; input.Update(frame);
  Require(input.MouseX()==0 && input.MouseY()==0,"Desktop cursor changes must not be added to raw camera motion");
  frame.relativeMouse=false; frame.screen.x+=300; input.Update(frame);
  Require(input.MouseX()==0,"Leaving capture between samples must not turn a cursor jump into camera motion");
  frame.relativeMouse=true;
  frame.down[VK_RBUTTON]=false; frame.mouseX=900; input.Update(frame);
  Require(input.Released(VK_RBUTTON) && input.MouseX()==0,"Button release must discard remaining camera movement");
  frame.down[VK_RBUTTON]=true; frame.focused=false; input.Update(frame);
  Require(input.MouseX()==0,"Unfocused raw input must not move the camera");
  frame.focused=true; input.Update(frame);
  Require(!input.Pressed(VK_RBUTTON) && input.MouseX()==0,"Focus return with RMB held must not start a capture");

  Game game; game.Initialize(); input=Input{}; frame=Input::Frame{};
  frame.focused=true; frame.client={600,300}; frame.screen=frame.client; frame.relativeMouse=true;
  const auto step=[&]() { input.Update(frame); game.Update(.05,input,0); };
  step(); frame.down[VK_RBUTTON]=true; step();
  Require(game.WantsMouseLook(),"A fresh world RMB press must request the native cursor lock");
  frame.mouseX=50; step();
  Require(game.CameraYaw()>.1f,"Raw motion must rotate the camera without moving the cursor");
  frame.down[VK_RBUTTON]=false; step();
  Require(!game.WantsMouseLook(),"RMB release must relinquish the cursor lock");
  frame.down[VK_RBUTTON]=true; step(); frame.down[VK_ESCAPE]=true; step();
  Require(!game.WantsMouseLook(),"Opening a menu must release the cursor lock");
  frame.down[VK_ESCAPE]=false; step(); frame.down[VK_ESCAPE]=true; step();
  Require(!game.WantsMouseLook(),"Closing a menu with RMB held must not reacquire the lock");
  frame.down[VK_ESCAPE]=false; frame.down[VK_RBUTTON]=false; step();
  frame.down[VK_RBUTTON]=true; step(); frame.focused=false; step();
  Require(!game.WantsMouseLook(),"Focus loss must cancel camera capture");
  frame.focused=true; step();
  Require(!game.WantsMouseLook(),"Returning with RMB held must require a new gesture");
  frame.down[VK_RBUTTON]=false; step(); frame.down[VK_RBUTTON]=true; step();
  Require(game.WantsMouseLook(),"A new press after focus cancellation must work");
  Require(game.RestoreSnapshot(game.State().Save()) && !game.WantsMouseLook(),"Loading must release camera capture");
  step(); Require(!game.WantsMouseLook(),"Holding RMB through a load must not relock the cursor");
  frame.down[VK_RBUTTON]=false; step(); frame.down[VK_RBUTTON]=true; step();
  frame.down[VK_F11]=true; step();
  Require(!game.WantsMouseLook(),"Display changes must cancel the cursor lock");
  frame.down[VK_F11]=false; step();
  Require(!game.WantsMouseLook(),"Holding RMB through a display change must not relock the cursor");
}

void InterruptedInputRegressions(Win32Window& window,Renderer& renderer) {
  // The real message-pump order: latch native interruptions, cancel gestures,
  // sample input, update, render. No system focus or desktop input is changed.
  Game game; Input input; Input::Frame frame;
  frame.focused=true; frame.client={1000,563}; frame.screen=frame.client;
  const auto step=[&]() {
    if (window.ConsumeInputCancellation()) { game.CancelUiGesture(); input.CancelGestures(); }
    input.Update(frame); game.Update(.01,input,0);
    renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Interrupted input render");
  };
  const auto click=[&](LONG x,LONG y) {
    frame.client={x,y}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
    frame.down[VK_LBUTTON]=false; step();
  };
  const auto interrupt=[&](UINT message) {
    SendMessage(window.GetHandle(),message,0,0);
    if (message==WM_KILLFOCUS) SendMessage(window.GetHandle(),WM_SETFOCUS,0,0);
  };
  for (UINT message : {WM_KILLFOCUS,WM_CANCELMODE,WM_ENTERSIZEMOVE,WM_ENTERMENULOOP,WM_MOVE,WM_SIZE}) {
    Require(game.RestoreSnapshot(Fixture("veteran-preview.save")),"Interrupted build fixture");
    game.OpenPanel(ui::Panel::Build); step();
    interrupt(message);
    // This press started during the interruption, not on the game's Learn button.
    frame.client={1000,563}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
    Require(game.State().BuildMask()==0,"A press across a native interruption must not learn a talent");
    step(); Require(game.State().BuildMask()==0,"Holding an interrupted press must not activate a control after redraw");
    frame.down[VK_LBUTTON]=false; step(); click(1000,563);
    Require(game.State().BuildMask()==1,"A fresh click after release must still learn normally");
    click(700,118); click(980,568); // Store allocation, pending confirmation.
    Require(game.State().Loadout(0)==-1,"Storage still requires confirmation");
    interrupt(message); step();
    click(660,568);
    Require(game.State().Loadout(0)==-1,"An interrupted store confirmation must not survive a native focus/move/menu event");
    click(980,568); click(660,568);
    Require(game.State().Loadout(0)==1,"Storage must work after a new explicit request and confirmation");
  }
  for (bool native : {true,false}) {
    game.Initialize(); game.OpenPanel(ui::Panel::Inventory); frame.down.fill(false); step();
    frame.client={1028,274}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
    frame.client={1068,274}; frame.screen=frame.client; step();
    if (native) interrupt(WM_CANCELMODE); else frame.focused=false;
    step(); frame.focused=true; step(); frame.down[VK_LBUTTON]=false; step();
    Require(game.State().Items().Bag()[0].item==rpg::Item::Potion && game.State().Items().Bag()[1].item==rpg::Item::None,
      "Focus/cancel-mode interruption must discard an in-flight bag drag without moving or consuming the item");
    frame.client={1028,274}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
    frame.client={1068,274}; frame.screen=frame.client; step();
    frame.down[VK_LBUTTON]=false; step();
    Require(game.State().Items().Bag()[0].item==rpg::Item::None && game.State().Items().Bag()[1].item==rpg::Item::Potion,
      "A fresh inventory drag must work after cancellation");
  }
}

int main() {
  RelativeMouseRegressions();
  {
    Input input; Input::Frame frame; frame.focused=true; frame.relativeMouse=true; input.Update(frame);
    frame.down[VK_RBUTTON]=true; input.Update(frame);
    frame.mouseX=40; input.Update(frame);
    Require(input.MouseX()==40,"Cancellation fixture must have live raw motion");
    input.CancelGestures();
    Require(!input.Pressed(VK_RBUTTON) && !input.Released(VK_RBUTTON) && !input.Held(VK_RBUTTON) && input.MouseX()==0,
      "Cancellation must discard sampled transitions and raw motion immediately");
    for (int key : std::array<int,6>{VK_LBUTTON,VK_RBUTTON,VK_F5,VK_F9,'N','4'}) frame.down[key]=true;
    input.Update(frame);
    for (int key : std::array<int,6>{VK_LBUTTON,VK_RBUTTON,VK_F5,VK_F9,'N','4'})
      Require(!input.Pressed(key) && input.Held(key),"Resynchronization must retain held state without fabricating a press");
    Require(input.MouseX()==0,"The first sample after cancellation must ignore queued camera motion");
    input.Update(frame);
    Require(!input.Pressed(VK_LBUTTON) && !input.Pressed(VK_F9),"Held interrupted inputs cannot replay on the next frame");
    frame.down.fill(false); input.Update(frame); frame.down[VK_LBUTTON]=true; frame.down[VK_F9]=true; input.Update(frame);
    Require(input.Pressed(VK_LBUTTON) && input.Pressed(VK_F9),"Fresh presses after release must resume normally");
  }
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
      const auto col=unsigned((x/rpg::WorldMap::surveyWidth+.5f)*terrainSurfaceSize),
        row=unsigned((.5f-z/rpg::WorldMap::surveyDepth)*terrainSurfaceSize);
      return pixels[(row*terrainSurfaceSize+col)*4+channel];
    };
    Require(sample(0,-10,1)==255 && sample(0,10,1)==0 && sample(0,10,0)>245 && sample(12,-2,0)==255 &&
      sample(-15,16,0)==0 && sample(-15,16,1)==0,"Ground texture orientation must match world and atlas coordinates.");
  }
  ui::Interface screen;
  for(auto panel:{ui::Panel::Character,ui::Panel::Inventory,ui::Panel::Map,ui::Panel::Journal,ui::Panel::Build,ui::Panel::Quest,ui::Panel::Merchant}) {
    screen.SetOnly(panel);Require(!screen.CapturesMovement(),"Only Escape/menu may capture movement");
  }
  screen.SetOnly(ui::Panel::None);
  screen.MoveHotbar(0,9,rpg::ClassId::Warrior);
  Require(screen.HotbarSpell(9,rpg::ClassId::Warrior)==0 && screen.HotbarSpell(9,rpg::ClassId::Mage)==-1,"Hotbar layouts are class-specific");
  const auto bindings=screen.SaveHotbars();ui::Interface copied;
  Require(copied.LoadHotbars(bindings) && copied.SaveHotbars()==bindings,"Hotbar bindings round trip");
  Require(!copied.LoadHotbars(bindings+"junk") && copied.SaveHotbars()==bindings,"Malformed settings must preserve bindings");
  {
    std::string legacy="HOTBARS 3\n";
    for(int id=0;id<int(rpg::ClassId::Count);++id) legacy+="0 1 2 3 4 5 6 -1 -1 -1\n";
    ui::Interface migrated;Require(migrated.LoadHotbars(legacy),"Version 3 hotbar rejected");
    Require(migrated.HotbarSpell(7,rpg::ClassId::Mage)==-1,"Migration overwrote an existing empty binding");
    migrated.AssignHotbar(7,9,rpg::ClassId::Mage);ui::Interface reload;
    Require(reload.LoadHotbars(migrated.SaveHotbars()) && reload.HotbarSpell(9,rpg::ClassId::Mage)==7,"Setup binding did not persist");
  }
  screen.MoveHotbar(9,0,rpg::ClassId::Warrior);

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
  for (auto panel : {ui::Panel::Menu,ui::Panel::Inventory,ui::Panel::Character,ui::Panel::Map,ui::Panel::Journal,ui::Panel::Build}) {
    Game game; game.Initialize(); Require(game.RestoreSnapshot(Fixture("combat-preview.save")),"Combat fixture must load.");
    game.OpenPanel(panel); const float before=game.State().Health(); const Input idle;
    for (int frame=0;frame<20;++frame) game.Update(.05,idle,0);
    Require(game.State().Health()<before,"Enemy windup/damage must continue while a menu or panel is open.");
  }
  rpg::Simulation simulation; rpg::Command command;
  const auto upgrade=ui::SummarizeItem(simulation,ui::Item::Watchguard);
  for(int id=0;id<int(rpg::ClassId::Count);++id) {
    rpg::Simulation hero;Require(hero.CreateCharacter(rpg::ClassId(id),"Inspector"),"Tooltip class fixture");
    const auto summary=ui::SummarizeItem(hero,hero.Items().Worn(rpg::GearSlot::Weapon));
    Require(summary.stats=="Strike "+std::to_string(int(hero.StrikeDamage()))+"  /  Heavy "+std::to_string(int(hero.HeavyDamage())),
      "Weapon tooltip omits class damage and disagrees with actual attacks");
    const auto candidate=ui::SummarizeItem(hero,rpg::Item::Watchguard);
    const auto weapon=hero.ItemDefinition(rpg::Item::Watchguard);
    Require(candidate.stats=="Strike "+std::to_string(int(weapon.strike+hero.StrikeBonus()+hero.ClassInfo().strike))+
      "  /  Heavy "+std::to_string(int(weapon.heavy+hero.HeavyBonus()+hero.ClassInfo().heavy)),"Shop weapon preview omits class damage");
  }
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
  window.SetLoading(true);
  const auto loadingGdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
  for(int paint=0;paint<32;++paint) window.SetLoading(true);
  window.SetLoading(false);
  Require(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=loadingGdi,"Startup title card leaked GDI resources");
  Require(!window.SetMouseLook(true) && !window.MouseLook(),"Hidden windows must never capture the desktop cursor");
  // Native interruptions must reach UI gestures even without an RMB capture.
  // Focus can leave and return between two engine samples.
  window.ConsumeInputCancellation();
  SendMessage(window.GetHandle(),WM_KILLFOCUS,0,0);
  SendMessage(window.GetHandle(),WM_SETFOCUS,0,0);
  Require(window.ConsumeInputCancellation(),"A complete focus round trip between frames must cancel pending UI actions without RMB");
  Require(!window.ConsumeInputCancellation(),"Native cancellation must be consumed once");
  for (UINT message : {WM_CANCELMODE,WM_ENTERSIZEMOVE,WM_ENTERMENULOOP,WM_MOVE,WM_SIZE,WM_ACTIVATEAPP}) {
    SendMessage(window.GetHandle(),message,0,0);
    Require(window.ConsumeInputCancellation(),"Window move, size, menu and deactivation must cancel UI gestures without RMB");
  }
  window.SetMouseLook(false);
  Require(!window.ConsumeInputCancellation(),"Ordinary inactive mouse-look polling must not cancel UI actions");
  SendMessage(window.GetHandle(),WM_CAPTURECHANGED,0,0);
  Require(!window.ConsumeInputCancellation(),"A release notification with no active capture must not create another cancellation");
  SendMessage(window.GetHandle(),WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),0);
  window.SetMouseLook(false);
  Require(window.ConsumeWheel()==1,"Ordinary inactive mouse-look polling must retain legitimate camera scroll");
  SendMessage(window.GetHandle(),WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),0);
  SendMessage(window.GetHandle(),WM_KILLFOCUS,0,0); SendMessage(window.GetHandle(),WM_SETFOCUS,0,0);
  Require(window.ConsumeWheel()==0 && window.ConsumeInputCancellation(),"Focus interruptions must discard queued wheel motion");
  Renderer renderer;
  const bool initialized=renderer.Initialize(window.GetHandle(),1280,720);
  if (!initialized) std::wcerr<<renderer.Error()<<L'\n';
  Require(initialized,"screen renderer initialization failed.");
  Require(renderer.InterfaceBytes()<20*1024*1024,"Interface artwork exceeds its 20 MiB texture budget.");
  InterruptedInputRegressions(window,renderer);
  // Quest conversations keep rewards/objectives visible and paginate all prose.
  for (const auto size : {std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(renderer.Resize(size.first,size.second),"Quest dialogue resize");
    for (int scenario=0;scenario<4;++scenario) {
      rpg::Simulation state; ui::Interface dialogue;
      if (scenario==1) Require(state.Load(test::patrolReturn),"Patrol hand-in UI fixture");
      if (scenario==2) Require(state.Load(test::anchorOffer),"Anchor offer UI fixture");
      if (scenario==3) Require(state.Load(test::anchorReturn),"Anchor hand-in UI fixture");
      rpg::Command talk; talk.interact=true; state.Update(.01f,talk); dialogue.SetOnly(ui::Panel::Quest);
      const auto snapshot=state.Save(); const ui::Layout layout(float(size.first),float(size.second));
      const auto click=[&](float x,float y) {
        return dialogue.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,
          float(size.first),float(size.second),true,false);
      };
      const auto render=[&](const wchar_t* stage) {
        renderer.BeginFrame(); dialogue.Render(renderer,state,0,false);
        const auto path=L"quest-dialogue-"+std::to_wstring(scenario)+L"-"+stage+L"-"+
          std::to_wstring(size.first)+L"x"+std::to_wstring(size.second)+L".png";
        Require(renderer.Capture(path) && renderer.EndFrame(false),"Quest dialogue capture");
      };
      render(L"preview");
      Require(state.Save()==snapshot && !dialogue.CapturesMovement(),"Reading must preserve progress without capturing movement");
      Require(click(700,300).action==ui::Action::None && click(493,681).action==ui::Action::None,
        "Quest dialogue must prevent clicks reaching the world and action bar");
      Require(click(60,380).action==ui::Action::None,"Previous page is disabled on the first page");
      auto next=click(400,385);
      Require(next.action==ui::Action::QuestPage && next.value==1,"Detailed offers/turn-ins must expose a next page");
      dialogue.questPage=1000; render(L"last-page");
      Require(dialogue.questPage>0 && click(400,385).action==ui::Action::None,"The last page must clamp and disable Next");
      Require(click(60,385).action==ui::Action::QuestPage,"Previous must remain reachable on the last page");
      auto confirm=click(340,608);
      Require(confirm.action==ui::Action::QuestConfirm &&
        confirm.value==int(state.QuestPrimaryAction()),"Scaled confirmation must target the displayed quest and stage");
      rpg::Command chosen; chosen.questAction=rpg::QuestAction(confirm.value); state.Update(.01f,chosen);
      render(L"result");
      Require(dialogue.questPage==0 && click(340,608).action==ui::Action::Close,
        "Accepted/completed dialogue must return to page one and replace the confirmation with Close");
      const auto result=state.Save(); state.Update(.01f,chosen);
      Require(state.Save()==result,"A stale UI confirmation must not repeat the transaction");
    }
  }
  Require(renderer.Resize(1280,720),"Restore size after quest captures");
  {
    Game game; game.Initialize();
    Input input; Input::Frame frame; frame.focused=true; frame.client={600,300}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.01,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Quest input render");
    };
    step(); frame.down[VK_RBUTTON]=true; step();
    Require(game.WantsMouseLook(),"Quest fixture must start with RMB orbit");
    frame.down['E']=true; step();
    Require(game.State().QuestDialogue()==rpg::QuestId::Patrol && !game.WantsMouseLook() &&
      game.State().QuestState()==rpg::Quest::Available,"E must open the offer and release RMB without accepting");
    const auto standing=game.State().Player(); frame.down['A']=frame.down['3']=true; step();
    Require((game.State().Player().x!=standing.x || game.State().Player().z!=standing.z) &&
      game.State().QuestState()==rpg::Quest::Available,"Reading must allow movement without accepting a quest");
    frame.down['A']=frame.down['3']=frame.down[VK_RBUTTON]=false;
    frame.client={340,608}; frame.down[VK_LBUTTON]=true; step();
    if(game.State().QuestState()!=rpg::Quest::Active) std::cerr<<"Quest input: "<<game.State().Message()<<" dialogue="<<int(game.State().QuestDialogue())<<" position="<<game.State().Player().x<<","<<game.State().Player().z<<'\n';
    Require(game.State().QuestState()==rpg::Quest::Active && game.State().Gold()==0,
      "Clicking Accept must dispatch a real quest action");
    frame.down[VK_LBUTTON]=false; frame.down['E']=false; step();
    frame.down[VK_ESCAPE]=true; step();
    Require(game.State().QuestDialogue()==rpg::QuestId::None,"Escape must dismiss conversation without reopening it");
    frame.down[VK_ESCAPE]=false; step(); frame.down[VK_ESCAPE]=true; step();
    frame.down[VK_ESCAPE]=false; frame.down['E']=true; step();
    Require(game.State().QuestDialogue()==rpg::QuestId::Patrol,"The keeper must reopen an active reminder");
    frame.focused=false; step();
    Require(game.State().QuestDialogue()==rpg::QuestId::None,"Focus loss must discard dialogue confirmation state");
    const auto saved=game.State().Save();
    Require(game.RestoreSnapshot(saved) && game.State().QuestDialogue()==rpg::QuestId::None,
      "Reload must not replay a conversation or pending click");
  }

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
    const auto triangles=renderer.Triangles()-renderer.CloudLightUpdates();
    const auto windup=snapshot.find("1 0.5 3.14159"); Require(windup!=std::string::npos,"Expected pending enemy swing.");
    snapshot.replace(windup,std::string("1 0.5 3.14159").size(),"1 0 3.14159");
    Require(game.RestoreSnapshot(snapshot),"Defeated neutral fixture must load.");
    renderer.BeginFrame(); game.Render(renderer,false);
    Require(renderer.EndFrame(false),"Defeated neutral render failed.");
    Require(renderer.Triangles()-renderer.CloudLightUpdates()==triangles,"A defeated save must not render an obsolete attack warning cone.");
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
  for (auto panel : {ui::Panel::Menu,ui::Panel::Map,ui::Panel::Journal,ui::Panel::Build}) {
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
    Require(!game.WantsMouseLook(),"Bag right-click actions must keep the cursor visible and free");
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
    Game game;game.Initialize();game.OpenPanel(ui::Panel::Inventory);
    Input input;Input::Frame frame;frame.focused=true;frame.client={1216,237};
    const auto draw=[&]() {renderer.BeginFrame();game.Render(renderer);Require(renderer.EndFrame(false),"Load notice regression render");};
    const auto step=[&]() {input.Update(frame);game.Update(.01,input,0);draw();};
    step();frame.down[VK_LBUTTON]=true;step();frame.down[VK_LBUTTON]=false;step();
    const auto snapshot=game.State().Save();Require(game.RestoreSnapshot(snapshot),"Notice reload fixture");draw();
    frame.client={700,140};frame.down[VK_RBUTTON]=true;step();
    Require(game.WantsMouseLook(),"Loaded character retained the previous inventory notification and its input blocker");
  }
  {
    Game game; game.Initialize(); Input input; Input::Frame frame;
    frame.focused=true; frame.client={802,628}; frame.screen=frame.client;
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
    Require((game.State().Player().x!=before.x || game.State().Player().z!=before.z),"Trade must leave movement live.");
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
  {
    Require(renderer.Resize(1280,720),"Build input test resolution");
    Game game; Require(game.RestoreSnapshot(Fixture("veteran-preview.save")),"Build input fixture");
    Input input; Input::Frame frame; frame.focused=true; frame.client={680,380}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.01,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Champion build input render");
    };
    step(); frame.down[VK_RBUTTON]=true; step();
    Require(game.WantsMouseLook(),"Build input test must begin with an active orbit");
    const auto position=game.State().Player();
    frame.down['N']=true; frame.down['W']=true; frame.down['3']=true; step();
    Require(game.State().Player().z!=position.z && game.State().Cooldown(2)==0,
      "N must preserve movement; a targetless dash remains unavailable");
    for (int i=0;i<3;++i) step();
    Require(game.State().Player().z!=position.z,"Holding N must preserve movement without toggling repeatedly");
    frame.down['N']=frame.down['W']=frame.down['3']=frame.down[VK_RBUTTON]=false; step();
    const auto clickAt=[&](LONG x,LONG y) {
      frame.client={x,y}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
      frame.down[VK_LBUTTON]=false; step();
    };
    clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::WeaponTraining) && game.State().TalentPoints()==1,
      "Build Learn button must reach the simulation command");
    clickAt(248,406); clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::WeaponMastery) && game.State().TalentPoints()==0,
      "Selecting a connected rank must learn that rank rather than the previous selection");
    clickAt(735,183);
    Require(game.State().HasTalent(rpg::Talent::WeaponMastery),"Inspecting a spell must preserve the build");
    clickAt(730,604);
    Require(game.State().TalentPoints()==0,"Requesting a reset must not reset before confirmation");
    clickAt(1000,575);
    Require(game.State().TalentPoints()==0,"Cancelling a reset must preserve all learned talents");
    clickAt(730,604); clickAt(1000,540);
    Require(game.State().TalentPoints()==2 && !game.State().HasTalent(rpg::Talent::WeaponMastery),
      "Confirmed reset must reach the simulation and refund the spent points");
    clickAt(248,512); clickAt(1000,563);
    Require(!game.State().HasTalent(rpg::Talent::Executioner),"Alternate specializations still require their foundation");
    clickAt(248,320); clickAt(1000,563); clickAt(248,512); clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::Executioner) && game.State().TalentPoints()==0,
      "Alternative spell upgrades must be learnable through real input");
    clickAt(730,604); frame.focused=false; step(); frame.focused=true; step(); clickAt(1000,540);
    Require(game.State().HasTalent(rpg::Talent::Executioner),"Focus loss must discard a pending reset confirmation");
    clickAt(700,118); // Saved builds opens with a snapshot of the active build.
    clickAt(980,568);
    Require(game.State().Loadout(0)==-1,"Store request cannot mutate a slot before confirmation");
    clickAt(660,568);
    Require(game.State().Loadout(0)==65 && game.State().BuildMask()==65,"Confirmed storage saves the active mixed choice without applying anything");
    clickAt(280,290); clickAt(376,550); // Slot II, Pursuit preset.
    Require(game.State().Loadout(1)==-1 && game.State().BuildMask()==65,"Selecting an empty slot and previewing a preset is read-only");
    clickAt(660,568); clickAt(980,568);
    Require(game.State().BuildMask()==65,"Cancelling apply preserves current talents");
    clickAt(660,568); clickAt(660,568);
    Require(game.State().BuildMask()==130 && game.State().MaxHealth()==160 && game.State().Health()==140,
      "Confirmed preset applies actual stats without healing");
    clickAt(980,568); clickAt(660,568);
    Require(game.State().Loadout(1)==130 && game.State().Loadout(0)==65,"Preset storage must target the selected slot only");
    clickAt(204,584); clickAt(660,568); // Renewal preview and request.
    frame.focused=false; step(); frame.focused=true; step(); clickAt(660,568);
    Require(game.State().BuildMask()==130,"Focus loss must require a new build confirmation");
    clickAt(280,196); // Select saved slot I while confirmation is pending.
    clickAt(660,568);
    Require(game.State().BuildMask()==130,"Changing preview must cancel the previous request");
    clickAt(660,568);
    Require(game.State().BuildMask()==65,"A saved slot must be independently applicable");
    clickAt(376,584); clickAt(980,568); // Mercy, overwrite slot I.
    Require(game.State().Loadout(0)==65,"Overwrite requires confirmation too");
    clickAt(980,568);
    Require(game.State().Loadout(0)==65,"Cancel preserves an existing slot");
    clickAt(980,568); clickAt(660,568);
    Require(game.State().Loadout(0)==260 && game.State().BuildMask()==65,"Overwrite changes the chosen saved plan, not active talents");
    clickAt(660,568); // Request application, then switch to the talent page.
    clickAt(700,118); clickAt(660,568);
    Require(game.State().BuildMask()==65,"Switching pages must discard pending confirmation and stale click regions");
    clickAt(500,118); // Plan a custom build without changing the hero.
    clickAt(248,320);clickAt(1000,563); // Remove Arms and its descendant.
    Require(game.State().BuildMask()==65,"Planning removed live talents before confirmation");
    clickAt(480,320);clickAt(1000,563); // Add Vitality to draft.
    Require(game.State().BuildMask()==65,"Planning added a live talent before confirmation");
    clickAt(700,118);clickAt(660,568); // Review, request apply.
    Require(game.State().BuildMask()==65,"Draft application bypassed confirmation");
    clickAt(660,568);
    Require(game.State().BuildMask()==2,"Confirmed custom draft was not applied");
    frame.down['N']=frame.down['W']=true; step();
    Require(game.State().Player().z>position.z,"A fresh N press must close the menu and restore movement");
  }
  for (const auto size : {std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(simulation.Load(Fixture("veteran-preview.save")),"Reset pointer fixture.");
    Require(renderer.Resize(size.first,size.second),"screen resize failed.");
    const ui::Layout layout(float(size.first),float(size.second));
    const auto click=[&](float x,float y) { auto e=screen.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),true,false); if(e.action==ui::Action::None) e=screen.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),false,false,false,true);return e; };
    screen.SetOnly(ui::Panel::Inventory); screen.selected=ui::Item::Starter;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Inventory render failed.");
    Require(click(1028,274).action==ui::Action::SelectItem,"Item slot did not receive a scaled click.");
    const auto equip=click(1110,620);
    Require(equip.action==ui::Action::UseItem && equip.value==int(ui::Item::Starter),"Equip button did not target the inspected item.");
    const auto empty=click(1228,400);
    Require(empty.action==ui::Action::SelectItem && empty.value==int(ui::Item::None),
      "Clicking an empty bag cell must clear the old inspected item");
    screen.selected=ui::Item(empty.value); screen.selectedBag=empty.inventory.source;
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Empty selection render failed.");
    Require(click(1110,620).action==ui::Action::None,
      "Clearing inspection must disable the old item's action");
    Require(screen.Pointer(layout.x+1228*layout.scale,layout.y+400*layout.scale,float(size.first),float(size.second),false,true).action==ui::Action::None,
      "Right-clicking an empty bag cell must not issue an item-use command");
    screen.Show(ui::Panel::Character);
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Combined equipment/bag render failed.");
    Require(click(40,210).action==ui::Action::SelectItem && screen.inventoryOpen,"Worn gear inspection must leave the bag open.");
    const auto closeBag=click(1250,213);
    Require(closeBag.action==ui::Action::Close && closeBag.value==int(ui::Panel::Inventory),"Bag close must identify only the bag.");
    Require(click(1040,580).action==ui::Action::None && screen.PointerOverWindow(),"Blank bag surface must capture its pointer.");
    Require(click(680,380).action==ui::Action::None && !screen.PointerOverWindow(),"World gap must remain available for camera orbit.");
    Require(click(493,681).action==ui::Action::None,"Dash requires an available target with side windows open.");
    const auto beforeDash=simulation.Save();
    Require(simulation.Load(test::dashFixture),"Interface target dash fixture");
    rpg::Command selectDash; selectDash.nextTarget=true; simulation.Update(.001f,selectDash);
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false); Require(renderer.EndFrame(false),"Target dash render");
    Require(click(493,681).action==ui::Action::Ability,"Available Dash must remain usable with both side windows open.");
    const auto gesture=[&](float x,bool press,bool held,bool release) {
      return screen.Pointer(layout.x+x*layout.scale,layout.y+681*layout.scale,float(size.first),float(size.second),press,false,held,release);
    };
    Require(gesture(493,true,true,false).action==ui::Action::None,"Hotbar press must not cast before a potential drag");
    gesture(898,false,true,false);const auto drop=gesture(898,false,false,true);
    Require(drop.action==ui::Action::None && screen.HotbarSpell(9,simulation.Class())==2 && screen.HotbarSpell(2,simulation.Class())==-1,"Drag must move spell to key zero without casting");
    renderer.BeginFrame();screen.Render(renderer,simulation,0,false);Require(renderer.EndFrame(false),"Moved hotbar render");
    Require(click(898,681).action==ui::Action::Ability && click(493,681).action==ui::Action::None,"Moved action uses its new slot; old slot is empty");
    screen.MoveHotbar(9,2,simulation.Class());
    Require(simulation.Load(beforeDash),"Restore interface state after target dash check");
    screen.SetOnly(ui::Panel::None);
    renderer.BeginFrame();screen.Render(renderer,simulation,0,false);Require(renderer.EndFrame(false),"Tracker layout render");
    Require(click(1120,245).action==ui::Action::None && screen.PointerOverUi(),
      "Visible expedition tracker must block camera gestures behind its text");
    {
      ui::Interface overlay;rpg::Simulation idle;
      const auto draw=[&]() {renderer.BeginFrame();overlay.Render(renderer,idle,0,false);Require(renderer.EndFrame(false),"Overlay hit layout");};
      const auto hover=[&](float x,float y) {overlay.Pointer(layout.x+x*layout.scale,layout.y+y*layout.scale,float(size.first),float(size.second),false,false);return overlay.PointerOverUi();};
      overlay.noticeTime=2;overlay.inventoryNotice="Equipment updated.";draw();
      Require(hover(700,140),"Visible inventory notice must block camera capture");
      overlay.noticeTime=0;draw();Require(!hover(700,140),"Expired notice must release camera input");
      overlay.Show(ui::Panel::Inventory);draw();
      Require(hover(1003,244),"Bag replacing the tracker must retain pointer protection");
      overlay.Close(ui::Panel::Inventory);overlay.hasWaypoint=true;draw();
      Require(hover(1120,365),"Tracker waypoint extension must block camera input");
      overlay.hasWaypoint=false;draw();Require(!hover(1120,365),"Removed waypoint must release its old tracker region");
    }
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
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false);
    const auto atlasCapture=L"atlas-drawn-"+std::to_wstring(size.first)+L"x"+std::to_wstring(size.second)+L".png";
    Require(renderer.Capture(atlasCapture) && renderer.EndFrame(false),"Atlas render/capture failed.");
    Require(click(477.5f,366).action==ui::Action::PlaceWaypoint,"Atlas must accept a map click.");
    Require(std::abs(screen.pendingWaypoint.x)<.001f && std::abs(screen.pendingWaypoint.z)<.001f,"Waypoint coordinates must match the world map.");
    screen.hasWaypoint=true; screen.waypoint={0,0}; click(160,366);
    Require(!simulation.CanStand(screen.pendingWaypoint) && screen.waypoint.x==0 && screen.waypoint.z==0,"Invalid map candidate must preserve the existing waypoint.");
    screen.Pointer(layout.x+477.5f*layout.scale,layout.y+366*layout.scale,float(size.first),float(size.second),false,true);
    Require(!screen.hasWaypoint,"Right-clicking the atlas must clear a waypoint.");

    {
      rpg::Simulation buildState; Require(buildState.Load(Fixture("veteran-preview.save")),"Scaled build fixture");
      screen.SetOnly(ui::Panel::None);
      renderer.BeginFrame(); screen.Render(renderer,buildState,0,false); Require(renderer.EndFrame(false),"Build HUD render");
      Require(click(608,628).action==ui::Action::Build,"N shortcut must be clickable at every UI scale");
      screen.SetOnly(ui::Panel::Build); screen.selectedTalent=0;
      Require(click(1000,563).action==ui::Action::None,"Opening N must invalidate the previous layout");
      const auto drawBuild=[&]() {
        renderer.BeginFrame(); screen.Render(renderer,buildState,0,false); Require(renderer.EndFrame(false),"Scaled build render");
      };
      drawBuild();
      Require(!screen.CapturesMovement(),
        "Build menu must leave movement live");
      for (int i=0;i<int(rpg::Talent::Count);++i) {
        screen.talentPage=i/9;drawBuild();
        const auto node=click(248+(i%3)*232.f,320+((i%9)/3)*106.f);
        Require(node.action==ui::Action::SelectTalent && node.value==i,"Every connected talent node must accept scaled inspection");
      }
      screen.talentPage=0;screen.selectedTalent=4; drawBuild();
      Require(click(1000,563).action==ui::Action::None,"A rank with a missing prerequisite must disable Learn");
      screen.selectedTalent=1; drawBuild();
      const auto learn=click(1000,563);
      Require(learn.action==ui::Action::LearnTalent && learn.value==1,"Learn must target the selected talent");
      rpg::Command learnCommand; learnCommand.learnTalent=learn.value; buildState.Update(.01f,learnCommand); drawBuild();
      Require(buildState.MaxHealth()==160 && buildState.TalentPoints()==1 && click(1000,563).action==ui::Action::None,
        "Learning must update stats/points and disable duplicate allocation");
      screen.selectedTalent=4; drawBuild();
      Require(click(1000,563).action==ui::Action::LearnTalent,"Learning the parent must unlock its connected rank");
      learnCommand.learnTalent=4; buildState.Update(.01f,learnCommand);
      renderer.BeginFrame(); screen.Render(renderer,buildState,0,false);
      const auto capture=L"champion-build-"+std::to_wstring(size.first)+L"x"+std::to_wstring(size.second)+L".png";
      Require(renderer.Capture(capture) && renderer.EndFrame(false),"Learned build capture");
      Require(buildState.TalentPoints()==0 && click(1000,563).action==ui::Action::None,"Spent points must not allow another purchase");
      Require(screen.Pointer(layout.x+1000*layout.scale,layout.y+563*layout.scale,float(size.first),float(size.second),false,true).action==ui::Action::None,
        "Right-click must not learn a talent");
      Require(click(1159,73).action==ui::Action::Close && click(1159,73).value==int(ui::Panel::Build),
        "Build close button must identify its modal panel");
      for(int card=0;card<7;++card) {
        const int spell=card>=4?card+1:card;
        const auto inspect=click(210+(card%4)*173.f,167+(card/4)*34.f);
        Require(inspect.action==ui::Action::SelectSpell && inspect.value==spell,
          "Each foundation spell must open inspection, not cast through the modal");
        screen.selectedSpell=spell;
        Require(click(1000,563).action==ui::Action::None,"Changing inspection must invalidate old Learn regions");
        const auto saved=buildState.Save();
        const auto book=L"spellbook-"+std::to_wstring(spell)+L"-"+std::to_wstring(size.first)+L".png";
        renderer.BeginFrame(); screen.Render(renderer,buildState,0,false);
        Require(renderer.Capture(book) && renderer.EndFrame(false) && buildState.Save()==saved,"Spell inspection must be read-only");
        const auto related=click(1000,515);
        Require(related.action==ui::Action::SelectTalent && related.value==int(rpg::spells[spell].upgrades[0]),
          "Spellbook upgrade links must target real talents");
      }
      Require(click(730,604).action==ui::Action::ConfirmTalentReset,"A learned build must offer a reset confirmation");
      screen.confirmTalentReset=true; drawBuild();
      Require(click(1000,540).action==ui::Action::ResetTalents && click(1000,575).action==ui::Action::CancelTalentReset,
        "Reset must require a separate confirmation with a cancel action");
      const auto confirmation=L"build-reset-"+std::to_wstring(size.first)+L".png";
      renderer.BeginFrame(); screen.Render(renderer,buildState,0,false);
      Require(renderer.Capture(confirmation) && renderer.EndFrame(false),"Reset confirmation capture");
      screen.Close(ui::Panel::Build);
      Require(!screen.confirmTalentReset && click(1000,540).action==ui::Action::None,"Closing must discard a pending reset and its hit regions");
      screen.SetOnly(ui::Panel::Build);
      rpg::Command resetBuild; resetBuild.resetTalents=true; buildState.Update(.01f,resetBuild);
      learnCommand.learnTalent=int(rpg::Talent::Vitality); buildState.Update(.01f,learnCommand);
      learnCommand.learnTalent=int(rpg::Talent::Pursuit); buildState.Update(.01f,learnCommand);
      screen.selectedSpell=2;
      const auto upgraded=L"spellbook-pursuit-"+std::to_wstring(size.first)+L".png";
      renderer.BeginFrame(); screen.Render(renderer,buildState,0,false);
      Require(renderer.Capture(upgraded) && renderer.EndFrame(false) && buildState.AbilityCooldown(2)==3.5f,
        "Upgraded spellbook must show the actual fractional cooldown and learned talent links");
      screen.loadoutsPage=true; screen.previewBuild=65; screen.selectedLoadout=1;
      rpg::Command store; store.build={rpg::BuildAction::Store,130,0}; buildState.Update(.01f,store);
      const auto drawLoadout=[&](const wchar_t* label) {
        const auto saved=buildState.Save();
        renderer.BeginFrame(); screen.Render(renderer,buildState,0,false);
        Require(renderer.Capture(std::wstring(L"loadouts-")+label+L"-"+std::to_wstring(size.first)+L".png") &&
          renderer.EndFrame(false) && buildState.Save()==saved,"Build preview render/capture must not mutate gameplay");
      };
      drawLoadout(L"comparison");
      Require(click(700,118).action==ui::Action::BuildPage,"Loadout page must retain navigation back to talents");
      for (int slot=0;slot<3;++slot) {
        const auto select=click(280,190+slot*94.f);
        Require(select.action==ui::Action::SelectLoadout && select.value==slot,"Every saved slot must be selectable at every scale");
      }
      for (int preset=0;preset<6;++preset) {
        const auto select=click(204+(preset%2)*172.f,516+(preset/2)*34.f);
        Require(select.action==ui::Action::SelectBuildPreset && select.value==preset,"All specialization presets must be inspectable at every scale");
      }
      Require(click(660,568).action==ui::Action::RequestBuildApply && click(980,568).action==ui::Action::RequestBuildStore,
        "Apply and store must request confirmation, never directly mutate");
      screen.pendingBuild={rpg::BuildAction::Store,65,1};
      Require(click(660,568).action==ui::Action::None,"Pending build changes must invalidate the previous actions");
      drawLoadout(L"confirmation");
      Require(click(660,568).action==ui::Action::ConfirmBuild && click(980,568).action==ui::Action::CancelBuild,
        "Confirmation requires a separate rendered button and offers cancellation");
      screen.CancelBuildConfirmation();
      Require(click(660,568).action==ui::Action::None,"Cancellation discards stale confirmation regions");
      screen.pendingBuild={rpg::BuildAction::Apply,65,1}; drawBuild();
      screen.Escape();
      Require(screen.pendingBuild.action==rpg::BuildAction::None && click(660,568).action==ui::Action::None,
        "Escape discards pending build commands");
      screen.SetOnly(ui::Panel::Build); screen.loadoutsPage=true; screen.previewBuild=260;
      buildState=rpg::Simulation{}; drawLoadout(L"locked");
      Require(click(660,568).action==ui::Action::None && click(980,568).action==ui::Action::None &&
        click(376,584).action==ui::Action::SelectBuildPreset,"Level one may inspect presets but cannot apply or store unearned talents");
      screen.SetOnly(ui::Panel::Build);
      screen.selectedSpell=-1;
      buildState=rpg::Simulation{}; screen.selectedTalent=0; drawBuild();
      Require(click(1000,563).action==ui::Action::None && click(248,320).action==ui::Action::SelectTalent,
        "Level one can inspect future talents but cannot learn them");
      screen.Close(ui::Panel::Build);
      Require(click(1000,563).action==ui::Action::None,"Closing N must discard stale learning regions");
    }

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
    render(); const auto rightUse=pointer(40,338,false,false,false,true); rpg::Command removeBoots;
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
    apply(drag(1028+float(helmCell%6)*40,274+float(helmCell/6)*42,277,338));
    Require(simulation.Items().Worn(rpg::GearSlot::Head)==rpg::Item::Helm && screen.panel==ui::Panel::Merchant,
      "Bought helm must drag into Head while trade remains open.");
    render(); purchase=click(900,363); command={}; command.purchase=purchase.value; simulation.Update(.01f,command); render();
    const int shieldCell=simulation.Items().Find(rpg::Item::Shield); Require(shieldCell>=0,"Shield purchase must reach the bag.");
    apply(drag(1028+float(shieldCell%6)*40,274+float(shieldCell/6)*42,277,274));
    Require(simulation.Items().Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield && simulation.DamageReduction()==5,"Off-hand drag must update actual defense.");
    render(); Require(click(493,681).action==ui::Action::None,"Trade must block the underlying action bar.");
    const auto rotate=click(147,593); Require(rotate.action==ui::Action::RotatePreview && rotate.value==1,"Portrait turn control must be clickable at every scale.");
    for (auto item : {rpg::Item::Shoulders,rpg::Item::Gloves,rpg::Item::Belt,rpg::Item::Cloak}) {
      const auto gear=ui::Describe(item).slot;
      const float gx=(item==rpg::Item::Shoulders || item==rpg::Item::Cloak)?40.f:277.f;
      const float gy=(item==rpg::Item::Shoulders || item==rpg::Item::Gloves)?402.f:466.f;
      render(); const auto remove=pointer(gx,gy,false,false,false,true);
      Require(remove.action==ui::Action::UseItem && remove.value==int(item),"Each added slot must route its own right-click.");
      command={}; ui::ApplyItem(command,simulation,item); simulation.Update(.01f,command);
      const auto outfit=CharacterOutfit::From(simulation.Items());
      const bool visible=item==rpg::Item::Shoulders?outfit.shoulders:item==rpg::Item::Gloves?outfit.gloves:
        item==rpg::Item::Belt?outfit.belt:outfit.cloak;
      Require(simulation.Items().Worn(gear)==rpg::Item::None && !visible,
        "Removing a travel piece must remove it from both portrait and world outfit.");
      screen.selected=item; screen.selectedBag=simulation.Items().Find(item); render();
      const auto sellTravel=click(1070,587);
      Require(sellTravel.action==ui::Action::Sell && sellTravel.value==int(item),"Removed travel gear must be sellable.");
      command={}; command.trade={rpg::TradeAction::Sell,item,sellTravel.inventory.source}; simulation.Update(.01f,command);
      Require(simulation.BuybackCount(item)==1,"Selling travel gear must put it on the second buyback row.");
      render(); const auto reclaimTravel=click(511+(int(item)-int(rpg::Item::Shoulders))*51.f,535);
      Require(reclaimTravel.action==ui::Action::Buyback && reclaimTravel.value==int(item),
        "Second-row buyback must select the correct added item at every scale.");
      command={}; command.trade={rpg::TradeAction::Buyback,item}; simulation.Update(.01f,command);
      const int cell=simulation.Items().Find(item); Require(cell>=0,"Reclaimed travel gear must return to the bag.");
      render(); apply(drag(1028+float(cell%6)*40,274+float(cell/6)*42,gx,gy));
      Require(simulation.Items().Worn(gear)==item && simulation.Items().Count(item)==1,
        "Each added slot must accept its matching reclaimed item without duplication.");
    }
    renderer.BeginFrame(); screen.Render(renderer,simulation,0,false);
    const auto gearCapture=std::wstring(L"equipment-ten-slots-")+std::to_wstring(size.first)+L"x"+std::to_wstring(size.second)+L".png";
    Require(renderer.Capture(gearCapture.c_str()) && renderer.EndFrame(false),"Expanded equipment capture failed.");
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
  {
    Require(renderer.Resize(1280,720),"Build input test resolution");
    Game game; Require(game.RestoreSnapshot(Fixture("veteran-preview.save")),"Build input fixture");
    Input input; Input::Frame frame; frame.focused=true; frame.client={680,380}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.01,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Champion build input render");
    };
    step(); frame.down[VK_RBUTTON]=true; step();
    Require(game.WantsMouseLook(),"Build input test must begin with an active orbit");
    const auto position=game.State().Player();
    frame.down['N']=true; frame.down['W']=true; frame.down['3']=true; step();
    Require(game.State().Player().z!=position.z && game.State().Cooldown(2)==0,
      "N must preserve movement; a targetless dash remains unavailable");
    for (int i=0;i<3;++i) step();
    Require(game.State().Player().z!=position.z,"Holding N must preserve movement without toggling repeatedly");
    frame.down['N']=frame.down['W']=frame.down['3']=frame.down[VK_RBUTTON]=false; step();
    const auto clickAt=[&](LONG x,LONG y) {
      frame.client={x,y}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
      frame.down[VK_LBUTTON]=false; step();
    };
    clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::WeaponTraining) && game.State().TalentPoints()==1,
      "Build Learn button must reach the simulation command");
    clickAt(248,406); clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::WeaponMastery) && game.State().TalentPoints()==0,
      "Selecting a connected rank must learn that rank rather than the previous selection");
    frame.down['N']=frame.down['W']=true; step();
    Require(game.State().Player().z>position.z,"A fresh N press must close the menu and restore movement");
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
  for(const auto size:{std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(renderer.Resize(size.first,size.second),"Long-label UI resize");
    rpg::Simulation hero;Require(hero.CreateCharacter(rpg::ClassId::Engineer,"WWWWWWWWWWWWWWWWWWWW"),"Long-name UI fixture");
    ui::Interface screen;screen.Show(ui::Panel::Character);screen.Show(ui::Panel::Inventory);
    renderer.BeginFrame();screen.Render(renderer,hero,0,false);Require(renderer.EndFrame(false),"Long-label layout");
    const ui::Layout layout(float(size.first),float(size.second));
    screen.Pointer(layout.x+360*layout.scale,layout.y+197*layout.scale,float(size.first),float(size.second),false,false);
    renderer.BeginFrame();screen.Render(renderer,hero,0,false);
    Require(renderer.Capture(L"long-labels-"+std::to_wstring(size.first)+L".png") && renderer.EndFrame(false),"Long-label capture");
  }
  Require(renderer.ValidateDebugMessages(),"DirectX reported an screen validation issue.");
  {
    Require(renderer.Resize(1280,720),"Build input test resolution");
    Game game; Require(game.RestoreSnapshot(Fixture("veteran-preview.save")),"Build input fixture");
    Input input; Input::Frame frame; frame.focused=true; frame.client={680,380}; frame.screen=frame.client;
    const auto step=[&]() {
      input.Update(frame); game.Update(.01,input,0);
      renderer.BeginFrame(); game.Render(renderer); Require(renderer.EndFrame(false),"Champion build input render");
    };
    step(); frame.down[VK_RBUTTON]=true; step();
    Require(game.WantsMouseLook(),"Build input test must begin with an active orbit");
    const auto position=game.State().Player();
    frame.down['N']=true; frame.down['W']=true; frame.down['3']=true; step();
    Require(game.State().Player().z!=position.z && game.State().Cooldown(2)==0,
      "N must preserve movement; a targetless dash remains unavailable");
    for (int i=0;i<3;++i) step();
    Require(game.State().Player().z!=position.z,"Holding N must preserve movement without toggling repeatedly");
    frame.down['N']=frame.down['W']=frame.down['3']=frame.down[VK_RBUTTON]=false; step();
    const auto clickAt=[&](LONG x,LONG y) {
      frame.client={x,y}; frame.screen=frame.client; frame.down[VK_LBUTTON]=true; step();
      frame.down[VK_LBUTTON]=false; step();
    };
    clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::WeaponTraining) && game.State().TalentPoints()==1,
      "Build Learn button must reach the simulation command");
    clickAt(248,406); clickAt(1000,563);
    Require(game.State().HasTalent(rpg::Talent::WeaponMastery) && game.State().TalentPoints()==0,
      "Selecting a connected rank must learn that rank rather than the previous selection");
    frame.down['N']=frame.down['W']=true; step();
    Require(game.State().Player().z>position.z,"A fresh N press must close the menu and restore movement");
  }
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
  const auto loadingRoot=std::filesystem::current_path()/"loading-check";
  std::filesystem::create_directories(loadingRoot);
  const auto textureBytes=renderer.InterfaceBytes();
  for(const auto size:{std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Require(renderer.Resize(size.first,size.second),"Loading layout resize");
    for(int stage=0;stage<4;++stage) {
      renderer.BeginFrame();ui::RenderLoading(renderer,ui::LoadingStage(stage));
      Require(renderer.Capture((loadingRoot/(std::to_string(size.first)+"-"+std::to_string(stage)+".png")).wstring()) &&
        renderer.EndFrame(false),"Loading stage capture failed");
    }
  }
  Require(renderer.InterfaceBytes()==textureBytes,"Loading UI allocated extra art textures");
  Require(renderer.InterfaceBytes()<20*1024*1024,"Map and repeated UI draws exceed the combined texture budget.");
  Require(renderer.ValidateDebugMessages(),"Combat HUD emitted a DirectX warning/error");
  std::cout<<"PASS: live menus, item actions, persistence, scaled pointer routing and UI rendering.\n";
}
