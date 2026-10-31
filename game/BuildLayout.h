#pragma once
#include "game/Interface.h"

namespace ui::buildlayout {
// Logical 1280x720 coordinates shared by drawing and scaled pointer tests.
// Every discipline is visible together; page state from older callers is ignored.
inline constexpr Rect Spells{124,150,192,420};
inline constexpr Rect Field(int branch) { return {330.f+170.f*branch,150,162,420}; }
inline constexpr Rect Spell(int card,int count=11) { return {132,180.f+(count>9?33.f:41.f)*card,176,count>9?31.f:38.f}; }
inline constexpr Rect Talent(int index) { return {344.f+170.f*(index%3),184.f+62.f*(index/3),142,54}; }
}
