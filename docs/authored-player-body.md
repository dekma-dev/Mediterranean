# Anatomical torso and limb adoption

Current class-art pass: the same preserved anatomical source now exports
18,000 / 5,000 / 1,400 body triangles. New fitted class clothing and current
review paths are described in [oldstyle-player-art.md](oldstyle-player-art.md).

The playable torso, pelvis, arms and legs now use the preserved anatomical study
instead of the runtime capsule/ellipsoid isosurface. This is one connected,
closed skinned mesh at each detail level. The existing head, hands, feet,
weapons, guardian armor and class fittings remain separate attachments.

## Source and export

The source is `assets/authored/player-anatomy.blend`, byte-identical to the local
`Human_Male_Anatomical.blend` study at inspection. SHA-256:
`bbb0590a5dd6a06c0ea20004ac5c6cdda0f3f1754dc18407dde3ed6b53625e79`.
It derives from the locally available CC0 Blender Studio realistic male; see
`assets/characters/PLAYER-ANATOMY-NOTICE.md`. No source files were edited or saved,
and no assets were downloaded. Both copies of collection01 still match
`533e343d20fc977fa38e42e9ed1abc44940435c7fe0328efadb8a7c37533b4b2`.
All thirteen collection models retain their world roles and imported guardian
armor remains equipped through its original slots.

```powershell
& 'D:/Steam/steamapps/common/Blender/blender.exe' -b assets/authored/player-anatomy.blend --python-exit-code 1 --python scripts/export-player-body.py
```

The read-only exporter evaluates one subdivision level, cuts/caps the neck,
wrists and ankles, and retargets the source anatomy to the existing ten-bone
presentation bind pose. Distance-field weights are computed offline on the
source anatomy, then interpolated during LOD reduction. Each vertex retains up
to four normalized influences. These are not manually painted production
weights or the full 26-joint authored skeleton. The source's tailored garments
are not exported by this step.

`player-body-source.json` records the source and output hashes, both bind poses,
LOD counts and constrained normal repairs at small collapsed cap faces. The
export checks manifold topology, triangle area, normal orientation and the
unchanged source hash before writing. Preserve newer user edits: compare the
tracked/local source copies before every future export.

| Detail | Vertices | Triangles | Immutable GPU bytes |
| --- | ---: | ---: | ---: |
| Near | 3,002 | 6,000 | 288,144 |
| Medium | 1,302 | 2,600 | 124,944 |
| Far | 501 | 998 | 48,048 |

The three meshes share **461,136 GPU bytes** across actors. The native archive is
403,756 bytes, embedded as resource 304 from `assets/authored/player-body.skin`.
The game does not load Blender or generate body topology at runtime. The decoder
checks the header, byte/count budgets, matching bind pose, finite fitted bounds,
normals, bone indices, weights, triangles and trailing data before GPU upload.

## Equipment and rendering

The current contact/IK and animation controller still provide joint transforms.
Gameplay collision, spell timing, class kits and saved data are unchanged.
Head placement is lowered by 55 mm to fit the new shoulders; tracking and helmet
attachments share that transform. The head/neck remains an overlap socket,
not a welded or fully skeletal neck.

A seventh material region keeps the pelvis covered by cloth when trousers are
removed. Equipped trousers use one matching hip/thigh color. The body shader
evaluates neckline, sleeve, waist and shorts boundaries in interpolated bind
coordinates; they no longer form jagged edges along LOD triangles. Forearms are
classified before the pelvis band to prevent shorts material appearing at the
wrists. Only the fitted player body uses these seams: other authored actors and
external preview meshes retain their original material assignments.

The shared palette has 263 float4 rows (4,208 bytes) per actor. Vertex and pixel
stages read the same bounded buffer. Color, shadow and portrait paths use the
same deformed geometry; world detail selection and portrait full-detail rules
remain unchanged. No extra body draw call or texture set was added.

## Evidence and acceptance

Release checks pass for `PlayerBodyTests`, `SkinningTests`, `GeometryTests`,
`ClassEquipmentTests`, `ActionClipTests`, `CharacterArtTests`,
`AttachmentLodTests`, `AuthoredLocomotionTests`, `AuthoredAssetTests` and
`ImportedBodyTests`. They cover corrupt archive rejection, palette isolation,
connected topology, identity/posed skinning, bounds, finite normals, actual GPU
material changes, class equipment, imported preview parity and LOD silhouettes.
The medium/far silhouette checks differ by 23/5,470 and 23/1,058 covered pixels;
neither has a silhouette point beyond the existing two-pixel allowance.

Ninja Debug also passes `PlayerBodyTests`, `SkinningTests`, `CharacterArtTests`,
`ImportedBodyTests` and `ClassEquipmentTests` (5/5).

`build-assets/torso-adoption/review.html` embeds unprocessed native before/after
captures using matching equipment, poses and cameras. Its baseline already has
the preceding anatomical head integration. Separate `player-body-check` views
show the new body with gear removed under portrait lighting; these are inspection
views, not comparisons with the recorded game. The legacy feet still use the
boot-shaped presentation geometry, including when footwear is removed.

Build/test, shader and smoke logs are under `build-assets/torso-adoption`.
The independent shader check matches all 13 embedded bytecodes to current
source. Native Release and Ninja Debug smoke pass startup, fullscreen restoration,
resize and world/UI rendering. Verified output paths are `build-ninja-release`
and `build-ninja-debug`; the older Visual Studio/MSBuild process-launch problem
is outside this change, and `build-local/Debug` must not be assumed current.

The hidden 1280x720 workload (`performance.txt`) reports walking CPU update p95
0.033 ms and complete CPU-frame p95 2.886 ms on this machine. The run ends at
359.6 MiB working set / 412.0 MiB private memory. This is a short synthetic
observation, not a controlled before/after performance claim or interactive FPS.

Perform manual check 291 for live motion, neck/wrist/ankle joins, extreme poses
and equipment combinations. This step does not finish reference-level character
art: authored hands/feet, tailored garment geometry, skin/hair textures, more
character identities, the 26-joint skeleton, corrective shapes and class-specific
authored combat clips remain open. The current cloth shading follows anatomical
form rather than a separately simulated garment.
