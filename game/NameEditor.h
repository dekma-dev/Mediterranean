#pragma once
#include "game/Appearance.h"
#include <algorithm>

namespace ui {
// Positions count Unicode characters, never UTF-8 bytes. Incomplete names
// remain editable; profile publication still validates the finished result.
struct NameEditor {
  std::size_t caret=0,anchor=0;
  static std::size_t Byte(const std::string& value,std::size_t character) {
    std::size_t offset=0;while(character>0 && offset<value.size()){text::Next(value,offset);--character;}return offset;
  }
  std::size_t Begin() const {return std::min(caret,anchor);}
  std::size_t End() const {return std::max(caret,anchor);}
  bool Selected() const {return caret!=anchor;}
  void Place(const std::string& value,std::size_t position,bool extend=false) {
    caret=std::min(position,rpg::NameLength(value));if(!extend)anchor=caret;
  }
  void All(const std::string& value) {anchor=0;caret=rpg::NameLength(value);}
  void Move(const std::string& value,int direction,bool extend) {
    if(!extend && Selected()){Place(value,direction<0?Begin():End());return;}
    Place(value,direction<0?(caret?caret-1:0):caret+1,extend);
  }
  void Erase(std::string& value,bool backward) {
    if(!Selected()) {
      if(backward){if(!caret)return;anchor=caret-1;}
      else {if(caret>=rpg::NameLength(value))return;anchor=caret+1;}
    }
    const auto begin=Begin(),from=Byte(value,begin),to=Byte(value,End());
    value.erase(from,to-from);caret=anchor=begin;
  }
  void Insert(std::string& value,std::string_view incoming) {
    // An invalid paste must not erase a selected name.
    std::string accepted;const auto room=20-(rpg::NameLength(value)-(End()-Begin()));
    for(std::size_t offset=0,count=0;offset<incoming.size() && count<room;) {
      const auto from=offset;const auto ch=text::Next(incoming,offset);
      if(rpg::NameLetter(ch) || ch==' ' || ch=='-' || ch=='\'') {accepted.append(incoming.substr(from,offset-from));++count;}
    }
    if(accepted.empty())return;
    if(Selected())Erase(value,false);
    value.insert(Byte(value,caret),accepted);caret+=rpg::NameLength(accepted);anchor=caret;
  }
};
}
