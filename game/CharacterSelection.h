#pragma once
#include "game/Interface.h"
#include "engine/Input.h"
#include <filesystem>

namespace ui {
// A new profile is published only by Confirm, using an exclusive file move.
// Roster entry uses the same validated snapshot displayed to the player.
class CharacterSelection {
public:
  enum class Status { New, Saved, Unreadable };
  enum class Page { Roster, Class, Customize };
  enum class Control { Hero, Enter, Refresh, Quit, Left, Right, Name,
    Class0,Class1,Class2,Class3,Class4,Class5,Class6,Class7,Class8,Class9,Class10,
    Create,Next,Back,Cancel,Confirm,SkinLeft,SkinRight,HairLeft,HairRight,DyeLeft,DyeRight,
    Skin0,Skin1,Skin2,Skin3,Skin4,Hair0,Hair1,Hair2,Hair3,Hair4,Dye0,Dye1,Dye2,Dye3,Dye4,
    Closeup,Pose,ResetView,None };
  enum class Action { None, Enter, Quit };
  struct Slot { Status status=Status::New; std::string name; int level=1; };
  void Open(const std::filesystem::path& file);
  void Refresh();
  const std::filesystem::path& SaveFile() const { return file_; }
  rpg::ClassId SelectedClass() const { return class_; }
  Action Update(float seconds,const Input& input);
  void CancelGesture() { pressed_=Control::None; }
  static Rect Bounds(Control control);
  bool Enabled(Control control) const;
  const rpg::Simulation& Hero() const { return hero_; }
  const std::string& Snapshot() const { return snapshot_; }
  Status State() const { return status_; }
  Page Screen() const { return page_; }
  const Slot& ClassSlot(int index) const { return slots_.at(index); }
  Control Focus() const { return focus_; }
  Control Hover() const { return hover_; }
  float Time() const { return time_; }
  float Yaw() const { return yaw_; }
  float PreviewZoom() const { return zoom_; }
  bool Closeup() const { return closeup_; }
  float PreviewAction() const { return actionTime_; }
  const std::string& DraftName() const { return draftName_; }
  const std::string& Notice() const { return notice_; }
  bool Changed() const { return changed_; }
private:
  std::filesystem::path file_,base_;
  rpg::ClassId class_=rpg::ClassId::Warrior;
  std::array<Slot,int(rpg::ClassId::Count)> slots_;
  Page page_=Page::Roster;
  rpg::Appearance look_;
  std::string draftName_,notice_;
  rpg::Simulation hero_;
  std::string snapshot_;
  Status status_=Status::New;
  Control focus_=Control::Enter,hover_=Control::None,pressed_=Control::None;
  float time_=0,yaw_=0,width_=0,height_=0;
  bool changed_=false;
  bool closeup_=false;
  float zoom_=0,actionTime_=-1;
  std::filesystem::path Path(rpg::ClassId id) const;
  void ReadSlots();
  void PreviewDraft();
  void SetPage(Page page,Control focus);
  void CreateProfile();
  Action Activate(Control control);
};
void RenderCharacterSelection(Renderer& renderer,const CharacterSelection& selection);
}
