#pragma once
#include "engine/Geometry.h"
#include "game/Appearance.h"
#include <array>

// Presentation identities do not alter class, equipment or saved player choices.
enum class HumanFace : unsigned { Player, Broad, Lean, Young, Mature, Elder, Count };
inline visual::Mesh FaceHead(HumanFace face) {
  const auto id=unsigned(face);
  return id>0 && id<unsigned(HumanFace::Count)?
    visual::Mesh(unsigned(visual::Mesh::FaceBroadHead)+(id-1)*5):visual::Mesh::Head;
}
inline visual::Mesh FaceHair(HumanFace face) {
  const auto head=FaceHead(face);
  return head==visual::Mesh::Head?visual::Mesh::Hair:visual::Mesh(unsigned(head)+3);
}
struct ResidentAppearance { HumanFace face; rpg::Appearance colors; };
inline constexpr std::array<ResidentAppearance,15> residentAppearances{{
  {HumanFace::Broad,{1,1,0}}, // Elian
  {HumanFace::Young,{1,0,0}}, // Mara
  {HumanFace::Lean,{1,4,0}}, // Serevan (authored collection actor)
  {HumanFace::Young,{2,1,0}}, // Veyra
  {HumanFace::Lean,{1,4,0}}, // Orin
  {HumanFace::Broad,{2,0,0}}, // Radan
  {HumanFace::Mature,{2,0,0}}, // Tessa
  {HumanFace::Broad,{0,1,0}}, // Ilar
  {HumanFace::Mature,{1,3,0}}, // Vesna
  {HumanFace::Mature,{1,0,0}}, // Darya
  {HumanFace::Broad,{0,0,0}}, // Boris
  {HumanFace::Elder,{2,0,0}}, // Yana
  {HumanFace::Lean,{0,1,0}}, // Sevastian
  {HumanFace::Elder,{1,4,0}}, // Lada
  {HumanFace::Broad,{2,4,0}}, // Makar
}};
static_assert(unsigned(HumanFace::Count)==visual::faceVariantCount+1);
