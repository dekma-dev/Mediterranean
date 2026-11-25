# Owned HUD state and update-driven interface clocks

## Change

Game now publishes `ui::HudSnapshot` alongside its scene publication. The
gameplay HUD and every in-world panel draw from this owned read model instead
of querying mutable Simulation. This covers player/target frames, incoming
warnings, abilities, inventory/equipment, trade, quests, atlas, journal, build
preview, sparring, loot and the Escape menu.

The read model owns evaluated scalar values, dynamic text, inventory, reports,
spell readiness/cooldowns, talent/merchant availability, quest tracking and
accepted step events. `HudValues.inc` is the shared list for declaring and
capturing value-only accessors; there is no hidden Simulation copy. Actors
contain presentation data and evaluated properties without AI paths or
perception history. Target and incoming-warning entries own their actors;
retaining or copying a frame does not retain pointers into the source roster.
Catalog descriptions and enemy names refer only to static immutable literals.

Immutable map geometry is copied once on first capture and held through shared
const ownership. Retained packets remain valid after their source simulation
and publisher are destroyed. The current engine has one immutable world; a
future map replacement/streaming system must add an explicit geometry revision
and replace this shared owner rather than silently reusing it.

`Game::ExecuteCommand` publishes the scene and HUD from the same accepted step
sequence before feedback advances. Restore, restart and focus cancellation
refresh the HUD too. Feedback adapters consume this published state and the
accepted step receipts. Interface code cannot commit a reward, sale or talent:
input still emits commands, and Simulation revalidates them when accepted.

## Shared calculations

Build-stat previews, loadout availability and sell/buyback availability now use
common read-only functions in `BuildRead.h`. Both Simulation and the HUD call
them with their own data. Class bonuses also share one calculation. This avoids
maintaining a second set of gameplay rules in the interface while allowing
arbitrary build drafts and inventory selections to be inspected from a frozen
frame. Existing transaction order, validation messages and prices are retained.

## Clock correction

Health trails, walking-route updates and hover dwell now advance in
`Interface::AdvanceHud` during game updates, including when the HUD is hidden.
The published-frame Render overload only draws and rebuilds layout/hit regions;
repeated rendering cannot age these signals or perform another route search.
Previously health/navigation/hover progression occurred inside Render, so an
unrendered HUD did not maintain its presentation state.

The legacy Simulation Render overload remains an explicit capture/advance
adapter for standalone interface fixtures. Engine gameplay uses the owned
overload. UI layout normalization, pointer regions and panel/selection state
still belong to Interface; this is not yet a fully immutable draw-command
packet or a concurrent renderer. Character-selection presentation has its own
state and is not migrated by this in-world HUD pass.

## Evidence

`HudPublicationTests` compares published spell readiness, cooldowns, buffering,
build previews and transaction availability with authority for all eleven
existing classes. It checks retained hero/target/inventory/map data after
source replacement and publisher destruction, plus actor-storage reuse across
100 additional publications per class. Incoming-warning ownership is tested
independently of the selected target.

Twelve native full-frame cases cover four focused-class casts, seven panels
and a real quest dialogue. Each records a published image, replaces live
Simulation without publishing, and records a retained image. The pair must be
byte-identical; the next accepted update must change the image and publication.
These are complete world-plus-HUD captures, extending the earlier 3D-only
retention checks. Output: `hud-publication-review/` in the build directory.

A real hostile hit starts a health trail, then 1.5 seconds of simulation run
without drawing. The trail must settle, and repeated subsequent draws must
remain identical. The first version of this test placed the attacker five
metres away, outside melee contact; its fixture was corrected to a real nearby
windup. No production combat rule or damage-trail threshold was weakened.

## Final acceptance

Both Ninja Release and Visual Studio Debug public executables are rebuilt.
The selected regression set consists of `CommandExecutionTests`,
`ScenePublicationTests`, `GraphicsLifecycleTests`, `AudioFrameTests`,
`HudPublicationTests`, `InterfaceTests`, `CombatHudTests`,
`CombatHudWorldTests`, `NavigationTests`, `QuestTrackingTests`,
`InventoryTests`, `TalentTests`, `QuestDialogueTests` and `TradeTests`.

The first run passed twelve of fourteen suites in Release (100.96 s) and
Debug (336.86 s). The remaining Talent/Trade failures also reproduce when
linking the unchanged tests against authority sources from HEAD `10b98fb`:
their synthetic version-4--8 saves retained the later version-23 `CHAINS`
record. The loader correctly rejected those incompatible records. Both
fixtures now remove that record when constructing legacy input; production
save parsing and existing assertions remain unchanged. Rebuilt affected
suites pass 2/2 in Release (0.32 s) and Debug (2.19 s). All fourteen selected
suites therefore have passing results on the final relevant code, across
the broader run and affected-test reruns; this was not one all-green run.

Logs in each build directory: `hud-acceptance.log` and
`hud-migration-final.log`. The isolated pre-change reproduction is retained
under `build-ninja-release/hud-authority-baseline/`. Release build logs are
`hud-final-build.log` and `hud-migration-build.log`; Debug logs are
`hud-tests-build.log` and `hud-final-exe-build.log`. An initial Debug EXE link
was blocked by a file lock. An alternate executable was linked inside the
project while preserving the running file; a later normal build succeeded,
and the final public check uses `build-local/Debug/RPGNative.exe`.

Both standard public executables return 0 from `--verify-command-replay`:
90 sealed commands, PASS, first mismatch 0. Each admits 330 frames / 660
steps; scene and HUD both finish at sequence 660, revision 661. The live
39-actor HUD retains 17,072 bytes in Release / 17,608 bytes in Debug with
one actor-vector capacity growth. Reports: `hud-publication-replay.txt`
in each build directory. Separate all-class fixtures retain 16,880 bytes
in Release / 17,416 bytes in Debug and reuse their actor capacity across
100 additional captures per class.

The Release stationary diagnostic records CPU frame p50/p95 of 3.744/4.891
ms and GPU p50/p95 of 3.658/4.810 ms on the local RTX 4060, at 1280x720
with hidden presentation and interval 0. These are single diagnostic runs,
not an FPS improvement claim or a crowded-combat performance guarantee.
HUD publication currently evaluates once per accepted simulation step;
the new report makes its retained size visible for subsequent work.

The final public Release `--smoke-test` returns 0. Of 748 pre-existing
baseline PNGs, 219 were regenerated by this pass and all 219 match their
pre-change SHA-256 hashes, including all 26 public smoke images. The other
529 baseline images were not regenerated and are not claimed as fresh
acceptance. Evidence: `build-ninja-release/hud-publication-baseline.json`
and `hud-publication-image-comparison.json`. The separate new native HUD
suite writes 38 captures per configuration for retention/update checks.
`git diff --check` passes. Existing Interface compiler warnings are not
claimed fixed by this pass.

## Diagnostics and remaining work

Execution reports include HUD revision/step sequence, actor count, retained
packet bytes and actor-vector capacity growths. Bytes include owned string heap
capacity and actor capacity, excluding shared immutable geometry and static
catalogs; inline small-string storage is not counted twice. Growth counts
refer to the actor vector, not all allocations in the game. There is no claimed
FPS gain or whole-process memory reduction.

Next: collect camera, solved animation/effect state and UI layout ownership into
a complete presentation boundary before attempting a render thread or measured
previous/current interpolation. Current stepping remains synchronous without
an accumulated fixed clock. Interactive long-session/focus/resize acceptance
remains separate from hidden automated captures.

No model, face, artwork, class balance, soundtrack or save format is changed.
No files are downloaded, and no commit or push is performed.
