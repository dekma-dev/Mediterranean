# Running support, recovery arcs and jump handoff — 2026-11-11

The editable source is `assets/authored/player-motion-v18.blend`: 57 bones,
51 actions, with sixteen runs, four jumps and the heavy landing revised.
All 36 other complete PCL5 records are preserved byte for byte against v17.
Twenty frozen sources include the previous motion source and character,
equipment and collection assets. Facial meshes, eyes, hair, materials and
expression shading remain frozen. Movement speed, jump impulse, gravity and
combat timing are unchanged.

## Changes and reasons

The source previously eased to zero velocity at every intermediate recovery
pose. Shape-preserving cubic curves now pass through intermediate poses
without those repeated stops. The forward run retains its 0.7-second cycle
and 4.2-metre stride at 6 m/s, with support extended from 18% to 20% of the
cycle. Sideways/backward runs retain 18%. Native contact timing now blends
the matching directional metadata, instead of taking only the forward value.

Forward foot contact is closer beneath the body. A small horizontal pelvis
transfer moves body weight over the support foot. An offline periodic
optimization smooths the pelvis trajectory under both Blender and native
leg-reach limits; it anticipates load instead of waiting for the native rig
to pull the pelvis down. Earlier candidates failed the existing persistent-
crouch test and were rejected. Shoulder and elbow curves use separate phase
delays; leg recovery is more compact. Foot anchors retain world contact.

Jump keys now use continuous trajectories through ascent, descent and
landing. Reduced leading-leg offsets retain takeoff asymmetry without the
previous exaggerated airborne running pose. Native arm handoff lasts 220 ms
instead of 140 ms. Extended falls retain their existing fall-arm pose through
ground contact. Equipped grips and hand-to-weapon attachment remain active.

A new regression checks support timing in all four running directions.
The leading-leg regression now requires 10–18 cm of separation, matching
the deliberately compact pose. Existing crouch, foot-slip and discontinuity
limits remain unchanged.

## Measurements and limits

The same native replay is sampled at 120 Hz before and after the change.
Sideways pelvis acceleration RMS falls from 77.28 to 43.53 m/s²; wrist RMS
falls from 77.60/77.07 to 45.39/45.27 m/s². In the moving-jump window,
left/right wrist peak speed falls from 4.18/5.63 to 3.29/4.16 m/s. The
independent 1 ms moving-jump check falls from 5.47 to 4.08 mm of wrist travel.

Forward-running pelvis RMS is essentially unchanged (32.44 to 32.60 m/s²).
Free-wrist RMS rises slightly; equipped right-wrist RMS rises from 106.37
to 136.10 m/s². The worst reversal hand step also rises, from 8.06 to
11.88 mm/ms, within the existing limit. These are limitations, not universal
smoothness gains. Pose, support and jump changes should be judged in motion;
these replay measurements are neither human mocap nor a realism score.

## Reference and review

Reviewed existing extracts from the user's recordings in
`D:/assets/learning_material`, including location, raid, combat/interface
and character-creation sequences. Combat effects, mounts and camera distance
obscure many foot contacts; character creation shows equipment posture more
clearly than running. These references inform balance, silhouette and grip,
but do not establish exact joint trajectories or a one-to-one reconstruction.
The [Blender walk-cycle tutorial](https://docs.blender.org/api/htmlI/x8053.html)
also describes contact, recoil and weight transfer as distinct poses.

Local comparison: `.blender-local/motion-v18-20261111/review.html`.
Authoring and review scripts remain ignored by Git. Blender opens the motion
rig; the unchanged character mesh remains in `player-hero-study.blend`.
The comparison includes front/side/back sequences and actual game-world
captures, with walking retained as a control.

## Completed validation

All twelve selected motion/art suites pass in Release (234.64 seconds) and
Debug (386.77 seconds). All six game outputs pass fixture-save smoke launches
and embed the exact current PCL5, equipment and garment-weight resources.
Frozen body data and facial/scalp shader blocks retain their checked values.
All 1,110 PNGs are fresh, valid and complete; seven new videos decode with
the expected frame counts, rates and dimensions. Five v17 comparison videos
remain byte-identical to the baseline. Captured and diagnostic joint CSVs
match exactly. Twenty frozen sources and 36 complete native records pass the
preservation audit. The machine-readable results are recorded locally in
`.blender-local/motion-v18-20261111/validation-summary.json`.
