# Authoring static equipment meshes

Skeletal pipeline checkpoint (2026-10-23): [skeletal asset contract](skeletal-assets.md) documents the validated hierarchy/clip CPU reference and bounded GLB rig/LINEAR-animation/indexed-skinned-mesh reader and inspector. CPU deformation is validated; GPU integration remains open; this does not add rig support to the OBJ importer below.

Class weapon extension (2026-10-22): the Warrior blade below remains the editable OBJ example. The other ten class weapon meshes are original code-authored geometry in `engine/Geometry.cpp` (`Equipment`), selected through `game/WeaponVisual.h`. They use grip-local metres: +Y along the haft, +Z along the launcher barrel/bolt rail, grip at the origin. Keep the shaft inside the tested 25.6 mm radius envelope through the hand; launcher release sockets are shared with `HandPose.cpp`. Changes must pass ClassEquipmentTests, GeometryTests and ActionClipTests. Weapon meshes are generated once at startup, not per frame. Their separate budget and current costs are recorded in `graphics.md`; skeletal/animation import remains open.

The first asset pipeline imports a triangulated OBJ during the native build and embeds its vertices and indices in the executable. The game still uses our C++ / DirectX 11 renderer. No model editor, library or asset is downloaded by the build.

## Edit the blade

The editable source is [assets/weapons/frontier-blade.obj](../assets/weapons/frontier-blade.obj). It is original project geometry: a tapered blade with bevels and a recessed centre groove. It replaces the previous procedural blade for all items using that mesh slot, including world characters and portraits. The handle, guard, weapon sockets and combat-effect centreline keep their existing transforms.

1. Import the OBJ into a model editor, or edit its text directly. Keep a copy inside the project while trying a different shape.
2. Preserve the local coordinate contract: **Y runs from root to tip**, root at `(0, -0.5, 0)`, tip at `(0, 0.5, 0)`; X is width and Z is thickness. Every coordinate must stay within `[-0.5, 0.5]` (the compiler allows only a small rounding tolerance). Do not recentre or independently normalize the exported mesh.
3. Apply object transforms, triangulate the surface and recalculate outward normals. Export the selected blade only. Preserve its axes and scale on both import and export. Export normals; turn off material/MTL export. Smoothing groups are used when normals are absent. UV coordinates can be present but are currently discarded.
4. Save the result over `assets/weapons/frontier-blade.obj`, then build from the repository root:

   ```powershell
   .\scripts\build-native.ps1 -Backend Ninja -Configuration Debug -Test
   ```

5. Launch `build-ninja-debug/RPGNative.exe`. Inspect the sword at rest, during Strike/Heavy, jumping, turning and in the character panel. Check both sides, its shadow and alignment with the guard. Manual check 173 covers this workflow.

For Visual Studio solution users, rebuild `build-local/RPGNative.sln` in Debug/x64 and then press F5. Folder/CMake users can build the `RPGNative` target as usual. The generated header belongs to that build directory and configuration; never edit it manually.

Blender is one optional authoring tool. Its [OBJ documentation](https://docs.blender.org/manual/en/latest/files/import_export/obj.html) describes normals, axis conversion and triangulation options. Match the Y-up/Z-forward file convention on import and export, then verify the resulting coordinates against the contract above. This milestone validates OBJ files and native builds; it has **not** validated a round trip through an installed Blender version. No Blender installation is required to build the supplied asset.

The world blade transform currently uses scale `(0.12, 0.86, 0.07)` and a centre offset of `0.53 m` along the weapon frame. These values are in `game/CharacterView.cpp`. Modelling inside the normalized slot preserves attachment locations; changing that contract requires a coordinated change to the weapon/effect code and tests.

## Supported data and limits

| Data | Current behavior |
| --- | --- |
| Positions | `v x y z`; optional `r g b` vertex colors in `[0, 1]`, default white |
| Triangles | `f` with exactly three corners; positive or relative negative indices |
| Normals | Explicit `vn` values are normalized; missing normals are generated |
| Smoothing | `s off`, `s 0`, `s on` or a positive group number; hard seams retain separate vertices |
| UV coordinates | `vt u v [w]` references are validated, then discarded |
| Names and comments | `o`, `g`, `#` comments, CRLF and a UTF-8 BOM are accepted |
| Materials | `mtllib` and `usemtl` are rejected; the engine supplies the existing procedural metal material |
| Other data | Quads/ngons, curves, scene transforms, rigs, skin weights and animation are unsupported |

Per equipment asset, the compiler permits at most 2 MiB of OBJ text, 16,384 entries in each source position/normal/UV array, 4,096 triangles and 8,192 final render vertices. Lines are limited to 8,192 bytes. Explicit normals and smoothing seams can increase the render-vertex count. The supplied blade uses 62 source positions, 168 render vertices and 120 triangles. Its regression fixture has a tighter 256-triangle budget; update that deliberately if the art needs more detail. The mesh-library budget is also checked independently.

An invalid index, non-finite coordinate, degenerate triangle, opposing normal or exceeded budget fails the build with an `OBJ line N:` diagnostic. Disable materials or triangulate in the exporter when the error asks for it. Successful output is deterministic, and the compiler replaces the generated header only after validation and a complete write. A failed import leaves the previous header and executable on disk: **fix the source and obtain a successful build before testing**, so you do not mistake an older executable for the new art.

## Build and runtime behavior

`tools/ObjImport.cpp` is the bounded parser. `MeshAssetCompiler` writes `generated/<configuration>/Blade.generated.h`; `engine/Geometry.cpp` consumes that data for `Mesh::Blade`. CMake tracks both the source OBJ and compiler target, and one `EmbeddedMeshes` target owns generation for the client and standalone geometry tests. No OBJ parser or source-file lookup is used during gameplay. Ordinary builds leave source assets unchanged.

The current renderer uses position, normal and vertex color, with its procedural material path. Importing UVs does not enable authored texture maps. Adding another mesh currently requires an explicit CMake generation rule, a renderer mesh slot and an integration test; this is not automatic directory discovery or an in-game import browser.

`ObjImportTests` checks syntax, normal seams, malformed input, budgets, exact equality between the source and embedded geometry, blade closure/winding and attachment bounds. It also renders isolated and equipped blade views. `MeshAssetCompilerTests` checks deterministic output, paths with spaces, source-file protection, failed-import preservation and recovery. Existing geometry, hand, character, shadow and interface suites exercise the integrated renderer.

Next asset milestones are a skeletal/animation format, rig and skin-weight validation, authored anatomy/clothing/hair, UV/PBR material support and more general attachment fitting. The static blade pipeline does not complete those tasks.
