#include "game/WorldView.h"
#include "game/WorldMap.h"
#include "game/CharacterView.h"
#include "game/LandmarkView.h"
#include "engine/Renderer.h"
#include <cmath>

namespace {
void DrawScenery(Renderer& r,const rpg::WorldMap& map,bool sparring) {
  using DirectX::XMFLOAT3;
  using DirectX::XMFLOAT4;
  using M=visual::Mesh;
  using S=visual::Material;
  const XMFLOAT4 rock{.48f,.46f,.41f,1},timber{.29f,.19f,.105f,1};
  const XMFLOAT4 brass{.65f,.47f,.22f,1};
  const auto box=[&](XMFLOAT3 p,XMFLOAT3 s,XMFLOAT4 color,S material,float yaw=0.f) {
    r.DrawMesh(M::Box,p,s,color,material,yaw);
  };
  r.DrawMesh(M::Terrain,{0,0,0},{1,1,1},{1,1,1,1},S::Ground);
  r.DrawMesh(M::Cliff,{0,0,0},{1,1,1},rock,S::Stone);
  // Low coastal strata remain outside the walkable rectangle. They extend
  // the cliff silhouette without introducing invisible collision in the meadow.
  for(int n=0;n<15;++n) {
    const float x=-26.f+n*3.7f;
    r.DrawMesh(M::Rock,{x,-.8f,-32.5f-float(n%3)*.35f},
      {3.4f+float(n%2)*.5f,1.4f+float(n%3)*.25f,2.5f},
      {.40f,.405f,.35f,1},S::Stone,float(n)*.7f);
  }
  for (const auto& cover : map.Cover()) {
    const float scale=cover.scale;
    if (cover.shrub) {
      r.DrawMesh(M::Foliage,{cover.position.x,.30f*scale,cover.position.z},{1.1f*scale,.65f*scale,1.05f*scale},
        {.30f,.40f,.16f,1},S::Leaves,cover.yaw);
    } else {
      const bool meadow=cover.position.z<-19;
      r.DrawMesh(M::Grass,{cover.position.x,0,cover.position.z},{scale,(meadow?.9f:.65f)*scale,scale},
        meadow?XMFLOAT4{.48f,.46f,.24f,1}:XMFLOAT4{.35f,.43f,.19f,1},S::Leaves,cover.yaw,0,visual::ShadowMode::None);
      if(meadow) {
        r.DrawMesh(M::Grass,{cover.position.x+.22f,0,cover.position.z-.18f},{scale*.6f,scale*.52f,scale*.6f},
          {.33f,.40f,.20f,1},S::Leaves,cover.yaw+1.7f,0,visual::ShadowMode::None);
        for(int stem=0;stem<3;++stem) r.DrawMesh(M::Feature,
          {cover.position.x+(stem-1)*.14f,.25f*scale,cover.position.z+float(stem%2)*.13f},
          {.065f,.035f,.065f},{.65f,.58f,.37f,1},S::Leaves,cover.yaw,0,visual::ShadowMode::None);
      }
    }
  }
  // Small verge stones follow the same route segments as the ground and atlas.
  for (const auto& trail : map.Trails()) {
    const float dx=trail.to.x-trail.from.x,dz=trail.to.z-trail.from.z,length=std::hypot(dx,dz);
    for (int i=0;i<int(length/1.4f);++i) {
      const float t=(i+.5f)*1.4f/length,side=i%2?-1.f:1.f,offset=trail.halfWidth+.35f;
      const rpg::Vec2 p{trail.from.x+dx*t+dz/length*offset*side,trail.from.z+dz*t-dx/length*offset*side};
      if (!map.CanStand(p,.25f) || map.SurfaceAt(p).paving>.2f) continue;
      r.DrawMesh(M::Rock,{p.x,.025f,p.z},{.21f+i%3*.045f,.095f,.25f+i%2*.07f},rock,S::Stone,float(i));
    }
  }
  for (const auto& o : map.Obstacles()) {
    const float x=o.center.x,z=o.center.z;
    if(o.kind==rpg::PropKind::SurveyStone) continue; // Detailed marker assembly below.
    if (o.kind==rpg::PropKind::Counter) { DrawSupplyStall(r,o); continue; }
    if (o.kind==rpg::PropKind::Tree) {
      r.DrawMesh(M::Trunk,{x,1.35f,z},{o.halfX*2,2.7f,o.halfZ*2},timber,S::Wood,z*.3f);
      for (int branch=0;branch<3;++branch) {
        const float angle=branch*2.1f+z,dx=std::sin(angle)*.65f,dz=std::cos(angle)*.65f;
        r.DrawSegment({x,1.35f,z},{x+dx,2.5f,z+dz},.2f,.2f,timber,S::Wood);
        r.DrawSegment({x+dx*.8f,2.25f,z+dz*.8f},{x+dx*1.65f,2.9f,z+dz*1.65f},.085f,.085f,timber,S::Wood);
        r.DrawMesh(M::Foliage,{x+dx,2.8f+float(branch)*.22f,z+dz},{2.f,2.f,1.9f},
          {.18f+branch*.018f,.29f+branch*.02f,.105f,1},S::Leaves,angle);
      }
      continue;
    }
    if (o.kind==rpg::PropKind::Ruin) { DrawWatchRuin(r,o); continue; }
    DrawBuilding(r,o);
  }
  // Decorative planks stay inside the playable island.
  const auto dockMin=rpg::WorldMap::dockMin,dockMax=rpg::WorldMap::dockMax;
  for (int i=0;i<10;++i) box({(dockMin.x+dockMax.x)*.5f,.07f,dockMin.z+.2f+i*.45f},
    {dockMax.x-dockMin.x,.12f,.4f},timber,S::Wood);
  for (float z : {dockMin.z+.2f,dockMax.z-.2f}) for (float x : {dockMin.x+.25f,dockMax.x-.25f})
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
  for(const auto site:rpg::WorldMap::surveySites) {
    r.DrawMesh(M::Rock,{site.x,.35f,site.z},{.7f,.7f,.5f},rock,S::Stone);
    r.DrawMesh(M::Crystal,{site.x,1.05f,site.z},{.12f,.22f,.12f},{.42f,.69f,.76f,1},S::Glow,0,0,visual::ShadowMode::None);
    box({site.x,.75f,site.z-.25f},{.35f,.20f,.04f},brass,S::Metal);
  }
  for (int i=0;i<7;++i) {
    const float angle=i*.9f,x=std::cos(angle)*48,z=std::sin(angle)*48,y=-4+float(i%3)*3;
    r.DrawMesh(M::Cliff,{x,y,z},{.24f,.8f,.26f},rock,S::Stone,angle);
    r.DrawMesh(M::Terrain,{x,y,z},{.24f,1,.26f},{1,1,1,1},S::Ground,angle);
    r.DrawMesh(M::Rock,{x,y+1,z},{3,3,3},rock,S::Stone,angle);
  }
}
}
void WorldView::Draw(Renderer& r,const rpg::WorldMap& map,float time,bool sparring,facial::Pose merchantFace,facial::Pose keeperFace) {
  // The map is immutable between Game::Initialize calls. Sparring temporarily
  // replaces one authored dummy with an animated opponent.
  if (sparring!=sparring_) { scenery_.Invalidate(); sparring_=sparring; }
  r.DrawStaticScene(scenery_,[&] { DrawScenery(r,map,sparring); });
  using M=visual::Mesh;
  using S=visual::Material;
  for (const auto& o : map.Obstacles()) if (o.kind==rpg::PropKind::Ruin)
    r.DrawMesh(M::Crystal,{o.center.x,3.4f+std::sin(time)*.12f,o.center.z},{.7f,1.5f,.7f},
      {.35f,.68f,.81f,1},S::Glow,time*.3f,0,visual::ShadowMode::None);
  const auto giver=rpg::WorldMap::questGiver;
  CharacterOutfit merchant; merchant.weapon=false; merchant.helm=true;
  auto merchantMotion=animation::Idle(time,1.7f); merchantMotion.face=merchantFace;
  DrawCharacter(r,rpg::WorldMap::quartermaster,0,0,{.28f,.39f,.22f,1},0,0,merchant,merchantMotion,2);
  CharacterOutfit keeper; keeper.weapon=false;
  auto keeperMotion=animation::Idle(time,4.1f); keeperMotion.face=keeperFace;
  DrawCharacter(r,giver,0,1.25f,{.57f,.45f,.26f,1},0,0,keeper,keeperMotion,3);
  for(unsigned i=0;i<rpg::WorldMap::residents.size();++i) {
    const auto& npc=rpg::WorldMap::residents[i];CharacterOutfit outfit;
    outfit.weapon=i==0;outfit.armor=i==0;outfit.cloak=true;outfit.helm=i==0;
    outfit.classId=i==0?rpg::ClassId::Paladin:i==1?rpg::ClassId::Scout:rpg::ClassId::Mage;
    const DirectX::XMFLOAT4 colors[]={{.47f,.30f,.17f,1},{.26f,.46f,.34f,1},{.38f,.30f,.55f,1}};
    DrawCharacter(r,npc.position,0,3.14159f,colors[i],0,0,outfit,animation::Idle(time,6.1f+i),4+i);
    r.DrawMesh(M::Crystal,{npc.position.x,2.75f+std::sin(time*2+i)*.08f,npc.position.z},
      {.17f,.3f,.17f},{.85f,.67f,.27f,1},S::Glow,time,0,visual::ShadowMode::None);
  }
  r.DrawMesh(M::Crystal,{giver.x,2.9f+std::sin(time*2)*.15f,giver.z},{.30f,.55f,.30f},{1,.8f,.25f,1},S::Glow,time,0,visual::ShadowMode::None);
}
