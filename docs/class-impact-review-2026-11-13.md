# Four-class impact and flight silhouettes

## Reference and scope

Revisited the supplied training and raid footage, including 24 new decoded samples at 50 ms intervals from `D:/assets/learning_material/allods_fight_interface.mp4` (120.8–122.0 seconds), and the existing `pve_raid_2.mp4` samples at 20–23 seconds. Inspected full-size training/raid frames as well as the contact sheets. These short passages show directional release streams, concentrated bright contacts and readable gaps between overlapping effects. They do not establish a particular spell formula or complete frame-by-frame equivalence to the whole recording.

The native baseline had narrow Warrior/Scout cues and largely shared impact silhouettes. This pass implements original world-space choreography for existing accepted actions. No reference textures, models or client data were copied. Blender assets and faces are unchanged.

## Runtime changes

- A new continuous filament material carries several moving bright strands inside a soft, broad translucent ribbon. Coordinates continue across mesh joins; the accepted effect age drives motion. Depth softening, opaque occlusion and the existing bounded glare pass apply to it, including character previews. It uses the existing effect vertex batch and adds no texture, GPU buffer or persistent combat state.
- Warrior heavy attack, Sweeping steel, Shieldbreaker, Iron cyclone and Crossing steel have differentiated lateral, descending, circular and crossing cuts. Their broad impact sheets are anchored to the accepted caster and adapt to the height difference to the victim. Small hot metal fragments and ground dust finish the contact. The independent measured blade trail retains the actual grip/tip history and now uses flowing material coordinates anchored to sample birth times.
- Scout heavy/piercing/signature/finishing shots gain tapered crossed air wakes along the actual recorded flight path. Confirmed strong contacts eject a compact forward fan and physical chips. Ordinary arrows, fan spacing, delayed volley releases and the committed target remain authoritative; this does not turn arrows into fireballs or add through-wall damage.
- Mage heavy fireball and Falling star compress a hot volume at contact, then release irregular curved flame tongues, spreading billows, cooling smoke and ballistic embers. The heavy fireball stays compact around its victim; the meteor expands from ground contact. Its falling mass and torn wake are also wider. Existing frost, arcane and sustained firestorm identities remain separate.
- Priest holy flights have broad flowing vanes. Sanctuary unfolds paired gold/ivory feather bands with staggered shafts, an open central volume and a short ground seal. A first native review exposed excessive white overlap; the final bands have separated tips and lower opacity so the target remains visible.
- Selected strong impacts replace their old generic layers instead of stacking both. Secondary area victims keep smaller local receipts. Damage, casting/recovery times, resource costs, range, line of sight, save data and the learned spell catalogue are unchanged.

## Reproducible review

`SpellMotionReview OUTPUT --signatures` records four accepted actions (Warrior 8, Scout 9, Mage 9, Priest 6) through the normal game update/render path at 1280×720, 60 Hz, five seconds per class. The same fixture, camera, input frames and UI are used before and after. The review earns Priest reserve with real attacks. Targets in this comparison are rooted deliberately; use the separate crowded review for movement and warning overlap. No user profiles are written.

- Baseline: `build-class-impact/before/combat-motion.mp4`.
- Final comparison: `build-class-impact/after/combat-motion.mp4`.
- Moving three-opponent encounter: `build-class-impact/crowded/combat-motion.mp4`.
- Each directory includes accepted release/contact frames in `events.csv`, peak submission counts in `budgets.csv`, and preparation/flight/contact stills.
- Raw reference samples: `build-class-impact/reference-warrior/`; first native material/angle inspection: `build-class-impact/contact-preview/`.

Generated review files are ignored local build artifacts, not new tracked binary assets. Video captures are silent and GPU/encoder-bound offline runs; their wall-clock recording speed is not interactive FPS. The implementation is a larger silhouette/material pass for the four classes, not complete reference parity, new animation clips or a new balance pass.

## Validation

The selected eight suites passed in Release (208.51 seconds) and Debug (289.55 seconds): `CharacterSelectionTests`, `CombatEffectsTests`, `ClassEffectsTests`, `VfxMaterialsTests`, `SpellVolumeTests`, `MovingCombatTests`, `CombatReleaseTests` and `CastingEffectsTests`. After the final fire-shape refinement, the complete `VfxMaterialsTests` suite passed again in both configurations.

New coverage checks filament continuity across one versus nineteen quads, visible material motion, identical repeated renders, opaque-wall/glare clipping and malformed input rejection. Expanded contact coverage exercises the new Warrior/Scout abilities and meteor at four ages from three ordinary views; equal-time 30/60/144 Hz, expiry, secondary-contact and six-major-contact tests retain their existing limits. Six simultaneous major contacts must leave at least 1,000 of the 6,144 effect vertices for other cues, without silently losing geometry to the 24-volume ceiling. No limits were raised to make the new effects pass.

Visual Studio Debug/Release game and launcher targets were rebuilt. Both public game executables and both review Debug/Release game executables passed hidden startup, device/shader, resize, fullscreen-restoration and world/UI capture smoke checks. `git diff --check` passed for this pass. Existing unrelated compiler shadowing warnings remain; no model or facial source/export was changed. Validation completed on November 14 after the work began on November 13.

Concurrent-work qualification: after these builds, another working session changed `PlayerClips`, `CharacterPose`, `AuthoredWeapon` and `FootPlacement` (observed at 00:02–00:03 on November 14). Those edits were preserved and are not part of this VFX pass. The results above apply to the built VFX snapshot, not a claim that the later combined animation working tree has already passed them. `build-class-impact/validation-snapshot.json` records the tested executable fingerprints. No unrelated animation edits were reverted or incorporated into the VFX commit description.

The two 20-second movies have identical `events.csv` contents: accepted release/contact frames are unchanged for all four actions. Peak counts across each five-second scene:

| Action | Before vertices | After vertices | Before/after peak draw calls |
| --- | ---: | ---: | ---: |
| Sweeping steel | 1,254 | 1,194 | 213 / 213 |
| Piercing arrow | 666 | 906 | 208 / 208 |
| Falling star | 1,950 | 1,452 | 208 / 208 |
| Sanctuary | 1,656 | 1,566 | 198 / 198 |

Peak transient lights remain four. Reduced vertex counts do not by themselves prove better FPS: wider translucent coverage can cost more pixels. Build/test times above are wall-clock validation times, not performance measurements.

The final 28-second three-opponent review completed with actual accepted actions, pursuing enemies, hostile cast warnings and player movement. Peak effect vertices were 3,180 (Warrior), 4,896 (Mage frost), 3,894 (Scout fan) and 3,816 (Priest Sanctuary); peak transient lights stayed at four. Inspected dense/contact frames keep warning boundaries, incoming cast meters and enemy silhouettes readable in this fixture. This is not a raid-scale acceptance test.

The separate 1920×1080 native workload also passed. Six mixed-class contacts plus six aftermath patches peaked at 2,862 effect vertices and 238 draw calls. Effect-phase GPU observations: 178 samples, median 7.220 ms, p95 7.835 ms; CPU submission/Present: 180 samples, median 7.286 ms, p95 7.948 ms. The preceding no-contact phase supplied only 12 ready GPU observations, so its timing is too weak for a claimed speedup or reliable incremental-effect comparison. The raw report is `build-class-impact/workload/workload.txt`; these offline fixture timings are not a promised live-game FPS.
