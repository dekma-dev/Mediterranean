# Player recovery and contact revision — 2026-11-10

The editable source is `assets/authored/player-motion-v11.blend`. The existing
57-bone rig retains 51 actions; 32 directional gait and five jump/landing
actions were revised. The airborne fall keeps its accepted poses with the new
ankle/knee channels. The player model and frozen facial sources were not edited.

## Reference and observed problem

Reviewed the chronological extracts of the user's `D:/assets/what_we_have_now/01.mp4`,
the Allods videos under `D:/assets/learning_material`, and image `3.jpg`.
The current-game baseline for the comparison is v10, captured on November 6.
The reference gameplay is useful for timing and silhouette; its character is
often small or obscured by equipment and spell effects. This is authored motion,
not extracted motion capture or a claim of an identical reconstruction.

The old runtime discarded authored ankle orientation and knee bend direction.
It replaced ankle roll with a generic sine and capped the recovering ankle at
0.50 m. That flattened the leg even when the Blender action folded it farther.

## Changes

- Contact, compression, toe-off, heel recovery, knee advance and terminal
  extension are authored as connected poses. The heel folds behind the knee;
  the foot then extends and retracts before landing.
- Pelvis loading, chest counter-rotation and shoulder/elbow swing share the
  contact clock. Fast forward contact lands closer to the centre of mass.
- Jump poses retain ankle rotation, tuck the legs separately and extend them
  before contact. The bow's supporting-hand transition remains attached to its
  existing carry arrangement.
- PCL4 adds ankle pitch/yaw and pelvis-space knee directions to the original
  30 channels. Native IK preserves those bend planes and fixed bone lengths;
  world-space contacts remain anchored. Recovery retains live toe-off and flat
  landing orientations without an orientation jump at either endpoint.

PCL4 contains 57 tracks, 40 floats per frame and 882,704 bytes, within a 1 MiB
archive bound. The decoder accepts PCL3 with zero extension channels. All
original 30 channels in twenty records remain byte-identical.
Combat timings, actor movement, jump gravity and simulation were not changed.

The pole-direction transport follows the distinction between a limb endpoint
and its bend plane described in the [Blender IK documentation](https://docs.blender.org/manual/en/latest/animation/constraints/tracking/ik_solver.html).

## Review and validation

Local review: `.blender-local/motion-v11-20261110/review.html`.
This contains native front/side/back comparisons at normal speed and slow
motion, plus actual Game update/render sequences with and without equipment.
The Blender file opens the animation rig and Action Editor; the separate
assembled model source remains `player-hero-study.blend`.

Regression coverage includes source ankle rotation reaching the rendered pose,
recovery height, knee-direction influence without moving contacts, fixed limb
lengths, PCL3 compatibility, malformed PCL4 channels, grounded contact slip,
flight transitions and equipped grips. Motion and art checks pass in Release
and Debug (six suites each). All six game outputs embed the current clip archive
and pass fixture-save smoke launches. Build and preservation evidence is recorded in
the local `validation-summary.json`; tests establish these bounds, not visual
equivalence to the reference games.

Authoring/capture scripts and review output remain ignored under `.blender-local`.
