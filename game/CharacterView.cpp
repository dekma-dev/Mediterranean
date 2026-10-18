#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "engine/Renderer.h"
#include <cmath>
CharacterOutfit CharacterOutfit::From(const rpg::Inventory& inventory) {
 return {inventory.Worn(rpg::GearSlot::Chest)==rpg::Item::Ward,inventory.Worn(rpg::GearSlot::Weapon)==rpg::Item::Watchguard,
  inventory.Worn(rpg::GearSlot::Weapon)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Chest)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Legs)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Feet)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Head)==rpg::Item::Helm,inventory.Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield};
}
void DrawCharacter(Renderer& renderer,rpg::Vec2 p,float base,float yaw,DirectX::XMFLOAT4 color,float swing,float gait,CharacterOutfit outfit,animation::Sample dynamics) {
 using namespace DirectX; using M=visual::Mesh; using S=visual::Material;
    const auto pose=BuildCharacterPose(swing,gait,outfit.shield,dynamics);
    const XMFLOAT4 skin{.72f,.51f,.35f,1},leather{.20f,.14f,.095f,1},steel{.52f,.59f,.61f,1};
    const XMFLOAT4 blade=outfit.rewardWeapon?XMFLOAT4{.76f,.67f,.42f,1}:steel;
    const XMFLOAT4 cloth=outfit.tunic?color:XMFLOAT4{.61f,.58f,.46f,1};
    const auto point=[&](float x,float h,float z) -> XMFLOAT3 {
      return {p.x+std::cos(yaw)*x+std::sin(yaw)*z,base+h,p.z-std::sin(yaw)*x+std::cos(yaw)*z};
    };
    const auto world=[&](XMFLOAT3 local) { return point(local.x,local.y,local.z); };
    const auto upper=[&](XMFLOAT3 local) { return world(pose.UpperPoint(local)); };
    const auto part=[&](M mesh,float x,float h,float z,XMFLOAT3 scale,XMFLOAT4 tint,S material) {
      renderer.DrawMesh(mesh,upper({x,h,z}),scale,tint,material,yaw,pose.upperPitch);
    };
    part(M::Torso,0,1.23f,0,{.61f,.64f,.36f},outfit.armor?steel:cloth,outfit.armor?S::Metal:S::Cloth);
    part(M::Limb,0,.95f,0,{.51f,.08f,.32f},leather,S::Cloth);
    part(M::Box,0,.95f,.16f,{.10f,.075f,.025f},{.67f,.49f,.22f,1},S::Metal);
    if (outfit.tunic) {
      part(M::Torso,0,.83f,.015f,{.54f,.27f,.36f},cloth,S::Cloth);
      const XMFLOAT3 seam[]={{0,1.02f,.139f},{0,1.064f,.135f},{0,1.198f,.149f},{0,1.371f,.171f},{0,1.448f,.149f}};
      for (int i=0;i<4;++i) renderer.DrawMeshBetween(M::Box,
        upper(seam[i]),upper(seam[i+1]),
        .022f,.009f,{.63f,.50f,.25f,1},S::Cloth,{std::sin(yaw),0,std::cos(yaw)});
      part(M::Torso,0,1.51f,0,{.25f,.065f,.24f},leather,S::Cloth);
    }
    if (outfit.armor) {
      part(M::Torso,0,1.29f,.18f,{.17f,.22f,.035f},{.68f,.53f,.26f,1},S::Metal);
      part(M::Torso,0,1.29f,.20f,{.09f,.14f,.02f},{.21f,.37f,.43f,1},S::Metal);
    }
    part(M::Limb,0,1.57f,0,{.15f,.17f,.15f},skin,S::Skin);
    part(M::Head,0,1.76f,.012f,{.265f,.31f,.26f},skin,S::Skin);
    if (!outfit.helm) part(M::Hair,0,1.76f,.012f,{.267f,.31f,.262f},{.19f,.13f,.08f,1},S::Cloth);
    part(M::Sphere,0,1.748f,.12f,{.037f,.06f,.055f},skin,S::Skin);
    part(M::Box,0,1.70f,.106f,{.051f,.006f,.01f},{.36f,.22f,.16f,1},S::Skin);
    if (outfit.helm) {
      part(M::Sphere,0,1.86f,.012f,{.31f,.21f,.30f},steel,S::Metal);
      part(M::Box,0,1.817f,.15f,{.26f,.035f,.04f},{.72f,.54f,.24f,1},S::Metal);
      part(M::Blade,0,1.99f,0,{.04f,.19f,.16f},{.26f,.38f,.47f,1},S::Cloth);
      for (float side : {-1.f,1.f}) part(M::Torso,side*.13f,1.73f,.075f,{.055f,.18f,.115f},steel,S::Metal);
    }
    // Fixed-length joints prevent the old stretched forearms during a strike.
    for (int i=0;i<2;++i) {
      const float side=i==0?-1.f:1.f;
      part(M::Sphere,side*.052f,1.784f,.11f,{.019f,.011f,.012f},{.11f,.10f,.08f,1},S::Plain);
      part(M::Box,side*.053f,1.801f,.112f,{.043f,.009f,.012f},{.22f,.15f,.10f,1},S::Cloth);
      part(M::Sphere,side*.121f,1.755f,-.004f,{.03f,.06f,.037f},skin,S::Skin);
      const auto& leg=pose.legs[i]; const auto& arm=pose.arms[i];
      const auto hip=world(leg.root),knee=world(leg.joint),ankle=world(leg.end);
      const XMFLOAT4 trousers=outfit.trousers?XMFLOAT4{.24f,.25f,.23f,1}:XMFLOAT4{.42f,.37f,.27f,1};
      renderer.DrawSegment(hip,knee,.26f,.29f,trousers,S::Cloth);
      renderer.DrawMesh(M::Sphere,knee,{.19f,.19f,.20f},outfit.trousers?trousers:skin,outfit.trousers?S::Cloth:S::Skin,yaw);
      renderer.DrawSegment(knee,ankle,.22f,.24f,outfit.boots?leather:outfit.trousers?trousers:skin,outfit.boots || outfit.trousers?S::Cloth:S::Skin);
      const float bootYaw=yaw+pose.footYaw[i];
      const XMFLOAT3 foot{ankle.x+std::sin(bootYaw)*.035f,ankle.y-.035f,ankle.z+std::cos(bootYaw)*.035f};
      renderer.DrawMesh(M::Boot,foot,{.23f,.24f,.37f},outfit.boots?leather:skin,outfit.boots?S::Cloth:S::Skin,bootYaw);
      if (outfit.boots) {
        renderer.DrawMesh(M::Boot,{foot.x,foot.y-.105f,foot.z},{.24f,.035f,.38f},{.09f,.075f,.05f,1},S::Cloth,bootYaw);
        renderer.DrawSegment({knee.x,knee.y-.025f,knee.z},{knee.x,knee.y+.025f,knee.z},.245f,.26f,{.39f,.29f,.15f,1},S::Cloth);
      }
      const auto shoulder=world(arm.root),elbow=world(arm.joint),hand=world(arm.end);
      renderer.DrawSegment(shoulder,elbow,.25f,.27f,cloth,S::Cloth);
      renderer.DrawMesh(M::Sphere,elbow,{.15f,.15f,.15f},skin,S::Skin,yaw);
      renderer.DrawSegment(elbow,hand,.19f,.21f,outfit.armor?steel:skin,outfit.armor?S::Metal:S::Skin);
      renderer.DrawMesh(M::Sphere,hand,{.14f,.17f,.14f},skin,S::Skin,yaw);
      if (side>0 && outfit.weapon) {
        const auto along=[&](float distance) { return world(pose.WeaponPoint(distance)); };
        const auto forward=pose.UpperDirection({0,0,1});
        const XMFLOAT3 facing{std::sin(yaw)*forward.z,forward.y,std::cos(yaw)*forward.z};
        renderer.DrawMeshBetween(M::Blade,along(.10f),along(.96f),.12f,.07f,blade,S::Metal,facing);
        renderer.DrawMeshBetween(M::Limb,along(-.09f),along(.10f),.065f,.065f,leather,S::Cloth,facing);
        auto guard=along(.10f);
        renderer.DrawMeshBetween(M::Box,{guard.x-.13f*std::cos(yaw),guard.y,guard.z+.13f*std::sin(yaw)},
          {guard.x+.13f*std::cos(yaw),guard.y,guard.z-.13f*std::sin(yaw)},.045f,.055f,blade,S::Metal,facing);
        renderer.DrawMesh(M::Sphere,along(-.11f),{.08f,.08f,.08f},blade,S::Metal,yaw);
      }
      if (side<0 && outfit.shield) {
        const auto shieldPoint=[&](XMFLOAT3 local) {
          const auto offset=pose.UpperDirection(local);
          return point(arm.end.x+offset.x,arm.end.y+offset.y,arm.end.z+offset.z);
        };
        renderer.DrawMesh(M::Torso,shieldPoint({-.035f,-.04f,.12f}),{.54f,.75f,.15f},{.66f,.49f,.22f,1},S::Metal,yaw,pose.upperPitch);
        renderer.DrawMesh(M::Torso,shieldPoint({-.035f,-.04f,.205f}),
          {.45f,.64f,.045f},{.18f,.31f,.43f,1},S::Cloth,yaw,pose.upperPitch);
        renderer.DrawMesh(M::Sphere,shieldPoint({-.035f,0,.25f}),
          {.16f,.16f,.09f},steel,S::Metal,yaw,pose.upperPitch);
      }
    }
    if (outfit.armor) {
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.31f,1.46f,0,{.26f,.18f,.31f},steel,S::Metal);
        part(M::Torso,side*.31f,1.41f,0,{.27f,.03f,.32f},{.66f,.50f,.23f,1},S::Metal);
      }
    }
}

