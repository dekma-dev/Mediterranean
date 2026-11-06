#pragma once
#include "game/Simulation.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <string>

namespace ui {
struct CombatTextView {
  std::string text;
  std::array<float,3> color{1,.86f,.46f};
  float x=0,y=0,size=1.65f,opacity=0;
};
// Screen-space motion in a 720p reference view. Camera distance changes the
// anchor, not the text size/speed. No render-time state or vector-index offsets.
inline CombatTextView ReadCombatText(const rpg::CombatFeedback& feedback) {
  CombatTextView view;
  const float t=feedback.age;
  if(!std::isfinite(t) || t<0 || t>=1.5f || !std::isfinite(feedback.position.x) || !std::isfinite(feedback.position.z))return view;
  const auto smooth=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
  view.opacity=smooth(t/.035f)*(1-smooth((t-1.0f)/.5f));
  // Stable alternating fan lanes prevent labels jumping sideways as old hits
  // leave the bounded feedback buffer. The onset remains attached to contact.
  constexpr int lanes[]={0,-1,1,-2,2};const int lane=lanes[feedback.sequence%5];
  view.x=lane*24.f*smooth(t/.34f);
  view.y=-14.f-72.f*(1-std::exp(-t*1.3f));
  view.size=1.65f+.34f*std::sin(std::clamp(t/.22f,0.f,1.f)*3.14159265f);
  const auto amount=std::to_string(feedback.amount);
  switch(feedback.kind) {
    case rpg::FeedbackKind::Damage:view.text=amount;break;
    case rpg::FeedbackKind::Hurt:view.text="-"+amount;view.color={1,.32f,.26f};view.x-=66;view.y+=44;break;
    case rpg::FeedbackKind::Heal:view.text="+"+amount;view.color={.40f,1,.62f};view.x+=66;view.y+=18;break;
    case rpg::FeedbackKind::Absorb:view.text="ABSORB "+amount;view.color={.70f,.84f,1};view.x-=54;view.y+=70;view.size*=.8f;break;
    case rpg::FeedbackKind::Experience:view.text="XP "+amount;view.color={.68f,.74f,1};view.y-=46;view.size*=.8f;break;
    case rpg::FeedbackKind::LevelUp:view.text="LEVEL "+amount+" / N: BUILD";view.y-=74;break;
    case rpg::FeedbackKind::Miss:view.text="MISSED";view.color={.8f,.8f,.8f};view.size*=.85f;break;
    case rpg::FeedbackKind::Interrupt:view.text="INTERRUPTED";view.color={.4f,.85f,1};view.y-=26;view.size*=.85f;break;
  }
  return view;
}
}
