# Four-class preparation and release continuity

## Reference and scope

This pass revisits `D:/assets/learning_material/pve_loc2.mp4` at 10.4–11.6 seconds (24 samples at 50 ms intervals) and `pve_raid_2.mp4` at 20–24 seconds (20 samples at 200 ms intervals). Full-size PvE frames and both raid contact sheets were inspected. The useful observations are a local buildup at the actor, a short directed discharge, a brighter confirmed contact and a quieter aftermath. The raid sequence also shows why long opaque flashes would conceal other combat cues. These sampled passages do not identify every class or establish exact spell formulas or complete reference parity.

The native baseline still used small floating brackets for strong Scout preparation and flat art for much of frost/arcane gathering. This work replaces those preparation forms and connects them to the accepted release. It follows the previous impact/flight pass rather than adding another generic explosion to every spell. Models, faces, authored files, icons and audio assets are outside this pass.

## Runtime changes

- Warrior preparation follows the actual equipped blade endpoints. A travelling metal glint and restrained sparks distinguish loaded techniques from ordinary cuts; the effect disappears when the blade is absent. Direction-only legacy sword clips return along an outboard arc when blending opposing cuts, preserving the accepted hit clock and complete authored grip frames.
- Scout strong shots draw open air filaments from the string hand toward the real launch socket. The nock gathers a small highlight, then opens a short discharge behind the arrow. Basic/opening/twin shots remain quiet. The old floating preparation brackets are retained only for Engineer.
- Mage frost gathers closed six-sided crystal facets in three dimensions around a cold volume, with palm streams and falling mist. Arcane gathers three intersecting contracting spirals and travelling knots. Both retain their gathered core at release and compress it while the actual projectile grows, instead of abruptly swapping to a flat release card. Existing fire preparation keeps its separate flame material.
- Priest preparation uses paired hand streams and, for Sanctuary/healing, ascending gold and ivory paths. Offensive release compresses the gathered holy volume. Completed healing opens an attached upward fountain; Sanctuary retains its separate outward target contact from the previous pass.
- All preparation is derived from the current accepted action and solved attachment sockets. No persistent emitters, asset downloads, new textures, damage events, resource grants or save-format changes were added. Cancellation, death and load still remove the corresponding action presentation.

## Reproduced gameplay and motion faults

The new four-class queue regression first failed with `Accepted queued attack was lost at its cooldown/deadline boundary`. A newly accepted queue was charged for the same frame's elapsed time, and expiration was checked before a simultaneous cooldown crossing. Fresh queue time now starts when input is accepted; a ready action passes through normal ability validation before expiration. Target identity, life, range, line of sight, resources and ordinary cooldown checks remain authoritative.

The combined animation snapshot also failed the unchanged alternating-cut audit: 13.8964 m/s peak wrist speed at 144 Hz, above its 13 m/s bound. A diagnostic probe traced this to the direction-only legacy grip: the direct guard-to-cut interpolation passed through the forearm axis, destabilizing its fallback roll. The legacy sword blend now follows the existing outboard spherical arc. Full-weight source direction, complete authored grip frames, transition duration and gameplay clocks remain unchanged. The isolated old-resource regression then passed at 30/60/144/1000 Hz (5.54/6.63/7.16/7.27 m/s); single/cancelled actions peaked at 12.67 m/s. Test bounds were not relaxed.

The concurrently updated PCL6 resources exposed a separate full-grip reversal (18.7308 m/s at 144 Hz). Re-selecting the shortest quaternion arc each frame flipped the guard/action interpolation as the source stroke crossed 180 degrees from its guard. That outer sword blend now uses a common outboard quaternion hemisphere, with exact source endpoints and an antipodal fallback. Source-key interpolation, authored roll, cast/contact clocks and the source/export files remain intact. The native pose probe passed at 30/60/144/1000 Hz (5.38/6.14/6.94/7.15 m/s). The final Release executable, including the guarded production implementation, passed the complete signature suite with 5.11/5.58/5.52/5.55 m/s alternating-cut peaks and 12.67 m/s for single/cancelled actions. The 13 m/s bound is unchanged.

## Native review

`SpellMotionReview OUTPUT --invocations` records six accepted actions through normal game update/rendering: Warrior Shieldbreaker, Scout Piercing arrow, Mage frost and arcane, Priest Sanctuary and healing. Each scene lasts five seconds at 1280×720/60 Hz. The review earns Priest reserve through actual accepted attacks and writes no user profile. This is a controlled rooted-target comparison; crowded movement is reviewed separately.

- Baseline: `build-class-casting/before/combat-motion.mp4`.
- First inspection: `build-class-casting/after/combat-motion.mp4` (before the final Scout visibility adjustment and concurrent animation-resource update).
- Earlier effects integration: `build-class-casting/final/combat-motion.mp4` (includes the final Scout visibility adjustment; predates the final PCL6 grip correction).
- Final validated build: `build-class-casting/verified/combat-motion.mp4`, recorded after the final native checks with the PCL6 correction and current embedded animation resources. Scenes begin at 0/5/10/15/20/25 seconds: Warrior, Scout, Mage frost, Mage arcane, Priest Sanctuary and Priest healing.
- Accepted action frames: each directory's `events.csv`; submission peaks: `budgets.csv`.
- Reference samples: `build-class-casting/reference/` and `reference-raid/`.
- Reproduction logs: `build-class-casting/queue-before.log`, `queue-after.log` and `tests-release.log` (initial combined-build motion failure).

Movies are silent, ignored local build artifacts. Their offline capture duration does not measure interactive FPS. Other session edits to gait, grips and cloak were preserved; comparison images may also reflect that concurrent presentation work. This pass does not claim complete visual equivalence to the reference game.

The earlier `final/` movie establishes the effects adjustment but predates the PCL6 sword transition repair. The `verified/` movie records the final tested executable; its SHA-256 is in `build-class-casting/movie-verified-snapshot.json`. The longer alternating-cut motion regression checks transitions beyond this six-action movie. Inspected final preparation, contact and healing frames as well as the earlier sampled release sequences.

The baseline and final validated movies have identical `events.csv` contents for all six actions. Inspected preparation/release/contact stills and 14 additional samples each around frost release (10.90–11.34 seconds) and holy release (21.10–21.54 seconds). The core contracts as the real projectile leaves; there is no new contact at the caster. The final validated movie retains the earlier effects integration's submission peaks over each five-second scene:

| Action | Before vertices | Final vertices | Before/final draw calls |
| --- | ---: | ---: | ---: |
| Shieldbreaker | 1,146 | 1,146 | 213 / 213 |
| Piercing arrow | 906 | 906 | 208 / 208 |
| Frost gathering | 2,262 | 2,262 | 208 / 208 |
| Arcane prism | 1,326 | 1,452 | 208 / 208 |
| Sanctuary | 1,566 | 1,566 | 198 / 198 |
| Greater healing | 1,356 | 1,056 | 196 / 198 |

Peak transient lights remain four. Preparation retains its existing 512-vertex test bound; the 6,144 effect vertices and 24 volume-instance limits were not raised. These counts do not establish an FPS improvement: translucent pixel coverage and the concurrent character work must be measured separately.

## Validation

The original combined material/world review exceeded its 120-second CTest limit, and the combined locomotion/math/capture review exceeded its Release 60-second limit. Material checks, eight martial world actions and six caster world actions now run as separate CTest processes. Locomotion uses its existing motion-only and capture-only modes. The final combined HUD review also exceeded its 60-second Debug limit while capturing the last of its three world classes; it now separates control/picking/cast-receipt checks from full world encounters. These splits remove no assertions or cases, raise no numerical tolerances or per-process time limits, and retain the complete reviews in the default standalone invocations. Native rendering checks run sequentially to avoid competing for the same GPU.

Shared-workspace provenance: `accepted-inputs.json` records the critical source/resource snapshot used for the final game builds. A later concurrent CMake edit changes only the ClassEquipmentTests Debug timeout from 45 to 120 seconds; `post-build-config.json` records the difference. The reported equipment runs passed under the original 45-second limit, before that edit. The subsequent HUD test split changes test registration/driver code only, so it does not require rebuilding the already validated game runtime. User staging and concurrent asset changes were preserved.

- Visual Studio Debug/Release game and launcher builds succeeded, as did both Ninja review builds. Existing local-shadowing compiler warnings in CharacterPose remain; this is not a warning-free build claim.
- The final Release run passed all 19 then-registered selected suites in 473.17 seconds. Debug passed 18 of 19 in 1,292.18 seconds; the sole failure was the combined HUD timeout described above, with no failed behavioral assertion reported.
- After splitting the HUD driver, both resulting suites passed in Release (9.92 and 5.14 seconds) and Debug (47.28 and 15.05 seconds), under the original 60-second limit per process. Together with the unchanged 18 other suites, this covers all 20 selected suites in each configuration. Unrelated repository suites were not rerun. Logs: `tests-accepted-release.log`, `tests-accepted-debug.log`, `hud-split-tests-release.log` and `hud-split-tests-debug.log` under `build-class-casting/`.
- Debug and Release agree on the final alternating-cut measurements above. Preparation checks retain the 512-vertex limit and verify 30/60/144 Hz equality, real blade/hand sockets, invalid-input rejection, cancellation, death and load isolation. The queue regression fails before the gameplay fix and passes afterward.
- All four game hosts passed hidden startup, device/shader initialization, resize, fullscreen restoration and world/UI rendering with PNG captures. Executable SHA-256 hashes and capture paths are in `build-class-casting/validation-snapshot.json`; each capture has its own `.png.log`.
