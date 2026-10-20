#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "engine/Renderer.h"
#include <cmath>
#include <algorithm>
CharacterOutfit CharacterOutfit::From(const rpg::Inventory& inventory) {
 return {inventory.Worn(rpg::GearSlot::Chest)==rpg::Item::Ward,inventory.Worn(rpg::GearSlot::Weapon)==rpg::Item::Watchguard,
  inventory.Worn(rpg::GearSlot::Weapon)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Chest)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Legs)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Feet)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Head)==rpg::Item::Helm,inventory.Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield,
  inventory.Worn(rpg::GearSlot::Shoulders)==rpg::Item::Shoulders,inventory.Worn(rpg::GearSlot::Hands)==rpg::Item::Gloves,
  inventory.Worn(rpg::GearSlot::Waist)==rpg::Item::Belt,inventory.Worn(rpg::GearSlot::Back)==rpg::Item::Cloak};
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
      renderer.DrawMesh(mesh,upper({x,h,z}),scale,tint,material,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
    };
    part(M::Torso,0,1.23f,0,{.61f,.64f,.36f},outfit.armor?steel:cloth,outfit.armor?S::Metal:S::Cloth);
    if (outfit.belt) {
      part(M::Limb,0,.95f,0,{.55f,.085f,.37f},leather,S::Cloth);
      part(M::Box,0,.95f,.19f,{.10f,.075f,.025f},{.67f,.49f,.22f,1},S::Metal);
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.245f,.86f,-.10f,{.16f,.22f,.16f},leather,S::Cloth);
        part(M::Box,side*.245f,.91f,-.005f,{.15f,.065f,.03f},{.36f,.24f,.13f,1},S::Cloth);
        part(M::Sphere,side*.245f,.90f,.015f,{.025f,.025f,.012f},{.67f,.49f,.22f,1},S::Metal);
      }
    }
    if (outfit.cloak) {
      renderer.DrawCloak(upper({0,1.49f,-.19f}),yaw+pose.upperYaw,pose.upperPitch,pose.upperRoll,
        dynamics.clothSide,dynamics.clothBack);
      for (float side : {-1.f,1.f})
        part(M::Torso,side*.13f,1.49f,.015f,{.12f,.09f,.39f},{.12f,.24f,.36f,1},S::Cloth);
      part(M::Sphere,0,1.48f,.195f,{.09f,.065f,.03f},{.72f,.54f,.24f,1},S::Metal);
    }
    if (outfit.tunic) {
      part(M::Torso,0,.83f,.015f,{.54f,.27f,.36f},cloth,S::Cloth);
      const XMFLOAT3 seam[]={{0,1.02f,.139f},{0,1.064f,.135f},{0,1.198f,.149f},{0,1.371f,.171f},{0,1.448f,.149f}};
      for (int i=0;i<4;++i) renderer.DrawMeshBetween(M::Box,
        upper(seam[i]),upper(seam[i+1]),
        .022f,.009f,{.63f,.50f,.25f,1},S::Cloth,{std::sin(yaw),0,std::cos(yaw)});
      part(M::Torso,0,1.51f,0,{.25f,.065f,.24f},leather,S::Cloth);
    }
    if (outfit.armor) {
      // Overlapping abdominal lames and fasteners give the cuirass real relief.
      for(int plate=0;plate<3;++plate) {
        const float h=1.04f+plate*.065f;
        part(M::Torso,0,h,.125f,{.43f+plate*.035f,.11f,.095f},steel,S::Metal);
        for(float side : {-1.f,1.f})
          part(M::Sphere,side*(.15f+plate*.014f),h+.018f,.17f,{.018f,.018f,.012f},blade,S::Metal);
      }
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
      const auto handColor=outfit.gloves?leather:skin;
      const auto handMaterial=outfit.gloves?S::Cloth:S::Skin;
      if (outfit.gloves) {
        const XMFLOAT3 cuff{hand.x+(elbow.x-hand.x)*.32f,hand.y+(elbow.y-hand.y)*.32f,hand.z+(elbow.z-hand.z)*.32f};
        renderer.DrawSegment(cuff,hand,.205f,.22f,leather,S::Cloth);
      }
      renderer.DrawMesh(M::Sphere,hand,{.14f,.17f,.14f},handColor,handMaterial,yaw);
      // A palm, curled finger ridges and thumb replace the featureless mitten.
      renderer.DrawMesh(M::Sphere,{hand.x+side*.052f*std::cos(yaw),hand.y+.015f,hand.z-side*.052f*std::sin(yaw)},
        {.052f,.085f,.055f},handColor,handMaterial,yaw);
      for(int finger=0;finger<3;++finger)
        renderer.DrawMesh(M::Limb,{hand.x+std::sin(yaw)*.054f,hand.y-.045f+finger*.027f,hand.z+std::cos(yaw)*.054f},
          {.10f,.021f,.032f},outfit.gloves?XMFLOAT4{.30f,.22f,.14f,1}:XMFLOAT4{.60f,.40f,.28f,1},handMaterial,yaw);
      if(outfit.armor) {
        renderer.DrawMesh(M::Torso,{knee.x+std::sin(yaw)*.105f,knee.y,knee.z+std::cos(yaw)*.105f},
          {.22f,.24f,.075f},steel,S::Metal,yaw);
      }
      if (side>0 && outfit.weapon) {
        const auto along=[&](float distance) { return world(pose.WeaponPoint(distance)); };
        const auto forward=pose.UpperDirection({0,0,1});
        const XMFLOAT3 facing{std::cos(yaw)*forward.x+std::sin(yaw)*forward.z,forward.y,
          -std::sin(yaw)*forward.x+std::cos(yaw)*forward.z};
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
        renderer.DrawMesh(M::Torso,shieldPoint({-.035f,-.04f,.12f}),{.54f,.75f,.15f},{.66f,.49f,.22f,1},S::Metal,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
        renderer.DrawMesh(M::Torso,shieldPoint({-.035f,-.04f,.205f}),
          {.45f,.64f,.045f},{.18f,.31f,.43f,1},S::Cloth,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
        renderer.DrawMesh(M::Sphere,shieldPoint({-.035f,0,.25f}),
          {.16f,.16f,.09f},steel,S::Metal,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
      }
    }
    if (outfit.armor || outfit.shoulders) {
      const auto guardColor=outfit.shoulders?leather:steel;
      const auto guardMaterial=outfit.shoulders?S::Cloth:S::Metal;
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.31f,1.46f,0,{.26f,.18f,.31f},guardColor,guardMaterial);
        part(M::Torso,side*.31f,1.41f,0,{.27f,.03f,.32f},{.66f,.50f,.23f,1},S::Metal);
        for(int layer=0;layer<3;++layer) {
          part(M::Torso,side*(.32f+layer*.018f),1.43f-layer*.045f,0,
            {.27f,.07f,.32f-layer*.015f},outfit.shoulders?leather:layer==2?blade:steel,guardMaterial);
          part(M::Sphere,side*(.32f+layer*.018f),1.44f-layer*.045f,.13f,
            {.024f,.024f,.018f},blade,S::Metal);
        }
      }
    }
}

