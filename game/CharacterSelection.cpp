#include "game/CharacterSelection.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace ui {
void CharacterSelection::Open(const std::filesystem::path& file) {
  base_=file;file_=file;class_=rpg::ClassId::Warrior;draftName_=rpg::DescribeClass(class_).title;editingName_=false; time_=yaw_=width_=height_=0; hover_=pressed_=Control::None;
  Refresh(); focus_=status_==Status::Unreadable?Control::Refresh:Control::Enter;
}
void CharacterSelection::Refresh() {
  CancelGesture(); changed_=false; snapshot_.clear(); hero_=rpg::Simulation{};
  status_=Status::Unreadable;
  std::error_code error;
  const bool exists=std::filesystem::exists(file_,error);
  if(error) return;
  if(!exists) {
    status_=Status::New;
    if(!hero_.CreateCharacter(class_,draftName_)) hero_.CreateCharacter(class_,rpg::DescribeClass(class_).title);
    return;
  }
  const auto length=std::filesystem::file_size(file_,error);
  if(error || length==0 || length>4096) return;
  std::ifstream file(file_,std::ios::binary);
  std::string content(static_cast<std::size_t>(length),'\0');
  if(!file.read(content.data(),static_cast<std::streamsize>(length)) || file.peek()!=std::char_traits<char>::eof()) return;
  if(!hero_.Load(content) || hero_.Class()!=class_) {hero_=rpg::Simulation{};return;}
  snapshot_=std::move(content); status_=Status::Saved;
}
Rect CharacterSelection::Bounds(Control control) {
  if(control>=Control::Class0 && control<=Control::Class10) return {84,186+35.f*(int(control)-int(Control::Class0)),180,31};
  switch(control) {
  case Control::Name:return {842,326,350,36};
  case Control::Hero:return {842,182,350,128};
  case Control::Enter:return {842,532,350,52};
  case Control::Refresh:return {842,602,168,36};
  case Control::Quit:return {1024,602,168,36};
  case Control::Left:return {394,604,44,32};
  case Control::Right:return {582,604,44,32};
  default:return {};
  }
}
bool CharacterSelection::Enabled(Control control) const {
  if(control==Control::Enter && status_==Status::New && (draftName_.empty() || draftName_.back()==' ')) return false;
  if(control==Control::Name) return status_==Status::New;
  if(control>=Control::Class0 && control<=Control::Class10) return true;
  return control!=Control::None && (status_!=Status::Unreadable || control==Control::Refresh || control==Control::Quit);
}
CharacterSelection::Action CharacterSelection::Activate(Control control) {
  if(!Enabled(control)) return Action::None;
  if(control>=Control::Class0 && control<=Control::Class10) {
    class_=rpg::ClassId(int(control)-int(Control::Class0));
    file_=class_==rpg::ClassId::Warrior?base_:base_.parent_path()/("hero-"+std::to_string(int(class_))+".save");
    draftName_=rpg::DescribeClass(class_).title;editingName_=false;Refresh();focus_=Control::Enter;return Action::None;
  }
  switch(control) {
  case Control::Name:editingName_=true;break;
  case Control::Hero:focus_=Control::Enter;break;
  case Control::Enter: {
    editingName_=false;
    // Another build or an external editor can change this file while the screen
    // is open. Show the new state for review instead of entering a different hero.
    const auto previous=snapshot_; const auto state=status_; Refresh();
    if(status_!=state || snapshot_!=previous) {changed_=true;break;}
    return Action::Enter;
  }
  case Control::Refresh:Refresh();break;
  case Control::Quit:return Action::Quit;
  case Control::Left:yaw_=std::remainder(yaw_-.4f,6.2831853f);break;
  case Control::Right:yaw_=std::remainder(yaw_+.4f,6.2831853f);break;
  default:break;
  }
  return Action::None;
}
CharacterSelection::Action CharacterSelection::Update(float seconds,const Input& input) {
  if(!input.Focused()) {CancelGesture();hover_=Control::None;return Action::None;}
  time_=std::fmod(time_+std::clamp(seconds,0.f,.05f),600.f);
  // A display change must not turn a release into a click on a new layout.
  const bool resized=width_!=input.Width() || height_!=input.Height();
  width_=input.Width();height_=input.Height();
  if(resized || input.Pressed(VK_F11)) {CancelGesture();hover_=Control::None;return Action::None;}
  const Layout layout(width_,height_);
  hover_=Control::None;
  for(int i=0;i<int(Control::None);++i) {
    const auto control=Control(i);
    if(Enabled(control) && Bounds(control).Contains(layout.LocalX(input.ClientX()),layout.LocalY(input.ClientY()))) hover_=control;
  }
  if(input.Pressed(VK_TAB)) {
    const int direction=input.Held(VK_SHIFT)?-1:1;
    do {focus_=Control((int(focus_)+direction+int(Control::None))%int(Control::None));} while(!Enabled(focus_));
  }
  if(input.Pressed(VK_ESCAPE)) {focus_=Control::Quit;CancelGesture();return Action::None;}
  if(input.Pressed(VK_LBUTTON)) {pressed_=hover_;if(hover_!=Control::None) focus_=hover_;}
  if(input.Released(VK_LBUTTON)) {
    const auto clicked=pressed_;CancelGesture();
    if(clicked==hover_) return Activate(clicked);
  }
  if(focus_==Control::Name && status_==Status::New) {
    if(!editingName_) {draftName_.clear();editingName_=true;}
    if(input.Pressed(VK_BACK) && !draftName_.empty()) draftName_.pop_back();
    for(int key='A';key<='Z';++key) if(input.Pressed(key) && draftName_.size()<20) draftName_+=char(input.Held(VK_SHIFT)?key:key+32);
    if(input.Pressed(VK_SPACE) && !draftName_.empty() && draftName_.back()!=' ' && draftName_.size()<19) draftName_+=' ';
    auto name=draftName_;while(!name.empty() && name.back()==' ') name.pop_back();
    if(!name.empty()) hero_.CreateCharacter(class_,name);
    if(input.Pressed(VK_RETURN)) {if(!name.empty()) {draftName_=name;editingName_=false;focus_=Control::Enter;}return Action::None;}
    return Action::None;
  }
  if(input.Pressed(VK_RETURN) || input.Pressed(VK_SPACE)) return Activate(focus_);
  if(input.Pressed('R')) return Activate(Control::Refresh);
  if(input.Pressed(VK_LEFT)) return Activate(Control::Left);
  if(input.Pressed(VK_RIGHT)) return Activate(Control::Right);
  return Action::None;
}
}
