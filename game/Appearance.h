#pragma once
#include <array>
#include <string>
#include "engine/Utf8.h"

namespace rpg {
// Small, versioned cosmetic choices; independent of combat stats and equipment.
struct Appearance {
  int skin=0, hair=0, dye=0;
  static constexpr int count=5;
  bool Valid() const { return skin>=0 && skin<count && hair>=0 && hair<count && dye>=0 && dye<count; }
};
struct Swatch { const char* name; float r,g,b; };
inline constexpr std::array<Swatch,5> skinTones{{
  {"Sunworn",.72f,.51f,.35f},{"Fair",.88f,.70f,.55f},{"Olive",.60f,.46f,.30f},
  {"Bronze",.43f,.28f,.18f},{"Umber",.26f,.16f,.11f}
}};
inline constexpr std::array<Swatch,5> hairColors{{
  {"Chestnut",.19f,.13f,.08f},{"Raven",.055f,.047f,.042f},{"Wheat",.58f,.43f,.21f},
  {"Copper",.40f,.15f,.065f},{"Silver",.62f,.63f,.61f}
}};
inline constexpr std::array<Swatch,5> tunicDyes{{
  {"Class colors",0,0,0},{"Oxblood",.34f,.095f,.08f},{"Forest",.12f,.24f,.15f},
  {"Midnight",.12f,.19f,.31f},{"Linen",.62f,.54f,.39f}
}};
inline bool NameLetter(unsigned c) {
  return (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>=0x410 && c<=0x44f) || c==0x401 || c==0x451;
}
inline std::size_t NameLength(std::string_view name) {
  std::size_t length=0;
  for(std::size_t offset=0;offset<name.size();++length) text::Next(name,offset);
  return length;
}
inline bool ValidCharacterName(const std::string& name) {
  if(name.empty() || name.size()>40 || NameLength(name)>20 || name.front()==' ' || name.back()==' ') return false;
  bool letter=false;
  for(std::size_t offset=0;offset<name.size();) {
    const auto c=text::Next(name,offset);
    if(NameLetter(c)) letter=true;
    else if(c!=' ' && c!='-' && c!='\'') return false;
  }
  return letter;
}
}
