#include "game/WorldView.h"
#include "game/WorldMap.h"
#include "game/CharacterView.h"
#include "game/LandmarkView.h"
#include "game/CourtView.h"
#include "game/ExpeditionView.h"
#include "game/PopulationView.h"
#include "game/LocationView.h"
#include "game/Grove.h"
#include "engine/Renderer.h"
#include "engine/Landscape.h"
#include <cmath>

namespace {
void DrawScenery(Renderer& r,const rpg::WorldMap& map) {
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
  courtview::Plates(r);
  expeditionview::CampDetails(r);
  populationview::Scenery(r);
  locationview::Details(r,map);
  // Low coastal strata remain outside the walkable rectangle. They extend
  // the cliff silhouette without introducing invisible collision in the meadow.
  for(int n=0;n<15;++n) {
    const float x=-26.f+n*3.7f;
    r.DrawMesh(M::Rock,{x,-.8f,-130.5f-float(n%3)*.35f},
      {3.4f+float(n%2)*.5f,1.4f+float(n%3)*.25f,2.5f},
      {.40f,.405f,.35f,1},S::Stone,float(n)*.7f);
  }
  for (const auto& cover : map.Cover()) {
    const auto elevation=r.Elevate(map.Height(cover.position));
    const float scale=cover.scale;
    if (cover.shrub) {
      const auto plant=int(std::abs(cover.position.x*7+cover.position.z*11))%3==0?authored::Asset::Fern:authored::Asset::Shrub;
      r.DrawAuthoredAsset(plant,{cover.position.x,0,cover.position.z},{scale,scale,scale},cover.yaw);
    } else {
      const bool meadow=cover.position.z<-19;
      r.DrawMesh(M::Grass,{cover.position.x,0,cover.position.z},{scale,(meadow?.55f:.42f)*scale,scale},
        meadow?XMFLOAT4{.40f,.37f,.25f,1}:XMFLOAT4{.29f,.34f,.23f,1},S::Leaves,cover.yaw,0,visual::ShadowMode::None);
      // Low, offset tufts make irregular patches instead of evenly spaced spikes.
      const bool outer=std::abs(cover.position.x)>68 || std::abs(cover.position.z)>76;
      for(int tuft=0;tuft<(outer?1:2);++tuft) {
        const float a=cover.yaw+tuft*2.4f;
        const rpg::Vec2 p{cover.position.x+std::sin(a)*.40f,cover.position.z+std::cos(a)*.40f};
        const auto surface=map.SurfaceAt(p);
        if(map.CanStand(p,.35f) && surface.dirt+surface.paving<.06f)
          r.DrawMesh(M::Grass,{p.x,map.Height(p)-map.Height(cover.position),p.z},{scale*.85f,scale*.24f,scale*.85f},
            {.33f,.34f,.24f,1},S::Leaves,a,0,visual::ShadowMode::None);
      }
    }
  }
  // Sparse, low chips complement stone groups. They no longer form an evenly
  // spaced bright dotted line on both sides of every footpath.
  for (const auto& trail : map.Trails()) {
    const float dx=trail.to.x-trail.from.x,dz=trail.to.z-trail.from.z,length=std::hypot(dx,dz);
    for (int i=0;i<int(length/2.3f);++i) {
      const float seed=std::sin(trail.from.x*12.31f+trail.from.z*7.17f+i*13.67f);
      if(seed<.3f)continue;
      const float t=(i+.3f+seed*.35f)*2.3f/length,side=i%2?-1.f:1.f,offset=trail.halfWidth+.4f+seed*.5f;
      const rpg::Vec2 p{trail.from.x+dx*t+dz/length*offset*side,trail.from.z+dz*t-dx/length*offset*side};
      if (!map.CanStand(p,.25f) || map.SurfaceAt(p).paving>.2f) continue;
      r.DrawMesh(M::Rock,{p.x,map.Height(p)-.015f,p.z},{.18f+seed*.12f,.07f,.23f+seed*.12f},{.34f,.33f,.29f,1},S::Stone,float(i));
    }
  }
  for (const auto& o : map.Obstacles()) {
    if(o.kind==rpg::PropKind::Dressing)continue;
    const auto elevation=r.Elevate(map.Height(o.center));
    const float x=o.center.x,z=o.center.z;
    if(z<-65 && (o.kind==rpg::PropKind::Building || o.kind==rpg::PropKind::Ruin)) {
      float drop=.2f;
      for(float sx:{-1.f,1.f})for(float sz:{-1.f,1.f})
        drop=std::max(drop,map.Height(o.center)-map.Height({x+sx*o.halfX,z+sz*o.halfZ})+.2f);
      box({x,-drop*.5f+.05f,z},{o.halfX*2,drop,o.halfZ*2},{.30f,.31f,.28f,1},S::Stone);
    }
    if(o.kind==rpg::PropKind::SurveyStone) continue; // Detailed marker assembly below.
    if (o.kind==rpg::PropKind::Counter) { DrawSupplyStall(r,o); continue; }
    if (o.kind==rpg::PropKind::Tree) {
      const auto profile=rpg::grove::Tree(o);const auto scale=rpg::grove::Size(o);
      const auto tree=authored::Asset(unsigned(authored::Asset::Oak)+unsigned(profile.species));
      const int variety=int(std::abs(x*7+z*11));
      r.DrawAuthoredAsset(tree,{x,-.04f,z},{scale.width,scale.height,scale.width},variety*.618034f);
      continue;
    }
    if (o.kind==rpg::PropKind::Ruin) {
      if(z>20 || z<-65) {
        const auto& b=authored::Collection()[unsigned(authored::Asset::Tower)*authored::lodCount].bounds;
        const float scale=std::min({(o.CameraHeight()-.1f)/b.maximum.y,o.halfX/std::max(std::abs(b.minimum.x),std::abs(b.maximum.x)),o.halfZ/std::max(std::abs(b.minimum.z),std::abs(b.maximum.z))});
        r.DrawAuthoredAsset(authored::Asset::Tower,{x,0,z},{scale,scale,scale});
        if(z<-65) for(int stone=0;stone<7;++stone) {
          const float a=stone*2.39996f;
          // Broken courses stay inside the already blocked tower footprint.
          r.DrawMesh(M::Rock,{x+std::cos(a)*o.halfX*.72f,.08f,z+std::sin(a)*o.halfZ*.75f},
            {.42f,.24f,.32f},{.34f,.35f,.31f,1},S::Stone,a);
        }
      } else DrawWatchRuin(r,o);
      continue;
    }
    DrawBuilding(r,o);
    if(z<-65) {
      // Small sheltered lanterns, weathered threshold stones and store fittings
      // reuse shared materials; their silhouettes fit the building camera box.
      const float front=z-o.halfZ-.22f;
      for(float side:{-1.f,1.f}) {
        const float lx=x+side*o.halfX*.65f;
        box({lx,1.85f,front},{.18f,.34f,.20f},{.12f,.13f,.12f,1},S::Metal);
        box({lx,1.85f,front-.11f},{.10f,.20f,.025f},{.72f,.40f,.12f,1},S::Glow);
        for(int iron=0;iron<3;++iron)box({lx+(iron-1)*.05f,1.85f,front-.13f},{.012f,.27f,.02f},rock,S::Metal);
      }
      for(int stone=0;stone<4;++stone)box({x-o.halfX+.5f+stone*(o.halfX*2-1)/3,.08f,front-.08f},
        {.6f,.16f,.3f},{.35f,.34f,.30f,1},S::Stone);
    }
  }
  // Decorative planks stay inside the playable island.
  const auto dockMin=rpg::WorldMap::dockMin,dockMax=rpg::WorldMap::dockMax;
  for (int i=0;i<10;++i) box({(dockMin.x+dockMax.x)*.5f,.07f,dockMin.z+.2f+i*.45f},
    {dockMax.x-dockMin.x,.12f,.4f},timber,S::Wood);
  for (float z : {dockMin.z+.2f,dockMax.z-.2f}) for (float x : {dockMin.x+.25f,dockMax.x-.25f})
    r.DrawMesh(M::Trunk,{x,.55f,z},{.34f,1.1f,.34f},timber,S::Wood);
  for (float x : {-4.f,4.f}) {
    r.DrawAuthoredAsset(authored::Asset::Signpost,{x,0,-3},{1,1.15f,1},DirectX::XM_PI);
  }
  for(const auto site:rpg::WorldMap::surveySites) {
    const auto elevation=r.Elevate(map.Height(site));
    r.DrawMesh(M::Rock,{site.x,.35f,site.z},{.7f,.7f,.5f},rock,S::Stone);
    r.DrawMesh(M::Crystal,{site.x,1.05f,site.z},{.12f,.22f,.12f},{.42f,.69f,.76f,1},S::Glow,0,0,visual::ShadowMode::None);
    box({site.x,.75f,site.z-.25f},{.35f,.20f,.04f},brass,S::Metal);
  }
  for(unsigned i=0;i<rpg::WorldMap::expeditionSites.size();++i) {
    const auto site=rpg::WorldMap::expeditionSites[i];const auto elevation=r.Elevate(map.Height(site));
    if(i>=6) {
      r.DrawAuthoredAsset(authored::Asset::Barrel,{site.x,0,site.z},{1,1,1});
      r.DrawAuthoredAsset(authored::Asset::SackStack,{site.x+.42f,0,site.z+.16f},{.65f,.65f,.65f});continue;
    }
    r.DrawMesh(M::Rock,{site.x,.55f,site.z},{.7f,1.1f,.5f},rock,S::Stone);
    r.DrawMesh(M::Crystal,{site.x,1.3f,site.z},{.16f,.3f,.16f},i<3?XMFLOAT4{.7f,.26f,.10f,1}:XMFLOAT4{.3f,.55f,.75f,1},S::Glow);
  }
  {const auto p=rpg::WorldMap::ritualSite;const auto elevation=r.Elevate(map.Height(p));
    r.DrawMesh(M::Rock,{p.x,.5f,p.z},{1.1f,1,1.1f},rock,S::Stone);
    r.DrawMesh(M::Crystal,{p.x,1.75f,p.z},{.4f,1.4f,.4f},{.34f,.62f,.75f,1},S::Glow);
  }
  // Wooded outcrops frame the horizon, outside the navigable/camera rectangle.
  // Shared authored trees use the existing view-dependent LOD and static cache.
  for (int i=0;i<11;++i) {
    const float angle=i*6.2831853f/11+.15f,x=std::cos(angle)*152,z=std::sin(angle)*164;
    const float sx=11.f+i%3*1.5f,sz=10.f+(i+1)%3*1.5f,sy=3.5f+(i%4)*.6f,y=-1.8f;
    r.DrawMesh(M::Ridge,{x,y,z},{sx,sy,sz},{1,1,1,1},S::Ground);
    for(int tree=0;tree<11;++tree) {
      const float a=tree*2.39996f+i,radius=.18f+float(tree%5)*.12f;
      const float u=std::sin(a)*radius,v=std::cos(a)*radius,scale=.70f+float((i+tree)%4)*.16f;
      const float ground=y+sy*visual::RidgeHeight(u,v)-.08f;
      r.DrawAuthoredAsset(tree%5==2?authored::Asset::DeadTree:tree%4==0?authored::Asset::Oak:authored::Asset::Pine,
        {x+u*sx,ground,z+v*sz},{scale*.78f,scale*1.35f,scale*.78f},a);
      if(tree%3==0) r.DrawMesh(M::Rock,{x+u*sx,ground-.1f,z+v*sz},
        {1.6f,.65f,1.1f},{.35f,.36f,.32f,1},S::Stone,a);
    }
  }
}
}
void WorldView::UpdateResidents(float dt,rpg::Vec2 player,bool alive,bool talking,rpg::Vec2 speaker) {
  for(unsigned i=0;i<residents_.size();++i) {
    const auto p=rpg::WorldMap::residents[i].position;
    const float distance=std::hypot(player.x-p.x,player.z-p.z);
    const auto aim=alive && distance<4?facial::LookAt(player.x-p.x,
      rpg::WorldMap::Height(player)-rpg::WorldMap::Height(p),player.z-p.z,3.14159f):facial::Aim{};
    const bool speaking=alive && talking && std::hypot(speaker.x-p.x,speaker.z-p.z)<.1f;
    residents_[i].Update(dt,aim,0,0,speaking?.5f:alive && distance<3?.12f:0);
  }
}
void WorldView::Draw(Renderer& r,const rpg::WorldMap& map,float time,bool sparring,facial::Pose merchantFace,facial::Pose keeperFace,rpg::Vec2 observer,const std::array<TrainingPropPose,3>& training) {
  // Reactive training props stay outside the immutable cache. Sparring no
  // longer rebuilds the entire village just to hide its first training post.
  r.DrawStaticScene(scenery_,[&] { DrawScenery(r,map); });
  locationview::Lanterns(r,map,observer,time);
  for(unsigned i=0;i<training.size();++i) {
    if(sparring && i==0)continue;
    const rpg::Vec2 position{12,-2.f+i*3.f};
    r.DrawAuthoredAsset(authored::Asset::TrainingDummy,{position.x,map.Height(position),position.z},
      {1,1,1},DirectX::XM_PI,3,training[i].pitch,training[i].roll);
  }
  // Ambient actors update outside rendering; repeated views cannot advance AI.
  if(wildlifePreview_) for(unsigned i=0;i<wildlife_.Animals().size();++i) {
    const auto& a=wildlife_.Animals()[i];
    r.DrawAnimatedAsset(i==0?authored::Asset::Wolf:authored::Asset::Boar,
      {a.position.x,map.Height(a.position),a.position.z},a.yaw,time+i*2,a.gait,0,a.travel);
  }
  using M=visual::Mesh;
  using S=visual::Material;
  for (const auto& o : map.Obstacles()) if (o.kind==rpg::PropKind::Ruin && std::abs(o.center.z)<20)
    r.DrawMesh(M::Crystal,{o.center.x,map.Height(o.center)+3.4f+std::sin(time)*.12f,o.center.z},{.7f,1.5f,.7f},
      {.35f,.68f,.81f,1},S::Glow,time*.3f,0,visual::ShadowMode::None);
  const auto giver=rpg::WorldMap::questGiver;
  CharacterOutfit merchant; merchant.weapon=false; merchant.helm=true;
  merchant.face=HumanFace::Broad;merchant.appearance={2,1,0};
  auto merchantMotion=animation::Idle(time,1.7f); merchantMotion.face=merchantFace;
  DrawCharacter(r,rpg::WorldMap::quartermaster,0,0,{.28f,.39f,.22f,1},0,0,merchant,merchantMotion,2);
  CharacterOutfit keeper; keeper.weapon=false;
  keeper.face=HumanFace::Lean;keeper.appearance={1,4,0};
  auto keeperMotion=animation::Idle(time,4.1f); keeperMotion.face=keeperFace;
  DrawCharacter(r,giver,0,1.25f,{.57f,.45f,.26f,1},0,0,keeper,keeperMotion,3);
  for(unsigned i=0;i<rpg::WorldMap::residents.size();++i) {
    const auto& npc=rpg::WorldMap::residents[i];
    if(std::hypot(npc.position.x-observer.x,npc.position.z-observer.z)>42)continue;
    const auto elevation=r.Elevate(map.Height(npc.position));CharacterOutfit outfit;
    static_assert(residentAppearances.size()==rpg::WorldMap::residents.size());
    outfit.face=residentAppearances[i].face;outfit.appearance=residentAppearances[i].colors;
    outfit.weapon=i==0 || i==12;outfit.armor=i==0 || i==12;outfit.cloak=i<9 || i==11 || i==12;outfit.helm=i==0 || i==14;
    outfit.classId=i==0?rpg::ClassId::Paladin:i==1 || i==12?rpg::ClassId::Scout:i==9 || i==10 || i==14?rpg::ClassId::Warrior:rpg::ClassId::Mage;
    const DirectX::XMFLOAT4 colors[]={{.47f,.30f,.17f,1},{.26f,.46f,.34f,1},{.38f,.30f,.55f,1}};
    auto idle=animation::Idle(time,6.1f+i);idle.face=residents_[i].pose;
    if(i==0 || i==2) {
      // These authored rigs have a chest joint, not independent facial bones.
      // A small upper-body acknowledgement preserves the planted lower body.
      authored::Gesture attention;attention.yaw=idle.face.yaw*.23f;
      attention.pitch=idle.face.pitch*.18f-idle.face.smile*.025f;
      r.DrawAnimatedAsset(i==0?authored::Asset::Guardian:authored::Asset::Mage,{npc.position.x,0,npc.position.z},3.14159f,time+i,0,0,0,3,{1,1,1,1},attention);
    } else DrawCharacter(r,npc.position,0,3.14159f,colors[i%3],0,0,outfit,idle,4+i);
    if(i<9) r.DrawMesh(M::Crystal,{npc.position.x,2.75f+std::sin(time*2+i)*.08f,npc.position.z},
      {.17f,.3f,.17f},{.85f,.67f,.27f,1},S::Glow,time,0,visual::ShadowMode::None);
  }
  r.DrawMesh(M::Crystal,{giver.x,2.9f+std::sin(time*2)*.15f,giver.z},{.30f,.55f,.30f},{1,.8f,.25f,1},S::Glow,time,0,visual::ShadowMode::None);
}
