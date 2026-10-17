# Mediterranean Progress

Last updated: 2026-10-18

## Current status

The active `main` working tree is native C++ / DirectX 11 only. Allods Online is the primary reference; development is single-player first on one map. **Debug and Release Ninja/MSVC builds pass all nine regression suites and 107 hidden smoke captures per configuration, including world/interface layouts, outfits, sparring, the shop and buyback.** Representative captures were visually inspected. The map has two quests, levels 1–3, a captain, telegraphed combat, progress-preserving respawn, repeatable nonlethal training and a village quartermaster. The 24-cell bag supports drag/sort, stack splitting and six worn slots; bought helmet/shield change visuals and protection. Selling and persistent buyback conserve ownership and gold. Version 6 preserves exact stacks, gear, purchase stock and merchant-held items and migrates versions 1–5. Full interactive playtesting and finished art remain pending. Repository HEAD was last verified as `e6acae0` (0.0.5). The agent has not committed or pushed; input fixes, trading development and documentation remain in the working tree.

## Pre-migration repository snapshot

| Reference | Commit | Inspected state |
| --- | --- | --- |
| Original `main`, now `codex/archive-js-prototype` | `a55db3e` | Archived Electron / Three.js prototype |
| `origin/dev` | `8ae6325` | Later JS changes; history mentions physics, walls, map, and lighting improvements |
| `origin/C++` | `65f2efa` | Native engine skeleton, commit message: `moving to C++` |

These references were inspected locally. No remote refresh was performed, so this is not a claim about the latest GitHub state. No common ancestor was found between `main` and `origin/C++` in the available local history.

## Existing JavaScript implementation

Historical reference only; these files have been removed from the active tree.

Source inspection found:

- Electron desktop entry point and Three.js rendering.
- World geometry, obstacles, a player, and enemies.
- Third-person camera and input handling.
- Movement, jumping, health, targeting, and combat ability code.
- Slash, bolt, dash, and heal abilities with cooldowns.
- HUD and character, inventory, quest, and map panel code.

These are source-level findings, not confirmation that every feature or panel is complete or works at runtime. The JS application was not launched during this review.

## Existing native implementation

Imported from `origin/C++` without generated binaries or build caches:

- CMake project targeting C++17 and an `RPGNative` Windows executable.
- Win32 window creation and message processing.
- DirectX 11 device, swap chain, render target, and viewport setup.
- Frame timing, game update/render calls, framebuffer clearing, and presentation.

The original empty game methods have now been replaced. The working source contains:

- Runtime-compiled HLSL shaders, reusable indexed curved meshes, generated material layers, instanced shadow/color passes, camera/light culling, depth testing, sun/hemisphere lighting, fog, a generated sky, an installed-font atlas, and alpha-blended HUD batching.
- A floating island with named village/dock/training/camp areas, an irregular cliff edge, pitched timber/plaster houses, branched trees, banners, distant fragments, a quest beacon, shaped characters, and three enemies.
- Camera-relative movement, sprinting, jumping, collision against buildings/trees/island bounds, mouse orbit, zoom, and camera obstruction checks.
- Target cycling, melee range and line-of-sight checks, strike, heavy strike, dash, mend, cooldowns, enemy pursuit/return behavior, damage, death, and restart.
- A patrol acceptance/kill/return/reward loop with protection against duplicate rewards and support for a kill before quest acceptance.
- Health/target/ability HUD with availability reasons, live map and loot markers, quest view, character statistics, potion use, and sword switching.
- Static-grid A* navigation with path smoothing, enemy behavior states and home leashes, soft enemy separation, and stable enemy IDs.
- One-time loot, potion cooldowns, reward sword and ward armor, two quests, derived XP/levels and interruptible attack windups.
- A simulation-owned 24-cell bag, six worn slots, drag swaps/equip/remove, stack splitting/merging, sorting, exact-cell right-click actions, comparisons and shared outfit visuals. Version 6 preserves placement/equipment, purchases and buyback, migrating versions 1–5.
- A quartermaster, solid supply counter, orange map marker, limited potion/helm/shield stock, validated purchases/sales and persistent buyback in a shop that coexists with character/bag windows. Portrait arrows rotate the actual worn outfit.
- A repeatable 30-second sparring opponent after the campaign, with real armor effects, nonlethal damage, result reports and no rewards or resource refunds.
- Versioned local save/load with bounds checking and transactional simulation replacement; invalid data does not replace the current simulation. Save files are replaced only after a successful temporary-file write.
- Borderless fullscreen, windowed restoration, per-monitor DPI awareness, focus/minimize suspension, a live Escape menu, DirectX error reporting, and shutdown handling.

Compilation, domain regression tests, and hidden rendering now validate parts of this implementation. Automated tests do not certify the complete interactive experience. Art now has a first procedural surface/silhouette pass with bounded memory; it remains stylized prototype art, not photorealism or finished Allods-style art. See [graphics and measurements](docs/graphics.md).

## Known gaps

- The historical C++ branch tracks generated files under `build/`, `build_vs/`, and `out/build/`. Those files were not imported into the working checkout.
- Real keyboard/mouse interaction, focus/minimize behavior, all panels, and full gameplay feel still need the manual checklist.
- No sound, skeletal animation, imported art or networking. Stationary Release GPU measurements exist; moving-camera, large-encounter and long-session performance remain unverified.
- Navigation uses a static grid and is suited to this small map; there is no general navmesh or crowd solver. Enemy separation is soft, and player/enemy collision is not a full rigid-body system.
- Decorative dock rails, banners, and training targets are non-solid. Trees have trunk-based collision and conservative camera bounds.
- The catalog has two swords, one potion type, tunic, trousers, boots, ward armor, helm and shield. Six worn slots, drag/sort/splitting, limited purchases, selling and buyback work. Permanent disposal, active shield blocking, more items and progression beyond level 3 remain pending.
- Camera, target, panel state, jump height, and transient hit flashes are not saved.
- World geometry is instanced and F3 includes asynchronous GPU timing. Broader visible-gameplay profiling and final art acceptance remain pending.

## Development environment

- Visual Studio Community 2022 version 17.13.3 was detected.
- The initial inspection found no usable toolchain. A later inspection found installed CMake 3.30.5-msvc23, MSVC 19.43.34809.0, and Windows SDK 10.0.22621.0, which now build the project successfully.
- MSBuild required execution outside the restricted sandbox to read its SDK-discovery directory. This did not install software or change system settings.
- No tools were installed or downloaded by the agent.

## Completed in this session

- Read project instructions in `AGENTS.md`.
- Removed the local `.vs/` cache at the user's request and added `.vs/` to `.gitignore`.
- Inspected the current working tree and locally available branch references.
- Reviewed the native skeleton and identified the gap between engine initialization and gameplay migration.
- Created `roadmap.md` and this progress record in English.

## Next validation milestone

Run the interactive checklist in `tests/manual-smoke.md`, record input/combat/camera/panel behavior, and fix observed problems before expanding beyond this map.

The latest bug-fix pass adds reproduced regressions for stale panel/load input, camera gesture leakage, the unarmed starter-sword hotkey and impossible patrol save states. All fixes pass Debug/Release validation; actual desktop input and monitor behavior remain manual checks.

## 2026-10-17: native development baseline

- Prioritized Allods Online and defined a small floating-island outpost slice in the roadmap.
- Imported only CMake and native source files from `origin/C++`; preserved the JS prototype, current branch, and pre-existing staged changes.
- Ignored generated build directories.
- Added a Windows platform guard to CMake and a PowerShell build/run helper for Visual Studio 2022.
- Checked render-target creation failures and added a retry without the debug layer when the SDK debug component is missing.
- Stopped frame processing after quit, paused rendering while minimized, and capped frame delta at 50 ms.
- Build-tool inspection still found no available CMake or registered MSVC toolchain. No installation or download was performed.
- Validation: PowerShell parser reported no errors in the build helper. Running `.\scripts\build-native.ps1 -Configuration Release` stopped at its prerequisite check with `CMake was not found`; no compilation or executable launch occurred.

## 2026-10-17: first native gameplay slice

- Implemented the renderer, input, gameplay simulation, scene, HUD, supporting panels, quest, and persistence described above.
- Added `tests/SimulationTests.cpp` and CTest integration for collision, camera obstruction, invalid attacks, jumping, quest/reward progression, save/load, malformed data, death/restart, and invalid timesteps.
- Added `-Test` to the native build helper and documented the controls and smoke-test procedure.
- Validation performed: PowerShell parser accepted the helper; `git diff --check` passed; reviewed source wiring and edge cases.
- Build/test attempt: `.\scripts\build-native.ps1 -Configuration Debug -Test` stopped at `CMake was not found`. Neither the C++ tests nor the application ran.
- Additional environment checks found no compiler on PATH, no MSVC Tools directory in the installed VS instance, and no registered WSL distribution. No tools were installed or downloaded.

Next: compile both configurations, execute CTest, fix compiler/runtime findings, and run `tests/manual-smoke.md` before extending the slice further. Visual quality, combat feel, and performance must be judged in a running build.

The planned feature sequence is maintained in [roadmap.md](roadmap.md).

## 2026-10-17: native-only repository migration

- Created the local `codex/archive-js-prototype` branch at `a55db3e` before removing the JS files. Verified that those files had no local modifications.
- Removed the legacy `src/`, Electron entry point, HTML UI, and npm manifests from the active tree.
- Preserved all current native source files, build helper, tests, `AGENTS.md`, `roadmap.md`, `progress.md`, and `LICENSE`.
- Kept `main` as the active development branch. The unrelated historical `origin/C++` branch was not checked out or merged.
- Updated the README with native-only instructions and Visual Studio Open Folder guidance.
- This migration changes repository organization only; native build/runtime validation is still pending. No dependencies were downloaded and no remote push was performed.

## 2026-10-17: start and debug manual

- Added `START_AND_DEBUG.md` with Visual Studio folder/solution workflows, build/test/run commands, breakpoint guidance, controls, save-file behavior, and troubleshooting.
- Linked the manual from `README.md`. Instructions were checked against the current build helper and source; this documentation update does not constitute a successful build or runtime test.

## 2026-10-17: validated map and gameplay expansion

- Separated platform-independent simulation/world data into `MediterraneanSimulation`; extracted map presentation into `WorldView`.
- Added shared tree/building collision, named map areas, A* routes with clearance checks and smoothing, explicit enemy states, home return, and soft enemy separation.
- Implemented one-time corpse loot, healing potions (Q), the watchguard blade reward and equipment switching (G), availability feedback, and a debug overlay (F3).
- Introduced version 2 saves with stable enemy IDs, behavior/loot flags, inventory/equipment validation, and migration of valid version 1 saves.
- Added pyramid meshes, house trim, banners, dock and arena props, simple limb motion, and batched HUD/text geometry.
- Added hidden `--smoke-test` rendering and PNG capture; the PowerShell helper exposes it through `-SmokeTest`.
- Fixed issues found during validation: a missing lambda capture and a WIC pixel-format mismatch. Final runs pass.

| Validation | Debug | Release |
| --- | --- | --- |
| Native build | PASS | PASS |
| SimulationTests | PASS | PASS |
| NavigationTests | PASS | PASS |
| InventoryTests | PASS | PASS |
| DirectX device/shaders, resize, rendering, PNG readback | PASS | PASS |

Commands executed: `./scripts/build-native.ps1 -Configuration Debug -Test -SmokeTest` and the same command with `Release`. The render test exercises 960x600, 1600x900, and 1280x720 targets, then captures the final size. Captures and logs live in ignored `build-local/Debug-smoke.png` and `build-local/Release-smoke.png` (with `.log` appended for results). The Debug capture was visually inspected after HUD batching.

Regression coverage includes the original patrol/combat/save checks, routing around buildings, a pursuing enemy navigating around a tower, one-build command replay consistency, one-time loot, potion consumption/cooldowns, equipment damage, legacy save migration, and rejection of unknown enemy IDs. This is not a claim of cross-platform deterministic networking or complete manual QA.

No commits, amendments, pushes, downloads, or system-setting changes were made during this development pass. User edits to `AGENTS.md` were preserved.

## 2026-10-17: Visual Studio startup troubleshooting

- The reported toolbar showed `RPGNative.sln` as the launch item, and Solution Explorer had an active search filter. Documented how to open the generated solution through **File > Open > Project/Solution**, clear the search, and select the `RPGNative` C++ startup project.
- Confirmed the folder-mode CMake cache identifies the installed MSVC compiler and exposes `RPGNative` as an executable target. Confirmed `build-local/RPGNative.sln` and `build-local/Debug/RPGNative.exe` exist, with a passing render smoke-test log from the prior build.
- Added an immediate existing-executable launch path to `START_AND_DEBUG.md`. No source, IDE settings, or system settings were changed; this documentation check does not verify an interactive Visual Studio launch.

## 2026-10-17: two-quest campaign and readable combat

- Added committed enemy windups, visible attack cones, delayed impact checks, heavy-strike interrupts, and a distinct captain with 120 health and a longer, heavier swing. Returning enemies cannot be targeted or damaged.
- Added bounded floating combat/XP feedback, player hit flash, level and XP bars, a persistent objective tracker, interaction prompts, channel progress, and updated journal/map/character/inventory panels.
- Extended the patrol into the astral-anchor quest: clear three enemies, complete an interruptible three-second channel, return for a unique reward. Added levels 1–3, level health/damage bonuses, and automatically equipped ward armor. XP derives from durable objective flags.
- Added Enter recovery after death that retains progression, inventory, and dead/looted enemies while restoring living enemies at home. R remains the explicit full reset.
- Introduced save version 3 with anchor state and committed enemy swings. Versions 1/2 migrate, preserving third-enemy health fraction and deriving progression from prior accomplishments. Invalid data is rejected transactionally.
- Added combat and progression regression suites. The latter completes both quests through real movement/combat/interaction commands, then tests channel interruption, saved progress, duplicate rewards, respawn, migration, and restart.
- Added `-Backend Ninja` to the existing build helper, with a child-process Visual Studio compiler environment and separate build directories. Added a raw localized MSVC include-prefix probe after observing CMake 3.30 corrupt the Russian prefix; Ninja dependency records now include `Simulation.h` and `WorldMap.h` correctly.
- Extended hidden rendering to four authored scenarios at three sizes. Snapshot loading stays in memory and never overwrites player saves.

| Validation | Debug | Release |
| --- | --- | --- |
| Ninja/MSVC native build | PASS | PASS |
| SimulationTests | PASS | PASS |
| NavigationTests | PASS | PASS |
| InventoryTests | PASS | PASS |
| CombatTests | PASS | PASS |
| ProgressionTests | PASS | PASS |
| Spawn, raider, captain, veteran captures at 960x600 / 1280x720 / 1600x900 | PASS | PASS |

Commands: `./scripts/build-native.ps1 -Backend Ninja -Configuration Debug -Test -SmokeTest` and the same command with `Release`. Debug was rebuilt cleanly after the dependency-prefix fix. Logs are in `build-ninja-debug/validation.log` and `build-ninja-debug/release-validation.log`. Captures are in the corresponding `build-ninja-debug` / `build-ninja-release` directories. Spawn/veteran captures at 960x600 and raider/captain captures at 1280x720 were visually inspected. Latest executables are `build-ninja-debug/RPGNative.exe` and `build-ninja-release/RPGNative.exe`.

The initial MSBuild request could not run through automatic approval review (HTTP 403, then timeout); a restricted MSBuild attempt failed on its per-user SDK lookup. Validation succeeded with bundled Ninja under the existing sandbox permissions, without changing permissions or system settings. The older `build-local` executable was not replaced; rebuild the generated solution before using its output for these changes.

Updated README, launch/debug manual, gameplay rules, architecture notes, roadmap, and the manual checklist. Full interactive playtesting, finished art, sound, and frame profiling remain pending. Ward armor is awarded after the existing enemies are dead; repeatable encounters are future work. No commits, amendments, staging, pushes, downloads, or system-setting changes were made in this pass. `AGENTS.md` was not modified.

## 2026-10-17: procedural graphics with bounded memory

- Replaced the visible box/pyramid blockout with an indexed reusable mesh library: shaped bodies, articulated limbs, heads, boots, blades, pitched roofs, irregular stonework, branched trees and a joined island cliff skirt. Kept floor height and gameplay obstacle footprints unchanged.
- Generated eleven 128-square material layers with mipmaps at startup. Added world-space surface detail, a dirt/grass blend, hemisphere fill, directional sun, filtered 1024-square shadows, fog and a generated sky. No art datasets or downloaded assets were used.
- Batched world instances by mesh with separate camera/light culling and one bounded instance upload. Removed shadow casting from emissive and interface markers. Added asynchronous GPU timestamps and the repeatable `-Benchmark` / `--benchmark` path.
- Added F2 interface hiding and updated F3 to show completed-frame draw calls, GPU milliseconds and mesh KiB. Pause, panels and death restore essential HUD visibility while active.
- Added GeometryTests covering valid indices, finite normals, winding, nondegenerate triangles, collision-floor coverage, joined cliff boundaries, frustum culling and allocation budgets. The tests exposed incorrect smoothing across trunk/limb caps; separate cap vertices fixed it. Visual inspection exposed overlapping armor; the torso now uses the armor material directly.
- Extended each render fixture with overview and front-character captures. Debug smoke tests inspect DirectX warning/error messages when the optional debug layer is available. Expanded camera bounds to cover pitched roofs/chimneys and tree crowns, with regressions confirming actor traversal underneath foliage is unaffected.
- Updated README, architecture, roadmap, manual checklist and START_AND_DEBUG.md. Added docs/graphics.md with renderer budgets, benchmark methodology, the old/new comparison and remaining art work.

Validation: Ninja/MSVC Debug and Release builds pass all six CTest suites and all four render scenarios at 960x600, 1280x720 and 1600x900, plus two inspection angles per scenario. Representative village, combat, captain, veteran and front/overview captures were visually inspected. Logs: `build-ninja-debug/graphics-validation.log` and `build-ninja-release/procedural-final-validation.log`. Captures remain under those ignored build directories.

Measured spawn submissions fell from 186 to 26. Material/shadow allocation is 5,155,484 bytes (4.917 MiB); indexed meshes use 70,496 bytes. GPU work is higher than the blockout because geometry, surface sampling and a shadow pass were added. Exact CPU/GPU/memory results and the hidden-window limitations are recorded in [docs/graphics.md](docs/graphics.md). They are not a gameplay FPS guarantee.

Interactive playtesting, authored character quality, foliage refinement, skeletal animation, audio and long-session performance remain pending. No commits, staging, pushes, downloads, or system-setting changes were made. AGENTS.md and the earlier gameplay work were preserved.

## 2026-10-17: fullscreen and RPG interface

- Gameplay opens in borderless fullscreen without changing desktop display settings. F11 and the menu Display button restore/toggle the previous window placement. Added process-local per-monitor DPI awareness. Hidden tests verify monitor-sized client bounds, hidden visibility and exact windowed client restoration.
- Replaced Escape pause with a live clickable menu: Return, Character, Inventory, Atlas, Journal, Display, Save, Load and Exit. Every panel captures local movement/camera/combat keys while simulation continues. Tests prove committed enemy attacks still damage the player behind each panel.
- Added a compact HUD, minimap, readable objective tracker, clickable action bar and hover descriptions. Replaced the old pixel glyphs with an atlas rasterized from the installed Windows font; added alpha-blended interface batching.
- Added a 24-slot belongings view, original item icons, equipped markers, stack counts, hover details and selected-item actions. Equip/Drink issue real validated simulation commands. Sword changes survive the existing version 3 save format.
- Added equipment slots and a lit 3D preview using the same dressed CharacterView as the world. The reward sword has a distinct finish; ward armor appears when earned. Empty head/off-hand slots and baseline clothes are represented honestly; additional armor selection and drag sorting remain future work.
- Added a generated illustrated atlas shared with the minimap, preserving world proportions. Terrain, coastline, roads, rooftops, tree crowns, ruins and dock follow project data. Live markers and player facing overlay the art; traversable-ground waypoints can be placed and cleared without affecting saves.
- Added a two-card quest journal, menu save/load feedback and seven-suite validation through a shared MediterraneanClient library. InterfaceTests covers live menus, item ownership/use, equipment persistence, scaled real-renderer pointer routing at 960x600/1280x720/2560x1080, modal click isolation and map coordinates.
- Researched Allods/WoW interface guides and indexed image references. Source links and access limitations are recorded in docs/interface.md. No reference assets or font files were downloaded/copied. All new artwork is generated locally from code and world data.

Validation: Debug and Release native builds, all seven CTest suites and the expanded hidden fullscreen/world/interface smoke test pass. Each of four scenarios has three gameplay sizes, two inspection views and five panels at three sizes (80 captures per configuration). Representative menu, inventory, equipment, atlas and journal captures were visually inspected, including 960x600. Latest logs are `build-ninja-debug/interface-validation.log` and `build-ninja-release/interface-validation.log`.

The font/map textures add 1,529,172 bytes (about 1.46 MiB). The current stationary Release report records 25 draw submissions, 0.123 ms CPU median and 0.127 ms GPU median; this hidden scene is not an interactive/fullscreen FPS guarantee. See docs/interface.md for the full limits and benchmark conditions.

Updated README, START_AND_DEBUG.md, architecture notes, roadmap and the manual checklist. Real mouse/keyboard use, Alt-Tab, multiple physical monitors, display-scaling changes and long sessions remain manual checks. No commits, staging, pushes, downloads or system-setting changes were made. AGENTS.md and prior gameplay work remain preserved.

## 2026-10-17: Allods screenshot interface pass

- Used the user-supplied character/bag screenshot as the visual layout reference: green/gold carved framing, beveled title plaques, serif text, compact item cells, a left character alcove with adjacent attributes, right-hand bag, circular minimap and a bottom combat dock.
- Replaced the exclusive character/inventory overlay with independent side windows. C then I opens both without dimming the world. Each closes independently; Esc opens the live menu and preserves the side-window state. Selecting worn gear opens its details in the bag.
- Side windows allow keyboard movement/combat and preserve camera orbit over the visible world. Their surfaces consume pointer input; menu/map/journal remain modal for player commands. Simulation time continues in every case.
- Kept item ownership, Equip/Drink actions, character appearance and stats connected to the actual simulation. The grid has 24 inspection cells and only owned items; four engraved action-bar cells are unbound. Arbitrary item storage, drag sorting, movable windows and finished item/character illustrations remain future work.
- Added original code-drawn rails, leaf inlays, cloth backdrop and item-slot treatments. Switched the small installed-font atlas to Georgia. Reused the existing map texture for the circular minimap; texture memory remains 1,529,172 bytes for the font and map together. No assets or dependencies were downloaded.
- Extended InterfaceTests for coexisting windows, independent closing, menu restoration, scaled item/close/action clicks, and camera-pointer capture versus the world gap. Hidden smoke captures now include combined equipment/bag at all three sizes (92 captures per configuration).

Validation: Ninja/MSVC Debug and Release pass all seven CTest suites and the four hidden fullscreen/world/UI scenarios. Representative normal HUD, combined side-window and central overlay captures were inspected, including 960x600. Logs: `build-ninja-debug/allods-interface-validation.log` and `build-ninja-release/allods-interface-validation.log`. Combined previews use the `-equipment-bag.png` suffix.

Release stationary HUD benchmark at 1280x720: 26 draw submissions, CPU median 0.264 ms / p95 0.317 ms, GPU median 0.173 ms / p95 0.178 ms (240 GPU samples); working set 51.824 MiB, private bytes 76.105 MiB. This hidden-window HUD workload does not measure combined-window cost or guarantee visible-gameplay FPS. Real input, Alt-Tab, multiple monitors and long sessions remain on the manual checklist.

Repository HEAD changed to `6144929` during this pass through an external action. The agent made no commits or pushes; remaining working-tree edits are left for user review.

## 2026-10-17: real equipment, bag storage and repeatable sparring

- Added a platform-independent Inventory model with one canonical item catalog, 24 bag cells, stack limits and four worn slots. Equipped items have one location. Additions, moves and equipment changes validate capacity, compatibility and expected source items before mutation.
- Connected the Allods-inspired side windows to real storage: drag cell swaps, drag equip/remove, right-click actions, Sort, selected-item comparisons, feedback and a drag preview. Invalid drops preserve state. Escape, closing panels, focus loss, minimization and display changes cancel the drag. Movement remains available behind the side windows.
- Made the tunic, trousers, boots, swords and ward freely equipable/removable. Damage/protection derive from worn gear; unarmed base damage is 6/10, starting clothes add no armor, and ward reduces damage by 2 only when worn. Reward ownership and earned XP remain independent of outfit selection.
- Introduced version-4 saves with exact bag/gear serialization and transactional rejection of unknown items, duplicate unique gear, bad stacks, wrong slots and inconsistent rewards. Versions 1–3 migrate with their potions, rewards and sword selection preserved. Old campaign fixtures remain useful migration coverage.
- Refined character proportions, tunic layering, armor crest/shoulders and boots. World and preview use the same outfit. Empty weapon/feet slots remove those visuals; neutral underclothes remain when clothing is removed. Geometry/material resources are reused; no asset download or new texture allocation was needed.
- Added a dedicated 180-HP sparring guard at the eastern training post after both quests. Rounds last 30 seconds, use the real combat/armor rules, stop through E/arena exit and end nonlethally at 1 HP. Reports show damage dealt/taken. No XP, gold, loot or campaign credit is granted and resources are not refunded. Practice is transient across save/load; health, consumed potions, cooldowns and gear persist.
- Added training markers, prompts and a live round/result panel. Nearby unclaimed campaign loot takes precedence over starting practice. Kept all development on the existing map.
- The new sparring capture exposed a camera collapse beside a tree. Camera collision now uses trunk bounds below the canopy and wider crown bounds above it. Regression checks retain crown/roof obstruction while allowing a clear retreat underneath foliage. The corrected training view was visually inspected.

Validation: Debug and Release Ninja/MSVC builds pass all eight CTest suites. Expanded InventoryTests covers capacity, swaps, stale commands, sorting, unarmed damage, migration and malformed version-4 saves. InterfaceTests exercises scaled drag/drop, right-click, invalid drops, cancellation and round trips at 960x600, 1280x720 and 2560x1080. TrainingTests verifies actual armored/unarmored hits (6/8), victory, stop/restart, timeout, arena exit, nonlethal yield, consumable costs and no farming.

Both configurations pass 96 hidden smoke captures: the existing 92 world/interface views plus veteran base/travel outfits and sparring with/without side windows. InterfaceTests also captures a bag drag separately. Representative combined, outfit, drag, small-window and corrected sparring views were inspected. Latest logs: `build-ninja-debug/inventory-validation.log` and `build-ninja-release/inventory-validation.log`. Latest executables: `build-ninja-debug/RPGNative.exe` and `build-ninja-release/RPGNative.exe`. The older generated Visual Studio solution output still needs rebuilding before use.

Release stationary HUD measurement at 1280x720: 26 draw submissions, 112,348 submitted triangles, CPU median/p95 0.281/0.316 ms, GPU median/p95 0.178/0.181 ms (240 samples), working set 52.371 MiB and private bytes 75.574 MiB. Mesh bytes remain 70,496; materials/shadows 5,155,484; font/map textures 1,529,172. This hidden stationary workload does not measure moving cameras, combined windows, practice combat or long-session performance.

Updated README, launch/debug manual, gameplay/interface/architecture/graphics notes, roadmap and manual checklist. Real desktop input, focus/display transitions, combat feel and finished art remain pending. No commits, staging, pushes, downloads or system changes were made. AGENTS.md and the prior development remain preserved.

## 2026-10-17: input and save bug fixes

- Reproduced right-clicks reaching old bag cells immediately after opening the menu. Panel changes now invalidate hit regions; dispatch also checks rendered state/dimensions. Closing a menu cannot activate its invisible Exit button, loading cannot use old equipment hit regions, and resizing cannot complete a stale drop. HUD visibility changes invalidate hits as well.
- Reproduced camera rotation after an RMB gesture began in the bag and crossed into the world. Orbit now requires a fresh RMB press over the world; side windows and interactive HUD controls capture their gestures. Focus, resize/fullscreen, load and restart cancel orbit ownership along with item dragging. A new world gesture still rotates normally.
- Reproduced G failing to re-equip an unequipped starter sword before the patrol reward. It now restores that owned sword while retaining the normal reward-sword toggle after hand-in.
- Reproduced a load accepting an active patrol with an already cleared camp, leaving no kill available to advance the quest. Legacy and version-4 loads now reject that inconsistent state transactionally. Valid kills before quest acceptance and cleared patrols awaiting hand-in remain supported.
- Separated Win32 sampling from input transition calculation through `Input::Frame`, so tests exercise the production input/Game path without synthesizing desktop input. Added focus loss/restoration, bag/minimap-to-world orbit, fresh orbit, load, stale menu/gear hits, resize cancellation, starter re-equip and invalid quest-save coverage.

Reproduction evidence: `build-ninja-debug/bug-reproduction.log` failed on the stale menu hit and starter hotkey; `input-bug-reproduction.log` failed on camera gesture leakage; `save-bug-reproduction.log` failed on inconsistent quest state. After the fixes, Debug and Release pass all eight suites and all 96 smoke captures per configuration. Final logs: `build-ninja-debug/bugfix-validation.log` and `build-ninja-release/bugfix-validation.log`. Both `RPGNative.exe` outputs were rebuilt, and `git diff --check` passes.

Updated interface/gameplay/architecture notes, roadmap, launch manual and the interactive checklist. The automated input checks do not claim an actual Alt-Tab/multi-monitor playtest. Prior work and AGENTS.md are preserved. No commits, staging, pushes, downloads or system changes were made.

## 2026-10-17: quartermaster, six-slot equipment and persistent trading

- Continued the roadmap's village/equipment milestone on the existing island, guided by the supplied Allods character/bag composition. Added a central green/gold quartermaster window alongside the independent character and bag windows, with stock, prices, effects, availability reasons and purchase feedback. E opens/closes trade; the world stays live while local movement/attacks are captured. Disabled combat slots are visibly dimmed while shopping.
- Added a village quartermaster at `(3, -12)`, an orange atlas/minimap marker, and a wooden supply counter with bottles. The counter is solid shared map data, so navigation, movement, camera queries and map artwork agree. The NPC and counter reuse existing resources.
- Added a limited offer catalog: six draughts at 6 gold each, a helm at 20, and a shield at 35. Gear requires the patrol hand-in. Simulation validates range, life/grounded state, quest, stock, gold and capacity before updating item, charge and purchase count together. Purchases are stowed first. No selling, refund or restock exists; death recovery and loading preserve spending/stock, while R resets the expedition.
- Activated Head and Off hand slots. The helm adds 1 protection and the shield 2 while worn; with ward armor, an actual normal 8-point hit becomes 3. Both items appear in the world and portrait. Drag/right-click equip/remove, comparisons and exact placement use the existing inventory rules. Added portrait rotation arrows and Front reset.
- Introduced save version 5 with bounded SHOP receipts and six-slot equipment. Validate gold against earned rewards minus purchases, stock limits and actual item ownership. Versions 1–4 migrate; version 4 retains exact previous cells/worn slots and starts with empty new slots. An older actor position inside the newly solid counter moves to its nearest free edge instead of losing access to that save. Current saves inside solid geometry remain invalid.
- Added TradeTests for actual charges/items, sold-out and insufficient-gold rejection, quest/range/death/jump checks, fully armored combat, purchase persistence, forged receipts, stock bounds, respawn/reset and v4/v5 migration. NavigationTests checks routing around the counter. InterfaceTests drives shop opening, purchase and E close through sampled Game input, and scaled buy/drag/rotation controls with all three windows visible.
- Added eight veteran render captures: shop and purchased outfit at three sizes, a world close-up and equipped sparring. Inspected representative shop/gear/portrait views at 960x600 and 1280x720. The dressed world capture now uses clear ground and faces the player, avoiding overlap with the merchant during inspection.

Validation: Debug and Release Ninja/MSVC builds pass all nine CTest suites and 104 hidden smoke captures per configuration. Logs are `build-ninja-debug/quartermaster-validation.log` and `build-ninja-release/quartermaster-validation.log`; updated executables are in the same directories. The separate `inventory-drag.png` integration capture is not included in the 104. Player saves were not overwritten. `git diff --check` passes.

Release stationary HUD benchmark: 26 draw submissions, 125,948 submitted triangles, CPU median/p95 0.293/0.355 ms, GPU median/p95 0.179/0.183 ms (240 samples), working set 52.055 MiB, private bytes 75.711 MiB. Meshes remain 70,496 bytes, materials/shadows 5,155,484, font/map textures 1,529,172. This hidden stationary workload does not measure interactive FPS, shop-window cost, movement spikes or long sessions.

Updated README, START_AND_DEBUG, gameplay/interface/architecture/graphics notes, roadmap and the manual checklist. Official Allods/Blizzard merchant and equipment references were checked through indexed excerpts and linked in docs/interface.md; rules/prices and code-drawn artwork here are original. Real desktop input, Alt-Tab/monitor behavior, combat feel and finished character/environment art remain pending. No assets/dependencies were downloaded or system settings changed. The agent performed no commits, staging or pushes. HEAD changed externally to `e6acae0` during the pass; that state was preserved. AGENTS.md and earlier development remain preserved.

## 2026-10-18: fullscreen drop cancellation and defeat recovery

- Reproduced an F11/release race: Game applied the item drop before Engine cancelled gestures for the fullscreen transition. Game now cancels gestures and invalidates hit regions before dispatching pointer input on the F11 frame. The item stays in its original location; a fresh drag afterward equips normally.
- Reproduced Enter being swallowed by the menu, atlas and journal while defeated. Recovery now bypasses central-panel command capture only while health is zero. It uses the existing progress-preserving respawn, retaining quests/items/gold instead of resetting the expedition.
- Defeat now closes the current central panel and cancels drag/orbit ownership, revealing the recovery prompt while preserving side-window state. Players can reopen a menu after defeat and still use Enter. The rendered `defeat-recovery.png` was visually inspected.
- Extended InterfaceTests with sampled Game input for simultaneous F11/drop, a subsequent fresh drag, death during each central overlay, reopening the overlay while defeated, and progress-preserving Enter recovery. The capture is separate from the 104 scenario images and does not write a player save.

Reproduction logs: `build-ninja-debug/fullscreen-drop-reproduction.log` fails on the unintended equip; after that fix, `panel-recovery-reproduction.log` fails on swallowed Enter. The original nine suites passed before adding these cases (`bugfix-2026-10-18-baseline.log`), showing the missing coverage.

Final validation: Debug and Release Ninja/MSVC builds pass all nine suites and 104 hidden render captures each. Logs: `build-ninja-debug/bugfix-2026-10-18-validation.log` and `build-ninja-release/bugfix-2026-10-18-validation.log`. Both executables were rebuilt. `git diff --check` passes. No renderer resources or save format changed; the previous benchmark was not repeated for these input-only changes.

Updated the launch guide, interface/gameplay notes, roadmap and manual checklist. Automated sampled-input and hidden rendering checks do not certify physical F11/Alt-Tab, DPI, multiple monitors or combat feel; those remain manual. Existing local documentation edits and AGENTS.md were preserved. HEAD remains `e6acae0`; all new fixes are uncommitted. No commits, staging, pushes, downloads or system changes were performed.

## 2026-10-18: stack splitting, selling and persistent buyback

- Extended the existing-map equipment/trading milestone with Split half, explicit Sell one actions and a nine-cell buyback shelf. The compact merchant offers and buyback fit between the Allods-inspired character and bag windows. Shelf tooltips show the item, held count, price and eligibility. Right-click retains equip/remove/drink behavior; worn gear must be removed before selling.
- Added validated stack splitting with exact bag-cell selection. Split half moves the floor of half the source count into an empty cell; dragging matching stacks together and Sort merge them. Drinking/selling targets the selected stack, while Q retains first-available consumption. Stale sources cannot consume another stack or grant free healing.
- Added original sale prices for every catalog item and simulation-owned sell/reclaim transactions. Selling transfers one item to bounded merchant holdings and adds its price; buyback returns one to the bag at the same price. Holdings survive leaving, save/load and respawn without expiry; R clears them. Selling does not restock retail offers or change quest XP. Insufficient gold leaves buyback items available for later. Repeated sell/reclaim cycles preserve gold and item counts.
- Introduced version 6 with a BUYBACK record. Save validation reconciles exact gold with campaign income minus retail purchases plus the value of merchant-held items. Player-owned plus merchant-held unique gear must match earned/purchased entitlements, and remaining potions cannot exceed supply. This permits legitimate sale proceeds above 90 gold without accepting invented credits. Versions 1–5 migrate; v4/v5 retain their exact cells and existing gear/purchases with empty buyback. G no longer reports equipping a sold sword.
- Extended InventoryTests with odd splits, full bags, invalid counts/destinations, stale consumption, merging and exact-stack save/healing. TradeTests covers all catalog items through repeated sale/reclaim cycles, missing/worn/stale item rejection, unaffordable buyback, persistent sold quest gear, XP retention, forged but balanced credits, duplicate ownership, range/death/jump rejection, respawn/reset and v4–v6 migration. InterfaceTests exercises the production Game input path and scaled split/sell/buyback/right-click routing at 960x600, 1280x720 and 2560x1080.
- Added three veteran buyback/split-stack captures. Inspected Debug merchant/buyback at 1280x720 and buyback at 960x600. All three windows, gold, shelf prices and bag actions fit. No new mesh, material, texture or asset dependency was introduced. The previous stationary benchmark was not repeated; merchant-window performance and interactive input remain manual validation work.

Validation: Debug and Release Ninja/MSVC builds pass all nine CTest suites and 107 hidden smoke captures per configuration. Logs: `build-ninja-debug/trading-2026-10-18-validation.log` and `build-ninja-release/trading-2026-10-18-validation.log`. Both executables were rebuilt. The separate drag and defeat-recovery integration captures are outside the 107 scenario images. Player saves were not overwritten. `git diff --check` passes.

Updated README, START_AND_DEBUG, roadmap, gameplay/interface/architecture/graphics notes and the manual checklist. Existing local input fixes, documentation and AGENTS.md were preserved. HEAD remains `e6acae0`. All changes remain uncommitted; no staging, commits, pushes, downloads or system changes were performed.
