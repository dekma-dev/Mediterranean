#include "game/Localization.h"
#include "engine/Utf8.h"
#include "game/Simulation.h"
#include "game/ClassEquipment.h"
#include <iostream>
#include <set>
int main() {
  using namespace localization;
  const auto directory=std::filesystem::current_path()/"localization-test-settings";
  std::filesystem::create_directories(directory);
  {std::ofstream(directory/"language.ini")<<"unknown\n";}
  Initialize(directory);
  const auto check=[](bool ok){if(!ok) throw std::runtime_error("Localization regression");};
  check(language==Language::Russian);
  std::set<std::string_view> keys;
  for(const auto& entry:Dictionary()) {
    check(keys.insert(entry.english).second);
    check(Translate(entry.english)==entry.russian);
    check(Translate(entry.russian)==entry.russian);
    for(std::size_t at=0;at<entry.russian.size();) {
      const auto ch=text::Next(entry.russian,at);
      check(ch=='\n' || (ch>=32 && ch<127) || (ch>=0x400 && ch<0x460) ||
        ch==0xab || ch==0xbb || ch==0x2013 || ch==0x2014 || ch==0x2212 || ch==0x2019);
    }
  }
  check(Translate("2 talent points available")=="2 очк. вех доступно");
  check(Translate("Casting / 1.5s")=="Применение / 1.5с");
  check(Translate("Level 10  /  Warrior")=="Уровень 10  /  Воин");
  const auto translated=[](const char* value){return !*value || Translate(value)!=value;};
  for(int id=0;id<int(rpg::ClassId::Count);++id) {
    rpg::Simulation hero;check(hero.CreateCharacter(rpg::ClassId(id),"Алёна-Ёж"));
    const auto saved=hero.Save();
    check(translated(hero.ClassInfo().name) && translated(hero.ClassInfo().title) && translated(hero.ClassInfo().role) && translated(hero.ClassInfo().identity));
    for(int spell:{0,1,2,3,5,6,7}) {
      const auto data=hero.Spell(spell);
      check(translated(data.name) && translated(data.description) && translated(data.targeting));
    }
    check(hero.Save()==saved);
  }
  for(int id=int(rpg::QuestId::Patrol);id<=int(rpg::QuestId::Watch10);++id) {
    const auto& quest=rpg::DescribeQuest(rpg::QuestId(id));
    check(translated(quest.title) && translated(quest.objective));
    for(auto stage:{rpg::QuestStage::Offer,rpg::QuestStage::Progress,rpg::QuestStage::TurnIn,rpg::QuestStage::Complete})
      check(translated(rpg::QuestStory(rpg::QuestId(id),stage)));
  }
  check(Translate("Return to game")=="Вернуться в игру");
  check(Select(Language::English)); Initialize(directory);
  check(language==Language::English && Translate("Return to game")=="Return to game");
  check(Select(Language::Russian)); Initialize(directory);
  check(language==Language::Russian);
  playerName="Warrior"; check(Translate(playerName)==playerName);
  std::size_t offset=0; check(text::Next("Язык",offset)==0x42f && offset==2);
  check(Translate("English")=="English");
  std::cout<<"Russian catalog, glyphs, complete class/quest descriptions, composed HUD, language persistence and UTF-8 passed\n";
}
