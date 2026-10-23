# Dawnwatch frontier expansion

The playable bounds increased from 34 × 38 to **58 × 62 metres**, about **2.78 times the area**. Dawnwatch, the dock, training post, merchant and original anchor keep their coordinates. New buildings, trees and survey stones lie outside the old playable rectangle, so migrated actors do not spawn inside new scenery. The southern margin remains open meadow; this is a larger prototype island, not a finished open world.

## Residents and assignments

Complete the original patrol, anchor and seven keeper dispatches. Then follow the atlas objective to these residents in order. All three can be visited earlier; they give a greeting until their prerequisite is complete.

| Resident | Location (x, z metres) | Assignment | Goal |
|---|---|---|---|
| Warden Elian | Western lodge, −21, 2 | The west road opens | Defeat three roadbreakers in Westward Reach; return to Elian |
| Surveyor Mara | Eastern survey house, 21, 3 | An honest map | Read three coastal stones with E; return to Mara |
| Archivist Serevan | Northern shelter, 10, 18 | The bastion remembers | Defeat three sentinels in the northern court; return to Serevan |

Each has an original offer, reminder, turn-in and receipt passage, a visible model, nearby nameplate and atlas/minimap marker. E opens the existing dialogue; acceptance and completion remain explicit. After handing in an assignment, the tracker leads to the next speaker. Each awards 250 XP once; level 10 and five talent points remain the caps. No additional loot currency is introduced.

Survey stones are at (22, 11), (26, 22) and (16, 25). Stand within 2.3 metres, grounded and out of a dash, and press E. The stones are solid, so interaction checks sight to their near face. Read them in any order; the tracker selects the nearest unread stone. Reading a stone twice grants nothing. All three notes survive F5/F9, death and returning to another area. R resets the expedition and its survey.

## World and UI

New dirt routes lead west past the dock, east past the training ground, and north around the old ruin. Three additional buildings, a northern ruin, nine trees and bounded ground cover extend the scenery. The parchment atlas, minimap, terrain texture sampling, collision, camera and navigation use the enlarged bounds. The minimap title follows the player's region. The journal lists twelve assignments; dialogue objective counters distinguish bearings from enemies and do not inherit the previous squad's completion on a new offer.

The three new NPCs reuse the existing humanoid meshes with outfit colors and idle motion. The two new squads reuse existing enemy archetypes at higher campaign ranks. Only three campaign enemies are active at once; this pass does not add independent ambient spawns or schedules. Ground cover remains deterministic and capped at 340 clumps, including at most 28 shrubs. Map/ground textures keep their existing resolution; no assets or dependencies were downloaded.

## Saves and checks

Version 13 appends `SURVEY 0..7`; versions 1–12 remain readable. Existing classes, names, appearance, gear, builds and campaign progress are preserved. Completed version-12 campaigns continue at the western lodge. Invalid masks or inconsistent completed-survey states reject the whole load without mutating the current game. Older executables cannot read version-13 saves; use the current build consistently.

`FrontierExpansionTests` walks to all residents, verifies locked conversations, migrates old saves, completes both new battles and the survey, checks duplicate rewards and round-trips each stage, and renders atlas/dialogue/journal/world captures. `NavigationTests` sweeps the enlarged ground and checks every drawn road segment for actor clearance. Existing terrain, geometry, interface, campaign and save tests cover shared behavior. Captures are in `build-ninja-debug/frontier-expansion-check/`; run manual checks 202–204 for live travel, readability and balance.

Launch `build-ninja-release/MediterraneanLauncher.exe` after rebuilding Release. M opens the atlas, J the journal, E speaks/reads, and F5/F9 save/load. Current automated results are recorded in [progress](../progress.md).
