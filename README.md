# Mediterranean

The project uses C++17 and a custom DirectX 11 engine for Windows PC, with Allods Online as the primary reference for setting, visuals, and gameplay. The active tree is native-only. The original JavaScript prototype is preserved in Git history and on the local `codex/archive-js-prototype` branch.

## Native build and launch

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

The native source now implements a procedural floating-island outpost, a third-person player, three raiders, combat, a patrol quest, HUD and supporting panels. These features are **not yet compiled or playtested on this machine**: CMake and a usable native toolchain were not found. The intended output is now a 3D scene, not a blank window. All geometry and the bitmap font are built in; no asset downloads are required.

## Native controls and test loop

| Input | Action |
| --- | --- |
| WASD / Shift | Move relative to the camera / sprint |
| Space | Jump |
| Hold right mouse and drag / wheel | Orbit / zoom |
| Tab | Cycle nearby living targets |
| 1 / 2 | Strike / heavy strike (melee range) |
| 3 / 4 | Dash / heal |
| E | Interact with the gold quest beacon |
| C / I / J / M | Character / inventory summary / quest / map |
| Escape | Close the open panel, otherwise toggle pause |
| R | Reset the current slice, including reward and enemies |
| F5 / F9 | Save / load local progress while the game has focus |

Accept the patrol at the gold beacon near the spawn, follow the path north, select a raider with Tab, and defeat it using melee abilities. Return to the beacon and press E for 25 gold. The reward can only be claimed once per session. Character and inventory panels are informational; equipment management is not implemented.

Simulation pauses when the window loses focus or is minimized. Panels keep the world active; Escape pause stops it. Saves are written under the project's ignored `saves/` directory. Save/load preserves position, health, cooldowns, enemies, quest state, and gold; jump, camera, target selection, and open panels are not persisted. An invalid save must leave the current simulation unchanged.

The `-Test` switch runs CTest after building and before launching. Simulation checks cover collision, targeting and cooldown rejection, jumping, the patrol/reward loop, save validation, death/restart, and invalid timesteps. They do not verify DirectX rendering; see [tests/manual-smoke.md](tests/manual-smoke.md) for the runtime checklist.

See [roadmap.md](roadmap.md) for the plan and [progress.md](progress.md) for implementation and validation status.

## Historical reference

The local `codex/archive-js-prototype` branch points to `a55db3e`, the last JS commit used by this checkout. Inspect it without replacing the current files, for example:

```powershell
git show codex/archive-js-prototype:src/game/player.js
```

The locally available `origin/dev` reference contains later JS changes, and `origin/C++` contains the original native skeleton. Neither reference was changed or merged during the native migration. Electron, Three.js, npm manifests, HTML, and JS source files are no longer part of the active project.
