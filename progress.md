# Mediterranean Progress

Last updated: 2026-10-17

## Current status

The active `main` working tree is native C++ / DirectX 11 only. Allods Online is the primary setting, visual, and mechanics reference; development is single-player first, focused on one test map. **Debug and Release builds pass, all three regression suites pass in both, and the hidden DirectX render smoke test passes.** Rendered output has been visually inspected. Full interactive playtesting and finished art remain pending. Current development changes are uncommitted and have not been pushed.

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

- Runtime-compiled HLSL shaders, box/pyramid meshes, correct normal transforms, depth testing, directional lighting, fog, a bitmap font, and batched HUD geometry.
- A layered floating island with named village/dock/training/camp areas, detailed placeholder houses, trees, banners, distant fragments, a quest beacon, a player, and three enemies.
- Camera-relative movement, sprinting, jumping, collision against buildings/trees/island bounds, mouse orbit, zoom, and camera obstruction checks.
- Target cycling, melee range and line-of-sight checks, strike, heavy strike, dash, mend, cooldowns, enemy pursuit/return behavior, damage, death, and restart.
- A patrol acceptance/kill/return/reward loop with protection against duplicate rewards and support for a kill before quest acceptance.
- Health/target/ability HUD with availability reasons, live map and loot markers, quest view, character statistics, potion use, and sword switching.
- Static-grid A* navigation with path smoothing, enemy behavior states and home leashes, soft enemy separation, and stable enemy IDs.
- One-time enemy loot, potion cooldowns, a stronger patrol reward sword, and version 2 saves with version 1 migration.
- Versioned local save/load with bounds checking and transactional simulation replacement; invalid data does not replace the current simulation. Save files are replaced only after a successful temporary-file write.
- Window resizing, focus/minimize suspension, pause, DirectX error reporting, and shutdown handling.

Compilation, domain regression tests, and hidden rendering now validate parts of this implementation. Automated tests do not certify the complete interactive experience. All art remains procedural placeholder geometry, not finished Allods-style art.

## Known gaps

- The historical C++ branch tracks generated files under `build/`, `build_vs/`, and `out/build/`. Those files were not imported into the working checkout.
- Real keyboard/mouse interaction, focus/minimize behavior, all panels, and full gameplay feel still need the manual checklist.
- No sound, skeletal animation, imported art, networking, or GPU performance measurements.
- Navigation uses a static grid and is suited to this small map; there is no general navmesh or crowd solver. Enemy separation is soft, and player/enemy collision is not a full rigid-body system.
- Decorative dock rails, banners, and training targets are non-solid. Trees have trunk-based collision and conservative camera bounds.
- Inventory currently supports two swords and healing potions. Armor, item grids, trading, and broader RPG progression remain pending.
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
