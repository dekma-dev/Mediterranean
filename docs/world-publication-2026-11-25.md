# Owned combat and world presentation

## Engine change

The 3D game view now consumes one published simulation state for actors,
projectile destinations, preparation and opportunity effects, combat lights,
barriers, control statuses, frost fields, campaign props and floating combat
text. Previously these consumers mixed an owned actor frame with direct reads
of the live simulation. The artwork and gameplay rules remain unchanged.

`SceneSnapshot` now owns the additional evaluated inputs:

- Actor root/snare/exposure timers and authoritative contact height. Projectile
  tracking resolves the accepted target ID in this publication, including
  dummies and the active sparring opponent, without consulting current selection.
- Player barrier, evasion, frost field, discipline, guard and elemental state.
  Cast preparation, class opportunity shapes and local spell lights use the
  same accepted action clock and solved sockets as the drawn character.
- Court activity, seal mask, captain/ward state; recovered supplies and supply
  channel progress; ritual stage/charge; anchor completion/channel progress.
  These are evaluated presentation values, not a second quest state machine.
- Owned combat feedback entries. A later simulation update or reset cannot
  change text positions, amounts, ages or sequence IDs in a retained frame.

`Game::PresentStep` advances sustained envelopes and projectile paths using
that publication. `Game::Render` uses it for all dynamic 3D simulation inputs.
Terrain/scenery queries continue to use the immutable map; HUD/menu rendering
and feedback adaptation still use Simulation synchronously. Existing
Simulation overloads on effect utilities are compatibility adapters for tests
and isolated previews; the gameplay path uses the already-published views.

## Identity correction

The court ward used to attach to `Enemies().back()`. That only selected the
captain while the roster retained its original order. Publication now resolves
the authoritative captain ID 105 and evaluates `CourtProtected` for that actor.
Reordering enemies preserves the attachment, and an absent captain clears it.
No combat protection rule, damage or quest condition was changed.

A second regression reproduced a visible training bug: the practice mannequin
is hidden during sparring, but sustained control effects still selected it and
projectile tracking could still resolve it. `ShowsDummy` now supplies the same
visibility decision to drawing, dummy animation, status selection and flight
target lookup. Starting sparring immediately removes the hidden control aura;
ending it restores the prop and fades any still-authoritative status in afresh.
An in-flight effect keeps its last accepted destination when the target becomes
hidden; it does not jump to the trainer or manufacture a hit. The failing
before-fix result is saved in `world-publication-visibility-before.log`.

## Ownership and cost

The publication retains no actor/status/feedback pointers into Simulation. Derived
properties are evaluated from Simulation, avoiding duplicate class/quest rule
tables. Actor and feedback vectors reuse capacity. Storage reservations occur
before updating published values; normal actor/status capture does not create
another roster or copy navigation paths. A 39-actor fixture with no combat
feedback owns 5,136 bytes in Release (5,152 in MSVC Debug); additional text capacity is included in
the existing scene-byte/storage-growth diagnostics.

This remains an owned **synchronous** publication. It is not a concurrent render
packet, a render thread or interpolation. Animation state, emitter histories,
resident presentation and camera still live in Game. Separating threads also
requires those values, the HUD/audio boundary and renderer ownership to be
handled explicitly. No frame-rate or whole-process-memory improvement is claimed.

## Regression evidence

`ScenePublicationTests` exercises real accepted casts and campaign fixtures:

- Frozen statuses survive source replacement; 300 publications reuse storage.
- An actual released Mage projectile retains its accepted destination after
  live target movement, death and reselection; a new dead-target publication
  cannot manufacture a replacement target or contact.
- Captain attachment survives roster reversal and an empty roster.
- Actual start/stop sparring hides and restores practice-target status effects
  consistently with target visibility, without carrying an old envelope back.
- Twenty native cases cover preparation/release/contact for Warrior, Mage,
  Scout and Priest; defenses/controls; the actual frost field; supplies, ritual
  and court objectives. Each captures a published frame, replaces Simulation
  without publishing, and captures again. The two PNGs must be byte-identical.
  The next accepted step must produce a different image and new revision.
- Retained combat text is checked independently of the hidden HUD. Public
  restore clears old-world transient effects.

The native retention comparison is recorded in
`build-ninja-release/scene-publication-review/comparison.json`: 20 unchanged
retained frames and 20 changed next publications, with 60 PNGs. Court and frost
field images were visually inspected. This tests 3D-state isolation; it does
not claim that the HUD can already render independently of Simulation.

Before implementation, the existing combat/status suites and smoke output were
hashed in `build-ninja-release/world-publication-baseline.json`. All **304**
PNGs match after migration: 278 combat/status images and 26 ordinary smoke
captures. Results are in `build-ninja-release/world-publication-comparison.json`.

Release acceptance passes all twelve selected suites in 137.58 seconds:
`ScenePublicationTests`, `CommandExecutionTests`, `GraphicsLifecycleTests`,
`ClassEffectsTests`, `MovingCombatTests`, `SustainedEffectsTests`,
`CombatReleaseTests`, `CastingEffectsTests`, `ClassDisciplineTests`,
`InterfaceTests`, `CombatHudTests` and `SouthernTests`. Log:
`build-ninja-release/world-publication-acceptance.log`.
The same twelve suites pass in Visual Studio Debug in 264.43 seconds;
log: `build-local/world-publication-acceptance.log`.

The later practice-prop visibility regression first fails on the migrated
implementation, then passes after the shared visibility fix. The final
affected Release suites (`ScenePublicationTests`, `CommandExecutionTests`,
`ClassEffectsTests`, `SustainedEffectsTests`) pass 4/4 in 32.06 seconds; log:
`build-ninja-release/world-publication-visibility-after.log`.
The same four pass on the final Debug build in 71.53 seconds; log:
`build-local/world-publication-visibility-after.log`. Both public executables
were rebuilt again after this fix.

Both public executables are rebuilt. The Release public smoke test returns 0;
both actual public `--verify-command-replay` runs return 0 with 90 sealed
commands and no mismatch. Each executes 330 frames / 660 steps and reports
one scene storage growth in the stationary workload. Reports are
`build-ninja-release/world-publication-replay.txt` and
`build-local/world-publication-replay.txt`. These are native execution checks,
not a manual long-session or interactive-FPS claim. The D3D debug layer is
unavailable. Build logs retain pre-existing UI shadowing/conversion warnings.

Initial failures in the new test were fixture errors: jumping directly from a
three-enemy legacy save to a six-enemy campaign, then attempting to reload a
stress actor with health/cooldown above save limits. Fixtures now accept the
intermediate campaign through real commands and restore a valid save. No save
validation or production assertions were weakened.

## Next boundary

Next work is explicit HUD/audio read views and a complete presentation packet,
followed by measured previous/current interpolation if an accumulated fixed
clock is introduced. Preserve per-step event receipts, one-shot scheduling,
reset cancellation and retained-frame tests through that transition.

No source assets, faces, system settings or player saves are changed. Existing
staged/unstaged work is preserved; no commit or push is performed.
