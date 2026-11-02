#pragma once
#include "game/WorldMap.h"
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace presentation {
struct CreatureContact {
 rpg::EntityId actor=0;
 DirectX::XMFLOAT3 position{};
 float facing=0,strength=0;
 bool hoof=false;
};
// Shared presentation events for ground effects and sound, never damage/saves.
struct CreatureContacts {
 static constexpr unsigned capacity=16;
 std::array<CreatureContact,capacity> events{};
 unsigned count=0;
 void Reset() {count=0;}
 void Add(CreatureContact event,rpg::Vec2 observer) {
   const auto distance=[&](const CreatureContact& e){return std::hypot(e.position.x-observer.x,e.position.z-observer.z);};
   if(!std::isfinite(event.position.x+event.position.y+event.position.z+event.strength+event.facing) || distance(event)>18 || event.strength<=0)return;
   if(count<capacity){events[count++]=event;return;}
   auto farthest=std::max_element(events.begin(),events.end(),[&](const auto& a,const auto& b){return distance(a)<distance(b);});
   if(distance(event)<distance(*farthest))*farthest=event;
 }
};
}
