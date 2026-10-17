#include "game/WorldView.h"
#include "game/WorldMap.h"
#include "engine/Renderer.h"
#include <cmath>

void DrawWorld(Renderer& r,const rpg::WorldMap& map,float time) {
  using DirectX::XMFLOAT4;
  const XMFLOAT4 grass{.34f,.48f,.29f,1}, rock{.38f,.34f,.43f,1};
  const XMFLOAT4 timber{.34f,.18f,.10f,1}, plaster{.81f,.70f,.50f,1};
  const XMFLOAT4 roof{.48f,.12f,.16f,1}, brass{.87f,.62f,.23f,1};
  r.DrawBox({0,-1,0},{34,2,38},grass);
  r.DrawBox({0,-3,0},{29,3,33},rock,.04f);
  r.DrawPyramid({1,-6,1},{23,5,27},{.25f,.27f,.37f,1},0,3.14159265f);
  r.DrawBox({0,.025f,0},{3,.05f,30},{.66f,.55f,.38f,1});
  r.DrawBox({0,.03f,-11},{24,.05f,3},{.62f,.52f,.36f,1});
  r.DrawBox({0,.03f,0},{26,.05f,2},{.59f,.51f,.37f,1});
  // Low stone borders give the primary route a readable shape.
  for (float x : {-1.7f,1.7f}) for (int z=-9;z<14;z+=3)
    r.DrawBox({x,.08f,float(z)},{.25f,.15f,1.7f},{.57f,.56f,.53f,1});
  for (const auto& o : map.Obstacles()) {
    const float x=o.center.x,z=o.center.z;
    if (o.kind==rpg::PropKind::Tree) {
      r.DrawBox({x,1,z},{o.halfX*2,2,o.halfZ*2},timber);
      r.DrawPyramid({x,2.1f,z},{2.5f,2.5f,2.5f},{.14f,.31f,.22f,1});
      r.DrawPyramid({x,3,z},{1.8f,1.8f,1.8f},{.20f,.39f,.26f,1},.2f);
      continue;
    }
    if (o.kind==rpg::PropKind::Ruin) {
      r.DrawBox({x,o.height*.5f,z},{o.halfX*2,o.height,o.halfZ*2},rock);
      for (int i=-2;i<=2;++i) r.DrawBox({x+i*.8f,o.height+.3f,z},{.45f,.6f,o.halfZ*2},rock);
      r.DrawPyramid({x,3.4f+std::sin(time)*.12f,z},{.7f,1.5f,.7f},{.35f,.68f,.81f,1},time*.3f);
      continue;
    }
    r.DrawBox({x,.25f,z},{o.halfX*2+.2f,.5f,o.halfZ*2+.2f},rock);
    r.DrawBox({x,o.height*.5f,z},{o.halfX*2,o.height,o.halfZ*2},plaster);
    for (float side : {-1.f,1.f}) {
      r.DrawBox({x+side*(o.halfX-.15f),o.height*.5f,z-o.halfZ-.04f},{.22f,o.height,.15f},timber);
      r.DrawBox({x+side*o.halfX*.58f,2.f,z-o.halfZ-.08f},{.65f,.9f,.12f},timber);
      r.DrawBox({x+side*o.halfX*.58f,2.f,z-o.halfZ-.15f},{.44f,.66f,.04f},{.96f,.69f,.30f,1});
    }
    r.DrawBox({x,1,z-o.halfZ-.04f},{.85f,2,.1f},timber);
    r.DrawBox({x,o.height-.2f,z-o.halfZ-.06f},{o.halfX*2,.2f,.16f},timber);
    r.DrawBox({x,o.height+.08f,z},{o.halfX*2+.6f,.16f,o.halfZ*2+.6f},brass);
    r.DrawPyramid({x,o.height+.5f,z},{o.halfX*2+.7f,1.f,o.halfZ*2+.7f},roof);
  }
  // Dock planks and low rails are decorative; the dock lies within island bounds.
  for (int i=0;i<13;++i) r.DrawBox({-12.5f,.07f,-3.f+i*.45f},{5.5f,.12f,.38f},timber);
  for (float z : {-3.f,2.4f}) for (float x : {-15.f,-10.f})
    r.DrawBox({x,.65f,z},{.22f,1.3f,.22f},brass);
  // Training targets deliberately remain non-solid props during this milestone.
  for (float z : {-2.f,1.f,4.f}) {
    r.DrawBox({12,.7f,z},{.2f,1.4f,.2f},timber);
    r.DrawBox({12,1.5f,z},{.95f,.95f,.18f},plaster);
    r.DrawBox({12,1.5f,z-.1f},{.4f,.4f,.04f},roof);
  }
  for (float x : {-4.f,4.f}) {
    r.DrawBox({x,1.4f,-3},{.16f,2.8f,.16f},brass);
    r.DrawBox({x+.35f,2.2f,-3},{.7f,.9f,.06f},{.18f,.34f,.63f,1});
    r.DrawPyramid({x,2.95f,-3},{.3f,.4f,.3f},brass);
  }
  for (int i=0;i<7;++i) {
    const float angle=i*.9f,x=std::cos(angle)*48,z=std::sin(angle)*48,y=-4+float(i%3)*3;
    r.DrawBox({x,y,z},{8,4,10},rock,angle);
    r.DrawBox({x,y+2.1f,z},{8,.3f,10},grass,angle);
    r.DrawPyramid({x,y+3.2f,z},{2,3,2},{.36f,.50f,.57f,1},angle);
  }
  const auto giver=rpg::WorldMap::questGiver;
  r.DrawBox({giver.x,.1f,giver.z},{1.4f,.2f,1.4f},rock);
  r.DrawBox({giver.x,1,giver.z},{.8f,1.8f,.8f},brass);
  r.DrawPyramid({giver.x,2.5f+std::sin(time*2)*.15f,giver.z},{.55f,.9f,.55f},{1,.8f,.25f,1},time);
}
