#pragma once

class Renderer;

class Game {
public:
  void Initialize();
  void Update(double deltaSeconds);
  void Render(Renderer& renderer);
};