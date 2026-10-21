#pragma once
#include "game/Interface.h"
#include "engine/Input.h"
#include <filesystem>

namespace ui {
// One local slot per class; the legacy warrior uses patrol.save. Selection
// never replaces a character; entering uses the same validated snapshot shown in the card.
class CharacterSelection {
public:
  enum class Status { New, Saved, Unreadable };
  enum class Control { Hero, Enter, Refresh, Quit, Left, Right, Name, Class0,Class1,Class2,Class3,Class4,Class5,Class6,Class7,Class8,Class9,Class10, None };
  enum class Action { None, Enter, Quit };
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
  Control Focus() const { return focus_; }
  Control Hover() const { return hover_; }
  float Time() const { return time_; }
  float Yaw() const { return yaw_; }
  const std::string& DraftName() const { return draftName_; }
  bool Changed() const { return changed_; }
private:
  std::filesystem::path file_,base_;
  rpg::ClassId class_=rpg::ClassId::Warrior;
  std::string draftName_;
  bool editingName_=false;
  rpg::Simulation hero_;
  std::string snapshot_;
  Status status_=Status::New;
  Control focus_=Control::Enter,hover_=Control::None,pressed_=Control::None;
  float time_=0,yaw_=0,width_=0,height_=0;
  bool changed_=false;
  Action Activate(Control control);
};
void RenderCharacterSelection(Renderer& renderer,const CharacterSelection& selection);
}
