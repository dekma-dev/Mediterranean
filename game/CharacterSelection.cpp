#include "game/CharacterSelection.h"
#include "game/ProfileStorage.h"
#include "game/CastEffects.h"
#include "game/CharacterPose.h"
#include "engine/Renderer.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace ui {
namespace {
CharacterSelection::Status ReadProfile(const std::filesystem::path& path,rpg::ClassId id,
    rpg::Simulation& hero,std::string& snapshot) {
  using S=CharacterSelection::Status;
  snapshot.clear();std::string content;const auto read=profile::Read(path,content);
  if(read==profile::ReadResult::Missing) return S::New;
  rpg::Simulation candidate;
  if(read!=profile::ReadResult::Ready || !candidate.Load(content) || (rpg::ValidClass(id) && candidate.Class()!=id)) return S::Unreadable;
  hero=std::move(candidate);snapshot=std::move(content);return S::Saved;
}
}
std::filesystem::path CharacterSelection::Path(rpg::ClassId id) const {
  return id==rpg::ClassId::Warrior?base_:base_.parent_path()/("hero-"+std::to_string(int(id))+".save");
}
std::filesystem::path CharacterSelection::AdditionalPath(int slot) const {
  return base_.parent_path()/(base_.stem().wstring()+L"-character-"+std::to_wstring(slot)+L".save");
}
std::filesystem::path CharacterSelection::AvailablePath() const {
  std::string ignored;
  auto path=Path(class_);
  if(profile::Read(path,ignored)==profile::ReadResult::Missing)return path;
  for(int slot=1;slot<=maxProfiles;++slot) {
    path=AdditionalPath(slot);
    if(profile::Read(path,ignored)==profile::ReadResult::Missing)return path;
  }
  return {};
}
int CharacterSelection::SelectedProfile() const {
  for(int i=0;i<ProfileCount();++i)if(profiles_[i].file==file_)return i;
  return -1;
}
void CharacterSelection::RevealProfile() {
  const int index=SelectedProfile();if(index>=0)profileOffset_=index/profilesPerPage*profilesPerPage;
}
void CharacterSelection::SelectProfile(int index) {
  if(index<0 || index>=ProfileCount())return;
  const auto outgoing=PreviewStage();const bool hadFigure=status_==Status::Saved;
  file_=profiles_[index].file;class_=rpg::ValidClass(profiles_[index].school)?profiles_[index].school:rpg::ClassId::Warrior;
  Refresh(false);RevealProfile();focus_=status_==Status::Saved?Control::Enter:Control::Refresh;
  if(hadFigure && status_==Status::Saved){previousStage_=outgoing;previousStage_.effects=false;stageAge_=0;}
}
void CharacterSelection::ApplyLook(rpg::Appearance look) {
  if(look.skin==look_.skin && look.hair==look_.hair && look.dye==look_.dye)return;
  if(lookHistorySize_==lookHistory_.size()) {
    std::move(lookHistory_.begin()+1,lookHistory_.end(),lookHistory_.begin());--lookHistorySize_;
  }
  lookHistory_[lookHistorySize_++]=look_;look_=look;PreviewDraft();audition_.Add(sound::Cue::UiClick);
}
void CharacterSelection::SetPage(Page page,Control focus) {
  stageAge_=1;
  pinnedTalent_=-1;
  if(page==Page::Roster)dress_=Dress::Equipped;
  page_=page;focus_=focus;hover_=Control::None;pageAge_=0;highlights_.fill(0);CancelGesture();ResetPreview();
  if(page==Page::Customize)nameEditor_.Place(draftName_,rpg::NameLength(draftName_));
}
void CharacterSelection::ReadSlots() {
  profiles_.clear();
  const auto inspect=[&](const std::filesystem::path& path,rpg::ClassId expected) {
    Profile entry;entry.file=path;entry.school=expected;std::string content;
    const auto read=profile::Read(path,content);
    if(read==profile::ReadResult::Missing){entry.status=Status::New;return entry;}
    if(read==profile::ReadResult::Ready) {
      rpg::Simulation hero;
      if(hero.Load(content) && (!rpg::ValidClass(expected) || hero.Class()==expected))
        {entry.status=Status::Saved;entry.name=hero.Nickname();entry.school=hero.Class();entry.level=hero.Level();}
    }
    return entry;
  };
  for(int i=0;i<int(slots_.size());++i) {
    auto entry=inspect(Path(rpg::ClassId(i)),rpg::ClassId(i));auto& slot=slots_[i];
    slot={entry.status,entry.name,entry.level};
    if(entry.status!=Status::New)profiles_.push_back(std::move(entry));
  }
  for(int slot=1;slot<=maxProfiles;++slot) {
    const auto path=AdditionalPath(slot);std::error_code error;
    if(!std::filesystem::exists(path,error))continue;
    auto entry=inspect(path,rpg::ClassId::Count);
    if(entry.status!=Status::New)profiles_.push_back(std::move(entry));
  }
  profileOffset_=std::min(profileOffset_,std::max(0,(ProfileCount()-1)/profilesPerPage)*profilesPerPage);
}
void CharacterSelection::Open(const std::filesystem::path& file,rpg::ClassId selected,const std::filesystem::path& preferred) {
  base_=file;file_=file;class_=int(selected)>=0 && selected<rpg::ClassId::Count?selected:rpg::ClassId::Warrior;
  draftName_.clear();look_={};notice_.clear();
  nameEditor_={};measuredName_.clear();nameAdvances_.fill(0);
  dress_=Dress::Equipped;
  draftFile_.clear();lookHistorySize_=0;profileOffset_=0;
  appearanceSeed_=static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
  callingView_=CallingView::Overview;branch_=0;
  file_=preferred.empty()?Path(class_):preferred;
  time_=yaw_=width_=height_=zoom_=zoomGoal_=0;closeup_=false;ResetPreview();SetPage(Page::Roster,Control::Enter);Refresh();
  if(preferred.empty() && status_==Status::New && !profiles_.empty()) {
    const auto ready=std::find_if(profiles_.begin(),profiles_.end(),[](const Profile& entry){return entry.status==Status::Saved;});
    SelectProfile(ready==profiles_.end()?0:int(ready-profiles_.begin()));stageAge_=1;
  }
  focus_=status_==Status::Saved?Control::Enter:status_==Status::New?Control::Create:Control::Refresh;
}
void CharacterSelection::Refresh(bool rescan) {
  stageAge_=1;
  CancelGesture();ResetPreview();changed_=false;notice_.clear();if(rescan)ReadSlots();
  hero_=rpg::Simulation{};hero_.CreateCharacter(class_,rpg::DescribeClass(class_).title);
  auto expected=rpg::ClassId::Count;
  for(int id=0;id<int(rpg::ClassId::Count);++id)if(file_==Path(rpg::ClassId(id))){expected=rpg::ClassId(id);break;}
  status_=ReadProfile(file_,expected,hero_,snapshot_);
  if(status_==Status::Saved)class_=hero_.Class();
  if(!rescan) {
    // Selection validates the clicked file, not every other campaign again.
    // Opening, explicit Refresh, creation and entry still rescan the roster.
    if(status_==Status::New)status_=Status::Unreadable;
    const int index=SelectedProfile();
    if(index>=0){auto& entry=profiles_[index];entry.status=status_;entry.name=status_==Status::Saved?hero_.Nickname():"";
      if(status_==Status::Saved){entry.school=class_;entry.level=hero_.Level();}}
  }
  RevealProfile();
}
void CharacterSelection::PreviewDraft() {
  auto name=draftName_;while(!name.empty() && name.back()==' ') name.pop_back();
  hero_.CreateCharacter(class_,rpg::ValidCharacterName(name)?name:rpg::DescribeClass(class_).title,look_);
}
void CharacterSelection::PickName(float x,bool extend) {
  const auto length=rpg::NameLength(draftName_);std::size_t position=length;
  if(measuredName_==draftName_) {
    x-=Bounds(Control::Name).x+10;
    for(std::size_t i=0;i<length;++i)if(x<(nameAdvances_[i]+nameAdvances_[i+1])*.5f){position=i;break;}
  }
  nameEditor_.Place(draftName_,position,extend);
}
int CharacterSelection::InspectedTalent() const {
  if(page_!=Page::Class || callingView_!=CallingView::Talents)return -1;
  const auto hint=Hint();
  if(hint>=Control::Talent0 && hint<=Control::Talent5)return (int(hint)-int(Control::Talent0))*3+branch_;
  return pinnedTalent_;
}
Rect CharacterSelection::Bounds(Control control) {
  if(control>=Control::Profile0 && control<=Control::Profile7)return {1004,112+56.f*(int(control)-int(Control::Profile0)),244,52};
  if(control>=Control::Talent0 && control<=Control::Talent5) {
    const int row=int(control)-int(Control::Talent0);
    return {row==1?57.f:row==2?239.f:148.f,326+64.f*(row<=2?std::min(row,1):row-1),52,42};
  }
  if(control>=Control::Dress0 && control<=Control::Dress2) return {472+42.f*(int(control)-int(Control::Dress0)),608,38,38};
  if(control>=Control::Overview && control<=Control::Talents) return {34+94.f*(int(control)-int(Control::Overview)),236,90,30};
  if(control>=Control::Art0 && control<=Control::Art13) {
    const int n=int(control)-int(Control::Art0);return {34+40.f*(n%7),280+44.f*(n/7),38,42};
  }
  if(control>=Control::Branch0 && control<=Control::Branch2) return {34+94.f*(int(control)-int(Control::Branch0)),280,90,30};
  if(control>=Control::Skin0 && control<=Control::Dye4) {
    const int n=int(control)-int(Control::Skin0);
    return {1047+33.f*(n%5),240+86.f*(n/5),30,22};
  }
  if(control>=Control::Class0 && control<=Control::Class10) return {1004,112+44.f*(int(control)-int(Control::Class0)),244,42};
  switch(control) {
  case Control::PreviousProfiles:return {1004,578,38,32};case Control::NextProfiles:return {1210,578,38,32};
  case Control::RandomAppearance:return {332,650,44,44};case Control::UndoAppearance:return {384,650,44,44};
  case Control::Name:return {34,316,280,40};
  case Control::Hero:return {34,182,280,128};
  case Control::Create:return {34,526,280,44};
  case Control::Next:case Control::Confirm:case Control::Enter:return {1044,648,204,48};
  case Control::Back:case Control::Refresh:return {32,654,130,38};
  case Control::Cancel:case Control::Quit:return {174,654,130,38};
  case Control::Left:return {422,608,36,38};
  case Control::Right:return {812,608,36,38};
  case Control::Closeup:return {616,608,38,38};
  case Control::Pose:return {660,608,38,38};
  case Control::ResetView:return {748,608,38,38};
  case Control::Showcase:return {704,608,38,38};
  case Control::Replay:return {34,604,136,32};
  case Control::Stop:return {178,604,136,32};
  case Control::SkinLeft:return {1150,202,32,32};case Control::SkinRight:return {1220,202,32,32};
  case Control::HairLeft:return {1150,288,32,32};case Control::HairRight:return {1220,288,32,32};
  case Control::DyeLeft:return {1150,374,32,32};case Control::DyeRight:return {1220,374,32,32};
  default:return {};
  }
}
bool CharacterSelection::Enabled(Control control) const {
  if(control==Control::None) return false;
  if(control>=Control::Profile0 && control<=Control::Profile7)return page_==Page::Roster && profileOffset_+int(control)-int(Control::Profile0)<ProfileCount();
  if(control==Control::PreviousProfiles || control==Control::NextProfiles)return page_==Page::Roster &&
    (control==Control::PreviousProfiles?profileOffset_>0:profileOffset_+profilesPerPage<ProfileCount());
  if(control==Control::RandomAppearance || control==Control::UndoAppearance)return page_==Page::Customize &&
    (control==Control::RandomAppearance || lookHistorySize_>0);
  if(control>=Control::Talent0 && control<=Control::Talent5)return page_==Page::Class && callingView_==CallingView::Talents;
  if(control==Control::Replay || control==Control::Stop)
    return page_==Page::Class && callingView_==CallingView::Arts &&
      (control==Control::Replay || actionTime_>=0 || showcase_ || PreviewEffectCount()>0);
  if((control>=Control::Dress0 && control<=Control::Dress2) || control==Control::Showcase)
    return page_!=Page::Roster || status_!=Status::Unreadable;
  if(control>=Control::Overview && control<=Control::Talents) return page_==Page::Class;
  if(control>=Control::Art0 && control<=Control::Art13) return page_==Page::Class && callingView_==CallingView::Arts && int(control)-int(Control::Art0)<PreviewArtCount();
  if(control>=Control::Branch0 && control<=Control::Branch2) return page_==Page::Class && callingView_==CallingView::Talents;
  if(control==Control::Closeup || control==Control::Pose || control==Control::ResetView)
    return page_!=Page::Roster || status_!=Status::Unreadable;
  if(control>=Control::Skin0 && control<=Control::Dye4) return page_==Page::Customize;
  if(control==Control::Left || control==Control::Right) return page_!=Page::Roster || status_!=Status::Unreadable;
  if(control>=Control::Class0 && control<=Control::Class10) return page_==Page::Class;
  if(page_==Page::Roster) {
    if(control==Control::Enter || control==Control::Hero) return status_==Status::Saved;
    if(control==Control::Refresh || control==Control::Quit) return true;
    if(control==Control::Create) return ProfileCount()<maxProfiles;
    return false;
  }
  if(control==Control::Back || control==Control::Cancel) return true;
  if(page_==Page::Class) return control==Control::Next && ProfileCount()<maxProfiles;
  if(control==Control::Name || (control>=Control::SkinLeft && control<=Control::DyeRight)) return true;
  return control==Control::Confirm && rpg::ValidCharacterName(draftName_) && ProfileCount()<maxProfiles;
}
void CharacterSelection::CreateProfile() {
  PreviewDraft();notice_.clear();ReadSlots();
  if(ProfileCount()>=maxProfiles){notice_="The character roster is full. Your draft is kept.";return;}
  if(draftFile_.empty())draftFile_=AvailablePath();
  const auto result=draftFile_.empty()?profile::WriteResult::Error:profile::Write(draftFile_,hero_.Save());
  if(result!=profile::WriteResult::Saved) {
    if(result==profile::WriteResult::Conflict){ReadSlots();draftFile_.clear();}
    notice_=result==profile::WriteResult::Conflict?"Another character was created. Your draft is kept; press Create again.":
      result==profile::WriteResult::Busy?"Another game is saving this class. Your draft is kept; try again.":
      "Could not save this character. Your draft is kept; try again.";
    return;
  }
  file_=draftFile_;draftFile_.clear();
  SetPage(Page::Roster,Control::Enter);Refresh();
  notice_=status_==Status::Saved?"Character created. Select Enter World to begin.":"Character saved, but unreadable. Refresh to check it.";
}
CharacterSelection::Action CharacterSelection::Activate(Control control) {
  if(!Enabled(control)) return Action::None;
  if(control>=Control::Profile0 && control<=Control::Profile7){SelectProfile(profileOffset_+int(control)-int(Control::Profile0));return Action::None;}
  if(control==Control::PreviousProfiles || control==Control::NextProfiles) {
    profileOffset_+=control==Control::PreviousProfiles?-profilesPerPage:profilesPerPage;CancelGesture();return Action::None;
  }
  if(control==Control::Left || control==Control::Right || control==Control::Closeup || control==Control::ResetView ||
     (control>=Control::Dress0 && control<=Control::Dress2))stageAge_=1;
  if(control>=Control::Dress0 && control<=Control::Dress2) {
    dress_=Dress(int(control)-int(Control::Dress0));ResetPreview();audition_.Add(sound::Cue::UiClick);return Action::None;
  }
  if(control>=Control::Overview && control<=Control::Talents) {
    callingView_=CallingView(int(control)-int(Control::Overview));pinnedTalent_=-1;CancelGesture();return Action::None;
  }
  if(control>=Control::Branch0 && control<=Control::Branch2) {branch_=int(control)-int(Control::Branch0);pinnedTalent_=-1;CancelGesture();return Action::None;}
  if(control>=Control::Talent0 && control<=Control::Talent5) {
    const int talent=(int(control)-int(Control::Talent0))*3+branch_;
    pinnedTalent_=pinnedTalent_==talent?-1:talent;audition_.Add(sound::Cue::UiClick);return Action::None;
  }
  if(control>=Control::Art0 && control<=Control::Art13) {
    showcase_=false;
    StartPreview(previewArts[int(control)-int(Control::Art0)]);return Action::None;
  }
  if(control>=Control::Class0 && control<=Control::Class10) {
    pinnedTalent_=-1;
    const bool changed=class_!=rpg::ClassId(int(control)-int(Control::Class0));
    const auto outgoing=PreviewStage();const bool hadFigure=status_!=Status::Unreadable;
    ResetPreview();
    class_=rpg::ClassId(int(control)-int(Control::Class0));draftFile_.clear();
    PreviewDraft();status_=Status::New;focus_=Enabled(Control::Next)?Control::Next:control;if(changed)QueueIntroduction();
    if(changed && hadFigure && status_!=Status::Unreadable) {
      previousStage_=outgoing;previousStage_.effects=false;stageAge_=0;
    }
    return Action::None;
  }
  switch(control) {
  case Control::RandomAppearance: {
    appearanceSeed_=appearanceSeed_*1664525u+1013904223u;
    constexpr int n=rpg::Appearance::count,combinations=n*n*n;
    const int current=look_.skin*n*n+look_.hair*n+look_.dye;
    const int next=(current+1+int(appearanceSeed_%(combinations-1)))%combinations;
    ApplyLook({next/(n*n),(next/n)%n,next%n});break;
  }
  case Control::UndoAppearance:look_=lookHistory_[--lookHistorySize_];PreviewDraft();audition_.Add(sound::Cue::UiClick);break;
  case Control::Replay:showcase_=false;StartPreview(rpg::ValidSpell(previewAbility_)?previewAbility_:0);break;
  case Control::Stop:
    // Cancel sound/effects immediately, but let both body controllers settle
    // through their normal interrupted-action blend on subsequent updates.
    introduction_=-1;automaticPreview_=showcase_=false;previewFinished_=false;
    actionTime_=-1;released_=false;showcaseRest_=0;audition_.Reset();
    contactRemaining_=-1;
    if(effects_.value)effects_->Reset();
    break;
  case Control::Closeup:closeup_=!closeup_;zoomGoal_=closeup_?1.f:0.f;break;
  case Control::Pose:showcase_=false;StartPreview(automaticPreview_?0:previewAbility_==0?1:previewAbility_==1?5:previewAbility_==5?6:previewAbility_==6?7:0);break;
  case Control::Showcase:
    if(showcase_)ResetPreview();
    else {showcase_=true;showcaseStep_=0;showcaseRest_=0;StartPreview(0);}
    break;
  case Control::ResetView:closeup_=false;zoomGoal_=0;yaw_=yawGoal_=0;dress_=Dress::Equipped;ResetPreview();break;
  case Control::Name:break;
  case Control::Hero:focus_=Control::Enter;break;
  case Control::Enter: {
    const auto previous=snapshot_;Refresh();
    if(status_!=Status::Saved || snapshot_!=previous) {changed_=true;break;}
    return Action::Enter;
  }
  case Control::Create:
    ReadSlots();
    if(ProfileCount()>=maxProfiles){notice_="The character roster is full. Your draft is kept.";break;}
    rosterClass_=class_;draftFile_.clear();lookHistorySize_=0;callingView_=CallingView::Overview;
    look_={};draftName_.clear();notice_.clear();PreviewDraft();status_=Status::New;SetPage(Page::Class,Control::Next);QueueIntroduction();break;
  case Control::Next:
    ReadSlots();
    if(ProfileCount()>=maxProfiles){notice_="The character roster is full. Your draft is kept.";break;}
    draftFile_=AvailablePath();
    notice_.clear();PreviewDraft();SetPage(Page::Customize,Control::Name);break;
  case Control::Confirm:CreateProfile();break;
  case Control::Cancel:class_=rosterClass_;draftFile_.clear();SetPage(Page::Roster,Control::Create);Refresh();break;
  case Control::Back:
    if(page_==Page::Customize) {draftFile_.clear();ReadSlots();SetPage(Page::Class,Control::Next);notice_.clear();}
    else {class_=rosterClass_;draftFile_.clear();SetPage(Page::Roster,Control::Create);Refresh();}
    break;
  case Control::Refresh:Refresh();break;
  case Control::Quit:return Action::Quit;
  case Control::Left:yawGoal_=std::remainder(yawGoal_-.4f,6.2831853f);break;
  case Control::Right:yawGoal_=std::remainder(yawGoal_+.4f,6.2831853f);break;
  default:
    if(control>=Control::Skin0 && control<=Control::Dye4) {
      const int n=int(control)-int(Control::Skin0);
      auto next=look_;(n<5?next.skin:n<10?next.hair:next.dye)=n%5;ApplyLook(next);
    }
    if(control>=Control::SkinLeft && control<=Control::DyeRight) {
      const int offset=int(control)-int(Control::SkinLeft);
      auto next=look_;int& choice=offset<2?next.skin:offset<4?next.hair:next.dye;
      choice=(choice+(offset%2?1:-1)+rpg::Appearance::count)%rpg::Appearance::count;ApplyLook(next);
    }
    break;
  }
  return Action::None;
}
void CharacterSelection::ResetPreview() {
  contactRemaining_=-1;previewContacts_=0;previewContact_={};
  previewFinished_=false;
  introduction_=-1;automaticPreview_=false;
  showcase_=false;showcaseRest_=0;audition_.Reset();
  actionTime_=-1;previewAbility_=-1;released_=false;previewController_.Reset();previewWeapon_={};if(effects_.value)effects_->Reset();
}
void CharacterSelection::QueueIntroduction() {
  // A single entrance, not an unattended loop. Only the four current focus
  // classes have a reviewed introduction; ordinary roster entry stays quiet.
  if(class_==rpg::ClassId::Warrior || class_==rpg::ClassId::Scout ||
     class_==rpg::ClassId::Mage || class_==rpg::ClassId::Priest) introduction_=.55f;
}
void CharacterSelection::StartPreview(int ability) {
  contactRemaining_=-1;previewContacts_=0;previewContact_={};
  previewFinished_=false;
  stageAge_=1;
  introduction_=-1;automaticPreview_=false;
  closeup_=false;zoomGoal_=0;previewAbility_=ability;actionTime_=0;released_=false;
  effects_->Reset();previewWeapon_.last=-1;
  audition_.Reset();
  audition_.preparing=ability==3?hero_.RecoveryCastDuration()>0:ability!=2 && hero_.ClassInfo().range>3;
  if(audition_.preparing)audition_.Add(sound::PreparationCue(class_,ability));
  if(ability==2)audition_.Add(sound::Cue::Dash);
}
float CharacterSelection::PreviewRate() const {
  return ArtRate(previewAbility_);
}
float CharacterSelection::ArtRate(int ability) const {
  return ability==2 || ability==3?1.f:ability>=8?rpg::AdvancedRate(class_,ability):
    rpg::ClassAttackRate(class_,ability!=0 && ability!=7 && ability!=12)*
      (ability==7?.85f:ability==6?.7f:ability==5?.8f:1.f);
}
CharacterSelection::ArtDetails CharacterSelection::InspectArt(int ability) const {
  if(!rpg::ValidSpell(ability))return {};
  const auto spell=hero_.Spell(ability);
  if(ability==2 || ability==3)return {spell.contact,std::max(0.f,spell.recovery-spell.contact),false};
  const bool heavy=ability!=0 && ability!=7 && ability!=12,ranged=hero_.ClassInfo().range>3;
  const float rate=ArtRate(ability),windup=(ranged?combat::StrokeStart(heavy):combat::Contact(heavy))/rate;
  return {windup,std::max(0.f,combat::Duration(heavy)/rate-windup),ranged,
    ranged?(combat::Contact(heavy)-combat::StrokeStart(heavy))/rate:0.f};
}
float CharacterSelection::PreviewDuration() const {
  return previewAbility_==2?.50f:previewAbility_==3?hero_.RecoveryCastDuration()+.42f:
    animation::ActionDuration(PreviewHeavy()?animation::ActionClip::Heavy:animation::ActionClip::Basic);
}
float CharacterSelection::PreviewRelease() const {
  return previewAbility_==2?.12f:previewAbility_==3?hero_.RecoveryCastDuration():
    hero_.ClassInfo().range>3?combat::StrokeStart(PreviewHeavy()):combat::Contact(PreviewHeavy());
}
CharacterSelection::ArtTiming CharacterSelection::PreviewTiming() const {
  const float duration=PreviewDuration(),release=PreviewRelease();
  const float follow=std::min(duration,release+(previewAbility_==2?.22f:.09f));
  ArtTiming view;view.release=release/duration;view.follow=follow/duration;
  view.progress=previewFinished_?1.f:actionTime_<0?0.f:std::clamp(actionTime_/duration,0.f,1.f);
  view.phase=previewFinished_?"Demonstration complete":actionTime_<0?"Ready to demonstrate":
    actionTime_<release?"Preparation":actionTime_<follow?"Release":"Recovery";
  return view;
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
CharacterSelection::StageView CharacterSelection::PreviewStage() const {
  // Hold only presentation data for the brief outgoing dissolve. Profiles,
  // inventory and live simulation are never copied into a second hero.
  if(stageAge_<.10f) {
    auto view=previousStage_;view.opacity*=1-combat::Smooth(stageAge_/.10f);return view;
  }
  return {class_,PreviewOutfit(),PreviewBody(),yaw_,zoom_,combat::Smooth((stageAge_-.10f)/.22f),true};
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
  if(!input.Focused()) {CancelGesture();ResetPreview();stageAge_=1;hover_=Control::None;return Action::None;}
  if(!std::isfinite(seconds))seconds=0;
  time_=std::fmod(time_+std::clamp(seconds,0.f,.05f),600.f);
  const float uiSeconds=std::clamp(seconds,0.f,.05f);
  stageAge_=std::min(1.f,stageAge_+uiSeconds);
  pageAge_=std::min(1.f,pageAge_+uiSeconds);
  for(int i=0;i<int(Control::None);++i) {
    const auto control=Control(i);const float goal=Enabled(control) && (hover_==control || focus_==control)?1.f:0.f;
    highlights_[i]+=(goal-highlights_[i])*(1-std::exp(-14.f*uiSeconds));
  }
  const float dt=std::clamp(seconds,0.f,.05f);
  yaw_=std::remainder(yaw_+std::remainder(yawGoal_-yaw_,6.2831853f)*-std::expm1(-12.f*dt),6.2831853f);
  zoom_+=(zoomGoal_-zoom_)*-std::expm1(-12.f*dt);
  if(effects_.value)effects_->Advance(dt);bool emitRelease=false,emitSwing=false;float releaseElapsed=0,swingElapsed=0;
  const auto contact=[&](float elapsed) {
    previewContact_.elapsed=std::clamp(elapsed,0.f,dt);
    effects_->Emit(previewContact_);++previewContacts_;contactRemaining_=-1;
    audition_.Add(sound::ContactCue(previewContact_.school,previewContact_.ability,previewContact_.heavy));
  };
  // A released stage projectile owns its destination and contact clock. Turning
  // the figure afterward cannot redirect it; cancellation clears the pending hit.
  if(contactRemaining_>=0) {
    contactRemaining_-=dt;
    if(contactRemaining_<=0)contact(-contactRemaining_);
  }
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
      if(previewAbility_!=3 && hero_.ClassInfo().range<=3){emitSwing=true;swingElapsed=std::max(0.f,(actionTime_-audibleRelease)/PreviewRate());}
    }
    const float release=PreviewRelease();
    if(!released_ && previewAbility_!=2 && actionTime_>=release) {released_=true;emitRelease=true;releaseElapsed=std::max(0.f,(actionTime_-release)/PreviewRate());}
    if(actionTime_>=PreviewDuration()) {actionTime_=-1;previewFinished_=true;}
  }
  const float recovery=hero_.RecoveryCastDuration();
  const float recoveryPose=previewAbility_!=3 || actionTime_<0?-1:actionTime_<recovery?
    std::min(.30f,actionTime_/std::max(.001f,recovery)*1.5f):.30f+actionTime_-recovery;
  previewController_.Update(dt,class_,previewAbility_==2 || previewAbility_==3?-1:actionTime_,PreviewHeavy(),
    recoveryPose,true,previewAbility_==5,previewAbility_==6,previewAbility_==7,-1,previewAbility_);
  const auto form=rpg::WeaponFor(class_).form;
  const bool staff=rpg::StaffWeapon(form),bow=form==rpg::WeaponForm::Handbow,prayer=form==rpg::WeaponForm::Scepter;
  const bool blade=form==rpg::WeaponForm::Blade || form==rpg::WeaponForm::Mace || form==rpg::WeaponForm::Cleaver;
  previewWeapon_.Update(dt,previewAbility_==2 || previewAbility_==3?-1:actionTime_,PreviewHeavy(),
    staff||prayer,PreviewOutfit().weapon && (staff||bow||blade||prayer),PreviewRate(),bow,
    previewAbility_,class_==rpg::ClassId::Warrior?0:class_==rpg::ClassId::Scout?1:class_==rpg::ClassId::Mage?2:class_==rpg::ClassId::Priest?3:-1);
  if(emitRelease || emitSwing) {
      const auto outfit=PreviewOutfit();
      const auto pose=BuildCharacterPose(0,0,outfit.shield,PreviewBody(),form);
      const auto sockets=castfx::FromPose(pose,{0,0,0},yaw_,form,outfit.shield,outfit.weapon);
      rpg::CombatVisualEvent e;e.school=class_;e.ability=previewAbility_;e.facing=yaw_;e.heavy=PreviewHeavy();
      e.technique=previewAbility_==5;e.signature=previewAbility_==6;e.opening=previewAbility_==7;
      e.kind=previewAbility_==3?rpg::CombatVisualKind::Mend:hero_.ClassInfo().range>3?rpg::CombatVisualKind::SpellCast:
        PreviewHeavy()?rpg::CombatVisualKind::HeavyStrike:rpg::CombatVisualKind::Strike;
      e.destination={std::sin(yaw_)*1.7f,std::cos(yaw_)*1.7f};e.destinationHeight=1.2f;e.flight=InspectArt(previewAbility_).flight;
      e.elapsed=emitSwing?swingElapsed:releaseElapsed;
      if(emitSwing || hero_.ClassInfo().range>3 || previewAbility_==3)effects_->Emit(e,&sockets.focus);
      if(emitRelease && previewAbility_!=3) {
        previewContact_=e;previewContact_.kind=PreviewHeavy()?rpg::CombatVisualKind::HeavyHit:rpg::CombatVisualKind::Hit;
        previewContact_.position=e.destination;
        previewContact_.height=rpg::WorldMap::Height(e.destination)+e.destinationHeight;
        // No ground damage area is invented on the character stage.
        previewContact_.primary=false;
        contactRemaining_=e.kind==rpg::CombatVisualKind::SpellCast?e.flight-releaseElapsed:0;
        if(contactRemaining_<=0)contact(e.kind==rpg::CombatVisualKind::SpellCast?-contactRemaining_:releaseElapsed);
      }
    }
  if(effects_.value && effects_->Tracking(0)) {
    const auto outfit=PreviewOutfit();
    const auto pose=BuildCharacterPose(0,0,outfit.shield,PreviewBody(),form);
    const auto world=[&](DirectX::XMFLOAT3 p) {return DirectX::XMFLOAT3{std::cos(yaw_)*p.x+std::sin(yaw_)*p.z,p.y,-std::sin(yaw_)*p.x+std::cos(yaw_)*p.z};};
    effects_->TrackWeapon(0,world(pose.WeaponPoint(0)),world(pose.WeaponPoint(.8f)));
  }
  const bool resized=width_!=input.Width() || height_!=input.Height();width_=input.Width();height_=input.Height();
  if(resized || input.Pressed(VK_F11)) {
    stageAge_=1;
    if(automaticPreview_)ResetPreview();else introduction_=-1;
    CancelGesture();hover_=Control::None;return Action::None;
  }
  const Layout layout(width_,height_);hover_=Control::None;
  const float localX=layout.LocalX(input.ClientX()),localY=layout.LocalY(input.ClientY());
  if(page_==Page::Roster && std::isfinite(wheel) && wheel!=0 && Rect{996,106,262,510}.Contains(localX,localY)) {
    CancelGesture();return Activate(wheel>0?Control::PreviousProfiles:Control::NextProfiles);
  }
  if(std::abs(localX-hintPointerX_)>.5f || std::abs(localY-hintPointerY_)>.5f)keyboardHints_=false;
  hintPointerX_=localX;hintPointerY_=localY;
  const bool overTalentCard=InspectedTalent()>=0 && TalentCardBounds().Contains(localX,localY);
  const bool stage=StageBounds().Contains(localX,localY) && !overTalentCard && Enabled(Control::Left);
  // A deliberate input wins over a pending entrance in the very same frame.
  // Do not zoom out or emit a late charge after the player begins inspection.
  bool deliberate=std::isfinite(wheel) && wheel!=0;
  for(int key=1;key<256 && !deliberate;++key)deliberate=input.Pressed(key);
  if(deliberate) {
    if(stage && (input.Pressed(VK_LBUTTON) || wheel!=0))stageAge_=1;
    introduction_=-1;
    if(automaticPreview_ && (input.Pressed(VK_LBUTTON) || wheel!=0 || input.Pressed(VK_LEFT) || input.Pressed(VK_RIGHT)))ResetPreview();
  } else if(introduction_>=0 && page_==Page::Class) {
    introduction_-=dt;
    if(introduction_<=0) {
      StartPreview(class_==rpg::ClassId::Warrior || class_==rpg::ClassId::Scout?1:6);
      automaticPreview_=true;
    }
  }
  for(int i=0;i<int(Control::None);++i) {
    const auto control=Control(i);
    if(Enabled(control) && Bounds(control).Contains(layout.LocalX(input.ClientX()),layout.LocalY(input.ClientY()))) hover_=control;
  }
  const auto nextHint=keyboardHints_?focus_:hover_;
  if(nextHint!=hint_) {hint_=nextHint;hintAge_=0;}
  else hintAge_=std::min(1.f,hintAge_+dt);
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
    nameRepeatKey_=0;
    keyboardHints_=true;hintAge_=0;
    const int direction=input.Held(VK_SHIFT)?-1:1;
    do {focus_=Control((int(focus_)+direction+int(Control::None))%int(Control::None));} while(!Enabled(focus_));
  }
  if(page_==Page::Roster && (input.Pressed(VK_DOWN) || input.Pressed(VK_UP)) && ProfileCount()>0) {
    const int index=SelectedProfile();SelectProfile(std::clamp(index+(input.Pressed(VK_DOWN)?1:-1),0,ProfileCount()-1));return Action::None;
  }
  if(input.Pressed(VK_ESCAPE)) {
    if(pinnedTalent_>=0){pinnedTalent_=-1;CancelGesture();focus_=Control(int(Control::Branch0)+branch_);return Action::None;}
    CancelGesture();if(page_!=Page::Roster) return Activate(Control::Back);
    focus_=Control::Quit;return Action::None;
  }
  if(input.Pressed(VK_LBUTTON)) {
    keyboardHints_=false;hintAge_=0;
    pressed_=hover_;if(hover_!=Control::None) focus_=hover_;
    rotating_=stage && hover_==Control::None;dragX_=localX;
    nameDragging_=page_==Page::Customize && hover_==Control::Name;
    if(nameDragging_)PickName(localX,input.Held(VK_SHIFT));
  }
  if(nameDragging_ && input.Held(VK_LBUTTON))PickName(localX,true);
  if(rotating_ && input.Held(VK_LBUTTON)) {
    yaw_=std::remainder(yaw_+std::clamp(localX-dragX_,-150.f,150.f)*.012f,6.2831853f);dragX_=localX;
    yawGoal_=yaw_;
  }
  const bool nameEditing=focus_==Control::Name && page_==Page::Customize;
  const auto heldRotation=input.Held(VK_LBUTTON) && (pressed_==Control::Left || pressed_==Control::Right)?pressed_:
    !nameEditing && input.Held(VK_LEFT) && !input.Held(VK_RIGHT)?Control::Left:
    !nameEditing && input.Held(VK_RIGHT) && !input.Held(VK_LEFT)?Control::Right:Control::None;
  if(heldRotation!=Control::None && Enabled(heldRotation)) {
    if(rotateControl_!=heldRotation){rotateControl_=heldRotation;rotateHold_=0;rotateRepeated_=false;}
    const float before=rotateHold_;rotateHold_+=dt;
    const float travel=std::max(0.f,rotateHold_-.28f)-std::max(0.f,before-.28f);
    if(travel>0){rotateRepeated_=true;yawGoal_=std::remainder(yawGoal_+(heldRotation==Control::Left?-1:1)*travel*1.6f,6.2831853f);}
  }
  if(input.Released(VK_LBUTTON)) {
    const auto clicked=pressed_;const bool heldTurn=rotateRepeated_ && rotateControl_==clicked;
    const bool dragged=rotating_ || nameDragging_;const float heldGoal=yawGoal_;CancelGesture();
    if(heldTurn)yawGoal_=heldGoal;
    if(!dragged && !heldTurn && clicked==hover_) return Activate(clicked);
  }
  if(heldRotation==Control::None){rotateControl_=Control::None;rotateHold_=0;rotateRepeated_=false;}
  if(focus_==Control::Name && page_==Page::Customize) {
    const auto previousName=draftName_;
    const bool control=input.Held(VK_CONTROL),alt=input.Held(VK_MENU),shift=input.Held(VK_SHIFT);
    if(control && !alt && input.Pressed('A'))nameEditor_.All(draftName_);
    if(control && !alt && input.Pressed('V'))nameEditor_.Insert(draftName_,input.PasteText());
    if(!control && !alt) {
      const auto editKey=[&](int key){
        if(key==VK_LEFT || key==VK_RIGHT)nameEditor_.Move(draftName_,key==VK_LEFT?-1:1,shift);
        else nameEditor_.Erase(draftName_,key==VK_BACK);
      };
      bool began=false;
      for(int key:{VK_LEFT,VK_RIGHT,VK_BACK,VK_DELETE})if(input.Pressed(key)) {
        editKey(key);nameRepeatKey_=key;nameRepeatDelay_=.40f;began=true;
      }
      if(nameRepeatKey_ && !input.Held(nameRepeatKey_))nameRepeatKey_=0;
      if(nameRepeatKey_ && !began) {
        nameRepeatDelay_-=dt;
        while(nameRepeatDelay_<=0){editKey(nameRepeatKey_);nameRepeatDelay_+=.05f;}
      }
      if(input.Pressed(VK_HOME))nameEditor_.Place(draftName_,0,shift);
      if(input.Pressed(VK_END))nameEditor_.Place(draftName_,rpg::NameLength(draftName_),shift);
      if(input.NativeText()) {
        nameEditor_.Insert(draftName_,input.Characters());
      } else {
        // Deterministic synthetic input used by tools/tests without a message loop.
        for(int key='A';key<='Z';++key)if(input.Pressed(key))nameEditor_.Insert(draftName_,std::string(1,char((shift!=input.CapsLock())?key:key+32)));
        if(input.Pressed(VK_SPACE))nameEditor_.Insert(draftName_," ");
        for(auto key:{VK_OEM_MINUS,VK_OEM_7})if(input.Pressed(key))nameEditor_.Insert(draftName_,key==VK_OEM_MINUS?"-":"'");
      }
    } else nameRepeatKey_=0;
    if(previousName!=draftName_) PreviewDraft();
    if(input.Pressed(VK_RETURN)) {
      while(!draftName_.empty() && draftName_.back()==' ') draftName_.pop_back();
      nameEditor_.Place(draftName_,rpg::NameLength(draftName_));
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
