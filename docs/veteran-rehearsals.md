# Veteran encounters and tactical areas

Implemented 2026-10-31. This follows the Iron Court checkpoint: make completed encounters available to established characters, deepen the three priority kits, and give their staging camps more identity. The map remains 240 x 256 metres, the campaign has eighteen assignments, and the level cap is ten. These are three repeatable versions of existing assignments, not three additional campaign chapters.

## Playing the encounters

After completing an assignment, revisit its leader with no campaign assignment currently active:

| Leader | Position (X, Z) | Replay |
| --- | --- | --- |
| Marshal Radan | -8, 56 | The last gate: two patrols and the Ashen Overlord |
| Warden Ilar | -74, -70 | Three answers in the mist: three waves and beacon holds |
| Captain Vesna | 73, -91 | The iron court: five sentries, three seals and the protected Iron Regent |

Press **E**, read the repeat-encounter rules, then choose **Begin rehearsal** (Russian: **Начать поход**). These are lethal encounters with normal combat and existing character equipment/talents. The campaign's completed assignments, experience and level do not change. Enemies have their original encounter strength; this is not a new difficulty multiplier.

The upper-left panel records elapsed objective time, actual damage dealt/taken, absorbed damage and successful interrupts. Time stops when all objectives are complete or the player dies. Return to the leader to finish and receive the report, or use **End rehearsal** to abandon. Reports remain visible for twenty seconds. The timer includes approach/travel time: it is not a DPS meter.

There are no XP, gold or loot rewards. Health, potions and cooldowns are retained, so leaving cannot refund combat costs. Temporary reserve, wards, evasion and frost fields are cleared at encounter boundaries. Death remains lethal; normal Enter recovery ends the rehearsal and restores the original campaign encounter. Repeated/stale reward commands cannot complete another campaign chapter.

Saving keeps current health, inventory, equipment, cooldowns and position, paired with the original campaign enemies and progress. Saving alone does not interrupt live play. Loading exits the transient rehearsal; partial replay objectives and reports are intentionally not persisted. Invalid loads leave the active rehearsal intact. Save format remains version 21; existing version 1–21 migration is retained.

## Tactical differences

- **Warrior / Sweeping steel, level 5:** a successful forward sweep raises a guard of six per victim, capped at eighteen. It does not stack on top of a larger existing ward. Protected targets grant no guard. Positioning to catch several enemies now has a defensive payoff.
- **Mage / Frost bloom, level 5:** the original four-second root and direct damage remain. A valid contact also leaves one three-metre frost field for five seconds. Enemies entering it are slowed, with a short 0.3-second exit tail. It does not repeatedly root, deal periodic damage or affect targets through walls; returning enemies and the warded captain are excluded. A subsequent field replaces the first. Death, load and encounter transitions remove it. Terrain-conforming fissures and a soft boundary show the area without tall particle walls.
- **Scout / Piercing shot, level 8:** the original target takes 130% heavy damage. Living enemies behind it, within a 1.1-metre-wide corridor and the normal spell range, take 65% heavy damage. Each contact requires line of sight from both shooter and primary contact. Rooted victims retain the existing additional 35% heavy-damage bonus; exposure lasts five seconds. Selection changes cannot redirect the committed shot. This rewards lining up targets instead of surrounding them.

Use **N** to inspect the translated descriptions and drag the abilities to the action bar. Existing bindings are preserved.

## Camps and interface

Radan's, Ilar's and Vesna's existing authored shelters gain subdued, ragged standards and raised charcoal baskets with bounded drifting embers. The basket uses the authored asset adopted by the concurrent art workstream. Placements reuse the established shelter/camera apron; this pass adds no blocking structures or new textures. The surrounding house/character asset revisions belong to that parallel workstream.

Quest dialogue now reports actual site, wave and six-enemy counts instead of treating every assignment as three kills. Court dialogue includes seal progress. Repeat offers explain their lack of rewards, begin at zero objectives, and provide a working leave button and live report. All added prose is available in Russian and English.

References reviewed: [Allods' official combat basics](https://allods.my.games/en/game/game-guide/basics) describes explicit targets, ability range and rejection feedback. [Blizzard's Artcraft level-design discussion](https://worldofwarcraft.blizzard.com/en-us/news/16595426/artcraft-level-design-part-4) connects encounter spaces with landmark history and sightlines. The replay rules and class changes above are our implementation choices. Existing local video/environment analysis remains the visual baseline; this pass does not claim a fresh frame-by-frame video review or visual parity.

## Verification and remaining work

`SouthernTests` exercises all three replays with all three classes, genuine interaction/channel completion, no repeated rewards, real potion costs, death/recovery, save/load isolation, stale commands and objective timers. Tactical geometry checks run at 30/144 Hz: guard caps, secondary shot damage, corridor/range exclusions, wall rejection, late frost entrants, exit decay and field expiry. Native captures exercise the actual localized screens, leave-button hit region, camps and frost rendering.

Integration found a Debug stack overflow while drawing the localized UI. A local debugger identified the catalog initializer and then nested character-pose evaluation on an exhausted caller stack. The catalog source is now static read-only data; the renderer's bounded body/hand staging arrays live on the heap. Buffer byte counts, shader layout and actor limits are unchanged. InterfaceTests now passes the previously failing scenario without increasing the process stack limit. The temporary invalid normal winding in a concurrently updated asset archive was corrected by that art workstream; strict decoder checks remain enabled.

Outputs are under `build-frontier-expansion/native/southern-check`: `replay-offer.png`, `replay-active.png`, `replay-report.png`, `veteran-camp-0.png` through `veteran-camp-2.png`, and `frost-field.png`. `six-actors.txt` records the existing hidden six-enemy CPU submission workload; it is not interactive FPS or a GPU benchmark. Final build/test results are recorded in `progress.md`.

Manual play is still required to assess full encounter difficulty, resource attrition, class/build balance and subjective visual readability. Automated lifecycle tests deliberately arrange deaths to isolate objectives; they are not evidence of three complete, balanced human combat runs. Further roadmap work remains: slope-aware gait, richer authored terrain/architecture composition, separate spell icon art and larger quest variety.
