# Continuous player gait and support revision — 2026-11-10

Current editable source: `assets/authored/player-motion-v12.blend`.
The existing 57-bone rig retains 51 actions. Thirty-two directional walking and
running actions were rebaked with rear shoulder swing and contact loading;
twenty-five other native records remain byte-identical to v11. The faces,
body meshes, equipment and original thirteen-model collection are preserved.

## Observations and correction

Reviewed the chronological extracts of `D:/assets/what_we_have_now/01.mp4`
and the Allods material in `D:/assets/learning_material`, then captured v11
and the revised animation through the native renderer.

At a constant 6 m/s, v11's authored body phase varied between 0.95 and 2.42
cycles/s. Recovering that phase from the left foot alone accelerated the
torso during support, slowed it during recovery and could reverse it after
a jump. Both feet and the upper body now advance from one bilateral clock;
landing resumes at the next contact with a short upper-body crossfade.

The pelvis was also fitted to the free recovering foot. A foot behind the
body could pull the entire runner down even though it carried no weight.
Only support constrains the pelvis fully; the last part of recovery prepares
the next contact progressively. These reach weights survive takeoff, so the
first airborne frame does not suddenly reapply both leg constraints. Abrupt
starts release an overextended support leg before it exceeds its reach.

The shoulder now swings behind the body while the elbow remains flexed,
instead of keeping both wrists in front. Source pelvis compression occurs
around contact. The native foot recovery also keeps clearance between the
legs, prioritizing the planted or landing foot over the early recovering leg.

The source remains authored keyframe animation with native contact correction.
It is not motion capture or a one-to-one reconstruction of the reference.
The relationship between arm swing and locomotion was checked against
[Pontzer et al., 2009](https://pubmed.ncbi.nlm.nih.gov/19181900/) and
[Yegian et al., 2019](https://pubmed.ncbi.nlm.nih.gov/31289110/).
Gameplay movement speed, gravity, combat timing and simulation are unchanged.

## Review

Local review: `.blender-local/motion-v12-20261110/review.html`.
It includes v11/v12 comparisons from three views, normal/slow playback,
and actual Game update/render recordings with and without equipment.
The source opens its animation rig with `Run_Forward` active; the assembled
model remains in `player-hero-study.blend`.

Regression checks cover constant cadence at 30/60/144 Hz, alternating contact
phase, non-reversing phase through flight, support-only reach fitting,
foot anchors, fixed limb lengths, start/stop/reversal continuity and weapon
grips. Local `validation-summary.json` records the completed validation and
preservation checks. Authoring and review scripts remain ignored by Git.

The six motion/art suites pass in Release and Debug. All six game outputs
pass fixture-save smoke launches and embed the current clips. The review
contains seven decoded native videos and five unchanged v11 comparison videos.
