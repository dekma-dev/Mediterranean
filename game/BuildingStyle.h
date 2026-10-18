#pragma once
#include "game/WorldMap.h"
#include <DirectXMath.h>

struct BuildingStyle {
  DirectX::XMFLOAT4 plaster,roof,trim;
};
// World art and map roofs use one palette for each existing building role.
inline BuildingStyle StyleFor(rpg::BuildingRole role) {
  using R=rpg::BuildingRole;
  switch (role) {
  case R::WatchHall: return {{.65f,.69f,.65f,1},{.23f,.31f,.39f,1},{.24f,.35f,.48f,1}};
  case R::Storehouse: return {{.64f,.57f,.42f,1},{.35f,.26f,.16f,1},{.43f,.31f,.15f,1}};
  case R::TrainingHall: return {{.74f,.67f,.48f,1},{.39f,.23f,.18f,1},{.50f,.28f,.17f,1}};
  default: return {{.79f,.71f,.54f,1},{.43f,.20f,.14f,1},{.30f,.41f,.25f,1}};
  }
}
