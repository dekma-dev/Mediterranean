#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace visual {
inline constexpr unsigned bodyLodCount=3;
// Full body at useful inspection sizes; the medium/far meshes share the rig.
// Enter/leave bands prevent camera breathing from toggling a mesh every frame.
inline unsigned SelectBodyLod(float pixels,int previous=-1) {
  if(!std::isfinite(pixels) || pixels<0) return 0;
  if(previous<0 || previous>=int(bodyLodCount)) return pixels>=150?0:pixels>=65?1:2;
  unsigned level=unsigned(previous);
  if(level==0 && pixels<135) level=1;
  if(level==1 && pixels<58) level=2;
  if(level==2 && pixels>72) level=1;
  if(level==1 && pixels>165) level=0;
  return level;
}
class BodyLodHistory {
public:
  void BeginFrame() { if(++frame_==0) { entries_={}; frame_=1; } }
  void Reset() { entries_={}; frame_=1; }
  unsigned Select(std::uint64_t actor,float pixels) {
    if(!actor) return SelectBodyLod(pixels);
    Entry* slot=&entries_[0];
    for(auto& entry : entries_) {
      if(entry.actor==actor) {
        entry.level=SelectBodyLod(pixels,frame_-entry.frame<=2?int(entry.level):-1);
        entry.frame=frame_; return entry.level;
      }
      if(entry.frame<slot->frame) slot=&entry;
    }
    *slot={actor,frame_,SelectBodyLod(pixels)}; return slot->level;
  }
private:
  struct Entry { std::uint64_t actor=0,frame=0; unsigned level=0; };
  std::array<Entry,64> entries_{};
  std::uint64_t frame_=1;
};
}
