#pragma once
#include "game/Interface.h"
#include "engine/Input.h"
#include "game/ActionClips.h"
#include "game/CharacterView.h"
#include "game/CombatEffects.h"
#include "game/NameEditor.h"
#include "engine/Audio.h"
#include <filesystem>
#include <memory>

namespace ui {
// A new profile is published only by Confirm, using an exclusive file move.
// Roster entry uses the same validated snapshot displayed to the player.
class CharacterSelection {
public:
  enum class Status { New, Saved, Unreadable };
  enum class Page { Roster, Class, Customize };
  enum class CallingView { Overview, Arts, Talents };
  enum class Dress { Equipped, Travel, Veteran };
  enum class Control { Hero, Enter, Refresh, Quit, Left, Right, Name,
    Class0,Class1,Class2,Class3,Class4,Class5,Class6,Class7,Class8,Class9,Class10,
    Create,Next,Back,Cancel,Confirm,SkinLeft,SkinRight,HairLeft,HairRight,DyeLeft,DyeRight,
    Skin0,Skin1,Skin2,Skin3,Skin4,Hair0,Hair1,Hair2,Hair3,Hair4,Dye0,Dye1,Dye2,Dye3,Dye4,
    Closeup,Pose,ResetView,Overview,Arts,Talents,Art0,Art1,Art2,Art3,Art4,Art5,Art6,Art7,Art8,Art9,Art10,Art11,Art12,Art13,Branch0,Branch1,Branch2,Dress0,Dress1,Dress2,Showcase,Replay,Stop,None };
  enum class Action { None, Enter, Quit };
  struct Slot { Status status=Status::New; std::string name; int level=1; };
  struct StageView {
    rpg::ClassId school=rpg::ClassId::Warrior;
    CharacterOutfit outfit{};
    animation::Sample body{};
    float yaw=0,zoom=0,opacity=1;
    bool effects=true;
  };
  void Open(const std::filesystem::path& file,rpg::ClassId selected=rpg::ClassId::Warrior);
  void Refresh();
  const std::filesystem::path& SaveFile() const { return file_; }
  rpg::ClassId SelectedClass() const { return class_; }
  Action Update(float seconds,const Input& input,float wheel=0);
  void CancelGesture() { pressed_=Control::None; rotating_=nameDragging_=false; nameRepeatKey_=0; yawGoal_=yaw_; hintAge_=0; }
  void SuspendPreview() { CancelGesture();ResetPreview();stageAge_=1; }
  static Rect Bounds(Control control);
  static Rect StageBounds() { return {326,70,650,550}; }
  bool Enabled(Control control) const;
  const rpg::Simulation& Hero() const { return hero_; }
  const std::string& Snapshot() const { return snapshot_; }
  Status State() const { return status_; }
  Page Screen() const { return page_; }
  CallingView CallingScreen() const { return callingView_; }
  int CallingBranch() const { return branch_; }
  static constexpr std::array<int,14> previewArts{0,1,2,3,5,6,7,12,13,14,8,9,10,11};
  int PreviewArtCount() const { return rpg::AdvancedClass(class_)?14:7; }
  const Slot& ClassSlot(int index) const { return slots_.at(index); }
  Control Focus() const { return focus_; }
  Control Hover() const { return hover_; }
  Control Hint() const { return hintAge_>=.4f && !rotating_ && pressed_==Control::None?hint_:Control::None; }
  bool Pressed(Control control) const { return pressed_==control && hover_==control; }
  float Highlight(Control control) const { return highlights_[int(control)]; }
  float PageAge() const { return pageAge_; }
  float Time() const { return time_; }
  float Yaw() const { return yaw_; }
  float PreviewZoom() const { return zoom_; }
  bool Closeup() const { return closeup_; }
  float PreviewAction() const { return actionTime_; }
  struct ArtTiming { float progress=0,release=0,follow=0; const char* phase="Ready to demonstrate"; };
  ArtTiming PreviewTiming() const;
  int PreviewAbility() const { return previewAbility_; }
  const animation::ActionChannels& PreviewPose() const { return previewController_.pose; }
  animation::Sample PreviewBody() const;
  CharacterOutfit PreviewOutfit() const;
  StageView PreviewStage() const;
  Dress PreviewDress() const { return dress_; }
  bool Showcasing() const { return showcase_; }
  bool Introducing() const { return automaticPreview_ && actionTime_>=0; }
  const sound::Audition& PreviewAudio() const { return audition_; }
  void RenderEffects(Renderer& renderer) const;
  bool Rotating() const { return rotating_; }
  std::size_t PreviewEffectCount() const { return effects_.value?effects_->BurstCount()+effects_->TrailCount():0; }
  const std::string& DraftName() const { return draftName_; }
  const NameEditor& NameSelection() const { return nameEditor_; }
  void MeasureName(const std::array<float,21>& advances) const {nameAdvances_=advances;measuredName_=draftName_;}
  const std::string& Notice() const { return notice_; }
  bool Changed() const { return changed_; }
private:
  std::filesystem::path file_,base_;
  rpg::ClassId class_=rpg::ClassId::Warrior;
  std::array<Slot,int(rpg::ClassId::Count)> slots_;
  Page page_=Page::Roster;
  CallingView callingView_=CallingView::Overview;
  int branch_=0;
  rpg::Appearance look_;
  std::string draftName_,notice_;
  NameEditor nameEditor_;
  bool nameDragging_=false;
  int nameRepeatKey_=0;
  float nameRepeatDelay_=0;
  mutable std::string measuredName_;
  mutable std::array<float,21> nameAdvances_{};
  void PickName(float x,bool extend);
  rpg::Simulation hero_;
  std::string snapshot_;
  Status status_=Status::New;
  Control focus_=Control::Enter,hover_=Control::None,pressed_=Control::None;
  float time_=0,yaw_=0,width_=0,height_=0;
  bool changed_=false;
  bool closeup_=false;
  bool rotating_=false,released_=false;
  Dress dress_=Dress::Equipped;
  bool showcase_=false;
  bool automaticPreview_=false;
  float introduction_=-1;
  int showcaseStep_=0;
  float showcaseRest_=0;
  sound::Audition audition_;
  float dragX_=0,zoomGoal_=0,yawGoal_=0,hintAge_=0;
  float hintPointerX_=0,hintPointerY_=0;
  Control hint_=Control::None;
  bool keyboardHints_=false;
  float zoom_=0,actionTime_=-1;
  bool previewFinished_=false;
  StageView previousStage_{};
  float stageAge_=1;
  int previewAbility_=-1;
  animation::ActionController previewController_;
  animation::AuthoredWeaponController previewWeapon_;
  // Do not add a second large fixed VFX pool to every stack-allocated Game.
  // Copies own their audition state; an unopened roster allocates nothing.
  struct PreviewEffects {
    std::unique_ptr<CombatEffects> value;
    PreviewEffects()=default;
    PreviewEffects(const PreviewEffects& other):value(other.value?std::make_unique<CombatEffects>(*other.value):nullptr) {}
    PreviewEffects& operator=(const PreviewEffects& other) {
      if(this!=&other)value=other.value?std::make_unique<CombatEffects>(*other.value):nullptr;
      return *this;
    }
    PreviewEffects(PreviewEffects&&)=default;
    PreviewEffects& operator=(PreviewEffects&&)=default;
    CombatEffects* operator->() { if(!value)value=std::make_unique<CombatEffects>();return value.get(); }
    const CombatEffects* operator->() const { return value.get(); }
  } effects_;
  float pageAge_=0;
  std::array<float,int(Control::None)+1> highlights_{};
  std::filesystem::path Path(rpg::ClassId id) const;
  void ReadSlots();
  void PreviewDraft();
  void SetPage(Page page,Control focus);
  void CreateProfile();
  void StartPreview(int ability);
  void ResetPreview();
  void QueueIntroduction();
  float PreviewRate() const;
  float PreviewDuration() const;
  float PreviewRelease() const;
  bool PreviewHeavy() const { return previewAbility_!=0 && previewAbility_!=7 && previewAbility_!=12; }
  Action Activate(Control control);
};
void RenderCharacterSelection(Renderer& renderer,const CharacterSelection& selection);
}
