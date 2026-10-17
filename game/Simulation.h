#pragma once
#include <array>
#include <string>
#include <vector>

namespace rpg {
struct Vec2 { float x = 0, z = 0; };
struct Obstacle { Vec2 center; float halfX, halfZ, height; };
struct Enemy {
  Vec2 position, home;
  float health = 80, attackCooldown = 0, hitFlash = 0;
};
struct Command {
  Vec2 movement;
  bool sprint = false, jump = false, nextTarget = false, interact = false, restart = false;
  int ability = -1;
};
enum class Quest { Available, Active, Return, Complete };

class Simulation {
public:
  Simulation();
  void Update(float seconds, const Command& command);
  bool CanStand(Vec2 position, float radius = .4f) const;
  float CameraDistance(Vec2 origin, float originY, Vec2 direction, float slope, float desired) const;
  const std::vector<Obstacle>& Obstacles() const { return obstacles_; }
  const std::vector<Enemy>& Enemies() const { return enemies_; }
  Vec2 Player() const { return player_; }
  float Height() const { return height_; }
  float Facing() const { return facing_; }
  float Health() const { return health_; }
  float AttackFlash() const { return attackFlash_; }
  int Target() const { return target_; }
  int Gold() const { return gold_; }
  Quest QuestState() const { return quest_; }
  float Cooldown(int index) const { return cooldowns_[index]; }
  const std::string& Message() const { return message_; }
  std::string Save() const;
  bool Load(const std::string& data);
  void Notify(const std::string& message) { message_ = message; }
  static constexpr Vec2 questGiver{-3,-10};
  static constexpr std::array<float,4> abilityCooldowns{.9f,4.f,5.f,10.f};
private:
  void Move(Vec2& position, Vec2 displacement, float radius);
  void SelectTarget();
  void UseAbility(int index);
  void Interact();
  bool HasLineOfSight(Vec2 from, Vec2 to) const;
  std::vector<Obstacle> obstacles_;
  std::vector<Enemy> enemies_;
  Vec2 player_{0,-10};
  float height_ = 0, velocityY_ = 0, facing_ = 0, health_ = 100, attackFlash_ = 0;
  std::array<float,4> cooldowns_{};
  int target_ = -1, gold_ = 0;
  Quest quest_ = Quest::Available;
  std::string message_ = "Approach the gold beacon and press E to accept the patrol.";
};
}
