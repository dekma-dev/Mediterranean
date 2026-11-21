#pragma once
#include "game/Simulation.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>

namespace ui {
struct TrackedQuest {
  rpg::QuestId id=rpg::QuestId::None;
  rpg::QuestStage stage=rpg::QuestStage::Offer;
  std::string title,detail,summary;
  int route=-1;
  bool selected=false;
};
struct QuestTracking {
  std::array<TrackedQuest,7> entries{};
  unsigned count=0;
  const TrackedQuest* Find(rpg::QuestId id) const {
    for(unsigned i=0;i<count;++i)if(entries[i].id==id)return &entries[i];
    return nullptr;
  }
};
// Read campaign and local work independently of the currently selected route.
inline QuestTracking ReadQuestTracking(const rpg::Simulation& s) {
  using namespace rpg;
  QuestTracking result;
  const auto main=s.CurrentQuest();
  if(main!=QuestId::None && (s.FrontierCompleted()<frontierQuestCount || s.ReplayActive())) {
    auto& q=result.entries[result.count++];q.id=main;
    q.title=DescribeQuest(main).title;q.selected=s.TrackedHunt()<0 || s.ReplayActive();
    const auto objective=s.CampaignObjective();const auto newline=objective.find('\n');
    q.detail=newline==std::string::npos?objective:objective.substr(newline+1);
    const bool ready=main==QuestId::Patrol?s.QuestState()==Quest::Return:
      main==QuestId::Anchor?s.AnchorState()==AnchorQuest::Return:s.FrontierActive() && s.FrontierReady();
    const bool active=main==QuestId::Patrol?s.QuestState()==Quest::Active:
      main==QuestId::Anchor?s.AnchorState()==AnchorQuest::Active:s.FrontierActive();
    q.stage=ready?QuestStage::TurnIn:active?QuestStage::Progress:QuestStage::Offer;
    q.summary=ready?"Ready to turn in":active?"Campaign in progress":"Speak to the quest giver";
  }
  if(s.ReplayActive())return result;
  for(int i=0;i<6;++i)if(s.HuntState(i)==1) {
    auto& q=result.entries[result.count++];q.id=QuestId(int(QuestId::Hunt1)+i);q.route=i;
    q.title=DescribeQuest(q.id).title;q.detail=s.HuntSummary(i);
    q.selected=s.TrackedHunt()==i && !s.ReplayActive();
    q.stage=s.HuntReady(i)?QuestStage::TurnIn:QuestStage::Progress;
    q.summary=q.stage==QuestStage::TurnIn?"Ready to turn in":
      "Progress: "+std::to_string(std::min(s.HuntProgress(i),HuntRequired(i)))+" / "+std::to_string(HuntRequired(i))+
      ((i==3 || i==4) && !(s.HuntMarks(i)&32)?" / Leader required":"");
  }
  return result;
}
inline bool QuestDelivered(const rpg::Simulation& s,rpg::QuestId id) {
  using namespace rpg;
  if(id==QuestId::Patrol)return s.QuestState()==Quest::Complete;
  if(id==QuestId::Anchor)return s.AnchorState()==AnchorQuest::Complete;
  if(id>=QuestId::Watch1 && id<=QuestId::Watch16)return s.FrontierCompleted()>int(id)-int(QuestId::Watch1);
  return HuntingQuest(id) && s.HuntState(HuntIndex(id))==2;
}
enum class QuestNoticeKind { Accepted,Progress,Ready,Delivered };
struct QuestNotice {
  rpg::QuestId id=rpg::QuestId::None;
  QuestNoticeKind kind=QuestNoticeKind::Progress;
  std::string title,detail;
  float remaining=0;
  float Opacity() const {return std::min(std::clamp((5-remaining)/.16f,0.f,1.f),std::clamp(remaining/.6f,0.f,1.f));}
  const char* Label() const {
    return kind==QuestNoticeKind::Accepted?"QUEST ACCEPTED":kind==QuestNoticeKind::Ready?"READY TO TURN IN":
      kind==QuestNoticeKind::Delivered?"QUEST DELIVERED":"QUEST PROGRESS";
  }
};
class QuestActivity {
public:
  void Reset(){previous_={};notices_={};observed_=false;}
  const std::array<QuestNotice,3>& Notices() const{return notices_;}
  void Update(float dt,const rpg::Simulation& s) {
    if(!std::isfinite(dt) || dt<=0 || dt>.25f)return;
    for(auto& n:notices_)n.remaining=std::max(0.f,n.remaining-dt);
    auto next=ReadQuestTracking(s);
    if(s.ReplayActive()){previous_=std::move(next);observed_=false;notices_={};return;}
    if(observed_) {
      for(unsigned i=0;i<next.count;++i) {
        const auto& q=next.entries[i];const auto* old=previous_.Find(q.id);
        if(q.stage==rpg::QuestStage::Offer)continue;
        if(!old || old->stage==rpg::QuestStage::Offer)Push(q,QuestNoticeKind::Accepted);
        else if(q.stage==rpg::QuestStage::TurnIn && old->stage!=q.stage)Push(q,QuestNoticeKind::Ready);
        else if(q.stage==rpg::QuestStage::Progress && q.detail!=old->detail)Push(q,QuestNoticeKind::Progress);
      }
      for(unsigned i=0;i<previous_.count;++i) {
        const auto& q=previous_.entries[i];
        if(!next.Find(q.id) && QuestDelivered(s,q.id))Push(q,QuestNoticeKind::Delivered);
      }
    }
    previous_=std::move(next);observed_=true;
  }
private:
  void Push(const TrackedQuest& q,QuestNoticeKind kind) {
    // Coalesce repeated increments of one quest, keeping the newest truth first.
    auto found=std::find_if(notices_.begin(),notices_.end(),[&](const auto& n){return n.id==q.id;});
    if(found!=notices_.end())std::move(found+1,notices_.end(),found);
    std::move_backward(notices_.begin(),notices_.end()-1,notices_.end());
    notices_[0]={q.id,kind,q.title,kind==QuestNoticeKind::Delivered?"Reward received":
      kind==QuestNoticeKind::Progress && rpg::HuntingQuest(q.id)?q.summary:q.detail,5};
  }
  QuestTracking previous_;
  std::array<QuestNotice,3> notices_{};
  bool observed_=false;
};
}
