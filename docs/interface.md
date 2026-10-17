# Fullscreen and interface

Updated: 2026-10-17

## Display and menu behavior

Normal gameplay starts in borderless fullscreen on the window's monitor. F11, or the Display button in the Escape menu, switches between fullscreen and the previous window placement. The app does not change the monitor resolution, desktop scaling, taskbar settings or exclusive display mode. Per-monitor DPI awareness is process-local. Hidden tests remain hidden and restore their original client size after testing the fullscreen transition.

Escape always opens the game menu; pressing it again returns to the game. From another panel, Escape opens the menu. It never pauses simulation. The menu has Return, Character, Inventory, Atlas, Journal, Display, Save, Load and Exit buttons. Save/load results appear in the menu. Exit closes the game without an automatic save; F5 or Save explicitly writes progress.

Menus and panels capture movement, camera orbit and keyboard combat commands while open. Enemies, committed attacks, cooldowns and other simulation time continue. Bag actions can still use a potion or equip an owned sword. Focus loss/minimization retains the existing single-player suspension behavior; it is independent of Escape.

## Controls and screens

| Input | Behavior |
| --- | --- |
| Escape | Open/close the live game menu |
| F11 | Toggle borderless fullscreen/windowed |
| C | Character and equipment |
| I or B | Inventory |
| M | Atlas |
| J | Journal |
| Left click | Buttons, item slots, action-bar abilities and map waypoint |
| Right click on atlas | Clear waypoint |
| X button / same panel hotkey | Close the current panel |
| F2 / F3 | Hide HUD / show renderer metrics |

The HUD has player health/XP, target health and windup state, a north-up minimap, objective tracker, context action prompt, five action slots, gold and panel shortcuts. Hovering an action shows its effect and availability. Interface scaling uses a centered 1280x720 design canvas, maintaining proportions on standard and ultrawide displays.

### Inventory

The 24-slot belongings view shows the items actually owned by the simulation. Item icons, stacks, equipped markers, hover details and a selected-item description are functional. Selecting an unequipped owned sword enables Equip; selecting a usable draught enables Drink. These buttons issue the same validated gameplay commands as G and Q. Full health, death, cooldowns and lack of ownership cannot bypass simulation checks. Sword changes persist in the existing version 3 saves.

The starting tunic, trousers and boots are the current outfit. Ward armor is automatically worn over the tunic after its quest reward; it is not yet a freely interchangeable armor system. The item grid is an inspection/action interface over the current small inventory. Drag sorting, arbitrary item storage, trading, additional armor choices and item disposal are not implemented.

### Character

The character screen places equipment slots around a lit 3D preview. The preview and world share `CharacterView`; both use the current weapon and earned armor. The watchguard blade has a distinct brass/gold finish. Filled slots open that item in the bag; head/off-hand show empty slots. Attributes come from the simulation. Clothing descriptions do not invent additional combat bonuses.

### Atlas

The atlas and minimap share a generated 512-square illustration: coastline from the rendered island mesh, roads, tiled roofs, shaded tree crowns, ruins and dock planks. The chart uses undistorted world coordinates, a compass, named areas and a legend. Player facing, living raiders, unclaimed loot, keeper, anchor and current objective are live overlays.

Click traversable ground to place a waypoint; right click or Clear removes it. Invalid ground does not replace an existing valid waypoint. Waypoints are session UI state and do not move the character or alter saves. The illustration depicts this small flat test island; it is not a large authored world atlas or a claim that additional terrain exists.

### Journal

Both quest cards show their current completion/unlock state, objectives and actual rewards. A button opens the atlas. Reading the journal leaves combat active.

## Art and rendering

Original UI shapes/icons are authored in C++; no reference screenshots or downloaded art are included. The installed Windows Segoe UI font is rasterized into a 512x256 R8 atlas at startup. No font file is copied or modified. The map is generated once and uploaded with mipmaps. Font and map textures together use 1,529,172 bytes (about 1.46 MiB), additional to the existing world material/shadow allocation. These are texture allocations, not total process or VRAM use.

The interface uses alpha-blended batched triangles. A character preview flushes preceding UI, renders the shared model in a bounded viewport with studio lighting, then restores the game camera and viewport. It does not allocate a second large scene target or update the simulation.

## Reference research

Research used web search/indexed excerpts and image-search references. Direct article/image fetches were unavailable for some sources, so this is not a claim to have inspected every original page or to reproduce a specific live-game version.

- [Allods Online quick-start guide](https://allods.my.games/en/game/game-guide/quick-start): interface/navigation and game-menu reference.
- [Allods character and inventory screenshot in its forum](https://allods.my.games/forum/index.php?page=Thread&postID=74926): character information alongside an item grid and tooltips.
- [Allods character equipment reference](https://alloder.pro/main/guides/sistema-odevaniya-v-krae-mira-r1055/): dressed character, equipment slots and adjacent attributes, used as a visual reference.
- [Blizzard's UI/HUD overview](https://worldofwarcraft.blizzard.com/en-us/news/23837944/get-into-the-grid-of-things-with-the-updated-ui-and-hud): readable HUD organization and combined backpack presentation.
- [Blizzard's map/quest interface preview](https://worldofwarcraft.blizzard.com/en-gb/news/15707238/patch-602-preview-user-interface): connecting map and quest information while reducing clutter.

Allods remains the main reference. The brass/dark-panel palette, icons, small-map illustration, layouts and procedural player model here are our own implementation. No game assets were downloaded or copied.

## Validation

`InterfaceTests` checks Escape transitions, live damage during all five panels, ownership/use rules, actual equipment changes and save round trips. It creates a hidden DirectX window to check inventory/menu/map click routing at 960x600, 1280x720 and 2560x1080. Modal panels reject clicks aimed at the underlying HUD. DirectX debug warnings/errors are rejected when that optional runtime is available.

`-SmokeTest` validates hidden fullscreen size/restoration and renders spawn, raider, captain and veteran scenes. Each scenario captures the world at three sizes, two inspection views, and Menu/Inventory/Equipment/Atlas/Journal at 960x600, 1280x720 and 1600x900. Representative captures were visually inspected. Debug and Release pass all seven CTest suites.

Real keyboard/mouse interaction, Alt-Tab, multiple physical monitors, display scaling changes and longer gameplay sessions still need the manual checklist. Automated pointer tests call the same interface routing with coordinates; they do not synthesize desktop input.

The updated stationary Release benchmark reports 25 draw submissions, 0.123 ms CPU median and 0.127 ms GPU median at 1280x720 (122 asynchronous GPU samples, 240 measured frames). Process working set was 58.465 MiB and private bytes 83.547 MiB. These hidden-window measurements are not a guarantee of visible/fullscreen FPS. Re-run `-Backend Ninja -Configuration Release -Benchmark` for current local results.
