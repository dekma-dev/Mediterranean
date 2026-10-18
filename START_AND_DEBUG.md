# Starting and Debugging Mediterranean

This guide covers the native C++17 / DirectX 11 game on Windows.

Jump physics: run, jump, release movement and observe landing. Momentum should continue through flight, with ground braking starting at contact. Takeoff no longer gets an extra frame of ground acceleration, and landing frames count only grounded walking distance for gait/audio. Manual check 98 covers live feel; `landing-physics-validation.log` records automated checks in each Ninja build directory.

Audio: launch the visible game for music and gameplay sounds. Escape provides separate Music/Effects +/- controls; 0 mutes a channel. Initial levels are 35% music and 70% effects, retained only during the session. Alt-Tab/minimize suspends playback. If no audio device can initialize, the menu shows "Audio unavailable" and gameplay continues; restart after changing a failed output device. Hidden smoke/benchmark runs stay silent. `AudioTests` writes `dawnwatch-music.wav` and `effects-preview.wav` and runs a muted device check; logs are `audio-validation.log` in each Ninja directory. Manual checks 93–95 cover listening and lifecycle behavior.

Cursor: rebuild and launch to see the emerald/gold native pointer inside the game. The click hotspot is its tip; normal window-border cursors remain available. `cursor-validation.log` records automated checks, and `cursor-preview.bmp` is an enlarged diagnostic in each build directory. Native cursors are absent from GPU smoke PNGs. See manual check 92 for live/mixed-DPI behavior.

Interface pass: open I/C and hover bag or equipment cells, then hover Buy in the quartermaster panel. Cards show stats, colored differences from worn gear, counts/value and current action availability. `InterfaceTests` writes six `tooltip-bag-*.png` / `tooltip-shop-*.png` diagnostics at three resolutions; logs are `item-tooltip-validation.log` in each Ninja directory. See manual check 91.

Physics pass: walk/sprint diagonally into a wall, trace building corners, and dash toward trees/the island boundary. The actor should reach contact and slide without crossing solids or favoring one axis. `NavigationTests` covers swept collision; full logs are `swept-physics-validation.log` in both Ninja directories. See manual check 90. This is flat-map character collision, not a rigid-body or terrain-slope system.

Latest bug-fix logs are `trail-history-validation.log` in both Ninja directories. `CombatEffectsTests` adds `effects-history-60.png`, `effects-history-144.png` and `effects-history-1000.png` to check blade history at high update rates. Manual check 89 covers the same issue during play.

Latest CPU optimization logs: `transform-validation.log` in each Ninja build directory. Release `transform-before-*.txt` / `transform-after-*.txt` contain the three-run workload comparison, and `transform-image-comparison.txt` records pixel differences. Manual check 88 covers live performance acceptance.

Shadow filtering: inspect boot contacts, thin posts and the village buildings while moving/orbiting slowly. Cubic weights sharpen nearby shadow detail without increasing the sixteen-comparison filter or texture memory. Manual check 87 includes the near/far transition. Validation logs are `shadow-cubic-validation.log` in each Ninja directory; Release reports compare the old filter (`shadow-cubic-before.txt`) with the new filter (`Release-benchmark-suite.txt`).

Camera smoothness: scroll in/out and reverse while zooming, then move past a wall or tree. Zoom and camera recovery should accelerate gently, while collision still retracts immediately. Manual check 86 covers this pass; its build/test logs are `camera-smoothness-validation.log` in each Ninja directory.

To inspect recovery effects, take damage, retreat to safety, then press 4 for Mend or Q for a potion. Mend produces green ascending ribbons; potions produce a smaller gold pulse. Both fade within 0.8 seconds at the activation position. Full-health/cooldown input must produce neither duplicate effects nor extra consumption. Manual checks 84–85 cover this pass. Latest combined logs are `recovery-validation.log` in each Ninja build directory; new diagnostics include `effects-mend-gameplay.png`, `effects-potion-gameplay.png` and three update-rate views per effect.

**Validation status:** Debug and Release builds pass all fifteen regression suites, 132 hidden smoke captures, 39 combat-effect diagnostics and twelve foot-placement captures per configuration using Ninja/MSVC. Representative PNG captures have been inspected. A complete interactive playtest remains pending; see [progress.md](progress.md).

To build and play the latest version from PowerShell in `D:\Mediterranean`:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Debug -Test -Run
```

The verified output is `D:\Mediterranean\build-ninja-debug\RPGNative.exe`. The script locates Visual Studio and prepares its x64 compiler environment in a child process. It neither installs tools nor changes machine settings. This backend also works when MSBuild's per-user Windows SDK lookup is inaccessible.

## Quick recovery: Visual Studio cannot launch the startup project

Compiler debug records now use `/Z7` in Debug and RelWithDebInfo builds to avoid dependence on the shared MSVC program-database server (which failed with C1902 in this environment). Linking still produces `RPGNative.pdb` for source-level debugging. These are project-local CMake options; no installed toolchain or machine settings are modified.

To inspect the new planted-foot movement, walk/sprint over the village paving with F2, stop, reverse, turn, jump and dash. Watch supporting boots against paving joints; they should stay fixed until lifted. Repeat while a raider approaches and stops for its warning cone. Manual checks 81–83 cover these paths. `FootPlacementTests` runs through `-Test` and writes twelve `feet-*.png` diagnostics, including `feet-live-walk.png`, `feet-live-turn.png`, `feet-live-enemy.png` and `feet-live-combat.png`, separate from the 132 smoke views and 39 effect images. Current combined validation logs are `foot-placement-validation.log` in each Ninja directory. Release cost reports are `foot-placement-baseline-benchmark.txt` and `foot-placement-final-benchmark.txt`.

The latest movement pass is included in both Ninja executables. To inspect it, walk/sprint, reverse direction, release movement, jump, orbit/zoom and let an enemy approach. Expect a short braking step on release and retained momentum in flight; the camera should pull in immediately for obstacles and ease back out. `MotionTests` checks acceleration at 30/60/144 Hz and reset/combat/input behavior. Use manual checks 47–49 and 97 for physical input and visual acceptance.

The smoothing bug-fix follow-up adds checks 50–52: hold W while orbiting, cancel/reopen panels mid-orbit, skim an obstacle with the camera and pull an enemy close behind a trunk. `MotionTests` now also compares jump apex, constant-speed gait and camera clearance recovery across update rates. The camera holds briefly at obstruction edges before recovering, while inward zoom remains available.

If the green Start button shows `RPGNative.sln`, open that file as a solution instead of trying to launch it from Folder View:

1. Dismiss the error dialog. Clear the Solution Explorer search field using its **X** button; this field only filters the tree and does not select a startup target.
2. Choose **File > Open > Project/Solution** (**Файл > Открыть > Проект или решение**) and open `D:\Mediterranean\build-local\RPGNative.sln`.
3. In Solution Explorer, right-click the **RPGNative** C++ project under the solution and choose **Set as Startup Project** (**Назначить запускаемым проектом**).
4. Select **Debug** and **x64**, then press **F5**. The Start button should now show the local Windows debugger rather than the `.sln` filename.

This uses the solution already generated by the build helper. No manual compiler or linker paths are needed. If `build-local/RPGNative.sln` is missing in a fresh checkout, generate it using section 3 below.

To play the latest verified build immediately without Visual Studio, open `D:\Mediterranean\build-ninja-debug\RPGNative.exe` in Windows Explorer. This launches the last compiled version without a debugger; rebuild after source changes. The generated solution uses its own `build-local/Debug` output, so rebuild it in Visual Studio before testing new source there.

## 1. Required tools

Use Visual Studio 2022 with these components:

- Desktop development with C++ workload.
- MSVC x64/x86 build tools.
- Windows SDK.
- C++ CMake tools for Windows.

Visual Studio itself being installed does not guarantee that these components are present. Check them in Visual Studio Installer under **Modify**. The project build script does not install or download anything.

## 2. Recommended: open the project folder in Visual Studio

1. Start Visual Studio.
2. Choose **File > Open > Folder** and select `D:\Mediterranean`.
3. Let Visual Studio configure the root `CMakeLists.txt`. Watch the **Output** window for configuration errors.
4. Select an **x64 Debug** configuration. Its exact displayed name depends on Visual Studio's CMake configuration.
5. Select **RPGNative.exe** as the startup target in the toolbar. Do not select `SimulationTests` unless you intend to run the tests.
6. Press **F5** to build and start with the debugger, or **Ctrl+F5** to start without the debugger.

You do not need to create a new Visual Studio project or supply a `.sln` file. If configuration fails, resolve the first CMake error before attempting to launch.

## 3. Alternative: generate and open a solution

Open PowerShell and run:

```powershell
cd D:\Mediterranean
.\scripts\build-native.ps1 -Configuration Debug -Test
```

The script configures an x64 Visual Studio 2022 build, compiles it, and runs the simulation tests. It searches for CMake on PATH and then in the Visual Studio installation.

After successful configuration/build:

1. Open `D:\Mediterranean\build-local\RPGNative.sln` in Visual Studio.
2. Select **Debug** and **x64**.
3. In Solution Explorer, right-click **RPGNative** and select **Set as Startup Project**.
4. Press **F5**.

The generated solution belongs to the build directory. Edit the root `CMakeLists.txt` when changing the project structure; generated project files can be overwritten by CMake.

Visual Studio's Open Folder workflow uses its own output directory. The helper's default `VisualStudio` backend uses `build-local`; `-Backend Ninja` uses `build-ninja-debug` or `build-ninja-release`. Do not mix generators in one build directory.

## 4. Build, test, or launch from PowerShell

Run these commands from `D:\Mediterranean`:

| Task | Command |
| --- | --- |
| Build, test, and play using Ninja/MSVC | `.\scripts\build-native.ps1 -Backend Ninja -Configuration Debug -Test -Run` |
| Validate Ninja Release, including rendered captures | `.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -Test -SmokeTest` |
| Measure Release rendering and memory | `.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -Benchmark` |
| Profile walking, combat and the merchant interface | `.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -BenchmarkSuite` |
| Launch the verified Ninja Debug build | `.\build-ninja-debug\RPGNative.exe` |
| Build Debug | `.\scripts\build-native.ps1 -Configuration Debug` |
| Build Debug and run regression tests | `.\scripts\build-native.ps1 -Configuration Debug -Test` |
| Build, test, and launch Debug | `.\scripts\build-native.ps1 -Configuration Debug -Test -Run` |
| Build, test, and launch Release | `.\scripts\build-native.ps1 -Configuration Release -Test -Run` |
| Build, test, and capture a hidden render smoke test | `.\scripts\build-native.ps1 -Configuration Debug -Test -SmokeTest` |
| Launch an existing Debug build | `.\build-local\Debug\RPGNative.exe` |
| Launch an existing Release build | `.\build-local\Release\RPGNative.exe` |

`-Run` starts the game without attaching the Visual Studio debugger. Launching an existing executable does not rebuild changed source files. Use Debug for breakpoints and inspecting variables; use Release for later performance checks.

`-SmokeTest` captures spawn, combat windup, captain windup, and a veteran after both quests. Each scene is captured at 960x600, 1600x900, and 1280x720 in the selected build directory. For example, `build-ninja-debug/Debug-combat.png` is the 1280x720 frame and `Debug-combat-960x600.png` is the smaller variant. Each scenario also creates `-overview.png` and `-character.png` without the HUD, plus `-menu`, `-inventory`, `-equipment`, `-atlas`, `-journal` and combined `-equipment-bag` at all three sizes. The veteran adds `-base-outfit`, `-travel-outfit`, `-sparring`, `-sparring-interface`, `-watchman-character` and `-equipped-sparring` at 1280x720, plus `-quartermaster`, `-purchased-equipment` and `-buyback` at all three sizes. Eight further 1280x720 views inspect the character: `-pose-front`, `-pose-profile`, `-pose-back`, `-pose-walk-left`, `-pose-walk-right`, `-pose-strike-start`, `-pose-strike-mid` and `-pose-strike-end`. Total: 132 smoke captures per configuration, including six animation layers and four live jump stages described below. The test also verifies hidden fullscreen size and windowed restoration. The adjacent `.png.log` records the result. `-Test` runs fifteen suites: `AudioTests`, `FootPlacementTests`, `CombatEffectsTests`, `AnimationTests`, `ShadowTests`, `MotionTests`, `InterfaceTests`, `GeometryTests`, `SimulationTests`, `NavigationTests`, `InventoryTests`, `CombatTests`, `ProgressionTests`, `TrainingTests` and `TradeTests`. InterfaceTests also writes `inventory-drag.png` in the build directory.

For a specific render fixture: `RPGNative.exe --smoke-test <absolute-output.png> <absolute-snapshot.save>`. This optional snapshot is validated in memory and never replaces `saves/patrol.save`. Provided fixtures are under `tests/fixtures`. Captures do not replace interactive playtesting. See [graphics and measured budgets](docs/graphics.md) for the generated art, memory limits and benchmark methodology. `-Benchmark` writes `<Configuration>-benchmark.txt`; it runs a hidden stationary scene and does not measure interactive FPS.

The veteran also adds seven environment views: `-environment-overview`, `-environment-canopy`, `-environment-training`, `-environment-dock`, `-environment-village`, `-environment-stall` and `-environment-ruin`. These are part of the 132-capture total. To inspect in game, follow the paved street from the spawn plaza and the dirt branches to the dock, training post and northern anchor; M shows those same routes and building roof colors. Use F2 and mouse orbit to inspect vegetation, house details, the supply canopy and the filled ruin gateway. The gameplay floor remains flat and building interiors remain inaccessible.

`-BenchmarkSuite` writes `<Configuration>-benchmark-suite.txt` in the selected build directory. It runs four hidden 1280x720 workloads: stationary HUD, village walking with an orbiting camera, repeated combat with camera movement, and the merchant with character/bag windows. Each uses 90 warmup and 600 measured frames with a fixed 1/60-second simulation step and presentation interval 0. It reports CPU median/p95/max, asynchronous GPU observations, peak submissions/triangles and resource bytes. Walk distance and combat hits verify the workloads actually ran. Setup and in-memory snapshot loads are excluded; player saves are never written. For an existing executable use `RPGNative.exe --benchmark-suite <absolute-report.txt>`. This is a short scripted workload test; use F3 and the manual checklist for visible-gameplay, input and long-session checks.

The latest CPU optimization comparison uses three runs of each build, alternating baseline/optimized order. Saved reports are `build-ninja-release/performance-baseline-run1.txt` through `run3.txt` and `performance-optimized-run1.txt` through `run3.txt`; interpretation and resource totals are in [graphics.md](docs/graphics.md). `performance-image-comparison.csv` records exact matches for all 132 saved Release captures. Rebuilding or running a benchmark does not change the application settings or player saves. Use manual check 68 for actual fullscreen/input performance; hidden timing includes submission/presentation and should not be converted directly into gameplay FPS.

For shadow regressions, run the normal `-Test` command or `ctest --test-dir build-ninja-debug -R ShadowTests --output-on-failure` from a terminal with CTest available. The hidden GPU test writes `shadow-*.png` diagnostic pairs in the build directory and compares actual pixels for contact, material changes, inclined surfaces, moving/offscreen casters, clearing, resize and portrait isolation. Focused-map checks cover thin-post retention, snapped-grid stability, the detail/distant overlap and distant fallback; CPU checks verify stable projection matrices. Queue regressions compare fully culled world/portrait batches against clean references and test a dense scene plus the 4,096/4,097-object capacity boundary. These diagnostics are separate from the 132 game smoke views. To inspect live shadows, rebuild/run Release, hide the HUD with F2, then orbit around boots, village steps, tree trunks, the stall and wall braces. Use manual checks 53–55 and 64–67; fullscreen motion and visual acceptance still require playing the game. Latest validation logs are `shadow-fixes-validation.log` in each Ninja build directory, and the saved Release cost report is `build-ninja-release/shadow-fixes-benchmark.txt`. Shadow storage remains 8 MiB after the queue fixes.

The animation pass adds veteran captures `-animation-idle-inhale`, `-animation-idle-exhale`, `-animation-jump-rise`, `-animation-jump-fall`, `-animation-landing` and `-animation-recoil`. Four further `-jump-rise`, `-jump-apex`, `-jump-fall` and `-jump-landing` images exercise the real jump simulation and its world shadows. All ten belong to the 132-image smoke total. To inspect live, rebuild/run the current executable, use F2, stand on clear paving and press Space; watch the legs extend before landing and the knees briefly compress. Stand near the keeper/quartermaster and open C to inspect restrained idle motion. Use sparring for nonlethal hit reactions. Manual checks 56–60 cover transitions, attachments, repeated hits, defeat-screen cleanup and reset paths; automated captures cannot establish how the animation feels during play. AnimationTests also checks exact landing timing at 30/60/144 Hz and uneven steps. The bug-fix validation logs are `animation-fixes-validation.log` in each Ninja build directory.

## 5. Debugging basics

For the latest surface/lighting pass, rebuild and run Release, hide the HUD with F2 and inspect the village paving, roof tiles, timber, plaster, clothing and armor. Walk/turn while watching fabric and wood grain: detail should follow the object. Open C, rotate the portrait, then inspect shadows near wall braces and steps. Orbit/pitch the camera to check that clouds follow the view direction and remain distant during walking. Manual checks 70–73 cover relief, highlights, movement, sky and performance. `graphics-bugs-validation.log` in each Ninja build directory records final tests/captures; `build-ninja-release/graphics-bugs-benchmark.txt` contains final workload timings. The earlier paired surface-cost reports are `realism-baseline-benchmark.txt` and `realism-final-benchmark.txt`; earlier screenshots are retained under `build-ninja-release/realism-baseline-captures` for comparison.

For combat effects, rebuild/run Release, select a nearby raider with Tab, and use 1 for the gold Strike crescent or 2 during a windup for the larger cyan/violet Heavy Strike and interrupt burst. Watch for a bright contact core, expanding ring, tapered sparks and fading motes. After completing the campaign, E at the eastern training post provides repeatable nonlethal practice. F2 makes the effects easier to inspect; C/I and Esc exercise panel transitions. Manual checks 74–80 cover trails, hits/misses, camera angles, scenery occlusion, lifecycle and performance. `CombatEffectsTests` is included in `-Test` and creates 39 `effects-*.png` diagnostics, including `effects-heavy-gameplay.png`, `effects-heavy-front.png`, `effects-heavy-opposite.png`, `effects-heavy-afterglow.png`, `effects-enemy-hit.png` and `effects-heavy-equipment.png`; these are additional to the 132 smoke views. Current validation logs are `cinematic-effects-validation.log` in each Ninja directory. Release reports `cinematic-effects-baseline-benchmark.txt` and `cinematic-effects-final-benchmark.txt` compare the initial and enhanced effects, including effect frames/vertices. Tests do not write player saves.

For the latest smoothness pass, use manual checks 61–63. On open ground, alternate short WASD taps, sustained movement and Shift, then reverse direction and release the keys. Character motion should settle without physical drift. Use sparring to compare walking into a windup and strike recovery; the displayed attack cone must stay authoritative. MotionTests covers smooth starts/stops, rapid retargeting, reset velocities, bounded channels and update-rate agreement. Latest validation logs: `smoothness-validation.log` in each Ninja build directory.

With the Visual Studio editor focused:

| Shortcut | Action |
| --- | --- |
| F9 | Toggle a breakpoint on the selected executable source line |
| F5 | Start debugging or continue after a breakpoint |
| F10 | Step over the next statement |
| F11 | Step into a function |
| Shift+F11 | Step out of the current function |
| Shift+F5 | Stop debugging |

Useful breakpoint locations:

- `main.cpp`, inside `wWinMain`: application entry and initialization failure.
- `engine/Engine.cpp`, inside `Engine::Tick`: frame update and rendering sequence.
- `engine/Renderer.cpp`, inside `Renderer::Check`: failed DirectX calls. A conditional breakpoint on `result < 0` avoids stopping on successful calls.
- `game/Simulation.cpp`, inside `Simulation::UseAbility`: target, range, damage, and cooldown behavior.
- `game/Simulation.cpp`, inside `Simulation::Interact`: quest and reward transitions.
- `game/Simulation.cpp`, inside `Simulation::Load`: save validation.
- `game/Inventory.cpp`, inside `Inventory::Apply`: bag swaps, equipment compatibility and stale-drag rejection.
- `game/Trade.cpp`, inside `Simulation::PurchaseStatus` / `Buy`: range, quest, stock, capacity and gold checks.
- `game/Progression.cpp`, inside `Simulation::StartSparring` / `EndSparring`: practice lifecycle and results.

When execution pauses, inspect **Locals**, **Watch**, and **Call Stack** under **Debug > Windows**. The **Output** window's Debug output contains shader compiler messages and renderer diagnostics. Use Build output for compiler/linker errors.

For an exception, inspect the call stack and the first relevant project frame before continuing. If a breakpoint is hollow or never hits, confirm that you built Debug, launched the matching executable, and are using its matching symbols. Stop the game before rebuilding if the linker reports that the executable is in use.

**F5 and F9 depend on focus:** in Visual Studio they control debugging and breakpoints; with the game window focused they save and load game progress. The game also suspends simulation while it is unfocused or minimized.

## 6. First gameplay test

Normal gameplay opens in borderless fullscreen. Use F11 or the Display button in the Escape menu to restore the previous window placement. This changes only the game window. The intended scene is a floating island with an outpost, blue player, red raiders, and gold quest beacon. A blank window is no longer the expected result.

1. Press **E** near the dressed keeper beneath the gold marker at spawn to accept the patrol. Existing text calls him the beacon keeper.
2. Follow the path north with **WASD**. Hold **Shift** to sprint and press **Space** to jump.
3. Hold the **right mouse button** and drag to orbit; use the **mouse wheel** to zoom.
4. Press **Tab** to select a raider, move into melee range, and use **1** for strike or **2** for heavy strike.
5. Use **3** to dash and **4** to heal after taking damage.
6. Defeat a raider and press **E** near its bag to collect 5 gold and one healing potion.
7. Return to the beacon and press **E** for 25 gold and the watchguard blade. Press **G** to equip it and increase melee damage.
8. The patrol reward also grants 60 XP; together with one kill this earns level 2 and 120 maximum health. Press **E** again to accept **Secure the Astral Anchor**.
9. Defeat the remaining raider and the violet captain. Orange cones announce attacks: move outside them, dash away, or press **2** to interrupt a windup. The captain has a longer windup and hits harder.
10. Approach the violet crystal in front of the northern ruin, press **E**, and stand still for three seconds. Movement and actions cancel stabilization. Return for 50 gold, 120 XP, ward armor, and level 3.
11. Press **C then I**. Drag the tunic from the bag onto Chest to swap armor; drag gear back to a bag cell to remove it. Right-click also equips/removes/drinks. Use Sort to compact the bag. Attributes and both character views follow the worn items; ward armor reduces damage only while equipped.
12. Follow the cyan map marker to the eastern training post at `(12, -2)`. Within 2.5 units, press **E** to start a 30-second sparring round. The 180-HP guard uses telegraphed melee attacks; **2** interrupts. Compare damage with ward armor and the tunic. **E** stops, leaving the area ends the round, and reaching 1 HP ends it nonlethally. No XP/loot is awarded and no health, cooldowns or potions are refunded. A damage report appears when the round ends.
13. Visit the orange quartermaster marker at `(3, -12)`, near the village entrance. Stand within 2.75 units and press **E**. The shop opens beside the bag; **C** adds the character window. Buy the helm for 20 gold and shield for 35, then right-click or drag each into **Head / Off hand**. Gear unlocks after the patrol hand-in. Bought gear starts in the bag; no item equips automatically. Use the portrait arrows and **Front** to inspect it.
14. With both quests completed and all three loot bags claimed, 90 gold becomes 35 after buying both pieces. Both rows become sold out. The fully equipped ward/helm/shield outfit shows 5 armor; a normal sparring hit now deals 3 instead of 8. Save/load and verify the same gold, stock and exact item locations. The vendor also stocks six draughts at 6 gold each. Retail stock resets only with a new expedition.
15. Select a potion stack with at least two items in the bag and press **Split half**. Half (rounded down) moves to a free cell. Select the second stack and use it while injured; only that stack should shrink. Drag the stacks together or Sort to merge them. Splits preserve the total number of potions.
16. While trading, select an unused bag item and click **Sell one** at the displayed price. Worn gear shows **Unequip to sell**. Selling a shield gives 17 gold; click its **Buyback** icon to reclaim it for 17. It returns to the bag and its retail stock stays depleted. Buyback survives closing the shop, saving/loading and death recovery. Spending the proceeds may make reclaiming unaffordable until you sell something else. R clears the whole expedition, including buyback.

Additional controls:

| Key | Action |
| --- | --- |
| C / I or B / J / M | Character equipment / item bag / journal / atlas |
| Escape | Open/close the live game menu; the world continues |
| F11 | Toggle borderless fullscreen/windowed |
| Left click | Use UI buttons, select items, click action slots or place a map waypoint |
| Drag item | Move/swap bag cells, equip into a compatible slot, or remove gear into the bag |
| Right-click item | Equip, remove, or drink; does not rotate the camera over a window |
| Split half | Split the selected potion stack into a free bag cell |
| Sell one / Buyback item | Sell from the selected bag cell / reclaim one at its sale price |
| Right click on atlas | Clear the waypoint |
| E in the shop / shop X | Close trade; the bag/character can remain open |
| Character < / > / Front | Rotate the outfit preview / restore front view |
| R | Reset the current session, including enemies, quest, and reward |
| Enter while defeated | Return to Dawnwatch while preserving earned progression and loot |
| F5 / F9 | Save / load while the game has focus |
| Q | Use a healing potion; full health does not consume one |
| G | Switch owned swords; re-equip the starter sword when unarmed before the reward |
| F2 | Hide / restore the HUD for art inspection |
| F3 | Show completed-frame draw calls, GPU milliseconds and mesh KiB |

The menu and every panel keep combat active. Press **C then I** to open the character on the left and bag on the right together. Each X or hotkey closes only its own window. These side windows allow keyboard movement/combat; mouse orbit and zoom work over the visible world. The menu, atlas, journal and shop capture movement/combat commands until closed. Escape opens the menu and preserves the side windows for returning; it closes an active trade. Inventory details compare the selected item with worn gear and provide Equip, Unequip or Drink actions. Invalid drops leave the inventory unchanged. Closing a window, Escape, focus loss or a display resize cancels the active drag. C and the world show the same outfit; removing clothing leaves neutral underclothes. The atlas has generated terrain/landmarks, live markers and a traversable-ground waypoint. See [interface details and references](docs/interface.md), [gameplay rules](docs/gameplay.md) and the [manual smoke test](tests/manual-smoke.md).

Start a camera orbit by pressing the right mouse button over the world. A press that starts on a side window or interactive HUD control stays captured until release, even when the pointer leaves it. After Alt-Tab or a display change, release and press again to resume orbit. Panel transitions and loading invalidate old hit regions so the next click cannot activate a hidden item or menu button.

F11 cancels an item drag before processing a simultaneous release, preventing accidental equipment changes during the display transition. Defeat closes the current central panel to show the recovery prompt. **Enter** still returns you to Dawnwatch if you reopen the menu, atlas or journal while defeated; quests, equipment, gold and loot are retained.

After a fatal hit, enemy warning cones must disappear immediately and strike poses must settle. There must be no final enemy movement/separation step after defeat, including when several enemies attack together. Loading a defeated save must not restore a frozen swing or warning. Manual check 69 covers these paths; automated validation is logged in `defeat-fixes-validation.log` in each Ninja build directory.

## 7. Save files

The build configures saves under the source checkout's `saves` directory, currently `D:\Mediterranean\saves\patrol.save`. This location is independent of the shell's working directory and is ignored by Git.

Version 6 stores both quests, exact bag cells/split stacks, six worn slots, purchase counts in `SHOP`, merchant-held item counts in `BUYBACK`, remaining gold, enemy identities/behavior/windups/facing, loot, health, position and cooldowns. Gold equals campaign rewards/loot minus purchases plus the value of current buyback items. Player-owned plus merchant-held gear must agree with campaign rewards and purchases. XP/levels derive from completed objectives; selling quest gear does not remove XP. Protection comes from worn gear. Versions 1–3 migrate potions, rewards and sword selection; earned ward armor initially equips, placing the tunic in the bag. Version 4 retains exact old bag/gear placement with empty head/off-hand slots and fresh stock. Version 5 retains all six slots, bag cells and purchases; versions 1–5 start with empty buyback. Versions 1/2 also migrate the third raider to a captain at the same health fraction. An old actor position inside the newly added counter moves to its nearest free edge. Camera, jump, target, panels, preview rotation, unfinished channels and sparring rounds reset; health and consumed resources persist. Invalid IDs, slots, duplicate gear, bad stacks, stock, receipts, buyback credits or campaign-inconsistent ownership reject the whole load. **R** resets the session without erasing the existing save; a later **F5** replaces it.

If you move the source checkout, reconfigure and rebuild so the compiled save path points to the new location.

## 8. Troubleshooting

| Symptom | What to check |
| --- | --- |
| `CMake was not found` | Verify the C++ CMake component in Visual Studio Installer, or an existing CMake installation on PATH. |
| No compiler or Windows SDK found | Verify the MSVC and Windows SDK components; configuration must succeed before building. |
| MSBuild reports access denied to `AppData/Local/Microsoft SDKs` | Use `-Backend Ninja`; it uses the installed compiler without MSBuild's per-user SDK lookup. No folder permissions or machine settings need changing. |
| Ninja prints every included header or misses header edits | Reconfigure with the current root CMake files. The project probes the raw localized MSVC dependency prefix. Clean and rebuild the generated Ninja output once if it was built with the old configuration. |
| PowerShell refuses to execute the script | Use Visual Studio's Open Folder workflow. Changing machine execution policy is not required by this guide. |
| CMake reports a generator/cache mismatch | Do not reuse a cache from another generator or checkout. Use the intended build directory; historical caches from `origin/C++` are not valid inputs. |
| The wrong program starts | Select `RPGNative.exe` in Open Folder mode, or set `RPGNative` as the startup project in solution mode. |
| Start shows `RPGNative.sln` and debugging cannot start | Follow the quick recovery steps at the top of this guide: open the `.sln` through **File > Open > Project/Solution**, then select the **RPGNative** C++ startup project. |
| Searching for `RPGNative` shows no results in Solution Explorer | Clear the search using **X**. Search filters the displayed tree; it does not select an executable. |
| Startup reports a DirectX error | Record the operation and HRESULT shown in the dialog and check Debug output. The renderer requires DirectX feature level 11.0; this is more specific than the installed DirectX version. |
| Shader compilation fails | Read Debug output for the HLSL compiler message; shader source is embedded in `engine/SceneShader.h`. |
| Game freezes when another window is focused | This is intentional suspension. Return focus to the game to resume. |
| Save/load fails | Check the configured project `saves` directory and the in-game message. Invalid files are rejected. |
| Shop gear is unavailable | Hand in the frontier patrol, check price/stock and approach the quartermaster while alive and grounded. Bought gear appears in the bag; equip it explicitly. |
| Tests fail | The helper stops before launching. Read the CTest failure output; do not treat a completed compilation as a passing test run. |

When reporting a failure, include the configuration (Debug or Release), how you opened the project, the command or steps used, and the first meaningful error message. Record actual test outcomes in [progress.md](progress.md).
`effects-preview.wav` now previews layered swings, impacts, spell chimes, potion bubbles and movement textures. Manual check 104 covers the new sound design. Preview order: Strike, Heavy, Enemy, Hit, HeavyHit, Hurt, Interrupt, Mend, Potion, Step, Jump, Land, Dash. The preview contains raw samples; the game applies its overlap-safe effects gain and volume control.
Movement tuning: jump flight now lasts 0.60 s with a 1.08 m apex. Direction+Space from rest gives a takeoff push; running jumps preserve momentum and Shift changes airborne speed only on the next takeoff. Manual check 106 covers control feel and landing poses. Validation logs: jump-weight-validation.log in each Ninja build directory.
Jump chaining: tap Space up to 120 ms before landing to launch again at contact. The queued press expires or clears on focus suspension, movement-capturing menus, defeat/respawn and loading. Manual check 107 covers chaining; jump-buffer-validation.log records regression results.
Movement presentation: reversals now face the actual travel direction while momentum brakes; acceleration/braking add a bounded torso lean. The camera follows only 35% of jump height. Manual check 109 covers live reversals, collisions and camera comfort; validation logs are locomotion-weight-validation.log.
Audio mix update: combat-mix-preview.wav is a seven-second effects-only audition of varied steps, repeated strikes, a heavy impact/interrupt and recovery spells. Runtime effects now use active-voice gain budgeting rather than permanent twelve-voice attenuation. Manual check 110 covers live listening; tests remain master-muted.
Skill 3 (Dash) now moves continuously for 0.28 seconds across 3.5 m with a dedicated pose and swept collision. It is no longer instant displacement. Movement-capturing panels/focus loss cancel the burst; cooldown remains spent. See manual check 111 and timed-dash-validation.log. FootPlacementTests writes dash-start.png, dash-peak.png and dash-finish.png.
