#pragma once
#include "game/WorldMap.h"
#include <DirectXMath.h>
class Renderer;
void DrawCharacter(Renderer& renderer,rpg::Vec2 position,float base,float yaw,DirectX::XMFLOAT4 color,
  float swing,float gait,bool armor,bool rewardWeapon = false);
