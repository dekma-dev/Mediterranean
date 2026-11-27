# Connected neck and head attachment — v27

The rejected v26 model left the head at its former height after changing the
shoulders. Its body export also cut the anatomical neck at 1.535 m and narrowed
that visible junction to a 4 cm radius. The separate head mesh includes its own
lower neck; rotating that entire assembly about a high pivot made the lower end
swing out through clothing.

The complete frozen head assembly is now 45 mm lower. Facial proportions,
expressions, eyes, hair and head source meshes are retained. The attachment pivot
is at the neck base (1.49 m). A single `HeadAttachmentTransform` is shared by the
skin, facial features, hair, helmet and review exporter.

The continuous body neck is restored from the existing Blender anatomical source
above the old cut. It is fitted to cross-sections of the retained head, with the
terminal cap inside that head instead of on the visible shoulder plane. This
retains the source clavicle/trapezius topology. Runtime body LODs contain
32,000 / 7,000 / 1,800 triangles and remain inside the existing 2 MiB archive limit.

A native neck joint (14) blends the body skin from the chest to the head.
The upper neck follows head rotation, while the clavicles and lower neck stay
anchored to the torso. Garment collars keep their existing chest attachment.
The previous runtime collar correction, which narrowed the body neck by 28%
and lowered it by 2 cm, is disabled for the reconstructed mesh. The normal
covered-torso fit remains active. Final-bind neck weights are locked after
surface fitting so an inward-moved vertex cannot retain upper-arm influence.
The shared GPU palette is 294 float4 rows, checked against its CPU layout;
the ten-bone source archive format and limb bind header remain unchanged.

All three class necklines have a graded front-to-back height, leaving clearance
under the mandible. Only six upper-garment records are re-exported; the other
27 equipment records and six lower-garment weight records are byte-identical.
The thirteen user-created collection sources and exports, v24 animation clips,
original anatomy and v26 review sources are preserved. No files were downloaded.

## Sources and review

- `assets/authored/player-anatomy-v27.blend`: continuous editable anatomical body.
- `assets/authored/player-hero-v27.blend`: assembled native review model.
- `assets/authored/oldstyle-classes.blend`: updated necklines, with preceding
  meshes retained in a hidden archive collection.
- `.blender-local/player-neck-v27-20261128/review.html`: native before/after
  comparisons, including diagnostic anatomy, profile and head rotations.

Authoring scripts, diagnostic iterations, captures and machine-readable checks
remain in the ignored local task directory. They are not Git-tracked.

## Verification

Final native inspection and integration checks are in progress. The regression
checks cover actual jaw-to-shoulder height, lower-neck movement during head
turns, upper-neck skinning continuity and stationary garment collars. Facial
pixel tests keep their existing limits and reframe the camera to the corrected
head attachment.
