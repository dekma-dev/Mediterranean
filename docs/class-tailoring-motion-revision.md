# Class tailoring and strike revision — 2026-11-02

Warrior, Scout (archer) and Mage retain the current anatomical body, frozen faces,
equipment slots and combat rules. This pass revises the accepted clothing source
and four weapon actions, and makes whole coat hems deform on the existing native
body palette. It does not establish visual parity with Allods or finish the
environment and creature art milestones.

## Reference and source preservation

The local reference is `C:/Users/Хозяин/Videos/NVIDIA/Allods  Online/3.jpg`:
fitted military tunics, standing collars, front closures, pockets, restrained
equipment and long split garments. `loc1.jpg` was inspected for the broader
setting; this revision does not replace buildings, trees or creatures.

Both supplied videos were sampled at 5, 15, 30, 45, 60, 90, 105, 120, 150 and
180 seconds. The dummy sequence in `allods_fight_interface.mp4`, especially
90–120 seconds, gives rear weapon/casting views; the dungeon sequence is heavily
obscured by effects. These are visual references, not extracted motion capture
or precise biomechanical measurements. Reference frames and comparison sheets
remain in `.blender-local/reference-pass-20261102`.

The Blender manual's [cloth shape/pinning description](https://docs.blender.org/manual/en/latest/physics/cloth/settings/shape.html)
informs the separation between waist anchors and free hems. The implementation
here uses exported skin weights and bounded secondary motion, not a cloth solver.

The pass starts from the actual pre-edit `oldstyle-classes.blend` and v4 action
library. Publication checks compare source hashes before replacing derived
exports. Original parts are retained in a hidden source collection inside the
edited blend. Both copies of the thirteen-model user collection remain identical
and untouched. All 33 recorded face files, the anatomical body source, wolf motion
and environment archive retain their baseline hashes. Python authoring/export
helpers, backups and diagnostic captures remain ignored under `.blender-local`.

## Clothing and native integration

Twelve of the 27 class parts change, in both LODs: six chest variants, three
whole hems and three belt/pouch assemblies. The other fifteen parts, including
weapons, headgear, shields and shoulders, retain byte-identical KIT1 records.

Changes include fitted waists, separately layered pocket bags/flaps, seams,
closures projected onto the actual shell and flatter gusseted leather pouches.
The starter Warrior shows his textile doublet; the veteran retains the existing
authored breastplate. This is the existing equipment selection, not a new item.
Cloth weave is reduced to a fine surface scale; fold geometry supplies the larger
relief. A cloth-only shadow receiver correction removes the horizontal self-shadow
bands without changing caster geometry or frozen facial shading.

Three split hems replace the former four independently transformed leg plaques.
Waists stay attached to the pelvis. The lower cloth blends left/right thigh and
small shin influences, with restrained delayed motion toward the hem. Source
vertex groups and an editable ten-bone bind rig are in `oldstyle-classes.blend`.
The original body mesh is unchanged; a continuous covered-body fit prevents the
under-layer protruding through fitted clothing, without altering the unequipped
body, face, neck or exposed extremities.

`class-garments.weights` is embedded as resource 306. Its bounded GWT1 format
stores four bone indices and normalized weights per exported vertex, for three
parts and two LODs. The decoder rejects count mismatch, invalid joints, nonfinite
or negative weights, non-unit sums, truncation and trailing data. Garments reuse
their owner's palette in color, portraits and both shadow passes; they do not
consume another actor slot. Dye, removable equipment and distance LOD remain live.

Current exports: KIT1 **16,291,016 bytes**, **460,304 shared triangles across all
parts and both LODs**; GWT1 **1,551,016 bytes**. These are library totals, not a
per-character triangle count. Per-part geometry counts and source/export hashes
are in `assets/authored/oldstyle-classes.json`.

## Authored attacks

`player-motion-v5.blend` retains the existing 57-bone action library and replaces
Sword Attack Diagonal/Overhead and Staff Attack Thrust/Sweep. Compact preparation,
pelvis-led movement, implement lag, recoil and longer settling replace large
source swings compressed into the native contact window. Hands are solved to
the weapon grip; support feet remain grounded in authoring.

| Action | Authored duration | Contact from start |
| --- | ---: | ---: |
| Sword diagonal | 1.20 s | 0.18 s |
| Sword overhead | 1.40 s | 0.30 s |
| Staff thrust | 0.85 s | 0.09 s |
| Staff sweep | 1.20 s | 0.21 s |

Existing damage/release clocks, movement speeds, cooldowns and gameplay state
remain authoritative. A short 0.18-second contact window necessarily limits
anticipation; no slower gameplay timing is implied. Runtime still retargets to
the native ten-bone body and finger rig rather than playing all 57 source bones.
These are authored IK trajectories, not motion capture.

Only native PCL2 records 0, 1, 14 and 15 change. The remaining **50 of 54** records
are byte-identical, including directional locomotion, archery and jump/fall
tracks. The archive is **399,132 bytes**. The v4 source remains intact; the v5
source hash and per-action metadata are in `player-clips.json`.

## Validation and review

`OldStyleArtTests` now checks that weighted hems follow the legs while waist
anchors remain attached, and captures additional front/profile/isolated-shell
views. Existing equipment variants, weapon carry, attacks and armed flight are
captured through the real renderer. `ClassArtDecoderTests` covers malformed GWT1
input. Body, face, locomotion, foot contact and actor-presentation checks cover
the surrounding integration.

Final run records, preservation evidence and native review images are local in
`.blender-local/reference-pass-20261102`. Review garments through walking, running,
turning, attacks and airborne poses, including their distance LOD. The current
ten-bone deformation and rigid upper clothing still limit shoulder and extreme
pose fidelity; no full cloth simulation or finished reference-quality claim is made.
