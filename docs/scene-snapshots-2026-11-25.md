# Owned actor presentation snapshots

## Result

Player and enemy animation, actor drawing, equipment, targeting outlines,
camera following and hostile cast attachments now consume an owned scene
publication instead of borrowing the mutable simulation's actor storage.
World attack warnings also retain their accepted visibility, priority and
damage footprints in that publication. Simulation remains the sole authority
for combat, movement, target selection and warning selection.

This extends the command/event boundary implemented earlier on November 25.
It is infrastructure for independent update/render scheduling, not a change
to animation timing, combat balance or artwork.

## Publication and lifetime

- `game/SceneSnapshot.h/.cpp` defines compact value records. An actor carries
  identity, pose inputs, life/loot state and evaluated presentation properties;
  it has no navigation path, AI history, inventory, save string or borrowed
  actor pointer. The player record includes motion/action channels and the
  resolved outfit. Appearance is copied without modifying facial data.
- `Game::ExecuteCommand` publishes after the authoritative command step and
  before animation consumes its owned events. Sequence identifies the command;
  revision also advances on lifecycle refreshes without inventing another tick.
- Initialization, successful load/restart/respawn, replay transitions and input
  suspension refresh the scene. Failed loads leave it intact. A cancelled cast
  is published on focus loss even when no subsequent game tick occurs.
- Consumers use a const scene; a retained copy owns its actor/target/warning
  values independently of later publications or destruction of the simulation.
  Animation binding still matches ID, kind, rank and spawn home, retaining
  surviving actors through reorder and resetting reused IDs/respawns/teleports.
- Enemy derived properties come from the authoritative `Enemy` methods when
  captured. There is no second table of windup durations or damage geometry.
  The existing danger-first selector supplies up to eight owned warnings.
- Facial calculations, authored meshes/clips, save format, icons, UI design,
  spell appearance and the existing capped variable-step clock are preserved.

The actor vector reuses capacity on ordinary publications and grows only with
the authoritative roster. It is not a fixed maximum-entity pool. A normal
39-combatant publication owns 4,112 bytes in Release (4,120 in MSVC Debug,
including the debug vector bookkeeping), including capacity and fixed player,
trainer, dummy, target and warning records. The regression keeps the same
storage through 300 subsequent captures. Diagnostics now report scene revision,
command sequence, actor count, owned bytes and storage growth count.

## Validation

`CommandExecutionTests` now exercises retained frames after replacing the
simulation, target/cast ownership, storage reuse, roster reorder, death/revival,
spawn identity reuse, frozen cast sockets, crowded warning selection and actual
four-class Game lifecycle transitions. Its existing replay/audio checks remain.

Native comparison evidence is under the ignored build directory:

- `build-ninja-release/snapshot-smoke-comparison.json`: all 26 before/after
  game world, camera and UI PNG payloads have identical SHA-256 hashes. Capture
  filenames use the `snapshot-before*` and `snapshot-after*` prefixes; the PNG
  payloads intentionally have no extension because the smoke tool takes a file
  prefix, not a directory.
- `build-ninja-release/snapshot-combat-comparison.json`: all 179 generated
  combat captures match an isolated prior-index presentation build exactly.
  The baseline uses archived index versions of Game, actor/cast/warning code
  and the same native renderer/simulation libraries. It does not revert the
  working tree or change the index. Its source, response files, logs and output
  remain in `build-ninja-release/snapshot-baseline/`.
- `CommandExecutionTests`, `ActorPresentationTests`, `SustainedEffectsTests`,
  `MotionTests` and `CameraControlsTests` pass in Release and Visual Studio Debug.
  The first new ownership test incorrectly used already-dead campaign actors
  for an alive/dead/alive scenario; the fixture now explicitly begins alive.
  Production code and lifecycle assertions were not relaxed to address it.
- Additional `ExecutionTests`, `GraphicsLifecycleTests`, `CombatEffectsTests`
  and `MovingCombatTests` pass in Release (24.46 s) and Debug (46.92 s).
  Thus nine distinct focused suites pass in both configurations; the separate
  lighting failure below is the tenth suite, not a passing result.
- Logs: `build-ninja-release/snapshot-tests.log` records the initial fixture
  failure and the lighting failure; `snapshot-command-tests.log` records the
  corrected ownership fixture passing; `snapshot-integration-tests.log` records
  the four additional suites. Debug results are in the corresponding
  `build-local/snapshot-tests.log` and `snapshot-integration-tests.log`.
- Both public `RPGNative.exe` outputs are rebuilt. Both actual executables return
  zero for `--verify-command-replay`, with 90 commands, a sealed trace, PASS and
  mismatch zero; reports are `build-ninja-release/snapshot-replay.txt` and
  `build-local/snapshot-replay.txt`. Their 330-step ordinary workload reports
  one scene storage growth. This is not an interactive performance guarantee.
- Build logs have no C++/linker errors or compiler warnings; working-tree and
  staged `git diff --check` pass. The pre-existing staged changes are preserved;
  the user must refresh staging to include this continuation before committing.

### Existing lighting test failure

Follow-up on the same date: the frame-scheduling pass diagnosed a roof-occluded
test camera. An unobstructed view passes the unchanged assertions; see
`frame-scheduling-2026-11-25.md`. The historical observations below are retained.

`CombatReleaseTests` reaches its Mage basic-contact lighting assertion and
reports `World spell light had no visible receivers` in Release and Debug.
The isolated **pre-snapshot** implementation fails at the same assertion;
all 179 captures produced up to that point are byte-identical. The failure
is retained, not hidden by reducing its threshold. Lighting acceptance needs
separate investigation; this pass does not establish that assertion as passing.
The GPU driver debug layer is unavailable on this installation.

## Remaining boundary

The scene is an owned synchronous publication, not a lock-free mailbox or a
thread-safe render packet. Effects' projectile/status/lighting queries, world
objective views, audio conversion and UI still read Simulation synchronously.
Static terrain queries also remain shared. These consumers must be included
before claiming that rendering can run independently of simulation.

Next: define held versus one-shot command scheduling, move remaining dynamic
consumers to explicit views, then introduce measured previous/current
interpolation and multiple simulation ticks. Teleports, focus/load/roster
boundaries and event delivery must retain their current tests. No fixed-tick
conversion, FPS improvement or whole-process memory reduction is claimed here.

No assets, system settings or user saves were changed; no commit or push.
