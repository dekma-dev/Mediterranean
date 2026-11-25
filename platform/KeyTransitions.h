#pragma once
#include <array>
#include <cstdint>

// Bounded per-key edges between game frames, not an unbounded repeat queue.
// Multiple presses of one key in a frame coalesce; the first press owns modifiers.
class KeyTransitions {
public:
  enum Modifier : unsigned { Control=1,Shift=2,Alt=4 };
  struct Edges {
    std::array<bool,256> pressed{},released{};
    std::array<unsigned char,256> modifiers{};
  };
  void Add(unsigned key,bool down,bool repeat,unsigned modifiers=0) {
    if(key>=256)return;
    if(down && !repeat && !edges_.pressed[key]){edges_.pressed[key]=true;edges_.modifiers[key]=static_cast<unsigned char>(modifiers&7);}
    if(!down)edges_.released[key]=true;
  }
  Edges Consume(){const auto result=edges_;edges_={};return result;}
  void Clear(){edges_={};}
private:
  Edges edges_;
};
