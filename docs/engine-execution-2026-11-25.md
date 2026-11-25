# Frame admission and native event processing — 2026-11-25

## Runtime changes

The interactive engine now owns an explicit frame clock and processes Windows
messages in bounded batches. A continuously replenished posted queue previously
kept the inner `PeekMessage` loop running instead of returning to a game frame.
Native keyboard sampling also missed presses released between two frames.

`platform/MessagePump.h` returns after at most 256 dispatched messages or a
two-millisecond soft budget, checked between dispatches. Outstanding messages
remain in the Windows queue. No application-owned copy of that queue is built.
An available quit is retrieved before ordinary work, and the engine stops if
its main window was destroyed. `PostQuitMessage` is an on-demand, low-priority
Windows notification, so it cannot always be retrieved ahead of posted work;
the explicit destroyed-window check covers that close path.

Inactive waiting uses `MsgWaitForMultipleObjectsEx` with `MWMO_INPUTAVAILABLE`.
Already inspected queued work can therefore wake the loop; it does not depend
on another new message arriving. Individual window handlers, sent messages
dispatched inside `PeekMessage`, and native modal loops cannot be preempted by
this budget. It is fairness between batches, not a two-millisecond hard deadline.

`engine/FrameClock.h` accounts separately for wall time, admitted update time,
excluded pause time and time clipped by the existing 50 ms step ceiling. Focus
loss, minimization, native move/resize/menu loops, DPI transitions, power resume
and graphics reconstruction resynchronize time and input. Long interruptions
do not create deferred movement, an attack catch-up step or accumulated debt.
The final inactive wait is included in the shutdown accounting.

Normal gameplay still uses one variable update per presented frame, with the
same 50 ms maximum. Admission counters describe time supplied to the engine
tick, not proof of simulation advancement inside a menu, input barrier or
failed graphics operation. This pass does not implement fixed-step simulation,
render interpolation or separate simulation and presentation threads.

## Keyboard delivery

`platform/KeyTransitions.h` holds fixed-size pressed/released flags and the
first press's Ctrl/Shift/Alt state for each virtual key. The window procedure
captures native keyboard messages; auto-repeat does not create repeated game
actions. A key pressed and released between frames can now be both pressed
and released in the next frame while its held state is false.

Native keyboard actions use message edges exclusively, preventing a physical
poll from firing early and a later queued message firing the same action again.
Held movement keys still use the current physical snapshot. Focus/display
cancellation discards pending edges and establishes a fresh held-state baseline.
Mouse gestures retain their existing sampling and ownership rules.

Ctrl hotbar selection and character-name editing use modifiers from the actual
press. Releasing Ctrl before the next render no longer turns a second-row
shortcut into a first-row action; released Ctrl+A/Ctrl+V and Shift selection
retain their meaning. This is an input correction, not interface redesign.

Repeated presses of the same key within one game frame coalesce into one edge.
These latches are not a chronological action queue: existing Game input/action
priorities remain. Native text continues through the existing bounded text path.

## Diagnostics and validation

`Engine::ExecutionDiagnostics()` exposes clock totals, capped/reset/invalid
sample counts, dispatched messages and pump budget yields. `Run()` writes the
report to debugger output on exit. It does not write a new player file or change
any system timer, power, input or driver setting.

`ExecutionTests` checks:

- Time conservation and unchanged ordinary step delivery at 20/30/60/144/1000 Hz;
  capped stalls without catch-up debt, inactive/modal resumes, backward samples
  and an invalid frequency.
- Short taps, modifiers, coalescing, 100,000 repeat notifications, cancellation,
  refocus and preserved pointer sampling. A short Space tap reaches the actual
  Game/Simulation jump path.
- Real native message bursts, self-reposting work, count/time limits,
  already-inspected message wakeup, generated/posted quits and modal events.
- The real hidden `Engine::Run` idle/wait/quit path with 600 posted messages,
  zero admitted frames and a worker wakeup; a second real engine closes its
  window while a backlog remains. Tests neither move desktop focus nor inject
  physical input or write player saves.

`CharacterSelectionTests` also covers released Ctrl+A/Ctrl+V and Shift+Right
on the real name editor. Existing camera, UI, graphics recovery, motion and
combat tests are part of the focused acceptance set. Final acceptance:
**7/7 pass in Release (61.13 s)** and **7/7 in Visual Studio Debug (111.26 s)**.
Both public `RPGNative.exe` builds are rebuilt and `git diff --check` passes.
This is focused acceptance, not a run of all 96 registered project tests.

The real hidden-loop Release report dispatched 601 messages with two budget
yields, admitted zero frames and accounted 0.064901 seconds entirely as excluded
time. The close-with-backlog scenario also passed. GraphicsLifecycleTests still
compares before/after native recovery captures byte-for-byte. Interactive
foreground input and real sleep/resume remain manual checks below.

## Existing motion fixture correction

The old MotionTests trunk-pursuit fixture failed before these changes too.
An isolated export/build of the original `ce85856` simulation reproduced it:
the enemy travelled 1.78977 m but never attacked. The fixture loaded a hidden
target without an observation, whereas the November 19 perception change
deliberately stopped granting hidden-player knowledge during load.

The navigation fixture now explicitly seeds the already observed destination.
Its original travel, collision clearance and attack assertions are retained.
Actual observation, hidden-player independence and memory-free loading remain
covered by CombatTests. No enemy AI, navigation or balance code was changed.
The isolated source, probe and logs are in
`build-ninja-release/execution-baseline/`; no art assets were copied.

## Evidence and remaining work

- Release: `build-ninja-release/execution-verified-build.log` and
  `execution-verified-tests.log`.
- Visual Studio Debug: `build-local/execution-verified-build.log` and
  `execution-verified-tests.log`.
- Each build's `Testing/Temporary/LastTest.log` contains the actual execution
  accounting from the hidden engine, and `graphics-lifecycle-review/` contains
  the before/after recovery frames.

Visible manual acceptance remains: short taps and Ctrl shortcuts at low/high
refresh, Alt-Tab, native title-bar dragging/resizing, fullscreen switching,
sleep/resume and normal close. Automated timing tests do not establish a new
interactive FPS number or hard real-time scheduling guarantee.

Next: measured simulation/presentation separation with replayable command and
event boundaries, then cancellable CPU resource preparation with bounded
pending memory and render-thread/device-generation upload ownership.

Implementation references: Microsoft [PeekMessage](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew),
[message-aware waiting](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex),
[message-time key state](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeystate),
[QPC timing](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps)
and [the on-demand nature of PostQuitMessage](https://devblogs.microsoft.com/oldnewthing/20051104-33/?p=33453).
