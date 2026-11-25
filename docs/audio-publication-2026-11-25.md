# Owned audio publication and device isolation

## Engine boundary

The interactive engine now passes one owned `sound::Frame` from Game to Audio.
Native playback no longer traverses live Simulation, creature contacts or the
character-selection audition object. Per-step collection uses the existing
owned scene and accepted command receipts; it may query the immutable map for
ground material. The device receives evaluated listener, music-threat and
preparation state, copied cue batches, source identities, living emitter
positions, volume settings and an optional copied audition.

`Game::TakeAudioFrame` drains pending one-shots exactly once and advances a
sequence that survives Game reinitialization. A separate world generation
continues to clear native effects on lifecycle changes. The producer reuses
emitter capacity; a caller retaining a packet across the next publication must
copy it. The main loop consumes it synchronously. Public smoke, benchmark and
command-replay workloads also execute this publication path without opening
an audio device.

The device rejects duplicate, older and malformed packets before changing
playback. Validation covers finite listener/source coordinates and timing,
cue bounds and batch counts. Packets delivered while suspended are consumed
without playback, so replaying the same sequence after resume cannot emit an
old cue. This is a process-local stream contract, not network serialization.

## Behavioral fixes

- Each accepted preparation has a token carried through priority selection,
  bounded delivery and native voice assignment. If a cast is cancelled and a
  replacement starts between audio updates, the old preparation fades while
  the new one retains its own envelope. A shared `preparing` boolean previously
  could keep the old tail at full level during the new cast.
- Moving warning/creature sounds bind the accepted actor ID, kind, rank and
  home. Losing that source or observing a changed identity permanently
  detaches the existing tail at its last position. A later matching ID cannot
  recapture an already orphaned voice. A delayed cue also checks its accepted
  identity before binding. Fixed contacts keep their accepted location.
- Combat music now considers the full published combatant roster and the
  active sparring opponent. The previous campaign-enemy-only traversal missed
  pursuing wildlife and the trainer when the player was not attacking.
- Malformed counts/cue IDs cannot index native sample storage, and repeated
  frame submission cannot replay a spell, click or step.

No music, samples, class rules, facial/model data, save format or rendering
artwork are changed. These are delivery/lifetime corrections, not a new sound
pack or a claim of improved recording quality.

## Ownership and diagnostics

The existing 96-entry delivery queue, 24-cue output, 250 ms expiry and twelve
effect voices remain. Cue entries now retain cast and accepted-source keys
when shifted, selected or drained. Scene publication includes the authoritative
one-step jump flag so ground/jump collection does not read mutable Simulation.

Diagnostics report publication sequence, living emitter count and owned
packet bytes. Interactive shutdown also reports accepted device frames,
rejected packets and submitted native cues. Stable-world tests verify emitter
storage reuse through 200 publications per focused class. The retained Release
fixture contains 36 emitters and 2,504 bytes (2,512 in Debug); this includes the copied vector's
capacity, not total engine memory or a hard population limit. Resident recorded
PCM remains 8,318,538 bytes in the audio test.

The device still owns its acoustic grid and evaluates obstruction/reflection
against that static data. Older Simulation overloads remain compatibility
adapters for isolated tests; Engine::Tick uses only the owned-frame overload.

## Validation

`AudioFrameTests` exercises actual Game commands for Warrior, Mage, Scout and
Priest, frozen packets across reset, one-shot draining, malformed values,
wildlife/trainer threat, source movement/disappearance/reappearance and queued
source identity. The two-cast fixture accepts a real Mage heavy cast, cancels
it, and accepts a new basic preparation before the next device update.

Muted XAudio2 checks inspect submitted voices: the first preparation fades
independently of the second, duplicate/stale/invalid packets submit nothing,
moving/dead sources retain correct positions, suspended packets do not replay
and unchanged menu auditions do not restart. Endpoint unavailability is an
explicit skip rather than a reported device pass.

The eight selected suites pass in Release (46.93 s) and Visual Studio Debug
(109.25 s): `AudioFrameTests`, `AudioTests`, `RecordedAudioTests`,
`CommandExecutionTests`, `ScenePublicationTests`, `ExecutionTests`,
`GraphicsLifecycleTests` and `InterfaceTests`. Logs:
`build-ninja-release/audio-publication-acceptance.log` and
`build-local/audio-publication-acceptance.log`. Both native endpoints were
available and exercised; neither packet test skipped XAudio2.

After adding interactive device counters and removing a new local-variable
shadow warning, both public executables and the four directly affected suites
were rebuilt. `AudioTests`, `AudioFrameTests`, `ExecutionTests` and
`GraphicsLifecycleTests` pass again on the final Release build (17.74 s) and
Debug build (40.82 s). Logs are `audio-publication-diagnostics-build.log` and
`audio-publication-final-tests.log` in each build directory. The broader build
still contains pre-existing Interface warnings. D3D driver debug-layer
availability is unchanged; this is not a claim of running a missing layer.

Both final public executables return 0 from `--verify-command-replay`: 90 sealed
commands, PASS and no mismatch. Each reports 330 audio publications, 330 frames
and 660 simulation steps, with 39 emitters and retained packet storage of
2,600 bytes in Release / 2,608 in Debug. Reports:
`build-ninja-release/audio-publication-replay.txt` and
`build-local/audio-publication-replay.txt`. Diagnostic mode does not initialize
an audio device; actual device checks are covered separately above.

The final Release public smoke test returns 0. All 26 captures match the
pre-change images byte-for-byte, recorded in
`build-ninja-release/audio-publication-image-comparison.json` against
`audio-publication-image-baseline.json`. This confirms unchanged ordinary
rendering through the new publication path. `git diff --check` passes.

## Limits and next work

This is synchronous owned publication, not an audio thread, general streaming
or a complete concurrent render packet. XAudio2 still renders on its native
threads. No FPS or whole-process memory gain is claimed. The source key does
not include a spawn-incarnation counter: a same-ID/same-home respawn occurring
entirely between observations is not distinguished by this pass.

Visible listening acceptance remains: rapid cancel/recast for the four focused
classes; wildlife and sparring music transitions; moving sources near walls;
focus loss, mute, load and roster preview transitions. The automated native
checks are muted and do not establish subjective sound quality.

Next: explicit HUD read views and a complete presentation packet, then measured
previous/current interpolation if introducing an accumulated fixed clock.
Keep accepted command receipts, cue expiry, cast identity and lifecycle
cancellation intact. Mesh/audio/world preparation remains a separately
profiled task. No commit or push is performed.
