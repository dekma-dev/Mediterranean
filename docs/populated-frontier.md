# Populated frontier districts

Implemented 2026-10-31. This pass fills the existing 240 x 256 m heightfield with
six inhabited districts and persistent combat wildlife. It adds playable content
and uses the current authored collection; it does not enlarge the world bounds
or claim final environment art or encounter balance.

## Added content

- Six residents, bringing friendly residents and the original two services to 17.
- Six independent local commissions, alongside 18 campaign assignments: 24 total.
- 36 persistent creatures: 18 wolves, 18 boars, including six elite leaders.
- 12 buildings, 105 additional trees and 186 authored prop placements. Of these
  props, 174 have solid footprints; small lanterns and signs remain detail.
- Dirt approaches, farm/service yards, supply stacks, wells, carts, benches,
  fences, braziers, grave markers and collectible record/medicine chests.

The wolf, boar, trees and buildings use the accepted Blender foundations.
Fifteen prop types use the existing expanded 34-model archive. This pass does
not regenerate or edit Blender sources, skin weights or exported locomotion
clips. See [asset provenance](blender-assets.md) before changing source art.

## Where to play

Run `build-frontier-expansion/native/RPGNative.exe`, or the rebuilt Visual Studio
output `build-local/Debug/RPGNative.exe`. The matching launcher resides beside
each executable. Existing characters acquire the population on loading; a new
character is not required. Russian is the default UI language.

Press **E** near a speaker and explicitly accept the commission. **J > Local
commissions** lists the six assignments and allows tracking an accepted one.
The atlas and objective tracker lead to the remaining creature or cache and
then back to the speaker. Unaccepted work has `!`, ready hand-ins have `?`.

| District | Speaker coordinates (x, z) | Rank | Objective | Gold / XP |
| --- | --- | ---: | --- | ---: |
| Greyfield Farm | Darya (-27, -30) | 1 | Defeat four boars | 25 / 100 |
| Timbermen's Rest | Boris (29, -30) | 2 | Defeat four wolves | 30 / 120 |
| Old Pine Lodge | Yana (-68, 25) | 4 | Recover three record chests | 40 / 180 |
| Barrow Watch | Sevastian (73, 41) | 6 | Pack leader and two other wolves | 55 / 240 |
| Mirestead | Lada (-68, 74) | 7 | Matriarch and two other boars | 65 / 260 |
| Slate Quarry | Makar (69, 82) | 9 | Recover three medicine chests | 75 / 300 |

Ranks indicate creature strength, not quest acceptance gates. Darya and Boris
are the nearest new contacts southwest and southeast of the starting outpost.
Collection commissions permit avoiding or controlling creatures. Killing three
ordinary animals cannot bypass the two leader requirements. Earlier kills do
not count toward an unaccepted commission. Rewards are delivered once.

## Wildlife rules

Animals can be clicked or selected with Tab, attacked by all existing class
weapons and spells, interrupted, rooted and slowed. They roam near fixed homes,
notice visible players within 6.5 m, pursue, and return when the player moves
more than 20 m from home. Wolves circle during recovery. Boars prepare for 1.45
seconds, then physically rush along their committed heading in the final 0.4
seconds; walls, other actors, roots and stuns constrain the rush. A sidestep can
avoid contact. Slows also reduce rush distance. HUD and ground warnings include
the remaining rush reach. There is no teleport or sword trail on an animal hit.

Wolf combat adds neck/jaw motion over its existing 30-joint authored locomotion;
boars lower their heads using the existing presentation rig. Both settle on
their sides on defeat. These additive poses are not newly authored Blender
attack clips. Contact particles are bounded; bite/charge sounds reuse current
spatial hurt/ground cues, not new animal recordings.

Defeated animals restore after 120 seconds of active simulation, only when the
player is farther than 22 m from both their home and corpse. Loading preserves
the timer; it does not restore the creature. First defeat grants 10 XP (25 for
an elite); first search with E grants 2 gold. Each creature identity grants these
rewards only once per character. Repeated kills can finish an accepted hunt but
cannot farm additional XP/gold. Population updates suspend during veteran
rehearsals. Campaign enemy slots, progression, rewards and the level-ten cap
remain separate.

## Persistence and runtime bounds

Save version **22** adds the `WILDERNESS` section: creature identities, positions,
health, state, attack/respawn timers, first-kill/loot masks, commission progress
and tracked commission. Versions 1-21 migrate without resetting existing
progress. Old player positions covered by a new prop relocate to a clear edge;
invalid current saves reject atomically. Game, roster and launcher readers now
share a 16 KiB maximum snapshot size. Older executables cannot read version 22.

World layout and obstacle-aware road routing are constructed once and copied
into independent simulations. Scenery stays in retained static batches with
existing LODs. Nearby residents and creatures are distance-culled; the GPU actor
palette budget is unchanged. Creature positions are independent of render calls.

## Evidence and remaining acceptance

`PopulationTests` covers all six commissions cumulatively, actual spell-contact
kills for Warrior/Mage/Scout, spending quest earnings, duplicate rewards,
pre-acceptance kills, nearby/far respawn, current/legacy saves, corrupt-load
atomicity, all NPC/habitat paths, collision-free roads, bite/rush contacts at
30/144 Hz, sidesteps, control effects, blocked charges and skinned death poses.
Quest completion fixtures reduce target health to isolate progression rules;
they do not certify live combat balance.

Native captures are in `build-frontier-expansion/native/population-check`:
`yard-0.png` through `yard-5.png`, six district views, `commissions.png`, and
boar/wolf target, contact and defeated views. The local workload log records 180
CPU update/render/submission samples after 60 warm-up frames near a populated
habitat. The inspected Release run measured p50 1.57 ms, p95 1.92 ms, max 2.28
ms, 184 peak draw calls and 4,893,114 submitted triangles including shadow
passes. This is not a GPU-tail, interactive-FPS or long-session guarantee.

The full Release run passed 51 of 55 suites initially. The four remaining
fixture mismatches were corrected and their rerun passed: road-material
coordinates, campaign-only speaker gating, the adopted anatomical body budgets
and the fitted head's blink region. Thus all 55 registered suites have passed
against the resulting source. Logs use the `population-*` prefix under
`build-frontier-expansion`.

Nine selected Debug suites also pass: authored assets, character selection,
launcher, interface, simulation, navigation, class combat, population and trade.
The standalone selected-target dash regression passes in Release. Localization
audit reports 1,993 source occurrences and 1,847 Russian entries with no missing
player prose. Scoped text whitespace checks pass.

Both normal outputs (`build-local/Debug` and `build-ninja-release`) and the
independent verification builds were rebuilt. Hidden native smoke checks of
Visual Studio Debug, normal Release and verification Release all exit 0 after
fullscreen restoration, shader/device setup, resize and populated world/UI
capture. Visual Studio's first multiprocess build stalled at MSBuild startup;
the owned build was cancelled and the serial `/nodeReuse:false` retry succeeded.
No player profile was used or overwritten for these captures.

Manual priorities remain normal-health pulls and travel pacing across all three
classes, camera/foliage visibility while orbiting, creature feet on slopes,
individual district composition, dedicated animal audio and richer authored
attack clips. Several yards deliberately reuse the same kit/layout. This
quantity pass is not final reference-level world art.
