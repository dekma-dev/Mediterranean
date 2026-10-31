# Anatomical head integration

Current class-art pass: the preserved source now exports 10,000 / 3,000 / 480
head triangles and 2,999 / 400 hair triangles. The ignored exporter is
`.blender-local/oldstyle-three-classes/export_dense_head.py`. Source anatomy and
fitting are unchanged; see [oldstyle-player-art.md](oldstyle-player-art.md).
The smaller budgets and historical review below describe the initial adoption.

The shared playable head now derives from the existing local anatomical study,
whose realistic male base is Blender Studio Human Base Meshes v1.4.1 (CC0).
`assets/characters/PLAYER-ANATOMY-NOTICE.md` records attribution and provenance.
The thirteen collection assets retain their roles; their authored guardian armor
remains attached to the player. This step replaces the procedural face, not the
whole player body, equipment or animation skeleton. The subsequent torso/limb
adoption is documented in `docs/authored-player-body.md`.

## Source and native assets

`assets/authored/player-anatomy.blend` preserves the inspected local study without
modification. Compare its SHA-256 against the local authoring copy before future
exports. It is covered by the existing Git LFS rule. The export reads the source,
evaluates one subdivision level, fits the neck to the presentation rig and emits
three normalized head LODs and two fitted scalp LODs. It never saves the source.

```powershell
& 'D:/Steam/steamapps/common/Blender/blender.exe' -b assets/authored/player-anatomy.blend --python-exit-code 1 --python scripts/export-player-head.py
```

`--python-exit-code 1` is important: Blender otherwise returns success for Python
exceptions. The exporter validates all five outputs before publishing any of
them. `player-head-source.json` records source/output hashes and the fitting
origin/scale. CMake's existing MeshAssetCompiler validates and embeds the OBJ
files. Building and playing require neither Blender nor the `.blend` source.

The head carries the nose, lips, ears, jaw and neck as one surface. Separate lip,
nostril and ear primitives were removed. Eye centres are fitted from the source;
gaze remains animated. The scalp follows the cranium with a curved hairline and
retains selected hair color. Complexion, class proportions, head tracking and
removable helmets remain shared by world, roster, creation and inventory views.

Blink and a small signed smile/pain channel deform the face on the GPU. Color and
shadow passes use the same deformation; inverse-Jacobian normal correction
follows the displacement. Inputs are sanitized. This is a procedural facial
layer over imported geometry, not an authored facial rig or lip-sync system.

The old body's neck cap was shortened during head adoption; the subsequent body
export supplies a fitted neck socket and lowers the head attachment by 55 mm.
Head/neck remains a fitted attachment, not a welded full-body mesh. Check
extreme head angles and armor collars during live play. Hair is a compact scalp
surface, not strand simulation or final hair artwork.

## Review and remaining work

`CharacterArtTests` captures the same native camera before and after adoption,
including profile, rear, helmet, complexion, hair, blink, pain, smile and extreme
head tracking. It checks visible localized blink changes, exact neutral recovery
and neutral rendering for invalid inputs. Geometry, body topology/LOD, attachment
LOD, all class equipment, action and locomotion checks cover surrounding systems.
Manual acceptance is check 290 in `tests/manual-smoke.md`.

The head/scalp LODs total 7,980 triangles and 261,600 immutable GPU bytes across
all five meshes (shared across actors), within the 320 KiB allowance. The source
study is 16,295,998 bytes but is never loaded by the game. No new texture resources
or per-frame geometry generation were introduced.

Evidence is under `build-assets/body-adoption`: `before/` retains the original
native portraits; final captures are in each Ninja build's `character-art-check`.
`review.html` is a standalone before/after comparison with eight selectable
camera/expression views and unprocessed embedded native captures.
`release-tests.log` covers seven suites, `debug-tests.log` four. The standalone
shader checker compares every embedded bytecode entry with current source; an
oversized existing literal was split to keep its MSVC token guard passing.
`smoke/world.png.log` records hidden game startup/fullscreen restoration, resize
and world/UI rendering. `performance.txt` contains the four-scene hidden workload;
it is a current-run observation, not an FPS guarantee or a controlled comparison
with the previous version. Visual Studio/MSBuild's earlier process-launch issue
was not changed; the tested outputs are `build-ninja-release` and
`build-ninja-debug`.

The subsequent body checkpoint adopts torso/limbs with offline retarget weights.
Production retopology/weight painting, interchangeable tailored garments,
hands/feet and the 26-joint authored skeleton remain unfinished. Detailed skin/eye/hair
textures and broader character identities are still needed. This checkpoint does
not establish parity with the recorded reference or complete the character-art
milestone. Continue those assets before further minor UI recoloring.
