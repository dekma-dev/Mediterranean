#pragma once
#include "game/WorldMap.h"
#include <vector>

namespace rpg::population {
inline constexpr int districtCount=6,beastsPerDistrict=6,beastCount=districtCount*beastsPerDistrict;
inline constexpr EntityId beastBase=400;
struct District { Vec2 yard,habitat; int rank; bool wolves; const char* name; };
inline constexpr District districts[]{
  {{-32,-35},{-46,-40},1,false,"Greyfield Farm"},
  {{34,-36},{45,-23},2,true,"Timbermen's Rest"},
  {{-73,20},{-88,40},4,true,"Old Pine Lodge"},
  {{78,35},{94,55},6,true,"Barrow Watch"},
  {{-75,70},{-86,94},7,false,"Mirestead"},
  {{78,81},{94,107},9,false,"Slate Quarry"}
};
enum class Prop { Barrel,Crate,Chest,Cart,Well,Bench,Lantern,Brazier,Anvil,Logs,Sacks,Sign,Fence,Stump,Grave };
struct Placement { Prop kind; Vec2 position; float yaw=0,scale=1; };
inline Vec2 Cache(int district,int index) {
  const auto p=districts[district].habitat;
  return {p.x+float(index-1)*5,p.z+(index==1?9.f:-7.f)};
}
inline const std::vector<Placement>& Props() {
  static const auto result=[] {
    std::vector<Placement> p;
    for(int d=0;d<districtCount;++d) {
      const auto c=districts[d].yard;
      const auto add=[&](Prop kind,float x,float z,float yaw=0,float scale=1) {p.push_back({kind,{c.x+x,c.z+z},yaw,scale});};
      add(Prop::Well,6,1,0,.8f);add(Prop::Cart,-6,3,.3f,.85f);
      add(Prop::Bench,2,5,3.14159f);add(Prop::Bench,-2,5,3.14159f);
      add(Prop::Brazier,4,5,0,.75f);add(Prop::Anvil,-5,-3,0,.8f);
      add(Prop::Logs,-6,-1,.1f,.8f);add(Prop::Logs,-6,-3,0,.8f);
      add(Prop::Barrel,4,-2);add(Prop::Barrel,5,-2);add(Prop::Barrel,4,-3);
      add(Prop::Crate,-4,0);add(Prop::Crate,-4,-1.2f);add(Prop::Chest,4,0,1.5708f,.8f);
      add(Prop::Sacks,5,-4,0,.85f);add(Prop::Sacks,4,-5,.3f,.7f);
      add(Prop::Sign,1,8,0,.8f);add(Prop::Lantern,-3,4,0,.8f);
      for(int i=0;i<4;++i) {add(Prop::Fence,-7+i*2.f,-7);add(Prop::Fence,7,-5+i*2.f,1.5708f);}
      add(Prop::Stump,-8,6,0,.85f);add(Prop::Grave,8,6,.3f,.8f);
      for(int i=0;i<3;++i) p.push_back({d==2||d==5?Prop::Chest:Prop::Grave,Cache(d,i),i*.4f,.75f});
    }
    return p;
  }();return result;
}
// Conservative footprints, shared by collision and native scenery. Small
// lanterns/wayposts remain presentation detail; substantial props are solid.
inline Vec2 HalfSize(Prop kind) {
  switch(kind) {
  case Prop::Cart:return {1.05f,1.35f};case Prop::Well:return {1,1};
  case Prop::Bench:return {.95f,.35f};case Prop::Logs:return {.85f,.55f};
  case Prop::Fence:return {1,.12f};case Prop::Lantern:case Prop::Sign:return {};
  case Prop::Sacks:return {.6f,.5f};default:return {.4f,.4f};
  }
}
inline float PropHeight(Prop p) {return p==Prop::Well?2.6f:p==Prop::Cart?1.2f:p==Prop::Fence?1.1f:p==Prop::Grave?1.3f:.8f;}
}
