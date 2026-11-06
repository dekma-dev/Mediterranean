# Player motion review — 2026-11-06

The user's `D:/assets/what_we_have_now/01.mp4` shows folded free wrists,
persistently compressed running legs and an abrupt airborne pose. This pass
revises player locomotion and flight in Blender and their native retargeting.

## Reference inspection

Local review includes overview frames from that recording and all six videos
in `D:/assets/learning_material`, plus images 1–4. Dense chronological samples
at 20 Hz cover the current run (23–25 s), lateral movement (17–19 s), jump
(32.5–35.5 s), and the reference run (151–154 s), combat (104–106 s) and stance
(30–32 s). These are sampled sections, not a claim that every minute was viewed.
Metadata, extracted frames and comparison sheets remain under
`.blender-local/motion-review-20261106`.

## Corrected causes

- Free-hand orientation formerly used an unsigned angle to the vertical. It
  mirrored the finger direction when the forearm moved backwards. The hand now
  retains the signed forearm direction, an inward resting palm and a gradually
  closing running grip; weapon grip frames retain their existing contract.
- The reusable Blender setup exchanged toe and heel pivots. Correct pivots keep
  heel rise from forcing the source ankle below the floor and lowering the body.
- Swing velocity compensation formerly acted throughout recovery, overshooting
  the leg's reach and producing excessive hip compression. Compensation now
  acts near release/contact; the next contact is closer to the body, and forward
  running uses a narrower foot track.
- Upper-body animation previously used a distance clock that drifted relative
  to actual foot contacts. Contacts now coordinate the looping body/arm phase,
  while retaining start/stop offsets. The departing phase persists through
  takeoff and landing instead of abruptly changing when support is released.
- Revised jump/fall poses use restrained arm balance, asymmetric leg recovery,
  pre-takeoff heel rise, contact compression and settling. Sword, staff and bow
  retain their grips through these poses.

## Editable source and integration

`assets/authored/player-motion-v9.blend` retains the accepted 57-bone rig and
51 actions. Thirty-eight actions are revised: eight unarmed directional gaits,
twenty-four equipped directional gaits, four jumps and two fall/landing actions.
The preceding v8 source is retained. This is the editable motion rig/action
library; the assembled character remains in `player-hero-study.blend`. The v9
file opens with its rig linked into the scene and `Run_Forward` selected.

`player.clips` retains the PCL3 layout, all 57 track identities and its
662,544-byte size. Nineteen unrevised native records, including attacks, guards
and compatibility tracks, are byte-identical to the pre-edit archive. The export
manifest identifies the revised actions and source/export hashes. Authoring,
comparison and export scripts stay local and ignored by Git.

The original thirteen-model collection (both source copies and runtime export),
faces, clothing/equipment geometry and weights are unchanged. Movement speed,
collision, jump physics, attack contact timing and save formats are unchanged.
This pass uses authored keyframes with contact/reach constraints, not mocap or
a new rigid-body simulation. It does not claim visual parity with the reference.

## Native evidence

The local `review.html` compares the pre-edit executable against this revision
at the same real-time rate. Five 4.6-second sequences show run, equipped run,
walk, strafe and jump from front, side and back. A separate seven-second clip
uses actual `Game::ExecuteCommand` and `Game::Render` for forward movement,
jump, lateral/backward movement and stopping. Videos use 30 frames per second;
the underlying simulation is advanced at 120 Hz.

The complete Release `AuthoredLocomotionTests` run passes. Controlled measurements:

| Check | Final measurement |
| --- | ---: |
| Maximum running hip compression | 0.0902 m |
| Running foot separation | 0.2500 m |
| Free finger/forearm direction agreement | 1.000 dot product |
| Same-side arm/foot travel covariance | -0.0170 (opposite motion) |
| Maximum planted toe slip | 0.000000030 m |
| Maximum equipped jump hand displacement per 1 ms | 0.00541 m |
| Maximum equipped jump weapon-tip displacement per 1 ms | 0.01167 m |
| Maximum bow nock attachment error | 0.00280 m |

These are regression checks in controlled scenarios, not universal bounds or a
substitute for visual review. One earlier CTest invocation hit its 60-second
limit during concurrent recording; the final sequential Release CTest run
passes in 29.59 seconds without omitting checks or loosening assertions.

Final Release and Debug validation passes `AuthoredLocomotionTests`,
`AnimationTests`, `HandTests`, `FootPlacementTests`, `HumanFaceTests` and
`ClassEquipmentTests`. All six game/launcher output pairs are rebuilt; all six
game executables pass fixture-save smoke launches. Their embedded PCL3,
equipment and garment-weight hashes match the current exports. Frozen body,
face/scalp shader blocks and source hashes also pass preservation checks.
Eleven comparison/game videos decode at their expected duration, frame count
and resolution. `validation-summary.json`, `preservation.json`,
`embedded-resources.json` and `runtime-verification.json` in the local review
directory record the evidence.
