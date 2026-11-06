# Player recovery and arm articulation — 2026-11-06

The follow-up to the [v9 review](player-motion-review-20261106.md) addresses
motion that still looked constrained after correcting the wrist orientation.
The supplied `D:/assets/what_we_have_now/01.mp4` and learning-material files have
the same timestamps as in the preceding review. Existing local frame sequences
were inspected again; comparison captures use v9 as their immediate baseline.

## What changes

The previous Blender gait placed wrists on Cartesian sine paths. V10 constructs
the free arm from shoulder rotation and elbow flexion, then bakes the resulting
joint transforms. Walking uses a low, relaxed arm; running bends the elbow and
swings from the shoulder. The source elbow plane accompanies each wrist target.
Free running fingers close into a loose fist; existing equipped grips and cast
opening remain independently controlled.

Native foot placement previously imported only ankle height. It replaced all
forward/backward recovery with smoothstep interpolation. `SwingProgress` now
projects the authored ankle trajectory between its release and landing poses
and fits that progress to the live world-space endpoints. Early heel recovery,
forward extension and pre-contact retraction therefore survive export.
Live contact anchors, terrain height and final leg reach remain authoritative.
The source recovery curve has also been reauthored together with the arms.

Joint timing during flight previously used spring-filtered vertical speed,
which could initially move backwards through the clip and lag the actual apex.
It now follows the existing ballistic velocity. The filtered signal remains
available for secondary body motion. A moving jump retains the leading leg
from its departing stride, then blends back to authored extension before
contact. This is an additive presentation adjustment; jump speed, gravity,
travel, collision and combat timing are unchanged.

## Assets and preservation

`assets/authored/player-motion-v10.blend` contains the retained 57-bone rig and
51 actions, with 32 directional walking/running actions revised for unarmed,
sword, staff and bow movement. Nineteen other source actions are retained.
The v9 source remains available. The final v10 file opens with the rig linked
to its scene and the Action Editor set to the forward run.

`player.clips` keeps PCL3, 57 track identities and 662,544 bytes. Twenty-five
native records are byte-identical to the v9 baseline, including flight/landing,
attacks and compatibility records. Existing jump poses are replayed with the
corrected native clock rather than reauthored unnecessarily.

Faces, head/scalp meshes, hair, eyes, facial materials, the original thirteen
models, clothing/equipment geometry and garment weights are preserved.
All authoring, backup and review scripts remain ignored under
`.blender-local/motion-v10-20261106`.

## Review and checks

Local `review.html` compares v9 against v10 with synchronized video playback,
scrubbing and slow motion. Native recordings show front, side and rear views
for running, walking, strafing, equipped running and jumping. A separate clip
uses the actual game update/render loop. All playback defaults to real time.

New regression checks ensure native recovery differs from a generic smoothstep
path while keeping its contact endpoints, jump playback advances monotonically
at 30/60/144 Hz, and opposite departing strides produce opposite leading legs
in flight. Existing checks continue to constrain limb lengths, contact slip,
grips, weapon clearance, transition continuity and simulation/save equivalence.
Final suite results, preserved hashes, executable hashes and decoded video
metadata are recorded in the local review directory.

Final validation passes all six selected suites in Release and Debug:
`AuthoredLocomotionTests`, `AnimationTests`, `HandTests`, `FootPlacementTests`,
`HumanFaceTests` and `ClassEquipmentTests`. All six game/launcher pairs are
rebuilt; each game executable passes a fixture-save smoke launch and embeds
the current PCL3, equipment and garment-weight resources. Twelve videos decode
with the expected 30 fps timing and 1280×720 dimensions. The two world clips
show equipped and unequipped movement through the same actual game loop.

The final contact test measures less than 0.000001 m planted toe slip. At 1 ms
sampling, the equipped-flight test measures maximum hand and weapon-tip
displacements of 0.00538 m and 0.01154 m. These are controlled regression
measurements; visual review remains necessary. A trailing-foot clearance issue
found during development was corrected without weakening its floor assertion.

This remains authored keyframe animation with native contact and reach
constraints. The reference files contain visual targets, not imported motion
capture; visual parity with them is not claimed.
