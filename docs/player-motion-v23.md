# Authored sword orientation, narrower hips and connected cloth — v23

Editable source: `assets/authored/player-motion-v23.blend`, retaining 53 Actions
and the existing 57 rest bones. Fifteen sword Actions revise carrying, eight
directional gaits, flight and four cuts. Native playback uses 59 PCL6 tracks.

## Causes and changes

- PCL5 exported the blade direction but omitted the gripping hand's roll.
  PCL6 adds the transverse blade axis. Source sampling and stance/attack
  blending interpolate the complete orthonormal frame with quaternions.
  PCL3, PCL4 and PCL5 decoding remains available with the legacy grip fallback.
- Export now retargets the upper arm and forearm separately, then applies the
  native hand's wrist-to-hilt offset. The former uniform arm/socket scaling
  changed that offset and bent the wrist after native IK. Source carry/guard
  poses are fitted to the corrected mapping. Native reach clamping waits until
  the wrist offset has been applied instead of clamping the hilt as a wrist.
- The native arm solver previously moved authored elbow poles into a preferred
  hemisphere. That could reverse the elbow during a folded sword preparation.
  PCL6 sword poses retain their authored elbow plane, including the final wrist
  offset solve. Other weapon and legacy solvers retain their previous behavior.
- The final integration also retains the parallel correction to the outer
  guard/cut blend: a common quaternion hemisphere prevents the returning
  sword from switching interpolation paths around 180 degrees. Source-key
  interpolation and authored endpoints remain unchanged. The complete motion
  checks were rerun in Release and Debug after combining these changes.
- Sword carrying and flight now rotate a connected shoulder/forearm chain.
  Cutting keys coordinate upper arm, forearm and blade. The short first cut
  begins from low carry; the prepared light cut avoids an extra wind-up that
  caused an excessive speed peak. Gameplay hit times are unchanged.
- Presentation hip roots narrow from 34 to 25 cm apart; standing ankle targets
  use the same 25 cm span. This addresses the spread of the upper legs that
  remained after v22 narrowed the forward foot tracks. Skin bind data and
  planted world contacts are retained.
- A six-link strip with a pinned shoulder band replaces the two independent
  cloak springs. Fixed 240 Hz integration, distance constraints and separate
  segment velocities produce a changing bend profile and trailing momentum.
  Rendering uses its middle and hem response, with varying folds and a closed
  cloth mesh about 3 mm thick. The local back envelope remains.
- Two existing sword blades have 320 longitudinal sections, split normals at
  the bevel/fuller's planes, shallow tooling relief and edge nicks. Each near
  LOD has 19,722 triangles (previously 16,522); distant LODs have 5,297.
  Steel roughness varies more distinctly between polished and oxidized areas.
  KIT1 occupies 31,522,284 bytes within the existing 32 MiB budget.

The oval hilt dimensions and fitted finger articulation are retained. This
revision preserves all 31 other equipment records, garment weights, all thirteen
collection models and the frozen face/scalp/eye/hair sources and shader blocks.
The original 53 channels of 44 motion tracks remain byte-identical. Within the
15 revised tracks only the right hand, elbow pole and blade direction change,
plus the new gripping-frame channels. Protected Blender curves and all rest
bones are independently compared after reopening the saved file.

In the matched 1.2–3.5 s running sequence, mean knee separation changes from
25.8 to 21.4 cm; maximum wrist-axis deviation changes from 22.4 to 4.3 degrees.
The walking comparison changes from 29.3 to 24.9 cm at the knees, while maximum
wrist deviation increases from 7.6 to 9.1 degrees. Walking therefore is not
reported as an improvement in that particular metric. The revised carry test
requires forearm/grip-axis agreement above 0.98, compared with 0.85 previously,
alongside downward/backward blade direction and actual tip clearance. These
measurements describe the reviewed sequences, not every gameplay blend.

## References and limits

Reviewed the supplied movement recording `D:\assets\what_we_have_now\01.mp4`
and extracted sequences from `D:\assets\learning_material\pve_loc.mp4` and
`character_creating.mp4`, alongside the prior raid overview. The travel footage
shows useful step/contact rhythms; the creation sequence shows a staff grip.
They do not supply an isolated sword performance or motion-capture data.

Research references:

- [Academy of Historical Fencing: holding the sword](https://swordfight.uk/hold-the-sword/)
  and [gripping the sword](https://swordfight.uk/gripping-the-sword/).
- [GDC: animation-driven locomotion](https://www.gdcvault.com/play/1015387/Animation-Driven-Locomotion-for-Smoother).
- [Blender cloth examples](https://docs.blender.org/manual/en/3.6/physics/cloth/examples.html).

No animation assets were downloaded. The cloak is local presentation cloth,
without world/leg collision or self-collision; the rendered surface interpolates
the simulated middle/hem rather than exposing every strip node. This is not
motion matching or mocap. The existing character art remains stylized, and the
revision does not claim photorealism or an exact recreation of the reference.
Movement speed, hit times, jump physics and the roadmap are unchanged by this
work. Faces remain frozen.

## Verification

Local scripts, audit reports and captures are under
`.blender-local/motion-v23-20261114/`, ignored by Git. Review uses native game
rendering, with side/front/back views, grip detail, attack readiness variants,
cloak motion, actual `Game` travel and an isolated combat fixture. It does not
use the player's saved game.

The strict attack checks retain their existing speed and wrist-alignment
limits. Cloth checks cover settling, acceleration, turning, stopping, link
stretch below 1 mm and 30/60/144 Hz agreement. Decoder checks exercise all four
archive versions and reject malformed hand frames. Full art/motion suites use
an isolated source snapshot; ordinary game executables are rebuilt and checked
separately. Final results and exact hashes are recorded in the local
`validation-summary.json`.

All twelve related suites passed in Release. Debug passed eleven in its first
run; the equipment suite hit its 45-second timeout, then passed unchanged in
44.44 seconds when run separately. Its future Debug timeout is 120 seconds;
Release retains 45 seconds. After preserving the concurrent return-cut blend
correction, the complete motion checks passed again in both configurations.
All six ordinary game executables were rebuilt, launched successfully, and
verified to embed the exact current motion, equipment and garment archives.

The final review validates 3,150 image frames, fourteen current motion traces,
three decoded 720p/30 fps videos and all local links. Actual world travel and
light/heavy combat captures use the final main Release library. The retained
v22/v23 comparison includes frame stepping and synchronized slow playback.
