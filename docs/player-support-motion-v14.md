# Running support and flight, v14 — 2026-11-10

The player's running cycle now separates contact loading, push-off and rise
through flight. The recovery leg folds behind the pelvis and lowers before
contact; the rear elbow opens instead of keeping the forearm at one height.
The native controller also fixes two discontinuities that distorted authored
motion during rapid stops and direction changes.

## Source and reference scope

Editable source: `assets/authored/player-motion-v14.blend`. It contains the
accepted 57-bone rig and all 51 actions, with `Run_Forward` selected for review.
The assembled character remains in its separate, unchanged model source.
The PCL4 export retains 57 runtime track identities. Sixteen directional
running actions and four jumps were revised. The heavy fall/landing action
was rebaked with identical output; it and the other 36 retained records are
byte-identical to v13 (37 preserved records in total).

The review used the user's `D:/assets/what_we_have_now/01.mp4`, images and
video sequences under `D:/assets/learning_material`, and the v13 native
captures. The Allods combat recordings often hide feet behind effects and
equipment. They provide movement and presentation references, not clean
motion-capture data or evidence of a one-to-one match.

## What changed

- Running contact duty is 0.24 of each leg's cycle, read by the native
  controller from the exported clip. The running cadence and game speed are
  unchanged. The revised pelvis curve loads the support and rises through
  flight; reach correction no longer filters away the authored vertical curve.
- Recovery height peaks earlier, behind the pelvis. The forward knee no longer
  carries the same high ankle throughout the recovery. Free elbows open on
  the return swing and close on the forward swing. Equipped clips retain the
  accepted weapon grip.
- A step released just after phase zero could miss the old completion check
  on its next wrap and return to its old takeoff point. Completion now checks
  consecutive phase samples. A rapid-reversal diagnostic exposed a roughly
  39 cm displacement in the requested ankle target before the fix.
- The landing lead follows filtered velocity, while the goal follows actual
  actor travel. An abrupt stop therefore does not remove the complete landing
  lead in one render extrapolation. The lead's gait timing uses the same
  filtered speed to avoid introducing another discontinuity at zero speed.
- Imported arm motion fully cancels the legacy contact-driven wrist swing,
  including during idle/direction blends. A correcting foot no longer adds
  an unrelated swing to the authored hand path.
- Normal jump descent no longer mixes in a fall pose sampled from the idle
  clock. Extended falling is driven by downward velocity beyond ordinary jump
  descent. The same jump now produces the same pose regardless of how long
  the character previously waited.

The engine remains DirectX 11. Gameplay movement speed, gravity, jump impulse,
attack timings, faces, geometry, materials, customization, equipment sources
and all thirteen collection assets are preserved. This revision improves
motion; it does not claim photorealistic models or motion-capture fidelity.

## Review and checks

Local review: `.blender-local/motion-v14-20261110/review.html`. It presents
synchronized v13/v14 comparisons for running, equipped running, walking,
strafing and jumping, plus actual game-update/render sequences with and
without equipment. Scripts and review media stay under ignored
`.blender-local`.

Regression coverage includes world-space support, fixed limb lengths,
lateral leg clearance, early-release cycle wrapping, rapid stop/reversal
continuity, frame-rate-independent phase, jump clock independence, ordinary
versus extended falling, weapon grips and GPU deformation. Numeric checks
complement the visual review; they do not establish artistic realism.

All twelve selected motion/art suites pass in both Release and Debug
(24 suite runs). In the millisecond rapid-reversal diagnostic, maximum hand
displacement is 9.23 mm and ankle displacement is 21.79 mm. The planted-foot
check stays below its 2 mm tolerance. These are diagnostic bounds, not
measurements of real human motion.

Seven new videos were encoded and decoded successfully. All 1,110 source
frames were checked for freshness and PNG integrity. Source hashes confirm
that the sixteen frozen model, collection, equipment and earlier motion
files are unchanged. The body archive and face/scalp shader sections also
match the preserved baseline.

The Ninja Release/Debug, Visual Studio Release/Debug and frontier native/debug
outputs all embed the current clip archive and pass fixture-save smoke
launches. The local `validation-summary.json` records the executable and
archive hashes, test results, video verification and preservation audit.
Normal play uses `build-ninja-release/MediterraneanLauncher.exe`.
