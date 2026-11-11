# Fitted sword grasp, walking support and cloth response — v21

The editable motion source is `assets/authored/player-motion-v21.blend`.
The native PCL5 library still has 57 clips. Thirty-seven existing actions were
rebaked from the preceding source; twenty complete records, every clip header
and every channel outside the stated edit set are unchanged. No face, scalp,
eye, hair, expression material or facial shader block was modified.

## Grasp and arm motion

The previous hand closure assumed a 26 mm circular handle. The existing sword
has a substantially thinner oval grip. Merely rotating that hand left an air
gap around the hilt. The new sword-specific closure fits its approximately
20 × 16 mm outer half-section, including the leather winding. The palm moves
11 mm toward the grip and 35 mm toward the guard. Arm IK uses the same offset,
so the wrist remains connected to the forearm. Fingers retain their individual
phalange lengths; the thumb opposes the closure. Staff and bow sockets retain
their existing calibration.

Sword carry is lower, the elbow pole is less flared, and the weapon follows the
transported forearm frame. Walking free-arm paths and elbow bend planes were
rebaked alongside the lower sword carry.
Existing attack trajectories retain their contact and recovery clocks, with
the new fitted grasp and revised arm bend plane throughout. This is authored
animation plus native retargeting, not captured human motion. The first strike
from relaxed carry remains limited by the unchanged 0.18/0.30 second gameplay
contacts; this pass does not change damage timing or movement rules.

## Feet and cloak

Walking and running knee planes now follow the direction of travel rather than
pointing outward. Forward walking prepares a restrained heel contact, accepts
weight, rolls through flat support and pushes from the toe. Each support pivot
stays fixed in world space. Lateral movement and running retain their respective
contact patterns rather than sharing the walking heel angle.

The cloak's separate middle and hem modes now respond to forward acceleration
and changing airflow, in addition to speed, descent and turning. Flutter is
slower. The hem rises along an arc as it trails, reducing the appearance of
cloth stretching straight backward. A smooth local back envelope restricts
forward folds into the torso. Shoulder attachment, color/shadow deformation
and normal reconstruction share the existing native pipeline. This bounded
deformation does not solve self-collision, leg contact or general cloth contact.

## Existing equipment collections

The six existing sword, bow and staff collections were edited in place:

- Swords have shorter lower hilts, flat overlapping leather strips, octagonal
  forged guards, terminating fullers and small edge irregularities.
- Bow arrow vanes have curved surfaces while retaining the arrow deformation
  category, so the feathers continue to follow release.
- Staff leather wraps are flat strips; mineral heads retain irregular planar
  facets. Wood, leather and metal use separate native surface categories.

| Collection | Previous near triangles | Current near triangles |
| --- | ---: | ---: |
| WarriorSword | 15,254 | 15,916 |
| WarriorVeteranSword | 14,776 | 15,468 |
| ScoutBow | 14,826 | 15,398 |
| ScoutVeteranBow | 14,826 | 15,398 |
| MageStaff | 13,504 | 15,356 |
| MageVeteranStaff | 15,456 | 17,308 |

The KIT1 export is 32,152,232 bytes, within the existing 32 MiB budget. Each
weapon retains a reduced distant LOD. The other 27 equipment records and the
garment weight archive are byte-identical. The thirteen user-created collection
models and their source/local copies are unchanged.

## Reference review

Chronological local sequences were reviewed from:

- `D:\assets\what_we_have_now\01.mp4`, 23.5–25.5 seconds: outward leg tracking
  and raised arms in the recorded old client.
- `D:\assets\learning_material\character_creating.mp4`, 34.5–35.7 seconds:
  visible wrapped grip and a stable relationship between hand and item.
- `D:\assets\learning_material\pve_loc.mp4`, 72–74 seconds: travel of a distant
  character. The foreground player is stationary, so this is not a clean
  close-up motion source and does not provide measurable joint trajectories.

The [Academy of Historical Fencing](https://swordfight.uk/hold-the-sword/)
describes how grasp varies with weapon and action. The correction here fits
the existing one-handed cutting sword rather than applying a universal fist.
[Autodesk's prop attachment documentation](https://help.autodesk.com/cloudhelp/ENU/GOLAEM-Maya/files/tutorials/assets-conversion-0/converting-characters/glm_adding-movable-props-character.html)
and [Epic's two-bone IK documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/two-bone-ik?application_version=4.27)
inform the shared position/orientation constraint between item, hand and arm.
Neither engine is introduced into this DirectX 11 project.

## Reproducible review and checks

Local authoring, extraction and inspection scripts remain ignored under
`.blender-local/motion-v21-20261111/`. `review.html` presents native frames from
matching command sequences and cameras, with playback, slow motion and frame
stepping. `player-motion-v21.blend` and `oldstyle-classes.blend` remain editable.

New regressions check actual skinned-hand clearance and fingertip pad contact
against the fitted oval, wrist attachment, heel/flat/toe progression and world
support at 30/60/144 Hz, and cloth acceleration response at equal travel speed.
The flat-ground legacy foot fixture was moved into the level clearing because
the concurrent location work placed banks across its previous curved route.
Its contact and frame-rate tolerances were retained.

In the captured forward run from 1.0 to 3.5 seconds, mean knee separation fell
from 32.1 to 29.0 cm while foot separation stayed at 23.3 cm. The sword elbow's
mean outward offset from its shoulder fell from 14.2 to 7.8 cm, and that hand
is about 4.4 cm lower relative to the pelvis. These are measurements of this
specific review sequence, not a general biomechanical quality score. The free
walking hand is not universally lower: its mean height rises about 1.8 cm in
the sword-walk comparison while its path and bend plane change.

Final build, runtime, preservation and native-review evidence is recorded in
the local `validation-summary.json`; source preservation includes all 57 rest
bones and every protected curve in all source actions. The visual result is
still constrained by the existing body/outfit art and frozen faces; this record
does not claim photorealism or equivalence to the reference game's production.
