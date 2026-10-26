#include "game/LandmarkView.h"
#include "game/BuildingStyle.h"
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
void Barrel(Renderer& r,XMFLOAT3 p,float width,float height) {
  r.DrawMesh(M::Barrel,p,{width,height,width},{.47f,.30f,.15f,1},S::Wood);
  for (float level : {-.31f,.31f})
    r.DrawMesh(M::Barrel,{p.x,p.y+level*height,p.z},{width*.96f,.055f,width*.96f},{.25f,.28f,.28f,1},S::Metal);
}
void Lantern(Renderer& r,float x,float y,float z) {
  r.DrawMesh(M::Box,{x,y,z},{.22f,.32f,.16f},{.93f,.69f,.28f,1},S::Glow,0,0,visual::ShadowMode::None);
  for (float side : {-1.f,1.f}) {
    Box(r,{x+side*.12f,y,z},{.035f,.38f,.2f},brass,S::Metal);
    Box(r,{x,y+side*.19f,z},{.28f,.045f,.2f},brass,S::Metal);
  }
  r.DrawMesh(M::Roof,{x,y+.26f,z},{.34f,.13f,.26f},wood,S::Metal);
}
}

void DrawBuilding(Renderer& r,const rpg::Obstacle& o) {
  if(o.role==rpg::BuildingRole::Homestead || o.role==rpg::BuildingRole::WatchHall) {
    const bool inn=o.role==rpg::BuildingRole::WatchHall;
    const auto asset=inn?authored::Asset::Inn:authored::Asset::Cottage;
    const auto& bounds=authored::Collection()[unsigned(asset)*authored::lodCount].bounds;
    const float height=std::min(o.height/(inn?2.55f:1.8f),(o.CameraHeight()-.05f)/bounds.maximum.y);
    r.DrawAuthoredAsset(asset,{o.center.x,0,o.center.z},
      {o.halfX/(inn?1.4f:1.1f),height,o.halfZ/.95f},DirectX::XM_PI);
    return;
  }
  const float x=o.center.x,z=o.center.z,roofHeight=o.RoofHeight();
  const auto style=StyleFor(o.role);
  const bool store=o.role==rpg::BuildingRole::Storehouse;
  Box(r,{x,.22f,z},{o.halfX*2+.2f,.44f,o.halfZ*2+.2f},stone,S::Stone);
  Box(r,{x,o.height*.5f,z},{o.halfX*2,o.height,o.halfZ*2},style.plaster,S::Plaster);
  // Separate stone courses break up the continuous plinth and catch sunlight.
  for(float face : {-1.f,1.f}) {
    const int blocks=int(std::ceil(o.halfX*2/.65f));
    const float width=o.halfX*2/blocks;
    for(int block=0;block<blocks;++block) {
      const float shade=.88f+.035f*((block+int(face)+3)%5);
      r.DrawMesh(M::Masonry,{x-o.halfX+(block+.5f)*width,.23f,z+face*o.halfZ},
        {width-.025f,.38f,.24f},{stone.x*shade,stone.y*shade,stone.z*shade,1},S::Stone);
    }
    for(float side : {-1.f,1.f}) for(int course=0;course<6;++course) {
      const float height=.53f+course*.29f;
      r.DrawMesh(M::Masonry,{x+side*(o.halfX-.11f),height,z+face*(o.halfZ+.025f)},
        {course%2?.28f:.40f,.25f,.18f},stone,S::Stone);
    }
  }
  for (float side : {-1.f,1.f}) {
    Box(r,{x+side*o.halfX,o.height-.12f,z},{.18f,.2f,o.halfZ*2},wood,S::Wood);
    const float wall=x+side*(o.halfX+.055f);
    Box(r,{wall,o.height*.5f,z},{.12f,o.height,.15f},wood,S::Wood);
    for (float bay : {-1.f,1.f}) {
      const float wz=z+bay*o.halfZ*.52f;
      Box(r,{wall,2.0f,wz},{.15f,.90f,.65f},wood,S::Wood);
      Box(r,{wall+side*.09f,2.0f,wz},{.04f,.69f,.46f},{.27f,.35f,.39f,1},S::Metal);
      Box(r,{wall+side*.12f,2.0f,wz},{.05f,.69f,.055f},wood,S::Wood);
      Box(r,{wall+side*.12f,2.0f,wz},{.05f,.055f,.46f},wood,S::Wood);
      Box(r,{wall,1.50f,wz},{.28f,.10f,.86f},stone,S::Stone);
      Box(r,{wall,2.50f,wz},{.23f,.12f,.80f},style.trim,S::Wood);
      r.DrawSegment({wall,.52f,z+bay*.15f},{wall,1.3f,z+bay*(o.halfZ-.15f)},.10f,.12f,style.trim,S::Wood);
    }
    for (float face : {-1.f,1.f}) {
      const float front=z+face*(o.halfZ+.035f),wx=x+side*o.halfX*.59f;
      Box(r,{x+side*(o.halfX-.12f),o.height*.5f,front},{.19f,o.height,.16f},wood,S::Wood);
      if (!store) {
        Box(r,{wx,2,front},{.65f,.95f,.16f},wood,S::Wood);
        Box(r,{wx,2,front+face*.09f},{.47f,.74f,.04f},{.27f,.35f,.39f,1},S::Metal);
        Box(r,{wx,2,front+face*.12f},{.055f,.74f,.05f},wood,S::Wood);
        Box(r,{wx,2,front+face*.12f},{.47f,.055f,.05f},wood,S::Wood);
        Box(r,{wx,1.48f,front+face*.035f},{.93f,.12f,.29f},stone,S::Stone);
        Box(r,{wx,2.53f,front},{.91f,.12f,.24f},style.trim,S::Wood);
        // Narrow iron glazing bars and shutter hinge straps remain close to the wall.
        for(float bar : {-1.f,1.f})
          Box(r,{wx+bar*.15f,2,front+face*.14f},{.018f,.70f,.025f},{.16f,.19f,.18f,1},S::Metal);
        for (float shutter : {-1.f,1.f}) {
          Box(r,{wx+shutter*.42f,2,front+.03f*face},{.19f,.86f,.10f},style.trim,S::Wood);
          for (int slat=0;slat<3;++slat)
            Box(r,{wx+shutter*.42f,1.73f+slat*.26f,front+.09f*face},{.20f,.035f,.035f},wood,S::Wood);
          for(float hinge : {-1.f,1.f})
            Box(r,{wx+shutter*.42f,2+hinge*.26f,front+face*.105f},{.21f,.035f,.025f},{.18f,.20f,.19f,1},S::Metal);
        }
      } else {
        for (int plank=0;plank<5;++plank)
          Box(r,{x+side*(.72f+plank*.12f),1.4f,front},{.095f,2.3f,.10f},wood,S::Wood);
      }
    }
  }
  for (float face : {-1.f,1.f}) {
    const float front=z+face*(o.halfZ+.09f);
    const float doorWidth=store?1.25f:.85f;
    Box(r,{x,.91f,front},{doorWidth,1.8f,.12f},wood,S::Wood);
    for(int plank=0;plank<6;++plank) {
      const float px=x-doorWidth*.5f+(plank+.5f)*doorWidth/6;
      const float shade=.88f+.04f*(plank%4);
      Box(r,{px,.91f,front+face*.07f},{doorWidth/6-.012f,1.72f,.025f},
        {wood.x*shade,wood.y*shade,wood.z*shade,1},S::Wood);
    }
    for(float hinge : { .43f,1.43f }) {
      Box(r,{x-doorWidth*.19f,hinge,front+face*.10f},{doorWidth*.58f,.055f,.035f},{.16f,.18f,.17f,1},S::Metal);
      for(float pin : {-.39f,.05f})
        r.DrawMesh(M::Sphere,{x+pin*doorWidth,hinge,front+face*.125f},{.025f,.025f,.018f},brass,S::Metal);
    }
    for (float edge : {-1.f,1.f}) Box(r,{x+edge*(doorWidth*.5f+.06f),.95f,front},{.10f,1.9f,.20f},style.trim,S::Wood);
    Box(r,{x,1.94f,front},{doorWidth+.23f,.15f,.20f},style.trim,S::Wood);
    Box(r,{x+.26f,1,front+face*.08f},{.07f,.09f,.06f},brass,S::Metal);
    Box(r,{x,o.height-.14f,front},{o.halfX*2,.2f,.2f},wood,S::Wood);
    Box(r,{x,.075f,front+face*.23f},{1.1f,.15f,.55f},stone,S::Stone);
    // Carved gable trim and roof braces remain inside the existing camera bounds.
    const float gable=z+face*(o.halfZ+.34f);
    for (float side : {-1.f,1.f})
      r.DrawSegment({x+side*(o.halfX+.16f),o.height,gable},{x,o.height+roofHeight+.04f,gable},.13f,.13f,wood,S::Wood);
    Box(r,{x,o.height+roofHeight*.28f,gable},{.13f,roofHeight*.56f,.13f},style.trim,S::Wood);
    if (o.role==rpg::BuildingRole::WatchHall) {
      r.DrawMesh(M::Torso,{x,o.height+.65f,gable+face*.07f},{.85f,.95f,.13f},style.trim,S::Cloth);
      Box(r,{x,o.height+.65f,gable+face*.14f},{.08f,.61f,.045f},brass,S::Metal);
      Box(r,{x,o.height+.48f,gable+face*.14f},{.4f,.08f,.045f},brass,S::Metal);
      for (float side : {-1.f,1.f}) {
        Box(r,{x+side*(o.halfX-.42f),2.8f,front},{.45f,1.45f,.055f},style.trim,S::Cloth);
        Box(r,{x+side*(o.halfX-.42f),3.53f,front},{.6f,.08f,.1f},brass,S::Metal);
      }
    } else if (o.role==rpg::BuildingRole::TrainingHall) {
      for (float side : {-1.f,1.f})
        r.DrawSegment({x+side*.34f,o.height+.25f,gable},{x-side*.34f,o.height+.90f,gable},.11f,.08f,brass,S::Wood);
    } else if (o.role==rpg::BuildingRole::Homestead) {
      Box(r,{x,o.height+.28f,gable},{.6f,.42f,.08f},{.21f,.30f,.30f,1},S::Metal);
      r.DrawMesh(M::Arch,{x,o.height+.42f,gable},{.85f,.8f,.13f},style.trim,S::Wood);
    } else {
      Box(r,{x,1.05f,front+face*.08f},{.055f,1.65f,.06f},brass,S::Metal);
      for (float side : {-1.f,1.f})
        r.DrawSegment({x+side*.52f,.3f,front+face*.1f},{x-side*.52f,1.7f,front+face*.1f},.09f,.08f,style.trim,S::Wood);
    }
    Lantern(r,x+doorWidth*.5f+.33f,1.62f,front+face*.13f);
  }
  r.DrawMesh(M::Roof,{x,o.height+roofHeight*.5f-.06f,z},{o.halfX*2+.6f,roofHeight,o.halfZ*2+.6f},style.roof,S::Roof);
  Box(r,{x,o.height+roofHeight-.03f,z},{.16f,.16f,o.halfZ*2+.7f},wood,S::Wood);
  Box(r,{x+o.halfX*.55f,o.height+roofHeight*.65f,z+.4f},{.55f,1.45f,.6f},stone,S::Stone);
  Box(r,{x+o.halfX*.55f,o.height+roofHeight*.65f+.75f,z+.4f},{.7f,.16f,.75f},stone,S::Stone);
  // Chimney brick courses and a dark inset flue replace the featureless block.
  for(int course=0;course<5;++course) {
    const float y=o.height+roofHeight*.65f-.55f+course*.27f;
    for(float face : {-1.f,1.f})
      Box(r,{x+o.halfX*.55f,y,z+.4f+face*.306f},{.55f,.025f,.015f},{.25f,.24f,.22f,1},S::Stone);
  }
  Box(r,{x+o.halfX*.55f,o.height+roofHeight*.65f+.835f,z+.4f},{.43f,.015f,.47f},{.045f,.04f,.035f,1},S::Plain);
}

void DrawSupplyStall(Renderer& r,const rpg::Obstacle& o) {
  const float x=o.center.x,z=o.center.z;
  Box(r,{x,o.height*.5f,z},{o.halfX*2,o.height,o.halfZ*2},wood,S::Wood);
  Box(r,{x,o.height+.02f,z},{o.halfX*2+.1f,.08f,o.halfZ*2+.06f},brass,S::Metal);
  for (float side : {-1.f,1.f}) {
    Box(r,{x+side*(o.halfX-.06f),1.3f,z},{.12f,2.6f,.12f},wood,S::Wood);
    r.DrawSegment({x+side*(o.halfX-.06f),2.08f,z},{x+side*.60f,2.57f,z},.08f,.08f,brass,S::Wood);
  }
  Box(r,{x,2.57f,z},{2.65f,.09f,.10f},wood,S::Wood);
  r.DrawMesh(M::Canopy,{x,2.63f,z},{2.75f,1,1.35f},{.38f,.355f,.28f,1},S::Cloth);
  for (int i=0;i<3;++i) {
    r.DrawMesh(M::Barrel,{x-.7f+i*.27f,o.height+.18f,z},{.16f,.29f,.16f},{.52f,.18f,.14f,1},S::Cloth);
    Box(r,{x-.7f+i*.27f,o.height+.36f,z},{.06f,.09f,.06f},brass,S::Metal);
  }
  Barrel(r,{x+.65f,o.height+.27f,z},.40f,.50f);
  // Framed parcel and tied cord, displayed on the same solid counter.
  Box(r,{x+.17f,o.height+.15f,z},{.40f,.25f,.40f},{.56f,.42f,.25f,1},S::Wood);
  Box(r,{x+.17f,o.height+.28f,z},{.045f,.03f,.43f},brass,S::Cloth);
  Box(r,{x+.17f,o.height+.28f,z},{.43f,.03f,.045f},brass,S::Cloth);
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
