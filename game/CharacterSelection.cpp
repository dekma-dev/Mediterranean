#include "game/CharacterSelection.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace ui {
void CharacterSelection::Open(const std::filesystem::path& file) {
  file_=file; time_=yaw_=width_=height_=0; hover_=pressed_=Control::None;
  Refresh(); focus_=status_==Status::Unreadable?Control::Refresh:Control::Enter;
}
void CharacterSelection::Refresh() {
  CancelGesture(); changed_=false; snapshot_.clear(); hero_=rpg::Simulation{};
  status_=Status::Unreadable;
  std::error_code error;
  const bool exists=std::filesystem::exists(file_,error);
  if(error) return;
  if(!exists) {status_=Status::New;return;}
  const auto length=std::filesystem::file_size(file_,error);
  if(error || length==0 || length>4096) return;
  std::ifstream file(file_,std::ios::binary);
  std::string content(static_cast<std::size_t>(length),'\0');
  if(!file.read(content.data(),static_cast<std::streamsize>(length)) || file.peek()!=std::char_traits<char>::eof()) return;
  if(!hero_.Load(content)) return;
  snapshot_=std::move(content); status_=Status::Saved;
}
Rect CharacterSelection::Bounds(Control control) {
  switch(control) {
  case Control::Hero:return {842,182,350,128};
  case Control::Enter:return {842,532,350,52};
  case Control::Refresh:return {842,602,168,36};
  case Control::Quit:return {1024,602,168,36};
  case Control::Left:return {282,604,44,32};
  case Control::Right:return {510,604,44,32};
  default:return {};
  }
}
bool CharacterSelection::Enabled(Control control) const {
  return control!=Control::None && (status_!=Status::Unreadable || control==Control::Refresh || control==Control::Quit);
}
CharacterSelection::Action CharacterSelection::Activate(Control control) {
  if(!Enabled(control)) return Action::None;
  switch(control) {
  case Control::Hero:focus_=Control::Enter;break;
  case Control::Enter: {
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
  if(input.Pressed(VK_RETURN) || input.Pressed(VK_SPACE)) return Activate(focus_);
  if(input.Pressed('R')) return Activate(Control::Refresh);
  if(input.Pressed(VK_LEFT)) return Activate(Control::Left);
  if(input.Pressed(VK_RIGHT)) return Activate(Control::Right);
  return Action::None;
}
}
