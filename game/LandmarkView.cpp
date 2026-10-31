#include "game/LandmarkView.h"
#include "engine/Renderer.h"
#include <cmath>

namespace {
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using M=visual::Mesh;
using S=visual::Material;
const XMFLOAT4 stone{.48f,.46f,.41f,1},wood{.29f,.19f,.105f,1},brass{.65f,.47f,.22f,1};
void Box(Renderer& r,XMFLOAT3 p,XMFLOAT3 size,XMFLOAT4 color,S material) {
  r.DrawMesh(M::Box,p,size,color,material);
}
void Lantern(Renderer& r,float x,float y,float z) {
  r.DrawAuthoredAsset(authored::Asset::Lantern,{x,y-.27f,z},{1,1,1});
}
}

void DrawBuilding(Renderer& r,const rpg::Obstacle& o) {
  const bool wide=o.role==rpg::BuildingRole::WatchHall || o.role==rpg::BuildingRole::TrainingHall;
  const auto asset=o.role==rpg::BuildingRole::Storehouse?authored::Asset::Storehouse:
    o.role==rpg::BuildingRole::TrainingHall?authored::Asset::TrainingHall:
    wide?authored::Asset::Inn:authored::Asset::Cottage;
  const auto& bounds=authored::Collection()[unsigned(asset)*authored::lodCount].bounds;
  const float width=std::min(o.halfX/(wide?1.4f:1.1f),(o.CameraHalfX()-.03f)/std::max(std::abs(bounds.minimum.x),std::abs(bounds.maximum.x)));
  const float height=std::min(o.height/(wide?2.55f:1.8f),(o.CameraHeight()-.05f)/bounds.maximum.y);
  const float depth=std::min(o.halfZ/.95f,(o.CameraHalfZ()-.03f)/std::max(std::abs(bounds.minimum.z),std::abs(bounds.maximum.z)));
  r.DrawAuthoredAsset(asset,{o.center.x,0,o.center.z},{width,height,depth},DirectX::XM_PI);
  Lantern(r,o.center.x+o.halfX*.55f,std::min(1.75f,o.height*.68f),o.center.z-o.halfZ-.13f);
}

void DrawSupplyStall(Renderer& r,const rpg::Obstacle& o) {
  const auto& b=authored::Collection()[unsigned(authored::Asset::SupplyStall)*authored::lodCount].bounds;
  const float width=std::min(o.halfX/1.15f,(o.CameraHalfX()-.03f)/std::max(std::abs(b.minimum.x),std::abs(b.maximum.x)));
  const float depth=std::min(o.halfZ/.47f,(o.CameraHalfZ()-.03f)/std::max(std::abs(b.minimum.z),std::abs(b.maximum.z)));
  r.DrawAuthoredAsset(authored::Asset::SupplyStall,{o.center.x,0,o.center.z},{width,1,depth},DirectX::XM_PI);
}

void DrawWatchRuin(Renderer& r,const rpg::Obstacle& o) {
  const float x=o.center.x,z=o.center.z;
  // The blocked gateway is visibly filled with masonry; it does not promise a passage.
  Box(r,{x,.9f,z},{o.halfX*2,1.8f,1.7f},stone,S::Stone);
  for (float face : {-1.f,1.f}) {
    const float front=z+face*1.07f;
    for (int row=0;row<4;++row) for (int block=0;block<6;++block) {
      const float bx=x-1.65f+block*.66f,by=.23f+row*.44f;
      const float shade=.86f+float((block+row*3)%5)*.045f;
      r.DrawMesh(M::Masonry,{bx,by,front-face*.10f},{.65f,.43f,.25f},
        {stone.x*shade,stone.y*shade,stone.z*shade,1},S::Stone);
    }
    r.DrawMesh(M::Arch,{x,1.45f,front},{2.9f,2.55f,.20f},{.61f,.58f,.49f,1},S::Stone);
    for (float side : {-1.f,1.f}) {
      Box(r,{x+side*1.55f,1.16f,front},{.58f,2.32f,.24f},stone,S::Stone);
      for (int row=0;row<5;++row)
        Box(r,{x+side*1.55f,.23f+row*.45f,front+face*.04f},{.62f,.045f,.25f},{.33f,.32f,.29f,1},S::Stone);
      r.DrawMesh(M::Rock,{x+side*1.55f,2.38f,front},{.64f,.32f,.28f},stone,S::Stone,side);
    }
    Box(r,{x-.45f,1.0f,front+face*.14f},{.36f,1.2f,.04f},{.35f,.15f,.23f,1},S::Cloth);
    Box(r,{x-.45f,1.61f,front+face*.14f},{.47f,.06f,.06f},brass,S::Metal);
  }
  for (int i=0;i<5;++i)
    r.DrawMesh(M::Rock,{x-1.35f+i*.66f,1.94f,z},{.70f,.37f+i%2*.17f,1.65f},stone,S::Stone,i*.5f);
}
