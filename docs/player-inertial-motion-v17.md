# Continuous running support and airborne arms — 2026-11-10

The editable motion source is `assets/authored/player-motion-v17.blend`.
The 57-bone rig and 51 actions are retained. Sixteen running actions, four
jumps and the heavy landing are revised. All 36 remaining complete PCL5
records match v16 byte for byte, including headers and all 53 channels.
The nineteen frozen source assets, including the v16 motion source, are
preserved. Faces, hair, eyes, facial materials and expression shading are
unchanged. This revision changes presentation, not simulation movement,
jump physics or combat timing.

## Causes and changes

The Blender baking solver lowered the pelvis to reach both ankles, including
the airborne foot. Its frame-local reach limit also released abruptly at
toe-off. That discontinuity was baked into the pelvis and propagated to the
shoulders and wrists. The authoring solver now uses planted feet for pelvis
support; the free leg is solved within its fixed bone lengths. Before baking,
a conservative smooth support envelope anticipates contact and releases
support gradually. Merely removing the free-foot constraint was insufficient:
an intermediate candidate still failed the native reversal continuity test.

Running shoulder and elbow curves now have continuous periodic filtering and
different phase delays. The trunk counter-rotation and narrower lateral arm
arc remain coordinated with support. The accepted 4.2-metre stride, 0.7-second
cycle at 6 m/s, foot contacts and weapon grips are retained.

The four jump actions and heavy landing share the same contact pose and
recovery trajectory. Arm recoil settles after impact rather than retracting
before contact. Native arm flight no longer uses the foot-height mask that
previously pulled the hands abruptly back to idle during the final centimetres
of descent. Carry blending is applied once, avoiding the previous double
attenuation through the unarmed rest pose. A short fall captures its actual
outgoing arm frame; another takeoff during recovery also preserves that frame.
These transitions are driven by the update state, not render frequency.

In the fixed native diagnostic replay sampled at 120 Hz, forward-running
pelvis acceleration RMS changes from 92.19 to 32.44 m/s²; left-wrist RMS
changes from 96.09 to 37.04 m/s². In the moving-jump window, left-wrist peak
speed changes from 9.65 to 4.18 m/s and right-wrist peak speed from 8.77 to
5.63 m/s. Sideways running improves less: pelvis RMS changes from 84.14 to
77.28 m/s². These figures measure continuity in our replay; they are not
human motion-capture measurements or a score of realism.

New regressions bound source running acceleration and moving-jump wrist
continuity, and check that half-weight flight retains half of the outgoing
carry displacement. Existing foot-slip, equipment, short-jump and repeated-
takeoff continuity bounds remain in force. The repeated-takeoff test caught
an intermediate 231 mm wrist discontinuity; the corrected result is 0.15 mm
over its 1 ms transition. Equipped flight/landing reaches 5.06 mm/ms maximum
wrist displacement and 7.01 mm/ms at the weapon tip.

## Reference and review

Revisited the supplied Allods recordings and existing frame extractions in
`D:/assets/learning_material`, alongside the user's `what_we_have_now/01.mp4`
and v16 native comparisons. This pass extracts 180 additional full-resolution
frames at 20 Hz from `pve_loc.mp4` (36.5–39.5 and 74–77 seconds) and
`allods_dange_fight.mp4` (112–115 seconds). Effects, equipment and camera
distance often obscure feet. The footage informs posture and follow-through;
it cannot establish exact joint trajectories or a one-to-one reconstruction.

Local review: `.blender-local/motion-v17-20261110/review.html`. It contains
seven new native videos, five byte-identical v16 comparison videos and the
original user recording. Front/side/back sequences and actual game-world
recordings are included. Walking remains a control comparison. Blender opens
on the animation rig; the character mesh remains in `player-hero-study.blend`.

Authoring, extraction, measurement and review scripts remain ignored by Git.
The local `kinematics.json`, `source-solver.json`, `support-profiles.json`,
`preservation.json` and test/runtime reports record the checks and their scope.

## Completed validation

Validation completed on 2026-11-11. All twelve selected motion/art suites
pass in Release (278.35 seconds) and Debug (433.43 seconds). All six game
outputs pass fixture-save smoke launches and contain the exact current PCL5,
equipment and garment-weight resources. Body data and facial/scalp shader
blocks retain their frozen values. The seven new videos have the expected
frame counts, rates and dimensions; all 1,110 source PNGs pass freshness and
integrity checks. The review captures and numerical diagnostics use identical
joint CSV data. The final machine-readable record is
`.blender-local/motion-v17-20261110/validation-summary.json`.
