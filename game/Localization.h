#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace localization {
enum class Language { Russian, English };
inline Language language=Language::Russian;
inline std::filesystem::path settings;
inline std::string playerName;
inline void Initialize(const std::filesystem::path& directory) {
  settings=directory/"language.ini";language=Language::Russian;
  std::ifstream file(settings);std::string value;std::getline(file,value);
  if(value=="en") language=Language::English;
}
inline bool Select(Language next) {
  language=next;
  if(settings.empty()) return true;
  std::ofstream file(settings,std::ios::trunc);file<<(next==Language::Russian?"ru":"en")<<'\n';
  file.flush();return bool(file);
}
struct Entry {std::string_view english,russian;};
inline const std::vector<Entry>& Dictionary() {
  static const auto entries=[] {
    std::vector<Entry> values{
#include "game/Russian.inc"
    };
    std::stable_sort(values.begin(),values.end(),[](const Entry& a,const Entry& b){return a.english.size()>b.english.size();});
    return values;
  }();return entries;
}
inline std::string Translate(std::string_view input) {
  if(language==Language::English || input==playerName) return std::string(input);
  // Bound dynamic HUD text cache.
  static std::unordered_map<std::string,std::string> cache;
  if(auto entry=cache.find(std::string(input));entry!=cache.end()) return entry->second;
  const auto letter=[](char c){return (c>='A' && c<='Z') || (c>='a' && c<='z');};
  std::string result;result.reserve(input.size()*2);
  for(std::size_t i=0;i<input.size();) {
    bool found=false;
    for(const auto& entry:Dictionary()) {
      const auto size=entry.english.size();
      if(i+size>input.size() || input.substr(i,size)!=entry.english) continue;
      if((i && letter(input[i-1]) && letter(entry.english.front())) ||
         (i+size<input.size() && letter(input[i+size]) && letter(entry.english.back()))) continue;
      result+=entry.russian;i+=size;found=true;break;
    }
    if(!found) result+=input[i++];
  }
  if(cache.size()>=512) cache.clear();
  cache.emplace(std::string(input),result);
  return result;
}
}
