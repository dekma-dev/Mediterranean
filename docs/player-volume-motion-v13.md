# Articulated running, flight and body volume — 2026-11-10

Current editable motion source: `assets/authored/player-motion-v13.blend`.
The existing 57-bone source rig retains all 51 actions. Sixteen directional
running actions, four jump actions and heavy landing were revised. The native
PCL4 archive retains its 57 track identities; the other 36 records are byte
identical to v12. Source model geometry, faces and the thirteen-model collection
are preserved.

## Observation and changes

Reviewed the chronological extracts of `D:/assets/what_we_have_now/01.mp4`
and the Allods footage in `D:/assets/learning_material`, then compared the
native v12 and v13 movement with the same camera and simulation inputs.
The reference footage often obscures the character with effects or distance;
these are authored keyframes, not motion capture or a one-to-one reconstruction.

In v12 the free wrist stayed at nearly the same height through the running
cycle. The elbow was more flexed behind the body than in front. The revised
shoulder and elbow motion lets the wrist travel from the hip toward the lower
chest: approximately 70 degrees of elbow flexion behind and 105 in front.
Occupied weapon hands retain their carrying channels and grip relationships.
Heel recovery now peaks earlier behind the hip and unfolds earlier as the knee
comes forward. The v12 bilateral clock and support-weighted pelvis remain.

The jump poses reduce the shared upward curl of both feet, allowing native
departure-leg continuity to remain visible. Free arms prepare behind the torso,
swing during takeoff, then open toward landing. Weapon-specific jumps retain
their right-hand grip. Heavy landing retains its accepted precontact trajectory.
These changes do not alter movement speed, gravity, jump height or combat timing.

The native body used linear matrix blending, which loses joint volume when
neighboring bones rotate apart. Human bodies and weighted garments now use
dual-quaternion rotation/translation blending, with hemisphere alignment and
the small affine bind-length residual retained separately. This also covers
portraits and shadows. Imported collection assets and creatures retain their
existing matrix path. Separate face/scalp deformation remains unchanged.

The GPU receives the affine residual once per bone instead of recomputing four
rigid corrections for every vertex in each rendering pass. The palette grows
from 4,208 to 4,544 bytes per actor and remains a float4 structured buffer for
the native DirectX 11 pipeline. CPU reference skinning and GPU results are
checked against each other in both deformation modes.

Technical references: [Kavan et al., dual-quaternion skinning](https://users.cs.utah.edu/~ladislav/dq/index.html),
[Blender Armature Preserve Volume](https://docs.blender.org/UATEST/manual/en/dev/modeling/modifiers/deform/armature.html),
and [Yegian et al., walking/running arm posture](https://doi.org/10.1242/jeb.197228).

## Review and validation

Local review: `.blender-local/motion-v13-20261110/review.html`. It compares v12
and v13 from three views at normal or slow playback and includes actual Game
update/render sequences with and without equipment. The Blender file opens the
animation rig with `Run_Forward` active; the assembled model remains in
`player-hero-study.blend`. Authoring/review scripts remain local and Git-ignored.

New checks cover joint-volume retention, quaternion sign equivalence, affine
residual retention, CPU/GPU agreement and weighted-surface continuity at 1 ms
intervals. Existing contact, limb length, grip, head preservation and creature
checks remain in force. Local `validation-summary.json` records final results
and frozen-source hashes.

All twelve selected motion/art suites pass in both Release and Debug, with
additional final Release surface-continuity and locomotion checks. All six
game outputs pass fixture-save smoke launches and embed the current PCL4,
equipment and garment-weight resources. Seven new native videos decode at
their expected resolution, frame counts and 30 fps; five v12 comparison
videos remain byte-identical to their originals. Frozen source hashes, body
archive and face/scalp shader blocks are verified unchanged.

The hidden 1080p workload comparison on the RTX 4060 measured median CPU frame
times of 4.79–5.30 ms before and 4.70–5.45 ms after. Each of four workloads
contains 600 measured frames. This is a short diagnostic, not interactive FPS
or an isolated skinning benchmark: the builds also include concurrent interface
changes. See local `benchmark-v12.txt` and `benchmark-v13.txt` for full samples
and outliers.
