# Procedural graphics and performance budget

The current direction is a more grounded, stylized RPG scene built with C++ and DirectX 11. Triangles remain the GPU representation. The visible box/pyramid blockout has been replaced with shaped meshes, generated surfaces and shadows. This is an initial art pass, not photorealism or finished character art.

No model packs, scanned datasets, downloaded textures, external engine, or runtime asset service is required. The mesh library and material images are generated once during startup and reused. GPU instancing shares geometry between actors and props. Animation currently moves shaped body parts; there is no skeletal skinning yet.

## What is rendered

- A flat gameplay surface with an irregular cliff skirt. Its top covers the shared collision rectangle; the cliff joins that boundary without a crack. Surface noise changes color, not navigation height.
- Timber/plaster houses with pitched tiled roofs, windows, doors, foundations and chimneys; irregular stones, branched trees, training dummies and floating fragments.
- Shaped torsos, heads, limbs, boots, faces, blades and armor. Walking and attack poses use procedural joint positions.
- Eleven small material layers for ground, stone, wood, plaster, roofs, cloth, metal, leaves, skin and markers. World-space texture projection supplies surface detail without UV asset files.
- Hemisphere fill light, a directional sun, one filtered shadow map, distance fog and a generated sky.
- Combat cones, targeting cues and quest crystals remain deliberately abstract for gameplay readability. Emissive and plain markers do not cast world shadows.

## Resource limits

| Resource | Current allocation / limit |
| --- | --- |
| Immutable indexed mesh library | 70,496 bytes (about 68.8 KiB); test budget 256 KiB |
| Generated materials | 11 x 128 x 128 RGBA8 layers with full mip chains; 961,180 bytes |
| Sun shadows | One 1024 x 1024 32-bit depth texture; 4,194,304 bytes |
| Materials and shadows combined | 5,155,484 bytes (4.917 MiB); test budget below 6 MiB |
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

Remaining: authored character anatomy and clothing, more natural foliage and terrain composition, skeletal animation, texture seam refinement, camera/light stability during movement and actual interactive performance testing. Conservative camera envelopes now include roofs, chimneys and canopies; traversal still uses walls and trunks. The fixed small shadow map favors memory cost over close-up shadow sharpness. This pass does not claim AAA realism, final Allods-style art quality, or guaranteed FPS.

## Subsequent interface pass

The later fullscreen/interface pass adds 1,529,172 bytes for the generated map mip chain and installed-font atlas. The world mesh/material/shadow allocations above remain unchanged. A shared CharacterView now renders the player's actual equipment both in the world and in a bounded preview viewport. Preview lighting is separate from world lighting.

The earlier comparison above is a historical graphics checkpoint; the current `Release-benchmark.txt` is replaced by subsequent benchmark runs. See [interface.md](interface.md) for the latest recorded measurements, nine-suite validation, menu behavior, generated atlas and control guide.

## Equipment and training pass

The character now has a narrower torso/limbs, smaller head details, layered tunic trim, a belt, ward crest/pauldron edging and boot cuffs. World and preview rendering use the same inventory-derived outfit. Removing the weapon hides it; removing clothes leaves neutral underclothes and removing boots reveals bare feet. These changes reuse the mesh/material library and introduce no downloaded art or additional texture allocations. They remain procedural prototype art.

Sparring captures exposed a camera trapped beside the eastern tree because the wide crown bounds extended to the ground. Camera queries now use trunk bounds below the foliage and crown bounds above it. Regression checks retain roof/crown collision while allowing the training camera to retreat underneath the canopy. The corrected scene was visually inspected with both side windows open.

The veteran smoke scenario adds base/travel outfits and sparring with/without the interface. Together with the later shop/equipment views below, the current total is 107 smoke captures per configuration. Moving-camera stability, close-up art quality and real input still require interactive inspection.

## Quartermaster and six-slot equipment pass

The shared character renderer now draws a steel helm with cheek guards and crest, plus a blue shield with brass edge and metal boss, according to actual worn head/off-hand items. The portrait can rotate through UI controls. A green-clothed quartermaster and wooden supply counter with bottles make the village trade location visible. The counter uses shared solid world data for navigation, camera queries and atlas artwork. All these assets reuse existing meshes/materials; no files were downloaded and no texture allocation was added.

New veteran captures show shop/character/bag together and the bought outfit at three sizes, a world close-up, and fully equipped sparring. Representative 960x600 and 1280x720 images were inspected for clipping, legibility and equipment visibility. Geometry remains deliberately compact procedural art; these additions do not establish realistic anatomy or finished Allods art quality.

Latest stationary Release HUD measurement: 26 submissions, 125,948 submitted triangles, CPU median/p95 0.293/0.355 ms, GPU median/p95 0.179/0.183 ms (240 GPU samples), working set 52.055 MiB, private bytes 75.711 MiB. Immutable meshes remain 70,496 bytes; materials/shadows 5,155,484; font/map textures 1,529,172. Conditions match the hidden stationary benchmark above. This does not measure moving-camera, shop-window, encounter or long-session performance.

## Selling and buyback interface pass

The merchant now uses compact offer rows above a nine-cell buyback shelf; bag details add Sell one and Split half. Existing code-drawn icons and batched UI geometry are reused, with no new meshes, materials, textures or external assets. Three new `-buyback` captures bring the smoke total to 107 per configuration. Representative 960x600 and 1280x720 views were inspected with all three windows open. The stationary benchmark above remains historical; this pass did not measure the expanded merchant window's frame cost.
