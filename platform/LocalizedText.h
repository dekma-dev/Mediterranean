#pragma once
#include <Windows.h>
#include "game/Localization.h"

namespace localization {
inline std::wstring Wide(std::wstring_view input) {
  if(input.empty()) return {};
  const int count=int(input.size());
  std::string source(WideCharToMultiByte(CP_UTF8,0,input.data(),count,nullptr,0,nullptr,nullptr),'\0');
  WideCharToMultiByte(CP_UTF8,0,input.data(),count,source.data(),int(source.size()),nullptr,nullptr);
  const auto translated=Translate(source);
  std::wstring result(MultiByteToWideChar(CP_UTF8,0,translated.data(),int(translated.size()),nullptr,0),L'\0');
  MultiByteToWideChar(CP_UTF8,0,translated.data(),int(translated.size()),result.data(),int(result.size()));
  return result;
}
inline int Message(HWND owner,std::wstring_view message,std::wstring_view title,UINT flags) {
  return MessageBoxW(owner,Wide(message).c_str(),Wide(title).c_str(),flags);
}
}
