#pragma once
#include "game/Classes.h"
#include "game/Appearance.h"
#include "engine/ClassArt.h"
namespace oldstyle {
inline bool Focused(rpg::ClassId id){return id==rpg::ClassId::Warrior||id==rpg::ClassId::Scout||id==rpg::ClassId::Mage;}
inline unsigned Index(rpg::ClassId id){return id==rpg::ClassId::Scout?1u:id==rpg::ClassId::Mage?2u:0u;}
inline classart::Part Part(rpg::ClassId id,unsigned slot){return classart::Part(Index(id)*7+slot);}
inline visual::Mesh Mesh(rpg::ClassId id,unsigned slot){return classart::Mesh(Part(id,slot));}
inline DirectX::XMFLOAT4 Cloth(rpg::ClassId id){return id==rpg::ClassId::Scout?DirectX::XMFLOAT4{.28f,.32f,.23f,1}:id==rpg::ClassId::Mage?DirectX::XMFLOAT4{.30f,.265f,.335f,1}:DirectX::XMFLOAT4{.37f,.30f,.22f,1};}
inline DirectX::XMFLOAT4 Paint(rpg::ClassId id,rpg::Appearance appearance){
 if(!appearance.dye)return {1,1,1,1};
 const auto& dye=rpg::tunicDyes[appearance.dye];const auto base=Cloth(id);
 // Negative alpha selects a cloth-only tint in the authored-material shader.
 return {dye.r/base.x,dye.g/base.y,dye.b/base.z,-1};
}
}
