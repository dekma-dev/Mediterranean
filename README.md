# Mediterranean

The project uses C++17 and a custom DirectX 11 engine for Windows PC. The active tree is native-only. The original JavaScript prototype is preserved in Git history and on the local `codex/archive-js-prototype` branch.

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

Earlier native validation built **Debug and Release** with fifteen regression suites using Ninja/MSVC. Hidden DirectX tests capture the village, raider windup, captain, and completed campaign at three window sizes plus inspection angles and interface layouts. Additional veteran captures show outfit changes, sparring, the quartermaster, purchased gear, buyback, eight close-up character poses and seven environment views, plus six animation-layer inspections and four live jump stages, for 132 smoke captures per configuration. CombatEffectsTests adds 39 diagnostic captures for real attacks, camera angles, update-rate agreement and effect isolation. FootPlacementTests adds twelve close-up and live-game diagnostics. Representative captures from that validation were visually inspected. Current source registers sixteen suites, including TalentTests, and adds champion-build captures. Recent source changes have not been compiled or run because the Visual Studio locator/toolchain is unavailable in the current environment; prior results do not validate them. Full interactive playtesting remains pending.

The first map contains Dawnwatch Village, an astral dock, training grounds, and a ruined raider camp. It includes two linked quests, XP and levels 1–3, telegraphed melee attacks, a captain, loot, potions, a reward blade and ward armor, death recovery, and save/load. Procedural art includes shaped characters, pitched roofs, irregular rocks/trees, small generated materials, instancing and sun shadows. Characters now have shaped heads, fitted hair, flat-soled boots, fixed-length arm/leg poses and hand-attached weapons in both the world and portrait. It is still stylized prototype art. Geometry and map/material textures are generated locally; the interface uses the installed Windows font; no asset downloads are required. See [graphics and measured budgets](docs/graphics.md). See [gameplay rules](docs/gameplay.md) for details.

## Native controls and test loop

Player and enemy feet now hold their ground contact while supporting the body, then lift to take the next step. Boots retain their planted direction through turns; arms counter-swing with actual foot travel and the pelvis adjusts to keep leg lengths fixed. Stops settle into a quiet stance, while jumping, landing, dashing and reloads release or rebuild contacts. This is a procedural flat-ground controller, not skeletal animation or uneven-terrain adaptation. See manual checks 81–83.

Strike combines a gold crescent with a blade-following trail; Heavy Strike adds a larger cyan crescent and delayed violet counter-sweep. Bright contact cores, expanding impact rings, tapered sparks and rising motes give attacks a more expressive fantasy-MMO appearance. Enemy effects use restrained orange. Confirmed combat events drive the effects, which remain behind scenery and under the interface and expire within half a second. Damage, range and cooldowns are unchanged. Press Tab near a raider, then 1/2; after the campaign, use the eastern sparring guard. See manual checks 74–80.

Successful Mend (4) now produces green ascending ribbons and motes; a potion (Q) creates a smaller gold pulse. Recovery residue stays at the activation position and fades within 0.8 seconds. Full-health and cooldown rejections produce no effect. This uses the existing bounded effect pool and batch without new textures or GPU buffers. See manual checks 84–85.

The latest graphics pass adds stone/wood/tile relief, woven cloth, roughness-dependent highlights, warmer sunlight with cool ambient fill, irregular beveled paving and layered clouds. Material detail follows moving characters and blends across curved surfaces. It reuses the same eleven 128-square texture layers and existing geometry; no external assets are needed. This improves the prototype's surfaces and lighting, while authored character models and richer scenery remain future work. See the latest measurements in [graphics.md](docs/graphics.md) and manual checks 70–72.

Movement has short acceleration ramps, eased player turns, travel-driven walking and blended strike poses. Player turning and player/enemy walking/attack poses now retain velocity through transitions, so starting, stopping and retargeting settle smoothly. Foot lift and airborne leg extension ease into their grounded boundaries. Enemies traverse path points without an idle frame and ease into arrival; committed attack cones keep their facing. Mouse orbit, zoom and jump/landing camera motion are filtered, with immediate collision retraction. Movement release now brakes briefly; reversals preserve momentum and airborne steering is weaker. See the motion checks in [the playtest checklist](tests/manual-smoke.md).

The follow-up fixes camera-relative steering during orbit, pending rotation after cancellation, camera bouncing at obstacle edges, stale pursuit and enemies stalling behind nearby trunks. Jump height, constant-speed gait and camera recovery are checked at 30/60/144 Hz.

The environment connects the village plaza, dock, training ground and anchor with shared paved/dirt routes used by both ground rendering and the atlas. Trees use folded leaf surfaces, with 131 grass clumps and 18 shrubs clear of roads and services. The homestead, watch hall, storehouse and training hall now have distinct palettes, shutters, side windows and emblems; the keeper is a dressed NPC beneath a gold marker. A covered supply stall and masonry ruin add landmarks. Seven inspection captures cover the island, canopy, training area, dock, village, stall and ruin. These changes stay on the existing map and use generated assets only.

| Input | Action |
| --- | --- |
| WASD / Shift | Move relative to the camera / sprint |
| Space | Jump |
| Hold right mouse and drag / wheel | Orbit / zoom |
| Tab | Cycle nearby living targets |
| 1 / 2 | Strike / heavy strike (also interrupts enemy windups) |
| 3 / 4 | Dash / heal |
| E | Talk, loot, stabilize the anchor, browse/close the shop, or start/stop sparring |
| Q / G | Use a healing potion / switch owned swords |
| F2 | Hide / restore the HUD for visual inspection |
| F3 | Show completed-frame draw calls, GPU time and mesh memory |
| C / I or B / J / M | Equipment / inventory / journal / atlas |
| N | Champion build map / learn talents |
| Escape | Open/close the live menu; simulation continues |
| F11 | Toggle borderless fullscreen / windowed |
| Left click | UI buttons, items, action bar and map waypoint |
| Drag item / right-click item | Move or equip gear / equip, remove or drink |
| R | Reset the current slice, including reward and enemies |
| Enter while defeated | Return to Dawnwatch, retaining quests, XP, equipment, gold, and loot |
| F5 / F9 | Save / load local progress while the game has focus |

Accept the patrol at the gold beacon, defeat a raider, loot it with E, then return for 25 gold, 60 XP, and a watchguard blade. Equip it with G. Press E again to accept the anchor quest, defeat the remaining raiders and captain, then stand by the northern violet crystal and press E. Stay still for three seconds, return, and claim 50 gold, 120 XP, and automatically equipped ward armor. Quest rewards, kill XP, and loot are awarded once. The 24-slot bag supports drag swaps, Sort, item comparisons and right-click actions. C shows ten worn slots: weapon, chest, legs, feet, head, off hand, shoulders, hands, waist and back. Equipped items leave the bag. The arrow buttons rotate the dressed preview; Front resets it.

Press N to inspect the champion's build map. Levels 2 and 3 each grant one point. Learn upgrades along Arms (damage), Guard (health/protection) or Spirit (Mend); a second-rank talent requires the first in its branch. Choose one complete branch or two first-rank talents. Learning is available outside combat while grounded and idle; the menu keeps the world active. Choices persist with F5/F9 and respawn. R resets the expedition and build. See [talent effects and rules](docs/gameplay.md#champion-build-n).

Visit the quartermaster at the orange village marker `(3, -12)` and press E. Buy healing draughts for 6 gold, a helm for 20, or a shield for 35; gear unlocks after handing in the frontier patrol. Purchases go into the bag. Right-click or drag them into Head / Off hand to equip. The helm adds 1 protection and the shield adds 2; with ward armor, a normal 8-point hit becomes 3. The shop, bag and character window can stay open together. Stock is limited to six draughts and one of each equipment item per expedition; saving, loading and respawning preserve purchases.

Select a bag item and click **Sell one** while trading; worn gear must be removed first. The **Buyback** shelf returns one sold item at its sale price, provided you can afford it. Sold items persist across leaving, saving and death; R clears them with the expedition. Selling never restocks the shop. Select a potion stack and click **Split half** to put half in a free cell; drag stacks together or Sort to merge them. Drinking/selling uses the selected cell. See [prices and rules](docs/gameplay.md).

After both quests, visit the eastern training post (cyan map marker) and press E for a repeatable 30-second sparring round. Test attacks, dodges, interrupts and armor against a 180-HP guard. E stops; leaving the area or reaching 1 HP ends the round. The report shows damage dealt/taken. There is no XP, loot or resource refund; potions are consumed normally.

Enemy swings lock their facing during the orange ground telegraph. Step outside the cone, dash away, or interrupt with heavy strike. Enter after death retains progress and restores living enemies at home; R explicitly resets the expedition.

Simulation pauses when unfocused or minimized. All panels and the Escape menu keep the world active. C and I open independent character/bag windows together, leaving the world visible and keyboard movement/combat available; pointer input over a window is captured. The central menu, champion build, atlas, journal and shop capture movement/combat keys. The UI uses green/gold carved frames, a circular minimap and compact slots. Normal gameplay starts fullscreen; F11 restores windowed mode. Version 8 saves preserve both quests, derived progression, learned talents, exact split bag stacks, ten worn slots, purchased stock, buyback items, gold and committed campaign enemy windups. Valid versions 1–7 migrate; earned talent points start unspent. Jump, camera, target, panels, unfinished anchor channels and sparring sessions reset. Invalid saves leave current progress unchanged. Saves remain in the ignored `saves/` directory.

The `-Test` switch runs animation, shadow, motion, interface, geometry, simulation, navigation, inventory, combat, progression, talents, training and trade suites. Add `-SmokeTest` for hidden fullscreen restoration and rendered scenarios, including the shop, all other panels, inspection angles, outfit changes and sparring. Use `-BenchmarkSuite` with Release to measure stationary HUD, walking with camera orbit, repeated combat and the merchant with both side windows open. `-Benchmark` retains the shorter stationary report. These hidden workloads do not establish interactive FPS. Images and logs are written in the selected build directory:

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

The visible game now plays an original synthesized ambient music loop and gameplay effects (combat, healing, footsteps, jump/landing and dash). Escape offers separate Music/Effects levels, including mute at 0. Alt-Tab pauses playback; hidden checks remain silent. No downloads or external sound packs are required. This is prototype mono audio; listening/mix acceptance remains pending. Generated previews are `dawnwatch-music.wav` and `effects-preview.wav` in each tested build directory.
