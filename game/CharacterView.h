#pragma once
#include "game/WorldMap.h"
#include "game/ClassEquipment.h"
#include "game/Appearance.h"
#include <DirectXMath.h>
#include <cstdint>
#include "game/Animation.h"
class Renderer;
struct CharacterOutfit {
  bool armor=false,rewardWeapon=false,weapon=true,tunic=true,trousers=true,boots=true;
  bool helm=false,shield=false;
  bool shoulders=false,gloves=false,belt=true,cloak=false;
  rpg::ClassId classId=rpg::ClassId::Warrior;
  rpg::Appearance appearance;
  static CharacterOutfit From(const rpg::Inventory& inventory,rpg::ClassId id=rpg::ClassId::Warrior,rpg::Appearance look={});
};
void DrawCharacter(Renderer& renderer,rpg::Vec2 position,float base,float yaw,DirectX::XMFLOAT4 color,
  float swing,float gait,CharacterOutfit outfit = {},animation::Sample dynamics = {},std::uint64_t actor=0);
