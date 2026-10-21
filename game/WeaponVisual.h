#pragma once
#include "game/ClassEquipment.h"
#include "engine/Geometry.h"

inline visual::Mesh WeaponMesh(rpg::WeaponForm form) {
  using W=rpg::WeaponForm;using M=visual::Mesh;
  switch(form) {
    case W::Mace: return M::WeaponMace;
    case W::Handbow: return M::WeaponHandbow;
    case W::Scepter: return M::WeaponScepter;
    case W::GroveStaff: return M::WeaponGroveStaff;
    case W::AstralStaff: return M::WeaponAstralStaff;
    case W::AshStaff: return M::WeaponAshStaff;
    case W::Focus: return M::WeaponFocus;
    case W::Baton: return M::WeaponBaton;
    case W::Pistol: return M::WeaponPistol;
    case W::Cleaver: return M::WeaponCleaver;
    default: return M::Blade;
  }
}
