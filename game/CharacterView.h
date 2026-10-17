#pragma once
#include "game/WorldMap.h"
#include "game/Inventory.h"
#include <DirectXMath.h>
class Renderer;
struct CharacterOutfit {
  bool armor=false,rewardWeapon=false,weapon=true,tunic=true,trousers=true,boots=true;
  bool helm=false,shield=false;
  static CharacterOutfit From(const rpg::Inventory& inventory);
};
void DrawCharacter(Renderer& renderer,rpg::Vec2 position,float base,float yaw,DirectX::XMFLOAT4 color,
  float swing,float gait,CharacterOutfit outfit = {});
