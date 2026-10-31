# Authored player locomotion — v3 class equipment, 2026-10-30

The current archive is v3: 27 authored actions plus six retained compatibility
tracks, 275,940 bytes. It adds bow guard, two releases, equipped bow walk/run,
and a weight/settle pass over sword/staff attacks and directional gaits.
Editable actions are in `assets/authored/player-motion-v3.blend`; the v2 source
below is preserved. Current equipment, export paths and review are documented
in [oldstyle-player-art.md](oldstyle-player-art.md).

Archery uses source-time retiming so the held draw is not skipped by an
arc-length map. The right hand holds the bow and the left draws the string;
an equipped shield is stowed. The nock, limbs and arrow animate in both color
and shadow passes. Existing attack contacts remain authoritative.

## Preserved v2 integration notes

The player now uses the 22 actions from the reviewed `Human_Male_Motion_v2.blend`:
relaxed idle, sword/staff guards, four walks and four runs, equipped sword/staff
walks and runs, two sword attacks, two staff attacks, jump, airborne fall and
heavy landing. `assets/authored/player-motion-v2.blend` preserves an editable
copy of the action rig. The local full character source remains unchanged.

The native pipeline still retargets hand/ankle targets, pelvis displacement,
torso orientation and weapon direction onto the equipment-compatible body rig.
It does not play all 57 Blender bones directly. Ground contacts and horizontal
movement remain native; class appearance, clothing and interchangeable gear
remain available. This integration does not replace the thirteen collection
models or claim exact reproduction of Blender deformation and shading.

## Current playback

- Directional gait follows distance actually travelled, with smooth walk/run
  blending. Entering the authored layer fades over 180 ms, including after load.
- Sword and staff attacks use a separate presentation controller. Authored
  contact/release markers align with the existing combat clock. Arc-length
  retiming spreads rapid source keys across the available wind-up/cut interval;
  recovery continues after the simulation's attack timer ends at the same class
  playback rate. Existing damage,
  movement speed, cooldowns and command acceptance are unchanged. The short
  gameplay contact window still bounds the duration of anticipation.
- Interrupted attacks freeze and fade their outgoing pose; they do not advance
  to a cancelled contact. Focus loss, death and load clear transient playback.
- Staff direction comes from the clip. Both wrists are fitted to one shaft in
  their shared reach range, and the supporting fingers close around it. Class
  spell gestures can release that grip and take precedence over idle/gait.
- Jump sampling uses the authored takeoff/landing markers. Descent blends into
  the airborne fall; accepted landings begin at the landing contact pose.
- Six old tracks remain for start/stop and compatibility with callers using the
  previous attack/jump presentation. Other actor animation is not replaced.

## Current source and binary contract

The ignored exporter is
`.blender-local/male-study-20261028/motion-v2/runtime/export_runtime.py`.
Run it using Blender in background mode with `--python-exit-code 1`. It reads
the full v2 source, writes the editable action library, and atomically replaces
the runtime archive. Its compatibility inputs and pre-integration backups live
in the adjacent `before-integration` directory. Do not run the old tracked
Collection 04 exporter over this archive.

`player-clips.json` records source/archive/track SHA-256 hashes and timing
markers. PCL2 is little endian: magic/version/count, followed by 28 tracks.
Each track has a uint32 frame count, seven float32 values (duration, load,
contact, takeoff, landing, stance fraction, travel speed), a uint32 loop flag,
and 19 float32 channels per frame. V2 clips are sampled at 60 Hz; preserved
tracks retain their original sampling. The 230,768-byte archive stays embedded
as resource 303. Blender is not required to run the game.

The loader bounds allocation, track/frame counts and duration; validates all
markers, loop seams, finite channels and normalized weapon axes; and rejects
truncated or trailing data. Sampling is cubic, periodic for loops and clamped
for nonloops. Attack arc-length tables are built once when loading.

## Current review

The isolated build is `build-motion-v2-release/RPGNative.exe`, with its launcher
alongside it. Native screenshots are under
`build-motion-v2-release/authored-locomotion-check`, including 42-frame sword
and staff sequences for basic and heavy attacks, start/run/stop and eight
directional views. These captures use the game's renderer and equipment.

The focused motion test covers 30/60/144/1000 Hz playback, planted-foot slip,
rapid direction changes, weapon/finger continuity, staff grip, cancellation,
extended recovery and actual Warrior/Mage command streams compared against
plain Simulation saves. Build/test logs and source backups are local under
`.blender-local/male-study-20261028/motion-v2/runtime`.

Final verification: Release passed AuthoredLocomotion, Animation, ActionClip,
FootPlacement, ClassEquipment, AttachmentLod, PlayerBody, HandSkinning and Motion
(9/9). Debug passed AuthoredLocomotion, Animation and ActionClip (3/3), including
native capture/GPU diagnostics. Both executables passed the hidden smoke run.
The v2 fixture measured 0.00799 m maximum hand displacement per 1 ms during
rapid direction changes, 0.04357 m maximum attack-tip displacement per 1 ms,
and less than 0.000001 m staff grip error. These are bounded test measurements,
not a guarantee that every live pose is artifact-free. The full Blender source
SHA-256 and runtime manifest SHA-256 were verified unchanged/matching.

## Historical integration record — 2026-10-28

The following records the previous Collection 04 integration and its old
validation results. The source, archive format and review paths above supersede
their equivalents below.

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
