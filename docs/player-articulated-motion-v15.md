# Spine, shoulder and running articulation, v15 — 2026-11-10

The native player rig previously collapsed the source's two spine segments and
clavicles into one chest transform. This prevented part of the Blender motion
from reaching the body, clothing and shoulder attachments. The rig now retains
those channels, and the running arms are timed against each leg's support and
recovery phases. Jump poses add forward trunk balance through ascent and landing.

## Source and reference scope

`assets/authored/player-motion-v15.blend` contains the accepted 57-bone rig and
all 51 actions. Sixteen directional running actions and four jumps were revised.
The export keeps all 57 runtime track identities. The original forty channels,
headers and timing of the other 37 records are byte-identical to v14; the new
spine/clavicle extension is added separately. The native rig is still a fitted
14-bone body rig, not direct playback of the complete 57-bone Blender skeleton.

Review used `D:/assets/what_we_have_now/01.mp4`, the user's images and video
sequences in `D:/assets/learning_material`, and the previous native captures.
The Allods combat footage frequently obscures the feet with effects and
equipment. It is a presentation reference, not clean motion-capture data or
proof of a one-to-one match. The study
[Control and function of arm swing in human walking and running](https://pubmed.ncbi.nlm.nih.gov/19181900/)
also informed inspection of arm/shoulder/torso coordination; its results are
not a calibration or validation of this game's animation.

## Implementation

- PCL5 adds two global spine rotations, two chest-local shoulder translations
  and an articulation flag: 53 floats per sample. The archive is 1,168,912 bytes.
  The decoder still accepts PCL3 and PCL4, rejects invalid flags and oversized
  shoulder offsets, and leaves legacy playback on the original chest transform.
- Native body joints increase from 10 to 14, retaining all original indices.
  Existing immutable body and garment weight archives are redistributed in
  memory across lower spine, upper spine and clavicles. Rigid warrior chest
  plates retain their chest binding. The palette keeps its 32-joint capacity
  for imported creatures; its row stride and human volume-preservation data
  were updated consistently on CPU and GPU.
- Chest, shoulder roots, hands, shoulder armor and the whole-head attachment
  follow the same articulated chain. Locomotion and combat layers carry the
  new channels with their existing blend weights; additional gameplay pose
  accents are distributed over the spine. Existing weapon sockets and grips
  remain in use.
- Free running arms use a continuous periodic shoulder/elbow curve timed to
  contact, toe-off and leg recovery. Chest counter-rotation and independent
  clavicle motion accompany the swing. The lateral run uses a shorter arm
  arc: diagnostics found its original revision exceeded the existing takeoff
  wrist-speed bound even during ordinary grounded continuation.
- The four jump sources revise trunk pitch and shoulder elevation. They retain
  the preceding flight-phase, contact and weapon-hand solvers.

Movement speed, gravity, jump impulse, combat timing, roadmap and mechanics are
unchanged. Faces, hair, facial materials, expression shader sections, original
body geometry and all thirteen collection assets are preserved. This revision
does not add polygons or claim photorealistic models or motion-capture fidelity.

## Review and validation

Local review: `.blender-local/motion-v15-20261110/review.html`, with synchronized
v14/v15 running, equipped running, walking, strafing and jumping comparisons,
plus actual game-update/render sequences with and without equipment. Authoring,
capture and diagnostic scripts remain ignored under `.blender-local`.

New regression checks cover independent spine/shoulder motion, shoulder-to-arm
attachment continuity, redistributed body weights and PCL5 validation/legacy
decoding. Existing support, limb length, takeoff, grip, landing and CPU/GPU
deformation checks retain their motion tolerances.

The first full run exposed two failures. The takeoff wrist-speed failure was
traced to the lateral running source and corrected there. The face test also
counted eyelid shadows on clothing as facial deformation; its fixed-camera
comparison now covers the complete head and neck, retaining the existing pixel
thresholds. Facial geometry and expression shading were not changed to satisfy
that test. Initial failed logs and the diagnostic image remain in the local
review folder.

All twelve selected motion/art suites pass in Release and Debug. One Release
equipment run hit its 45-second timeout during concurrent build/encoding; the
quiet rerun passed in 11.67 seconds. The final takeoff diagnostic stays within
its unchanged 5 mm/ms wrist bound (4.972 mm/ms). Numeric checks supplement the
visual review; they do not establish artistic realism.

Seven new videos were encoded and decoded successfully, and all 1,110 source
PNGs passed freshness, dimensions and integrity checks. Seventeen frozen source
files match their baseline hashes. The body archive and facial/scalp shader
sections also match the preserved baseline.

Ninja Release/Debug, Visual Studio Release/Debug and frontier native/debug all
embed the current PCL5 archive and pass fixture-save smoke launches. The local
`validation-summary.json` records suite results, executable/resource hashes,
source preservation, measurements and video verification. Normal play uses
`build-ninja-release/MediterraneanLauncher.exe`.

A sequential 1080p workload comparison against the preserved v14 executable
also completed. Current CPU-frame medians span 4.665–5.467 ms across four
workloads; both builds showed occasional long frames. This is a short workload
snapshot, not interactive FPS or an isolated measurement of this rig change.
Reports and executable hashes remain in the local review folder.
