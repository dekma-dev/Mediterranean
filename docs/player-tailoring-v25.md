# Player wardrobe revision v25 — 2026-11-27

The male player warrior retains the accepted body and frozen face. This pass
rebuilds his arming garment as a continuous surface around that anatomy, instead
of the previous overlapping torso, sleeve, gusset and shoulder patches. Both
the leather brigandine and steel cuirass remain interchangeable equipment.

The existing Dawnwarden wardrobe and anatomical archive supplied the foundation.
The user-provided `D:/assets/learning_material/3.jpg` supplied the fitted clothing,
seam and layering reference. No external assets were downloaded.

## Changes and source

- `assets/authored/oldstyle-classes.blend` contains the revised doublets,
  integral collars, turned cuffs, sleeve/back seams, arming ties, leather edge
  binding, shaped skirt folds, denser pauldrons and leather pouches. The
  preceding meshes remain in the hidden `PRESERVED | before v25 tailoring`
  collection. Source materials reflect the native colors.
- `assets/authored/player-hero-v25.blend` is a separate assembled inspection
  copy: 30 posed pieces and 448,543 triangles, including the retained face,
  hands, boots, cloak and sword. Its hidden `SOURCE` collection contains
  editable bind-space garment parts. It is a static study, not a new motion rig.
- `.blender-local/player-realism-v25-20261127/review.html` compares native
  before/after captures and provides front/back movement poses. Authoring,
  diagnostic and export scripts remain ignored by Git in that directory.

The sculptable cloth receives substantially more geometry; redundant shoulder
overlays and the old disconnected neck mail are removed. The rigid breastplate
keeps its shape while the cloth underneath follows the spine and clavicles.
Nearest-surface weight transfer had assigned roughly 25% upper-arm influence
to some collar vertices. Explicit anatomical limits now keep the collar on the
chest and the cuff on the forearm. A small covered-body neckline adjustment
prevents the original anatomical cap from protruding through the collar. It is
enabled only for the equipped warrior's body under-layer; the separately drawn
head, hair, eyes and facial shaders are unchanged.

Forged steel has a broader, rougher highlight. Its shared material response also
applies to retained equipment using that material; weapon geometry and clips
are unchanged.

## Export limits

| Part | Previous near triangles | Current near | Current far |
| --- | ---: | ---: | ---: |
| Leather chest and doublet | 74,078 | 85,669 | 19,609 |
| Steel chest and doublet | 82,818 | 74,881 | 17,121 |
| Shoulder assembly | 17,352 | 36,104 | 8,295 |
| Split skirt | 33,809 | 80,903 | 19,669 |
| Belt and pouches | 10,463 | 34,204 | 8,071 |

The steel chest has fewer total triangles after removing the separate neck mail,
while its continuous fabric is denser. KIT1 contains 36,454,456 bytes; GWT2
contains 11,580,128 bytes. The complete 33-part library has explicit 40 MiB
geometry and 12 MiB weight bounds, increased from 32/10 MiB for these revised
near meshes. Per-part vertex/index bounds and both reduced LODs remain enforced.

## Preservation and verification

The local baseline records hashes before editing. Both user collection copies
matched, and the equipment source matched its export metadata. The face source,
face metadata, anatomical source/archive, both collection copies, collection
export, motion source and native clips remain byte-identical. The other 28
equipment geometry records and eight garment weight records are preserved.
Facial and scalp shader blocks are compared separately from the covered-body
fit change.

Regression checks cover collar arm-weight leakage, articulated cloth beneath
rigid armor, scope of the under-layer fit, both LODs, armhole/neckline coverage,
normalized weights and triangle normals. Native captures inspect two equipment
tiers from eight directions plus close views, and walk/run/jump/heavy-attack
poses from front and back. The local validation files retain build, test,
resource-hash and source-reopen results.

Final validation: `PlayerBodyTests`, `ImportedBodyTests`, `HumanFaceTests`,
`ClassEquipmentTests`, and `OldStyleArtTests --tailoring-review` pass in both
Release and Debug. The assembled Blender source reopens with the expected 30
pieces. All six existing game/launcher outputs were rebuilt; each game EXE
passes a hidden fixture-save smoke launch. Embedded motion, anatomy, KIT1 and
GWT2 resource hashes match the published files in all six EXEs. The comparison
retains 40 paired native views and 176 movement poses. Personal saves were not
used. These checks establish export/integration correctness, not a measured
frame-time or photorealism guarantee.

This is a garment construction and fitting revision. It does not reconstruct
the frozen face or claim photorealism. Blender study lighting approximates the
native materials; the DX11 comparisons are authoritative for in-game appearance.
