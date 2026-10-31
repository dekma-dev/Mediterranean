#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "game/CharacterRig.h"
#include "game/HandPose.h"
#include "game/WeaponVisual.h"
#include "game/ClassClothing.h"
#include "game/ArmorPose.h"
#include "engine/Renderer.h"
#include <cmath>
#include <algorithm>
CharacterOutfit CharacterOutfit::From(const rpg::Inventory& inventory,rpg::ClassId id,rpg::Appearance look) {
 return {inventory.Worn(rpg::GearSlot::Chest)==rpg::Item::Ward,inventory.Worn(rpg::GearSlot::Weapon)==rpg::Item::Watchguard,
  inventory.Worn(rpg::GearSlot::Weapon)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Chest)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Legs)!=rpg::Item::None,inventory.Worn(rpg::GearSlot::Feet)!=rpg::Item::None,
  inventory.Worn(rpg::GearSlot::Head)==rpg::Item::Helm,inventory.Worn(rpg::GearSlot::Offhand)==rpg::Item::Shield,
  inventory.Worn(rpg::GearSlot::Shoulders)==rpg::Item::Shoulders,inventory.Worn(rpg::GearSlot::Hands)==rpg::Item::Gloves,
  inventory.Worn(rpg::GearSlot::Waist)==rpg::Item::Belt,inventory.Worn(rpg::GearSlot::Back)==rpg::Item::Cloak,id,look};
}
void DrawCharacter(Renderer& renderer,rpg::Vec2 p,float base,float yaw,DirectX::XMFLOAT4 color,float swing,float gait,CharacterOutfit outfit,animation::Sample dynamics,std::uint64_t actor) {
 using namespace DirectX; using M=visual::Mesh; using S=visual::Material;
    const auto form=rpg::WeaponFor(outfit.classId).form;
    const bool guard=outfit.classId==rpg::ClassId::Warrior;
    const bool oldStyle=oldstyle::Focused(outfit.classId);
    const bool bow=outfit.classId==rpg::ClassId::Scout && outfit.weapon;
    const auto classPaint=oldstyle::Paint(outfit.classId,outfit.appearance);
    if(oldStyle && !dynamics.authoredGait && swing==0 && dynamics.attackSeconds<0) {
      dynamics.authoredGait=true;dynamics.authoredBlend=1;
      dynamics.authoredEquipment=!outfit.weapon?playerclips::Equipment::None:bow?playerclips::Equipment::Bow:guard?playerclips::Equipment::Sword:playerclips::Equipment::Staff;
    }
    const auto pose=BuildCharacterPose(swing,gait,outfit.shield,dynamics,form);
    const auto& tone=rpg::skinTones[outfit.appearance.skin];
    const auto& hair=rpg::hairColors[outfit.appearance.hair];
    const XMFLOAT4 hairTint{hair.r,hair.g,hair.b,1};
    const XMFLOAT4 skin{tone.r,tone.g,tone.b,1},leather{.20f,.14f,.095f,1},steel{.52f,.59f,.61f,1};
    const XMFLOAT4 blade=outfit.rewardWeapon?XMFLOAT4{.76f,.67f,.42f,1}:steel;
    const auto point=[&](float x,float h,float z) -> XMFLOAT3 {
      return {p.x+std::cos(yaw)*x+std::sin(yaw)*z,base+h,p.z-std::sin(yaw)*x+std::cos(yaw)*z};
    };
    const auto world=[&](XMFLOAT3 local) { return point(local.x,local.y,local.z); };
    const auto upper=[&](XMFLOAT3 local) { return world(pose.UpperPoint(local)); };
    const auto part=[&](M mesh,float x,float h,float z,XMFLOAT3 scale,XMFLOAT4 tint,S material,unsigned maxDetail=2) {
      renderer.DrawMesh(mesh,upper({x,h,z}),scale,tint,material,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll,maxDetail);
    };
    // A smaller, slightly longer head proportion makes the guard less toy-like.
    // Apply the same neck-centred transform to skin, features, hair and helmet.
    const auto headFit=guard?XMMatrixTranslation(0,-1.61f,0)*XMMatrixScaling(.92f,.97f,.92f)*XMMatrixTranslation(0,1.61f,0):XMMatrixIdentity();
    const auto headTransform=headFit*XMMatrixTranslation(0,-.055f,0)*pose.HeadTransform()*XMMatrixRotationY(yaw)*XMMatrixTranslation(p.x,base,p.z);
    const auto actorTransform=XMMatrixRotationY(yaw)*XMMatrixTranslation(p.x,base,p.z);
    const auto head=[&](M mesh,float x,float h,float z,XMFLOAT3 scale,XMFLOAT4 tint,S material,float roll=0.f,unsigned maxDetail=3) {
      const unsigned detail=maxDetail==3?(mesh==M::Feature?1u:2u):maxDetail;
      renderer.DrawMeshTransformed(mesh,XMMatrixScaling(scale.x,scale.y,scale.z)*XMMatrixRotationZ(roll)*
        XMMatrixTranslation(x,h,z)*headTransform,tint,material,mesh==M::Feature?visual::ShadowMode::None:visual::ShadowMode::Cast,detail);
    };
    const auto body=renderer.DrawBody({p.x,base,p.z},yaw,BuildCharacterRig(pose,outfit,color),actor);
    const auto attachments=renderer.AttachToBody(body);
    const auto fittings=clothing::Build(outfit);
    for(unsigned n=0;n<fittings.count;++n) {
      const auto& fitting=fittings.parts[n];
      if(oldStyle && fitting.mesh==oldstyle::Mesh(outfit.classId,2)) {
        const unsigned arm=fitting.position.x>0?1u:0u;
        const auto torso=XMMatrixRotationX(pose.upperPitch)*XMMatrixRotationZ(pose.upperRoll)*XMMatrixRotationY(pose.upperYaw);
        const auto direction=XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&pose.arms[arm].joint)-XMLoadFloat3(&pose.arms[arm].root),XMMatrixTranspose(torso)));
        const float side=arm?1.f:-1.f;
        const float roll=side*.55f*std::clamp(std::atan2(std::max(0.f,side*XMVectorGetX(direction)),-XMVectorGetY(direction))-.18f,0.f,1.4f);
        const float pitch=-.25f*XMVectorGetZ(direction);
        renderer.DrawMeshTransformed(fitting.mesh,XMMatrixRotationX(pitch)*XMMatrixRotationZ(roll)*
          XMMatrixTranslation(fitting.position.x,fitting.position.y-.95f,0)*torso*
          XMMatrixTranslation(pose.bodyShift,.95f+pose.bodyLift,0)*actorTransform,fitting.color,fitting.material);
        continue;
      }
      if(fitting.mesh==M::GuardShoulderLeft || fitting.mesh==M::GuardShoulderRight) {
        const unsigned arm=fitting.mesh==M::GuardShoulderLeft?0u:1u;
        renderer.DrawMeshTransformed(fitting.mesh,GuardianShoulderTransform(pose,arm)*actorTransform,
          fitting.color,fitting.material,visual::ShadowMode::Cast,2);
        continue;
      }
      part(fitting.mesh,fitting.position.x,fitting.position.y,fitting.position.z,
        fitting.scale,fitting.color,fitting.material,2);
    }
    if (outfit.belt && !oldStyle) {
      part(M::Limb,0,.95f,0,{.55f,.085f,.37f},leather,S::Leather);
      part(M::Box,0,.95f,.19f,{.10f,.075f,.025f},{.67f,.49f,.22f,1},S::Metal);
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.245f,.86f,-.10f,{.16f,.22f,.16f},leather,S::Leather);
        part(M::Box,side*.245f,.91f,-.005f,{.15f,.065f,.03f},{.36f,.24f,.13f,1},S::Cloth);
        part(M::Sphere,side*.245f,.90f,.015f,{.025f,.025f,.012f},{.67f,.49f,.22f,1},S::Metal,0);
      }
    }
    if (outfit.cloak) {
      renderer.DrawCloak(upper({0,1.49f,-.19f}),yaw+pose.upperYaw,pose.upperPitch,pose.upperRoll,
        dynamics.clothSide,dynamics.clothBack,guard,oldStyle?(guard?XMFLOAT4{.62f,.52f,.45f,1}:outfit.classId==rpg::ClassId::Scout?XMFLOAT4{.48f,.68f,.46f,1}:XMFLOAT4{.67f,.48f,.67f,1}):XMFLOAT4{1,1,1,1});
      if(!oldStyle)for (float side : {-1.f,1.f})
        part(M::Torso,side*.13f,1.49f,.015f,{.12f,.09f,.39f},{.12f,.24f,.36f,1},S::Cloth);
      if(!oldStyle)part(M::Sphere,0,1.48f,.195f,{.09f,.065f,.03f},{.72f,.54f,.24f,1},S::Metal);
    }
    if (outfit.tunic && !guard && !oldStyle) {
      const XMFLOAT3 seam[]={{0,1.02f,.15f},{0,1.064f,.16f},{0,1.198f,.183f},{0,1.371f,.175f},{0,1.448f,.145f}};
      for (int i=0;i<4;++i) renderer.DrawMeshBetween(M::Box,
        upper(seam[i]),upper(seam[i+1]),
        .022f,.009f,{.63f,.50f,.25f,1},S::Cloth,{std::sin(yaw),0,std::cos(yaw)});
      part(M::Torso,0,1.51f,0,{.25f,.065f,.24f},leather,S::Leather);
    }
    if (outfit.armor && !guard && !oldStyle) {
      // Overlapping abdominal lames and fasteners give the cuirass real relief.
      for(int plate=0;plate<3;++plate) {
        const float h=1.04f+plate*.065f;
        part(M::Torso,0,h,.125f,{.43f+plate*.035f,.11f,.095f},steel,S::Metal,1);
        for(float side : {-1.f,1.f})
          part(M::Sphere,side*(.15f+plate*.014f),h+.018f,.17f,{.018f,.018f,.012f},blade,S::Metal,0);
      }
      part(M::Torso,0,1.29f,.18f,{.17f,.22f,.035f},{.68f,.53f,.26f,1},S::Metal);
      part(M::Torso,0,1.29f,.20f,{.09f,.14f,.02f},{.21f,.37f,.43f,1},S::Metal);
    }
    if(guard && outfit.tunic && !oldStyle) {
      part(M::GuardFauld,0,1.03f,0,{1,1,1},{.8f,.86f,.93f,1},S::Metal);
      part(M::Torso,0,1.51f,-.005f,{.25f,.085f,.25f},{.24f,.22f,.17f,1},S::Leather);
    }
    const auto face=dynamics.face;
    const float pain=facial::Unit(face.pain),focus=facial::Unit(face.focus),smile=facial::Unit(face.smile)*(1-pain);
    const float closed=std::max(facial::Unit(face.blink),pain*.68f+focus*.16f);
    renderer.DrawHead(XMMatrixScaling(.265f,.42f,.26f)*XMMatrixTranslation(0,1.71f,.012f)*headTransform,
      skin,closed,smile-pain);
    if (!outfit.helm) head(M::Hair,0,1.71f,.012f,{.265f,.42f,.26f},hairTint,S::Hair);
    // Eye centres come from the study's eyeballs, fitted by the same export
    // transform as the face. The sculpt already includes lips, nose and ears.
    const float eyeX=animation::Signed(face.eyeYaw/.22f)*.003f,eyeY=-animation::Signed(face.eyePitch/.105f)*.002f;
    for(float side : {-1.f,1.f}) {
      const float x=side*.04205f,y=1.78241f,z=.09406f;
      head(M::Sphere,x,y,z,{.0307f,.02775f,.0266f},{.55f,.56f,.51f,1},S::Plain,0,1);
      // Spherical eye surface and restrained iris/pupil stay behind the lids.
      head(M::Feature,x+eyeX,y+eyeY,z+.0129f,{.0105f,.011f,.0017f},{.19f,.27f,.25f,1},S::Plain);
      head(M::Feature,x+eyeX,y+eyeY,z+.014f,{.0043f,.0055f,.0012f},{.025f,.028f,.025f,1},S::Plain);
      const float browRoll=-side*(.05f+focus*.15f-pain*.12f);
      head(M::Feature,x,1.805f-focus*.002f+pain*.002f,.100f,{.041f,.0045f,.006f},hairTint,S::Hair,browRoll);
    }
    if (outfit.helm) {
      if(oldStyle)renderer.DrawMeshTransformed(oldstyle::Mesh(outfit.classId,3),headTransform,classPaint,S::Count);
      else {
      head(M::Helm,0,1.86f,.012f,{.31f,.21f,.30f},steel,S::Metal);
      head(M::Box,0,1.817f,.15f,{.26f,.035f,.04f},{.72f,.54f,.24f,1},S::Metal);
      head(M::Blade,0,1.99f,0,{.04f,.19f,.16f},{.26f,.38f,.47f,1},S::Cloth);
      for (float side : {-1.f,1.f}) head(M::Torso,side*.13f,1.73f,.075f,{.055f,.18f,.115f},steel,S::Metal);
      }
    }
    // Fixed-length joints prevent the old stretched forearms during a strike.
    for (int i=0;i<2;++i) {
      const float side=i==0?-1.f:1.f;
      const auto& leg=pose.legs[i]; const auto& arm=pose.arms[i];
      const auto knee=world(leg.joint),ankle=world(leg.end);
      if(outfit.classId==rpg::ClassId::Scout && outfit.tunic) {
        auto hem=knee;hem.y+=.10f;
        for(float facing:{-1.f,1.f})renderer.DrawMeshBetween(M::OldScoutSkirt,hem,world(leg.root),.26f,.21f,classPaint,S::Count,
          {facing*std::sin(yaw),0,facing*std::cos(yaw)});
      }
      const bool coat=outfit.classId==rpg::ClassId::Priest || outfit.classId==rpg::ClassId::Druid ||
        outfit.classId==rpg::ClassId::Mage || outfit.classId==rpg::ClassId::Necromancer ||
        outfit.classId==rpg::ClassId::Psionic || outfit.classId==rpg::ClassId::Bard;
      if(outfit.tunic && coat) {
        auto cloth=color;
        if(outfit.appearance.dye) {const auto& dye=rpg::tunicDyes[outfit.appearance.dye];cloth={dye.r,dye.g,dye.b,1};}
        else {cloth.x*=.65f;cloth.y*=.65f;cloth.z*=.65f;}
        auto hem=oldStyle?ankle:knee;hem.y+=oldStyle?.12f:-.04f;
        const auto waist=world(leg.root);
        // Split panels follow the solved thighs: walking/jumping retains a
        // leg opening instead of stretching one rigid skirt across both legs.
        for(float side:{-1.f,1.f}) {
          const XMFLOAT3 facing{side*std::sin(yaw),0,side*std::cos(yaw)};
          if(oldStyle) {
            renderer.DrawCoatPanel(M::OldMageSkirt,hem,waist,knee,.36f,.24f,facing,classPaint);
          } else renderer.DrawMeshBetween(M::CoatPanel,hem,waist,.33f,.23f,cloth,S::Cloth,facing);
        }
      }
      const float bootYaw=yaw+pose.footYaw[i];
      const XMFLOAT3 foot{ankle.x+std::sin(bootYaw)*.035f,ankle.y-.035f,ankle.z+std::cos(bootYaw)*.035f};
      renderer.DrawMesh(M::Boot,foot,{.21f,.23f,.34f},outfit.boots?leather:skin,outfit.boots?S::Leather:S::Skin,bootYaw);
      if (outfit.boots) {
        renderer.DrawMesh(M::Boot,{foot.x,foot.y-.105f,foot.z},{.22f,.035f,.35f},{.09f,.075f,.05f,1},S::Cloth,bootYaw);
        if(!oldStyle)renderer.DrawSegment({knee.x,knee.y-.025f,knee.z},{knee.x,knee.y+.025f,knee.z},.245f,.26f,{.39f,.29f,.15f,1},S::Cloth);
      }
      if(outfit.boots) {
        const float sine=std::sin(bootYaw),cosine=std::cos(bootYaw);
        // Two narrow welts and a brass clasp break up the plain boot shaft.
        for(int strap=0;strap<2;++strap) {
          const float t=.25f+strap*.22f;
          XMFLOAT3 band{ankle.x+(knee.x-ankle.x)*t,ankle.y+(knee.y-ankle.y)*t,ankle.z+(knee.z-ankle.z)*t};
          renderer.DrawMesh(M::Limb,band,{.155f,.018f,.165f},{.16f,.105f,.065f,1},S::Cloth,bootYaw);
          band.x+=sine*.085f;band.z+=cosine*.085f;
          renderer.DrawMesh(M::Box,band,{.035f,.032f,.016f},{.50f,.37f,.18f,1},S::Metal,bootYaw);
        }
      }
      const auto elbow=world(arm.joint),hand=world(arm.end);
      if(guard) {
        const XMFLOAT3 forward{std::sin(yaw),0,std::cos(yaw)};
        const auto offset=[&](XMFLOAT3 a,float depth) {return XMFLOAT3{a.x+forward.x*depth,a.y,a.z+forward.z*depth};};
        const auto along=[](XMFLOAT3 a,XMFLOAT3 b,float t) {return XMFLOAT3{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};};
        if(outfit.trousers && outfit.tunic) {
          const auto hip=world(leg.root);
          // Split weathered tabard hangs below the articulated waist plates.
          // Each half follows its thigh, retaining the opening during a stride.
          auto cloth=XMFLOAT4{.31f,.105f,.065f,1};
          if(outfit.appearance.dye) {const auto& dye=rpg::tunicDyes[outfit.appearance.dye];cloth={dye.r,dye.g,dye.b,1};}
          renderer.DrawMeshBetween(oldStyle?M::OldWarriorSkirt:M::CoatPanel,offset(along(hip,knee,.95f),-.01f),offset(hip,-.01f),
            .22f,.17f,oldStyle?classPaint:cloth,oldStyle?S::Count:S::Cloth,forward);
          renderer.DrawMeshBetween(M::GuardPlate,offset(along(hip,knee,.54f),.13f),offset(along(hip,knee,.04f),.14f),
            .235f,.18f,{.43f,.435f,.405f,1},S::WornMetal,forward);
        }
        if(outfit.boots) {
          renderer.DrawMeshBetween(M::GuardPlate,offset(along(ankle,knee,.12f),.07f),offset(along(ankle,knee,.87f),.08f),
            .19f,.19f,{.43f,.435f,.405f,1},S::WornMetal,forward);
          renderer.DrawMesh(M::GuardPlate,offset(knee,.10f),{.22f,.19f,.22f},{.43f,.435f,.405f,1},S::WornMetal,yaw);
        }
        if(outfit.gloves)
          renderer.DrawMeshBetween(M::GuardPlate,offset(along(hand,elbow,.12f),.055f),offset(along(hand,elbow,.80f),.055f),
            .17f,.17f,{.43f,.435f,.405f,1},S::WornMetal,forward);
      }
      const auto handColor=outfit.gloves?leather:skin;
      const auto handMaterial=outfit.gloves?S::Leather:S::Skin;
      const auto fingers=BuildHandPose(pose,side>0?1u:0u,outfit.weapon,outfit.shield&&!bow,pose.handCast);
      const auto handFrame=XMLoadFloat4x4(&fingers.frame)*actorTransform;
      const auto handPoint=[&](XMFLOAT3 local) { return world(fingers.Point(local)); };
      if (outfit.gloves) {
        const XMFLOAT3 cuff{hand.x+(elbow.x-hand.x)*.32f,hand.y+(elbow.y-hand.y)*.32f,hand.z+(elbow.z-hand.z)*.32f};
        renderer.DrawSegment(cuff,hand,oldStyle?.115f:.205f,oldStyle?.125f:.22f,leather,oldStyle?S::WornLeather:S::Leather);
      }
      // Wrist bridge overlaps both the solved forearm and shaped palm.
      const XMFLOAT3 wristStart{hand.x+(elbow.x-hand.x)*.12f,hand.y+(elbow.y-hand.y)*.12f,hand.z+(elbow.z-hand.z)*.12f};
      const auto lateral=XMVector3TransformNormal(XMVectorSet(1,0,0,0),handFrame);
      XMFLOAT3 facing; XMStoreFloat3(&facing,lateral);
      renderer.DrawMeshBetween(M::Finger,wristStart,handPoint({0,-.03f,-.043f}),.072f,.067f,handColor,handMaterial,facing);
      renderer.DrawHand(handFrame,BuildHandRig(fingers),fingers.mirrored,handColor,handMaterial);
      if(outfit.armor && !guard && !oldStyle) {
        renderer.DrawMesh(M::Torso,{knee.x+std::sin(yaw)*.105f,knee.y,knee.z+std::cos(yaw)*.105f},
          {.22f,.24f,.075f},steel,S::Metal,yaw);
      }
      if (side>0 && outfit.weapon) {
        const auto weaponFrame=WeaponFrame(pose)*actorTransform;
        const auto weaponPart=[&](M mesh,float offset,XMFLOAT3 scale,XMFLOAT4 tint,S material) {
          renderer.DrawMeshTransformed(mesh,XMMatrixScaling(scale.x,scale.y,scale.z)*XMMatrixTranslation(0,offset,0)*weaponFrame,tint,material);
        };
        if(bow)renderer.DrawBow(oldstyle::Mesh(outfit.classId,outfit.rewardWeapon?6:5),weaponFrame,pose.bowDraw,pose.bowArrow);
        else if(oldStyle)renderer.DrawMeshTransformed(oldstyle::Mesh(outfit.classId,outfit.rewardWeapon?6:5),weaponFrame,{1,1,1,1},S::Count);
        else if(form==rpg::WeaponForm::Blade) {
          weaponPart(M::Blade,.53f,{.12f,.86f,.07f},blade,S::Metal);
          weaponPart(M::Limb,.005f,{.065f,.19f,.065f},leather,S::Leather);
          if(guard) {
            for(float wing:{-1.f,1.f}) renderer.DrawMeshTransformed(M::Blade,
              XMMatrixScaling(.055f,.17f,.05f)*XMMatrixRotationZ(wing*1.35f)*XMMatrixTranslation(wing*.095f,.10f,0)*weaponFrame,
              {.69f,.53f,.29f,1},S::Metal);
            for(int ring=0;ring<6;++ring) weaponPart(M::Limb,-.075f+ring*.027f,{.069f,.009f,.069f},{.36f,.27f,.16f,1},S::Leather);
            weaponPart(M::Crystal,-.12f,{.085f,.12f,.065f},{.67f,.49f,.25f,1},S::Metal);
            weaponPart(M::Torso,.17f,{.13f,.12f,.07f},{.65f,.49f,.26f,1},S::Metal);
          } else {
            weaponPart(M::Box,.10f,{.26f,.045f,.055f},blade,S::Metal);
            weaponPart(M::Sphere,-.11f,{.08f,.08f,.08f},blade,S::Metal);
          }
        } else renderer.DrawMeshTransformed(WeaponMesh(form),weaponFrame,
          outfit.rewardWeapon?XMFLOAT4{1,.92f,.70f,1}:XMFLOAT4{.88f,.91f,.94f,1},S::Metal);
      }
      if (side<0 && outfit.shield && !bow) {
        const auto shieldPoint=[&](XMFLOAT3 local) {
          const auto offset=pose.UpperDirection(local);
          return point(arm.end.x+offset.x,arm.end.y+offset.y,arm.end.z+offset.z);
        };
        if(oldStyle)renderer.DrawMesh(oldstyle::Mesh(outfit.classId,4),shieldPoint({-.035f,-.04f,.14f}),{1,1,1},{1,1,1,1},S::Count,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
        else {
        renderer.DrawMesh(M::Torso,shieldPoint({-.035f,-.04f,.12f}),{.54f,.75f,.15f},{.66f,.49f,.22f,1},S::Metal,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
        renderer.DrawMesh(M::Torso,shieldPoint({-.035f,-.04f,.205f}),
          {.45f,.64f,.045f},{.18f,.31f,.43f,1},S::Cloth,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
        renderer.DrawMesh(M::Sphere,shieldPoint({-.035f,0,.25f}),
          {.16f,.16f,.09f},steel,S::Metal,yaw+pose.upperYaw,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
        }
      }
    }
    if(bow && outfit.shield)renderer.DrawMesh(oldstyle::Mesh(outfit.classId,4),upper({-.16f,1.21f,-.31f}),{1,1,1},{1,1,1,1},S::Count,
      yaw+pose.upperYaw+DirectX::XM_PI,pose.upperPitch,visual::ShadowMode::Cast,pose.upperRoll);
    if (outfit.armor && !outfit.shoulders && !guard && !oldStyle) {
      const auto guardColor=outfit.shoulders?leather:steel;
      const auto guardMaterial=outfit.shoulders?S::Leather:S::Metal;
      for (float side : {-1.f,1.f}) {
        part(M::Pauldron,side*.31f,1.46f,0,{.26f,.18f,.31f},guardColor,guardMaterial);
        part(M::Torso,side*.31f,1.41f,0,{.27f,.03f,.32f},{.66f,.50f,.23f,1},S::Metal);
        for(int layer=0;layer<3;++layer) {
          part(M::Torso,side*(.32f+layer*.018f),1.43f-layer*.045f,0,
            {.27f,.07f,.32f-layer*.015f},outfit.shoulders?leather:layer==2?blade:steel,guardMaterial);
          part(M::Sphere,side*(.32f+layer*.018f),1.44f-layer*.045f,.13f,
            {.024f,.024f,.018f},blade,S::Metal,0);
        }
      }
    }
}

