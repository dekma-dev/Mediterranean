# Native architecture

## Current boundaries

The project is single-player today. Keep one map and its gameplay loop testable before expanding it. Allods Online is the primary design reference; the current geometry is a blockout.

| Component | Responsibility | Dependencies |
| --- | --- | --- |
| `WorldMap` | Map bounds, solid props, areas, traversal queries, navigation routes | C++ standard library |
| `Simulation` | Commands, actors, combat, quests, inventory, save validation | `WorldMap`, C++ standard library |
| `WorldView` | Procedural presentation of map data | `WorldMap`, `Renderer` |
| `Game` | Translate local input, update simulation, camera, panels, save-file I/O | Simulation, input, renderer, Windows file replacement |
| `Renderer` | DirectX device/resources, shaders, meshes, HUD batching, capture | DirectX 11, WIC |
| `Engine` / `Win32Window` | Window lifecycle, frame timing, pause on focus loss, resize | Win32, game, renderer |

`MediterraneanSimulation` is a separate library with no Windows or DirectX includes. Tests link that library rather than recompiling a separate copy of the gameplay implementation. Rendering consumes state and does not award loot or apply damage.

## Rules to preserve

- Change solid map geometry through `WorldMap`; movement, navigation, camera queries, map UI, and world presentation should agree about it.
- Use `Command` to express player intent. Validate movement, target range, cooldowns, loot eligibility, and equipment ownership inside the simulation.
- Enemy IDs are stable across save/load; vector indexes are currently local lookup details, not a future network identity contract.
- Loading validates into a candidate simulation and only replaces current state on success. Version 1 migration supplies inventory defaults; version 2 stores explicit identities, loot, behavior, and equipment state.
- Navigation paths and presentation state are transient. Paths are rebuilt after loading.
- Keep tests focused on outcomes: reachable routes with collision clearance, valid combat, exactly-once rewards, and persistent state.

## Future MMO considerations

This separation makes gameplay code reusable, but the project is not network-ready. A future server will need a fixed authoritative tick, sequenced per-player commands, validated entity ownership, state replication, interest management, durable persistence, and reconciliation. Current timing is local and frame-based with a bounded delta. Same-build command replay tests do not establish determinism across platforms or compiler versions.

Do not add transport code, speculative distributed services, or extra maps merely to anticipate that future. First validate this map, document gameplay rules, and profile the real workload.

## Validation

`-Test` runs simulation, navigation, and inventory tests. `-SmokeTest` checks a real DirectX render path and writes a PNG without taking over the user's desktop. Interactive controls and gameplay feel still use the manual checklist. Build artifacts, captures, and saves remain ignored by Git. Work must remain uncommitted unless explicitly requested.
