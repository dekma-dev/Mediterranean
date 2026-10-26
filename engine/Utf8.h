#pragma once
#include <cstdint>
#include <string_view>

namespace text {
inline std::uint32_t Next(std::string_view value,std::size_t& offset) {
  if(offset>=value.size()) return 0;
  const auto first=static_cast<unsigned char>(value[offset++]);
  if(first<128) return first;
  const unsigned count=first>=0xc2 && first<=0xdf?1:first>=0xe0 && first<=0xef?2:first>=0xf0 && first<=0xf4?3:0;
  if(!count || offset+count>value.size()) return '?';
  std::uint32_t code=first&((1u<<(6-count))-1);
  for(unsigned i=0;i<count;++i) {
    const auto ch=static_cast<unsigned char>(value[offset]);
    if((ch&0xc0)!=0x80) return '?';
    ++offset;code=(code<<6)|(ch&63);
  }
  if(code<(count==1?0x80u:count==2?0x800u:0x10000u) || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) return '?';
  return code;
}
}
