#include "game/Game.h"
#include "engine/Renderer.h"
#include "game/CharacterRig.h"
#include "game/ProfileStorage.h"
#include <atomic>
#include <thread>
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
void ReportedRegressions(const std::filesystem::path& root) {
  const auto folder=root/("reported-bugs-"+std::to_string(GetTickCount64()));std::filesystem::create_directories(folder);
  int failures=0;const auto expect=[&](bool value,const char* message) {if(!value) {std::cerr<<"REGRESSION: "<<message<<'\n';++failures;}};
  Game draft;draft.Initialize();draft.ChooseCharacter(folder/"draft.save");Driver d;d.Step(draft);
  d.Click(draft,Control::Create);d.Click(draft,Control::Next);d.Key(draft,'A');
  d.frame.down[VK_CONTROL]=true;d.Key(draft,'V');d.frame.down[VK_CONTROL]=false;d.Step(draft);
  expect(draft.Selection().DraftName()=="a","Ctrl+V inserts shortcut letters in the hero name");
  d.frame.down[VK_CONTROL]=true;d.Key(draft,'A');d.frame.down[VK_CONTROL]=false;d.Key(draft,'A');d.Key(draft,VK_SPACE);
  expect(std::string(draft.Selection().Hero().Nickname())=="a","Typing a word separator resets the preview name");
  const auto file=folder/"patrol.save";rpg::Simulation original;original.CreateCharacter(rpg::ClassId::Warrior,"First Hero");Write(file,original.Save());
  Game first,second;first.Initialize();second.Initialize();first.ChooseCharacter(file);second.ChooseCharacter(file);
  Driver one,two;one.Step(first);two.Step(second);one.Click(first,Control::Enter);two.Click(second,Control::Enter);one.Step(first);two.Step(second);
  one.Key(first,'W');one.Key(first,VK_F5);const auto latest=Read(file);
  two.Key(second,VK_F5);
  expect(Read(file)==latest,"An older running game overwrites newer saved progress");
  expect(second.State().Message().find("Save changed")!=std::string::npos,"A stale write did not explain why it was rejected");
  two.Key(second,VK_F9);two.Key(second,'W');two.Key(second,VK_F5);
  rpg::Simulation reloaded;
  expect(second.State().Message()=="Progress saved." && Read(file)!=latest && reloaded.Load(Read(file)),"F9 did not refresh the save baseline for a later F5");
  rpg::Simulation replacement;replacement.CreateCharacter(rpg::ClassId::Mage,"Another Hero");Write(file,replacement.Save());
  one.Key(first,VK_F5);
  expect(Read(file)==replacement.Save(),"F5 replaces an externally changed character profile");
  d.Key(draft,VK_BACK); // remove the trailing separator
  for(int modifier:{VK_CONTROL,VK_MENU}) {
    d.frame.down[modifier]=true;
    for(int key:std::array<int,8>{'C','X','V','Z',VK_SPACE,VK_BACK,VK_OEM_MINUS,VK_OEM_7}) d.Key(draft,key);
    d.frame.down[modifier]=false;d.Step(draft);
  }
  expect(draft.Selection().DraftName()=="a","Control/Alt shortcuts modified the name");
  d.frame.capsLock=true;d.Key(draft,'B');d.frame.down[VK_SHIFT]=true;d.Key(draft,'C');d.frame.down[VK_SHIFT]=false;d.Step(draft);
  expect(draft.Selection().DraftName()=="aBc","Caps Lock and Shift did not compose correctly");
  // Recover a failed creation without leaving the editor or losing appearance.
  const auto blocked=folder/"blocked";Game retry;retry.Initialize();retry.ChooseCharacter(blocked/"patrol.save");Driver r;r.Step(retry);
  r.Click(retry,Control::Create);r.Click(retry,Control::Next);r.Key(retry,'R');r.Key(retry,VK_RETURN);r.Click(retry,Control::SkinRight);
  Write(blocked,"temporary obstacle");r.Click(retry,Control::Confirm);
  expect(retry.Selection().Enabled(Control::Confirm),"A transient save failure disabled retry permanently");
  std::filesystem::rename(blocked,folder/"blocked-preserved.txt");r.Click(retry,Control::Confirm);
  rpg::Simulation created;
  expect(created.Load(Read(blocked/"patrol.save")) && std::string(created.Nickname())=="r" && created.Look().skin==1,"Retry failed to preserve and save the creation draft");
  Check(failures==0,"Reported regressions failed");
}
void StorageTransactions(const std::filesystem::path& root) {
  using R=profile::ReadResult;using W=profile::WriteResult;
  const auto folder=root/("storage-transactions-"+std::to_string(GetTickCount64()));std::filesystem::create_directories(folder);
  const auto file=folder/"patrol.save";std::string content="untouched";
  Check(profile::Read(file,content)==R::Missing && content=="untouched","Missing read changed output");
  rpg::Simulation hero;hero.CreateCharacter(rpg::ClassId::Warrior,"First");const auto first=hero.Save();
  hero.CreateCharacter(rpg::ClassId::Warrior,"Second");const auto second=hero.Save();
  hero.CreateCharacter(rpg::ClassId::Warrior,"Third");const auto third=hero.Save();
  Check(profile::Write(file,first)==W::Saved && profile::Write(file,second)==W::Conflict && Read(file)==first,"Create-only write overwrote a profile");
  const auto oldTemporary=folder/"patrol.save.tmp";Write(oldTemporary,"Unrelated existing file");
  Check(profile::Write(file,second,&first)==W::Saved && Read(oldTemporary)=="Unrelated existing file","Save reused an unowned temporary path");
  Check(profile::Write(file,third,&first)==W::Conflict && Read(file)==second,"Stale compare-and-write succeeded");
  Check(profile::Write(file,third,&second)==W::Saved && profile::Read(file,content)==R::Ready && content==third,"Valid replacement did not publish a complete snapshot");
  for(const auto invalid:{std::string{},std::string(4097,'X')}) {
    Write(file,invalid);content="untouched";
    Check(profile::Read(file,content)==R::Error && content=="untouched","Unbounded/empty read accepted");
    Check(profile::Write(file,first,&third)==W::Error && Read(file)==invalid,"Unreadable disk save replaced");
  }
  const auto removed=folder/"removed.save";
  Check(profile::Write(removed,second,&first)==W::Conflict && !std::filesystem::exists(removed),"Missing expected save was silently recreated");
  // Two writers see the same original snapshot. Exactly one may publish; the
  // other is busy or observes the new baseline. Dot-path aliases must share a lock.
  for(int iteration=0;iteration<8;++iteration) {
    Write(file,first);std::atomic<int> ready{0};std::atomic<bool> start{false};W outcomes[2]={W::Error,W::Error};
    const auto write=[&](int id) {
      ready.fetch_add(1);while(!start.load()) std::this_thread::yield();
      outcomes[id]=profile::Write(id?folder/"."/"patrol.save":file,id?second:third,&first);
    };
    std::thread left(write,0),right(write,1);while(ready.load()!=2) std::this_thread::yield();start.store(true);left.join();right.join();
    Check(int(outcomes[0]==W::Saved)+int(outcomes[1]==W::Saved)==1,"Concurrent writers both published or both failed");
    const int loser=outcomes[0]==W::Saved?1:0;
    Check(outcomes[loser]==W::Busy || outcomes[loser]==W::Conflict,"Concurrent writer returned an unexpected failure");
    Check(Read(file)==(outcomes[0]==W::Saved?third:second),"Concurrent write damaged the winning snapshot");
  }
  for(const auto& entry:std::filesystem::directory_iterator(folder))
    Check(entry.path().filename().wstring().find(L".write-")==std::wstring::npos,"Save left its own temporary file behind");
}
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
  Check(!fresh.Selection().Enabled(Control::Enter),"Missing profile offered world entry");
  n.Key(fresh,VK_RETURN);Check(fresh.Selection().Screen()==Selection::Page::Class,"First-run Create did not open class selection");
  n.Click(fresh,Control::Next);Check(fresh.Selection().Screen()==Selection::Page::Customize,"Appearance screen missing");
  Check(!fresh.Selection().Enabled(Control::Confirm),"Empty name enabled creation");
  n.Key(fresh,'A');n.Click(fresh,Control::SkinRight);n.Key(fresh,VK_ESCAPE);
  Check(fresh.Selection().Screen()==Selection::Page::Class,"Escape did not return to class choice");
  n.Click(fresh,Control::Cancel);Check(fresh.Selection().Screen()==Selection::Page::Roster && !std::filesystem::exists(missing),"Cancel wrote a character");

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
  {
    const auto folder=root/("return-roster-"+std::to_string(GetTickCount64()));
    std::filesystem::create_directories(folder);
    const auto base=folder/"patrol.save",mageFile=folder/("hero-"+std::to_string(int(rpg::ClassId::Mage))+".save");
    rpg::Simulation hero;hero.CreateCharacter(rpg::ClassId::Warrior,"Guard");Write(base,hero.Save());
    hero.CreateCharacter(rpg::ClassId::Mage,"Scholar");Write(mageFile,hero.Save());
    Game returning;returning.Initialize();returning.ChooseCharacter(base);Driver driver;driver.Step(returning);
    const auto mageControl=Control(int(Control::Class0)+int(rpg::ClassId::Mage));
    driver.Click(returning,mageControl);driver.Click(returning,Control::Enter);driver.Step(returning);
    Check(!returning.ChoosingCharacter() && returning.State().Class()==rpg::ClassId::Mage,"Mage entry failed");
    driver.Key(returning,'W');const auto expected=returning.State().Save();
    const auto returnClick=[&]() {
      returning.OpenPanel(ui::Panel::Menu);renderer.BeginFrame();returning.Render(renderer);
      Check(renderer.Capture((root/"return-to-roster-menu.png").wstring()) && renderer.EndFrame(false),"Return menu render");
      const ui::Layout layout(float(driver.frame.width),float(driver.frame.height));
      driver.frame.client={LONG(layout.x+640*layout.scale),LONG(layout.y+522*layout.scale)};
      driver.frame.down[VK_LBUTTON]=true;driver.Step(returning);
    };
    const auto generation=returning.AudioGeneration();returnClick();
    Check(returning.ChoosingCharacter() && !returning.QuitRequested() && !returning.WantsMouseLook(),"Return failed to release gameplay");
    Check(Read(mageFile)==expected && returning.AudioGeneration()!=generation,"Return did not save/reset audio generation");
    const auto frozen=returning.State().Save();driver.frame.down[VK_RETURN]=true;
    for(int i=0;i<5;++i) driver.Step(returning);
    Check(returning.ChoosingCharacter() && returning.State().Save()==frozen,"Held input entered world or advanced simulation");
    driver.frame.down[VK_RETURN]=driver.frame.down[VK_LBUTTON]=false;driver.Step(returning);driver.Step(returning);
    Check(returning.Selection().Screen()==Selection::Page::Roster && std::string(returning.Selection().Hero().Nickname())=="Scholar","Returning to roster lost the active hero selection");
    driver.Click(returning,Control::Class0);
    Check(std::string(returning.Selection().Hero().Nickname())=="Guard","Return used class save as roster base");
    driver.Click(returning,mageControl);driver.Click(returning,Control::Enter);driver.Step(returning);
    Check(returning.State().Save()==expected,"Saved hero failed to re-enter unchanged");
    hero.CreateCharacter(rpg::ClassId::Mage,"External");Write(mageFile,hero.Save());
    returnClick();
    Check(!returning.ChoosingCharacter() && Read(mageFile)==hero.Save() && returning.State().Message().find("Save changed")!=std::string::npos,"Return discarded progress or overwrote external save");
    driver.frame.down[VK_LBUTTON]=false;driver.Step(returning);driver.Key(returning,VK_F9);
    returnClick();driver.frame.down[VK_LBUTTON]=false;driver.Step(returning);driver.Step(returning);
    driver.Click(returning,Control::Create);
    Check(returning.Selection().Screen()==Selection::Page::Class,"Creation unavailable after returning to roster");
  }
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
  Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control::Class5);
  const auto frozenWorld=game.State().Save();
  for(int classIndex=0;classIndex<int(rpg::ClassId::Count);++classIndex) {
    d.Click(game,Control(int(Control::Class0)+classIndex));
    const auto untouched=game.Selection().Hero().Save();
    d.Click(game,Control::Arts);
    Check(game.Selection().CallingScreen()==Selection::CallingView::Arts && !game.Selection().Enabled(Control::Branch0),"Art view exposes hidden branch controls");
    for(int art=0;art<7;++art) {
      d.Click(game,Control(int(Control::Art0)+art));
      Check(game.Selection().PreviewAbility()==Selection::previewArts[art] && game.Selection().PreviewAction()>=0,"Art click previews wrong ability");
      // Slow caster signature clips can outlast four seconds at their class rate.
      for(int step=0;step<900 && game.Selection().PreviewAction()>=0;++step) d.Step(game);
      Check(game.Selection().PreviewAction()<0,"Art preview never settles");
      Check(game.Selection().Hero().Save()==untouched,"Art preview mutates hero");
      Check(game.State().Save()==frozenWorld,"Art preview mutates world");
    }
    d.Click(game,Control::Talents);
    Check(!game.Selection().Enabled(Control::Art0),"Hidden art remains interactive");
    for(int branch=0;branch<3;++branch) {
      d.Click(game,Control(int(Control::Branch0)+branch));
      Check(game.Selection().CallingBranch()==branch,"Talent path selection failed");
    }
  }
  d.Click(game,Control::Class5);
  for(const auto size:{std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    d.frame.width=size.first;d.frame.height=size.second;d.Step(game);
    Check(renderer.Resize(size.first,size.second),"Calling view resize");
    for(int view=0;view<3;++view) {
      d.Click(game,Control(int(Control::Overview)+view));
      if(view==1) d.Click(game,Control::Art5);
      if(view==2) d.Click(game,Control::Branch1);
      renderer.BeginFrame();game.Render(renderer);
      Check(renderer.Capture((root/("calling-"+std::to_string(view)+"-"+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Calling view capture");
    }
  }
  d.frame.width=1280;d.frame.height=720;d.Step(game);d.Click(game,Control::Overview);d.Click(game,Control::ResetView);
  for(int stage=0;stage<2;++stage) {
    if(stage) {d.Click(game,Control::Next);d.frame.down[VK_SHIFT]=true;d.Key(game,'E');d.frame.down[VK_SHIFT]=false;
      for(int key:{'L','A','R','A'}) d.Key(game,key);d.Key(game,VK_RETURN);d.Click(game,Control::HairRight);d.Click(game,Control::DyeRight);}
    for(const auto size:{std::pair<int,int>{960,600},{1280,720},{1920,1080},{2560,1080}}) {
      Check(renderer.Resize(size.first,size.second),"Creation resize");renderer.BeginFrame();game.Render(renderer);
      Check(renderer.Capture((root/(std::string(stage?"creation-appearance-":"creation-class-")+std::to_string(size.first)+".png")).wstring()) && renderer.EndFrame(false),"Creation render/capture");
    }
  }
  Check(renderer.Resize(1280,720),"Appearance palette resize");
  for(int look=0;look<rpg::Appearance::count;++look) {
    renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/("creation-palette-"+std::to_string(look)+".png")).wstring()) && renderer.EndFrame(false),"Appearance palette capture");
    d.Click(game,Control::SkinRight);d.Click(game,Control::HairRight);d.Click(game,Control::DyeRight);
  }
  d.Click(game,Control::Name);d.frame.down[VK_CONTROL]=true;d.Key(game,'A');d.frame.down[VK_CONTROL]=false;
  d.frame.down[VK_SHIFT]=true;for(int i=0;i<21;++i) d.Key(game,'W');d.frame.down[VK_SHIFT]=false;d.Step(game);
  Check(game.Selection().DraftName()==std::string(20,'W'),"Name input did not enforce maximum length");
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/"creation-long-name.png").wstring()) && renderer.EndFrame(false),"Long-name render");
  const auto draftName=game.Selection().DraftName();
  for(int n=0;n<5;++n) {
    d.Click(game,Control(int(Control::Skin0)+n));
    d.Click(game,Control(int(Control::Hair0)+n));
    d.Click(game,Control(int(Control::Dye0)+n));
    Check(game.Selection().Hero().Look().skin==n && game.Selection().Hero().Look().hair==n &&
      game.Selection().Hero().Look().dye==n,"Direct swatches do not select the displayed palette");
  }
  const auto draft=game.Selection().Hero().Save();
  d.Click(game,Control::Closeup);
  for(int n=0;n<90;++n) d.Step(game);
  Check(game.Selection().PreviewZoom()>.99f,"Face inspection did not converge");
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/"creation-face-closeup.png").wstring()) && renderer.EndFrame(false),"Face closeup capture");
  d.Click(game,Control::Pose);
  Check(!game.Selection().Closeup() && game.Selection().PreviewAction()>=0,"Action preview failed to return to full figure");
  for(int n=0;n<12;++n) d.Step(game);
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/"creation-action.png").wstring()) && renderer.EndFrame(false),"Action preview capture");
  Check(game.Selection().PreviewAbility()==0,"First preview must use basic attack");
  d.Click(game,Control::Pose);
  Check(game.Selection().PreviewAbility()==1,"Second preview must use heavy attack");
  d.Click(game,Control::Pose);
  Check(game.Selection().PreviewAbility()==5,"Third preview must use class technique, not potion");
  const float previewDuration=animation::ActionDuration(animation::ActionClip::Technique)/(rpg::ClassAttackRate(game.Selection().Hero().Class(),true)*.8f);
  for(int n=0;n<int(60*previewDuration)+12;++n) d.Step(game);
  Check(game.Selection().PreviewAction()<0,"Technique preview never settles");
  d.Click(game,Control::Pose);
  Check(game.Selection().PreviewAbility()==6,"Signature missing from preview cycle");
  d.Click(game,Control::Pose);
  Check(game.Selection().PreviewAbility()==7,"Setup attack missing from preview cycle");
  d.Click(game,Control::Pose);
  Check(game.Selection().PreviewAbility()==0,"Preview cycle does not wrap");
  d.Click(game,Control::Right);d.Click(game,Control::ResetView);
  Check(game.Selection().Yaw()==0 && game.Selection().PreviewAction()<0 && !game.Selection().Closeup() &&
    game.Selection().Hero().Save()==draft && game.Selection().DraftName()==draftName,"Preview controls changed the draft or failed reset");
  d.Click(game,Control::Name);
  d.frame.down[VK_CONTROL]=true;d.Key(game,'A');d.frame.down[VK_CONTROL]=false;d.Step(game);
  Check(game.Selection().DraftName().empty() && !game.Selection().Enabled(Control::Confirm),"Ctrl+A did not clear and disable creation");
  d.Key(game,VK_OEM_MINUS);Check(!game.Selection().Enabled(Control::Confirm),"Punctuation-only name was accepted");
  d.Key(game,VK_BACK);d.Key(game,'X');d.Key(game,VK_SPACE);d.Key(game,VK_RETURN);
  Check(game.Selection().DraftName()=="x" && game.Selection().Focus()==Control::Confirm,"Finish editing did not trim name/focus confirmation");
  Check(renderer.ValidateDebugMessages(),"D3D validation failed");
}
void ClassSlots(const std::filesystem::path& root) {
  const auto folder=root/("roster-"+std::to_string(GetTickCount64()));std::filesystem::create_directories(folder);
  const auto base=folder/"patrol.save";
  for(int i=0;i<int(rpg::ClassId::Count);++i) {
    Game game;game.Initialize();game.ChooseCharacter(base);Driver d;d.Step(game);
    d.Click(game,Control(int(Control::Class0)+i));
    Check(game.Selection().Hero().Class()==rpg::ClassId(i) && game.Selection().State()==Selection::Status::New,"Class choice did not create a fresh profile");
    d.Click(game,Control::Create);d.Click(game,Control::Next);
    d.Click(game,Control::Name);d.Key(game,'A');d.Key(game,'S');d.Key(game,VK_RETURN);
    Check(game.Selection().DraftName()=="as","Name field did not accept keyboard text");
    d.Click(game,Control::SkinRight);d.Click(game,Control::HairRight);d.Click(game,Control::DyeLeft);
    d.Click(game,Control::Confirm);
    Check(game.ChoosingCharacter() && game.Selection().Screen()==Selection::Page::Roster && game.Selection().State()==Selection::Status::Saved,"Confirm did not persist and return to roster");
    Check(game.Selection().Hero().Look().skin==1 && game.Selection().Hero().Look().hair==1 && game.Selection().Hero().Look().dye==4,"Creation lost appearance");
    d.Click(game,Control::Enter);d.Step(game);
    Check(game.State().Look().skin==1 && game.State().Look().hair==1 && game.State().Look().dye==4,"World entry lost appearance");
    Check(!game.ChoosingCharacter() && game.State().Class()==rpg::ClassId(i) && std::string(game.State().Nickname())=="as","Created character identity lost at entry");
    d.Key(game,VK_F5);
    const auto file=i==0?base:folder/("hero-"+std::to_string(i)+".save");
    rpg::Simulation saved;Check(saved.Load(Read(file)) && saved.Class()==rpg::ClassId(i),"F5 wrote the wrong class slot");
    rpg::Simulation other;other.CreateCharacter(rpg::ClassId((i+1)%int(rpg::ClassId::Count)),"Other Hero");
    Write(file,other.Save());d.Key(game,VK_F9);
    Check(game.State().Class()==rpg::ClassId(i) && std::string(game.State().Nickname())=="as","F9 accepted a different class into this slot");
    Write(file,saved.Save());d.Key(game,VK_F9);Check(game.State().Save()==saved.Save(),"Same-class F9 restore failed");
    game.ChooseCharacter(base);d.Step(game);d.Click(game,Control(int(Control::Class0)+i));
    Check(game.Selection().State()==Selection::Status::Saved && !game.Selection().Enabled(Control::Name),"Existing character was editable/replaced");
  }
  Game full;full.Initialize();full.ChooseCharacter(base);
  Check(!full.Selection().Enabled(Control::Create),"Full roster allowed new creation");
}
void CreationSafety(const std::filesystem::path& root) {
  const auto folder=root/("creation-safety-"+std::to_string(GetTickCount64()));std::filesystem::create_directories(folder);
  const auto file=folder/"patrol.save";
  Game game;game.Initialize();game.ChooseCharacter(file);Driver d;d.Step(game);
  d.Click(game,Control::Create);d.Click(game,Control::Next);d.Key(game,'A');d.Key(game,VK_RETURN);
  // Another process wins the slot after the preview was opened.
  rpg::Simulation other;other.CreateCharacter(rpg::ClassId::Warrior,"Existing Hero");const auto stable=other.Save();Write(file,stable);
  d.Click(game,Control::Confirm);
  Check(Read(file)==stable && game.Selection().Screen()==Selection::Page::Customize && !game.Selection().Enabled(Control::Confirm) && !game.Selection().Notice().empty(),"Creation replaced an externally created hero");
  d.Click(game,Control::Back);Check(!game.Selection().Enabled(Control::Next),"Occupied class allowed customization");
  d.Click(game,Control::Cancel);Check(game.Selection().State()==Selection::Status::Saved,"Cancel failed to restore roster state");
  for(const auto& entry:std::filesystem::directory_iterator(folder)) Check(entry.path().extension()==".save","Creation left a temporary file");
  // Folder becomes unwritable (a regular file) after the creation form opens.
  const auto blocked=folder/"blocked";game.ChooseCharacter(blocked/"patrol.save");d.Step(game);
  d.Click(game,Control::Create);d.Click(game,Control::Next);d.Key(game,'B');d.Key(game,VK_RETURN);Write(blocked,"keep");
  d.Click(game,Control::Confirm);
  Check(Read(blocked)=="keep" && game.Selection().Screen()==Selection::Page::Customize && game.Selection().DraftName()=="b" && !game.Selection().Notice().empty(),"Save error lost the draft or overwrote a file");
  // Page transition while a button is held must not deliver its release to the next page.
  game.ChooseCharacter(folder/"new.save");d.Step(game);d.Point(Control::Create);d.frame.down[VK_LBUTTON]=true;d.Step(game);
  d.Key(game,VK_RETURN);d.Point(Control::Next);d.frame.down[VK_LBUTTON]=false;d.Step(game);
  Check(game.Selection().Screen()==Selection::Page::Class,"Stale mouse release advanced the creation wizard");
}
void AppearancePersistence() {
  for(int index=0;index<rpg::Appearance::count;++index) {
    rpg::Simulation s;Check(s.CreateCharacter(rpg::ClassId::Mage,"Dawn's Watch",{index,4-index,index}),"Valid appearance rejected");
    const auto saved=s.Save();rpg::Simulation copy;Check(copy.Load(saved) && copy.Save()==saved,"Appearance round-trip");
    rpg::Command reset;reset.restart=true;copy.Update(.01f,reset);Check(copy.Look().skin==index && copy.Look().hair==4-index && copy.Look().dye==index,"Restart lost appearance");
    const auto at=saved.find("APPEARANCE ");
    for(const auto* record:{"APPEARANCE -1 0 0\n","APPEARANCE 0 5 0\n","APPEARANCE 0 0 5\n","APPEARANCE 0 0\n","APPEARANCE 0 0 0 1\n",""}) {
      auto bad=saved.substr(0,at)+record;Check(!s.Load(bad) && s.Save()==saved,"Invalid appearance changed live profile");
    }
    auto legacy=saved.substr(0,at);legacy.replace(14,legacy.find('\n')-14,"11");
    Check(copy.Load(legacy) && copy.Class()==rpg::ClassId::Mage && copy.Look().skin==0 && copy.Look().hair==0 && copy.Look().dye==0,"Version 11 appearance migration");
    Check(!s.CreateCharacter(rpg::ClassId::Mage,"Test",{0,0,99}) && s.Save()==saved,"Invalid creation appearance mutated state");
    const auto outfit=CharacterOutfit::From(s.Items(),s.Class(),s.Look());
    const auto palette=BuildCharacterRig(BuildCharacterPose(0,0,false,animation::Idle(0)),outfit,{.1f,.2f,.3f,1});
    Check(palette.colors[5].x==rpg::skinTones[index].r,"Skin palette did not reach the continuous mesh");
    if(index) Check(palette.colors[0].x==rpg::tunicDyes[index].r,"Tunic dye did not reach the continuous mesh");
  }
}
}
int main() {
  const auto root=std::filesystem::current_path()/"character-selection-check";std::filesystem::create_directories(root);
  const auto veteran=Read(std::filesystem::path(RPG_TEST_FIXTURES)/"veteran-preview.save");
  ReportedRegressions(root);StorageTransactions(root);Flow(root,veteran);ClassSlots(root);CreationSafety(root);AppearancePersistence();Render(root,veteran);
  std::cout<<"PASS: character metadata, frozen world, input isolation, saved/new/error/repaired states, changed saves, resize/focus, hidden DX11 captures\n";
}
