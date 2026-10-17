# Native architecture

## Current boundaries

The project is single-player today. Keep one map and its gameplay loop testable before expanding it. Allods Online is the primary design reference; the current art is a procedural stylized prototype.

| Component | Responsibility | Dependencies |
| --- | --- | --- |
| `WorldMap` | Map bounds, solid props, areas, traversal queries, navigation routes | C++ standard library |
| `Inventory` | Canonical item catalog, bag stacks, worn slots, transactional item commands and serialization | C++ standard library |
| `Trade` | Offer catalog and Simulation purchase/range/stock rules | `Inventory`, `Simulation`, C++ standard library |
| `Simulation` | Commands, actors, combat, quests, training lifecycle, save validation | `WorldMap`, `Inventory`, C++ standard library |
| `WorldView` | Procedural presentation of map data | `WorldMap`, `Renderer` |
| `Game` | Translate local input, update simulation, camera, interface actions, save-file I/O | Simulation, input, renderer, Windows file replacement |
| `Interface` / `InterfaceState` | Scaled layouts, hit regions, HUD/panels, validated item commands | Renderer, simulation |
| `CharacterView` / `MapArtwork` | Shared dressed character, generated terrain/landmark illustration | Renderer, world data |
| `Geometry` | Reusable indexed meshes, generated material layers, frustum tests | DirectXMath, C++ standard library |
| `Renderer` / `GpuTimer` | DirectX resources, instanced shadow/color passes, HUD batching, capture, asynchronous timing | DirectX 11, WIC |
| `Engine` / `Win32Window` | Window lifecycle, frame timing, pause on focus loss, resize | Win32, game, renderer |

`MediterraneanSimulation` is a separate library with no Windows or DirectX includes. Tests link that library rather than recompiling a separate copy of the gameplay implementation. Rendering consumes state and does not award loot or apply damage.

`Progression.cpp` implements `Simulation`'s derived XP/level rules, objective presentation data, bounded combat feedback, death recovery and sparring lifecycle. The renderer projects event positions onto the screen; it never resolves combat. Enemy kind controls health, windup, damage, and recovery. Windup facing is committed in the simulation and shared with the visible ground cone. The transient trainer is separate from the campaign enemy collection, preventing practice kills from changing XP, quests or loot. `TargetActor()` resolves either target for the UI and ability checks.

## Rules to preserve

- Change solid map geometry through `WorldMap`; movement, navigation, camera queries, map UI, and world presentation should agree about it.
- Use `Command` to express player intent. Validate movement, target range, cooldowns, loot eligibility, and equipment ownership inside the simulation.
- Enemy IDs are stable across save/load; vector indexes are currently local lookup details, not a future network identity contract.
- Loading validates into a candidate simulation and replaces current state only on success. Version 5 stores `SHOP` purchase counts before six-slot `GEAR` and exact `BAG` blocks. Reject unknown IDs, incompatible slots, duplicate unique gear, invalid stack sizes, impossible stock and inconsistent ownership or gold. Version 4 reads four worn slots with empty head/off-hand slots; versions 1–3 migrate their old inventory, with proportional captain health for versions 1/2. Actors in old saves overlapping the newly solid counter move to its nearest free edge; all other position checks remain strict.
- XP and reward ownership derive from persistent kills/quest completion. Damage and protection derive from the canonical catalog and currently worn slots. Unequipping ward armor cannot remove earned XP. Respawn retains dead enemies and claimed rewards; only explicit restart resets the campaign.
- Inventory commands carry source/destination indices and the expected item. Validate them at application time so sorting or hotkey swaps cannot turn a stale drag into a different action. Add rewards atomically; full bags must not consume quest or loot credit. Worn items have a single location and never duplicate in the bag.
- Trade UI sends an offer index through `Command::purchase`. `Simulation::Buy` validates eligibility, stock, gold and capacity before committing item/charge/purchase count. `GoldSpent` derives from bounded receipts; save validation requires earned gold minus that spending. Respawn retains receipts, restart clears them. Prices/stock come from one catalog used by both the UI and simulation. Shop and portrait orientation are transient presentation state.
- Sparring uses ordinary combat rules but clamps lethal damage to 1 HP and ends. It grants no rewards and refunds no resources. The opponent, round clock and result reset on load; current health, potions and cooldowns remain durable.
- Feedback events are bounded to 24 with a 1.5-second lifetime. Paths, feedback, and unfinished anchor channels are transient; committed enemy swings are durable.
- Navigation paths and presentation state are transient. Paths are rebuilt after loading.
- Keep tests focused on outcomes: reachable routes with collision clearance, valid combat, exactly-once rewards, and persistent state.

Input polling produces an `Input::Frame`; the same transition code processes sampled frames in the hidden client integration tests. Panel/HUD/load transitions invalidate previous hit regions, and pointer dispatch checks rendered panel state and dimensions. Camera orbit belongs to the right-button gesture that began over the world; crossing from UI into the world cannot transfer that gesture to the camera. Engine focus/resize/fullscreen cancellation resets both item dragging and orbit ownership.

## Future MMO considerations

This separation makes gameplay code reusable, but the project is not network-ready. A future server will need a fixed authoritative tick, sequenced per-player commands, validated entity ownership, state replication, interest management, durable persistence, and reconciliation. Current timing is local and frame-based with a bounded delta. Same-build command replay tests do not establish determinism across platforms or compiler versions.

Do not add transport code, speculative distributed services, or extra maps merely to anticipate that future. First validate this map, document gameplay rules, and profile the real workload.

## Validation

`-Test` runs nine suites: interface, geometry, simulation, navigation, inventory, combat, progression, training and trade. The campaign test drives both quests; inventory/training/trade tests verify persistent equipment, purchases, armor and repeatable encounters without farming. `-SmokeTest` captures four scenarios at three sizes plus five interface panels, combined character/bag, inspection views and twelve extra veteran outfit/training/shop views, totaling 104 images per configuration. Tests load snapshots in memory without replacing player saves. Interactive input and feel still need the manual checklist. Generated captures and player saves remain ignored; small authored render fixtures live in `tests/fixtures`.

The build helper supports Visual Studio and Ninja in separate output directories. Ninja bootstraps MSVC in a child process, leaving machine settings untouched. `cmake/DetectMsvcIncludes.cmake` probes raw localized `/showIncludes` output to preserve Ninja header dependencies when CMake's automatic decoding mangles the prefix. Work remains uncommitted unless explicitly requested.

Graphics generation, allocations, culling and before/after measurements are documented in [graphics.md](graphics.md). Materials and indexed meshes are created once; transforms are grouped by mesh for a bounded instance upload. Shadows use their own visibility set. The render path never changes the simulation floor or obstacle map.

`MediterraneanClient` is a separate static library shared by the executable and the interface integration test. Simulation remains platform-independent. The Escape menu captures commands without bypassing `Simulation::Update`; focus-loss suspension remains an Engine responsibility. Character and bag have independent open states, with the central overlay stored separately. Reverse-ordered hit regions consume covered window surfaces; mouse orbit is suppressed over windows and during item dragging while keyboard movement stays available for the side panels. Item UI actions emit validated inventory/potion commands against the one simulation-owned inventory. Pointer release ends a drag; focus, window/display changes and panel closing cancel it. Both character render paths build their outfit from worn slots.

The renderer batches alpha-blended UI geometry and rasterizes the installed Windows font into a small atlas. Character previews temporarily use a bounded viewport and studio lighting, then restore world camera/viewport state. The illustrated map is generated once from world data. See [interface.md](interface.md) for controls, resources, reference sources and test coverage.

The merchant occupies a separate central panel while retaining independent character/bag states. Its buttons use simulation availability, and side-window item operations remain usable. E closes it without also sending an interaction that would reopen it. The counter is shared solid map data for rendering, camera collision, navigation and the atlas. Helmet/shield presentation uses the same worn-slot mapping in the world and portrait.
