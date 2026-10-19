# Quest text

Edit the UTF-8 text files in frontier-patrol/ and astral-anchor/:
- offer.txt: the assignment before accepting.
- progress.txt: the reminder while objectives remain.
- turn-in.txt: the report before claiming rewards.
- complete.txt: the receipt after rewards are granted.

Use blank lines to separate paragraphs. Dialogue automatically wraps and paginates the complete text; objectives and rewards remain visible on every page. Titles, speaker names, objectives and reward definitions are in QuestText.h.in. The existing font uses the project's Latin character set.

CMake embeds these files in QuestText.generated.h at build time. Editing a listed text file triggers CMake regeneration on the next build. No runtime file paths or external text downloads are required. Do not put the raw-string terminator )quest" in the prose; configuration rejects it. Rebuild to see edits.

Reward numbers and item IDs are gameplay data. Changing them also requires reviewing progression and save validation; editing story prose does not change saved progress.
