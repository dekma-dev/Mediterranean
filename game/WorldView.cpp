#include "game/WorldView.h"
#include "game/WorldMap.h"
#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include <cmath>

void DrawWorld(Renderer& r,const rpg::WorldMap& map,float time,bool sparring) {
  using DirectX::XMFLOAT3;
  using DirectX::XMFLOAT4;
  using M=visual::Mesh;
  using S=visual::Material;
  const XMFLOAT4 rock{.48f,.46f,.41f,1},timber{.29f,.19f,.105f,1};
  const XMFLOAT4 plaster{.76f,.68f,.51f,1},roof{.38f,.18f,.12f,1},brass{.65f,.47f,.22f,1};
  const auto box=[&](XMFLOAT3 p,XMFLOAT3 s,XMFLOAT4 color,S material,float yaw=0.f) {
    r.DrawMesh(M::Box,p,s,color,material,yaw);
  };
  r.DrawMesh(M::Terrain,{0,0,0},{1,1,1},{1,1,1,1},S::Ground);
  r.DrawMesh(M::Cliff,{0,0,0},{1,1,1},rock,S::Stone);
  // Small irregular stones break up the route without changing its collision.
  for (int i=0;i<32;++i) {
    const float side=i%2?-1.f:1.f,z=-9.f+float(i/2)*1.45f;
    r.DrawMesh(M::Rock,{side*(1.65f+.12f*std::sin(float(i)*3)),.04f,z},
      {.22f+float(i%3)*.09f,.14f,.35f+float(i%4)*.08f},rock,S::Stone,float(i));
  }
  for (const auto& o : map.Obstacles()) {
    const float x=o.center.x,z=o.center.z;
    if (o.kind==rpg::PropKind::Counter) {
      box({x,o.height*.5f,z},{o.halfX*2,o.height,o.halfZ*2},timber,S::Wood);
      box({x,o.height+.02f,z},{o.halfX*2+.1f,.08f,o.halfZ*2+.06f},brass,S::Metal);
      for (int i=0;i<3;++i) {
        r.DrawMesh(M::Sphere,{x-.65f+i*.43f,o.height+.15f,z},{.22f,.25f,.22f},{.45f,.18f,.16f,1},S::Cloth);
        box({x-.65f+i*.43f,o.height+.30f,z},{.08f,.10f,.08f},timber,S::Wood);
      }
      continue;
    }
    if (o.kind==rpg::PropKind::Tree) {
      r.DrawMesh(M::Trunk,{x,1.35f,z},{o.halfX*2,2.7f,o.halfZ*2},timber,S::Wood,z*.3f);
      for (int branch=0;branch<3;++branch) {
        const float angle=branch*2.1f+z,dx=std::sin(angle)*.65f,dz=std::cos(angle)*.65f;
        r.DrawSegment({x,1.35f,z},{x+dx,2.5f,z+dz},.2f,.2f,timber,S::Wood);
        r.DrawMesh(M::Foliage,{x+dx,2.8f+float(branch)*.22f,z+dz},{2.f,2.f,1.9f},
          {.18f+branch*.018f,.29f+branch*.02f,.105f,1},S::Leaves,angle);
      }
      continue;
    }
    if (o.kind==rpg::PropKind::Ruin) {
      box({x,o.height*.45f,z},{o.halfX*2,o.height*.9f,o.halfZ*2},rock,S::Stone);
      for (int row=0;row<4;++row) for (int stone=0;stone<6;++stone) {
        const float sx=x-o.halfX+.35f+stone*.66f,sy=.23f+row*.47f;
        r.DrawMesh(M::Rock,{sx,sy,z-o.halfZ},{.78f,.57f,.48f},rock,S::Stone,stone*.7f);
        if (row==3) r.DrawMesh(M::Rock,{sx,sy+.38f,z},{.75f,.6f+stone%2*.3f,1.9f},rock,S::Stone,stone*.2f);
      }
      r.DrawMesh(M::Crystal,{x,3.4f+std::sin(time)*.12f,z},{.7f,1.5f,.7f},{.35f,.68f,.81f,1},S::Glow,time*.3f);
      continue;
    }
    box({x,.22f,z},{o.halfX*2+.2f,.44f,o.halfZ*2+.2f},rock,S::Stone);
    box({x,o.height*.5f,z},{o.halfX*2,o.height,o.halfZ*2},plaster,S::Plaster);
    // Timber framing follows the existing building footprint and camera collision.
    for (float side : {-1.f,1.f}) {
      for (float face : {-1.f,1.f}) {
        const float front=z+face*(o.halfZ+.035f);
        box({x+side*(o.halfX-.12f),o.height*.5f,front},{.19f,o.height,.16f},timber,S::Wood);
        box({x+side*o.halfX*.59f,2.f,front},{.65f,.95f,.16f},timber,S::Wood);
        box({x+side*o.halfX*.59f,2.f,front+face*.09f},{.47f,.74f,.04f},{.23f,.30f,.32f,1},S::Metal);
        box({x+side*o.halfX*.59f,2.f,front+face*.12f},{.055f,.74f,.05f},timber,S::Wood);
        box({x+side*o.halfX*.59f,2.f,front+face*.12f},{.47f,.055f,.05f},timber,S::Wood);
      }
      box({x+side*o.halfX,o.height-.12f,z},{.18f,.2f,o.halfZ*2},timber,S::Wood);
    }
    for (float face : {-1.f,1.f}) {
      const float front=z+face*(o.halfZ+.09f);
      box({x,.91f,front},{.85f,1.8f,.12f},timber,S::Wood);
      box({x+.26f,1,front+face*.08f},{.07f,.09f,.06f},brass,S::Metal);
      box({x,o.height-.14f,front},{o.halfX*2,.2f,.2f},timber,S::Wood);
      box({x,.075f,front+face*.23f},{1.1f,.15f,.55f},rock,S::Stone);
    }
    const float roofHeight=o.RoofHeight();
    r.DrawMesh(M::Roof,{x,o.height+roofHeight*.5f-.06f,z},{o.halfX*2+.6f,roofHeight,o.halfZ*2+.6f},roof,S::Roof);
    box({x,o.height+roofHeight-.03f,z},{.16f,.16f,o.halfZ*2+.7f},timber,S::Wood);
    box({x+o.halfX*.55f,o.height+roofHeight*.65f,z+.4f},{.55f,1.45f,.6f},rock,S::Stone);
    box({x+o.halfX*.55f,o.height+roofHeight*.65f+.75f,z+.4f},{.7f,.16f,.75f},rock,S::Stone);
  }
  // Decorative planks stay inside the playable island.
  for (int i=0;i<13;++i) box({-12.5f,.07f,-3.f+i*.45f},{5.5f,.12f,.4f},timber,S::Wood);
  for (float z : {-3.f,2.4f}) for (float x : {-15.f,-10.f})
    r.DrawMesh(M::Trunk,{x,.55f,z},{.34f,1.1f,.34f},timber,S::Wood);
  for (float z : {-2.f,1.f,4.f}) {
    if (sparring && z==rpg::WorldMap::trainingPost.z) continue;
    r.DrawMesh(M::Trunk,{12,.7f,z},{.3f,1.4f,.3f},timber,S::Wood);
    r.DrawMesh(M::Torso,{12,1.3f,z},{.85f,.75f,.4f},{.6f,.49f,.29f,1},S::Cloth);
    r.DrawSegment({11.4f,1.4f,z},{12.6f,1.4f,z},.18f,.18f,timber,S::Wood);
    r.DrawMesh(M::Sphere,{12,1.9f,z},{.4f,.45f,.35f},{.61f,.49f,.28f,1},S::Cloth);
  }
  for (float x : {-4.f,4.f}) {
    r.DrawMesh(M::Trunk,{x,.2f,-3},{.7f,.4f,.7f},rock,S::Stone);
    r.DrawMesh(M::Trunk,{x,1.4f,-3},{.25f,2.8f,.25f},timber,S::Wood);
    box({x+.35f,2.2f,-3},{.7f,.9f,.045f},{.2f,.29f,.42f,1},S::Cloth);
    r.DrawMesh(M::Sphere,{x,2.85f,-3},{.26f,.36f,.26f},brass,S::Metal);
  }
  for (int i=0;i<7;++i) {
    const float angle=i*.9f,x=std::cos(angle)*48,z=std::sin(angle)*48,y=-4+float(i%3)*3;
    r.DrawMesh(M::Cliff,{x,y,z},{.24f,.8f,.26f},rock,S::Stone,angle);
    r.DrawMesh(M::Terrain,{x,y,z},{.24f,1,.26f},{1,1,1,1},S::Ground,angle);
    r.DrawMesh(M::Rock,{x,y+1,z},{3,3,3},rock,S::Stone,angle);
  }
  const auto giver=rpg::WorldMap::questGiver;
  CharacterOutfit merchant; merchant.weapon=false; merchant.helm=true;
  DrawCharacter(r,rpg::WorldMap::quartermaster,0,0,{.28f,.39f,.22f,1},0,0,merchant);
  r.DrawMesh(M::Rock,{giver.x,.1f,giver.z},{1.2f,.35f,1.2f},rock,S::Stone);
  r.DrawMesh(M::Trunk,{giver.x,.65f,giver.z},{.55f,1.3f,.55f},timber,S::Wood);
  r.DrawMesh(M::Sphere,{giver.x,1.55f,giver.z},{.55f,.7f,.55f},brass,S::Metal);
  r.DrawMesh(M::Crystal,{giver.x,2.5f+std::sin(time*2)*.15f,giver.z},{.38f,.7f,.38f},{1,.8f,.25f,1},S::Glow,time);
}
