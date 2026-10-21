#include "game/Game.h"
#include "engine/Renderer.h"
#include "platform/Win32Window.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>

namespace {
using Selection=ui::CharacterSelection;
using Control=Selection::Control;
void Check(bool result,const char* message) {if(!result) {std::cerr<<message<<'\n';std::exit(1);}}
std::string Read(const std::filesystem::path& path) {std::ifstream input(path,std::ios::binary);std::ostringstream out;out<<input.rdbuf();return out.str();}
void Write(const std::filesystem::path& path,const std::string& content) {std::ofstream out(path,std::ios::binary);out<<content;out.close();Check(bool(out),"Test fixture write failed");}
struct Driver {
  Input input;Input::Frame frame;
  Driver() {frame.focused=true;frame.client={0,0};input.Update(frame);}
  void Step(Game& game) {input.Update(frame);game.Update(1./60,input,0);}
  void Key(Game& game,int key) {frame.down[key]=true;Step(game);frame.down[key]=false;Step(game);}
  void Point(Control control) {
    const ui::Layout layout(float(frame.width),float(frame.height));const auto rect=Selection::Bounds(control);
    frame.client={LONG(layout.x+(rect.x+rect.w/2)*layout.scale),LONG(layout.y+(rect.y+rect.h/2)*layout.scale)};
  }
  void Click(Game& game,Control control) {Point(control);frame.down[VK_LBUTTON]=true;Step(game);frame.down[VK_LBUTTON]=false;Step(game);}
};
void Flow(const std::filesystem::path& root,const std::string& veteran) {
  const auto file=root/"current.save";Write(file,veteran);
  Game game;game.Initialize();const auto untouched=game.State().Save();game.ChooseCharacter(file);
  Check(game.ChoosingCharacter() && game.Selection().State()==Selection::Status::Saved,"Saved hero was not offered");
  Check(game.Selection().Hero().Level()==3 && std::string(game.Selection().Hero().Nickname())=="Frontier guard" &&
    std::string(game.Selection().Hero().ClassName())=="Dawnwatch guard","Incorrect nickname/level/class");
  Driver d;d.Step(game);
  for(int key:std::array<int,7>{'W','3',VK_F5,VK_F9,'C','N',VK_RBUTTON}) d.frame.down[key]=true;
  for(int i=0;i<120;++i) d.Step(game);
  Check(game.ChoosingCharacter() && game.State().Save()==untouched && Read(file)==veteran && !game.WantsMouseLook(),"World/save/input leaked through selection");
  d.frame.down.fill(false);d.Step(game);
  d.Key(game,VK_TAB);Check(game.Selection().Focus()==Control::Refresh,"Tab focus order failed");
  d.frame.down[VK_SHIFT]=true;d.Key(game,VK_TAB);d.frame.down[VK_SHIFT]=false;d.Step(game);
  Check(game.Selection().Focus()==Control::Enter,"Reverse Tab focus order failed");
  d.Key(game,VK_RIGHT);Check(game.Selection().Yaw()>0,"Preview did not rotate");
  d.Point(Control::Enter);d.frame.down[VK_LBUTTON]=true;d.Step(game);
  d.Point(Control::Quit);d.frame.down[VK_LBUTTON]=false;d.Step(game);
  Check(game.ChoosingCharacter() && !game.QuitRequested(),"Release on another control activated it");
  d.Point(Control::Enter);d.frame.down[VK_LBUTTON]=true;d.Step(game);
  d.frame.width=960;d.frame.height=600;d.Point(Control::Enter);d.Step(game);
  d.frame.down[VK_LBUTTON]=false;d.Step(game);
  Check(game.ChoosingCharacter(),"Resize committed a stale press");
  d.frame.down[VK_LBUTTON]=true;d.Step(game);d.frame.focused=false;d.Step(game);
  d.frame.focused=true;d.frame.down[VK_LBUTTON]=false;d.Step(game);
  Check(game.ChoosingCharacter(),"Focus loss committed a stale press");
  // The view changes if a different valid save appears after it was opened.
  Write(file,rpg::Simulation{}.Save());d.Click(game,Control::Enter);
  Check(game.ChoosingCharacter() && game.Selection().Changed() && game.Selection().Hero().Level()==1,"Changed save entered without review");
  Write(file,veteran);d.Key(game,'R');
  d.frame.down['W']=true;d.Click(game,Control::Enter);
  rpg::Simulation expected;Check(expected.Load(veteran),"Invalid veteran fixture");
  Check(!game.ChoosingCharacter() && game.State().Save()==expected.Save(),"Selected hero did not restore exact campaign state");
  for(int i=0;i<5;++i) d.Step(game);
  Check(game.State().Save()==expected.Save(),"Held selection input leaked into world");
  d.frame.down.fill(false);d.Step(game);d.Key(game,'W');
  Check(game.State().Save()!=expected.Save(),"World did not respond after releasing entry input");
  Check(Read(file)==veteran,"Selecting a hero modified the save file");

  const auto missing=root/("first-"+std::to_string(GetTickCount64())+".save");
  Game fresh;fresh.Initialize();fresh.ChooseCharacter(missing);Driver n;n.Step(fresh);
  Check(fresh.Selection().State()==Selection::Status::New && fresh.Selection().Hero().Level()==1,"First-run hero not available");
  n.Key(fresh,VK_RETURN);Check(!fresh.ChoosingCharacter() && !std::filesystem::exists(missing),"First entry should not write a save");

  for(const auto& invalid:{std::string{},std::string("broken save"),std::string(4097,'X')}) {
    Write(file,invalid);game.Initialize();game.ChooseCharacter(file);Driver bad;bad.Step(game);
    Check(game.Selection().State()==Selection::Status::Unreadable && !game.Selection().Enabled(Control::Enter),"Bad save offered a playable hero");
    bad.Click(game,Control::Enter);bad.Key(game,VK_RETURN);
    Check(game.ChoosingCharacter() && Read(file)==invalid,"Bad save was replaced/entered");
    Write(file,veteran);bad.Key(game,'R');Check(game.Selection().State()==Selection::Status::Saved,"Refresh did not recover repaired save");
    bad.Key(game,VK_ESCAPE);Check(!game.QuitRequested(),"Escape should focus Exit before confirming");
    bad.Key(game,VK_RETURN);Check(game.QuitRequested(),"Keyboard exit failed");
  }
  game.Initialize();game.ChooseCharacter(root);Check(game.Selection().State()==Selection::Status::Unreadable,"Directory treated as a missing save");
}
void Render(const std::filesystem::path& root,const std::string& veteran) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Character selection test",false),"Hidden window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer initialization");
  const auto file=root/"render.save";Write(file,veteran);
  Game game;game.Initialize();game.ChooseCharacter(file);
  for(const auto size:{std::pair<int,int>{960,600},{1280,720},{1920,1080},{2560,1080}}) {
    Check(renderer.Resize(size.first,size.second),"Selection resize");
    renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/("selection-"+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Selection render/capture");
  }
  Check(renderer.Resize(1280,720),"Reset render size");
  Write(file,"invalid");game.ChooseCharacter(file);renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/"selection-error.png").wstring()) && renderer.EndFrame(false),"Error-state render");
  game.ChooseCharacter(root/("empty-"+std::to_string(GetTickCount64())+".save"));renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/"selection-first.png").wstring()) && renderer.EndFrame(false),"First-character render");
  Check(renderer.ValidateDebugMessages(),"D3D validation failed");
}
}
int main() {
  const auto root=std::filesystem::current_path()/"character-selection-check";std::filesystem::create_directories(root);
  const auto veteran=Read(std::filesystem::path(RPG_TEST_FIXTURES)/"veteran-preview.save");
  Flow(root,veteran);Render(root,veteran);
  std::cout<<"PASS: character metadata, frozen world, input isolation, saved/new/error/repaired states, changed saves, resize/focus, hidden DX11 captures\n";
}
