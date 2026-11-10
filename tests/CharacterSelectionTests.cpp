#include "game/Game.h"
#include "game/Localization.h"
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
  for(const auto invalid:{std::string{},std::string(16385,'X')}) {
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

  for(const auto& invalid:{std::string{},std::string("broken save"),std::string(16385,'X')}) {
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
void Entrance(const std::filesystem::path& root,Renderer* renderer=nullptr) {
  for(const auto id:{rpg::ClassId::Warrior,rpg::ClassId::Scout,rpg::ClassId::Mage,rpg::ClassId::Priest})for(int hz:{30,60,144}) {
    auto owner=std::make_unique<Game>();auto& game=*owner;game.Initialize();
    game.ChooseCharacter(root/("entrance-"+std::to_string(int(id))+"-"+std::to_string(hz)+"-"+std::to_string(GetTickCount64()))/"patrol.save");
    Driver d;d.Step(game);d.Click(game,Control(int(Control::Class0)+int(id)));
    for(int n=0;n<60;++n)d.Step(game);
    Check(game.Selection().PreviewAction()<0,"Roster automatically performed a combat action");
    d.Click(game,Control::Create);const auto saved=game.Selection().Hero().Save(),world=game.State().Save();
    const auto step=[&]{d.input.Update(d.frame);game.Update(1./hz,d.input,0);};
    for(int n=0;n<int(.40f*hz);++n)step();
    Check(game.Selection().PreviewAction()<0,"Entrance starts before the class page has settled");
    int starts=0,releases=0;bool previous=false,captured=false;
    const int ability=id==rpg::ClassId::Warrior || id==rpg::ClassId::Scout?1:6;
    for(int n=0;n<hz*9;++n) {
      step();const auto& selection=game.Selection();
      if(selection.Introducing() && !previous)++starts;previous=selection.Introducing();
      if(previous)Check(selection.PreviewAbility()==ability,"Entrance used the wrong class action");
      for(unsigned k=0;k<selection.PreviewAudio().count;++k)
        releases+=selection.PreviewAudio().cues[k]==(id==rpg::ClassId::Warrior?sound::Cue::Heavy:sound::ReleaseCue(id,ability,true));
      if(renderer && hz==60 && !captured && selection.PreviewAction()>.20f) {
        renderer->BeginFrame();game.Render(*renderer);
        Check(renderer->Capture((root/("automatic-entrance-"+std::to_string(int(id))+".png")).wstring()) && renderer->EndFrame(false),"Entrance capture");captured=true;
      }
    }
    Check(starts==1 && releases==1 && !game.Selection().Showcasing(),"Entrance repeats, loses release audio or starts a loop");
    Check(saved==game.Selection().Hero().Save() && world==game.State().Save(),"Entrance altered profile or world");
    d.Click(game,Control::Pose);Check(game.Selection().PreviewAbility()==0,"Entrance changed the manual preview cycle");
    // Changing class replaces a pending entrance; focus/page/inspection wins.
    for(int cancel=0;cancel<5;++cancel) {
      d.Click(game,Control::Class0);d.Click(game,Control::Class5);
      if(cancel==0)game.SuspendInput();
      if(cancel==1)d.Click(game,Control::Closeup);
      if(cancel==2)d.Click(game,Control::Next);
      if(cancel==3){d.frame.width+=8;d.Step(game);}
      if(cancel==4){d.Point(Control::Pose);d.Click(game,Control::Pose);}
      for(int n=0;n<hz*2;++n){step();Check(!game.Selection().Introducing(),"Cancelled entrance restarted");}
      if(cancel==1)Check(game.Selection().Closeup(),"Entrance stole the close-up camera");
      if(cancel==2)d.Click(game,Control::Back);
    }
  }
}
void StageTransitions(const std::filesystem::path& root,Renderer& renderer) {
  for(int hz:{30,60,144}) {
    auto owned=std::make_unique<Game>();auto& game=*owned;game.Initialize();
    game.ChooseCharacter(root/("transition-"+std::to_string(hz)+"-"+std::to_string(GetTickCount64()))/"patrol.save");
    Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control::Class5);
    const auto profile=game.Selection().Hero().Save(),world=game.State().Save();float elapsed=0;
    const auto tick=[&](float until){while(elapsed<until-1e-6f){const float dt=std::min(1.f/hz,until-elapsed);d.input.Update(d.frame);game.Update(dt,d.input,0);elapsed+=dt;}};
    Check(game.Selection().SelectedClass()==rpg::ClassId::Mage && game.Selection().PreviewStage().school==rpg::ClassId::Warrior,
      "Class selection did not separate committed identity from outgoing presentation");
    for(float time:{.05f,.14f,.32f}) {
      tick(time);const auto view=game.Selection().PreviewStage();
      Check(view.school==(time<.1f?rpg::ClassId::Warrior:rpg::ClassId::Mage),"Dissolve changed school on the wrong phase");
      const float expected=time<.1f?1-combat::Smooth(time/.1f):combat::Smooth((time-.1f)/.22f);
      Check(std::abs(view.opacity-expected)<.0001f,"Creation dissolve varies with refresh rate");
      if(hz==60){renderer.BeginFrame();game.Render(renderer);
        Check(renderer.Capture((root/("stage-transition-"+std::to_string(int(time*1000))+".png")).wstring())&&renderer.EndFrame(false),"Stage dissolve native capture");}
      Check(game.Selection().Hero().Save()==profile && game.State().Save()==world,"Dissolve modified profile or gameplay");
    }
    d.Click(game,Control::Class3);for(int i=0;i<3;++i)d.Step(game);
    const auto before=game.Selection().PreviewStage();d.Click(game,Control::Class2);
    Check(game.Selection().PreviewStage().school==before.school && game.Selection().PreviewStage().opacity<=before.opacity,
      "Rapid class switching resurrected a fully opaque stale hero");
    d.Click(game,Control::Pose);Check(game.Selection().PreviewStage().opacity==1 && game.Selection().PreviewStage().school==rpg::ClassId::Scout,
      "Explicit art preview did not win over a pending dissolve");
    d.Click(game,Control::Class5);game.SuspendInput();Check(game.Selection().PreviewStage().opacity==1,"Suspension retained an unfinished dissolve");
    d.Click(game,Control::Class3);d.Click(game,Control::Next);
    Check(game.Selection().PreviewStage().opacity==1 && game.Selection().PreviewStage().school==rpg::ClassId::Priest,
      "Appearance page inherited another class's outgoing figure");
    d.Click(game,Control::Back);d.Click(game,Control::ResetView);
    const auto beforeInspection=game.Selection().Hero().Save();
    for(auto size:{std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
      d.frame.width=size.first;d.frame.height=size.second;d.Step(game);d.Point(Control::Class5);
      for(int n=0;n<32;++n)d.Step(game);
      Check(game.Selection().Hint()==Control::Class5 && game.Selection().SelectedClass()==rpg::ClassId::Priest &&
        game.Selection().Hero().Save()==beforeInspection,"Hover inspection selected or changed a different class");
      if(hz==60){Check(renderer.Resize(size.first,size.second),"Inspection resize");renderer.BeginFrame();game.Render(renderer);
        Check(renderer.Capture((root/("class-inspection-"+std::to_string(size.first)+".png")).wstring())&&renderer.EndFrame(false),"Class inspection native capture");}
    }
    d.frame.width=1280;d.frame.height=720;d.Step(game);Check(renderer.Resize(1280,720),"Restore stage capture size");
  }
}
void ArtTransport(const std::filesystem::path& root,Renderer& renderer) {
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Scout,rpg::ClassId::Mage,rpg::ClassId::Priest})for(int hz:{30,60,144}) {
    auto owned=std::make_unique<Game>();auto& game=*owned;game.Initialize();
    game.ChooseCharacter(root/("transport-"+std::to_string(int(id))+"-"+std::to_string(hz)+"-"+std::to_string(GetTickCount64()))/"patrol.save");
    Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control(int(Control::Class0)+int(id)));
    Check(!game.Selection().Enabled(Control::Replay),"Hidden replay remained interactive");
    d.Click(game,Control::Arts);d.Click(game,Control::Art5);
    const auto hero=game.Selection().Hero().Save(),world=game.State().Save();
    const auto tick=[&](float duration){while(duration>1e-6f){const float dt=std::min(1.f/hz,duration);d.input.Update(d.frame);game.Update(dt,d.input,0);duration-=dt;}};
    tick(.21f);const auto timing=game.Selection().PreviewTiming();
    Check(timing.progress>0 && timing.release<=timing.follow && timing.follow<=1,"Invalid rehearsal phase display");
    if(hz==60) {
      renderer.BeginFrame();game.Render(renderer);
      Check(renderer.Capture((root/("art-transport-"+std::to_string(int(id))+".png")).wstring())&&renderer.EndFrame(false),"Rehearsal native capture");
    }
    d.Click(game,Control::Stop);
    Check(game.Selection().PreviewAction()<0 && game.Selection().PreviewAbility()==6 && !game.Selection().Showcasing() &&
      game.Selection().PreviewEffectCount()==0 && !game.Selection().PreviewAudio().preparing,"Stop did not cancel rehearsal while retaining the selection");
    tick(.25f);const auto pose=game.Selection().PreviewPose();
    Check(std::abs(pose.left.x)+std::abs(pose.left.y)+std::abs(pose.left.z)+std::abs(pose.pitch)+std::abs(pose.yaw)<.0001f,"Stopped rehearsal did not settle");
    Check(!game.Selection().Enabled(Control::Stop),"Inactive stop stayed enabled");
    d.Click(game,Control::Replay);Check(game.Selection().PreviewAbility()==6 && game.Selection().PreviewAction()>=0,"Replay changed the selected ability");
    for(int n=0;n<7*hz && game.Selection().PreviewAction()>=0;++n)tick(1.f/hz);
    Check(game.Selection().PreviewTiming().progress==1 && game.Selection().PreviewAction()<0,"Finished rehearsal lost completion state");
    Check(game.Selection().Hero().Save()==hero && game.State().Save()==world,"Rehearsal controls changed profile or combat state");
    d.Click(game,Control::Replay);game.SuspendInput();tick(.3f);
    Check(game.Selection().PreviewAction()<0 && game.Selection().PreviewEffectCount()==0,"Suspension resumed an old replay");
    d.Click(game,Control::Overview);Check(!game.Selection().Enabled(Control::Replay) && !game.Selection().Enabled(Control::Stop),"Overview exposed rehearsal controls");
    if(hz==60 && (id==rpg::ClassId::Mage || id==rpg::ClassId::Priest)) {
      d.Click(game,Control::Arts);
      const auto arts=id==rpg::ClassId::Mage?std::vector<int>{1,5,9,13,14}:std::vector<int>{1,5,6,7};
      for(int ability:arts) {
        const auto found=std::find(Selection::previewArts.begin(),Selection::previewArts.end(),ability);
        d.Click(game,Control(int(Control::Art0)+int(found-Selection::previewArts.begin())));
        const bool heavy=ability!=7;const float phase=combat::StrokeStart(heavy)*.70f;
        while(game.Selection().PreviewAction()>=0 && game.Selection().PreviewAction()<phase)tick(.005f);
        d.frame.client={950,80};d.Step(game);renderer.BeginFrame();game.Render(renderer);
        Check(renderer.Capture((root/("art-gesture-"+std::to_string(int(id))+"-"+std::to_string(ability)+".png")).wstring())&&renderer.EndFrame(false),"Ability gesture capture");
      }
    }
  }
}
void ArtInspection(const std::filesystem::path& root,Renderer& renderer) {
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest}) {
    auto owned=std::make_unique<Game>();auto& game=*owned;game.Initialize();
    game.ChooseCharacter(root/("art-inspection-"+std::to_string(int(id))+"-"+std::to_string(GetTickCount64()))/"patrol.save");
    Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control(int(Control::Class0)+int(id)));
    d.Click(game,Control::Arts);d.Click(game,Control::Art1);d.Click(game,Control::Stop);
    const auto hero=game.Selection().Hero().Save(),world=game.State().Save();
    const auto recovery=game.Selection().InspectArt(3);
    Check(std::abs(recovery.windup-game.Selection().Hero().RecoveryCastDuration())<.00001f,"Defensive card misreports preparation time");
    Check(game.Selection().InspectArt(-1).windup==0 && game.Selection().InspectArt(4).recovery==0,"Invalid art exposes fabricated timing");
    for(auto size:{std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
      d.frame.width=size.first;d.frame.height=size.second;d.frame.client={950,10};d.Step(game);
      const auto art=id==rpg::ClassId::Mage?Control::Art11:Control::Art5;
      d.Point(art);for(int n=0;n<20;++n)d.Step(game);
      Check(game.Selection().Hint()==Control::None,"Art inspection ignored hover delay");
      for(int n=0;n<12;++n)d.Step(game);
      Check(game.Selection().Hint()==art && game.Selection().PreviewAction()<0,"Art hover started an unsolicited demonstration");
      Check(renderer.Resize(size.first,size.second),"Art inspection resize");renderer.BeginFrame();game.Render(renderer);
      Check(renderer.Capture((root/("art-inspection-"+std::to_string(int(id))+"-"+std::to_string(size.first)+".png")).wstring())&&renderer.EndFrame(false),"Art inspection capture");
      Check(game.Selection().Hero().Save()==hero && game.State().Save()==world,"Inspection mutated profile or world");
    }
    d.frame.width=1280;d.frame.height=720;d.frame.client={950,10};d.Step(game);
    for(int n=0;n<int(Control::None) && game.Selection().Focus()!=Control::Art1;++n)d.Key(game,VK_TAB);
    Check(game.Selection().Focus()==Control::Art1,"Keyboard cannot reach the art inspection");
    for(int n=0;n<32;++n)d.Step(game);
    Check(game.Selection().Hint()==Control::Art1,"Keyboard focus does not inspect an art");
    const auto details=game.Selection().InspectArt(1);
    d.Key(game,VK_RETURN);Check(game.Selection().PreviewAbility()==1 && game.Selection().PreviewAction()>=0,"Inspected keyboard art cannot be demonstrated");
    const float before=game.Selection().PreviewAction();d.Step(game);const float rate=(game.Selection().PreviewAction()-before)*60;
    const float boundary=details.ranged?combat::StrokeStart(true):combat::Contact(true);
    Check(rate>0 && std::abs(details.windup*rate-boundary)<.0001f,"Art card timing differs from the displayed action");
    d.Click(game,Control::Overview);Check(game.Selection().Hint()==Control::None,"Hidden Arts page retained an ability card");
  }
  Check(renderer.Resize(1280,720),"Restore creation view");
}
void Render(const std::filesystem::path& root,const std::string& veteran) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  Win32Window window;Check(window.Create(GetModuleHandleW(nullptr),1280,720,L"Character selection test",false),"Hidden window");
  Renderer renderer;Check(renderer.Initialize(window.GetHandle(),1280,720),"Renderer initialization");
  {
    const auto language=localization::language;localization::language=localization::Language::English;
    const float literalWidth=renderer.TextWidth("Warrior",2);
    localization::language=localization::Language::Russian;
    Check(renderer.TextWidth("Warrior",2,true)==literalWidth &&
      renderer.TextWidth("Warrior",2)==renderer.TextWidth("Воин",2,true),
      "Literal nicknames and translated interface captions share the same text path");
    localization::language=language;
  }
  Entrance(root,&renderer);
  StageTransitions(root,renderer);
  ArtTransport(root,renderer);
  ArtInspection(root,renderer);
  {
    renderer.BeginFrame();renderer.DrawHudRect(0,0,1,1,{.02f,.03f,.04f,1});
    renderer.SetCamera({0,3,-8},{0,1,0});DirectX::XMFLOAT2 before,after;
    Check(renderer.Project({0,1,0},before),"Portrait camera baseline");
    Check(renderer.BeginPortrait(.25f,.1f,.5f,.7f),"Ordinary portrait begin");
    renderer.DrawEffectGlow({0,1,0},.3f,{1,.5f,.1f,1});
    Check(renderer.EffectVertices()==0 && renderer.EndPortrait(),"Ordinary portrait unexpectedly accepted particles");
    Check(renderer.BeginPortrait(.25f,.1f,.5f,.7f,0,true),"Effect portrait begin");
    renderer.DrawEffectSprite(Renderer::EffectArt::Flame,{0,1,0},.3f,.5f,0,{1,.7f,.4f,1},true);
    Check(renderer.EffectVertices()>0 && renderer.EndPortrait(),"Opt-in portrait lost its particles");
    const auto submitted=renderer.EffectVertices();
    Check(renderer.Project({0,1,0},after) && std::abs(before.x-after.x)+std::abs(before.y-after.y)<.00001f,"Portrait did not restore world projection");
    Check(renderer.BeginPortrait(.7f,.2f,.15f,.3f),"Subsequent portrait begin");
    renderer.DrawEffectGlow({0,1,0},.3f,{1,.5f,.1f,1});
    Check(renderer.EffectVertices()==submitted && renderer.EndPortrait() && renderer.EndFrame(false),"Effect opt-in leaked to a later portrait");
  }
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
      driver.frame.client={LONG(layout.x+640*layout.scale),LONG(layout.y+556*layout.scale)};
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
    for(int art=0;art<game.Selection().PreviewArtCount();++art) {
      d.Click(game,Control(int(Control::Art0)+art));
      Check(game.Selection().PreviewAbility()==Selection::previewArts[art] && game.Selection().PreviewAction()>=0,"Art click previews wrong ability");
      // Slow caster signature clips can outlast four seconds at their class rate.
      float peakWeight=0;std::size_t peakBursts=0;
      for(int step=0;step<900 && game.Selection().PreviewAction()>=0;++step) {
        d.Step(game);peakWeight=std::max(peakWeight,game.Selection().PreviewBody().authoredStrikeWeight);
        peakBursts=std::max(peakBursts,game.Selection().PreviewEffectCount());
      }
      const auto id=game.Selection().SelectedClass();const int ability=Selection::previewArts[art];
      if((id==rpg::ClassId::Warrior || id==rpg::ClassId::Scout || id==rpg::ClassId::Mage || id==rpg::ClassId::Priest) && ability!=2 && ability!=3)
        Check(peakWeight>.25f,"Character audition bypassed the runtime weapon controller");
      if(ability!=2)Check(peakBursts>0,"Character audition failed to emit its release/completion");
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
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest}) {
    d.Click(game,Control(int(Control::Class0)+int(id)));d.Click(game,Control::Arts);d.Click(game,Control::Art1);
    const auto initial=game.Selection().Hero().Save();
    const float rate=rpg::ClassAttackRate(id,true);
    const int prepareTicks=int(combat::StrokeStart(true)/rate*.70f*60);
    for(int n=0;n<prepareTicks;++n)d.Step(game);
    renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/("stage-prepare-"+std::to_string(int(id))+".png")).wstring()) && renderer.EndFrame(false),"Stage preparation capture");
    if(id==rpg::ClassId::Mage || id==rpg::ClassId::Priest)Check(renderer.EffectVertices()>0,"Stage preparation has no rendered effects");
    while(game.Selection().PreviewEffectCount()==0 && game.Selection().PreviewAction()>=0)d.Step(game);
    for(int n=0;n<4;++n)d.Step(game);
    auto copied=game.Selection();const auto activeEffects=game.Selection().PreviewEffectCount();
    copied.Refresh();Check(copied.PreviewEffectCount()==0 && game.Selection().PreviewEffectCount()==activeEffects,
      "Copied selection shares mutable preview effects");
    renderer.BeginFrame();game.Render(renderer);
    Check(renderer.Capture((root/("stage-release-"+std::to_string(int(id))+".png")).wstring()) && renderer.EndFrame(false),"Stage release capture");
    Check(renderer.EffectVertices()>0 && renderer.EffectVertices()<=Renderer::maxEffectVertices,"Stage release missing/over budget");
    Check(initial==game.Selection().Hero().Save() && frozenWorld==game.State().Save(),"Stage effects changed a profile/world");
    d.frame.focused=false;d.Step(game);
    Check(game.Selection().PreviewEffectCount()==0 && game.Selection().PreviewAction()<0,"Focus loss retained a stage effect");
    d.frame.focused=true;d.Step(game);
    for(int dress=0;dress<3;++dress) {
      d.Click(game,Control(int(Control::Dress0)+dress));
      renderer.BeginFrame();game.Render(renderer);
      Check(renderer.Capture((root/("wardrobe-"+std::to_string(int(id))+"-"+std::to_string(dress)+".png")).wstring()) && renderer.EndFrame(false),"Wardrobe native capture");
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
  d.Key(game,VK_HOME);d.frame.down[VK_SHIFT]=true;for(int n=0;n<7;++n)d.Key(game,VK_RIGHT);d.frame.down[VK_SHIFT]=false;
  renderer.BeginFrame();game.Render(renderer);
  Check(renderer.Capture((root/"creation-name-selection.png").wstring()) && renderer.EndFrame(false),"Selected-name capture");
  d.Key(game,VK_END);
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
  Check(game.Selection().DraftName()==draftName && game.Selection().NameSelection().Selected(),"Ctrl+A did not select the complete existing name");
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
void NameEditing(const std::filesystem::path& root) {
  ui::NameEditor edit;std::string value="Алёна";edit.Place(value,2);
  edit.Insert(value,"ь");Check(value=="Альёна" && edit.caret==3,"Unicode insertion split a letter");
  edit.Erase(value,true);edit.Move(value,1,true);edit.Insert(value,"и");Check(value=="Алина","Selected Cyrillic replacement failed");
  edit.All(value);edit.Insert(value,"123\xff");Check(value=="Алина" && edit.Selected(),"Invalid paste erased selection");
  edit.Insert(value,std::string(21,'W'));Check(value==std::string(20,'W') && edit.caret==20,"Selected paste did not enforce character limit");
  edit.Place(value,0);edit.Erase(value,false);Check(value.size()==19 && edit.caret==0,"Delete did not remove next character");
  auto owner=std::make_unique<Game>();auto& game=*owner;game.Initialize();game.ChooseCharacter(root/"name-editor"/"patrol.save");
  Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control::Next);
  d.frame.nativeText=true;d.frame.text="Алёна";d.Step(game);d.frame.text.clear();
  d.Key(game,VK_HOME);d.Key(game,VK_RIGHT);d.frame.down[VK_SHIFT]=true;d.Key(game,VK_RIGHT);d.frame.down[VK_SHIFT]=false;
  d.frame.text="ри";d.Step(game);d.frame.text.clear();Check(game.Selection().DraftName()=="Ариёна","Native selected insertion failed");
  d.frame.down[VK_CONTROL]=true;d.Key(game,'A');d.frame.clipboard="Мирослав-Ёж123";d.Key(game,'V');d.frame.down[VK_CONTROL]=false;
  Check(game.Selection().DraftName()=="Мирослав-Ёж","Clipboard filtering or UTF-8 paste failed");
  // Measured proportional glyph boundaries are used in local layout space.
  for(const auto size:{std::pair<int,int>{960,600},{1920,1080},{2560,1080}}) {
    d.frame.width=size.first;d.frame.height=size.second;d.Step(game);
    std::array<float,21> advances{};for(int n=0;n<21;++n)advances[n]=float(n*10);
    game.Selection().MeasureName(advances);const ui::Layout layout(float(size.first),float(size.second));
    const auto name=Selection::Bounds(Control::Name);
    const auto point=[&](float x){d.frame.client={LONG(layout.x+(name.x+10+x)*layout.scale),LONG(layout.y+(name.y+20)*layout.scale)};};
    point(12);d.frame.down[VK_LBUTTON]=true;d.Step(game);point(42);d.Step(game);
    Check(game.Selection().NameSelection().Begin()==1 && game.Selection().NameSelection().End()==4,"Scaled text drag selected wrong characters");
    d.Point(Control::Confirm);d.frame.down[VK_LBUTTON]=false;d.Step(game);
    Check(game.Selection().Screen()==Selection::Page::Customize && !std::filesystem::exists(game.Selection().SaveFile()),"Text drag published a profile");
  }
  d.frame.down[VK_CONTROL]=true;d.Key(game,'A');d.frame.clipboard=std::string(20,'W');d.Key(game,'V');d.frame.down[VK_CONTROL]=false;
  d.frame.down[VK_BACK]=true;for(int n=0;n<48;++n)d.Step(game);d.frame.down[VK_BACK]=false;d.Step(game);
  Check(game.Selection().DraftName().size()<15 && !game.Selection().DraftName().empty(),"Held Backspace does not repeat after its delay");
  d.frame.down[VK_CONTROL]=true;d.Key(game,'A');d.frame.clipboard=" Алёна ";d.Key(game,'V');d.frame.down[VK_CONTROL]=false;
  Check(!game.Selection().Enabled(Control::Confirm),"Leading space allowed profile publication");
  d.Key(game,VK_HOME);d.Key(game,VK_DELETE);d.Key(game,VK_RETURN);
  Check(game.Selection().DraftName()=="Алёна" && game.Selection().Focus()==Control::Confirm,"Edited name failed final trim/validation");
}
void RussianNames(const std::filesystem::path& root) {
  Game game;game.Initialize();game.ChooseCharacter(root/("russian-"+std::to_string(GetTickCount64()))/"patrol.save");
  Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control::Next);
  d.frame.nativeText=true;d.frame.text="Алёна";d.Step(game);d.frame.text.clear();
  Check(game.Selection().DraftName()=="Алёна","Native Russian keyboard text lost");
  d.Key(game,VK_BACK);Check(game.Selection().DraftName()=="Алён","Backspace split a Cyrillic codepoint");
  d.frame.text="а-Ёж";d.Step(game);d.frame.text.clear();
  Check(game.Selection().DraftName()=="Алёна-Ёж","Cyrillic punctuation/yo lost");
  d.Key(game,VK_RETURN);d.Click(game,Control::Confirm);
  Check(std::string(game.Selection().Hero().Nickname())=="Алёна-Ёж" && game.Selection().State()==Selection::Status::Saved,"Russian character creation/save failed");
  rpg::Simulation s,copy;std::string twenty;
  for(int n=0;n<20;++n) twenty+="я";
  Check(s.CreateCharacter(rpg::ClassId::Mage,twenty) && copy.Load(s.Save()) && copy.Save()==s.Save(),"20 Cyrillic letters do not round-trip");
  Check(!rpg::ValidCharacterName(twenty+"я") && !rpg::ValidCharacterName("Я\xff") && !rpg::ValidCharacterName("Я1"),"Malformed/long names accepted");
}
void StageInteraction(const std::filesystem::path& root) {
  for(const auto size:{std::pair<int,int>{960,600},{1280,720},{2560,1080}}) {
    Game game;game.Initialize();game.ChooseCharacter(root/("stage-input-"+std::to_string(size.first)+"-"+std::to_string(GetTickCount64()))/"patrol.save");
    Driver d;d.frame.width=size.first;d.frame.height=size.second;d.Step(game);d.Click(game,Control::Create);
    d.Click(game,Control::Class5);d.Click(game,Control::Next);d.Key(game,'A');d.Key(game,VK_RETURN);
    const auto draft=game.Selection().Hero().Save(),world=game.State().Save();
    const ui::Layout layout(float(size.first),float(size.second));
    const auto point=[&](float x,float y){d.frame.client={LONG(layout.x+x*layout.scale),LONG(layout.y+y*layout.scale)};};
    point(620,360);d.frame.down[VK_LBUTTON]=true;d.Step(game);point(700,360);d.Step(game);
    Check(game.Selection().Rotating() && game.Selection().Yaw()>.8f,"Stage drag lost scaled movement");
    // Releasing over Create must never publish a profile from a stage drag.
    d.Point(Control::Confirm);d.frame.down[VK_LBUTTON]=false;d.Step(game);
    Check(game.Selection().Screen()==Selection::Page::Customize && !std::filesystem::exists(game.Selection().SaveFile()),"Stage release activated a UI button");
    point(620,360);d.input.Update(d.frame);game.Update(1./60,d.input,3);
    for(int n=0;n<60;++n)d.Step(game);
    Check(game.Selection().PreviewZoom()>.5f && game.Selection().PreviewZoom()<.6f,"Wheel did not use bounded smooth stage zoom");
    const auto zoom=game.Selection().PreviewZoom();d.Point(Control::Name);d.input.Update(d.frame);game.Update(1./60,d.input,10);
    for(int n=0;n<30;++n)d.Step(game);
    Check(std::abs(game.Selection().PreviewZoom()-zoom)<.002f,"Scrolling a control changed the stage camera");
    point(620,360);d.frame.down[VK_LBUTTON]=true;d.Step(game);d.frame.focused=false;d.Step(game);
    const float yaw=game.Selection().Yaw();point(900,360);d.frame.focused=true;d.Step(game);
    Check(!game.Selection().Rotating() && game.Selection().Yaw()==yaw,"Focus restoration replayed a drag");
    d.frame.down[VK_LBUTTON]=false;d.Step(game);
    point(620,360);d.frame.down[VK_LBUTTON]=true;d.Step(game);d.frame.width+=32;d.Step(game);
    Check(!game.Selection().Rotating(),"Resize retained an old stage gesture");
    d.frame.down[VK_LBUTTON]=false;d.Step(game);d.Click(game,Control::ResetView);
    Check(game.Selection().Yaw()==0 && !game.Selection().Closeup(),"Stage reset did not restore the view");
    Check(game.Selection().Hero().Save()==draft && game.State().Save()==world,"Camera navigation changed durable game state");
  }
}
void CompactDock(const std::filesystem::path& root) {
  auto storage=std::make_unique<Game>();auto& game=*storage;game.Initialize();game.ChooseCharacter(root/"dock"/"patrol.save");
  Driver d;d.Step(game);d.Click(game,Control::Create);
  const std::array<Control,9> controls{Control::Left,Control::Right,Control::Dress0,Control::Dress1,Control::Dress2,
    Control::Closeup,Control::Pose,Control::Showcase,Control::ResetView};
  for(auto a:controls)for(auto b:controls)if(a!=b) {
    const auto x=Selection::Bounds(a),y=Selection::Bounds(b);
    Check(!(x.x<y.x+y.w && x.x+x.w>y.x && x.y<y.y+y.h && x.y+x.h>y.y),"Dock hit regions overlap");
  }
  for(auto size:{std::pair<int,int>{960,600},{1920,1080},{2560,1080}}) {
    d.frame.width=size.first;d.frame.height=size.second;d.Step(game);d.Point(Control::Dress2);
    for(int n=0;n<12;++n)d.Step(game);
    Check(game.Selection().Hint()==Control::None,"Dock tooltip appears before deliberate hover");
    for(int n=0;n<18;++n)d.Step(game);
    Check(game.Selection().Hint()==Control::Dress2,"Scaled dock hover does not reveal its tooltip");
    d.frame.down[VK_LBUTTON]=true;d.Step(game);Check(game.Selection().Hint()==Control::None,"Pressed dock retained a blocking tooltip");
    d.frame.down[VK_LBUTTON]=false;d.Step(game);
    Check(game.Selection().PreviewDress()==Selection::Dress::Veteran && !game.Selection().Rotating(),"Dock click leaked through into stage rotation");
    d.frame.focused=false;d.Step(game);Check(game.Selection().Hint()==Control::None,"Focus loss retained a tooltip");
    d.frame.focused=true;d.Step(game);
  }
  d.Key(game,VK_TAB);const auto focused=game.Selection().Focus();
  for(int n=0;n<30;++n)d.Step(game);
  Check(game.Selection().Hint()==focused,"Keyboard navigation cannot inspect a compact control");
  d.Point(Control::Closeup);for(int n=0;n<30;++n)d.Step(game);
  Check(game.Selection().Hint()==Control::Closeup,"Mouse movement did not reclaim hints after keyboard navigation");
  float reference=0;
  for(int hz:{30,60,144}) {
    d.Click(game,Control::ResetView);const auto before=game.Selection().Hero().Save();
    d.Key(game,VK_RIGHT);
    Check(game.Selection().Yaw()>0 && game.Selection().Yaw()<.4f,"Arrow rotation snaps rather than easing");
    for(int n=0;n<hz;++n){d.input.Update(d.frame);game.Update(1./hz,d.input,0);}
    const float yaw=game.Selection().Yaw();
    Check(std::abs(yaw-.4f)<.0001f,"Arrow rotation does not settle at the requested angle");
    if(hz==30)reference=yaw;else Check(std::abs(yaw-reference)<.00001f,"Arrow rotation depends on refresh rate");
    Check(game.Selection().Hero().Save()==before,"View controls changed durable character data");
  }
}
void WardrobeAndShowcase(const std::filesystem::path& root) {
  for(auto id:{rpg::ClassId::Warrior,rpg::ClassId::Mage,rpg::ClassId::Scout,rpg::ClassId::Priest})for(int hz:{30,60,144}) {
    auto gameStorage=std::make_unique<Game>();auto& game=*gameStorage;game.Initialize();
    game.ChooseCharacter(root/("wardrobe-"+std::to_string(int(id))+"-"+std::to_string(hz)+"-"+std::to_string(GetTickCount64()))/"patrol.save");
    Driver d;d.Step(game);d.Click(game,Control::Create);d.Click(game,Control(int(Control::Class0)+int(id)));
    const auto original=game.Selection().Hero().Save(),world=game.State().Save();
    d.Click(game,Control::Dress2);const auto veteran=game.Selection().PreviewOutfit();
    Check(veteran.armor && veteran.rewardWeapon && veteran.cloak && veteran.shoulders && veteran.gloves,"Veteran wardrobe omitted real equipment pieces");
    Check(veteran.shield==(id==rpg::ClassId::Warrior),"Wardrobe lost class-specific offhand rules");
    d.Click(game,Control::Dress1);const auto travel=game.Selection().PreviewOutfit();
    Check(!travel.armor && !travel.rewardWeapon && !travel.cloak && !travel.shoulders && travel.weapon,"Travel outfit is not a separate real equipment view");
    d.Click(game,Control::Dress0);
    Check(game.Selection().PreviewOutfit().armor==CharacterOutfit::From(game.Selection().Hero().Items()).armor,"Equipped wardrobe failed to restore actual gear");
    d.Click(game,Control::Arts);d.Click(game,Control::Art1);
    sound::Events audio;unsigned release=0,preparation=0,effort=0;
    const auto collect=[&](){
      audio.Preview(game.Selection().PreviewAudio());
      for(unsigned n=0;n<audio.count;++n){const auto cue=audio.cues[n];
        preparation+=sound::IsPreparation(cue);effort+=cue==sound::Cue::WarriorEffort;
        release+=cue==(id==rpg::ClassId::Warrior?sound::Cue::Heavy:sound::ReleaseCue(id,1,true));
        Check(!audio.spatial[n],"Menu audition inherited a world-space sound source");
      }
      audio.Preview(game.Selection().PreviewAudio());Check(audio.count==0,"Repeated audio sampling replayed a stage event");
    };
    collect();
    for(int n=0;n<hz*8 && game.Selection().PreviewAction()>=0;++n){d.input.Update(d.frame);game.Update(1./hz,d.input,0);collect();}
    Check(release==1 && preparation==(id==rpg::ClassId::Warrior?0u:1u) && effort==(id==rpg::ClassId::Warrior?1u:0u),"Audition phases are duplicated, silent or class-incorrect");
    d.Click(game,Control::Showcase);Check(game.Selection().Showcasing(),"Showcase toggle did not start");
    std::array<bool,4> seen{};constexpr int arts[]={0,1,5,6};
    for(int n=0;n<hz*22;++n){d.input.Update(d.frame);game.Update(1./hz,d.input,0);for(int k=0;k<4;++k)seen[k]|=game.Selection().PreviewAbility()==arts[k];}
    Check(std::all_of(seen.begin(),seen.end(),[](bool value){return value;}),"Showcase did not demonstrate the full class sequence");
    d.Click(game,Control::Art0);Check(!game.Selection().Showcasing(),"Manual art selection did not stop automatic cycling");
    d.Click(game,Control::Showcase);game.SuspendInput();
    Check(!game.Selection().Showcasing() && game.Selection().PreviewAction()<0 && game.Selection().PreviewAudio().count==0,
      "Native suspension without an unfocused Update retained preview playback");
    d.Click(game,Control::Showcase);d.frame.focused=false;d.Step(game);
    Check(!game.Selection().Showcasing() && !game.Selection().PreviewAudio().preparing && game.Selection().PreviewAudio().count==0,"Focus loss left a queued demonstration or charge sound");
    d.frame.focused=true;d.Step(game);d.Click(game,Control::Dress2);
    Check(game.Selection().Hero().Save()==original && game.State().Save()==world,"Wardrobe/showcase modified durable state");
    d.Click(game,Control::Next);d.Key(game,'A');d.Key(game,VK_RETURN);d.Click(game,Control::Confirm);
    Check(game.Selection().State()==Selection::Status::Saved && !game.Selection().Hero().RewardWeaponEquipped(),"Veteran preview granted gear to the new character");
  }
}
}
int main() {
  const auto root=std::filesystem::current_path()/"character-selection-check";std::filesystem::create_directories(root);
  const auto veteran=Read(std::filesystem::path(RPG_TEST_FIXTURES)/"veteran-preview.save");
  ReportedRegressions(root);StorageTransactions(root);Flow(root,veteran);ClassSlots(root);CreationSafety(root);AppearancePersistence();NameEditing(root);RussianNames(root);StageInteraction(root);CompactDock(root);WardrobeAndShowcase(root);Render(root,veteran);
  std::cout<<"PASS: character metadata, frozen world, input isolation, saved/new/error/repaired states, changed saves, resize/focus, hidden DX11 captures\n";
}
