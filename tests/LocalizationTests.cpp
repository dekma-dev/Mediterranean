#include "game/Localization.h"
#include "engine/Utf8.h"
#include <iostream>
int main() {
  using namespace localization;
  const auto directory=std::filesystem::current_path()/"localization-test-settings";
  std::filesystem::create_directories(directory);
  {std::ofstream(directory/"language.ini")<<"unknown\n";}
  Initialize(directory);
  const auto check=[](bool ok){if(!ok) throw std::runtime_error("Localization regression");};
  check(language==Language::Russian);
  check(Translate("Return to game")=="Вернуться в игру");
  check(Select(Language::English)); Initialize(directory);
  check(language==Language::English && Translate("Return to game")=="Return to game");
  check(Select(Language::Russian)); Initialize(directory);
  check(language==Language::Russian);
  playerName="Warrior"; check(Translate(playerName)==playerName);
  std::size_t offset=0; check(text::Next("Язык",offset)==0x42f && offset==2);
  check(Translate("English")=="English");
  std::cout<<"Language defaults, persistence, nickname and UTF-8 passed\n";
}
