# Authored player locomotion — 2026-10-28

The player now consumes ten additional Collection 04 Dawnwarden tracks: forward,
backward, left and right walking; the same four running directions; Start_Walk
and Stop_Walk. The previous slash, overhead, jump and landing tracks remain.
This advances the roadmap's authored-animation integration. It does not replace
the customizable player mesh or complete the full 26-joint Blender body.

## Integration

- Existing directional springs blend the four directions, including diagonals
  and reversals. Actual ground travel advances gait phase; speed smoothly blends
  walking into running between 3 and 6 m/s. Collision-limited displacement, not
  held input, controls the animation.
- Imported torso, pelvis and hand targets replace the corresponding generic
  gait channels. Start/stop interior motion is layered over the directional pose
  with zero contribution at each endpoint. Combat timing, class gestures,
  weapon forms, shield guarding and equipment ownership remain in effect.
- Grounded feet retain their world-space support anchors. Each accepted swing
  samples the imported recovery-height curve; the walk has a lower recovery and
  the run lifts the knee further. Horizontal reach and contact decisions remain
  the native solver's responsibility. This is target-based retargeting, not
  literal playback of source root translation or original footstep timing.
- Loop sampling uses periodic cubic interpolation. A new stress test exposed
  the old 240 Hz foot-pose staircase: the remaining fraction of a contact tick
  is now evaluated on a temporary swing state. Planted anchors and persistent
  contact decisions do not change during this prediction.

The old code-authored turn, reversal and airborne correction layers remain.
Enemies retain their previous gait selection, but the shared foot solver gains
the sub-tick correction. No save format, movement speed, collision rule or
damage/contact timestamp changes.

## Sources and budget

`scripts/export-player-clips.py` reads existing action JSON and skeleton files
under `.blender-local/collection-02/collection-04/animation`. Run it from the
repository root using Python with NumPy. `--source` and `--output` can select a
candidate directory. It does not open, regenerate or save a `.blend` file.

`assets/authored/player-clips.json` records hashes of all fourteen input tracks,
the skeleton and the resulting archive. The original four tracks are preserved
byte for byte. The archive grows from 12,964 to 36,832 bytes, remains embedded as
resource 303 and adds no geometry, textures, per-frame allocations or draw calls.
The decoder rejects stale track counts, corrupt loop endpoints, invalid timing,
nonfinite/out-of-range channels, invalid weapon directions and incomplete data.

## Verification and review

Release: nine focused suites pass — AuthoredLocomotion, FootPlacement, Animation,
Motion, ActionClip, ClassEquipment, AttachmentLod, Geometry and CharacterSelection.
Ninja Debug: AuthoredLocomotion, FootPlacement, Animation, ActionClip and
ClassEquipment pass. The new suite covers loop wrapping, different walk/run
recovery, 30/60/144/1000 Hz traversal, rapid start/stop/reversal, eleven class
weapon forms and unchanged gameplay snapshots.

Both Release and Ninja Debug smoke runs pass device/shader initialization,
hidden fullscreen restoration, resizing, world/UI drawing and PNG capture.

The rapid-input regression's maximum hand displacement at a 1 ms step fell from
24.1 mm before the sub-tick correction to 8.74 mm afterward. The steady lateral
traversal test measured less than 0.001 mm support slip. These are fixture
measurements, not a claim that all live movements are artifact-free.

Native comparison captures are under `build-ninja-release/authored-locomotion-check`:
`comparison.gif`, `compare-0.png` through `compare-89.png`, and eight directional
views. Left shows the former gait, right the imported gait, using identical
equipment and view settings; both use the corrected contact sampler. The clip
shows a slow start, fast travel and stop while keeping the actors centered for
inspection. It is not a side-by-side recording of the reference game.

The hidden 1280x720 workload suite reports walking CPU update p50/p95 of
0.028/0.039 ms and complete CPU-frame p50/p95 of 1.046/2.854 ms on this machine.
No tested CPU frame exceeds 16.667 ms. This is a short synthetic run, not a
before/after performance comparison or an interactive FPS guarantee. Report:
`build-ninja-release/authored-locomotion-performance.txt`.

MSBuild's Visual Studio Debug configuration currently fails while launching
`cmd.exe` for a custom build step (MSB6003, elevation required). No Windows
settings were changed. The verified current executables are in
`build-ninja-release` and `build-ninja-debug`; `build-local/Debug` is not updated
by this work. Run manual check 289 for live acceptance before closing this
animation milestone.

## Next visible work

Continue the equipment-compatible authored body and materials, then imported
class-specific weapon/casting and turn clips. The flat playable terrain, source
foot timing, full skeletal skinning and reference-level anatomy remain open.
