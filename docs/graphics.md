# Procedural graphics and performance budget

The current direction is a more grounded, stylized RPG scene built with C++ and DirectX 11. Triangles remain the GPU representation. The visible box/pyramid blockout has been replaced with shaped meshes, generated surfaces and shadows. This is an initial art pass, not photorealism or finished character art.

No model packs, scanned datasets, downloaded textures, external engine, or runtime asset service is required. The mesh library and material images are generated once during startup and reused. GPU instancing shares geometry between actors and props. Animation moves shaped body parts using fixed-length, two-bone arm/leg poses; there is no skeletal skinning or authored animation clip yet.

## What is rendered

- A flat gameplay surface with an irregular cliff skirt, paved village plaza/street and dirt routes to the dock, training post and anchor. Its top covers the shared collision rectangle; the cliff joins that boundary without a crack. Surface noise changes color, not navigation height.
- Distinct homestead, watch hall, storehouse and training hall with pitched roofs, windows/shutters, timber braces, emblems and lanterns; a covered supply counter, masonry ruin, branched trees, training dummies and floating fragments.
- Folded leaf crowns, 18 shrubs and 131 grass clumps, placed away from roads, solid props and interaction areas. Small grass clumps receive shadows but do not cast them.
- Shaped torsos, heads, limbs, boots, faces, blades and armor. Walking and attack poses use procedural joint positions.
- Eleven small material layers for ground, stone, wood, plaster, roofs, cloth, metal, leaves, skin and markers. World-space texture projection supplies surface detail without UV asset files.
- Hemisphere fill light, a directional sun, one filtered shadow map, distance fog and a generated sky.
- Combat cones, targeting cues and quest crystals remain deliberately abstract for gameplay readability. Emissive and plain markers do not cast world shadows.

## Resource limits

| Resource | Current allocation / limit |
| --- | --- |
| Immutable indexed mesh library | 224,320 bytes (about 219.1 KiB); test budget 256 KiB |
| Generated materials | 11 x 128 x 128 RGBA8 layers with full mip chains; 961,180 bytes |
| Terrain surface weights | 256 x 256 RGBA8 with full mip chain; 349,524 bytes |
| Sun shadows | One 1024 x 1024 32-bit depth texture; 4,194,304 bytes |
| Materials, terrain weights and shadows combined | 5,505,008 bytes (5.250 MiB); test budget below 6 MiB |
| Instance upload | 4,096 records x 160 bytes; 655,360 bytes, shared by shadow and color passes |
| HUD upload | 16,380 vertices per upload; 655,200 bytes |

These figures describe specific renderer resources, not total VRAM or process memory. Back buffers, main depth, driver allocations, shaders, CPU caches and the window system are additional. Instance uploads fail with a diagnostic if the combined visible shadow/color instances exceed their budget. Camera and light frustum culling run independently; objects outside the camera can still cast visible shadows. No per-frame mesh or texture regeneration is performed.

## Measured comparison: 2026-10-17

Both measurements used the installed MSVC Release build, the same stationary spawn camera, a hidden 1280x720 window, presentation interval 0, 90 warmup frames and 240 measured frames. GPU timestamps are read asynchronously without forcing a flush. The baseline report contained 239 GPU samples; the final report contained 240. This is a small scene comparison on the development machine, not an interactive FPS guarantee or a cross-hardware benchmark.

| Measurement | Original blockout | Procedural pass |
| --- | ---: | ---: |
| Draw submissions including HUD | 186 | 26 |
| Submitted triangles including HUD | 9,382 | 108,119 |
| CPU frame median / p95, ms | 0.423 / 0.545 | 0.283 / 0.350 |
| GPU median / p95, ms | 0.082 / 0.084 | 0.171 / 0.175 |
| Process working set, MiB | 52.359 | 52.375 |
| Process private bytes, MiB | 69.484 | 74.504 |

The new triangle counter includes both the shadow and color passes; the original had no shadow pass. Geometry and shading cost more GPU work, while batching reduces CPU draw submissions. Working set is an OS snapshot, not a memory ceiling or leak test. The hidden benchmark does not cover input, moving-camera spikes, large encounters or long sessions.

Raw local reports: `build-ninja-release/primitive-baseline.txt` and `build-ninja-release/Release-benchmark.txt`. Generated reports are ignored by Git; the comparison above preserves this checkpoint. Re-running the benchmark replaces the current report, not the saved primitive baseline.

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -Test -SmokeTest -Benchmark
```

`-Benchmark` writes `<Configuration>-benchmark.txt` in the selected build directory. It does not modify saves. For an existing executable, use `RPGNative.exe --benchmark <absolute-report.txt>`.

In game, **F2** hides/restores the HUD. Opening the menu/panels or dying restores essential interface visibility while that state is active. Escape opens a live menu and does not pause. **F3** shows the previous completed frame's draw submissions, latest available GPU time and immutable mesh size. Use Release for performance observations; a debugger and the DirectX debug layer add overhead.

## Validation and remaining work

Debug and Release now pass all nine CTest suites. Geometry tests validate finite positions/normals, index ranges, nondegenerate triangles, normal orientation, playable-floor coverage, cliff joins, culling and resource budgets. They caught a cap-normal seam in the original procedural trunk/limb implementation, which was corrected by separating cap vertices.

Four smoke scenes (spawn, raider, captain, veteran) each render at three window sizes plus overview and front-character views. Debug smoke tests also reject DirectX warning/error messages when the optional debug layer is available. Representative captures were visually inspected. They caught intersecting armor surfaces; armor now uses the torso surface directly.

Remaining: authored character anatomy and clothing, more varied vegetation and village/ruin props, skeletal animation, texture seam refinement, camera/light stability during movement and actual interactive performance testing. Conservative camera envelopes now include roofs, chimneys and canopies; traversal still uses walls and trunks. The fixed small shadow map favors memory cost over close-up shadow sharpness. This pass does not claim AAA realism, final Allods-style art quality, or guaranteed FPS.

## Subsequent interface pass

The fullscreen/interface pass added 1,529,172 bytes for the generated map mip chain and installed-font atlas without changing world mesh/material/shadow allocations at that checkpoint. A shared CharacterView renders the player's actual equipment both in the world and in a bounded preview viewport. Preview lighting is separate from world lighting.

The earlier comparison above is a historical graphics checkpoint; the current `Release-benchmark.txt` is replaced by subsequent benchmark runs. See [interface.md](interface.md) for the latest recorded measurements, nine-suite validation, menu behavior, generated atlas and control guide.

## Equipment and training pass

The character now has a narrower torso/limbs, smaller head details, layered tunic trim, a belt, ward crest/pauldron edging and boot cuffs. World and preview rendering use the same inventory-derived outfit. Removing the weapon hides it; removing clothes leaves neutral underclothes and removing boots reveals bare feet. These changes reuse the mesh/material library and introduce no downloaded art or additional texture allocations. They remain procedural prototype art.

Sparring captures exposed a camera trapped beside the eastern tree because the wide crown bounds extended to the ground. Camera queries now use trunk bounds below the foliage and crown bounds above it. Regression checks retain roof/crown collision while allowing the training camera to retreat underneath the canopy. The corrected scene was visually inspected with both side windows open.

The veteran smoke scenario adds base/travel outfits and sparring with/without the interface. Together with the later shop/equipment and pose views below, the current total is 115 smoke captures per configuration. Moving-camera stability, close-up art quality and real input still require interactive inspection.

## Quartermaster and six-slot equipment pass

The shared character renderer now draws a steel helm with cheek guards and crest, plus a blue shield with brass edge and metal boss, according to actual worn head/off-hand items. The portrait can rotate through UI controls. A green-clothed quartermaster and wooden supply counter with bottles make the village trade location visible. The counter uses shared solid world data for navigation, camera queries and atlas artwork. All these assets reuse existing meshes/materials; no files were downloaded and no texture allocation was added.

New veteran captures show shop/character/bag together and the bought outfit at three sizes, a world close-up, and fully equipped sparring. Representative 960x600 and 1280x720 images were inspected for clipping, legibility and equipment visibility. Geometry remains deliberately compact procedural art; these additions do not establish realistic anatomy or finished Allods art quality.

Historical stationary Release HUD measurement for the quartermaster pass: 26 submissions, 125,948 submitted triangles, CPU median/p95 0.293/0.355 ms, GPU median/p95 0.179/0.183 ms (240 GPU samples), working set 52.055 MiB, private bytes 75.711 MiB. Immutable meshes were 70,496 bytes; materials/shadows 5,155,484; font/map textures 1,529,172. Conditions match the hidden stationary benchmark above. This did not measure moving-camera, shop-window, encounter or long-session performance.

## Selling and buyback interface pass

The merchant now uses compact offer rows above a nine-cell buyback shelf; bag details add Sell one and Split half. Existing code-drawn icons and batched UI geometry are reused, with no new meshes, materials, textures or external assets. Three new `-buyback` captures bring the smoke total to 107 per configuration. Representative 960x600 and 1280x720 views were inspected with all three windows open. The stationary benchmark above remains historical; this pass did not measure the expanded merchant window's frame cost.

## Character poses and workload profiling: 2026-10-18

The next character pass adds three reusable meshes: a shaped head with a jaw, cheekbones and flattened face, a fitted hair cap, and boots with an ankle, toe and flat sole. Torso proportions, collar, face details, belt, tunic trim and helmet fit were adjusted. Cloth trim follows the torso surface; profile captures exposed and helped correct floating trim and an overhanging hair cap. Ward bracers and joint covers reuse existing materials. World actors and the inventory portrait share the same outfit renderer.

`CharacterPose` solves local arm and leg joints while retaining their segment lengths. Walking bends knees forward and lifts the advancing foot; the shield arm stays forward, while the sword follows the hand through its grip, guard, blade and pommel. Player attack feedback drives the strike pose; enemies lift during windup and recover afterward. Combat timing, damage, collision and save version 6 are unchanged. This is a procedural pose system, without skinning, authored clips, root motion or true planted-foot animation.

Eight additional veteran captures show front/profile/back, both walking extremes and three strike phases under neutral preview lighting, bringing the total to 115 per configuration. GeometryTests checks fixed limb lengths, finite joints, forward knee bends, floor clearance and normalized weapon directions across the gait/attack envelope. Debug and Release pass all nine suites and all smoke captures. Close-ups and the equipped 960x600 interface were visually inspected; full interactive animation acceptance remains pending.

A subsequent bug-fix pass gates enemy walking on actual navigation travel instead of cached path presence. Stationary melee recovery and windup now retain a resting leg pose. Returning enemies turn toward their travel direction, and a fatal counterattack clears the player's active strike instead of leaving the sword raised indefinitely. Navigation/Combat regressions cover these cases; Debug/Release suites and all 115 captures were rerun. Mesh/texture allocations are unchanged. The workload tables below remain the preceding character-pass measurements, not a new benchmark of these fixes.

The [official Allods class descriptions](https://allods.ru/classes.php) provide the armored warrior/shield reference. Blizzard's [combat animation article](https://worldofwarcraft.blizzard.com/en-us/news/20946579/patch-73-new-combat-animations-its-magic) discusses responsiveness alongside animation weight in its indexed excerpt; direct page access was unavailable. Our concrete choice is to keep existing combat timing and improve visual limb/equipment continuity. These references do not establish either game's engine implementation. No game artwork or other asset files were downloaded or copied.

Run the expanded workload report with:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -Test -SmokeTest -BenchmarkSuite
```

Both comparison runs used the same hidden 1280x720 MSVC Release workload harness, presentation interval 0, a fixed 1/60-second simulation step, 90 warmup and 600 measured frames per scene. CPU timing includes the simulation step, camera setup, rendering and presentation. Snapshot setup/reloads are excluded. GPU values are asynchronous latest-ready observations, at most one per measured frame, not a frame-correlated GPU trace. The baseline had 600 GPU observations per scene; the final walk had 599 and the other scenes had 600.

| Workload | CPU median / p95 before, ms | CPU median / p95 after, ms | GPU median / p95 before, ms | GPU median / p95 after, ms |
| --- | ---: | ---: | ---: | ---: |
| Stationary spawn HUD | 0.287 / 0.326 | 0.291 / 0.331 | 0.179 / 0.183 | 0.180 / 0.187 |
| Village walking and camera orbit | 0.270 / 0.304 | 0.276 / 0.335 | 0.157 / 0.172 | 0.155 / 0.163 |
| Repeated combat and camera orbit | 0.286 / 0.364 | 0.290 / 0.358 | 0.157 / 0.167 | 0.160 / 0.170 |
| Merchant, character and bag | 0.788 / 0.979 | 0.780 / 0.947 | 0.353 / 0.365 | 0.352 / 0.368 |

| Workload | Peak submissions before / after | Peak submitted triangles before / after |
| --- | ---: | ---: |
| Stationary spawn HUD | 26 / 32 | 125,948 / 151,316 |
| Village walking and camera orbit | 26 / 32 | 126,180 / 151,548 |
| Repeated combat and camera orbit | 26 / 32 | 116,458 / 141,826 |
| Merchant, character and bag | 34 / 40 | 122,678 / 134,006 |

The walk covered 30.835 world units in the measured interval. Combat repeated a three-second scenario, with 309 measured windup frames and six received hits in both runs; these checks prevent an inert scene from passing as a movement/combat workload. The merchant scenario buys and equips the helm/shield and opens all three windows. No player saves are written.

Immutable meshes increased from 70,496 to 95,440 bytes (+24,944 bytes, about 24.4 KiB). Materials/shadows remain 5,155,484 bytes and map/font textures remain 1,529,172 bytes. End-of-suite working set/private memory changed from 59.105/83.246 to 59.172/83.832 MiB. These are process snapshots, not leak-test results or resource ceilings. The added geometry increases draw groups and submitted triangles; these short runs show comparable frame costs in this small scene, without establishing a speedup or guaranteed FPS.

Local reports: `build-ninja-release/character-baseline-benchmark.txt` and `build-ninja-release/Release-benchmark-suite.txt`. Validation logs: `build-ninja-debug/character-2026-10-18-validation.log` and the corresponding Release directory. Reports/captures remain ignored by Git; the tables preserve this checkpoint. Visible fullscreen input, large crowds, camera feel, long sessions and final character/environment quality remain unverified.

## Shared routes and foliage: 2026-10-18

The existing island now has ten authored route segments: a paved village street/plaza and dirt branches toward the dock, sparring post and northern anchor. The world shader and atlas consume the same surface query, replacing their separate infinite crossroads. A generated 256-square weight map keeps the shader from evaluating every route for every pixel; quantized weights retain a nonnegative grass remainder. Staggered paving uses derivative-smoothed mortar. The ground remains flat and existing actor/obstacle positions remain valid.

The former solid crown meshes are replaced with 144 small folded leaves per shared crown, with varied size, orientation and radius. Each leaf has separate front/back faces; ambient fill and a small transmitted-light term keep undersides readable. The same mesh forms 18 shrubs. A separate twelve-blade mesh supplies 131 grass clumps, with scattered roots, varied heights and darker bases. Placement is deterministic and bounded, avoiding roads, buildings, services, dock and sparring space. Grass skips the shadow-casting pass; trees/shrubs retain it. No alpha textures, transparency sorting, downloaded assets, extra material layers or per-frame geometry generation were introduced. Wind, seasonal variation and vegetation LODs remain unimplemented.

Four new veteran inspection views cover the island, canopy, training ground and dock, bringing the total to 119 per configuration. Their first renders exposed overly regular leaf alignment and the old dock overlapping a trunk and building. Leaf placement was varied, and the dock footprint was reduced to clear both while retaining the route endpoint. The atlas draws tree crowns over the dock. Corrected Debug close-ups, overview and atlas were inspected; Release captures also pass. Geometry/Navigation/Interface tests now check vegetation bounds, road traversal, service clearance, deterministic placement, dock clearance, surface weights and texture orientation. These tests do not establish temporal stability under interactive camera movement.

The [official Allods Svetolese description](https://allods.ru/world.php?show=zone_liga_svetolese) describes Kanian meadows, groves, Novograd's central square and shipyard. This supports our settlement/meadow/woodland direction, without copying its map or assets. Blizzard's [Artcraft level-design article](https://worldofwarcraft.blizzard.com/en-gb/news/16595426) describes reuse of wall and column art around locations in its indexed excerpt; direct access returned 403. Our implementation uses small shared meshes around existing landmarks. These sources do not establish either game's engine internals.

The before/after measurements below both include the preceding character and animation fixes. Each used the same hidden 1280x720 Release harness, presentation interval 0, 90 warmup frames and 600 measured frames per scene. The baseline combat scene has 599 asynchronous GPU observations; every other baseline/final scene has 600. Snapshot setup/reloads are excluded. Both runs record 30.835 walking units, 309 combat windup frames and six received hits.

| Workload | CPU median / p95 before, ms | CPU median / p95 after, ms | GPU median / p95 before, ms | GPU median / p95 after, ms |
| --- | ---: | ---: | ---: | ---: |
| Stationary spawn HUD | 0.297 / 0.354 | 0.312 / 0.382 | 0.180 / 0.186 | 0.203 / 0.213 |
| Village walking and camera orbit | 0.293 / 0.368 | 0.297 / 0.354 | 0.154 / 0.162 | 0.176 / 0.185 |
| Repeated combat and camera orbit | 0.305 / 0.377 | 0.326 / 0.398 | 0.159 / 0.171 | 0.187 / 0.196 |
| Merchant, character and bag | 0.769 / 0.899 | 0.802 / 0.970 | 0.349 / 0.355 | 0.376 / 0.387 |

| Workload | Peak submissions before / after | Peak submitted triangles before / after |
| --- | ---: | ---: |
| Stationary spawn HUD | 32 / 33 | 151,316 / 214,284 |
| Village walking and camera orbit | 32 / 33 | 151,548 / 216,724 |
| Repeated combat and camera orbit | 32 / 33 | 141,826 / 209,758 |
| Merchant, character and bag | 40 / 41 | 134,006 / 199,346 |

The mesh library rises from 95,440 to 165,056 bytes (+69,616 bytes, about 68 KiB); the weight texture adds 349,524 bytes (about 341 KiB). Combined world material/weight/shadow storage is 5,505,008 bytes, within the existing 6 MiB test limit. Map/font storage stays at 1,529,172 bytes. End working set/private memory changes from 59.207/84.484 to 60.098/85.715 MiB. CPU maxima before/after are 0.556/0.740, 0.698/0.485, 0.873/0.939 and 1.305/3.016 ms in table order. The merchant maximum increased even though median/p95 remain below 1 ms; short runs cannot attribute isolated spikes or establish long-session behavior. Additional foliage costs geometry/GPU time; these numbers are not a speedup or fullscreen FPS guarantee.

Reports: `build-ninja-release/environment-baseline-benchmark.txt` and `environment-final-benchmark.txt`. Final validation logs: `build-ninja-debug/environment-validation.log` and the matching Release path. Both builds pass all nine suites and 119 scenario captures, without compiler warnings/errors. Player saves are not written by this validation. See the manual checklist for visible fullscreen, camera motion and long-session checks.

## Dawnwatch landmarks: 2026-10-18

The four existing building footprints now have distinct roles and palettes: warm homestead, blue-gray watch hall, timber storehouse and training hall. Shutters, side windows, door frames, gable braces, crests and lanterns break up the walls seen from the street. The atlas consumes the same roof palette. The supply counter gains a bowed striped canopy, frame, bottles, parcel and a banded barrel. The northern ruin uses beveled masonry, arch wedges, pilasters and a faded banner; its gateway is visibly filled to match its solid footprint. The keeper proxy becomes a dressed NPC beneath the same gold quest marker, without changing interaction rules.

Four new shared meshes (barrel, arch, canopy and beveled masonry) add 59,264 bytes. Total immutable geometry is 224,320 bytes and 4,230 reusable triangles, within the existing 256 KiB / 10,000-triangle tests. Material, terrain-weight, shadow and interface texture allocations are unchanged. Instances reuse existing materials; no assets or dependencies were downloaded. Lanterns use emissive surfaces, not point lights. NPC idle/dialogue animation, interiors, destructible buildings and new gameplay services are not implemented by this art pass.

The initial ruin render showed stones buried in its backing wall; bevelled blocks and corrected depth now make the masonry visible. Village inspection framing was widened to include the gables. Supply camera checks distinguish counter/posts/braces from the curved cloth, leaving the open bay and sky clear. A regression verifies that the ordinary merchant camera retains its distance instead of snapping inward against an oversized bounding box. Building camera depth now includes the gable trim. Ground collision footprints, quest locations and save version 6 remain unchanged.

Three new veteran views (`-environment-village`, `-environment-stall`, `-environment-ruin`) bring the total to 122 captures per configuration. Corrected village/ruin/stall views, spawn HUD, atlas and merchant interface were inspected in Debug; Release rendering and all nine suites also pass. Geometry tests cover canopy bounds and the arch opening, while navigation tests cover the canopy, uprights, open bay, above-roof clearance and merchant/gable camera paths. Physical mouse orbit, temporal artifacts and visual acceptance remain on the manual checklist.

The [official Allods Svetolese description](https://allods.ru/world.php?show=zone_liga_svetolese) presents a settlement supported by rural trades, a market, civic buildings and a shipyard. We use those ideas to distinguish this small outpost's services and buildings. Blizzard's [Artcraft level-design discussion](https://worldofwarcraft.blizzard.com/en-gb/news/16595426) describes how locations express their inhabitants and history in its indexed excerpt; direct article access was unavailable. These are design references for original procedural scenery, not copied assets or evidence about either game's engine implementation.

Before/after measurements use the same hidden 1280x720 MSVC Release harness, presentation interval 0, 90 warmup and 600 measured frames per workload. All eight scene runs have 600 asynchronous GPU observations. Setup/snapshot loads are excluded. Both runs cover 30.835 walking units, 309 windup frames and six received hits.

| Workload | CPU median / p95 before, ms | CPU median / p95 after, ms | GPU median / p95 before, ms | GPU median / p95 after, ms |
| --- | ---: | ---: | ---: | ---: |
| Stationary spawn HUD | 0.316 / 0.394 | 0.350 / 0.408 | 0.202 / 0.213 | 0.207 / 0.222 |
| Village walking and camera orbit | 0.310 / 0.387 | 0.331 / 0.373 | 0.176 / 0.185 | 0.182 / 0.206 |
| Repeated combat and camera orbit | 0.320 / 0.378 | 0.354 / 0.428 | 0.185 / 0.196 | 0.194 / 0.204 |
| Merchant, character and bag | 0.793 / 0.960 | 0.855 / 1.009 | 0.370 / 0.382 | 0.375 / 0.391 |

| Workload | Peak submissions before / after | Peak submitted triangles before / after |
| --- | ---: | ---: |
| Stationary spawn HUD | 33 / 41 | 214,284 / 240,860 |
| Village walking and camera orbit | 33 / 41 | 216,724 / 242,742 |
| Repeated combat and camera orbit | 33 / 40 | 209,758 / 234,218 |
| Merchant, character and bag | 41 / 51 | 199,346 / 225,832 |

CPU maxima before/after are 0.588/1.128, 0.561/0.505, 0.972/0.720 and 1.248/1.424 ms in table order. End working set is 59.895 MiB in both runs; private memory is 84.637/84.004 MiB. These snapshots are not evidence of a memory optimization or a leak test. Added props increase draw groups, submitted geometry and CPU/GPU cost. Hidden short workloads do not establish visible fullscreen FPS or long-session stability.

Reports: `build-ninja-release/landmarks-baseline-benchmark.txt` and `landmarks-final-benchmark.txt`. Logs: `build-ninja-debug/landmarks-validation.log` and the corresponding Release path. Both final builds pass all nine suites and 122 scenario captures, with no compiler warnings/errors. Tests and benchmarks do not write player saves.
