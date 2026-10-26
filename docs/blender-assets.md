# Blender collection in the native renderer

The first collection contains thirteen original static models. `assets/authored/collection01.blend` preserves the editable source; `collection01.masset` embeds three detail levels per model in each native client executable. Blender and Python are not required to build or run the game; the authoring scripts remain in the locally ignored `.blender-local/collection-01` directory.

The cottage and inn replace homestead/watch-hall visuals. Oak, pine and dead-tree meshes replace existing tree visuals. Placement, collision, navigation, quest state and saves still come from the existing map. The other assets are available in the native review application; characters and beasts are static studies, not rigged playable replacements.

## Review

Build with `scripts/build-native.ps1 -Backend Ninja -Configuration Release`. Run `build-ninja-release/MediterraneanAssetReview.exe` for the collection in the actual DirectX 11 renderer. Left/right selects a model, A/D rotates it, Space toggles the turntable, L cycles detail levels, and Escape closes the window. This viewer never opens or writes player saves.

`MediterraneanAssetReview --capture <directory>` writes thirteen native-engine PNGs without a visible window. `AuthoredAssetTests` validates the archive and corrupt input rejection, then checks native rendering and camera-dependent LOD on cached scenery.

## Export contract

- Coordinates are metres, Y up and +Z forward. Gallery roots, platforms, text, lights and cameras are excluded.
- Geometry is triangulated, with outward unit normals, vertex colors and a native material ID per triangle. Curve/modifier output is evaluated offline. Dense individual fur strands are omitted.
- Blender procedural shaders are approximated using the engine's existing material layers; this is not a texture bake or identical Blender shading.
- Buildings omit loose barrels and entry steps; overhangs are compressed beyond their unchanged wall footprints to fit existing camera bounds.
- Each mesh is bounded to 65,536 vertices and 20,000 triangles; the full archive is bounded to 32 MiB. `collection01.json` records actual counts, bounds and the archive SHA-256.
- Native loading validates headers, lengths, counts, coordinates, normals, materials, index ranges, triangle winding, bounds and decreasing LOD sizes before GPU upload. No paths or external textures are opened.
- GPU geometry is uploaded once. Replayed static scenery resolves its detail level against the current camera before shared color/shadow culling.

The `MDA1` archive is little endian: magic, version, asset count, LOD count (four 32-bit values), followed by asset-major/LOD-minor records. A record contains two 32-bit vertex/index counts, six float32 bounds values, ten float32 values per vertex (position, normal, RGB, integer material ID), and uint32 triangle indices. Material IDs follow `visual::Material`; vertex alpha is a material ID only for authored draw instances.

Character retopology, skeletons, skin weighting, animation export and equipment-aware gameplay replacement remain separate work. Importing these static studies does not complete those requirements.
