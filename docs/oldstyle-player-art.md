# Warrior, archer and mage art

The three focused classes use fitted, worn equipment through the existing native
DX11 player and inventory renderer. Scout is the archer; class IDs, item IDs,
equipment slots, combat rules and saves retain their existing meaning. Starter
and Watchguard select two different weapon meshes per class.

## Sources and exports

- `assets/authored/oldstyle-classes.blend`: 27 editable equipment parts. The
  warrior uses the accepted Dawnwarden cuirass and shoulder meshes; Moonweaver
  garments are retained as tailoring references. Other equipment uses connected
  profiled surfaces, panels and fitted paths with seams, bindings and fasteners.
- `oldstyle-classes.kit`: bounded KIT1 archive, 27 items with two mesh LODs each,
  vertex colors and native material IDs. Resource 305 embeds it in client hosts.
- `oldstyle-classes.json`: per-item vertex/triangle counts and archive hash.
- `player-body.skin`: 18,000 / 5,000 / 1,400 body triangles. The anatomical source
  is unchanged. Covered torso vertices receive a local fit beneath the doublet;
  the bare body, neck and arms keep their original shape.
- `assets/characters/player-head*.obj`: 10,000 / 3,000 / 480 head triangles;
  hair uses 2,999 / 400 triangles. Source hashes remain in the provenance JSON.
- `player-motion-v3.blend`: editable 57-bone action library; `player.clips`
  contains 27 authored actions plus six compatibility tracks, totaling 275,940
  bytes. It retains the PCL2 contract and the earlier track indices.

The original collection and anatomical source were compared to their local
authoring copies before export and were not overwritten. Their SHA-256 values
are respectively `533e343d20fc977fa38e42e9ed1abc44940435c7fe0328efadb8a7c37533b4b2`
and `bbb0590a5dd6a06c0ea20004ac5c6cdda0f3f1754dc18407dde3ed6b53625e79`.

All new authoring/export scripts, full motion study, backups and build logs are
under the ignored `.blender-local/oldstyle-three-classes` directory. Compare
editable source hashes before any later regeneration to preserve user edits.
Blender is not needed to build or run the exported game.

## Equipment and motion

Warrior: arming doublet, shaped breastplate, articulated shoulder plates, sallet,
wood/iron shield and two forged swords. Archer: textile and leather jerkin, hood,
quiver, small shield and two yew/reflex bows. Mage: layered wool, hood, split coat
panels, grimoire, ward shield and two wood/iron/quartz staves. Worn metal, leather
and cloth have separate roughness layers; they use subdued colors and fine weave.

The higher-density meshes share immutable buffers. Equipment remains removable;
skin, hair and garment dye remain customizable. Cloth parts of the added
equipment follow the selected dye while leather and metal keep their authored
palettes.

V3 reduces gait bounce and adds stance loading, counterrotation and attack settle.
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
Blender source. Coats use articulated panels, not cloth simulation. These are
game assets and do not establish photorealism or final art acceptance.

## Review

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
