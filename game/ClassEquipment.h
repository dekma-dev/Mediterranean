#pragma once
#include "game/Classes.h"
#include "game/Inventory.h"

namespace rpg {
// Starter/Watchguard remain durable quality-tier IDs. A hero's class supplies
// their weapon identity; changing presentation never rewrites saves or prices.
enum class WeaponForm { Blade,Mace,Handbow,Scepter,GroveStaff,AstralStaff,AshStaff,Focus,Baton,Pistol,Cleaver };
struct ClassWeapon {
  WeaponForm form;
  const char* starter;
  const char* reward;
  const char* type;
  const char* description;
};
inline constexpr std::array<ClassWeapon,int(ClassId::Count)> classWeapons={{
  {WeaponForm::Blade,"Frontier sword","Watchguard blade","Sword","A balanced blade with a leather-bound grip."},
  {WeaponForm::Mace,"Dawnwatch mace","Dawn oath mace","Mace","A flanged steel head above a reinforced haft."},
  {WeaponForm::Handbow,"Trail bow","Watchguard longbow","Bow","Seasoned yew, a wrapped grip and a waxed bowstring."},
  {WeaponForm::Scepter,"Dawn censer","Dawn reliquary","Scepter","An open brass crown holds a sacred stone."},
  {WeaponForm::GroveStaff,"Grove staff","Living bough","Staff","A carved branch crowned with curling green shoots."},
  {WeaponForm::AstralStaff,"Apprentice staff","Astral conduit","Staff","A bound shaft carries a crystal within a metal cradle."},
  {WeaponForm::AshStaff,"Ash staff","Soul crook","Staff","An ashen crook encloses a pale spirit stone."},
  {WeaponForm::Focus,"Veil focus","Mind prism","Focus","A faceted prism rests within a small brass cage."},
  {WeaponForm::Baton,"Resonance baton","Chronicler baton","Baton","A tapered conductor's rod with a weighted pommel."},
  {WeaponForm::Pistol,"Brass pistol","Watchguard pistol","Pistol","A brass barrel, walnut grip and raised hammer."},
  {WeaponForm::Cleaver,"Iron cleaver","Ember cleaver","Cleaver","A broad, forward-weighted blade with a reinforced spine."}
}};
inline const ClassWeapon& WeaponFor(ClassId id) { return classWeapons[ValidClass(id)?int(id):0]; }
inline bool PhysicalLauncher(WeaponForm form) { return form==WeaponForm::Handbow || form==WeaponForm::Pistol; }
inline bool StaffWeapon(WeaponForm form) {
  return form==WeaponForm::GroveStaff || form==WeaponForm::AstralStaff || form==WeaponForm::AshStaff;
}
inline ItemInfo DescribeClassItem(Item item,ClassId id) {
  auto result=DescribeItem(item);
  if(item==Item::Starter || item==Item::Watchguard) {
    const auto& weapon=WeaponFor(id);
    result.name=item==Item::Starter?weapon.starter:weapon.reward;
    result.type=weapon.type;result.description=weapon.description;
  }
  return result;
}
}
