# Native architecture

## Current boundaries

The project is single-player today. Keep one map and its gameplay loop testable before expanding it. Allods Online is the primary design reference; the current art is a procedural stylized prototype.

| Component | Responsibility | Dependencies |
| --- | --- | --- |
| `WorldMap` | Map bounds, solid props, areas, traversal queries, navigation routes | C++ standard library |
| `Simulation` | Commands, actors, combat, quests, inventory, save validation | `WorldMap`, C++ standard library |
| `WorldView` | Procedural presentation of map data | `WorldMap`, `Renderer` |
| `Game` | Translate local input, update simulation, camera, interface actions, save-file I/O | Simulation, input, renderer, Windows file replacement |
| `Interface` / `InterfaceState` | Scaled layouts, hit regions, HUD/panels, validated item commands | Renderer, simulation |
| `CharacterView` / `MapArtwork` | Shared dressed character, generated terrain/landmark illustration | Renderer, world data |
| `Geometry` | Reusable indexed meshes, generated material layers, frustum tests | DirectXMath, C++ standard library |
| `Renderer` / `GpuTimer` | DirectX resources, instanced shadow/color passes, HUD batching, capture, asynchronous timing | DirectX 11, WIC |
| `Engine` / `Win32Window` | Window lifecycle, frame timing, pause on focus loss, resize | Win32, game, renderer |

`MediterraneanSimulation` is a separate library with no Windows or DirectX includes. Tests link that library rather than recompiling a separate copy of the gameplay implementation. Rendering consumes state and does not award loot or apply damage.

`Progression.cpp` implements `Simulation`'s derived XP/level rules, objective presentation data, bounded combat feedback, and death recovery. The renderer projects event positions onto the screen; it never resolves combat. Enemy kind controls health, windup, damage, and recovery. Windup facing is committed in the simulation and shared with the visible ground cone.

## Rules to preserve

- Change solid map geometry through `WorldMap`; movement, navigation, camera queries, map UI, and world presentation should agree about it.
- Use `Command` to express player intent. Validate movement, target range, cooldowns, loot eligibility, and equipment ownership inside the simulation.
- Enemy IDs are stable across save/load; vector indexes are currently local lookup details, not a future network identity contract.
- Loading validates into a candidate simulation and replaces current state only on success. Version 3 adds the anchor quest and enemy windups/facing. Versions 1 and 2 migrate with proportional captain health and progression derived from existing objectives.
- XP and ward armor derive from persistent kills/quest completion. Avoid adding duplicate mutable progression counters. Respawn retains dead enemies and claimed rewards; only explicit restart resets the campaign.
- Feedback events are bounded to 24 with a 1.5-second lifetime. Paths, feedback, and unfinished anchor channels are transient; committed enemy swings are durable.
- Navigation paths and presentation state are transient. Paths are rebuilt after loading.
- Keep tests focused on outcomes: reachable routes with collision clearance, valid combat, exactly-once rewards, and persistent state.

## Future MMO considerations

This separation makes gameplay code reusable, but the project is not network-ready. A future server will need a fixed authoritative tick, sequenced per-player commands, validated entity ownership, state replication, interest management, durable persistence, and reconciliation. Current timing is local and frame-based with a bounded delta. Same-build command replay tests do not establish determinism across platforms or compiler versions.

Do not add transport code, speculative distributed services, or extra maps merely to anticipate that future. First validate this map, document gameplay rules, and profile the real workload.

## Validation

`-Test` runs interface, geometry, simulation, navigation, inventory, combat, and progression tests. The campaign test drives real commands through both quests. `-SmokeTest` captures four scenarios at three sizes plus five interface panels and overview/front-camera views without taking over the desktop, optionally loading a validated snapshot in memory. Interactive input and feel still need the manual checklist. Generated captures and player saves remain ignored; small authored render fixtures live in `tests/fixtures`.

The build helper supports Visual Studio and Ninja in separate output directories. Ninja bootstraps MSVC in a child process, leaving machine settings untouched. `cmake/DetectMsvcIncludes.cmake` probes raw localized `/showIncludes` output to preserve Ninja header dependencies when CMake's automatic decoding mangles the prefix. Work remains uncommitted unless explicitly requested.

Graphics generation, allocations, culling and before/after measurements are documented in [graphics.md](graphics.md). Materials and indexed meshes are created once; transforms are grouped by mesh for a bounded instance upload. Shadows use their own visibility set. The render path never changes the simulation floor or obstacle map.

`MediterraneanClient` is a separate static library shared by the executable and the interface integration test. Simulation remains platform-independent. The Escape menu captures commands without bypassing `Simulation::Update`; focus-loss suspension remains an Engine responsibility. Item UI actions translate to the existing validated potion/equipment commands and do not introduce a parallel inventory model.

The renderer batches alpha-blended UI geometry and rasterizes the installed Windows font into a small atlas. Character previews temporarily use a bounded viewport and studio lighting, then restore world camera/viewport state. The illustrated map is generated once from world data. See [interface.md](interface.md) for controls, resources, reference sources and test coverage.
