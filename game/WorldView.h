#pragma once
class Renderer;
namespace rpg { class WorldMap; }
void DrawWorld(Renderer& renderer, const rpg::WorldMap& map, float time);
