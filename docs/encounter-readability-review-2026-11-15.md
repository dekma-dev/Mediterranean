# Encounter warnings and hostile preparation

## Reference and scope

Revisited the supplied `D:/assets/learning_material/pve_raid_2.mp4` at 28–31 seconds, with twenty samples spaced 150 ms apart, alongside the previous raid/PvE samples and the native crowded four-class review. The reference retains legible ground boundaries and actor-local buildup while several spells overlap. This observation guides readability; it does not establish the recorded game's damage formulas or complete visual parity.

This pass concerns the Warrior, Mage, Scout and Priest fighting active groups. It changes warning presentation and threat ordering, not combat balance, hit clocks, animation assets, models, faces, saves or networking.

## Visible and functional changes

- Incoming attacks prioritize a footprint that currently threatens the player, then the release deadline and stable actor identity. Previously three earlier safe casts could hide a later dangerous one. The world and HUD use the same bounded selection, including line of sight and the existing jump-height rule. Boar charge reach remains a conservative forecast because scenery and the player can shorten its movement.
- A new alpha-blended danger material combines the actual frontal sector and optional quake annulus. The inner flank and gap remain unpainted. Its outer boundary is fixed from the first frame; a quieter internal wave shows preparation progress. Interruptible and uninterruptible attacks retain amber/purple colouring, supplemented by different HUD marker shapes.
- Warning geometry uses the terrain's own grid, heights and triangle diagonal. This avoids the interpolation mismatch of a separately sampled patch. Conservative cell rejection reduces empty geometry around narrow lances. Opaque scenery still hides warnings; the danger material does not enter spell bloom.
- Quake preparation draws soil streams toward the animated palms; Tempest winds open rising sheets into a central volume; Wraith/Execution preparation forms a directed, contracting helix. Aim follows the committed attack direction and the origin follows the real hand. Existing release/contact effects remain connected to their captured origin.
- The incoming board gains restrained threat-row shading, a moving cast-bar edge and an enclosed marker for attacks that cannot be interrupted. Row clicks continue to select the actual caster at all supported HUD scales.

## Bounds and review

The renderer keeps the existing 6,144-vertex total effect limit, twelve field records, twenty-four volume records and four combat lights. Field constants grow from 384 to 768 bytes. Ground-mark storage is reserved once for 3,600 vertices; warning generation uses fixed local arrays and no per-frame heap allocation. Individual hostile preparations remain below 360 effect vertices. Cosmetic preparations cannot consume the budget before all selected ground warnings.

Local review artifacts are in `build-encounter-readability/`. `before/` uses the previous accepted opportunity-review executable; `after/` is an intermediate inspection before final terrain-grid alignment. `accepted/` uses the final Release executable. These are silent offline native captures, not measured gameplay FPS or an audio comparison. `SpellMotionReview --crowded-far` repeats the same accepted actions with a wider camera for legibility review.

## Validation and accepted evidence

All seven selected suites pass in Release (90.52 seconds) and Debug (167.69 seconds): SustainedEffectsTests, SpellVolumeTests, CombatReleaseTests, CastingEffectsTests, InterfaceTests, CombatHudTests and CombatHudWorldTests. Tests cover actual safe holes, three rotated narrow-lance directions, opaque occlusion, twelve probes around a sloped terrain annulus, warning priority under roster reordering, death/cancellation, invalid preparation timers, solved-hand attachment, existing release continuity and scaled HUD picking. Renderer debug-message checks remain enabled. The initial pixel-probe failure was caused by treating normalized projection coordinates as pixels; correcting the test conversion retained every visibility/safe-zone threshold.

Both ordinary Ninja and Visual Studio game/launcher configurations were rebuilt. Four public game hosts passed hidden startup, shader/device, resize, fullscreen-restoration and world/UI capture checks; paths, executable hashes and exit codes are in `validation-snapshot.json`. Visual Studio's aggregate invocation stalled at its file-glob check and was stopped; ordered direct project builds and explicit shader generation then completed. No system settings were changed. Existing Interface shadowing warnings remain; these are successful builds, not warning-free builds.

`accepted/combat-motion.mp4` contains four seven-second encounters at 1280x720/60 Hz: Warrior, Mage, Scout and Priest. Its release/contact CSV is byte-for-byte identical to `before/events.csv`; this is the player's accepted event trace, not a log of every hostile action. The movie was decoded into 56 samples at half-second intervals. Preparation, dense and contact stills also accompany each scene. Source references were sampled, not exhaustively annotated frame by frame.

`accepted-far/combat-motion.mp4` repeats all four encounters with the wider camera. Its player-event CSV also matches exactly. Both movies are 28 seconds at 1280x720/60 Hz, and both have half-second contact sheets; normal-camera progression and distant dense frames were inspected. The far view peaks at 2,154 / 3,642 / 2,550 / 2,760 effect vertices and 249 / 250 / 241 / 237 draw calls, with unchanged light/volume peaks. Movie hashes are in `movie-hashes.json`; `source-snapshot.json` identifies the local reviewed source and capture executable. These artifacts live in ignored build directories and are not added to Git.

Peak submitted effect geometry in the normal-camera encounter:

| Class | Before vertices | Accepted vertices | Before / accepted peak draw calls |
| --- | ---: | ---: | ---: |
| Warrior | 3,180 | 2,148 | 243 / 244 |
| Mage | 5,010 | 3,636 | 244 / 244 |
| Scout | 3,894 | 2,544 | 235 / 236 |
| Priest | 3,816 | 2,754 | 232 / 232 |

Every scene remains at four lights or fewer; accepted peak spell-volume counts are 2 / 3 / 1 / 3 respectively. The additional volume in relevant scenes is the hostile preparation core. These bounded geometry measurements do not prove an FPS improvement, and a three-opponent fixture is not a raid-scale load test.

## Remaining work

Larger encounter composition, individual player action silhouettes, audio and full reference correspondence remain open. These changes do not establish a complete recreation of the supplied game footage. Human playtesting is still needed for subjective warning strength and clutter at the player's preferred camera distance.
