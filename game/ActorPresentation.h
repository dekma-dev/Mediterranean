#pragma once
#include "game/EnemyDefeat.h"
#include "game/FootPlacement.h"
#include "game/BeastLocomotion.h"

namespace presentation {
struct Actor {
 rpg::EntityId id=0;
 rpg::EnemyKind kind=rpg::EnemyKind::Raider;
 int rank=0;
 rpg::Vec2 home{};
 bool alive=false;
 motion::Actor motion;
 motion::MovementSample movement;
 animation::State animation;
 EnemyDefeat defeat;
 facial::State face;
 animation::FootPlacement feet;
 BeastLocomotion beast;
 double clock=0;
 bool Matches(const rpg::Enemy& enemy) const {
   return id==enemy.id && kind==enemy.kind && rank==enemy.rank && home.x==enemy.home.x && home.z==enemy.home.z;
 }
 void Reset(const rpg::Enemy& enemy) {
   *this={};id=enemy.id;kind=enemy.kind;rank=enemy.rank;home=enemy.home;alive=enemy.health>0;
   motion.Reset(enemy.facing);animation.Reset(float(id)*.37f);defeat.Reset(alive);
   face.Reset(id+16);feet.Reset(enemy.position,enemy.facing);movement.Reset(enemy.position.x,enemy.position.z);
   beast.Reset(id);if(enemy.Beast())beast.Place(enemy);
   clock=double(id%31)*.137;
   // Saves can resume inside an authoritative windup. Restore that pose before
   // the first render instead of flashing idle until the next update.
   if(alive && enemy.windup>0) {
     motion.attack=combat::Smooth(1-enemy.windup/enemy.WindupDuration());
     if(enemy.kind==rpg::EnemyKind::Wolf)beast.gesture.jaw=beast.jaw=motion.attack;
   }
   if(!alive)defeat.held=Gesture(); // Settled corpses retain their terrain plane.
 }
 authored::Gesture Gesture() const {return kind==rpg::EnemyKind::Wolf || kind==rpg::EnemyKind::Boar?beast.gesture:AuthoredActorGesture(animation.pose);}
};

// Keep the hot update allocation-free. Only roster changes rebuild the small
// vector, matching identities so insertion/reordering cannot swap animation.
template<class Enemies> void Bind(std::vector<Actor>& actors,const Enemies& enemies,const rpg::Enemy& trainer) {
 const auto count=enemies.size()+1;
 const auto at=[&](std::size_t i)->const rpg::Enemy& {return i<enemies.size()?enemies[i]:trainer;};
 bool same=actors.size()==count;
 if(same)for(std::size_t i=0;i<count;++i)if(!actors[i].Matches(at(i))){same=false;break;}
 if(!same) {
   std::vector<Actor> next;next.reserve(count);
   for(std::size_t i=0;i<count;++i) {
     const auto& enemy=at(i);
     const auto old=std::find_if(actors.begin(),actors.end(),[&](const Actor& a){return a.Matches(enemy);});
     if(old!=actors.end())next.push_back(std::move(*old));
     else {next.emplace_back();next.back().Reset(enemy);}
   }
   actors=std::move(next);
 }
 for(std::size_t i=0;i<count;++i) {
   const auto& enemy=at(i);auto& actor=actors[i];
   const bool teleported=std::hypot(enemy.position.x-actor.movement.x,enemy.position.z-actor.movement.z)>.85f;
   if(enemy.health>0 && (!actor.alive || teleported))actor.Reset(enemy);
   actor.alive=enemy.health>0;
 }
}
}
