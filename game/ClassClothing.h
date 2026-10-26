#pragma once
#include "game/CharacterView.h"
#include "engine/Geometry.h"
#include <array>
#include <cmath>

// Small rigid fittings follow the solved torso. Limbs and the underlying
// garment remain skinned; these are not a replacement for cloth simulation.
namespace clothing {
struct Part {
 visual::Mesh mesh;
 DirectX::XMFLOAT3 position,scale;
 DirectX::XMFLOAT4 color;
 visual::Material material;
};
struct Fittings { std::array<Part,64> parts{}; unsigned count=0; };
inline Fittings Build(const CharacterOutfit& outfit) {
 using M=visual::Mesh;using S=visual::Material;using C=DirectX::XMFLOAT4;
 Fittings result;
 const C brass{.62f,.44f,.20f,1},iron{.32f,.38f,.40f,1},bone{.69f,.64f,.48f,1},
  hide{.24f,.13f,.075f,1},ink{.09f,.105f,.14f,1};
 const auto& info=rpg::classes[unsigned(outfit.classId)<rpg::classes.size()?unsigned(outfit.classId):0];
 C fabric{info.color[0]*.65f,info.color[1]*.65f,info.color[2]*.65f,1};
 if(outfit.appearance.dye>0 && unsigned(outfit.appearance.dye)<rpg::tunicDyes.size()) {
  const auto& dye=rpg::tunicDyes[outfit.appearance.dye];fabric={dye.r,dye.g,dye.b,1};
 }
 const auto add=[&](M mesh,float x,float y,float z,DirectX::XMFLOAT3 scale,C color,S material) {
  if(result.count<result.parts.size()) result.parts[result.count++]={mesh,{x,y,z},scale,color,material};
 };
 const auto chest=[&](float x,float y,DirectX::XMFLOAT3 scale,C c,S material) {
  add(M::Torso,x,y,.194f,scale,c,material);
 };
 if(outfit.tunic) {
  switch(outfit.classId) {
   case rpg::ClassId::Warrior:
    add(M::GuardCuirass,0,1.25f,0,{1,1,1},outfit.armor?C{1,.98f,.88f,1}:C{.78f,.83f,.89f,1},S::Metal);
    // Riveted shoulder straps and a restrained vertical heraldic clasp.
    for(float side:{-1.f,1.f}) {
      add(M::Torso,side*.18f,1.45f,.11f,{.065f,.16f,.06f},hide,S::Leather);
      add(M::Sphere,side*.18f,1.46f,.145f,{.026f,.026f,.013f},brass,S::Metal);
    }
    add(M::Blade,0,1.32f,.227f,{.045f,.19f,.015f},brass,S::Metal);break;
   case rpg::ClassId::Paladin:
    chest(0,1.25f,{.24f,.43f,.035f},fabric,S::Cloth);
    chest(0,1.32f,{.055f,.19f,.055f},brass,S::Metal);
    chest(0,1.34f,{.16f,.045f,.058f},brass,S::Metal);break;
   case rpg::ClassId::Scout:
    for(int n=0;n<5;++n) {chest(.14f-n*.065f,1.42f-n*.075f,{.075f,.09f,.04f},hide,S::Leather);
     chest(.14f-n*.065f,1.42f-n*.075f,{.03f,.025f,.065f},bone,S::Cloth);}break;
   case rpg::ClassId::Priest:
    for(float side:{-1.f,1.f}) {chest(side*.12f,1.27f,{.085f,.42f,.04f},bone,S::Cloth);
     for(int n=0;n<3;++n) chest(side*.12f,1.12f+n*.045f,{.08f,.014f,.052f},brass,S::Metal);}break;
   case rpg::ClassId::Druid:
    for(int n=0;n<5;++n) chest((n-2)*.065f,1.42f-std::abs(n-2)*.025f,{.07f,.12f,.05f},n%2?hide:fabric,S::Cloth);break;
   case rpg::ClassId::Mage:
    for(float side:{-1.f,1.f}) {add(M::Torso,side*.13f,1.53f,-.04f,{.12f,.22f,.25f},fabric,S::Cloth);
     chest(side*.11f,1.29f,{.025f,.36f,.04f},brass,S::Metal);}
    chest(0,1.42f,{.08f,.10f,.055f},bone,S::Metal);break;
   case rpg::ClassId::Necromancer:
    for(float side:{-1.f,1.f}) for(int n=0;n<3;++n) chest(side*(.065f+n*.042f),1.40f-n*.05f,{.033f,.11f,.06f},bone,S::Cloth);
    chest(0,1.25f,{.10f,.14f,.05f},ink,S::Metal);break;
   case rpg::ClassId::Psionic:
    for(int n=0;n<3;++n) chest(0,1.40f-n*.12f,{.08f-n*.012f,.08f-n*.012f,.055f},brass,S::Metal);
    for(float side:{-1.f,1.f}) chest(side*.15f,1.32f,{.03f,.26f,.035f},fabric,S::Cloth);break;
   case rpg::ClassId::Bard:
    chest(-.10f,1.29f,{.14f,.39f,.035f},fabric,S::Cloth);
    for(int n=0;n<5;++n) chest(.065f,1.43f-n*.065f,{.026f,.026f,.05f},brass,S::Metal);break;
   case rpg::ClassId::Engineer:
    chest(0,1.24f,{.34f,.38f,.04f},hide,S::Leather);
    for(float side:{-1.f,1.f}) {chest(side*.13f,1.40f,{.055f,.15f,.06f},iron,S::Metal);
     for(int n=0;n<3;++n) chest(side*.105f,1.28f-n*.045f,{.055f,.03f,.07f},brass,S::Metal);}break;
   case rpg::ClassId::Demonologist:
    for(float side:{-1.f,1.f}) for(int n=0;n<3;++n) chest(side*(.06f+n*.045f),1.36f-n*.06f,{.06f,.11f,.065f},n%2?hide:iron,S::Metal);break;
   default:break;
  }
 }
 // Layered closure, collar lining and small paired rivets read at portrait
 // distance. These remain torso-bound and obey the existing attachment LOD.
 if(outfit.tunic && outfit.classId!=rpg::ClassId::Warrior) {
  const bool armored=outfit.classId==rpg::ClassId::Warrior || outfit.classId==rpg::ClassId::Paladin || outfit.classId==rpg::ClassId::Engineer;
  for(float side:{-1.f,1.f}) {
   chest(side*.045f,1.26f,{.012f,.34f,.018f},armored?iron:hide,armored?S::Metal:S::Leather);
   add(M::Torso,side*.075f,1.51f,.075f,{.11f,.055f,.16f},hide,S::Leather);
   for(int n=0;n<4;++n) chest(side*.052f,1.13f+n*.075f,{.018f,.018f,.028f},brass,S::Metal);
  }
 }
 if(outfit.shoulders) {
  const bool plate=outfit.classId==rpg::ClassId::Warrior || outfit.classId==rpg::ClassId::Paladin || outfit.classId==rpg::ClassId::Engineer;
  const bool spines=outfit.classId==rpg::ClassId::Druid || outfit.classId==rpg::ClassId::Necromancer || outfit.classId==rpg::ClassId::Demonologist;
  for(float side:{-1.f,1.f}) {
   add(plate?M::Pauldron:M::Torso,side*.32f,1.48f,0,{plate?.31f:.25f,plate?.19f:.13f,.34f},plate?iron:fabric,plate?S::Metal:S::Cloth);
   for(int n=0;n<3;++n) add(spines?M::Blade:M::Torso,side*(.27f+n*.045f),1.55f+(spines?.035f*n:0),.015f,
    {spines?.035f:.025f,spines?.14f:.022f,.20f},spines?bone:brass,plate?S::Metal:S::Cloth);
  }
 }
 if(outfit.belt) {
  // Tools are attached to the waist equipment, not permanently to the hero.
  const bool tools=outfit.classId==rpg::ClassId::Engineer || outfit.classId==rpg::ClassId::Scout;
  if(tools) for(int n=0;n<3;++n) add(M::Limb,-.25f-n*.025f,.86f,.08f,{.024f,.20f,.024f},iron,S::Metal);
  else if(outfit.classId==rpg::ClassId::Mage || outfit.classId==rpg::ClassId::Priest || outfit.classId==rpg::ClassId::Necromancer)
   {add(M::Box,-.25f,.83f,.08f,{.13f,.19f,.07f},hide,S::Leather);
    add(M::Box,-.25f,.83f,.12f,{.08f,.14f,.012f},bone,S::Cloth);}
  else if(outfit.classId==rpg::ClassId::Bard) for(int n=0;n<3;++n)
   add(M::Limb,-.25f-n*.026f,.86f,.08f,{.02f,.18f-n*.025f,.02f},brass,S::Metal);
 }
 return result;
}
}
