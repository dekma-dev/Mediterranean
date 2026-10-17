# Mediterranean

The project uses C++17 and a custom DirectX 11 engine for Windows PC, with Allods Online as the primary reference for setting, visuals, and gameplay. The active tree is native-only. The original JavaScript prototype is preserved in Git history and on the local `codex/archive-js-prototype` branch.

## Native build and launch

For step-by-step setup, launch, breakpoints, controls, saves, and troubleshooting, see [Starting and Debugging](START_AND_DEBUG.md).

Requirements: Visual Studio 2022 with Desktop development with C++, an installed Windows SDK, and C++ CMake tools for Windows (or CMake on PATH). No dependencies are downloaded by the build helper.

The latest validated executable is `build-ninja-debug/RPGNative.exe`. Build, test, and play with the installed Visual Studio compiler and Ninja:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Debug -Test -Run
```

This prepares the compiler environment in a child process without changing machine settings. Debug and Release outputs are separate (`build-ninja-debug` and `build-ninja-release`). The default `VisualStudio` backend still generates `build-local/RPGNative.sln`; rebuilding that solution also uses the current source.

### Open in Visual Studio

Use **File > Open > Folder** and select `D:\Mediterranean` (or your checkout root). Visual Studio reads the root `CMakeLists.txt`; do not create a new Visual Studio project. Wait for CMake configuration, choose the x64 Debug configuration and `RPGNative.exe` as the startup target, then press F5. If CMake support is unavailable, the required Visual Studio components must be installed first.

Alternatively, the command below generates `build-local/RPGNative.sln`, which can then be opened as a solution. The generated solution is a build artifact, not the project's source entry point.

From PowerShell in the repository root:

```powershell
.\scripts\build-native.ps1 -Configuration Release -Test -Run
```

For debugging:

```powershell
.\scripts\build-native.ps1 -Configuration Debug
```

Open `build-local/RPGNative.sln` in Visual Studio, select Debug / x64, set RPGNative as the startup project, and press F5. Generated output belongs in `build-local`; do not reuse historical caches from the C++ branch.

The native game builds in **Debug and Release**, and all seven regression suites pass in both configurations using Ninja/MSVC. Hidden DirectX tests capture the village, raider windup, captain, and completed campaign at three window sizes plus overview, front-character and all five interface panels. Representative captures have been visually inspected; full interactive playtesting remains pending.

The first map contains Dawnwatch Village, an astral dock, training grounds, and a ruined raider camp. It includes two linked quests, XP and levels 1–3, telegraphed melee attacks, a captain, loot, potions, a reward blade and ward armor, death recovery, and save/load. The first procedural art pass adds shaped characters, pitched roofs, irregular rocks/trees, small generated materials, instancing and sun shadows. It is still stylized prototype art. Geometry and map/material textures are generated locally; the interface uses the installed Windows font; no asset downloads are required. See [graphics and measured budgets](docs/graphics.md). See [gameplay rules](docs/gameplay.md) for details.

## Native controls and test loop

| Input | Action |
| --- | --- |
| WASD / Shift | Move relative to the camera / sprint |
| Space | Jump |
| Hold right mouse and drag / wheel | Orbit / zoom |
| Tab | Cycle nearby living targets |
| 1 / 2 | Strike / heavy strike (also interrupts enemy windups) |
| 3 / 4 | Dash / heal |
| E | Talk, loot, or start stabilizing the cleared astral anchor |
| Q / G | Use a healing potion / switch owned swords |
| F2 | Hide / restore the HUD for visual inspection |
| F3 | Show completed-frame draw calls, GPU time and mesh memory |
| C / I or B / J / M | Equipment / inventory / journal / atlas |
| Escape | Open/close the live menu; simulation continues |
| F11 | Toggle borderless fullscreen / windowed |
| Left click | UI buttons, items, action bar and map waypoint |
| R | Reset the current slice, including reward and enemies |
| Enter while defeated | Return to Dawnwatch, retaining quests, XP, equipment, gold, and loot |
| F5 / F9 | Save / load local progress while the game has focus |

Accept the patrol at the gold beacon, defeat a raider, loot it with E, then return for 25 gold, 60 XP, and a watchguard blade. Equip it with G. Press E again to accept the anchor quest, defeat the remaining raiders and captain, then stand by the northern violet crystal and press E. Stay still for three seconds, return, and claim 50 gold, 120 XP, and automatically equipped ward armor. Quest rewards, kill XP, and loot are awarded once. The 24-slot belongings view has item details, equipment markers and working equip/drink actions. C shows the dressed 3D character and equipment slots.

Enemy swings lock their facing during the orange ground telegraph. Step outside the cone, dash away, or interrupt with heavy strike. Enter after death retains progress and restores living enemies at home; R explicitly resets the expedition.

Simulation pauses when unfocused or minimized. All panels and the Escape menu keep the world active while capturing local movement/combat keys. Normal gameplay starts fullscreen; F11 restores windowed mode. Version 3 saves preserve both quests, derived progression, inventory, and committed enemy windups. Valid version 1 and 2 saves migrate. Jump, camera, target, panels, and unfinished anchor channels reset. Invalid saves leave current progress unchanged. Saves remain in the ignored `saves/` directory.

The `-Test` switch runs interface, geometry, simulation, navigation, inventory, combat, and progression suites. Add `-SmokeTest` for hidden fullscreen restoration and four render scenarios at three sizes, including all five panels and two camera angles. Use `-Benchmark` with Release to write a repeatable frame-cost and memory report. Images and logs are written in the selected build directory:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Debug -Test -SmokeTest
```

The smoke test does not exercise real keyboard/mouse input or assess combat feel. Use [tests/manual-smoke.md](tests/manual-smoke.md) for those checks.

See [interface controls, design and research](docs/interface.md), the [roadmap](roadmap.md) for the plan, and [progress](progress.md) for implementation and validation status.

## Historical reference

The local `codex/archive-js-prototype` branch points to `a55db3e`, the last JS commit used by this checkout. Inspect it without replacing the current files, for example:

```powershell
git show codex/archive-js-prototype:src/game/player.js
```

The locally available `origin/dev` reference contains later JS changes, and `origin/C++` contains the original native skeleton. Neither reference was changed or merged during the native migration. Electron, Three.js, npm manifests, HTML, and JS source files are no longer part of the active project.
