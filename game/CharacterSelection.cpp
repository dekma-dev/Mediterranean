#include "game/CharacterSelection.h"
#include "game/ProfileStorage.h"
#include "game/CastEffects.h"
#include "game/CharacterPose.h"
#include "engine/Renderer.h"
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
  if(page==Page::Roster)dress_=Dress::Equipped;
  page_=page;focus_=focus;hover_=Control::None;pageAge_=0;highlights_.fill(0);CancelGesture();ResetPreview();
}
void CharacterSelection::ReadSlots() {
  for(int i=0;i<int(slots_.size());++i) {
    rpg::Simulation hero;std::string snapshot;auto& slot=slots_[i];
    slot.status=ReadProfile(Path(rpg::ClassId(i)),rpg::ClassId(i),hero,snapshot);
    slot.name=slot.status==Status::Saved?hero.Nickname():"";slot.level=hero.Level();
  }
}
void CharacterSelection::Open(const std::filesystem::path& file,rpg::ClassId selected) {
  base_=file;file_=file;class_=int(selected)>=0 && selected<rpg::ClassId::Count?selected:rpg::ClassId::Warrior;
  draftName_.clear();look_={};notice_.clear();
  dress_=Dress::Equipped;
  callingView_=CallingView::Overview;branch_=0;
  time_=yaw_=width_=height_=zoom_=zoomGoal_=0;closeup_=false;ResetPreview();SetPage(Page::Roster,Control::Enter);Refresh();
  focus_=status_==Status::Saved?Control::Enter:status_==Status::New?Control::Create:Control::Refresh;
}
void CharacterSelection::Refresh() {
  CancelGesture();ResetPreview();changed_=false;notice_.clear();ReadSlots();file_=Path(class_);
  hero_=rpg::Simulation{};hero_.CreateCharacter(class_,rpg::DescribeClass(class_).title);
  status_=ReadProfile(file_,class_,hero_,snapshot_);
}
void CharacterSelection::PreviewDraft() {
  auto name=draftName_;while(!name.empty() && name.back()==' ') name.pop_back();
  hero_.CreateCharacter(class_,rpg::ValidCharacterName(name)?name:rpg::DescribeClass(class_).title,look_);
}
Rect CharacterSelection::Bounds(Control control) {
  if(control>=Control::Dress0 && control<=Control::Dress2) return {412+152.f*(int(control)-int(Control::Dress0)),100,148,30};
  if(control>=Control::Overview && control<=Control::Talents) return {34+94.f*(int(control)-int(Control::Overview)),236,90,30};
  if(control>=Control::Art0 && control<=Control::Art13) {
    const int n=int(control)-int(Control::Art0);return {34+40.f*(n%7),280+44.f*(n/7),38,42};
  }
  if(control>=Control::Branch0 && control<=Control::Branch2) return {34+94.f*(int(control)-int(Control::Branch0)),280,90,30};
  if(control>=Control::Skin0 && control<=Control::Dye4) {
    const int n=int(control)-int(Control::Skin0);
    return {1047+33.f*(n%5),284+82.f*(n/5),30,30};
  }
  if(control>=Control::Class0 && control<=Control::Class10) return {1004,112+44.f*(int(control)-int(Control::Class0)),244,42};
  switch(control) {
  case Control::Name:return {34,316,280,40};
  case Control::Hero:return {34,182,280,128};
  case Control::Create:return {34,526,280,44};
  case Control::Next:case Control::Confirm:case Control::Enter:return {1044,648,204,48};
  case Control::Back:case Control::Refresh:return {32,654,130,38};
  case Control::Cancel:case Control::Quit:return {174,654,130,38};
  case Control::Left:return {482,606,36,30};
  case Control::Right:return {784,606,36,30};
  case Control::Closeup:return {398,654,154,38};
  case Control::Pose:return {560,654,154,38};
  case Control::ResetView:return {722,654,138,38};
  case Control::Showcase:return {876,654,154,38};
  case Control::SkinLeft:return {1008,284,30,30};case Control::SkinRight:return {1220,284,30,30};
  case Control::HairLeft:return {1008,366,30,30};case Control::HairRight:return {1220,366,30,30};
  case Control::DyeLeft:return {1008,448,30,30};case Control::DyeRight:return {1220,448,30,30};
  default:return {};
  }
}
bool CharacterSelection::Enabled(Control control) const {
  if(control==Control::None) return false;
  if((control>=Control::Dress0 && control<=Control::Dress2) || control==Control::Showcase)
    return page_!=Page::Roster || status_!=Status::Unreadable;
  if(control>=Control::Overview && control<=Control::Talents) return page_==Page::Class;
  if(control>=Control::Art0 && control<=Control::Art13) return page_==Page::Class && callingView_==CallingView::Arts && int(control)-int(Control::Art0)<PreviewArtCount();
  if(control>=Control::Branch0 && control<=Control::Branch2) return page_==Page::Class && callingView_==CallingView::Talents;
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
  if(control>=Control::Dress0 && control<=Control::Dress2) {
    dress_=Dress(int(control)-int(Control::Dress0));ResetPreview();audition_.Add(sound::Cue::UiClick);return Action::None;
  }
  if(control>=Control::Overview && control<=Control::Talents) {
    callingView_=CallingView(int(control)-int(Control::Overview));CancelGesture();return Action::None;
  }
  if(control>=Control::Branch0 && control<=Control::Branch2) {branch_=int(control)-int(Control::Branch0);return Action::None;}
  if(control>=Control::Art0 && control<=Control::Art13) {
    showcase_=false;
    StartPreview(previewArts[int(control)-int(Control::Art0)]);return Action::None;
  }
  if(control>=Control::Class0 && control<=Control::Class10) {
    ResetPreview();
    class_=rpg::ClassId(int(control)-int(Control::Class0));Refresh();
    if(page_==Page::Class) {PreviewDraft();focus_=Enabled(Control::Next)?Control::Next:control;}
    else focus_=status_==Status::Saved?Control::Enter:status_==Status::New?Control::Create:Control::Refresh;
    return Action::None;
  }
  switch(control) {
  case Control::Closeup:closeup_=!closeup_;zoomGoal_=closeup_?1.f:0.f;break;
  case Control::Pose:showcase_=false;StartPreview(previewAbility_==0?1:previewAbility_==1?5:previewAbility_==5?6:previewAbility_==6?7:0);break;
  case Control::Showcase:
    if(showcase_)ResetPreview();
    else {showcase_=true;showcaseStep_=0;showcaseRest_=0;StartPreview(0);}
    break;
  case Control::ResetView:closeup_=false;zoomGoal_=0;yaw_=0;dress_=Dress::Equipped;ResetPreview();break;
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
void CharacterSelection::ResetPreview() {
  showcase_=false;showcaseRest_=0;audition_.Reset();
  actionTime_=-1;previewAbility_=-1;released_=false;previewController_.Reset();previewWeapon_={};if(effects_.value)effects_->Reset();
}
void CharacterSelection::StartPreview(int ability) {
  closeup_=false;zoomGoal_=0;previewAbility_=ability;actionTime_=0;released_=false;
  effects_->Reset();previewWeapon_.last=-1;
  audition_.Reset();
  audition_.preparing=ability==3?hero_.RecoveryCastDuration()>0:ability!=2 && hero_.ClassInfo().range>3;
  if(audition_.preparing)audition_.Add(sound::PreparationCue(class_,ability));
  if(ability==2)audition_.Add(sound::Cue::Dash);
}
float CharacterSelection::PreviewRate() const {
  return previewAbility_==2 || previewAbility_==3?1.f:previewAbility_>=8?rpg::AdvancedRate(class_,previewAbility_):
    rpg::ClassAttackRate(class_,PreviewHeavy())*
      (previewAbility_==7?.85f:previewAbility_==6?.7f:previewAbility_==5?.8f:1.f);
}
CharacterOutfit CharacterSelection::PreviewOutfit() const {
  auto outfit=CharacterOutfit::From(hero_.Items(),class_,hero_.Look());
  if(dress_==Dress::Equipped)return outfit;
  // Reuse real interchangeable pieces without granting items or modifying a save.
  const bool veteran=dress_==Dress::Veteran;
  outfit.weapon=outfit.tunic=outfit.trousers=outfit.boots=outfit.belt=true;
  outfit.armor=outfit.rewardWeapon=outfit.shoulders=outfit.gloves=outfit.cloak=veteran;
  outfit.helm=false;
  outfit.shield=veteran && (class_==rpg::ClassId::Warrior || class_==rpg::ClassId::Paladin);
  return outfit;
}
animation::Sample CharacterSelection::PreviewBody() const {
  auto pose=animation::Idle(time_);pose.action=previewController_.pose;
  const auto form=rpg::WeaponFor(class_).form;
  const bool staff=rpg::StaffWeapon(form),bow=form==rpg::WeaponForm::Handbow;
  const bool blade=form==rpg::WeaponForm::Blade || form==rpg::WeaponForm::Mace || form==rpg::WeaponForm::Cleaver;
  const bool hasWeapon=PreviewOutfit().weapon;
  pose.authoredGait=true;pose.authoredClock=time_;
  pose.authoredEquipment=!hasWeapon?playerclips::Equipment::None:staff?playerclips::Equipment::Staff:
    bow?playerclips::Equipment::Bow:blade?playerclips::Equipment::Sword:playerclips::Equipment::None;
  pose.authoredStrike=previewWeapon_.frame;pose.authoredStrikeWeight=previewWeapon_.weight;
  pose.authoredStrikePhase=previewWeapon_.phase;pose.authoredStrikeHeavy=previewWeapon_.heavy;
  pose.authoredAccent=previewWeapon_.accent;pose.authoredReady=previewWeapon_.ready;
  if(actionTime_>=0 && previewAbility_!=2 && previewAbility_!=3 && !animation::RangedClass(class_)) {
    pose.attackSeconds=actionTime_;pose.heavyAttack=PreviewHeavy();
  }
  if(actionTime_>=0 && previewAbility_==2) {
    pose.dashPhase=std::clamp(actionTime_/.34f,0.f,1.f);pose.dash=motion::DashSpeed(pose.dashPhase);
    pose.dashBrake=animation::DashBrake(pose.dashPhase)*(1-animation::SmoothStep((actionTime_-.34f)/.16f));
    pose.retreat=hero_.RetreatMobility();
  }
  if(actionTime_>=0 && previewAbility_==3 && class_==rpg::ClassId::Warrior)
    pose.cast=animation::Gesture(actionTime_,.18f,.72f);
  return pose;
}
void CharacterSelection::RenderEffects(Renderer& renderer) const {
  const auto outfit=PreviewOutfit();
  const auto form=rpg::WeaponFor(class_).form;
  const auto pose=BuildCharacterPose(0,0,outfit.shield,PreviewBody(),form);
  auto sockets=castfx::FromPose(pose,{0,0,0},yaw_,form,outfit.shield,outfit.weapon);sockets.terrain=false;
  if(actionTime_>=0 && (hero_.ClassInfo().range>3 || previewAbility_==3) && previewAbility_!=2)
    castfx::PrepareAction(renderer,class_,previewAbility_,PreviewHeavy(),
      actionTime_,PreviewRate(),previewAbility_==3,actionTime_/std::max(.001f,hero_.RecoveryCastDuration()),sockets);
  if(effects_.value)effects_->Render(renderer,&sockets);
}
CharacterSelection::Action CharacterSelection::Update(float seconds,const Input& input,float wheel) {
  audition_.ClearFrame();
  if(!input.Focused()) {CancelGesture();ResetPreview();hover_=Control::None;return Action::None;}
  if(!std::isfinite(seconds))seconds=0;
  time_=std::fmod(time_+std::clamp(seconds,0.f,.05f),600.f);
  const float uiSeconds=std::clamp(seconds,0.f,.05f);
  pageAge_=std::min(1.f,pageAge_+uiSeconds);
  for(int i=0;i<int(Control::None);++i) {
    const auto control=Control(i);const float goal=Enabled(control) && (hover_==control || focus_==control)?1.f:0.f;
    highlights_[i]+=(goal-highlights_[i])*(1-std::exp(-14.f*uiSeconds));
  }
  const float dt=std::clamp(seconds,0.f,.05f);
  zoom_+=(zoomGoal_-zoom_)*-std::expm1(-12.f*dt);
  if(effects_.value)effects_->Advance(dt);bool emitRelease=false;float releaseElapsed=0;
  if(actionTime_>=0) {
    const float previousTime=actionTime_;
    actionTime_+=dt*PreviewRate();
    const bool heavy=PreviewHeavy();
    const float audibleRelease=previewAbility_==3?hero_.RecoveryCastDuration():combat::StrokeStart(heavy);
    if(previewAbility_!=2 && (previousTime<audibleRelease || (!released_ && audibleRelease==0)) && actionTime_>=audibleRelease) {
      audition_.preparing=false;
      audition_.Add(previewAbility_==3?sound::ClassRecoveryCue(class_):hero_.ClassInfo().range>3?
        sound::ReleaseCue(class_,previewAbility_,heavy):heavy?sound::Cue::Heavy:sound::Cue::Strike);
      if(class_==rpg::ClassId::Warrior && heavy)audition_.Add(sound::Cue::WarriorEffort);
    }
    const float release=previewAbility_==3?hero_.RecoveryCastDuration():hero_.ClassInfo().range>3?combat::StrokeStart(heavy):combat::Contact(heavy);
    if(!released_ && previewAbility_!=2 && actionTime_>=release) {released_=true;emitRelease=true;releaseElapsed=std::max(0.f,(actionTime_-release)/PreviewRate());}
    const float duration=previewAbility_==2?.50f:previewAbility_==3?hero_.RecoveryCastDuration()+.42f:animation::ActionDuration(!PreviewHeavy()?animation::ActionClip::Basic:animation::ActionClip::Heavy);
    if(actionTime_>=duration) actionTime_=-1;
  }
  const float recovery=hero_.RecoveryCastDuration();
  const float recoveryPose=previewAbility_!=3 || actionTime_<0?-1:actionTime_<recovery?
    std::min(.30f,actionTime_/std::max(.001f,recovery)*1.5f):.30f+actionTime_-recovery;
  previewController_.Update(dt,class_,previewAbility_==2 || previewAbility_==3?-1:actionTime_,PreviewHeavy(),
    recoveryPose,true,previewAbility_==5,previewAbility_==6,previewAbility_==7);
  const auto form=rpg::WeaponFor(class_).form;
  const bool staff=rpg::StaffWeapon(form),bow=form==rpg::WeaponForm::Handbow,prayer=form==rpg::WeaponForm::Scepter;
  const bool blade=form==rpg::WeaponForm::Blade || form==rpg::WeaponForm::Mace || form==rpg::WeaponForm::Cleaver;
  previewWeapon_.Update(dt,previewAbility_==2 || previewAbility_==3?-1:actionTime_,PreviewHeavy(),
    staff||prayer,PreviewOutfit().weapon && (staff||bow||blade||prayer),PreviewRate(),bow,
    previewAbility_,class_==rpg::ClassId::Warrior?0:class_==rpg::ClassId::Scout?1:class_==rpg::ClassId::Mage?2:class_==rpg::ClassId::Priest?3:-1);
  if(emitRelease) {
      const auto outfit=PreviewOutfit();
      const auto pose=BuildCharacterPose(0,0,outfit.shield,PreviewBody(),form);
      const auto sockets=castfx::FromPose(pose,{0,0,0},yaw_,form,outfit.shield,outfit.weapon);
      rpg::CombatVisualEvent e;e.school=class_;e.ability=previewAbility_;e.facing=yaw_;e.heavy=PreviewHeavy();
      e.technique=previewAbility_==5;e.signature=previewAbility_==6;e.opening=previewAbility_==7;
      e.kind=previewAbility_==3?rpg::CombatVisualKind::Mend:hero_.ClassInfo().range>3?rpg::CombatVisualKind::SpellCast:
        PreviewHeavy()?rpg::CombatVisualKind::HeavyStrike:rpg::CombatVisualKind::Strike;
      e.destination={std::sin(yaw_)*1.7f,std::cos(yaw_)*1.7f};e.destinationHeight=1.2f;e.flight=.42f;
      e.elapsed=releaseElapsed;effects_->Emit(e,&sockets.focus);
    }
  if(effects_.value && effects_->Tracking(0)) {
    const auto outfit=PreviewOutfit();
    const auto pose=BuildCharacterPose(0,0,outfit.shield,PreviewBody(),form);
    const auto world=[&](DirectX::XMFLOAT3 p) {return DirectX::XMFLOAT3{std::cos(yaw_)*p.x+std::sin(yaw_)*p.z,p.y,-std::sin(yaw_)*p.x+std::cos(yaw_)*p.z};};
    effects_->TrackWeapon(0,world(pose.WeaponPoint(0)),world(pose.WeaponPoint(.8f)));
  }
  const bool resized=width_!=input.Width() || height_!=input.Height();width_=input.Width();height_=input.Height();
  if(resized || input.Pressed(VK_F11)) {CancelGesture();hover_=Control::None;return Action::None;}
  const Layout layout(width_,height_);hover_=Control::None;
  const float localX=layout.LocalX(input.ClientX()),localY=layout.LocalY(input.ClientY());
  const bool stage=StageBounds().Contains(localX,localY) && Enabled(Control::Left);
  for(int i=0;i<int(Control::None);++i) {
    const auto control=Control(i);
    if(Enabled(control) && Bounds(control).Contains(layout.LocalX(input.ClientX()),layout.LocalY(input.ClientY()))) hover_=control;
  }
  if(showcase_ && actionTime_<0 && !rotating_ && !input.Held(VK_LBUTTON) && !(page_==Page::Customize && focus_==Control::Name)) {
    showcaseRest_+=dt;
    if(showcaseRest_>=1.4f) {
      constexpr int sequence[]={0,1,5,6};showcaseStep_=(showcaseStep_+1)%4;showcaseRest_=0;
      StartPreview(sequence[showcaseStep_]);
    }
  }
  if(std::isfinite(wheel) && wheel!=0 && stage && hover_==Control::None) {
    zoomGoal_=std::clamp(zoomGoal_+wheel*.18f,0.f,1.f);closeup_=zoomGoal_>.5f;
  }
  if(input.Pressed(VK_TAB)) {
    const int direction=input.Held(VK_SHIFT)?-1:1;
    do {focus_=Control((int(focus_)+direction+int(Control::None))%int(Control::None));} while(!Enabled(focus_));
  }
  if(input.Pressed(VK_ESCAPE)) {
    CancelGesture();if(page_!=Page::Roster) return Activate(Control::Back);
    focus_=Control::Quit;return Action::None;
  }
  if(input.Pressed(VK_LBUTTON)) {
    pressed_=hover_;if(hover_!=Control::None) focus_=hover_;
    rotating_=stage && hover_==Control::None;dragX_=localX;
  }
  if(rotating_ && input.Held(VK_LBUTTON)) {
    yaw_=std::remainder(yaw_+std::clamp(localX-dragX_,-150.f,150.f)*.012f,6.2831853f);dragX_=localX;
  }
  if(input.Released(VK_LBUTTON)) {
    const auto clicked=pressed_;const bool dragged=rotating_;CancelGesture();if(!dragged && clicked==hover_) return Activate(clicked);
  }
  if(focus_==Control::Name && page_==Page::Customize) {
    const auto previousName=draftName_;
    const bool control=input.Held(VK_CONTROL),alt=input.Held(VK_MENU);
    if(control && !alt && input.Pressed('A')) draftName_.clear();
    if(!control && !alt) {
      if(input.NativeText()) {
        for(std::size_t offset=0;offset<input.Characters().size() && rpg::NameLength(draftName_)<20;) {
          const auto begin=offset;const auto ch=text::Next(input.Characters(),offset);
          if(rpg::NameLetter(ch) || ch=='-' || ch=='\'' ||
             (ch==' ' && !draftName_.empty() && draftName_.back()!=' ' && rpg::NameLength(draftName_)<19))
            draftName_+=input.Characters().substr(begin,offset-begin);
        }
      } else {
        // Deterministic synthetic input used by tools/tests without a message loop.
        for(int key='A';key<='Z';++key) if(input.Pressed(key) && rpg::NameLength(draftName_)<20) draftName_+=char((input.Held(VK_SHIFT)!=input.CapsLock())?key:key+32);
        if(input.Pressed(VK_SPACE) && !draftName_.empty() && draftName_.back()!=' ' && rpg::NameLength(draftName_)<19) draftName_+=' ';
        for(auto key:{VK_OEM_MINUS,VK_OEM_7}) if(input.Pressed(key) && rpg::NameLength(draftName_)<20) draftName_+=key==VK_OEM_MINUS?'-':'\'';
      }
      if(input.Pressed(VK_BACK) && !draftName_.empty()) {
        auto begin=draftName_.size()-1;
        while(begin && (static_cast<unsigned char>(draftName_[begin])&0xc0)==0x80) --begin;
        draftName_.erase(begin);
      }
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
