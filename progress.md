# Mediterranean Progress

Last updated: 2026-10-17

## Current status

The active `main` working tree is native C++ / DirectX 11 only. Allods Online is the primary reference; development is single-player first on one map. **Debug and Release Ninja/MSVC builds pass, all seven regression suites pass in both, and four hidden render scenarios pass at three sizes plus overview/front-camera views and all five interface panels.** Representative captures were visually inspected. The map now has two linked quests, levels 1–3, a captain, telegraphed combat, and progress-preserving respawn. Full interactive playtesting and finished art remain pending. This development pass is uncommitted and has not been pushed.

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
- One-time loot, potion cooldowns, reward sword and ward armor, two quests, derived XP/levels, interruptible attack windups, and version 3 saves with version 1/2 migration.
- Versioned local save/load with bounds checking and transactional simulation replacement; invalid data does not replace the current simulation. Save files are replaced only after a successful temporary-file write.
- Borderless fullscreen, windowed restoration, per-monitor DPI awareness, focus/minimize suspension, a live Escape menu, DirectX error reporting, and shutdown handling.

Compilation, domain regression tests, and hidden rendering now validate parts of this implementation. Automated tests do not certify the complete interactive experience. Art now has a first procedural surface/silhouette pass with bounded memory; it remains stylized prototype art, not photorealism or finished Allods-style art. See [graphics and measurements](docs/graphics.md).

## Known gaps

- The historical C++ branch tracks generated files under `build/`, `build_vs/`, and `out/build/`. Those files were not imported into the working checkout.
- Real keyboard/mouse interaction, focus/minimize behavior, all panels, and full gameplay feel still need the manual checklist.
- No sound, skeletal animation, imported art or networking. Stationary Release GPU measurements exist; moving-camera, large-encounter and long-session performance remain unverified.
- Navigation uses a static grid and is suited to this small map; there is no general navmesh or crowd solver. Enemy separation is soft, and player/enemy collision is not a full rigid-body system.
- Decorative dock rails, banners, and training targets are non-solid. Trees have trunk-based collision and conservative camera bounds.
- Inventory now has a clickable 24-slot belongings view over the existing two swords, healing potions, starting outfit and automatic quest armor. Arbitrary item storage, drag sorting, selectable additional armor, trading and progression beyond level 3 remain pending.
- Camera, target, panel state, jump height, and transient hit flashes are not saved.
- World geometry still uses per-object draw calls; the HUD is batched. F3 reports draw submissions, not GPU timing. GPU profiling and world instancing remain future work.

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
