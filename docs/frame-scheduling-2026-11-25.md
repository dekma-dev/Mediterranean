# Bounded simulation steps and frame receipts

## Gameplay change

The interactive game now divides an admitted display frame into simulation
steps of at most 1/120 second, with at most six steps per frame. Previously a
50 ms frame also meant a single 50 ms gameplay/animation update. Short steps
reduce frame-rate-dependent attack startup and keep movement, jumps, enemy
updates, animation, contacts and effect tracking on a finer time scale when
rendering slows down.

Input is sampled once per display frame. Continuous movement, sprint, facing
and movement capture persist across its steps. Jump, spell activation, target
selection, equipment changes, purchases, quest actions and UI clicks execute
once. New command fields default to one-shot because continuation commands use
an explicit allow-list of held controls.

This is bounded semi-fixed integration, **not** an accumulator-based fixed
clock. Each frame consumes its admitted time completely, without waiting for
another 120 Hz tick or retaining catch-up debt. A frame shorter than 1/120 s
remains one shorter step. Rendering uses the final published state; render
interpolation and a separate simulation thread are not implemented here.

## Engine contract

- `game/CommandSchedule.h` creates an allocation-free plan and records admitted,
  subdivided, executed, ignored, capped and boundary-cancelled work. The existing
  50 ms frame-admission limit remains, with explicit accounting on both paths.
- `Game::ExecuteFrame` is the interactive frame entry point. The lower-level
  `ExecuteCommand` remains available for exact single-step fixtures and tools,
  and now returns its owned `StepReceipt`.
- Animation, accepted visual events and sound collection run for **every**
  simulation step. The audio device is still updated once per display frame
  using the bounded one-shot delivery queue. Early footsteps, releases and
  contacts cannot be overwritten by a later empty step.
- A compact frame receipt records event-kind presence, event/drop counts,
  earliest authoritative offsets within the frame, executed steps and reset
  boundaries. It does not create a second unbounded event queue. Event timing
  comes from accepted combat events, not from floating damage-text ages.
- Merchant-open requests are retained across the frame. Inventory/trade result
  messages are captured at the action step. Otherwise later steps clear the
  simulation's transient trade flag and can overwrite the action's message.
- A restart, respawn or replay transition cancels the remaining steps after
  publishing the replacement state. Held controls from the outgoing frame do
  not move the new character/world. Cancelled time is counted and is never
  carried into a later frame.
- Native benchmark-suite walking/combat workloads use the same frame scheduler
  as interactive play. Execution reports expose frame/step/peak/cancellation
  counters; the ordinary benchmark and replay mode use the interactive path.

No damage, ability cooldown, class progression, source model, facial anatomy,
shader artwork or saved-game format is changed. Increasing simulation work
does not add world draw submissions or promise increased FPS.

## Regression coverage

The extended `CommandExecutionTests` covers:

- continuous controls versus one-shot actions; zero, negative, non-finite and
  overlong durations; six-step and fast-frame bounds;
- actual equipment toggling and UI sound exactly once in a six-step frame;
- an actual merchant-open event emitted before the last step, one purchase and
  its receipt, and restart cancellation without post-reset movement;
- actual four-class heavy contacts and damage at 20/30/60/120/144/240/1000 Hz;
  the unchanged direct single-step path at 20 Hz supplies the legacy comparison;
- equal-time sprint/jump trajectories at 20/30/60/120 Hz;
- previous owned scene, replay, event delivery and lifecycle regressions.

`CombatReleaseTests` additionally runs actual four-class fights through
six-step display frames. Each action must produce one observed release and
one contact, no event loss, and identical repeated renders without advancing
simulation or scene publication. Captures use `scheduled-<class>-release` and
`scheduled-<class>-contact` names in its review directory.

## Lighting test diagnosis

The previously reported `World spell light had no visible receivers` failure
was a stale camera fixture. The game's requested camera behavior keeps its
chosen distance behind roofs. The old low eastern test view was almost fully
covered by a roof, hiding the surfaces the assertion intended to measure.

The fixture now observes the same real contact from a higher southern view.
The light-on/off threshold, material/face exclusions, portrait and HUD checks
are unchanged. No shader or lighting intensity was altered to pass the test.
This supersedes the open-failure note in `scene-snapshots-2026-11-25.md`.

## Measured acceptance

The same nine suites pass in both configurations: Release 9/9 in 68.44 s,
Visual Studio Debug 9/9 in 143.17 s. Logs are
`build-ninja-release/schedule-acceptance-tests.log` and
`build-local/schedule-acceptance-tests.log`. They cover the actual simulation,
animation, native render and input/lifecycle paths, not just the scheduler.

Actual heavy-attack contact times in the stationary training fixture:

| Class | Previous single 50 ms step | Scheduled at 20/30/60/120 Hz | Damage on both paths |
| --- | ---: | ---: | ---: |
| Warrior | 0.350000 s | 0.308333 s | 90 |
| Mage | 1.250000 s | 1.208330 s | 107 |
| Scout | 0.839474 s | 0.797807 s | 90 |
| Priest | 0.907143 s | 0.865477 s | 86 |

This removes about 41.67 ms of extra startup/contact delay at 20 FPS in these
fixtures, without changing damage. It is **not** a claim that every action or
frame rate has the same result. Above 120 Hz the smaller steps keep finer
quantization; the 144/240/1000 Hz observations remain within one maximum step
of the reference. The 32 cases are recorded in
`build-ninja-release/schedule-cadence.csv`; the verbose acceptance log is their
source. Sprint/jump trajectories match at equal times at 20/30/60/120 Hz.

Both public executables were rebuilt. Their actual
`--verify-command-replay` runs return exit 0 with 90 sealed commands and zero
mismatches. Each run reports 330 admitted frames, 660 steps and no cancelled
or dropped work. Reports: `build-ninja-release/schedule-replay.txt` and
`build-local/schedule-replay.txt`.

The Release public `--smoke-test` returns exit 0 and produces all 26 PNGs under
`build-ninja-release/schedule-smoke*`, covering fullscreen restoration,
device/shaders, resize and world/UI capture. Actual four-class contact/release
captures were inspected; no new art is claimed. `--benchmark-suite` also
passes all four native workloads through the frame scheduler; the report is
`build-ninja-release/schedule-benchmark-suite.txt`. Its per-workload execution
counts include warmup and setup, explicitly separate from measured frame
distributions. This is a hidden-window check, not interactive FPS or long-play
acceptance. The D3D debug layer remains unavailable.

## Limits and next work

Low display FPS still means fewer visible frames. Substepping improves update
granularity and bounds command timing; it does not invent rendered frames.
High-refresh frames shorter than the maximum step retain their finer input
quantization. Current dynamic effect/world/UI queries remain synchronous.

Next: explicit views for the remaining dynamic consumers, then measured
previous/current interpolation if introducing an accumulated fixed clock.
Retain current per-step event consumption, frame receipts, cancellation and
transaction tests through that transition. A separate render thread requires
more than the actor snapshots already present.

No commit or push is performed by the assistant.
