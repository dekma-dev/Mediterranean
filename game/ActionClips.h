#pragma once
#include "game/AttackMotion.h"
#include "game/ClassEquipment.h"
#include <array>

namespace animation {
// Original additive action clips, in metres/radians. The final arm/leg solves
// run after these channels; a clip never moves the authoritative character.
struct ActionVector { float x=0,y=0,z=0; };
inline ActionVector operator+(ActionVector a,ActionVector b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline ActionVector operator-(ActionVector a,ActionVector b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline ActionVector operator*(ActionVector a,float t) { return {a.x*t,a.y*t,a.z*t}; }
struct ActionChannels {
  ActionVector left,right;
  float pitch=0,yaw=0,roll=0,hip=0,open=0,aim=0;
};
inline ActionChannels operator+(ActionChannels a,ActionChannels b) {
  return {a.left+b.left,a.right+b.right,a.pitch+b.pitch,a.yaw+b.yaw,a.roll+b.roll,a.hip+b.hip,a.open+b.open,a.aim+b.aim};
}
inline ActionChannels operator*(ActionChannels a,float t) {
  return {a.left*t,a.right*t,a.pitch*t,a.yaw*t,a.roll*t,a.hip*t,a.open*t,a.aim*t};
}
inline ActionChannels operator-(ActionChannels a,ActionChannels b) { return a+b*-1.f; }
enum class ActionClip { Basic,Heavy,Restore,Technique,Signature,Opening };
struct ClassActionClip {
  const char* name;
  ActionChannels prepare,release,follow;
};
// Authored class gestures share the arm solve and the equipped right-hand
// socket. Physical launchers add a continuous aim channel at accepted release.
inline const std::array<ClassActionClip,int(rpg::ClassId::Count)> classActionClips={{
  {"Guarded cut",{{0,.02f,0},{0,0,0},-.03f,-.06f,.015f,-.015f},
    {{0,.015f,.01f},{0,0,0},.04f,.05f,-.02f,-.025f},
    {{0,.01f,0},{0,0,0},.015f,.025f,-.01f,-.01f}},
  {"Templar weight",{{0,.03f,-.025f},{.025f,.04f,-.02f},-.025f,-.035f,0,-.012f},
    {{0,.025f,.025f},{-.015f,-.02f,.025f},.025f,.025f,0,-.018f},
    {{0,.01f,.015f},{0,-.01f,.01f},.012f,.01f,0,-.008f}},
  {"Trail sight",{{.09f,.25f,.18f},{-.035f,.12f,-.04f},-.035f,-.12f,-.025f,-.015f,.4f},
    {{.10f,.32f,.34f},{-.015f,.10f,.03f},.045f,-.08f,-.015f,-.025f,.9f},
    {{.06f,.25f,.23f},{0,.08f,-.035f},-.04f,-.10f,-.01f,-.012f,.6f}},
  {"Radiant invocation",{{.15f,.37f,.13f},{-.035f,.10f,.015f},-.07f,.02f,0,-.005f,.8f},
    {{.10f,.47f,.28f},{.025f,.15f,.04f},-.025f,.05f,-.025f,-.01f,1},
    {{.07f,.37f,.24f},{.015f,.11f,.025f},.015f,.025f,-.01f,-.005f,.8f}},
  {"Gathering storm",{{-.08f,.12f,.12f},{.02f,.045f,.01f},.06f,-.14f,.04f,-.03f,.6f},
    {{.12f,.37f,.33f},{.015f,.12f,.02f},-.04f,.12f,-.035f,-.018f,1},
    {{.19f,.27f,.22f},{.01f,.09f,.025f},.025f,.08f,-.02f,-.008f,.7f}},
  {"Ember projection",{{.15f,.25f,.06f},{.035f,.09f,-.055f},-.07f,-.14f,.025f,-.02f,.55f},
    {{.17f,.34f,.36f},{.03f,.13f,.025f},.075f,.13f,-.035f,-.025f,1},
    {{.08f,.29f,.28f},{.015f,.09f,-.01f},.025f,.09f,-.025f,-.012f,.8f}},
  {"Soul gathering",{{.02f,.40f,.16f},{.02f,.08f,-.01f},-.045f,-.08f,.03f,-.02f,.7f},
    {{.10f,.31f,.34f},{.025f,.10f,.025f},.07f,.08f,-.02f,-.035f,.9f},
    {{.20f,.20f,.10f},{.015f,.07f,0},.04f,.11f,-.01f,-.02f,.4f}},
  {"Focused will",{{.18f,.53f,.02f},{0,.07f,-.015f},-.03f,-.055f,-.02f,-.008f,.5f},
    {{.12f,.38f,.33f},{.025f,.09f,.01f},.035f,.06f,-.015f,-.015f,1},
    {{.13f,.34f,.26f},{.01f,.06f,0},.012f,.03f,0,-.006f,.8f}},
  {"Conducted discord",{{-.08f,.30f,.12f},{.025f,.09f,-.02f},-.045f,-.12f,.04f,-.012f,.8f},
    {{.15f,.24f,.33f},{0,.08f,.035f},.045f,.16f,-.04f,-.02f,1},
    {{.20f,.32f,.18f},{.015f,.06f,0},.015f,.08f,-.02f,-.008f,.7f}},
  {"Braced discharge",{{.12f,.23f,.21f},{-.03f,.13f,-.025f},.025f,-.08f,-.025f,-.03f,.3f},
    {{.13f,.27f,.30f},{-.02f,.12f,.035f},.06f,-.03f,-.02f,-.04f,.8f},
    {{.10f,.22f,.14f},{0,.08f,-.035f},-.05f,-.07f,-.015f,-.02f,.5f}},
  {"Ravager coil",{{-.025f,.035f,-.03f},{.035f,.025f,-.025f},.035f,-.055f,.025f,-.025f},
    {{.035f,.015f,.025f},{-.025f,-.025f,.035f},.05f,.05f,-.025f,-.03f},
    {{.015f,0,.01f},{-.015f,-.015f,.015f},.025f,.035f,-.01f,-.015f}}
}};
inline bool RangedClass(rpg::ClassId id) { return rpg::DescribeClass(id).range>3; }
inline float ActionDuration(ActionClip clip) { return clip==ActionClip::Restore?.72f:combat::Duration(clip==ActionClip::Heavy || clip==ActionClip::Technique || clip==ActionClip::Signature); }
inline ActionChannels SampleAction(rpg::ClassId id,ActionClip clip,float seconds) {
  if(!rpg::ValidClass(id) || !std::isfinite(seconds) || seconds<0 || seconds>=ActionDuration(clip)) return {};
  const auto& definition=classActionClips[int(id)];
  const bool restore=clip==ActionClip::Restore,heavy=clip==ActionClip::Heavy || clip==ActionClip::Technique || clip==ActionClip::Signature;
  if(restore && id==rpg::ClassId::Warrior) return {}; // Existing Mend remains unchanged.
  const float release=restore?.18f:RangedClass(id)?combat::StrokeStart(heavy):combat::Contact(heavy);
  const float prepare=release*(restore || heavy?.65f:.40f),follow=restore?.34f:release+.09f;
  const std::array<float,5> times{0,prepare,release,follow,ActionDuration(clip)};
  auto ready=definition.prepare,cast=definition.release,settle=definition.follow;
  if(restore) {
    // Gathering the class's energy inward ends near the chest, instead of
    // aiming an offensive release at the selected opponent.
    ready=ready*.45f; cast=cast*.45f; settle=settle*.45f;
    ready.left=ready.left+ActionVector{.13f,.17f,.09f};
    cast.left={.27f,.32f,.10f}; settle.left={.20f,.24f,.08f};
    ready.open=.55f; cast.open=1; settle.open=.7f;
    // Recovery has its own silhouette: protective bracing, evasive gathering,
    // prayer and mechanical assembly share timing but not a universal heal pose.
    if(id==rpg::ClassId::Warrior || id==rpg::ClassId::Paladin) {
      ready.left={.10f,.19f,.15f};cast.left={.15f,.28f,.19f};settle.left={.12f,.21f,.12f};
      ready.pitch=-.035f;cast.pitch=.02f;cast.hip=-.025f;ready.open=cast.open=settle.open=.15f;
    } else if(id==rpg::ClassId::Scout) {
      ready.left={.10f,.18f,.06f};cast.left={.20f,.27f,.15f};settle.left={.12f,.18f,.08f};
      ready.yaw=-.09f;cast.yaw=.08f;cast.hip=-.035f;cast.open=.35f;
    } else if(id==rpg::ClassId::Engineer) {
      ready.left={.12f,.20f,.12f};cast.left={.24f,.28f,.18f};settle.left={.18f,.22f,.12f};
      ready.pitch=.04f;cast.pitch=.055f;cast.open=.25f;
    } else if(id==rpg::ClassId::Psionic) {
      ready.left={.12f,.40f,.02f};cast.left={.14f,.43f,.08f};settle.left={.16f,.30f,.09f};
      ready.pitch=cast.pitch=-.035f;
    } else if(id==rpg::ClassId::Necromancer || id==rpg::ClassId::Demonologist) {
      ready.left={.08f,.22f,.03f};cast.left={.21f,.31f,.12f};settle.left={.15f,.23f,.09f};
      ready.roll=.035f;cast.pitch=.045f;cast.hip=-.025f;
    }
  } else if(RangedClass(id) && !heavy) {
    // Quick attacks use a compact preparation: squeezing the full heavy
    // wind-up into 90 ms otherwise produces a visibly whipping shield/arm.
    ready=ready*.32f; cast=cast*.60f; settle=settle*.55f;
    ready.open*=.75f; cast.open*=.75f; settle.open*=.75f;
  }
  if(!restore && rpg::PhysicalLauncher(rpg::WeaponFor(id).form)) {
    ready.aim=.85f;cast.aim=1;settle.aim=.85f;
    ready.open=cast.open=settle.open=.08f;
    if(id==rpg::ClassId::Scout) {
      ready.left={.28f,.15f,.20f};cast.left={.45f,.23f,.29f};settle.left={.36f,.19f,.22f};
      if(!heavy) {ready.left=ready.left*.30f;cast.left=cast.left*.65f;settle.left=settle.left*.65f;}
    } else {
      ready.left={-.02f,.035f,0};cast.left={-.035f,.06f,.025f};settle.left={-.025f,.035f,0};
    }
  }
  const std::array<ActionChannels,5> keys{{{},ready,cast,settle,{}}};
  unsigned segment=0; while(segment<3 && seconds>times[segment+1]) ++segment;
  const float interval=times[segment+1]-times[segment],t=(seconds-times[segment])/interval;
  const auto& a=keys[segment]; const auto& b=keys[segment+1];
  // Tangents use actual key intervals. Zero endpoint derivatives give a clean
  // handoff to locomotion; interior derivatives remain continuous at release.
  const auto ma=segment?(b-keys[segment-1])*(interval/(times[segment+1]-times[segment-1])):ActionChannels{};
  const auto mb=segment<3?(keys[segment+2]-a)*(interval/(times[segment+2]-times[segment])):ActionChannels{};
  const float t2=t*t,t3=t2*t;
  auto result=a*(2*t3-3*t2+1)+ma*(t3-2*t2+t)+b*(-2*t3+3*t2)+mb*(t3-t2);
  if(clip==ActionClip::Technique) {
    // Smooth zero-ended accent differentiates the tactical gesture while
    // retaining the contact clock and the final fixed-length arm solve.
    const float accent=std::sin(seconds/ActionDuration(clip)*3.14159265f);
    const float weight=accent*accent;
    result.hip-=.018f*weight;
    result.pitch+=.025f*weight;
    if(RangedClass(id)) result.left=result.left+ActionVector{.025f,.045f,.025f}*weight;
    else {result.yaw-=.07f*weight;result.left=result.left+ActionVector{.02f,.04f,-.025f}*weight;}
  }
  if(clip==ActionClip::Signature) {
    const float wave=std::sin(seconds/ActionDuration(clip)*3.14159265f),weight=wave*wave;
    result.hip-=.028f*weight;
    result.pitch-=.035f*weight;
    if(RangedClass(id)) {result.left=result.left+ActionVector{.045f,.085f,-.025f}*weight;result.open+=.2f*weight;}
    else {result.yaw-=.11f*weight;result.roll+=.025f*weight;}
  }
  if(clip==ActionClip::Opening) {
    const float wave=std::sin(seconds/ActionDuration(clip)*3.14159265f),weight=wave*wave;
    result.yaw+=.045f*weight;result.hip-=.012f*weight;
    if(RangedClass(id)) result.left=result.left+ActionVector{-.025f,.03f,.05f}*weight;
  }
  result.open=std::clamp(result.open,0.f,1.f);
  result.aim=std::clamp(result.aim,0.f,1.f);
  return result;
}
struct ActionController {
  ActionChannels pose,releasePose;
  float lastSeconds=-1,releaseAge=1;
  ActionClip lastClip=ActionClip::Basic;
  rpg::ClassId lastClass=rpg::ClassId::Warrior;
  void Reset() { *this={}; }
  void Update(float dt,rpg::ClassId id,float attackSeconds,bool heavy,float healSeconds,bool alive,bool technique=false,bool signature=false,bool opening=false) {
    if(!alive || !rpg::ValidClass(id)) { Reset(); return; }
    if(!std::isfinite(dt) || dt<=0) return;
    dt=std::min(dt,.05f);
    const auto clip=attackSeconds>=0?(opening?ActionClip::Opening:signature?ActionClip::Signature:technique?ActionClip::Technique:heavy?ActionClip::Heavy:ActionClip::Basic):ActionClip::Restore;
    const float seconds=attackSeconds>=0?attackSeconds:healSeconds;
    const bool active=std::isfinite(seconds) && seconds>=0 && seconds<ActionDuration(clip);
    const bool changed=lastSeconds>=0 && (!active || id!=lastClass || clip!=lastClip || seconds+.00001f<lastSeconds);
    if(changed && (lastSeconds+dt<ActionDuration(lastClip)-.0001f || (active && clip!=lastClip))) {
      releasePose=pose; releaseAge=0;
    }
    releaseAge=std::min(.16f,releaseAge+dt);
    pose=active?SampleAction(id,clip,seconds):ActionChannels{};
    // An interrupted gesture settles even when a menu has taken input. A new
    // action crossfades out the previous pose without postponing its hit clock.
    const float carry=1-combat::Smooth(releaseAge/.16f);
    if(carry>0) pose=pose*(1-carry)+releasePose*carry;
    lastSeconds=active?seconds:-1; lastClip=clip; lastClass=id;
  }
};
}
