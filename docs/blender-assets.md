# Blender collection in the native renderer

## Accepted environment refinement (2026-10-26)

The reviewed v2 candidate is synchronized to the tracked source and local authoring copy: SHA-256 `5338426256f932d6be3db4fb83132adc9cae08cf9038bf7cf1b0713ebbb5f30c`. Original sources/exports remain in `.blender-local/environment-backup-20261026`; both original source hashes were checked before replacement.

Existing pine attachments now carry irregular needle shoots instead of opaque conical boughs. Whole shoot groups thin before distant export: 4570/1648/728 triangles. Cottage, inn and tower use object-linked weathered timber/limewash/slate/stone materials. Archive record hashes confirm all seven actors, oak and dead tree are unchanged. The archive is 12,655,740 bytes.

The reproducible exporter is `scripts/export-authored-collection.py`, with `assets/authored/collection01.sources.json`. Run Blender in background on the tracked source with `--python scripts/export-authored-collection.py`. Building/running does not require Blender. The refinement script writes a separate candidate and refuses an already-refined collection; do not rerun it on the accepted source.

## Source ownership and future use

The user designated this collection as the foundation for future model work on 2026-10-26. Develop all thirteen models rather than making duplicate primitive substitutes. The tracked source and `.blender-local/collection-01/Mediterranean_Collection_01.blend` were byte-identical at inspection (SHA-256 `6ccab74fa14ea87f9760ec0ab4e92d8ea5e939fd844d71c0fbede1cbd83d0d93`). The adjacent `.blend1` is a prior backup. Recheck for newer user edits before exporting or running generation scripts. The local contact sheet is an earlier Blender preview, not proof of current engine appearance.

| Blender model | Native role |
| --- | --- |
| Dawnwarden | Guardian / Warden Elian |
| Moonweaver | Mage / Archivist Serevan |
| Ironjaw | Orc / captains and reavers |
| Oathless | Revenant / wraiths |
| Ash sentry | Skeleton / raiders |
| Frostfang | Wolf / decorative wildlife |
| Thornback | Boar / decorative wildlife |
| Elder oak | Broadleaf trees |
| Highland pine | Conifer trees |
| Blighted elder | Dead trees |
| Hearth cottage | Homesteads |
| Moon inn | Watch hall |
| Border watch | Northern ruin tower |

The existing player is not yet replaced by this collection. Future playable-character work should adapt these characters with authored skin weights, action clips and equipment attachment support while retaining customization and gameplay systems.

The first collection contains thirteen original static models. `assets/authored/collection01.blend` preserves the editable source; `collection01.masset` embeds three detail levels per model in each native client executable. Blender and Python are not required to build or run the game; the authoring scripts remain in the locally ignored `.blender-local/collection-01` directory.

All thirteen assets now appear in the game. Cottage/inn replace homestead/watch-hall visuals; oak, pine and dead trees replace trees; the northern ruin uses the tower within its existing bounds. Warden Elian uses the guardian and Archivist Serevan uses the mage. Raiders use the skeleton, captains/reavers the orc, and wraiths the revenant. The wolf at (-18,17) and boar at (18,-18) are decorative wildlife. These are visual substitutions: existing enemy rules, names, collision, navigation, quests and saves are unchanged. The player retains the equipment-aware character renderer.

The seven actor models use a basic runtime presentation rig with smooth spatial weights at every LOD. Residents/wildlife breathe; enemies also swing their limbs from existing gait, travel and attack channels. Color and shadow passes use the same palette. These procedural motions are prototype animation, not artist-authored Blender actions or production skin weights. Animated actors must be queued each frame, outside the static-scene cache.

Frostfang and Thornback now have transient ambient behaviour (`game/Wildlife.h`): rest, roam, watch nearby players, flee close approaches and return toward their habitat. Their differing speeds/stride lengths drive the existing leg rig from actual travel. A fixed 120 Hz update and bounded steering probes use the shared map clearance; rendering does not advance behaviour. The controller has no combat, loot or saved progression and resets on world initialization, successful load and recovery/restart. It follows the live world's update policy, including the menu. Authored quadruped clips, foot planting and richer species behaviour remain future work.

## Review

Build with `scripts/build-native.ps1 -Backend Ninja -Configuration Release`. Run `build-ninja-release/MediterraneanAssetReview.exe` for the collection in the actual DirectX 11 renderer. Left/right selects a model, A/D rotates it, Space toggles the turntable, L cycles detail levels, and Escape closes the window. This viewer never opens or writes player saves.

`MediterraneanAssetReview --capture <directory>` writes thirteen native-engine PNGs without a visible window. Interactive review animates actors. `AuthoredAssetTests` validates the archive, corrupt input rejection, bind-pose identity, finite normalized influences, visible deformation, animated color/shadow rendering at all three LODs, and camera-dependent LOD on cached scenery.

## Export contract

- Coordinates are metres, Y up and +Z forward. Gallery roots, platforms, text, lights and cameras are excluded.
- Geometry is triangulated, with outward unit normals, vertex colors and a native material ID per triangle. Curve/modifier output is evaluated offline. Dense individual fur strands are omitted.
- Blender procedural shaders are approximated using the engine's existing material layers; this is not a texture bake or identical Blender shading.
- Buildings omit loose barrels and entry steps; overhangs are compressed beyond their unchanged wall footprints to fit existing camera bounds.
- Each mesh is bounded to 65,536 vertices and 20,000 triangles; the full archive is bounded to 32 MiB. `collection01.json` records actual counts, bounds and the archive SHA-256.
- Native loading validates headers, lengths, counts, coordinates, normals, materials, index ranges, triangle winding, bounds and decreasing LOD sizes before GPU upload. No paths or external textures are opened.
- GPU geometry is uploaded once. Replayed static scenery resolves its detail level against the current camera before shared color/shadow culling.

The `MDA1` archive is little endian: magic, version, asset count, LOD count (four 32-bit values), followed by asset-major/LOD-minor records. A record contains two 32-bit vertex/index counts, six float32 bounds values, ten float32 values per vertex (position, normal, RGB, integer material ID), and uint32 triangle indices. Material IDs follow `visual::Material`; vertex alpha is a material ID only for authored draw instances.

Production retopology, artist-authored skeletons/weights, facial animation, Blender action export and interchangeable equipment for these models remain future art work. The live integration uses the native procedural presentation rig and preserves the existing player customization system.

## Combat presentation channels

Defeated skeleton/orc/revenant enemies retain their authored mesh: a 1.05-second presentation collapse blends out the captured outgoing pose, then rests on the ground until looted. The renderer applies the fall to every palette joint and normal, including the root, so color and shadows agree. Settled floor clearance uses the asset bounds. Loaded dead enemies start settled; no death timer enters save data. This is a controlled fall, not a ragdoll or corpse collision solver; inspect nearby walls and other actors manually. Trainer behaviour keeps its existing representation.

Authored enemies now consume the existing smoothed action and impact channels through `game/AuthoredActorMotion.h`. Torso pitch/twist/roll, hip offset and independent arm lift/opening supplement the simple gait and weapon swing. Arm transforms inherit the torso transform; upper-body gestures leave the planted leg chains unchanged. Color and shadow rendering share the same bounded, finite palette. The game remains authoritative for accepted actions, interruption, contact and damage timing; rendering does not run another action clock. Wildlife and neutral residents retain their previous default poses.

`AuthoredAssetTests` captures neutral/action/hit views of Ash sentry, Ironjaw and Oathless in `authored-combat-check`, checks deformation at every LOD, planted lower legs and invalid-channel sanitization. This recovers presentation previously lost during model substitution; it is not a replacement for artist-authored skeletal clips or production skin weights. Inspect shoulder, cloak, shield and weapon clearance during live attacks (manual check 277).

## Collection 03 model adoption

The editable collection and embedded archive now include the locally refined Collection 03 versions of all seven actors, Elder oak and Blighted elder. Highland pine, Hearth cottage, Moon inn and Border watch retain the newer accepted environment versions; their twelve runtime LOD records are byte-identical to the preceding archive. Both editable source copies are synchronized. The previous source and exports are backed up locally under `.blender-local/collection-02/runtime-integration/before-integration`.

The Collection 03 export contained 39 meshes and occupied 13,001,708 bytes before the wolf replacement below. High-resolution source detail remains editable in Blender; runtime exports omit individual fur/hair strands, stitches, abrasion curves and disconnected chainmail rings, whose minimum geometry exceeds the native budget. Representative shader colors feed the existing native material layers. This remains an approximation of Blender shading, not a texture bake.

The reproducible export script is local and ignored: `.blender-local/collection-02/runtime-integration/export-runtime.py`. Run it with Blender in background mode against `assets/authored/collection01.blend`; it writes candidate exports into its runtime-integration directory and validates size limits before writing. `check_runtime.py` in the parent directory checks the archive and preserved environment records before adoption. Do not rerun the older exporter against the refined source without the detail exclusions.

World roles and the native procedural presentation rig remain in place. The separate Collection 03 Blender rigs and action library are not connected to gameplay by this model update. Playable characters retain their customizable equipment-aware renderer, including the separately adopted guardian cuirass; this update does not replace every player class with the guardian body.

## Frostfang anatomical rig integration

Frostfang now overrides the older wolf in `collection01.blend`. Its editable source is `assets/authored/frostfang.blend`; preserve that source when exporting the full collection. Only the wolf's three mesh records were replaced in `collection01.masset`; the other 36 records are byte-identical. The archive is now 13,327,876 bytes, with wolf LODs of 13,918 / 5,534 / 1,906 triangles. Coat vertex colors are retained; dense individual hairs and Blender procedural shading are not reproduced by the native material layer.

`assets/authored/frostfang.motion` embeds normalized four-weight skinning for all three LODs and four looping Blender clips: Idle, Walk, Trot and Gallop. Thirty joints include scapulae, carpi, hind stifles/hocks/metatarsi, toes, jaw, ears and a segmented tail. Runtime interpolation uses quaternion slerp and translation lerp, with smooth gait blending. Existing wildlife travel selects the gait and advances phase by distance actually moved. World rules, locations and saves are unchanged. This source still needs anatomical/art polish; integration does not certify photorealism, terrain IK or production-quality motion.

The native renderer supports 32 palette joints while keeping the existing ten-joint humanoid rig and 32-actor capacity. Palettes use a structured float4 buffer, shared by color and shadow shaders, rather than exceeding the DirectX 11 constant-buffer limit. The WLF1 decoder bounds sizes, counts, weights, quaternions, translations and loop endpoints, and rejects mesh/weight count mismatches.

The local ignored exporter is `.blender-local/collection-02/export_wolf_game.py`; it reads the wolf study, creates candidate files under `wolf-study/runtime`, and preserves other archive records. Previous exports are backed up there under `before-adoption`. `AuthoredAssetTests` additionally validates malformed motion data and captures all four wolf clips across LODs. Other actors still use their existing procedural presentation; their separate Blender clips are not integrated by this change.
