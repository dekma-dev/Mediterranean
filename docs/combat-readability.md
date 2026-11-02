# Combat readability and preparation — 2026-11-02

## Reference and visible changes

Revisited the local `allods_fight_interface.mp4` combat HUD at 00:20 and the
career frame at 02:54, together with `loc1.jpg`. Decoded the 00:20–00:20.6 range
into 36 consecutive full-resolution frames with the existing Media Foundation
decoder. This is a targeted comparison, not a new full-video or audio analysis.

The combat reference places the player and selected-target portraits beside
thin stacked bars, leaves the middle of the fight open, puts buffs underneath,
and uses a restrained text tracker over the world. The native HUD now follows
those proportions more closely:

- Player reserve is integrated beneath health, with class color, capacity
  segments and an inspection tooltip. The former separate center card is gone.
- Player and target frames sit at design coordinates (318,399) and (778,405),
  with portraits at opposite outer ends. Actual health, damage trails, reserve,
  statuses, friendly selection and the existing top layout for open panels remain.
- The objective tracker sizes to its translated text and opens the journal.
  Its backing no longer obscures a large rectangle of the landscape.
- Incoming casts retain identity-based selection and counterplay, in shorter
  rows. The world nameplate layout reserves their screen region.

World nameplates now expose health for nearby visible enemies, preparation
progress/name/time, an elite marker and a priority control-effect icon. Selection
has bracket emphasis. Up to 24 labels are prioritized by selection, casting,
engagement, distance and stable identity. Crowded labels search continuous
adjacent edges within 64 design pixels, then cull lower-priority labels instead
of overlapping. They avoid the unit frames, minimap, tracker and warning board.
Short leader lines retain the connection to displaced actors. Body picks are
ordered by camera depth, so orbiting behind an overlapping pair does not select
the actor farther from the camera merely because it is nearer to the player.

## Accepted combat clocks and effects

The player cast meter now represents ranged preparation or a real defensive
cast. It ends at the simulation's projectile-release boundary; melee swings and
post-release recovery are no longer presented as ongoing casts. Instant wards
remain instant. This changes presentation, not damage timing or ability balance.

Preparation effects use the existing live hand/weapon release socket and a
caster-relative plane. Scout/Engineer sight brackets follow facing instead of
remaining aligned to world X/Y. Mage fire has flowing counter-rotating bands,
frost has six crystalline spokes, and arcane preparation uses a broken seal.
Converging motes and a small core follow the accepted preparation phase. All
geometry remains transient and bounded; cancellation submits no residual halo.
The original model, face, skeleton, skin-weight and clip files were not edited
by this pass. Concurrent garment/renderer work in the shared workspace is separate.

## Evidence and checks

`CombatHudTests [output-directory]` renders isolated Warrior/Mage/Scout states
through the real game/DirectX renderer at 1920×1080 and 960×600. It also checks
crowded/edge labels, roster-order stability, reserved HUD space, camera-reversed
body picking, pointer routing, read-only rendering, cast clocks at 30/60/144 Hz,
instant/cancelled wards and the 512-vertex preparation budget. Existing interface
regressions retain three-resolution checks, dragging, status expiry, spellbook,
inventory, menus and incoming-caster selection.

Local evidence is under the ignored `build-combat-readability` directory:
`before`, `after`, `reference-20` and build/test logs. Before/after base views use
the same isolated character, camera and resolution. Additional `casting-*` and
`preparation-*` images start a fresh pull after loading, because loading correctly
cancels saved windups and restores campaign homes. No player save is overwritten.

Final validation: normal Ninja Release passes six targeted suites
(`LocalizationTests`, `CombatEffectsTests`, `ClassEffectsTests`, `InterfaceTests`,
`CombatHudTests`, `ClassCombatTests`); Visual Studio Debug passes `CombatHudTests`.
Both actual `RPGNative.exe` outputs pass isolated fullscreen/device/shader/resize/
world-UI smoke launches. Game and launcher targets were rebuilt in those two
configurations. The separate localization audit passes 2,006 source occurrences
and 1,865 catalog entries. Final review captures are also under
`build-ninja-release/combat-hud-review` and `build-local/combat-hud-review`.
The other build directories are not certified as current by this pass.

This pass does not establish one-to-one reference art. Portrait framing/art,
distinct spell icons, the dense career grids, full authored attack animation,
terrain/site composition and final lighting still need their own comparable
captures. The two-row hotbar layout and existing Blender model silhouettes are
retained; changing unit-frame proportions is not a replacement for that work.
