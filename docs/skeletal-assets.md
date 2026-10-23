# Skeletal asset contract: first implementation

Checkpoint: 2026-10-23. Roadmap R2 is in progress. `SkeletonRuntime` provides a validated CPU reference and `GltfRigImport` reads a bounded rig, animation and skinned-mesh GLB subset. Both are built independently of `MediterraneanClient`; the playable character still uses the existing procedural ten-bone body and hand skinning. One indexed mesh can now be imported and deformed in the CPU reference; this does not replace visible character models.

## Implemented representation

`engine/Skeleton.h` defines named joints, parent references, local translation/quaternion/scale, a hierarchical pose and dense per-joint clip tracks. The constructor accepts child-first storage, computes a dependency order and derives inverse bind matrices from the declared rest pose unless explicit matrices are supplied. Supplied matrices must match joint count and be finite, affine and invertible within the transform budget. Evaluation outputs global, mesh-space skin and inverse-transpose normal matrices in DirectXMath row-vector convention.

The canonical space is left-handed, in metres, with Y up and +Z forward. An importer must establish its source-to-engine conversion and apply it to geometry, joint transforms, animation, winding and sockets together. The current static OBJ slot convention must not silently become the humanoid rig convention.

Current validation bounds:

- 1–64 uniquely named joints, names 1–64 bytes, exactly one root, no cycles or invalid parents.
- Finite translations within 1,000 units per component; positive scales between 0.01 and 100. Reflections and zero scale are intentionally unsupported.
- Finite quaternions whose squared length differs from one by less than 0.001; accepted values are normalized for matrix evaluation.
- Hierarchy matrix components bounded to 100 million, finite inverses and positive determinants above the singularity threshold.
- Clip duration greater than zero and at most 600 seconds, one track per joint, at most 65,536 total keys, strictly increasing finite times within the duration.

Errors throw descriptive `std::runtime_error` messages. Constructor failure cannot publish a partially initialized skeleton; pose evaluation returns a new value. The future importer must add source-file/field context to these messages and enforce byte/allocation limits before constructing vectors.

## Clip behavior

Translations and positive scales interpolate linearly; rotations normalize both endpoints before spherical interpolation and normalize the result. This prevents accepted export-rounding differences from biasing the interpolated rotation. Empty tracks retain the rest transform. A single key holds its value. Values before/after a track's keys hold the first/last key. Non-looping playback clamps to the clip duration; looping uses positive modulo, including negative preview times. At an exact loop boundary, playback returns to time zero. Authors must supply compatible loop endpoints if they want a seamless loop.

This is a canonical baked-TRS representation, not a complete glTF animation sampler. Separate LINEAR source channels are converted as described below; STEP and cubic interpolation, morph targets, root-motion extraction, animation events, blending layers and retargeting remain open. The GLB reader preserves supplied source inverse-bind matrices. If they are omitted, it explicitly supplies identity matrices, as required by glTF; it does not substitute computed rest inverses.

`Sample` currently validates the complete clip and allocates pose vectors on each invocation. Use it as an offline/preview correctness reference. Before putting it into the per-character gameplay loop, introduce immutable validated clips, reusable pose storage and profiling. The 64-joint validation ceiling is not a claim that the existing ten-bone GPU body shader accepts 64 joints.

## Validation and next integration

`SkeletonTests` contains an original two-joint hinge with child-first storage. It checks neutral deformation, a rotated mesh-space point, midpoint interpolation, loop/clamp behavior, parent propagation, normal scaling, antipodal quaternion handling and malformed hierarchy/key rejection. Existing SkinningTests, HandSkinningTests and ActionClipTests cover the unchanged current rendering path.

## GLB rig reader and inspector

`tools/GltfRigImport.cpp` consumes a byte view, never opens referenced files and never performs network access. The command-line `RigAssetInspector` checks file size before reading and prints joint count and converted global rest positions. Exit codes: 0 success, 1 invalid/unreadable asset, 2 incorrect argument count. Neither path modifies input files.

Supported subset: GLB version 2, JSON chunk followed by at most one BIN chunk, glTF asset version 2.0, one skin, 1–64 TRS nodes all belonging to that skin. Child ordering may differ from skin joint order. Optional embedded inverse binds use a tightly packed FLOAT/MAT4 accessor, with exactly one matrix per joint. Missing node names receive stable `joint_<node-index>` names. Optional skeleton root must identify the hierarchy root. FLOAT SCALAR/VEC3/VEC4 accessors are also supported for animation; all accessors share one embedded buffer and use validated, tightly packed views.

Limits: 4 MiB file, 1 MiB JSON chunk, nesting depth 24, 8,192 JSON values, 1,024 bytes per JSON string. Names remain subject to the skeleton's smaller limit. This first reader accepts ASCII strings with quote/backslash/slash escapes; Unicode and other escapes fail explicitly. It rejects duplicate keys, malformed number syntax, trailing data, non-finite matrices, overlapping parent assignments, invalid indices and misaligned/out-of-range accessor reads. Unknown fields are rejected rather than silently ignored.

Not supported: external/data-URI buffers, standalone `.gltf`, multiple meshes/primitives, scenes, materials, matrix nodes, non-joint transform nodes other than the supported identity mesh node, sparse or strided accessors, extensions, arbitrary metadata and general Blender exports. These are deliberate subset errors, not claims that the source is invalid glTF. Position Z is reflected; quaternion X/Y signs and both bind-matrix Z axes are converted consistently to the engine's left-handed row-vector convention.

The [original hinge fixture](../assets/characters/README.md) exercises hierarchy remapping and explicit bind data. Import tests cover omitted-bind semantics, quaternion conversion, malformed chunks, buffer bounds, unsupported resources and schema restrictions. The inspector also runs as a CTest integration check.

Bug-fix regression checkpoint: rounded quaternion endpoints (within the accepted normalization tolerance) must match interpolation of unit endpoints. Optional `asset.generator` and `skin.name` must be strings even though they do not affect deformation; malformed metadata cannot pass the asset validator merely because it is unused.

Specification reference: [Khronos skin schema](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/skin.schema.json), especially joint ordering and omitted inverse-bind semantics. This implementation intentionally supports a smaller subset than the standard.

## Imported LINEAR animation channels

`ImportGlbAsset` returns a skeleton and named clips. Translation, quaternion rotation and positive scale channels accept LINEAR interpolation (also the glTF default). Time accessors require matching min/max metadata, finite nonnegative strictly increasing keys, and matching output counts. Duplicate node/path channels, invalid target nodes, unsupported paths, STEP/CUBICSPLINE and unused samplers fail explicitly. Accessor bounds metadata, when present, is checked against decoded values.

Independent timelines are merged per joint at every source key, plus zero and clip duration. Each source channel holds its first/last value outside its input range; missing channels retain the rest component. Quaternion interpolation normalizes its inputs and uses slerp. The canonical clip then preserves linear/spherical interpolation between merged keys. This is offline conversion, not a per-frame file parser.

Additional budgets: 256 accessors/views, 262,144 decoded scalar components (including integer joint/index data), 16 clips, 192 channels/samplers per clip and 65,536 converted keys across the asset. Clips must have positive duration at most 600 seconds; a lone key at zero without any later channel key is currently unsupported. Empty animation arrays and duplicate clip names reject. `ImportGlbRig` remains the static-only API and rejects animated or mesh input so callers cannot silently discard data.

The inspector prints clip count, name, duration and a sampled midpoint joint position. The reproducible original [animated fixture](../assets/characters/README.md) exercises staggered translation/rotation/scale timelines and static parent inheritance. Tests check converted positions, rotation, scale, looping, malformed targets/interpolation/bounds/counts and non-finite values. No animation has been wired into the playable character yet.

Next steps: adapt a dedicated renderer path, compare CPU/GPU deformation, then author one hero and a small animation set. The fixtures are not imported playable characters.


## Indexed skinned mesh checkpoint

One optional mesh with one indexed TRIANGLES primitive is supported. A separate identity root node must reference mesh 0 and skin 0; mesh-node TRS, children, parenting and animation are rejected rather than silently applying the wrong skin space. Joint indices in vertices refer directly to skin order, not node order. The joint hierarchy may still be child-first.

Required attributes are tightly packed FLOAT/VEC3 POSITION and NORMAL, FLOAT/VEC4 WEIGHTS_0, and UNSIGNED_SHORT/VEC4 JOINTS_0. Indices are UNSIGNED_SHORT/SCALAR. Position bounds must be present and match the actual data. View/accessor offsets are four-byte aligned in this deliberately narrow subset. Attribute counts must agree. Limits: 3�16,384 vertices and at most 65,535 triangle indices, also constrained by the shared decoded-component and file budgets.

Positions must be finite within 1,000 metres per component; normals must have squared length within 0.001 of one. Four finite nonnegative weights must sum to one within 0.001; the CPU reference renormalizes accepted rounding error. Every joint index is checked, including zero-weight slots. Indices must address existing vertices, triangles cannot repeat an index, and bind triangles with squared cross-product length at or below 1e-16 are rejected. This threshold intentionally excludes near-zero-area geometry.

Conversion reflects position/normal Z and reverses each triangle's winding. `DeformMesh` is an allocating offline reference: it blends positions through the skin matrices and normals through their inverse-transpose palettes, then normalizes the resulting normal. Non-finite or collapsed results reject. It is linear blend skinning, with its usual twisting/volume limitations; it is not a new gameplay renderer or a physically exact deformation model.

The original six-vertex/four-triangle `hinge-mesh.glb` exercises fixed, mixed and fully weighted rows. Tests verify rest-pose preservation, skin-order indexing, converted winding, exact midpoint positions and inverse-transpose normal handling under nonuniform scale. Malformed mode, node transforms, attribute types/counts, weights, joints, indices and normals reject. The inspector prints mesh counts and a CPU-deformed clip midpoint vertex.

UVs, tangents, textures/materials, additional influence sets, byte/32-bit indices, normalized integer weights and general editor exports remain unsupported. No fixture geometry is connected to the player or GPU yet.

Schema reference: [Khronos mesh primitive schema](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/mesh.primitive.schema.json).
