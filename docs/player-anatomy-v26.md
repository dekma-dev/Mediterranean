# Anatomical shoulder refit — v26

The former body export widened the realistic male chest by 35%, its limbs by
25%, and moved the shoulder joints from 41 cm to 61 cm apart. Clothing inherited
this distortion. The v25 doublet also raised its yoke to satisfy coverage probes
outside a natural shoulder silhouette. More triangles did not correct that shape.

This revision uses the existing Blender Studio anatomical source and the user's
five anatomy/topology references. The shoulder joints are now 45 cm apart; the
chest uses 1.08 lateral / 1.04 depth scale and upper limbs use 1.04 radial scale.
The acromion-to-neck slope is lowered locally, medial neck weights exclude arm
influence, and the hands translate with the corrected shoulder origins. Weapon
orientation, grip constraints, attack timing and all authored clip bytes remain
unchanged.

The warrior doublet is rebuilt as a continuous surface over that body, with an
integral low standing collar, sleeve seams and restrained compression folds.
Both armor tiers retain their existing breastplates and harness. All three
focused classes have refitted upper garments and shallower shoulder pieces;
24 other equipment records remain byte-identical. Bracer placement fits the
new forearm volume. Class ownership, dyeing and interchangeable equipment remain
in the native DirectX 11 pipeline.

## Sources and review

- `assets/authored/player-anatomy.blend`: retained upstream anatomical study;
  its contents are unchanged.
- `assets/authored/player-anatomy-v26.blend`: new dense editable body and native
  bind geometry. Upstream objects are retained hidden.
- `assets/authored/player-body.skin`: 31,998 / 7,000 / 1,800 triangles across
  the three runtime LODs, within the existing 2 MiB body archive budget.
- `assets/authored/oldstyle-classes.blend`: refitted wearable collections;
  preceding meshes and armature are retained in a hidden archive collection.
- `assets/authored/player-hero-v26.blend`: assembled native posed review study.
- `.blender-local/player-anatomy-v26-20261127/review.html`: local native images
  and motion inspection. Authoring scripts, logs and baselines stay untracked.

The face freeze remains in force: facial source/export data, head and scalp,
eyes, hair, facial materials and shader branches are preserved. The thirteen
user-created collection assets and v24 animation archives are unchanged.

The modeling basis follows the existing [Blender Human Base Meshes bundle](https://developer.blender.org/docs/release_notes/4.0/asset_bundles/).
The weight review follows the [Blender armature modifier documentation](https://docs.blender.org/manual/en/5.1/modeling/modifiers/deform/armature.html):
volume preservation does not replace correct joint placement and vertex groups.
No new third-party files were downloaded.

## Verification

Integration and verification completed on 2026-11-28:

- All 13 relevant suites pass in both Release and Debug, including body archives,
  garment coverage, CPU/GPU skinning, hand placement, authored locomotion and
  animation transitions. Five asset/render suites were repeated successfully
  after the final covered-body shader adjustment.
- All six existing native game builds contain the current body, equipment,
  garment weights and preserved animation resources. Each executable launched
  successfully against an isolated test save; its embedded resource hashes were
  checked against the published files.
- The native review includes 50 still views and 528 motion frames, encoded as
  eight 30 fps videos covering walking, running, jumping and attacks from front
  and back. The Blender review file reopens with 30 visible parts and 458,968
  triangles. These are actual game-renderer captures, not substitute renders.
- The bow's shared string contact fades out the independent shoulder-width
  correction. This removes the elbow-speed regression found when translating
  both hands inward by a fixed distance; the original motion limits still pass.
- Preservation checks confirm unchanged facial data/shader branches, thirteen
  collection assets, upstream anatomy, authored clips, 24 other equipment
  records and six lower-garment weight records. No authoring script is tracked.

Machine-readable results, executable/resource hashes and review manifests are
in `.blender-local/player-anatomy-v26-20261127/validation-summary.json` and its
companion reports. The captured improvement is the shoulder/body silhouette
and equipment fit; existing facial styling and material limitations remain.
