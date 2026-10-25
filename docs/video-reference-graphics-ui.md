# Recorded reference: materials and combat interface

## Review scope — 2026-10-25

Reviewed existing frame sheets from `allods_fight_interface.mp4` at approximately 00:00–00:19 and 01:34–01:53. Extracted and inspected frames from `allods_dange_fight.mp4` at 10, 60, 120, 180 and 240 seconds, plus six samples across 180.0–180.6 seconds. Sources remain in the user's NVIDIA recordings directory. This is a sampled visual review, not a complete frame-by-frame review or audio analysis. Local review images are in the ignored `build-video-review` directory.

## Observations and implemented response

- Reference armor and architecture separate reflective metal, dull stone and fabric. Replaced the broad specular approximation with roughness-based GGX distribution, geometric masking and Fresnel-weighted diffuse lighting. Added approximate sky/ground reflection and derivative roughness filtering. This is not full image-based lighting or a local reflection system.
- Reference surfaces have finer variations than our broad ground patches. Muted grass color, increased ground texture frequency and added deterministic grain/gravel relief to the existing packed material layer. No additional texture layers or draw calls; collision stays unchanged.
- Both recordings keep multiple timed effects visible beside combat frames. Our target previously summarized only one condition. Added simultaneous stun/root/snare/exposure badges, actual remaining timers and read-only descriptions. Player ward absorption, evasion duration and full class reserve have separate indicators.
- Dungeon movement and spell hazards remain readable around the combat HUD. The new strips sit above combat frames, and the interaction prompt moved upward to avoid them. Existing attack and movement rules are preserved.

## Remaining gaps, in priority order

1. Character silhouette, anatomy, armor construction and authored surface textures still limit realism. Lighting changes cannot replace this asset work.
2. Foliage shapes, terrain composition and repeated environment props remain visibly procedural. Improve their forms and distribution before adding indiscriminate polygon counts.
3. Expand and validate starts/stops, directional transitions and attack recovery animation against longer motion sequences; the short dungeon sample is insufficient for timing parity.
4. Two configurable action rows are implemented in the follow-up below. Richer distinct status/spell artwork, additional utility rails and encounter warnings remain separate interface work. The current pass does not recreate the entire reference interface.
5. Continue class-specific spell choreography and hazard readability with measured particle budgets. No combat choreography changes in this pass.

## Validation and limits

Release build succeeded. ClassEquipmentTests, ShadowTests, InterfaceTests, GeometryTests and ClassCombatTests pass (5/5). Visual Studio Debug test targets build; the first four suites pass (4/4). Inspected a full-scene combat capture and the compact status-tooltip capture. Regression cases cover concurrent statuses, expiry, dead targets, read-only interaction and three viewport sizes.

No measured FPS claim: the new lighting uses more shader arithmetic, although it adds no texture fetches or draw calls. Manual camera-motion checks for distant shimmer and highlight stability remain necessary. The user's running Debug game executable was left untouched; rebuild that executable after closing it to play these changes.

## Follow-up: stacked action deck — 2026-10-25

Inspected additional training-video sheets at 00:40–00:59 (inventory, parchment map, ribbon menu and settings), 02:14–02:33 (combat and action bars), and consecutive melee frames at 02:00.000–02:00.483. Revisited the dungeon frame at 02:00. This extends the sampled review; neither complete-video nor audio coverage is claimed.

The recordings show multiple compact action rows, modifier labels and utility rails, while our game had a single ten-slot spell row. Implemented a second ten-slot row with Ctrl+1–0, shared real spell cooldowns, buffered-action feedback, per-class persistence and dragging between rows. Spellbook binding labels include both rows. During a spell drag the rows rise above the build window so it cannot hide drop targets. Combat frames, cast feedback, navigation and interaction/channel prompts were repositioned to avoid the added row. XP progress is drawn above the bar frame instead of underneath it.

Compatibility: HOTBARS 5 reads all older formats atomically, preserves primary bindings and initializes migrated upper rows as empty. New layouts provide convenience shortcuts to existing techniques, signatures, setup, defense and potion actions. No new spells or fabricated multiplayer/chat widgets were added.

Remaining reference differences include the vertical utility rail, bespoke spell paintings, richer character/armor assets and full movement/combat choreography. The map and inventory shown in the reviewed frames are useful composition references but were not redesigned in this follow-up.

Validation: final Release InterfaceTests pass (12.03 s). Visual Studio Debug InterfaceTests and CharacterSelectionTests pass (30.37 s). Regression coverage includes versions 1–4 migration, truncated-load atomicity, three Ctrl variants, cross-row drag/cancel and spellbook drops at three resolutions. Inspected compact dual-row, world combat and raised spellbook-drop captures. Live acceptance is manual check 256.

## Follow-up: inventory, atlas and enemy casting — 2026-10-25

Inspected full-size training frames at 00:20, 00:41 and 00:44, and newly extracted dungeon footage at 03:30 (one inspected frame from three decoded frames). The inventory has tightly spaced icon cells and restrained frames; the atlas devotes its sidebar to live assignments rather than a large legend. The dungeon frame exposes an enemy cast countdown, health percentages and concurrent effects. These remain sampled observations, not a claim to have analyzed every frame.

Implemented a compact current-assignment card on the atlas with a real journal shortcut and smaller survey legend. Item inspection now shows the actual worn replacement in a second comparison card, including class-specific names and stats. Hovering bag equipment highlights the corresponding character-sheet slot, including empty slots; this is a usability addition informed by the equipment layout, not a behavior verified in the inspected video frames. Comparisons remain read-only and support vendor/buyback inspection too.

Target frames now include health percentage and a separate enemy strike cast strip driven by the existing windup clock. Cast strips block camera gestures only while visible. Interaction prompts were moved to avoid this strip. No enemy attack timing, equipment statistics, quest progression or save format changes in this follow-up. Manual check 257 covers live acceptance.

Validation: Release InterfaceTests/InventoryTests/TradeTests pass (12.26 s), followed by final Release InterfaceTests after removing duplicate target cast text (12.49 s). Debug all three suites pass (19.86 s). Reviewed compact comparison and status captures and the standard atlas capture. Automated checks cover comparison read-only behavior, scaled journal routing and expired cast hit regions. Live visual acceptance remains open.
