# Original rig import fixture

`hinge-rig.glb` is a project-authored GLB 2.0 fixture, not a downloaded character. It contains two named nodes, one skin and a 128-byte embedded inverse-bind buffer. No geometry, images, clips or external resources are present.

Node 0 is `root` with child node 1. Node 1 is `hinge`, translated `(0, 1, 2)` in glTF right-handed space. Skin joint order is `[1, 0]` deliberately, exercising child-first storage and matrix/joint association. `skeleton` identifies node 0.

The single accessor is FLOAT/MAT4, count 2, with no stride or offset. Matrix 0 is identity with glTF column-major translation `(0, -1, -2)`; matrix 1 is identity. Both use little-endian IEEE 754 floats. The buffer view spans all 128 bytes. JSON is space-padded to four bytes; the binary chunk is already aligned. All node rotations/scales use glTF defaults.

After conversion, the hinge's global position is `(0, 1, -2)` and both rest-pose skin matrices are identity. Tests also construct a separate no-bind rig to verify that omitted inverse-bind matrices retain glTF's identity semantics.

Validate without changing the source:

```powershell
.\build-ninja-release\RigAssetInspector.exe assets\characters\hinge-rig.glb
```

The inspector is for the narrow skeletal subset documented in [skeletal assets](../../docs/skeletal-assets.md). A normal full character export is not yet supported.

## Animated companion

`hinge-motion.glb` adds one original `hinge_motion` clip to the same rig. Regenerate it with `scripts/make-motion-fixture.ps1`; the script uses only local .NET JSON/binary writers and overwrites only the selected named project fixture.

Rotation keys at 0/1 seconds turn the hinge 90 degrees around source +Y. Translation keys at 0.25/0.75 seconds move from `(0,1,2)` to `(2,1,4)` with endpoint holds. Scale keys at 0/0.5/1 seconds go from `(1,1,1)` to `(2,3,4)` and back. This intentionally exercises three different source timelines. The converted track has keys at 0, 0.25, 0.5, 0.75 and 1 seconds; the root retains its rest transform. At 0.5 seconds the hinge position is `(1,1,-3)` in engine space, with a 45-degree converted rotation and nonuniform scale.

Run `RigAssetInspector.exe assets/characters/hinge-motion.glb` to inspect the imported clip. This fixture has no mesh and is not connected to the player's animation controller.


## Weighted mesh companion

`hinge-mesh.glb` adds an original six-vertex, four-triangle strip to the animated rig. Regenerate with `scripts/make-motion-fixture.ps1 -Mesh`. No third-party models, textures or downloads are involved. Node 2 is an identity mesh node referencing skin 0; the joints remain in deliberately reversed skin order.

In source coordinates the strip spans X = ±0.25, Y = 0/1/2, Z = 2, with +Z normals. The bottom row belongs to root, the middle row blends root/hinge equally, and the top row belongs to hinge. At 0.5 seconds the last vertex is approximately `(1.353553, 4, -2.646447)` in engine space. Rest-pose deformation preserves all six converted source positions.

`RigAssetInspector.exe assets/characters/hinge-mesh.glb` reports counts and a numerically deformed midpoint. This is a CPU validation fixture, not a replacement character model. GPU deformation and visual acceptance remain pending.
