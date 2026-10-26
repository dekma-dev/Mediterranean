# Blender collection in the native renderer

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
