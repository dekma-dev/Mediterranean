# Four-class opportunity presentation — 2026-11-14

## Reference and scope

Reviewed fresh contact sheets from `D:/assets/learning_material/pve_loc2.mp4` at 7.5–9.5 seconds (0.1-second spacing) and `pve_raid_2.mp4` at 24–28 seconds (0.2-second spacing). The relevant observations are actor-attached persistent auras, short separate contact accents, and readable ability/cast feedback through successive actions. These are sampled visual observations, not a complete frame-by-frame or audio analysis. They do not establish the recorded game's underlying spell formulas.

This pass makes the project's existing Riposte, Pursuit shot, Elemental confluence and Benediction visible throughout earning, holding and spending them. It does not introduce those mechanics or claim complete reference parity. Models, faces, authored animation files, audio, damage formulas and accepted contact times are outside this pass.

## Visible changes

- Warrior: an open shield-side parry arc; held Riposte flows along the actual blade endpoints; confirmed expenditure cuts across the victim with a separate directional metallic accent. A missing blade retains only the guard cue.
- Scout: three open air paths and fletching strokes follow the real bow release socket. Pursuit's confirmed contact expands that directional form at the struck actor.
- Mage: confirmed fire, frost and arcane schools build distinct spatial trails with a fire volume, ice facets and an arcane core. Ready confluence carries all three; an accepted moving cast gathers them toward the live release socket. Changing the selected target hides incomplete element progress for the previous target.
- Priest: held Benediction arches from both hands behind the torso with rising gold threads. Spending it on an offensive hit produces descending converging light at the victim, distinct from the existing healing completion.
- A brass-framed strip above the action rows shows the real state, remaining time, class icon and applicable current bindings. Mage accumulation names each earned/missing element. The strip fades at its boundaries, reserves room from world labels and leaves status icons/cast meters clear. Removed, duplicated and rearranged hotbar entries are handled explicitly; Russian strings are included.

`game/ClassOpportunity.cpp` owns the stateless attached effects and pulses. Invalid time/coordinates, unrelated event kinds, expired pulses and invalid/dead attachment owners are rejected. Existing 512-vertex per-opportunity test limits, 6,144 total effect vertices and 24 volume slots remain unchanged.

## Reproducible native review

`SpellMotionReview <output-directory> --opportunities --check-events` runs the same four nine-second sequences without rendering. The movie mode omits the final flag and records a 36-second 1280×720, 60 Hz clip. Both modes require actual ready/spent events and real release/contact events. The Mage sequence waits for accepted actions rather than blindly issuing the next spell at a fixed time. No synthetic opportunity or contact events are injected. Actor placement is a controlled review fixture; player profiles are not written. The movie has no audio and is not an FPS benchmark.

The initial attempts exposed a premature Mage command and a review fixture that moved a dummy's simulation position while its authored prop remained at the map location. The final fixture uses actual presented raiders (held in place for the ranged cases), and the Mage waits for accepted actions. Incomplete recordings are not accepted four-class evidence; their caster/HUD images provide only qualitative baseline context. Accepted evidence and test results are listed below after validation.

## Workspace provenance

There are concurrent character, equipment and renderer edits in this workspace. Review builds use a frozen source/runtime-asset copy under `build-opportunities/source`, then receive only this pass's code changes. No editable Blender file is copied or modified by this pass. Do not substitute these isolated libraries into another build tree. Rebuild the ordinary game from the complete working tree to include subsequent work from the other window.

The isolated Debug build uses `/Z7 /Ob0 /Od /RTC1`; the initial compiler probe with `/Zi` failed with C1902. This is a build-directory-only choice, with no system/toolchain settings changed. Assertions and the existing test timeouts remain enabled.

## Automated validation

- Release passed all seven selected CTest suites in 99.70 seconds: ClassEffects, SpellChains, SustainedEffects, CastingEffects, ClassDiscipline, CombatHud controls and CombatHud world.
- The initial Debug run passed six of seven. SpellChains exposed a stale test pointer coordinate at desktop HUD density: it still clicked the old unscaled hotbar location. The driver now applies the existing action-row transform to hotbar points, retaining every remove, drag, cancellation, reassignment, persistence and save-isolation assertion. All four affected/reviewed suites then passed in 114.62 seconds. Together with the unchanged three effects suites, this covers all seven selected suites in Debug.
- A final confluence-path correction makes each trail terminate at its corresponding elemental core. ClassDiscipline was rebuilt and passed again in Release (9.92 seconds) and Debug (20.75 seconds), including actual 30/60/144 Hz opportunity rules, target-bound accumulation, mobile casting, expiry/load/cancellation, binding rearrangement, finite-input rejection and per-effect geometry limits. No timeouts or tolerances were increased.
- The final event-only four-scene preflight passed. The rendered movie's entire event CSV is identical to that preflight (including ready/spent, release and contact frames). Unrelated suites were not rerun.
- Ordinary `build-ninja-debug` and `build-ninja-release` game/launcher builds succeeded with their existing compiler settings. Both public game executables and the isolated Release host passed hidden startup, shader/device initialization, fullscreen restoration, resize and world/UI capture. Existing local-shadowing compiler warnings remain; this is not a warning-free build claim. No commits, staging changes or pushes were made by this pass.

## Accepted evidence

- Movie: `build-opportunities/accepted/combat-motion.mp4`, 36 seconds, 1280×720 at 60 Hz, 19,573,651 bytes. Warrior occupies 0–9 seconds, Scout 9–18, Mage 18–27 and Priest 27–36.
- Reviewed full-size ready/contact/element frames for all four classes, plus a decoded 0.2-second sequence through the Mage's gathering, moving cast and contact at 20.7–23.1 seconds. Images and event/budget CSVs are beside the movie; the decoded contact sheet is `build-opportunities/accepted-flow/sheet-0.jpg`.
- `accepted-inputs.json` records working/snapshot source and runtime-asset hashes; `accepted-artifacts.json` records movie and executable hashes. Logs include `tests-release.log`, `tests-debug.log`, `tests-reviewed-debug.log`, `tests-endpoints-{debug,release}.log`, `event-comparison.log`, `movie-accepted.log` and `smoke-public-{debug,release}.png.log`, all under `build-opportunities/`.

Measured maxima over each complete recorded scene:

| Scene | Effect vertices | Transient lights | Draw calls | Spell volumes |
| --- | ---: | ---: | ---: | ---: |
| Warrior Riposte | 1,620 | 4 | 213 | 0 |
| Scout Pursuit | 2,118 | 4 | 232 | 0 |
| Mage Confluence | 2,442 | 4 | 211 | 8 |
| Priest Benediction | 1,566 | 4 | 198 | 2 |

These are complete-scene maxima, not just the added opportunity geometry. They remain below the existing effect/volume limits and do not establish an FPS or memory improvement. Rendering the review offline is intentionally slower than playing its 60 Hz recording.

## Remaining work

Full crowded encounter acceptance, every ability's individual motion silhouette, final art/lighting correspondence and audio comparison remain open. This checkpoint concerns class opportunity readability and attached effects, not a completed recreation of the reference game.
