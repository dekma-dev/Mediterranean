#include "game/CharacterView.h"
#include "game/CharacterPose.h"
#include "game/CharacterRig.h"
#include "game/HandPose.h"
#include "game/WeaponVisual.h"
#include "game/ClassClothing.h"
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
    const auto pose=BuildCharacterPose(swing,gait,outfit.shield,dynamics,form);
    const auto& tone=rpg::skinTones[outfit.appearance.skin];
    const auto& hair=rpg::hairColors[outfit.appearance.hair];
    const XMFLOAT4 hairTint{hair.r,hair.g,hair.b,1};
    const auto shade=[&](float r,float g,float b) {return XMFLOAT4{tone.r*r/.72f,tone.g*g/.51f,tone.b*b/.35f,1};};
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
    const auto headTransform=pose.HeadTransform()*XMMatrixRotationY(yaw)*XMMatrixTranslation(p.x,base,p.z);
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
      part(fitting.mesh,fitting.position.x,fitting.position.y,fitting.position.z,
        fitting.scale,fitting.color,fitting.material,2);
    }
    if (outfit.belt) {
      part(M::Limb,0,.95f,0,{.55f,.085f,.37f},leather,S::Cloth);
      part(M::Box,0,.95f,.19f,{.10f,.075f,.025f},{.67f,.49f,.22f,1},S::Metal);
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.245f,.86f,-.10f,{.16f,.22f,.16f},leather,S::Cloth);
        part(M::Box,side*.245f,.91f,-.005f,{.15f,.065f,.03f},{.36f,.24f,.13f,1},S::Cloth);
        part(M::Sphere,side*.245f,.90f,.015f,{.025f,.025f,.012f},{.67f,.49f,.22f,1},S::Metal,0);
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
      const XMFLOAT3 seam[]={{0,1.02f,.15f},{0,1.064f,.16f},{0,1.198f,.183f},{0,1.371f,.175f},{0,1.448f,.145f}};
      for (int i=0;i<4;++i) renderer.DrawMeshBetween(M::Box,
        upper(seam[i]),upper(seam[i+1]),
        .022f,.009f,{.63f,.50f,.25f,1},S::Cloth,{std::sin(yaw),0,std::cos(yaw)});
      part(M::Torso,0,1.51f,0,{.25f,.065f,.24f},leather,S::Cloth);
    }
    if (outfit.armor) {
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
    head(M::Head,0,1.76f,.012f,{.265f,.31f,.26f},skin,S::Skin);
    if (!outfit.helm) head(M::Hair,0,1.76f,.012f,{.267f,.31f,.262f},hairTint,S::Cloth);
    const auto face=dynamics.face;
    const float pain=facial::Unit(face.pain),focus=facial::Unit(face.focus),smile=facial::Unit(face.smile)*(1-pain);
    const float closed=std::max(facial::Unit(face.blink),pain*.68f+focus*.16f);
    const float eyeHeight=.011f*(1-closed);
    const float eyeX=animation::Signed(face.eyeYaw/.22f)*.0038f,eyeY=-animation::Signed(face.eyePitch/.105f)*.0022f;
    // Tapered lip volumes with an inset seam, instead of a rectangular mouth.
    const float mouthY=1.691f-pain*.002f,mouthWidth=.060f+smile*.008f+pain*.004f;
    head(M::Feature,0,mouthY,.106f,{mouthWidth,.003f+pain*.003f,.008f},shade(.23f,.12f,.10f),S::Skin);
    for(float side : {-1.f,1.f}) {
      head(M::Feature,side*.014f,mouthY+.0035f,.108f,{.031f,.005f,.010f},shade(.58f,.34f,.28f),S::Skin,side*(smile*.10f-pain*.10f));
      head(M::Feature,side*.013f,mouthY-.003f,.108f,{.030f,.006f,.010f},shade(.63f,.38f,.31f),S::Skin,side*smile*.08f);
      head(M::Feature,side*mouthWidth*.44f,mouthY+smile*.004f-pain*.002f,.104f,{.008f,.004f,.006f},shade(.40f,.24f,.19f),S::Skin,0,0);
      head(M::Feature,side*.014f,1.715f,.139f,{.010f,.0038f,.007f},shade(.29f,.17f,.12f),S::Skin,0,0);
      const float x=side*.052f,y=1.782f;
      // Dark socket/rim stays visible through closure. Iris and pupil are
      // contained inside the aperture, including at maximum gaze and blink.
      head(M::Feature,x,y,.109f,{.041f,eyeHeight+.004f,.014f},shade(.42f,.29f,.22f),S::Skin);
      if(eyeHeight>.0007f) {
        head(M::Feature,x,y,.114f,{.034f,eyeHeight,.010f},{.64f,.63f,.57f,1},S::Plain);
        const float irisHeight=std::min(.010f,eyeHeight*.78f);
        const float irisY=std::clamp(eyeY,-(eyeHeight-irisHeight)*.3f,(eyeHeight-irisHeight)*.3f);
        head(M::Feature,x+eyeX,y+irisY,.120f,{.010f,irisHeight,.003f},{.17f,.29f,.27f,1},S::Plain);
        head(M::Feature,x+eyeX,y+irisY,.122f,{.0045f,irisHeight*.65f,.002f},{.025f,.028f,.025f,1},S::Plain);
      }
      const float browRoll=-side*(.06f+focus*.21f-pain*.16f);
      head(M::Feature,x,y+.020f-focus*.003f+pain*.002f,.113f,{.046f,.0065f,.010f},hairTint,S::Cloth,browRoll);
      head(M::Feature,side*.121f,1.755f,-.004f,{.032f,.063f,.039f},skin,S::Skin,0,2);
      head(M::Feature,side*.132f,1.756f,.007f,{.009f,.037f,.020f},shade(.55f,.34f,.24f),S::Skin,0,0);
    }
    if (outfit.helm) {
      head(M::Sphere,0,1.86f,.012f,{.31f,.21f,.30f},steel,S::Metal);
      head(M::Box,0,1.817f,.15f,{.26f,.035f,.04f},{.72f,.54f,.24f,1},S::Metal);
      head(M::Blade,0,1.99f,0,{.04f,.19f,.16f},{.26f,.38f,.47f,1},S::Cloth);
      for (float side : {-1.f,1.f}) head(M::Torso,side*.13f,1.73f,.075f,{.055f,.18f,.115f},steel,S::Metal);
    }
    // Fixed-length joints prevent the old stretched forearms during a strike.
    for (int i=0;i<2;++i) {
      const float side=i==0?-1.f:1.f;
      const auto& leg=pose.legs[i]; const auto& arm=pose.arms[i];
      const auto knee=world(leg.joint),ankle=world(leg.end);
      const bool coat=outfit.classId==rpg::ClassId::Priest || outfit.classId==rpg::ClassId::Druid ||
        outfit.classId==rpg::ClassId::Mage || outfit.classId==rpg::ClassId::Necromancer ||
        outfit.classId==rpg::ClassId::Psionic || outfit.classId==rpg::ClassId::Bard;
      if(outfit.tunic && coat) {
        auto cloth=color;
        if(outfit.appearance.dye) {const auto& dye=rpg::tunicDyes[outfit.appearance.dye];cloth={dye.r,dye.g,dye.b,1};}
        else {cloth.x*=.65f;cloth.y*=.65f;cloth.z*=.65f;}
        auto hem=knee;hem.y-=.04f;
        const auto waist=world(leg.root);
        // Split panels follow the solved thighs: walking/jumping retains a
        // leg opening instead of stretching one rigid skirt across both legs.
        for(float side:{-1.f,1.f}) renderer.DrawMeshBetween(M::CoatPanel,hem,waist,.33f,.23f,cloth,S::Cloth,
          {side*std::sin(yaw),0,side*std::cos(yaw)});
      }
      const float bootYaw=yaw+pose.footYaw[i];
      const XMFLOAT3 foot{ankle.x+std::sin(bootYaw)*.035f,ankle.y-.035f,ankle.z+std::cos(bootYaw)*.035f};
      renderer.DrawMesh(M::Boot,foot,{.23f,.24f,.37f},outfit.boots?leather:skin,outfit.boots?S::Cloth:S::Skin,bootYaw);
      if (outfit.boots) {
        renderer.DrawMesh(M::Boot,{foot.x,foot.y-.105f,foot.z},{.24f,.035f,.38f},{.09f,.075f,.05f,1},S::Cloth,bootYaw);
        renderer.DrawSegment({knee.x,knee.y-.025f,knee.z},{knee.x,knee.y+.025f,knee.z},.245f,.26f,{.39f,.29f,.15f,1},S::Cloth);
      }
      if(outfit.boots) {
        const float sine=std::sin(bootYaw),cosine=std::cos(bootYaw);
        // Two narrow welts and a brass clasp break up the plain boot shaft.
        for(int strap=0;strap<2;++strap) {
          const float t=.25f+strap*.22f;
          XMFLOAT3 band{ankle.x+(knee.x-ankle.x)*t,ankle.y+(knee.y-ankle.y)*t,ankle.z+(knee.z-ankle.z)*t};
          renderer.DrawMesh(M::Limb,band,{.215f,.025f,.225f},{.16f,.105f,.065f,1},S::Cloth,bootYaw);
          band.x+=sine*.115f;band.z+=cosine*.115f;
          renderer.DrawMesh(M::Box,band,{.035f,.032f,.016f},{.50f,.37f,.18f,1},S::Metal,bootYaw);
        }
      }
      const auto elbow=world(arm.joint),hand=world(arm.end);
      const auto handColor=outfit.gloves?leather:skin;
      const auto handMaterial=outfit.gloves?S::Cloth:S::Skin;
      const auto fingers=BuildHandPose(pose,side>0?1u:0u,outfit.weapon,outfit.shield,pose.handCast);
      const auto handFrame=XMLoadFloat4x4(&fingers.frame)*actorTransform;
      const auto handPoint=[&](XMFLOAT3 local) { return world(fingers.Point(local)); };
      if (outfit.gloves) {
        const XMFLOAT3 cuff{hand.x+(elbow.x-hand.x)*.32f,hand.y+(elbow.y-hand.y)*.32f,hand.z+(elbow.z-hand.z)*.32f};
        renderer.DrawSegment(cuff,hand,.205f,.22f,leather,S::Cloth);
      }
      // Wrist bridge overlaps both the solved forearm and shaped palm.
      const XMFLOAT3 wristStart{hand.x+(elbow.x-hand.x)*.12f,hand.y+(elbow.y-hand.y)*.12f,hand.z+(elbow.z-hand.z)*.12f};
      const auto lateral=XMVector3TransformNormal(XMVectorSet(1,0,0,0),handFrame);
      XMFLOAT3 facing; XMStoreFloat3(&facing,lateral);
      renderer.DrawMeshBetween(M::Finger,wristStart,handPoint({0,-.03f,-.043f}),.072f,.067f,handColor,handMaterial,facing);
      renderer.DrawHand(handFrame,BuildHandRig(fingers),fingers.mirrored,handColor,handMaterial);
      if(outfit.armor) {
        renderer.DrawMesh(M::Torso,{knee.x+std::sin(yaw)*.105f,knee.y,knee.z+std::cos(yaw)*.105f},
          {.22f,.24f,.075f},steel,S::Metal,yaw);
      }
      if (side>0 && outfit.weapon) {
        const auto weaponFrame=WeaponFrame(pose)*actorTransform;
        const auto weaponPart=[&](M mesh,float offset,XMFLOAT3 scale,XMFLOAT4 tint,S material) {
          renderer.DrawMeshTransformed(mesh,XMMatrixScaling(scale.x,scale.y,scale.z)*XMMatrixTranslation(0,offset,0)*weaponFrame,tint,material);
        };
        if(form==rpg::WeaponForm::Blade) {
          weaponPart(M::Blade,.53f,{.12f,.86f,.07f},blade,S::Metal);
          weaponPart(M::Limb,.005f,{.065f,.19f,.065f},leather,S::Cloth);
          weaponPart(M::Box,.10f,{.26f,.045f,.055f},blade,S::Metal);
          weaponPart(M::Sphere,-.11f,{.08f,.08f,.08f},blade,S::Metal);
        } else renderer.DrawMeshTransformed(WeaponMesh(form),weaponFrame,
          outfit.rewardWeapon?XMFLOAT4{1,.92f,.70f,1}:XMFLOAT4{.88f,.91f,.94f,1},S::Metal);
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
    if (outfit.armor && !outfit.shoulders) {
      const auto guardColor=outfit.shoulders?leather:steel;
      const auto guardMaterial=outfit.shoulders?S::Cloth:S::Metal;
      for (float side : {-1.f,1.f}) {
        part(M::Torso,side*.31f,1.46f,0,{.26f,.18f,.31f},guardColor,guardMaterial);
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

