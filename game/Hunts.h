#pragma once
#include "game/Quests.h"
namespace rpg {
inline constexpr QuestDefinition huntQuests[]{
 {"Winter grain","Farmer Darya","Defeat four boars in the scrub west of Greyfield Farm.",
  "We stacked enough grain for two winters. Then the boars learned to break our fences. Four animals are rooting through the western scrub. Drive them away before the carters return. Watch the lowered head: a boar commits to its charge, so step across its path instead of running straight backwards.",
  "The western scrub still holds the brood. Defeat four boars after accepting this commission; earlier kills do not count.",
  "The fence can be repaired, and the carters can work again. Take this pay from the farm chest.",
  "Our winter grain is safe. The creatures will return eventually, but your commission is already paid.",25,100,Item::None,false},
 {"Teeth on the timber road","Sawyer Boris","Defeat four wolves east of Timbermen's Rest.",
  "Every cart leaves with an escort now. Wolves approach along the eastern hollow, then circle behind the pack animals. Clear four from that hollow. Their bite is fast, but they must recover before striking again. Do not let a second wolf reach your back while you watch the first.",
  "Four wolves must be defeated after taking this commission. The hollow lies east of the sawyer's yard.",
  "We can haul timber again. The watch needs it more than ever. Your pay is ready.",
  "The timber road commission is paid. Yana keeps the old lodge farther northwest.",30,120,Item::None,false},
 {"The forester's ledger","Forester Yana","Recover three record chests in the Old Pine hunting grounds with E.",
  "Those chests contain the coppice ledger: which trees may be cut, which must stand, and where the seedlings took root. Without it the next winter will strip this ridge bare. Three chests were left among the wolves. Read the marked caches with E. Fighting is optional; each recovered record survives the journey home.",
  "Recover all three marked record chests. Stand on the ground beside a chest and press E; moving away cannot erase the notes.",
  "The ledger is whole. We can cut what we need and leave the forest alive. Thank you.",
  "The forest records are returned. The lodge keeps your name beside this year's planting.",40,180,Item::None,false},
 {"The barrow pack","Hunter Sevastian","Defeat the pack leader and at least two other wolves near Barrow Watch.",
  "The scarred wolf does not hunt alone. Two flanks screen its approach and the younger animals follow. Draw them apart before challenging the leader. I need the leader and two of its pack defeated after you accept. The stones east of our watch mark their hunting ground; keep a retreat route between them.",
  "Defeat the pack leader and at least two other wolves. Three ordinary wolves alone will not break the pack.",
  "The pack has lost its leader. Travellers have a chance to pass the barrows again. This is earned pay.",
  "The barrow hunt is complete. Your reward cannot be claimed again when the pack returns.",55,240,Item::None,false},
 {"Mother of the mire","Herbalist Lada","Defeat the bristle matriarch and at least two boars in the northern mire.",
  "I have left medicine growing in the reed beds because no one can reach it. A great sow guards the northern mire, and her brood follows every charge. Separate the smaller animals, then face the matriarch. Her long preparation is your warning: move sideways or interrupt before the final rush.",
  "The matriarch and two members of her brood must fall. Prepare your equipment before entering the mire.",
  "The gatherers can return to the reeds. This payment comes from every household that depends on them.",
  "The mire hunt is recorded and paid. There is still work at the eastern quarry.",65,260,Item::None,false},
 {"Medicine under slate","Quarryman Makar","Recover three medicine chests from the quarry hunting grounds with E.",
  "When the boars broke through we abandoned the cutting ground. Three chests of medicine remain beyond the slate heaps. Bring them back. There is no reward for slaughter here: use the gaps between the animals, control them, or fight if you must. Our injured workers need those stores before nightfall.",
  "Three marked medicine chests remain among the quarry boars. Read each with E, then return to the yard.",
  "All three chests, seals intact. Tonight the injured will sleep instead of counting every breath. Take your pay.",
  "The quarry medicine has been delivered. Your commission is complete.",75,300,Item::None,false}
};
}
