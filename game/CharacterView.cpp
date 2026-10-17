#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include <cmath>
CharacterOutfit CharacterOutfit::From(const rpg::Inventory& inventory) {
 return {inventory.Worn(rpg::GearSlot::Chest)==rpg::Item::Ward,inventory.Worn(rpg::GearSlot::Weapon)==rpg::Item::Watchguard,
  inventory.Worn(rpg::GearSlot::Weapon)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Chest)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Legs)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Feet)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Head)==rpg::Item::Helm,inventory.Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield};
}
void DrawCharacter(Renderer& renderer,rpg::Vec2 p,float base,float yaw,DirectX::XMFLOAT4 color,float swing,float gait,CharacterOutfit outfit) {
 using namespace DirectX; using M=visual::Mesh; using S=visual::Material;
    const XMFLOAT4 skin{.72f,.51f,.35f,1},leather{.20f,.14f,.095f,1},steel{.52f,.59f,.61f,1};
    const XMFLOAT4 blade=outfit.rewardWeapon?XMFLOAT4{.76f,.67f,.42f,1}:steel;
    const XMFLOAT4 cloth=outfit.tunic?color:XMFLOAT4{.61f,.58f,.46f,1};
    const auto point=[&](float x,float h,float z) -> XMFLOAT3 {
      return {p.x+std::cos(yaw)*x+std::sin(yaw)*z,base+h,p.z-std::sin(yaw)*x+std::cos(yaw)*z};
    };
    const auto part=[&](M mesh,float x,float h,float z,XMFLOAT3 scale,XMFLOAT4 tint,S material) {
      renderer.DrawMesh(mesh,point(x,h,z),scale,tint,material,yaw);
    };
    part(M::Torso,0,1.22f,0,{.68f,.67f,.39f},outfit.armor?steel:cloth,outfit.armor?S::Metal:S::Cloth);
    part(M::Torso,0,.93f,0,{.61f,.11f,.42f},leather,S::Cloth);
    part(M::Box,0,.93f,.21f,{.12f,.09f,.04f},{.67f,.49f,.22f,1},S::Metal);
    if (outfit.tunic) {
      part(M::Torso,0,.80f,.04f,{.59f,.27f,.38f},cloth,S::Cloth);
      part(M::Box,0,1.22f,.205f,{.035f,.48f,.018f},{.63f,.50f,.25f,1},S::Cloth);
    }
    if (outfit.armor) {
      part(M::Torso,0,1.29f,.21f,{.19f,.24f,.04f},{.68f,.53f,.26f,1},S::Metal);
      part(M::Torso,0,1.29f,.235f,{.10f,.15f,.02f},{.21f,.37f,.43f,1},S::Metal);
    }
    part(M::Limb,0,1.55f,0,{.2f,.2f,.2f},skin,S::Skin);
    part(M::Sphere,0,1.76f,.02f,{.30f,.37f,.30f},skin,S::Skin);
    part(M::Sphere,0,1.86f,-.025f,{.31f,.20f,.31f},{.19f,.13f,.08f,1},S::Cloth);
    part(M::Sphere,0,1.75f,.155f,{.055f,.08f,.07f},skin,S::Skin);
    if (outfit.helm) {
      part(M::Sphere,0,1.88f,.015f,{.37f,.25f,.36f},steel,S::Metal);
      part(M::Box,0,1.83f,.18f,{.31f,.045f,.045f},{.72f,.54f,.24f,1},S::Metal);
      part(M::Blade,0,2.04f,0,{.045f,.23f,.19f},{.26f,.38f,.47f,1},S::Cloth);
      for (float side : {-1.f,1.f}) part(M::Torso,side*.157f,1.72f,.09f,{.065f,.22f,.14f},steel,S::Metal);
    }
    // Small facial landmarks help the front view without texture assets.
    for (float side : {-1.f,1.f}) {
      part(M::Sphere,side*.071f,1.79f,.153f,{.026f,.019f,.018f},{.11f,.10f,.08f,1},S::Plain);
      const float stride=side*gait*.23f;
      const auto hip=point(side*.19f,.9f,0),knee=point(side*.21f,.52f,stride*.6f+.07f);
      const auto ankle=point(side*.21f,.18f,stride);
      renderer.DrawSegment(hip,knee,.27f,.28f,outfit.trousers?XMFLOAT4{.24f,.25f,.23f,1}:XMFLOAT4{.42f,.37f,.27f,1},S::Cloth);
      renderer.DrawSegment(knee,ankle,.21f,.23f,outfit.boots?leather:outfit.trousers?XMFLOAT4{.24f,.25f,.23f,1}:skin,outfit.boots || outfit.trousers?S::Cloth:S::Skin);
      part(M::Sphere,side*.21f,.12f,stride+.07f,{.23f,.20f,.37f},outfit.boots?leather:skin,outfit.boots?S::Cloth:S::Skin);
      if (outfit.boots) part(M::Limb,side*.21f,.45f,stride*.5f,{.24f,.055f,.25f},{.39f,.29f,.15f,1},S::Cloth);
      const float lift=side>0?swing:0;
      const auto shoulder=point(side*.31f,1.43f,0);
      const auto elbow=point(side*.40f,1.16f+lift*.3f,-stride*.4f+.03f);
      const auto hand=point(side*.42f,1.0f+lift,-stride*.8f+.12f);
      renderer.DrawSegment(shoulder,elbow,.23f,.25f,cloth,S::Cloth);
      renderer.DrawSegment(elbow,hand,.17f,.19f,skin,S::Skin);
      renderer.DrawMesh(M::Sphere,hand,{.14f,.17f,.14f},skin,S::Skin,yaw);
      if (side>0 && outfit.weapon) {
        renderer.DrawMesh(M::Blade,{hand.x,hand.y+.49f,hand.z},{.19f,.88f,.16f},blade,S::Metal,yaw);
        renderer.DrawMesh(M::Box,{hand.x,hand.y+.08f,hand.z},{.32f,.055f,.09f},blade,S::Metal,yaw);
      }
      if (side<0 && outfit.shield) {
        const auto shield=point(-.47f,1.05f,.24f-stride*.5f);
        renderer.DrawMesh(M::Torso,shield,{.59f,.78f,.15f},{.66f,.49f,.22f,1},S::Metal,yaw);
        renderer.DrawMesh(M::Torso,{shield.x+std::sin(yaw)*.085f,shield.y,shield.z+std::cos(yaw)*.085f},
          {.50f,.67f,.045f},{.18f,.31f,.43f,1},S::Cloth,yaw);
        renderer.DrawMesh(M::Sphere,{shield.x+std::sin(yaw)*.13f,shield.y+.04f,shield.z+std::cos(yaw)*.13f},
          {.16f,.16f,.09f},steel,S::Metal,yaw);
      }
    }
    if (outfit.armor) {
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.33f,1.46f,0,{.28f,.20f,.34f},steel,S::Metal);
        part(M::Torso,side*.33f,1.40f,0,{.29f,.035f,.35f},{.66f,.50f,.23f,1},S::Metal);
      }
    }
}

