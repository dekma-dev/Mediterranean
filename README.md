# Mediterranean

The project uses C++17 and a custom DirectX 11 engine for Windows PC, with Allods Online as the primary reference for setting, visuals, and gameplay. The active tree is native-only. The original JavaScript prototype is preserved in Git history and on the local `codex/archive-js-prototype` branch.

## Native build and launch

For step-by-step setup, launch, breakpoints, controls, saves, and troubleshooting, see [Starting and Debugging](START_AND_DEBUG.md).

Requirements: Visual Studio 2022 with Desktop development with C++, an installed Windows SDK, and C++ CMake tools for Windows (or CMake on PATH). No dependencies are downloaded by the build helper.

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

The native game now builds in **Debug and Release**, and all three regression suites pass in both configurations. A hidden DirectX smoke test also verifies device/shader creation, three render-target sizes, rendering, and PNG capture. The captured scene has been visually inspected; full interactive playtesting remains pending.

The first map contains Dawnwatch Village, an astral dock, training grounds, and a ruined raider camp. It includes navigation around obstacles, melee combat, a patrol quest, loot, potions, equipment switching, and save/load. Art remains a procedural blockout. All geometry and the bitmap font are built in; no asset downloads are required.

## Native controls and test loop

| Input | Action |
| --- | --- |
| WASD / Shift | Move relative to the camera / sprint |
| Space | Jump |
| Hold right mouse and drag / wheel | Orbit / zoom |
| Tab | Cycle nearby living targets |
| 1 / 2 | Strike / heavy strike (melee range) |
| 3 / 4 | Dash / heal |
| E | Interact with the gold quest beacon or loot a defeated raider |
| Q / G | Use a healing potion / switch owned swords |
| F3 | Toggle the draw-count and position overlay |
| C / I / J / M | Character / inventory summary / quest / map |
| Escape | Close the open panel, otherwise toggle pause |
| R | Reset the current slice, including reward and enemies |
| F5 / F9 | Save / load local progress while the game has focus |

Accept the patrol at the gold beacon near the spawn, follow the path north, select a raider with Tab, and defeat it using melee abilities. Press E near its bag for 5 gold and a potion. Return to the beacon and press E for 25 gold and a watchguard blade; press G to equip it. Quest rewards and each enemy's loot can only be claimed once per session. Inventory lists consumables and the equipped weapon; drag-and-drop inventory and armor slots are not implemented.

Simulation pauses when the window loses focus or is minimized. Panels keep the world active; Escape pause stops it. Version 2 saves include position, health, cooldowns, enemy identities and states, loot flags, quest state, gold, potions, and equipment. Valid version 1 saves are migrated on load. Jump, camera, target selection, and open panels reset. Saves live in the project's ignored `saves/` directory; an invalid save leaves the current simulation unchanged.

The `-Test` switch runs simulation, navigation, and inventory suites. Add `-SmokeTest` to run the hidden DirectX test and write `build-local/Debug-smoke.png` or `build-local/Release-smoke.png` with a result log beside it:

```powershell
.\scripts\build-native.ps1 -Configuration Debug -Test -SmokeTest
```

The smoke test does not exercise real keyboard/mouse input or assess combat feel. Use [tests/manual-smoke.md](tests/manual-smoke.md) for those checks.

See [roadmap.md](roadmap.md) for the plan and [progress.md](progress.md) for implementation and validation status.

## Historical reference

The local `codex/archive-js-prototype` branch points to `a55db3e`, the last JS commit used by this checkout. Inspect it without replacing the current files, for example:

```powershell
git show codex/archive-js-prototype:src/game/player.js
```

The locally available `origin/dev` reference contains later JS changes, and `origin/C++` contains the original native skeleton. Neither reference was changed or merged during the native migration. Electron, Three.js, npm manifests, HTML, and JS source files are no longer part of the active project.
