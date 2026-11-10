# Native running stride and landing revision — 2026-11-10

The editable motion source is `assets/authored/player-motion-v16.blend`.
It retains the 57-bone rig and 51 actions. Sixteen directional/equipped runs,
four jumps and the heavy landing are rebaked. The remaining 36 complete PCL5
records, including their headers and all 53 channels, match the captured v15
baseline byte for byte. Facial and model sources are unchanged.

## What changed

The previous forward-running export described a 0.9333-second cycle at 1.85 m/s,
with even lower speed metadata for the other directions.
The native controller plays a 4.2-metre cycle in 0.7 seconds at 6 m/s.
Its old 0.24 support fraction required 1.008 m of travel past a fixed contact,
whereas the source stance path covered 0.54 m. The long native support left the
stance leg behind the pelvis; reach correction then forced a deeper crouch.
All 16 running actions now describe the actual native cycle and speed. Their
support fraction is 0.18, giving 0.756 m of contact travel; toe-off, recovery and
forward extension are authored against that distance. The shorter support and
revised pelvis curve change the pose; metadata alignment alone would not fix
it. Native IK still fits the recovery path between world contacts. Simulation
speed and the native stride function are unchanged.

Running pelvis loading, heel recovery, shoulder rotation and arm arcs are
revised together. The free hand uses a less compressed finger curl. Jump
recovery is more compact, and the landing's compression peak occurs later in
the existing 0.42-second presentation window. The bow retains its own free-hand
carry in flight instead of inheriting the unarmed arm swing. Weapon grips and
the gameplay jump trajectory are preserved.

In the fixed forward-run regression, maximum pelvis compression changes from
0.1530 to 0.0975 m and mean vertical offset from -0.0645 to -0.0270 m. In the
direction-reversal regression, maximum ankle displacement per millisecond
changes from 0.02179 to 0.01126 m. These are measurements of our replay, not
human motion-capture accuracy or a rating of visual realism.

The regression now requires every directional/equipped running source to
match the native stride and period. Persistent-crouch bounds are tightened,
as is equipped-flight wrist continuity (12 to 6 mm/ms). A candidate bow-hand
regression was caught at 11.6 mm/ms and corrected to 5.1 mm/ms before release.

## Reference and review

Revisited the user's `D:/assets/what_we_have_now/01.mp4` and the extracted
run/jump sequences; compared with the supplied `D:/assets/learning_material`
Allods footage and character images. Existing extractions are retained under
`.blender-local/motion-review-20261106`. The combat reference often hides feet
behind effects and equipment, so it does not establish exact joint positions.
No one-to-one reproduction or captured human motion is claimed.

Primary background material: Blender's [animation tools](https://www.blender.org/features/animation/)
and [animation teaching material](https://education.blender.org/blender_education_badges/animation/).
The numerical cycle and contact choices above come from the existing engine
and authored iteration, not a claim that those sources prescribe them.

Local review: `.blender-local/motion-v16-20261110/review.html`.
It includes the original user video, normal-speed native recordings with
front/side/back views, v15 comparisons, and actual game update/render footage
with and without equipment. Walking is retained as a control comparison.
The Blender file opens on the linked animation rig; the assembled character
mesh remains separately stored in `player-hero-study.blend`.

Verification artifacts in the same ignored directory include source and
archive hashes, frozen-source checks, per-configuration test logs, native
resource audits, runtime smoke captures and decoded-video/frame checks.
All twelve selected motion/art test suites pass in Release (347.93 seconds)
and Debug (661.84 seconds). The seven new videos decode at the expected rate
and dimensions; all 1,110 source PNGs pass freshness and integrity checks.
All six game outputs pass fixture-save smoke launches and embed the exact
current motion, equipment and garment-weight resources. The final machine-readable
record is `.blender-local/motion-v16-20261110/validation-summary.json`.
Authoring and review scripts remain outside Git tracking. No roadmap,
gameplay timing, movement constants, faces or collection geometry were changed
by this revision.
