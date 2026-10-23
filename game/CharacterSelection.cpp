#include "game/CharacterSelection.h"
#include "game/ProfileStorage.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace {
CharacterSelection::Status ReadProfile(const std::filesystem::path& path,rpg::ClassId id,
    rpg::Simulation& hero,std::string& snapshot) {
  using S=CharacterSelection::Status;
  snapshot.clear();std::string content;const auto read=profile::Read(path,content);
  if(read==profile::ReadResult::Missing) return S::New;
  rpg::Simulation candidate;
  if(read!=profile::ReadResult::Ready || !candidate.Load(content) || candidate.Class()!=id) return S::Unreadable;
  hero=std::move(candidate);snapshot=std::move(content);return S::Saved;
}
}
std::filesystem::path CharacterSelection::Path(rpg::ClassId id) const {
  return id==rpg::ClassId::Warrior?base_:base_.parent_path()/("hero-"+std::to_string(int(id))+".save");
}
void CharacterSelection::SetPage(Page page,Control focus) {
  page_=page;focus_=focus;hover_=Control::None;CancelGesture();
}
void CharacterSelection::ReadSlots() {
  for(int i=0;i<int(slots_.size());++i) {
    rpg::Simulation hero;std::string snapshot;auto& slot=slots_[i];
    slot.status=ReadProfile(Path(rpg::ClassId(i)),rpg::ClassId(i),hero,snapshot);
    slot.name=slot.status==Status::Saved?hero.Nickname():"";slot.level=hero.Level();
  }
}
void CharacterSelection::Open(const std::filesystem::path& file) {
  base_=file;file_=file;class_=rpg::ClassId::Warrior;draftName_.clear();look_={};notice_.clear();
  time_=yaw_=width_=height_=zoom_=0;closeup_=false;actionTime_=-1;SetPage(Page::Roster,Control::Enter);Refresh();
  focus_=status_==Status::Saved?Control::Enter:status_==Status::New?Control::Create:Control::Refresh;
}
void CharacterSelection::Refresh() {
  CancelGesture();changed_=false;notice_.clear();ReadSlots();file_=Path(class_);
  hero_=rpg::Simulation{};hero_.CreateCharacter(class_,rpg::DescribeClass(class_).title);
  status_=ReadProfile(file_,class_,hero_,snapshot_);
}
void CharacterSelection::PreviewDraft() {
  auto name=draftName_;while(!name.empty() && name.back()==' ') name.pop_back();
  hero_.CreateCharacter(class_,rpg::ValidCharacterName(name)?name:rpg::DescribeClass(class_).title,look_);
}
Rect CharacterSelection::Bounds(Control control) {
  if(control>=Control::Skin0 && control<=Control::Dye4) {
    const int n=int(control)-int(Control::Skin0);
    return {888+52.f*(n%5),348+58.f*(n/5),46,34};
  }
  if(control>=Control::Class0 && control<=Control::Class10) return {84,183+36.f*(int(control)-int(Control::Class0)),180,34};
  switch(control) {
  case Control::Name:return {842,262,350,38};
  case Control::Hero:return {842,182,350,128};
  case Control::Create:return {842,470,350,46};
  case Control::Next:case Control::Confirm:case Control::Enter:return {842,532,350,52};
  case Control::Back:case Control::Refresh:return {842,602,168,36};
  case Control::Cancel:case Control::Quit:return {1024,602,168,36};
  case Control::Left:return {394,604,44,32};
  case Control::Right:return {582,604,44,32};
  case Control::Closeup:return {330,646,142,32};
  case Control::Pose:return {480,646,142,32};
  case Control::ResetView:return {630,646,142,32};
  case Control::SkinLeft:return {842,348,34,34};case Control::SkinRight:return {1158,348,34,34};
  case Control::HairLeft:return {842,406,34,34};case Control::HairRight:return {1158,406,34,34};
  case Control::DyeLeft:return {842,464,34,34};case Control::DyeRight:return {1158,464,34,34};
  default:return {};
  }
}
bool CharacterSelection::Enabled(Control control) const {
  if(control==Control::None) return false;
  if(control==Control::Closeup || control==Control::Pose || control==Control::ResetView)
    return page_!=Page::Roster || status_!=Status::Unreadable;
  if(control>=Control::Skin0 && control<=Control::Dye4) return page_==Page::Customize;
  if(control==Control::Left || control==Control::Right) return page_!=Page::Roster || status_!=Status::Unreadable;
  if(control>=Control::Class0 && control<=Control::Class10) return page_!=Page::Customize;
  if(page_==Page::Roster) {
    if(control==Control::Enter || control==Control::Hero) return status_==Status::Saved;
    if(control==Control::Refresh || control==Control::Quit) return true;
    if(control==Control::Create) return std::any_of(slots_.begin(),slots_.end(),[](const Slot& slot){return slot.status==Status::New;});
    return false;
  }
  if(control==Control::Back || control==Control::Cancel) return true;
  if(page_==Page::Class) return control==Control::Next && slots_[int(class_)].status==Status::New;
  if(control==Control::Name || (control>=Control::SkinLeft && control<=Control::DyeRight)) return true;
  return control==Control::Confirm && rpg::ValidCharacterName(draftName_) && slots_[int(class_)].status==Status::New;
}
void CharacterSelection::CreateProfile() {
  PreviewDraft();file_=Path(class_);notice_.clear();
  const auto result=profile::Write(file_,hero_.Save());
  if(result!=profile::WriteResult::Saved) {
    if(result==profile::WriteResult::Conflict) ReadSlots();
    notice_=result==profile::WriteResult::Conflict?"This class slot is unavailable. The existing save was kept.":
      result==profile::WriteResult::Busy?"Another game is saving this class. Your draft is kept; try again.":
      "Could not save this character. Your draft is kept; try again.";
    return;
  }
  SetPage(Page::Roster,Control::Enter);Refresh();
  notice_=status_==Status::Saved?"Character created. Select Enter World to begin.":"Character saved, but unreadable. Refresh to check it.";
}
CharacterSelection::Action CharacterSelection::Activate(Control control) {
  if(!Enabled(control)) return Action::None;
  if(control>=Control::Class0 && control<=Control::Class10) {
    actionTime_=-1;
    class_=rpg::ClassId(int(control)-int(Control::Class0));Refresh();
    if(page_==Page::Class) {PreviewDraft();focus_=Enabled(Control::Next)?Control::Next:control;}
    else focus_=status_==Status::Saved?Control::Enter:status_==Status::New?Control::Create:Control::Refresh;
    return Action::None;
  }
  switch(control) {
  case Control::Closeup:closeup_=!closeup_;break;
  case Control::Pose:closeup_=false;actionTime_=0;break;
  case Control::ResetView:closeup_=false;yaw_=0;actionTime_=-1;break;
  case Control::Name:break;
  case Control::Hero:focus_=Control::Enter;break;
  case Control::Enter: {
    const auto previous=snapshot_;Refresh();
    if(status_!=Status::Saved || snapshot_!=previous) {changed_=true;break;}
    return Action::Enter;
  }
  case Control::Create:
    ReadSlots();
    if(slots_[int(class_)].status!=Status::New) {
      const auto empty=std::find_if(slots_.begin(),slots_.end(),[](const Slot& slot){return slot.status==Status::New;});
      if(empty==slots_.end()) {notice_="All class slots are occupied.";break;}
      class_=rpg::ClassId(empty-slots_.begin());
    }
    look_={};draftName_.clear();notice_.clear();PreviewDraft();SetPage(Page::Class,Control::Next);break;
  case Control::Next:
    ReadSlots();
    if(slots_[int(class_)].status!=Status::New) {notice_="This class slot is occupied. Choose another calling.";break;}
    notice_.clear();PreviewDraft();SetPage(Page::Customize,Control::Name);break;
  case Control::Confirm:CreateProfile();break;
  case Control::Cancel:SetPage(Page::Roster,Control::Create);Refresh();break;
  case Control::Back:
    if(page_==Page::Customize) {ReadSlots();SetPage(Page::Class,Control::Next);notice_.clear();}
    else {SetPage(Page::Roster,Control::Create);Refresh();}
    break;
  case Control::Refresh:Refresh();break;
  case Control::Quit:return Action::Quit;
  case Control::Left:yaw_=std::remainder(yaw_-.4f,6.2831853f);break;
  case Control::Right:yaw_=std::remainder(yaw_+.4f,6.2831853f);break;
  default:
    if(control>=Control::Skin0 && control<=Control::Dye4) {
      const int n=int(control)-int(Control::Skin0);
      (n<5?look_.skin:n<10?look_.hair:look_.dye)=n%5;PreviewDraft();
    }
    if(control>=Control::SkinLeft && control<=Control::DyeRight) {
      const int offset=int(control)-int(Control::SkinLeft);
      int& choice=offset<2?look_.skin:offset<4?look_.hair:look_.dye;
      choice=(choice+(offset%2?1:-1)+rpg::Appearance::count)%rpg::Appearance::count;PreviewDraft();
    }
    break;
  }
  return Action::None;
}
CharacterSelection::Action CharacterSelection::Update(float seconds,const Input& input) {
  if(!input.Focused()) {CancelGesture();hover_=Control::None;return Action::None;}
  time_=std::fmod(time_+std::clamp(seconds,0.f,.05f),600.f);
  const float dt=std::clamp(seconds,0.f,.05f);
  zoom_+=(float(closeup_)-zoom_)*-std::expm1(-12.f*dt);
  if(actionTime_>=0) {actionTime_+=dt;if(actionTime_>1.6f) actionTime_=-1;}
  const bool resized=width_!=input.Width() || height_!=input.Height();width_=input.Width();height_=input.Height();
  if(resized || input.Pressed(VK_F11)) {CancelGesture();hover_=Control::None;return Action::None;}
  const Layout layout(width_,height_);hover_=Control::None;
  for(int i=0;i<int(Control::None);++i) {
    const auto control=Control(i);
    if(Enabled(control) && Bounds(control).Contains(layout.LocalX(input.ClientX()),layout.LocalY(input.ClientY()))) hover_=control;
  }
  if(input.Pressed(VK_TAB)) {
    const int direction=input.Held(VK_SHIFT)?-1:1;
    do {focus_=Control((int(focus_)+direction+int(Control::None))%int(Control::None));} while(!Enabled(focus_));
  }
  if(input.Pressed(VK_ESCAPE)) {
    CancelGesture();if(page_!=Page::Roster) return Activate(Control::Back);
    focus_=Control::Quit;return Action::None;
  }
  if(input.Pressed(VK_LBUTTON)) {pressed_=hover_;if(hover_!=Control::None) focus_=hover_;}
  if(input.Released(VK_LBUTTON)) {
    const auto clicked=pressed_;CancelGesture();if(clicked==hover_) return Activate(clicked);
  }
  if(focus_==Control::Name && page_==Page::Customize) {
    const auto previousName=draftName_;
    const bool control=input.Held(VK_CONTROL),alt=input.Held(VK_MENU);
    if(control && !alt && input.Pressed('A')) draftName_.clear();
    if(!control && !alt) {
      for(int key='A';key<='Z';++key) if(input.Pressed(key) && draftName_.size()<20) draftName_+=char((input.Held(VK_SHIFT)!=input.CapsLock())?key:key+32);
      if(input.Pressed(VK_BACK) && !draftName_.empty()) draftName_.pop_back();
      if(input.Pressed(VK_SPACE) && !draftName_.empty() && draftName_.back()!=' ' && draftName_.size()<19) draftName_+=' ';
      for(auto key:{VK_OEM_MINUS,VK_OEM_7}) if(input.Pressed(key) && draftName_.size()<20) draftName_+=key==VK_OEM_MINUS?'-':'\'';
    }
    if(previousName!=draftName_) PreviewDraft();
    if(input.Pressed(VK_RETURN)) {
      while(!draftName_.empty() && draftName_.back()==' ') draftName_.pop_back();
      if(Enabled(Control::Confirm)) {PreviewDraft();focus_=Control::Confirm;}
    }
    return Action::None;
  }
  if(input.Pressed(VK_RETURN) || input.Pressed(VK_SPACE)) return Activate(focus_);
  if(input.Pressed('R')) return Activate(Control::Refresh);
  if(input.Pressed(VK_LEFT)) return Activate(Control::Left);
  if(input.Pressed(VK_RIGHT)) return Activate(Control::Right);
  return Action::None;
}
}
