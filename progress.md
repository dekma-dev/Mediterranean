# Mediterranean Progress

Last updated: 2026-10-17

## Current status

The active `main` working tree is now native C++ / DirectX 11 only, built on the sources imported from `origin/C++`. The JavaScript prototype is preserved in Git history and on the local `codex/archive-js-prototype` branch. Allods Online is the primary setting, visual, and mechanics reference. A first gameplay slice is implemented in source, but native compilation, regression test execution, and launch remain blocked by the missing local toolchain. No playable-build claim is made yet.

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

- Runtime-compiled HLSL shaders, a procedural box mesh, depth testing, directional lighting, atmospheric fog, and an embedded bitmap font.
- A layered floating island, outpost buildings with stepped roofs, decorative trees, distant fragments, a quest beacon, a player, and three enemies.
- Camera-relative movement, sprinting, jumping, collision against buildings and island bounds, mouse orbit, zoom, and camera obstruction checks.
- Target cycling, melee range and line-of-sight checks, strike, heavy strike, dash, mend, cooldowns, enemy pursuit/return behavior, damage, death, and restart.
- A patrol acceptance/kill/return/reward loop with protection against duplicate rewards and support for a kill before quest acceptance.
- Health/target/ability HUD, live map, quest view, character statistics, and a read-only inventory summary.
- Versioned local save/load with bounds checking and transactional simulation replacement; invalid data does not replace the current simulation. Save files are replaced only after a successful temporary-file write.
- Window resizing, focus/minimize suspension, pause, DirectX error reporting, and shutdown handling.

These are source-level implementation claims only. The intended first frame is now a 3D scene rather than a blank window. All art remains procedural placeholder geometry, not finished Allods-style art.

## Known gaps

- The historical C++ branch tracks generated files under `build/`, `build_vs/`, and `out/build/`. Those files were not imported into the working checkout.
- Native compilation, HLSL compilation, regression execution, and visual/runtime behavior remain unverified.
- No sound, skeletal animation, imported art, networking, or GPU performance measurements.
- Enemies use direct steering with collision sliding, not pathfinding; characters do not collide with one another.
- Decorative trees are not movement/camera collision objects.
- Inventory is informational; equipment management, loot items, and broader RPG progression remain pending.
- Camera, target, panel state, jump height, and transient hit flashes are not saved.
- The renderer currently uses immediate per-object/per-glyph draw calls; batching and profiling remain future work.

## Development environment

- Visual Studio Community 2022 version 17.13.3 was detected.
- `cmake` and `cl` were not found on the inspected PowerShell PATH. This alone does not prove that MSVC is absent.
- The Visual Studio component query did not confirm the combined C++ toolchain and CMake requirements; the expected bundled CMake executable was not found.
- No tools were installed and no system settings were changed.

## Completed in this session

- Read project instructions in `AGENTS.md`.
- Removed the local `.vs/` cache at the user's request and added `.vs/` to `.gitignore`.
- Inspected the current working tree and locally available branch references.
- Reviewed the native skeleton and identified the gap between engine initialization and gameplay migration.
- Created `roadmap.md` and this progress record in English.

## Next validation milestone

Confirm the required build tools and perform a clean build and launch using `scripts/build-native.ps1`. Use `build-local` rather than historical committed caches. Record the configuration and observed result here once performed.

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
