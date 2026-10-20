#pragma once
#include "game/WorldMap.h"
class Renderer;
void DrawBuilding(Renderer& renderer,const rpg::Obstacle& building);
void DrawSupplyStall(Renderer& renderer,const rpg::Obstacle& counter);
void DrawWatchRuin(Renderer& renderer,const rpg::Obstacle& ruin);
