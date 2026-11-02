# Warrior, archer and mage art

The three focused classes use fitted, worn equipment through the existing native
DX11 player and inventory renderer. Scout is the archer; class IDs, item IDs,
equipment slots, combat rules and saves retain their existing meaning. Starter
and Watchguard select two different weapon meshes per class.

## Current tailoring and attack pass (2026-11-02)

Six chest variants, three whole split hems and three belt/pouch assemblies are
reworked against the user's `3.jpg` reference. The hems now carry native skin
weights and reuse the body's palette; waist anchors stay fixed and the lower
cloth follows the legs. Fine cloth surface detail and local shadow reception
replace the former corrugated appearance. Four authored sword/staff attacks
have compact preparation and longer settling within unchanged contact clocks.
Faces remain frozen. See [the revision record](class-tailoring-motion-revision.md)
for exact source preservation, archive budgets, references and limitations.
All six game/launcher outputs are synchronized and smoke-checked with fixture
saves. Thirteen distinct Release and ten Debug suites pass; the revision record
documents the Debug-only timeout correction. Native comparisons and motion are
in `.blender-local/reference-pass-20261102/review.html`.

## Previous tailoring pass (2026-10-31)

The world art expansion also updates the editable class source and both runtime
LODs of each coat panel: waists are narrower, depth folds are softer, and mage
shoulders are less wide. The same contour field is applied to source and KIT1
geometry. All 27 items, interchangeable equipment, grip sockets, body anatomy
and 54 player motion tracks are retained. The earlier source is backed up under
`.blender-local/world-art-20261031/before`; compare the source hash recorded in
`oldstyle-classes.json` before later regeneration.

## Sources and exports

- `assets/authored/oldstyle-classes.blend`: 27 editable equipment parts. The
  warrior uses the accepted Dawnwarden cuirass and shoulder meshes; Moonweaver
  garments are retained as tailoring references. Other equipment uses connected
  profiled surfaces, panels and fitted paths with seams, bindings and fasteners.
- `oldstyle-classes.kit`: bounded KIT1 archive, 27 items with two mesh LODs each,
  vertex colors and native material IDs. Resource 305 embeds it in client hosts.
- `oldstyle-classes.json`: per-item vertex/triangle counts and archive hash.
- `class-garments.weights`: bounded GWT1 skin weights for the three whole hems
  and their distance LODs, embedded as resource 306. The editable source also
  contains corresponding vertex groups and a native ten-bone bind rig.
- `player-body.skin`: 26,028 / 5,000 / 1,400 body triangles. The anatomical source
  is unchanged. Covered torso/upper-hip vertices receive a continuous local fit
  beneath the layered outfit;
  the bare body, neck and arms keep their original shape.
- `assets/characters/player-head*.obj`: 15,997 / 3,000 / 480 head triangles;
  hair uses 4,993 / 399 triangles. Source hashes remain in the provenance JSON.
- `player-boots.blend`: separate editable fitted upper and layered sole from
  the existing human study. `assets/characters/player-boot*.obj` contains
  8,000 / 1,000 triangles, with a shaped vamp, ankle, heel and darker welt.
  Worn boots use this geometry at 0.145 × 0.23 × 0.30 metres instead of the
  rounded reusable boot and duplicate flattened sole. Bare feet retain their
  separate fallback. Provenance is in `player-boot-source.json`.
- `player-motion-v5.blend`: editable 57-bone action library; `player.clips`
  contains 48 authored actions plus six compatibility tracks, totaling 399,132
  bytes. It retains the PCL2 contract and the earlier track indices.
  The v4 source is retained; fifty native records are unchanged in v5.

The anatomical source remains unchanged at SHA-256
`bbb0590a5dd6a06c0ea20004ac5c6cdda0f3f1754dc18407dde3ed6b53625e79`.
The original collection was compared, backed up and refined on 2026-10-31;
both editable copies are synchronized. See `world-art-revision.json`
and [blender-assets.md](blender-assets.md) for source hashes and geometry counts.

Authoring/export scripts, full motion studies, backups and build logs are
under the ignored `.blender-local/oldstyle-three-classes` and
`.blender-local/refinement-20261031` and `.blender-local/world-art-20261031`
directories. Compare
editable source hashes before any later regeneration to preserve user edits.
Blender is not needed to build or run the exported game.

## Equipment and motion

Warrior: arming doublet, shaped breastplate, articulated shoulder plates, sallet,
wood/iron shield and two forged swords. Archer: textile and leather jerkin, hood,
quiver, small shield and two yew/reflex bows. Mage: layered wool, hood, a split long
coat, grimoire, ward shield and two wood/iron/quartz staves. Worn metal, leather
and cloth have separate roughness layers; they use subdued colors and fine weave.

The higher-density meshes share immutable buffers. Equipment remains removable;
skin, hair and garment dye remain customizable. Cloth parts of the added
equipment follow the selected dye while leather and metal keep their authored
palettes.

V3 reduces gait bounce and adds stance loading, counterrotation and attack settle.
V4 adds relaxed weapon carrying, separate directional equipped gaits, socket-based
grip export, continuous carry/guard transitions and one-handed staff transport.
Sword/staff hand targets retain the authored paths and two-hand staff constraint.
Bow guard, draw/release, power release and equipped walk/run are separate actions.
The archer holds the bow in the right hand, draws with the left and stows an
equipped shield on the back. The native bow bends, the string draws, and the
nocked arrow releases using the same deformation in color and shadows.

Attack contacts/releases remain authoritative. Recovery outlasts the combat
timer; interrupted actions freeze and fade. Bow timing preserves the held draw
instead of skipping it through an arc-length timing map. Short existing combat
windups still limit how slowly anticipation can play without changing gameplay.

This is target retargeting to the existing ten-bone body and finger rig, with
foot IK; it is not playback of every source bone. Materials approximate the
Blender source. The three focused classes use weighted whole hems with bounded
secondary motion, not cloth simulation; other class fallbacks retain articulated
panels. These are
game assets and do not establish photorealism or final art acceptance.

## Review

Previous world/tailoring pass: eleven relevant Release suites and five Debug
suites pass. The final game launches and renders the three class fixtures;
Visual Studio Debug smoke passes with Mage. Current world captures and the
34-model gallery are under `.blender-local/world-art-20261031`; the class
equipment and motion captures remain in `build-ninja-release/oldstyle-review`.
Two startup tests that timed out during concurrent compilation passed when
repeated without the builds. The final stationary-spawn CPU p50/p95 is
1.926/2.777 ms, with 150,400,656 immutable mesh bytes; see
[blender-assets.md](blender-assets.md) for measurement scope and limitations.

Earlier validation history follows.

2026-10-31 refinement: twelve targeted Release suites and five Debug suites
pass across geometry, authored actors, body, hands, attachments, equipment,
character selection, launcher, motion and archive decoding. The final coat
clearance change was rechecked with native equipment/animation captures,
attachment LOD and authored-actor tests. Native `GeometryTests` now also passes;
the older terrain-budget failure described below belongs to the previous pass.

`build-ninja-release/oldstyle-review/weapon-carry-v4.gif` shows the three classes
at real-time 20 Hz through idle, walking, running, attacking, recovering and
returning to relaxed carrying. `weapon-carry-v4-contact.png` compares selected
moments. The images use the actual game renderer and presentation controllers.

Release smoke checks pass with all three class fixtures; Visual Studio Debug
smoke passes with Mage. The final hidden 1280x720 stationary-spawn benchmark on
RTX 4060 records CPU p50/p95 1.610/2.263 ms over 240 frames and 76,208,256
immutable mesh bytes. Only nine GPU observations were available, so no GPU-tail
or interactive-FPS claim is made. Raw output is in the ignored
`.blender-local/refinement-20261031/benchmark.txt`; player saves were not used.

Normal launch: `build-ninja-release/MediterraneanLauncher.exe` (or the
`RPGNative.exe` beside it). The same assets and animation are included in
`build-ninja-debug` and the Visual Studio `build-local/Debug` build. The launcher
starts its sibling game executable; an older build folder keeps its older art
until rebuilt. No Blender step or separate asset installation is required.

`OldStyleArtTests` validates all class-archive triangles and captures both weapon
tiers, helmets, shields, rear views and normal/heavy attacks for all three
classes. Current captures are in `build-ninja-release/oldstyle-review`;
the earlier comparison sheets and GIFs remain in
`build-oldstyle-release/oldstyle-review`.
`AuthoredLocomotionTests` checks contacts, timing, interruption, bow nock reach,
multi-refresh-rate motion and unchanged simulation snapshots. Body, hand,
equipment and attachment-LOD tests cover surrounding presentation contracts.

Validation on 2026-10-30: all 12 targeted Release and seven targeted Debug suites
pass. Debug smoke passes with the archer fixture. Release smoke
launches for all three classes pass device/shader creation, fullscreen recovery,
resizing and world/UI capture. Review files `three-classes.png`,
`basic-attacks.gif` and `heavy-attacks.gif` show native output; GIFs preserve
the 20 Hz capture timing. They are inspection sequences, not an FPS benchmark.

Main-build integration on 2026-10-30: rebuilt both Ninja configurations and
Visual Studio Debug, including their sibling launchers. Eight targeted suites
pass in `build-ninja-release` (launcher, character selection, equipment, native
art, body, authored locomotion and archive decoding); four pass in
`build-ninja-debug` (launcher, character art, body and archive decoding).
The actual Release game passes smoke startup/render checks with Warrior,
Scout and Mage fixture saves, and Visual Studio Debug passes with Scout.
These checks use build-directory fixtures, not player saves. Native world
captures are `build-ninja-release/oldstyle-review/integrated-*.png`.

The broad `GeometryTests` run also inspected the new meshes but stopped at its
existing terrain budget assertion after the concurrent map expansion. That
unrelated assertion is not counted as a passing suite above. Class geometry is
separately checked in `OldStyleArtTests`; world/roadmap work is outside this pass.

Reference research used the supplied Allods images (especially `3.jpg`), the
[WoW hunter class](https://worldofwarcraft.blizzard.com/en-gb/game/classes/hunter),
the [Met brigandine](https://www.metmuseum.org/art/collection/search/27686), and
the [Walters archer's sallet](https://art.thewalters.org/object/51.466/).
No external files were downloaded for this pass.
