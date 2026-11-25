# Simulation steps and presentation delivery — 2026-11-25

## Runtime boundary

`Game::ExecuteCommand` now calls `rpg::CommandExecution::Advance`, which owns
time admission, simulation timing and an immutable-by-consumption result of the
accepted step. `StepReceipt` contains its sequence, admitted duration, command,
the prior state needed by presentation and an owned copy of combat events.
The next simulation update cannot overwrite that event copy.

`Game::PresentStep` advances animation, effects, footsteps, feedback and camera
after simulation completes. Its event consumers use the receipt rather than
repeatedly reading the simulation's transient event array. Simulation and
presentation CPU time are reported separately, along with admitted step time,
zero-time calls and accepted/dropped event counts. The simulation event buffer
has 64 fixed entries instead of 16, with explicit overflow accounting.

This is a first synchronous separation. Presentation still reads the current
simulation's actor state and must finish before the next step. It is **not** a
thread-safe world snapshot, a separate render thread, interpolation or a new
fixed timestep. The existing variable-step admission/clamp at 50 ms is retained;
no movement, cooldown or combat timing rule is retuned here.

## One-shot sound delivery

Previously the audio device update read the simulation's most recent transient
events directly. Multiple game steps before one audio update discarded earlier
cues, while another read without a new step could repeat the same cues.

Game now converts accepted events and actual creature foot contacts immediately
after each presentation step. The sound collector retains its locomotion and
warrior-effort counters across steps. A separate fixed delivery queue retains
the resulting cues, positions, height, spatial/environment flags and attached
actor identity. Engine drains that queue once before its audio-device update,
including when there is no audio device. Taking again returns no old events.
The device still reads current simulation state for continuous music, emitter
tracking, filtering and voice cancellation; those are not one-shot events.

- Delivery storage is 96 cues, with no heap allocation on append/take. At most
  24 cues reach one device update, matching the existing device cue batch.
- Pending cues older than 250 ms of admitted game time expire. Focus suspension,
  session/load/respawn/replay changes and roster transitions cancel queued
  previous-session sounds. A reset during a step does not publish that step's
  pre-reset events.
- On overload, warnings and confirmed impacts outrank incidental footstep/UI
  cues. Surviving events keep their original order. The per-step collector also
  uses this priority rule when its 24-cue budget fills.
- Counters expose accepted, delivered, expired, dropped and peak queued cues.
  Overload is observable rather than an unbounded backlog of late sound.
- Existing synchronous `sound::Events::Update(state)` / `Audio::Update` calls
  remain available to diagnostic hosts. The interactive engine explicitly
  supplies the drained batch; callers using the legacy direct-state overload
  are still responsible for updating it once per simulation step.

No waveform, soundtrack, voice count, icon, material, model or effect design is
changed by this delivery work.

## Diagnostic command replay

`CommandExecution::BeginTrace` is opt-in. It copies the complete current
simulation, including transient navigation/combat state that gameplay saves
deliberately omit, and records up to 256 admitted commands/durations. Reaching
the requested capacity seals the window; it does not grow or overwrite it.
Game invalidates an unfinished trace at external state changes such as loading,
suspension, notifications or directly changing the tracked quest. A sealed
historical window is independent of later state changes.

Verification runs commands on another copy. A field-wise diagnostic fingerprint
compares persistent state, messages, important player/actor motion and attack
state, paths/perception, cooldowns, feedback and every emitted combat event at
each step. It never hashes object padding or pointers. The live game and its
execution counters are not advanced by verification. A mismatch identifies the
first divergent step.

This is same-build, same-platform diagnostic replay, not portable serialized
replays, networking/rollback determinism or proof that every private internal
bit is identical. There is no trace allocation or fingerprint/save generation
in normal gameplay. Presentation and audio playback are not replayed by the
simulation verifier.

The existing hidden benchmark has an explicit diagnostic mode:

```powershell
.\build-ninja-release\RPGNative.exe --verify-command-replay D:\Mediterranean\build-ninja-release\command-replay.txt
```

It records the 90 warmup steps, verifies them outside measured frame timing and
reports PASS/FAIL plus the first mismatch. Failure returns a nonzero exit code.
Memory in this mode includes diagnostic allocations. Ordinary `--benchmark`
does not enable tracing and remains suitable for comparisons of that workload.

## Regression coverage and existing budget inconsistency

`CommandExecutionTests` exercises four-class real casts/hits, target changes,
jumps, event ownership after later steps, invalid/zero-time admission, exact
command replay, deliberate unrecorded mutation, trace invalidation/capacity,
multi-step cue delivery, repeated reads, accepted spatial metadata, expiry,
overload priority and actual Game load/suspend/restart boundaries. A crowded
real telegraph fixture checks that more than sixteen accepted warnings survive.

The first broader run exposed an existing disagreement: `MovingCombatTests`
still required 16 KiB of `CombatEffects`, whereas `CombatEffectsTests` already
allowed 32 KiB including the fixed 96-particle pool. An isolated compile of
`ce85856` headers and the current headers both measured **26,896 bytes**.
`CombatEffects::stateBudget` now shares the existing 32 KiB allowance between
the two suites. No pool or object size was increased to resolve that mismatch.
The reproduction is retained under
`build-ninja-release/command-baseline/` (`old-size.txt`, `current-size.txt`,
build logs and the HEAD header archive).

## Remaining work

Before admitting multiple fixed simulation ticks per render frame, move the
remaining direct state reads behind explicit presentation snapshots, define
one-shot versus held-command scheduling and add interpolation. Keep native
focus/menu/reset boundaries and the existing combat timing regressions intact.
This pass establishes owned events and bounded delivery needed for that work;
it does not silently switch gameplay to a different clock.
