# Procedural graphics and performance budget

Shadow transition repair (2026-10-19): the 20 m detail map now fades to the whole-island map over a 5.4 m edge band, with its central 8 m at full detail. This removes the bright-daylight thin-shadow transition regression: maximum channel change falls from 38 on the old X-axis test to 22 over four tested focus directions, under the unchanged limit of 25. Contact, slope, thin-caster and portrait isolation checks pass. Two shadow textures and sixteen comparisons per sampled layer are retained; the wider band increases the area sampling both layers, so this is not a performance improvement claim. Interactive acceptance is manual check 138.

## Dash motion effects - 2026-10-19

Spell 3 emits launch, finish and obstruction pulses from authoritative movement. A 32-sample path history draws three tapered cyan halos with pale inner cores. Samples interpolate actual swept endpoints on a fixed 120 Hz clock, with a separate live endpoint; the history retains at most 0.20 seconds. Speed and age shape opacity and width. Raised side strips improve low-angle visibility; two open pressure arcs follow the chest while moving. For target-directed approaches, each ribbon segment follows its own swept direction and the arcs follow the current heading, so selection changes cannot rotate old history. The final endpoint freezes when the burst finishes. A position discontinuity discards the wake. Launch cancels an old player blade trail; suspension/modal capture/defeat discard dash residue and load/restart/respawn reset all effect history.

Each 0.42-second pulse uses a 24-segment directional air ring, a small glow and ten ballistic motes; four motes become restrained grit only near the floor. Normal stops use cyan and place their ring on the ground when grounded; obstruction uses amber. Grit clamps above the floor. Launch and normal-stop events carry their elapsed time within the update so pulse age agrees with the movement. All geometry uses existing depth-tested additive effect primitives and the shared 6,144-vertex upload cap. The wake adds fixed CPU history and more vertex submissions; no new shader, texture, render target or GPU buffer is introduced. Runtime cost has not been measured.

CombatEffectsTests includes nine integrated dash captures: launch, drive, braking, normal stop, wall collision, air dash, settling, and low side/front views. Motion/animation/foot-placement tests cover the new curve and pose. None has run for this revision: native building stops at `Visual Studio locator was not found.` TargetDashTests covers the new targeting rules; existing capture fixtures now select a living actor before launch. Manual checks 128–133 cover targeting, motion, clipping, visibility and performance.

## Atmospheric daylight, cloud depth and sun - 2026-10-19

The sky uses a ground-level scattering approximation: wavelength-dependent Rayleigh extinction, broad and narrow forward Mie lobes, bounded horizon paths, and a solar disc with a 0.00465 radian angular radius. Pixel derivatives filter its edge. The sun vector is shared with both directional shadow projections. A compact analytic optical glow surrounds the disc; cloud transmission attenuates both. Reduced radiance near the horizon and a cooler lower hemisphere avoid the former pale, nearly uniform backdrop below the floating island.

The low-cloud sheet has been replaced by a shallow procedural volume with a rounded vertical profile, four billow octaves and fine erosion. Up to 48 samples integrate opacity and light front to back. Two density probes toward the sun shade the interior; sky fill and a restrained multiple-scattering approximation retain cool shadow detail and bright edges. Pixel footprint and march-segment filtering reduce aliasing. Static sub-step dither breaks slice contours without changing its pattern each frame; fine grain can still be visible, so camera-motion acceptance remains pending. The march skips rays below the cloud horizon and exits at negligible transmission. Distant banks fade into atmospheric radiance; a sparse higher layer adds thin streaks.

Broad sunlight attenuation uses twelve coarse samples of the same density field per world vertex, then interpolates across the surface. The low-frequency footprint spans hundreds of metres, avoiding a cloud-volume integration in every opaque pixel. This is an approximation: at the island origin, 24 sampled drift phases differ from sky-path transmission by up to 29/255. A separate 24-phase GPU probe bounds diagonal interpolation error across a 42 x 46 m ground face to 15/255; actual mesh integration and whole-scene cost still need native validation. A slow closed drift repeats every 1,800 seconds, with the existing bounded double clock. Clouds do not change gameplay or the static sun direction.

World surfaces retain warm attenuated sunlight and sky-coloured hemisphere fill. Height-sensitive exponential transmittance mixes distant surfaces with sky radiance in linear space before the fitted filmic shoulder/display encoding. Effects use matching haze transmittance. Portraits bypass cloud lighting and retain their independent neutral response; HUD colour paths are unchanged. The sky still renders behind opaque geometry in one fullscreen draw. No new textures, render targets, mesh assets, downloads or installations are required. Scene constants remain 384 bytes. The sky and world vertex stages do more work; reduced pixel-shader bytecode is not a performance claim. Historical timing tables below do not describe this revision.

RMB pitch reaches -0.75 radians so the sun is visible. A downward camera boom stays at least 0.30 m above the flat floor and still checks buildings/trees. These controls are retained from the first atmospheric pass.

Validation: `pwsh -File scripts/check-scene-shader.ps1 -Preview` compiles all eight production SM5 shader entries without warnings/errors using the installed Windows compiler. Its D3D11 WARP helper renders nine isolated 960x540 images for sun, horizon, opposite direction, clouds, later drift, closed period, next frame, clear sun and covered sun. Image probes confirm sun alignment, movement (173 maximum channel levels across 180 seconds), exact closed-period repeat, and removal of the solar core under an opaque deck. GPU function probes cover finite results, clear/opaque transmission and the 24-phase sky/surface-light comparison. A 60 Hz drift step produces 0.0079 levels of mean RGB change at the fixed test view. Captures were visually inspected. The report is `build-shader-check/sky-refinement-validation.log`; these are isolated shader diagnostics, not rebuilt-game screenshots or FPS measurements.

The native build/test helper still stops at `Visual Studio locator was not found.` Existing ShadowTests sun alignment/geometry occlusion/timing and NavigationTests/MotionTests camera checks remain unrun for this revision. Manual checks 123–124 cover integration, camera-motion grain, vertex interpolation, sunlight readability, portrait/HUD isolation and performance. Flying through clouds, temporal reconstruction, full multiple scattering, varying weather/day-night cycles and physically varying shadow penumbrae remain future work.

References: [NVIDIA GPU Gems: atmospheric scattering](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering) and [volume rendering](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-39-volume-rendering-techniques) provide background for extinction, scattering and front-to-back compositing. This implementation is a bounded procedural approximation. The tone curve uses the [Narkowicz fitted filmic shoulder](https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/). [Blizzard's lighting article](https://worldofwarcraft.blizzard.com/en-us/news/23494819) and the [official Allods media gallery](https://allods.my.games/en/media) inform the outdoor art direction. No reference artwork was copied or downloaded.

## Hand-drawn map artwork - 2026-10-19

The 512 x 512 RGBA map generator now uses parchment grain, light washes, antialiased sepia strokes, shoreline hachures and illustrated trees/buildings. It draws into the existing 1,048,576-byte CPU pixel result with small temporary coastline/symbol vectors. Renderer upload size and mip chain remain 1,398,100 bytes; combined map/font storage remains 1,529,172 bytes by the unchanged allocation formula. World geometry, collision and terrain surfaces are not modified by this presentation pass.

The atlas reuses its existing map rectangle/projection and adds ink lettering without drop shadows, marginal annotations, a compass rose and a five-metre scale. The minimap samples the same illustration. Objective rings have a dark under-stroke so gold stays visible on paper. The old extra gold coastline overlay is removed.

Reference direction: the user requested a drawn map; Blizzard's [classic manual](https://us.media.blizzard.com/manuals/wow/wow-classic-manual-enUS.pdf) describes an illustrated zone-map presentation. The art here is generated locally from this project's island data. No external images or assets were downloaded.

Native compilation, generated atlas captures and startup/per-frame cost measurements are pending because MSVC and Windows SDK headers are unavailable. No performance improvement or visual acceptance is claimed. Existing scaled interface checks request atlas-drawn captures; manual check 120 covers the map sheet, minimap, marker contrast and waypoint accuracy.

## Village facade detail - 2026-10-19

Buildings now use individual foundation stones, alternating corner masonry, projecting sills/lintels, narrow glazing bars, shutter hinges, individual door planks, iron straps/rivets and chimney courses with a dark inset flue. Details reuse existing meshes/materials and remain within existing camera bounds. The established coarse building collision footprints are unchanged; this does not add enterable interiors.

Release village capture inspected. One hidden benchmark pair (village-detail-before.txt / village-detail-after.txt) reports GPU median/p95 ms: spawn 0.418/0.430 to 0.433/0.456; walking 0.338/0.368 to 0.345/0.379; combat 0.349/0.370 to 0.360/0.381; merchant 0.645/0.664 to 0.667/1.011. CPU medians increase from 0.405/0.386/0.436/0.978 to 0.559/0.522/0.561/1.219 ms respectively. The merchant tail increases materially in this short sample; this is not a sustained FPS claim. Combat peak submissions/triangles rise from 57/537840 to 58/588056. Shared mesh and texture allocations remain unchanged; additional instances increase scene work. Manual check 117 covers camera clearance, readability and live cost.

## Character geometry and frame detail - 2026-10-19

Head/hair use 32 radial segments, torso/boots/spheres 24, and limbs 20, with additional face, torso and limb profile rings. Eye sockets are recessed. Armor adds overlapping abdominal and shoulder plates, rivets and knee guards; hands add thumbs and curled finger ridges. Interface corners use layered metal leaves and engraved banner trim. Existing equipment and animation transforms remain authoritative; no downloaded assets or new texture allocations.

Reusable geometry totals 278,112 bytes and 5,766 triangles (previous storage 224,320 bytes). The geometry budget is now 320 KiB; the 10,000 shared-triangle ceiling is unchanged. Whole-scene peak submitted triangles include instances and shadow passes: combat increases from 311,768 to 537,840. Draw submissions remain 57 in that workload. Material/shadow storage remains 9,699,312 bytes and interface storage 1,529,172 bytes.

One hidden 1280x720 workload pair reports GPU median/p95 ms: spawn 0.373/0.389 to 0.418/0.430, walking 0.319/0.341 to 0.338/0.368, combat 0.330/0.347 to 0.349/0.370, merchant 0.589/0.605 to 0.645/0.664. Reports: detail-before.txt and detail-after.txt in build-ninja-release. These short measurements are not sustained fullscreen FPS. Close-up character and combined character/bag captures were inspected. Anatomy, authored materials and world density still need substantial improvement. Manual check 116 covers motion and distance acceptance.

## Worn surfaces - 2026-10-19

Paving now uses rounded, irregular edges, chipped bevel relief, cooler/warmer stone variation and moss-toned joints. Recessed joints attenuate ambient light only; this is a material cavity approximation, not scene ambient occlusion. Rock surfaces add periodic mineral strata and small pits. Cloth gains restrained folds beneath its weave; metal gains fine directional texture and roughness variation. Texture dimensions, meshes, draw counts and fixed allocations are unchanged. Ground pixels add one noise evaluation and rounded-edge arithmetic; other material changes are generated at startup.

The supplied Allods screenshot and [official media gallery](https://allods.my.games/en/media) remain art references. This pass improves surface depth and material separation, not character anatomy or building silhouettes. Those remain significant gaps from the reference. No external assets were downloaded. Manual check 105 covers moving-camera acceptance and fullscreen performance.

One hidden 1280x720 before/after workload pair (90 warmup + 600 measured frames) reports GPU median/p95 ms: stationary 0.343/0.364 to 0.353/0.376; walking 0.302/0.321 to 0.306/0.325; combat 0.310/0.327 to 0.315/0.331; merchant 0.514/0.561 to 0.515/0.665. The merchant tail increased; this short pair includes run-to-run variation and does not establish fullscreen performance. Mesh bytes (224,320), material/shadow bytes (9,699,312), submissions and gameplay counters are unchanged. Reports: `surface-before.txt` / `surface-after.txt` in Release. Final village and character captures were visually reviewed; matching before/after copies use `surface-village-*` and `surface-character-*` names.

## Directional combat effects - 2026-10-19

Player slash crescents add five thin edge fragments (30 vertices). Heavy impacts and interrupts add one delayed colored ring (6 vertices each). Spark fans follow impact facing, slow horizontally using analytic drag and shift color over their fixed lifetime; the main contact glow is reduced. Interrupts retain a radial upward burst. Existing texture-free additive rendering, depth testing, read-only render behavior, 32 burst / 8 trail pools and the 6,144-vertex upload ceiling are unchanged. Healing geometry is unchanged.

One before/after hidden 1280x720 workload pair (90 warmup + 600 measured frames) reports combat CPU median/p95 0.524/0.649 to 0.486/0.590 ms and GPU 0.309/0.327 to 0.308/0.325 ms. This small unpaired sample includes run-to-run variation and does not establish a speedup or sustained FPS. Combat effect peak vertices rise from 534 to 564; effect frames (265), scene submissions (57), windup frames (270) and received hits (6) remain unchanged. Fixed resource allocations are unchanged. Reports are `effects-refinement-before.txt` / `effects-refinement-after.txt` in Release; build logs use `effects-refinement-validation.log`. Impact/fade captures were visually inspected, and manual check 101 covers moving-camera readability and overlapping effects.

## Weathered building materials - 2026-10-19

Plaster now combines broad limewash variation, elongated rain staining and a muted damp tint near foundations. Roof tiles vary individually in brightness while retaining their existing relief. Timber uses warped periodic grain with shallower relief and subtler stripe contrast, reducing the corrugated appearance on posts, doors and the counter. These are restrained surface refinements; building silhouettes and character anatomy remain prototype geometry. No external assets were downloaded.

Texture dimensions, mesh data, draw counts and resource allocations are unchanged: material/shadow storage remains 9,699,312 bytes. Timber/roof detail is generated once at startup; plaster adds two procedural noise evaluations to affected pixels. Existing material tests cover deterministic generation, roughness bounds, relief and resource budgets. Village and close-up stall captures were visually inspected.

One hidden 1280x720 Release before/after pair (90 warmup + 600 measured frames per workload) gives GPU median/p95 ms: stationary 0.345/0.367 to 0.356/0.372; walk/orbit 0.296/0.314 to 0.300/0.319; combat 0.307/0.323 to 0.309/0.325; merchant 0.511/0.554 to 0.517/0.544. CPU medians are 0.474/0.465/0.477/0.895 before and 0.444/0.429/0.436/0.891 after; this short unpaired sample does not demonstrate CPU improvement or sustained FPS. Geometry/submission counts and gameplay counters are unchanged. Reports: `material-weathering-before.txt` and `material-weathering-after.txt` in the Release directory. Before images are retained as `material-village-before.png` and `material-stall-before.png`. Manual check 100 covers moving-camera quality and fullscreen performance.

## Swept collision workload check - 2026-10-18

The kinematic collision pass adds no rendering resources or allocations. One hidden 1280x720 Release comparison (90 warmup + 600 measured frames per workload) gives CPU median/p95 ms: stationary 0.452/0.560 → 0.492/0.604, walking 0.444/0.518 → 0.455/0.554, combat 0.464/0.590 → 0.473/0.575, merchant 0.873/1.045 → 0.925/1.074. GPU medians remain within 0.003 ms; merchant GPU p95 changes from 0.551 to 0.704 ms. This small unpaired workload sample includes variation and does not isolate collision cost or certify sustained FPS. Walked distance (31.908), windup frames (270) and received hits (6) remain unchanged in these fixtures. Reports: `swept-physics-before.txt` / `swept-physics-after.txt` in the Release directory. Manual check 90 covers live behavior/performance near contacts.

## Blade history regression fix - 2026-10-18

Blade samples are now committed at most 120 times per second while retaining a live endpoint, so the fixed 32-slot trail covers its intended 160 ms at high update rates. Previously the history became shorter above roughly 200 Hz. The synthetic rendered sweep changed from 304/301/76 pixels wide at 60/144/1000 Hz to 304/301/301; this checks visible history retention, not exact rate-independent artwork. State size, GPU buffers and lifetime are unchanged. Three `effects-history-*.png` diagnostics bring CombatEffectsTests to 39 captures; the 1000-Hz result was inspected. Full validation logs are `trail-history-validation.log`; manual high-refresh acceptance is check 89. No new performance claim is made.

## Instance CPU optimization - 2026-10-18

The private instance submission path accepts scaled orthonormal transforms. Its normal basis is each world basis row divided by that row's squared length, equivalent in xyz to inverse-transpose for these transforms. The maximum squared length also yields the culling radius scale with one square root. No GPU formats, shaders, meshes, texture allocations, instance limits or draw ordering change. A new geometry regression compares 120 nonuniformly scaled/rotated/translated transforms to the previous general inverse. Sheared matrices are outside this helper's contract.

Three runs per build, each with four hidden 1280x720 workloads (90 warmup + 600 frames, present interval 0), give these medians across runs:

| Workload | CPU median before → after (ms) | CPU p95 before → after (ms) |
| --- | --- | --- |
| Stationary | 0.479 → 0.464 | 0.573 → 0.553 |
| Walk/orbit | 0.459 → 0.439 | 0.579 → 0.544 |
| Combat | 0.489 → 0.469 | 0.604 → 0.577 |
| Merchant | 0.914 → 0.904 | 1.085 → 1.055 |

GPU medians remain similar: stationary 0.327–0.339 → 0.328–0.338 ms, walking 0.295–0.298 → 0.297–0.298, combat 0.306–0.307 → 0.307–0.309, merchant 0.513–0.514 → 0.514–0.515. All before runs preceded the after runs; ordinary run-order/hardware variation may contribute to these small gains. No sustained/fullscreen FPS claim is made. Submissions, triangles, gameplay counters and fixed GPU allocations are unchanged.

Of 132 Release smoke captures, 113 are byte-identical. Nineteen contain only one or two changed channels, each by one 8-bit level, consistent with floating-point rounding. References were matched to original Release hashes. Reports: `transform-before-1.txt` through `transform-before-3.txt`, corresponding `transform-after-*.txt`, `transform-before-images.json` and `transform-image-comparison.txt` in the Release build directory. Full validation logs are `transform-validation.log` in each Ninja directory. Manual check 88 covers live performance and visual acceptance.

## Cubic shadow filtering - 2026-10-18

Both layers use normalized cubic B-spline weights over the same sixteen depth comparisons. Each tap retains receiver-plane depth correction; bias, resolution and texture allocations are unchanged. The tighter central lobe strengthens thin/contact shadows, and weight derivatives remain continuous across texel boundaries. This does not remove caster rasterization aliasing or supply distance-dependent soft shadows. The existing thin-post fixture's minimum/mean contrast improves from 34.33/45.33 to 41.00/56.96; snapped-focus pixel difference remains 1. The near/far blend step increases from 10 to 18, below its existing 25-unit threshold. Review this sharper transition in manual check 87.

One hidden Release before/after pair (1280x720, 90 warmup and 600 measured frames per workload) reports:

| Workload | CPU median/p95 ms, before → after | GPU median/p95 ms, before → after |
| --- | --- | --- |
| Stationary | 0.485/0.558 → 0.473/0.584 | 0.349/0.369 → 0.350/0.369 |
| Walk/orbit | 0.457/0.566 → 0.472/0.584 | 0.295/0.312 → 0.297/0.315 |
| Combat | 0.474/0.587 → 0.495/0.606 | 0.305/0.322 → 0.307/0.324 |
| Merchant | 0.885/1.040 → 0.906/1.077 | 0.510/0.534 → 0.514/0.687 |

GPU observations range from 591 to 600 per workload. This short pair is not a sustained FPS result: median GPU changes are small, but the merchant p95 increased and needs live profiling. Peak draws/triangles, gameplay counters and asset allocations are unchanged. End working/private memory is 61.016/88.996 → 61.074/89.832 MiB. Reports are `shadow-cubic-before.txt` and `shadow-cubic-after.txt` in the Release directory. Validation uses the existing fourteen suites and 132 smoke views per configuration, with tightened thin-shadow acceptance thresholds; representative contact and village captures were inspected.

## Recovery effects - 2026-10-18

Successful Mend emits two green ascending ribbons and ten motes; potions emit one gold ribbon and six motes. A restrained central glow completes each pulse. Residue stays at the activation position and fades in 0.8 seconds. Effects reuse the existing depth-tested additive batch, fixed 32-burst pool and capped 6,144-vertex upload. Mend submits 306 vertices, a potion 162; there are no new textures, shaders, meshes or GPU buffers and fixed effect state remains 8,520 bytes. Mixed-lifetime replacement favors expired effects over active longer pulses.

Debug/Release pass fourteen suites, 132 standard smoke views, 36 effect diagnostics and twelve foot-placement views. Recovery diagnostics cover gameplay views and equal-time 30/60/144-Hz rendering; the gameplay views were inspected. Current logs are `recovery-validation.log` in each build directory. Actual fullscreen readability and overlapping-effect cost remain manual checks 84–85; this pass has no new timing benchmark or FPS claim. Existing attack-only benchmark figures below do not measure healing.

The current direction is a more grounded, stylized RPG scene built with C++ and DirectX 11. Triangles remain the GPU representation. The visible box/pyramid blockout has been replaced with shaped meshes, generated surfaces and shadows. This is an initial art pass, not photorealism or finished character art.

No model packs, scanned datasets, downloaded textures, external engine, or runtime asset service is required. The mesh library and material images are generated once during startup and reused. GPU instancing shares geometry between actors and props. Animation moves shaped body parts using fixed-length, two-bone arm/leg poses; there is no skeletal skinning or authored animation clip yet.

## What is rendered

- A flat gameplay surface with an irregular cliff skirt, paved village plaza/street and dirt routes to the dock, training post and anchor. Its top covers the shared collision rectangle; the cliff joins that boundary without a crack. Surface noise changes color, not navigation height.
- Distinct homestead, watch hall, storehouse and training hall with pitched roofs, windows/shutters, timber braces, emblems and lanterns; a covered supply counter, masonry ruin, branched trees, training dummies and floating fragments.
- Folded leaf crowns, 18 shrubs and 131 grass clumps, placed away from roads, solid props and interaction areas. Small grass clumps receive shadows but do not cast them.
- Shaped torsos, heads, limbs, boots, faces, blades and armor. Walking and attack poses use procedural joint positions.
- Flat-ground planted-foot steps for the player and enemies, stable support-boot headings, arm counter-swing and pelvis compensation for fixed-length legs. Airborne/idle portrait poses still use the procedural animation layers; there is no skeletal skinning or terrain-slope adaptation.
- Eleven packed material layers for ground, stone, wood, plaster, roofs, cloth, metal, leaves, skin and markers. Object-aligned triplanar projection blends color, height slopes and roughness without UV asset files; detail follows animated parts. Ground routes remain in world coordinates.
- Linear-light surface shading, roughness-dependent highlights, warm sunlight/cool hemisphere fill, overlapping whole-island and focused detail shadow maps, surface-aware tent filtering, distance fog and layered procedural clouds.
- Combat cones, targeting cues and quest crystals remain deliberately abstract for gameplay readability. Their explicit caster flags keep them out of the world shadow pass; ordinary plain or emissive solids can cast shadows independently of material.
- Blade-following trails, layered gold/cyan/violet slash crescents, expanding impact rings, bright cores with soft halos, tapered sparks and rising motes use a bounded additive world pass with read-only depth; they cast no shadows and render beneath the interface.

## Resource limits

| Resource | Current allocation / limit |
| --- | --- |
| Immutable indexed mesh library | 224,320 bytes (about 219.1 KiB); test budget 256 KiB |
| Generated materials | 11 x 128 x 128 RGBA8 layers with full mip chains; 961,180 bytes |
| Terrain surface weights | 256 x 256 RGBA8 with full mip chain; 349,524 bytes |
| Sun shadows | Two 1024 x 1024 D32 layers; 8,388,608 bytes (8 MiB) |
| Materials, terrain weights and shadows combined | 9,699,312 bytes (9.250 MiB); test budget below 10 MiB |
| Instance upload | 4,096 distinct visible records x 160 bytes; 655,360 bytes, shared by all shadow/color passes |
| HUD / effect upload (shared) | 16,380 vertices per upload; 655,200 bytes |
| Combat effect CPU state | 8,520 bytes; 32 bursts, eight trails with 32 samples and attack origin/facing each; test budget 16 KiB |
| Combat effect CPU vertex queue | 6,144 vertices / 245,760 bytes reserved; newest excess quads are dropped |
| Foot-placement CPU controller | 152 bytes per actor, fixed two-foot state; 256-byte test budget |

These figures describe specific renderer resources, not total VRAM or process memory. Back buffers, main depth, driver allocations, shaders, CPU caches and the window system are additional. A world/portrait batch uploads at most 4,096 distinct instances visible in any of its passes and fails with a diagnostic if that union exceeds the limit. Each instance is uploaded once; shadow/color passes share its stored transform without multiplying the capacity cost. Dynamic-buffer discard allows the driver to retain data used by earlier frames; the listed buffer size is not a bound on driver backing allocations. Camera and light frustum culling run independently; objects outside the camera can still cast visible shadows. No per-frame mesh or texture regeneration is performed.

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

Debug and Release now pass all twelve CTest suites. AnimationTests checks channel transitions, rig constraints, floor/weapon clearance and simulation equivalence. ShadowTests compares GPU-rendered pixels for contact, caster materials, receiver slopes, moving/offscreen casters, clearing, resize and portrait isolation. Geometry tests validate finite positions/normals, index ranges, nondegenerate triangles, normal orientation, playable-floor coverage, cliff joins, culling and resource budgets. They caught a cap-normal seam in the original procedural trunk/limb implementation, which was corrected by separating cap vertices.

Four smoke scenes (spawn, raider, captain, veteran) each render at three window sizes plus overview and front-character views. Debug smoke tests also reject DirectX warning/error messages when the optional debug layer is available. Representative captures were visually inspected. They caught intersecting armor surfaces; armor now uses the torso surface directly.

Remaining: authored character anatomy and clothing, more varied vegetation and village/ruin props, skeletal animation, texture seam refinement, camera/light stability during movement and actual interactive performance testing. Conservative camera envelopes now include roofs, chimneys and canopies; traversal still uses walls and trunks. A focused detail map improves close-up shadows while distant coverage uses the coarser fixed map. Local shadowed lights and distance-varying penumbrae remain unimplemented. This pass does not claim AAA realism, final Allods-style art quality, or guaranteed FPS.

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

## Motion response checkpoint: 2026-10-18

The movement pass adds scalar pose/speed/camera state, without new meshes, textures, shaders or downloaded assets. Immutable geometry remains 224,320 bytes, material/terrain/shadow storage 5,505,008 bytes and interface storage 1,529,172 bytes. Character limb geometry is unchanged. Gait follows actual travel and blends into idle; strike poses blend, player heading follows the shortest arc and enemy heading matches its committed damage cone. Orbit, wheel zoom and camera height ease during play; geometry retraction remains immediate. This is procedural pose refinement, without skeletal clips or planted feet.

Final hidden 1280x720 Release workload results (90 warmup + 600 measured frames each, presentation interval 0):

| Workload | CPU median / p95 / max, ms | GPU median / p95, ms | GPU observations | Peak draws / triangles |
| --- | ---: | ---: | ---: | ---: |
| Stationary spawn HUD | 0.392 / 0.620 / 3.304 | 0.294 / 0.507 | 95 | 41 / 240,860 |
| Village walking and orbit | 0.337 / 0.422 / 0.793 | 0.270 / 0.292 | 591 | 41 / 243,244 |
| Repeated combat and orbit | 0.351 / 0.429 / 0.552 | 0.302 / 0.323 | 591 | 40 / 234,218 |
| Merchant, character and bag | 0.807 / 0.971 / 1.258 | 0.552 / 0.624 | 589 | 51 / 225,832 |

Walking covers 31.908 units; combat records 270 windup frames and six hits. The committed baseline covered 30.835 units / 309 windup frames / six hits: acceleration, arrival and facing rules intentionally change those paths/timings. GPU observations are asynchronous, and the stationary scene has sparse coverage. The benchmark calls the immediate camera-placement API; interactive orbit/zoom easing is verified separately by motion/input tests and still needs physical playtesting.

An unexpected rise in process memory triggered a fresh build of local committed HEAD `43a0afe`, exported under the ignored build directory without switching the working tree. Its working set/private memory measured 93.027/118.523 MiB initially and 180.586/205.625 MiB on a repeat. The final changed build measured 178.562/204.656 MiB; earlier changed runs were 152.543/177.539 and 180.336/205.586 MiB. The same elevated readings therefore also occur without these source changes. Fixed GPU resource counts agree, but these short unpaced hidden runs cannot establish a leak, memory improvement or steady gameplay working set. Queue/driver behavior is a possible explanation, not a measured attribution; paced long-session profiling remains open. No FPS or speedup claim follows from these numbers.

Final report: `build-ninja-release/motion-final-benchmark.txt`. Baseline reports: `build-ninja-release/motion-baseline-source/build-ninja-release/Release-benchmark-suite.txt` and `build-ninja-release/motion-baseline-repeat.txt`. Final Debug/Release logs: `motion-validation.log` in their respective build directories. All ten suites and 122 scenario captures per configuration pass; no compiler warnings/errors were reported. Combat/sparring captures were inspected. Saves were not written. Manual checks 47–49 cover motion quality, rapid camera input and collision recovery.

### Smoothing fixes follow-up

The gait filter now integrates its moving sine target over the travel interval, removing frame-dependent stride phase/amplitude. Squared gait gives torso lift a smooth neutral crossing. Camera recovery requires 100 ms of continuous clearance, while collision retraction and inward zoom remain responsive. WASD shares the displayed filtered heading, and cancellation discards pending rotation. Constant-gravity integration fixes frame-dependent jump height. Direct pursuit tracks current positions and close occluded enemies continue navigating. MotionTests reproduced seven failures before the fixes and now passes them with the existing combat, collision and save regressions.

The same Release harness reports the following after the fixes; all scenes contain 600 CPU samples. These are short hidden workloads, subject to the same asynchronous GPU/memory limitations above.

| Workload | CPU median / p95 / max, ms | GPU median / p95, ms | GPU observations |
| --- | ---: | ---: | ---: |
| Stationary spawn HUD | 0.394 / 0.615 / 2.120 | 0.293 / 0.508 | 110 |
| Village walking and orbit | 0.354 / 0.423 / 0.576 | 0.274 / 0.294 | 590 |
| Repeated combat and orbit | 0.375 / 0.464 / 0.604 | 0.312 / 0.335 | 587 |
| Merchant, character and bag | 0.838 / 0.990 / 1.460 | 0.574 / 0.627 | 588 |

Draw/triangle peaks match the preceding motion checkpoint. Walking still covers 31.908 units, combat records 270 windup frames and six hits, and fixed resource allocations remain 224,320 / 5,505,008 / 1,529,172 bytes for mesh / material-terrain-shadow / interface storage. End working set/private memory is 178.613/203.184 MiB, within the earlier observed variation; this is not a leak or FPS test. Report: `build-ninja-release/motion-fixes-benchmark.txt`. Both configurations pass all ten suites and 122 smoke captures, with no compiler warnings/errors reported. Logs: `motion-fixes-validation.log` in each build directory. No player saves were written. Manual checks 50–52 remain pending.

## Sun shadow refinement - 2026-10-18

The sun map remains fixed in world space at 1024 x 1024, using 4 MiB of D32 depth storage. Camera movement does not refit or rescale it. Opaque objects now cast independently of material through `ShadowMode`; the old material filter incorrectly removed Plain solids. Grass, hovering effects, lantern cores and combat cues opt out explicitly. The flag occupies an existing instance component, preserving the 160-byte record and all allocation budgets.

The shader fits the receiver plane in shadow UV/depth coordinates. Sixteen point comparisons at actual texel centers use a separable tent footprint, replacing nine bilinear comparisons with one depth value per footprint. This permits smaller, bounded depth bias and avoids the scalloped trim edges found in the first implementation during image review. The relative derivative guard avoids depending on output resolution; the gradient and raster offsets are bounded for grazing surfaces. Sun diffuse and metallic highlights remain shadowed, ambient fill remains visible, and portrait lighting stays independent. This is a soft filtered directional shadow, not distance-varying physical penumbra or a local-light shadow system.

Method references: Microsoft's [receiver-plane depth correction discussion](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps#calculating-a-per-texel-depth-bias-with-ddx-and-ddy-for-large-pcfs) and [Direct3D depth-bias behavior](https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias). This implementation uses one map, without cascades or downloaded sample code/assets.

GPU regressions reproduced a zero shadow contribution for a Plain post (Stone: 114.67 intensity levels), and just 6.33 levels of contact contrast for a 0.05-unit-high solid. Final values are 114.67 for both post materials and 65 for that low object. Fifteen receiver orientations remain within 3 intensity levels of their non-casting references at the sampled points. Additional tests cover marker exclusion, moving/offscreen casters, clearing when casters disappear, three render sizes and portrait isolation. These are same-run image comparisons, not cross-GPU golden screenshots or proof of perfect temporal stability. Diagnostic `shadow-*.png` files are separate from the existing 122 game smoke captures.

Measured before/after with the same prior movement code, hidden 1280x720, presentation interval 0, fixed 1/60-second simulation, 90 warmup and 600 measured frames per workload:

| Workload | CPU median / p95 before → after (ms) | GPU median / p95 before → after (ms) | GPU observations before → after |
| --- | --- | --- | --- |
| Stationary HUD | 0.392 / 0.618 → 0.389 / 0.629 | 0.378 / 0.867 → 0.367 / 0.607 | 79 → 43 |
| Walking/orbit | 0.368 / 0.476 → 0.353 / 0.451 | 0.257 / 0.282 → 0.293 / 0.319 | 531 → 394 |
| Combat/orbit | 0.379 / 0.476 → 0.370 / 0.460 | 0.317 / 0.337 → 0.318 / 0.340 | 592 → 579 |
| Merchant/character/bag | 0.835 / 0.985 → 0.801 / 0.958 | 0.580 / 0.613 → 0.612 / 0.660 | 588 → 581 |

Peak submissions remain 41 / 41 / 40 / 51. Submitted triangle peaks become 245,084 / 247,468 / 238,442 / 227,944 because Plain physical details now participate in the shadow pass. Walking still covers 31.908 units; combat records 270 windup frames and six hits. Mesh, material/terrain/shadow and interface allocations remain 224,320 / 5,505,008 / 1,529,172 bytes. Working set/private bytes at the end move from 138.332/164.438 to 180.566/204.391 MiB; prior unpaced runs already showed wide process-memory variation, so this is not evidence of a new leak or a memory ceiling. GPU observations are asynchronous, with sparse stationary coverage; the table does not establish an interactive FPS guarantee or a general speedup.

Reports: `build-ninja-release/shadows-baseline-benchmark.txt` and `shadows-final-benchmark.txt`. Both builds pass all eleven suites and 122 game smoke captures, with no compiler warnings/errors reported. Validation logs: `shadows-validation.log` in each build directory. Representative final character/wall-trim, stall and combat images were inspected; manual checks 53–55 cover visible motion, close surfaces, fullscreen/refocus and preview behavior. Player saves are not written by these tests. Remaining limitations include the fixed map's close-up resolution, distant decorative fragments outside its coverage, absent local shadowed lights and pending sustained interactive playtests.

## Layered character animation - 2026-10-18

Player/enemy presentation now blends idle breathing/sway, movement lean, air poses, landing compression and hit recoil with the existing walking/strike pose. Height and vertical speed distinguish ascent/descent; air leg lift reaches zero before grounded rendering. A bounded landing pulse lowers the hips while the fixed-length leg solve retains the standing ankle targets. The torso pivot also transforms the head, shoulder roots, clothing and hand attachments. NPC/portrait idle motion uses supplied visual time; player/enemy state advances only during updates. Hit refresh preserves ongoing recoil, including damage on consecutive frames. Combat facing, collision, root positions, jump physics and version 6 persistence are unchanged.

This adds small scalar CPU state, without new mesh/texture/shader resources or external assets. Immutable mesh / material-terrain-shadow / interface allocations remain 224,320 / 5,505,008 / 1,529,172 bytes. It is procedural part animation: authored skeletal clips, locomotion foot locking, anatomy improvements, cloth simulation and sound remain separate work.

The same four hidden 1280x720 workloads were measured before and after, with 90 warmup + 600 measured frames, a fixed 1/60-second simulation step and presentation interval 0:

| Workload | CPU median / p95 before → after (ms) | GPU median / p95 before → after (ms) | GPU observations before → after |
| --- | --- | --- | --- |
| Stationary HUD | 0.432 / 0.637 → 0.426 / 0.697 | 0.591 / 0.613 → 0.594 / 0.615 | 44 → 49 |
| Walking/orbit | 0.343 / 0.441 → 0.371 / 0.470 | 0.290 / 0.317 → 0.294 / 0.325 | 455 → 492 |
| Combat/orbit | 0.388 / 0.495 → 0.406 / 0.513 | 0.319 / 0.365 → 0.346 / 0.362 | 590 → 582 |
| Merchant/character/bag | 0.832 / 0.958 → 0.869 / 1.024 | 0.622 / 0.648 → 0.634 / 0.859 | 585 → 579 |

Peak submissions/triangles remain 41/245,084, 41/247,468, 40/238,442 and 51/227,944. Walking still covers 31.908 units; combat records 270 windup frames and six hits. End working set/private bytes are 180.527/205.605 MiB before and 180.613/206.086 MiB after. These short unpaced runs use asynchronous GPU observations and sparse stationary coverage; they are not an interactive FPS guarantee, long-session memory test or general performance conclusion.

Reports: `build-ninja-release/animation-baseline-benchmark.txt` and `animation-final-benchmark.txt`. Both configurations pass all twelve suites and 132 smoke captures. A final audit fixed a shadowed local variable in the capture code; both subsequent rebuilds report no compiler warnings/errors. Logs: `animation-validation.log` and `animation-rebuild.log` in each Ninja build directory. Added views cover idle inhale/exhale, rise/fall, landing and recoil plus four actual world-jump stages. Representative rise/fall/landing/recoil captures were inspected. Numerical tests cover fixed limb lengths, floor and sword clearance across extreme channel combinations, phase wrapping, 30/60/144-Hz transitions, repeated damage, resets, simulation/save equivalence and render immutability. Manual checks 56–58 remain necessary for temporal feel, attachment appearance during input and visible performance. These tests do not write player saves.

## Animation transition fixes - 2026-10-18

Recoil now consumes explicit transient damage events. The previous flash-timer comparison could miss a second hit on a longer frame and continuously refresh a reaction when defeat stopped the enemy's timer. Enemy/trainer flashes now decay before the defeated-player early return. Contact timing and impact velocity come from the jump solver; the landing pulse includes the part of the frame after touchdown. The same 0.8-second jump sample now yields 0.816757 compression at 30/60/144 Hz and uneven steps, replacing 0.955375 / 0.775920 / 0.849328. This changes transition timing, without adding GPU resources, altering damage rules or changing save version 6.

Both configurations pass all twelve suites and 132 smoke captures without compiler warnings/errors (`animation-fixes-validation.log`). Failing reproductions remain in Debug `animation-fixes-before.log`. The tests also verify single-update event lifetimes, cooldown rejection, exactly one contact per jump and save/load isolation. Landing/recoil images were inspected; visible timing still needs manual checks 59–60.

The same hidden workload was rerun in Release; final CPU/GPU timings below are observations, not an FPS or speedup claim:

| Workload | CPU median / p95 (ms) | GPU median / p95 (ms) | GPU observations |
| --- | --- | --- | --- |
| Stationary HUD | 0.424 / 0.651 | 0.433 / 0.614 | 40 |
| Walking/orbit | 0.377 / 0.469 | 0.304 / 0.331 | 397 |
| Combat/orbit | 0.386 / 0.470 | 0.325 / 0.356 | 587 |
| Merchant/character/bag | 0.849 / 0.983 | 0.623 / 0.659 | 580 |

Report: `build-ninja-release/animation-fixes-benchmark.txt`. Draw/triangle peaks remain unchanged, as do 31.908 units walked, 270 windup frames and six received hits. Fixed mesh / material-terrain-shadow / interface allocations remain 224,320 / 5,505,008 / 1,529,172 bytes. End working set/private memory is 180.703/205.191 MiB. The short, unpaced asynchronous measurements do not establish sustained interactive performance or a long-session memory bound.

## Smooth pose transitions - 2026-10-18

Visual player turns, gait and strikes now retain velocity through critically damped transitions. Gait integrates the sinusoidal target and its derivative analytically; walking still depends on actual travel, and constant-speed poses agree at 30/60/144 Hz. The player turns over the shortest arc while enemy headings remain simulation-owned. Positive stride/ascent lift uses a squared envelope, and height-based airborne weighting uses a cubic envelope with zero slope at its ends. Physical movement, combat events, collision, resources and persistence are unchanged.

At a 0.1 ms finite-difference probe, the gait stop's velocity discontinuity drops from 10.9279 to 0.0488758 pose units/second. Tests cover turn/attack starts, gait stops, rapid retargeting, bounds, reset velocities, exact enemy heading and foot/air boundary slopes. Both configurations pass twelve suites and 132 captures without compiler warnings/errors (`smoothness-validation.log`). Prior failing acceptance checks are retained in Debug `smoothness-before.log`. Walking/falling images were inspected; manual checks 61–63 still govern visible feel.

Release timings from the same hidden workload configuration:

| Workload | CPU median / p95 (ms) | GPU median / p95 (ms) | GPU observations |
| --- | --- | --- | --- |
| Stationary HUD | 0.424 / 0.651 | 0.423 / 0.610 | 41 |
| Walking/orbit | 0.354 / 0.454 | 0.304 / 0.331 | 273 |
| Combat/orbit | 0.371 / 0.484 | 0.311 / 0.344 | 587 |
| Merchant/character/bag | 0.806 / 0.962 | 0.615 / 0.644 | 581 |

Report: `build-ninja-release/smoothness-benchmark.txt`. Peak submissions/triangles remain 41/245,084, 41/247,468, 40/238,442 and 51/227,944. Walking remains 31.908 units, with 270 combat windup frames and six received hits. Fixed mesh / material-terrain-shadow / interface allocations remain 224,320 / 5,505,008 / 1,529,172 bytes. End working set/private memory is 180.781/207.133 MiB. These short asynchronous, unpaced measurements are not an interactive FPS or memory-bound guarantee and do not establish a speedup.

## Focused shadow detail - 2026-10-18

The fixed 62-unit projection sampled the world at about 0.0605 units per texel. A second 1024-square projection now covers 20 units around the world camera target, giving about 0.0195 units per texel (3.1 times finer linear sampling). Its extent, orientation and depth range stay fixed; its center snaps to whole light-space texels. The original projection remains fixed in world space. The pixel shader selects by map coverage and blends across an overlap, computing both receiver-plane gradients before selection branches. Most pixels use one 16-comparison filter; overlap pixels sample both maps. Portrait cameras cannot move the world projections.

The two D32 layers consume 8,388,608 bytes, exactly 4 MiB more than before. Combined material/terrain/shadow storage rises from 5,505,008 to 9,699,312 bytes (9.250 MiB); its test budget is deliberately revised from 6 to 10 MiB. Mesh/interface storage remains 224,320 / 1,529,172 bytes. Independent caster culling and a position-only depth vertex shader reduce unnecessary work in each depth pass. All three world passes reuse the existing 4,096-instance buffer, and no textures are allocated per frame. No assets or dependencies were downloaded.

Microsoft's [shadow projection stability, map selection and overlap guidance](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps) informed the fixed extents, texel snapping, blend and derivative placement. The shader uses [Texture2DArray comparison sampling](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texture2darray-samplecmplevelzero). This implementation uses two overlapping projections, without view-frustum split cascades or copied sample assets.

An eight-offset thin-post probe failed before the change: minimum/mean shadow contrast was 0 / 44.79 intensity levels. It now measures 78.67 / 91.38. A 0.05-unit-high object's contact contrast improves from 65 to 102.33. Stationary shadow edges change by at most one pixel-intensity level while the focus moves through twelve grid steps. Thirty-two overlap steps stay below 25 levels of change (observed maximum 19). CPU tests check sub-texel stability, whole-texel translation and unchanged distant projection. GPU tests also verify distant fallback and preserve material, slope, moving/offscreen caster, clearing, resize and portrait-isolation coverage. These are synthetic same-run probes, not universal shimmer or image-quality guarantees.

The same hidden 1280x720, presentation-interval-zero suite uses 90 warmup and 600 measured frames per workload at a fixed simulation step of 1/60 second:

| Workload | CPU median / p95 before (ms) | CPU median / p95 after (ms) | GPU median / p95 before (ms) | GPU median / p95 after (ms) | GPU observations before / after |
| --- | --- | --- | --- | --- | --- |
| Stationary HUD | 0.424 / 0.651 | 0.535 / 0.817 | 0.423 / 0.610 | 0.616 / 0.689 | 41 / 42 |
| Walking/orbit | 0.354 / 0.454 | 0.381 / 0.459 | 0.304 / 0.331 | 0.334 / 0.351 | 273 / 399 |
| Combat/orbit | 0.371 / 0.484 | 0.398 / 0.497 | 0.311 / 0.344 | 0.352 / 0.387 | 587 / 585 |
| Merchant/character/bag | 0.806 / 0.962 | 0.836 / 0.971 | 0.615 / 0.644 | 0.643 / 0.695 | 581 / 578 |

Peak submissions/triangles rise from 41/245,084 to 58/296,828, 41/247,468 to 58/301,078, 40/238,442 to 56/309,558 and 51/227,944 to 68/285,640 in table order. Final CPU maxima are 1.378 / 0.550 / 0.712 / 1.232 ms. Walking still covers 31.908 units; combat still records 270 windup frames and six hits. End working set/private memory changes from 180.781/207.133 to 180.977/211.332 MiB. GPU observations are asynchronous and stationary coverage is sparse; these short unpaced runs do not establish interactive FPS, steady memory use or long-session stability.

Reports: `build-ninja-release/smoothness-benchmark.txt` and `shadow-detail-benchmark.txt`. Debug and Release each pass all twelve suites and 132 smoke captures without compiler warnings/errors (`shadow-detail-validation.log`). The failing thin-post reproduction is in Debug `shadow-detail-before.log`. Representative character, village and thin-post captures were visually inspected. Manual checks 64–66 remain pending for visible movement, overlap transitions, display changes and sustained performance. Distant shadows remain coarser, coverage remains bounded, and local shadowed lights and distance-varying penumbrae remain future work. Tests do not write player saves.

## Shadow queue and capacity fixes - 2026-10-18

Fully culled submissions previously remained in the queue. Changing from the world camera to the character portrait could reveal those discarded objects inside the portrait; a fully culled portrait could likewise draw geometry back into the world. Two image comparisons reproduced maximum channel differences of 255 and 195 against clean references. Both now measure zero. Submitted queues are consumed even with no visible instances; already empty flushes remain no-ops, preserving existing world shadows across HUD/capture/portrait calls.

The additional shadow layer also reduced effective scene capacity because an object could occupy three copies of the fixed upload. A 1,400-object reproduction failed. The renderer now calculates per-object visibility masks, groups records so each pass can draw contiguous ranges, and uploads each distinct visible object once. Adjacent ranges are merged. All passes share one 655,360-byte upload buffer; the scene budget is 4,096 distinct visible objects across the union of passes. The dense reproduction and exact 4,096-object boundary pass, while 4,097 is rejected with a diagnostic. This does not remove the scene limit or bound all driver allocations.

The shadow filters, projections, caster policy and fixed mesh/material/interface allocations are unchanged. Existing contact, thin-post, grid-stability, blend, slope, clearing, resize and offscreen-caster checks still pass. Debug and Release each pass all twelve suites and 132 captures without compiler warnings/errors. Final logs: `shadow-fixes-validation.log` in each Ninja directory; failing reproductions: Debug `shadow-fixes-before.log`. Character and preview captures were inspected. Manual check 67 remains necessary for ordinary interactive preview transitions.

Release measurements use the same hidden 1280x720, presentation-interval-zero, 90-warmup/600-frame workload as the preceding checkpoint:

| Workload | CPU median / p95 before (ms) | CPU median / p95 after (ms) | GPU median / p95 before (ms) | GPU median / p95 after (ms) | GPU observations before / after |
| --- | --- | --- | --- | --- | --- |
| Stationary HUD | 0.535 / 0.817 | 0.440 / 0.702 | 0.616 / 0.689 | 0.665 / 0.687 | 42 / 38 |
| Walking/orbit | 0.381 / 0.459 | 0.379 / 0.485 | 0.334 / 0.351 | 0.333 / 0.352 | 399 / 332 |
| Combat/orbit | 0.398 / 0.497 | 0.383 / 0.491 | 0.352 / 0.387 | 0.343 / 0.373 | 585 / 589 |
| Merchant/character/bag | 0.836 / 0.971 | 0.811 / 0.984 | 0.643 / 0.695 | 0.643 / 0.681 | 578 / 580 |

Peak submissions/triangles remain 58/296,828, 58/301,078, 56/309,558 and 68/285,640 in table order. Final CPU maxima are 1.567 / 0.810 / 0.630 / 1.240 ms. Walking remains 31.908 units; combat records 270 windup frames and six hits. Fixed mesh / material-terrain-shadow / interface storage remains 224,320 / 9,699,312 / 1,529,172 bytes. End working set/private memory changes from 180.977/211.332 to 180.590/209.738 MiB. These asynchronous unpaced measurements, especially the sparse stationary GPU observations, do not prove a speedup, sustained FPS or a memory ceiling. Reports: `build-ninja-release/shadow-detail-benchmark.txt` and `shadow-fixes-benchmark.txt`. No player saves are written by validation.

## CPU rendering performance - 2026-10-18

This pass removes repeated CPU work while preserving the rendered result. Each flush prepares the camera/light planes and normal lengths once, then reuses them for all sphere tests. Visibility classification counts the seven nonempty masks; prefix offsets and a stable scatter pack instances in the existing order without seven mesh-queue rescans. Interface ovals, rings and scrollwork reuse 728 bytes of fixed curve samples, retaining current positions, scaling, colors and tessellation. The shaders, shadow maps, geometry, draw ranges and simulation behavior are unchanged.

All 132 Release PNGs match the pre-change files byte for byte by SHA-256. This includes the three capture sizes, combined character/bag/shop views, outfits, combat, environment and animation poses. Both configurations pass all twelve suites and 132 smoke captures without compiler warnings/errors, including culling boundaries, offscreen shadow casters, portrait isolation and the 4,096/4,097-instance capacity checks. Logs: `performance-validation.log` in each Ninja build directory. Capture comparisons: `build-ninja-release/performance-image-comparison.csv`; baseline PNGs remain in its `performance-baseline-captures` directory.

Three pairs of Release runs compare a saved pre-change executable against the optimized build. Order is baseline/optimized, optimized/baseline, then baseline/optimized. Each uses the same hidden 1280x720 window, zero presentation interval, fixed 1/60 simulation step, 90 warmup frames and 600 measured frames per workload. The following values are medians of the three per-run statistics, not percentiles pooled across 1,800 samples. CPU time includes submission and presentation.

| Workload | CPU median / p95 before (ms) | CPU median / p95 after (ms) | CPU median change | GPU median / p95 before (ms) | GPU median / p95 after (ms) |
| --- | --- | --- | --- | --- | --- |
| Stationary HUD | 0.435 / 0.692 | 0.456 / 0.697 | +4.8% | 0.526 / 0.688 | 0.575 / 0.687 |
| Walking/orbit | 0.369 / 0.478 | 0.349 / 0.440 | -5.4% | 0.335 / 0.356 | 0.336 / 0.379 |
| Combat/orbit | 0.404 / 0.502 | 0.357 / 0.456 | -11.6% | 0.342 / 0.373 | 0.318 / 0.346 |
| Merchant/character/bag | 0.827 / 0.977 | 0.778 / 0.930 | -5.9% | 0.642 / 0.684 | 0.629 / 0.662 |

All three paired runs improve active-scene CPU medians. Stationary medians vary from 0.427–0.528 ms before and 0.388–0.575 ms after, so that workload shows no consistent gain. GPU observations per run before/after range from 37–39 / 36–37 stationary, 59–232 / 41–56 walking, 574–588 / 264–359 combat and 581–583 / 581–583 merchant. The GPU sampling is asynchronous and sparse in some workloads; unchanged GPU work and these samples do not establish a general GPU speedup or gameplay FPS improvement.

Peak submissions/triangles remain 58/296,828, 58/301,078, 56/309,558 and 68/285,640. Walking still covers 31.908 units; combat still records 270 windup frames and six hits. Fixed mesh / material-terrain-shadow / interface allocations remain 224,320 / 9,699,312 / 1,529,172 bytes; the instance and HUD buffers are also unchanged. End working set ranges from 180.445–181.227 MiB before and 180.266–180.773 MiB after; private bytes range from 208.594–210.543 and 209.848–210.383 MiB. These snapshots are not memory ceilings or leak tests.

Saved paired reports are `build-ninja-release/performance-baseline-run1.txt` through `run3.txt` and `performance-optimized-run1.txt` through `run3.txt`. Initial validation reports are `performance-baseline-benchmark.txt` and `performance-final-benchmark.txt`. No assets/dependencies were downloaded, GPU resources added or player saves written. Visible fullscreen response and sustained performance remain manual check 68.

## Surface depth and lighting - 2026-10-18

This pass follows the user's Allods screenshot toward textured fantasy materials and readable outdoor lighting. Reference searches surfaced the [official Allods media gallery](https://allods.ru/official_media.php), the [Eldjun environment page](https://allods.ru/world.php?show=questionable_eldjun), and Blizzard's [lighting/shadow workshop](https://worldofwarcraft.blizzard.com/en-us/news/23494819). Search-indexed text and image results were available; a separate direct-page request timed out. These are art-direction references, not claims about Allods engine internals. No reference assets were copied or downloaded into the project.

The existing eleven RGBA8 layers now pack albedo intensity in R, signed height slopes in G/B and roughness in A. Slopes use code 128 as zero and 32 codes per unit. Periodic noise produces wood grain/plank seams, stone relief, plaster grain, woven cloth, overlapping roof tiles and brushed metal. Wrapped central differences build slopes once; generated mips filter them along with color and roughness. The alpha channel is material data, not opacity.

Object-aligned, scale-aware triplanar sampling replaces dominant-axis world projection. Curved surfaces blend projection directions, and textures follow moving/rotating body parts. Screen derivatives turn the blended slopes into a bounded surface gradient without tangent vertices or new mesh attributes. Paving has irregular joints, varied stone color and shallow bevel shading; its collision surface stays flat. Plaster darkens near the ground. Lighting now evaluates authored colors in linear space with warm sunlight, cool ambient fill, roughness-dependent highlights, a highlight shoulder and display conversion. Layered cloud noise replaces the previous broad patches. The follow-up below fixes sky orientation; physical atmosphere, local lights, authored anatomy, dense natural foliage and skeletal art are separate future work.

The new response exposed an existing near-grazing shadow error: the receiver-depth gradient clamp was too small. Raising its bound from 0.002 to 0.02 depth units per texel removes the stripe without increasing constant bias. All fifteen slope probes now report zero shadow/no-shadow difference; the prior test threshold was retained. Contact, thin-post, offscreen-caster, overlap, clearing, resize, portrait isolation and capacity checks still pass. Five translated curved/nonuniform material probes report mean channel differences below 0.00009 on the 0–255 scale, verifying that textures no longer slide through moving objects. GeometryTests checks deterministic packed surfaces, neutral marker/glow slopes, usable relief and roughness/resource bounds.

Debug and Release each pass all twelve suites and 132 smoke captures without compiler warnings/errors (`realism-validation.log` in both build directories). Village, stall, ruin and character views were inspected. Baseline Release captures remain in `build-ninja-release/realism-baseline-captures`. No geometry, texture, render-target or instance buffer was added. Fixed mesh / material-terrain-shadow / interface allocations remain 224,320 / 9,699,312 / 1,529,172 bytes. The changed shaders do more pixel work; this is a visual-quality improvement, not a performance optimization.

One before/after Release pair uses the same hidden 1280x720, zero-presentation-interval, fixed 1/60-second workload, with 90 warmup and 600 measured frames per scene:

| Workload | CPU median / p95 before → after (ms) | GPU median / p95 before → after (ms) | GPU observations before / after |
| --- | --- | --- | --- |
| Stationary HUD | 0.501 / 0.685 → 0.514 / 0.934 | 0.496 / 0.684 → 0.509 / 0.923 | 39 / 34 |
| Walking/orbit | 0.371 / 0.476 → 0.430 / 0.491 | 0.339 / 0.380 → 0.422 / 0.474 | 83 / 35 |
| Combat/orbit | 0.362 / 0.451 → 0.417 / 0.472 | 0.340 / 0.358 → 0.408 / 0.443 | 584 / 36 |
| Merchant/character/bag | 0.753 / 0.879 → 0.770 / 0.934 | 0.622 / 0.667 → 0.633 / 0.699 | 582 / 571 |

Peak submissions/triangles remain 58/296,828, 58/301,078, 56/309,558 and 68/285,640. Walking remains 31.908 units, with 270 combat windup frames and six received hits. End working set/private memory rises from 181.004/209.461 to 194.559/224.727 MiB; unchanged fixed asset buffers do not imply unchanged total process memory. These short asynchronous observations are sparse for several scenes and do not establish fullscreen FPS, native-resolution cost or a memory ceiling. Reports: `realism-baseline-benchmark.txt` and `realism-final-benchmark.txt` in the Release build directory. Manual checks 70–72 cover motion artifacts, material appearance and visible performance. No player saves were written.

### Camera-oriented sky bug fix - 2026-10-18

An image regression reproduced identical sky pixels for different camera headings/pitches. The sky now reconstructs a world ray from the current inverse view-projection, subtracts camera position and samples clouds in direction space. This avoids camera-translation parallax, a longitude seam and division by the horizon. `BeginFrame` schedules the sky; the first flush draws it once before geometry or a portrait changes the viewport. Camera overrides used by environment captures are therefore honored in the same frame. The inverse matrix reuses the existing constant-buffer world slot, and initialization sets a valid default camera. No extra draw, texture or buffer is introduced.

Debug and Release each pass twelve suites and 132 smoke captures (`graphics-bugs-validation.log`). Sky tests cover default-camera initialization, yaw/pitch response, translation invariance, one draw in an empty frame and repeat-view stability. Existing preview isolation, resize, culling and all fifteen grazing-shadow probes pass. Material anchoring remains below 0.00016 mean channel difference on the 0–255 scale after the sky change, well within the unchanged 0.15 test limit. GPU-test initialization failures now print the renderer's actual shader error. Village, canopy and sky-direction images were inspected; manual check 73 covers interactive transitions.

The final Release smoke/benchmark run uses the same workload configuration described above. Stationary/walking/combat/merchant CPU medians are 0.521/0.437/0.423/0.793 ms; GPU medians are 0.512/0.429/0.414/0.642 ms, from 36/36/35/571 asynchronous GPU observations. Peak submissions/triangles, gameplay counters and fixed allocations remain unchanged. End working set/private bytes are 185.465/213.691 MiB. This single follow-up run is not a new paired speedup claim or a memory ceiling; the preceding paired measurements remain historical evidence of the material pass. The full report is `build-ninja-release/graphics-bugs-benchmark.txt`.

## Initial combat attack effects - 2026-10-18

The effects keep the weapon and enemy telegraph readable: gold for Strike, blue-white for Heavy Strike and its interrupt glints, orange for enemy swing/hit feedback. These colors and timings are original prototype choices. Reference review used the official [Allods warrior description](https://allods.ru/about.php?show=class-warrior) for the melee identity and Blizzard's [combat animation article](https://worldofwarcraft.blizzard.com/en-us/news/20946579/patch-73-new-combat-animations-its-magic) for its emphasis on responsive feedback and distinct combat moments. No reference art, effect textures, models or dependencies were downloaded.

Simulation emits a bounded one-update event stream only after validated player attacks or resolved enemy windups. A hit burst is anchored to the struck surface toward the attacker; its six/nine sparks follow deterministic analytical trajectories for up to 0.42 seconds. The central soft glow fades within 0.16 seconds. Trails sample the actual posed blade for 0.24 seconds and retain at most 0.16 seconds of history, with a two-unit discontinuity cutoff. They follow the same `WeaponPoint` helper as the weapon mesh. Effects do not delay or alter authoritative damage. Failed commands create nothing, missed enemy swings retain a trail without a false impact, and interrupted windups do not strike. Death cancels trails while existing sparks expire; load/restart/respawn reset the pools. Rendering and portraits never advance effect time.

Eight trails (32 samples each) and 32 bursts occupy 8,392 bytes of fixed CPU state. The renderer reserves a 6,144-vertex, 240 KiB CPU queue and drops excess quads rather than reallocating. A new vertex/pixel shader pair, blend state, rasterizer and read-only depth state render a single additive batch after opaque world geometry, before HUD and portrait depth clearing. Analytic edge masks soften ribbons, elliptical sparks and glows without textures. Additive blending is independent of submission order; opaque depth still occludes the effects. The existing 655,200-byte dynamic HUD vertex buffer is reused with discard uploads; no additional GPU texture, mesh or vertex buffer is created. Pipeline objects and driver backing memory are additional to the fixed asset counters.

`CombatEffectsTests` adds nineteen diagnostic PNGs, bringing the suite count to thirteen; the ordinary 132 smoke views remain separate. Debug and Release pass both sets (`combat-effects-validation.log`). Coverage includes command validation, misses/interrupts, unarmed attacks, death/load/restart/respawn, live menus, 30/60/144-Hz expiration, overload bounds, teleport discontinuities, depth occlusion, additive order, HUD/portrait isolation, repeat-render stability and real Strike/Heavy/enemy-hit views. Images were inspected. Manual checks 74–77 cover temporal feel, fullscreen cost and physical window transitions.

One before/after Release pair uses the existing hidden 1280x720 workload, fixed 1/60-second step, zero presentation interval, 90 warmup and 600 measured frames:

| Workload | CPU median / p95 before → after (ms) | GPU median / p95 before → after (ms) | GPU observations before / after |
| --- | --- | --- | --- |
| Stationary HUD | 0.739 / 0.935 → 0.677 / 0.944 | 0.744 / 0.933 → 0.683 / 0.928 | 36 / 36 |
| Walking/orbit | 0.441 / 0.533 → 0.444 / 0.531 | 0.434 / 0.476 → 0.439 / 0.498 | 34 / 33 |
| Combat/orbit | 0.430 / 0.510 → 0.427 / 0.501 | 0.423 / 0.446 → 0.420 / 0.447 | 36 / 36 |
| Merchant/character/bag | 0.814 / 0.947 → 0.777 / 0.911 | 0.670 / 0.731 → 0.646 / 0.704 | 579 / 542 |

The combat run submits effects in 244/600 measured frames, peaking at 96 effect vertices. Peak combat submissions/triangles change from 56/309,558 to 57/309,570; other workloads submit zero effects and retain their previous geometry/draw peaks. Walking remains 31.908 units, with 270 combat windup frames and six hits. Fixed mesh/material-terrain-shadow/interface counts remain 224,320/9,699,312/1,529,172 bytes. End process working set/private memory changes from 181.031/209.438 to 209.863/238.215 MiB: buffer reuse does not imply unchanged total process memory. These short, sparse asynchronous observations do not establish a speedup, native-resolution FPS or a sustained memory ceiling. Full reports are `combat-effects-baseline-benchmark.txt` and `combat-effects-final-benchmark.txt` in the Release build directory.

### Layered crescents, halos and afterglow - 2026-10-18

The follow-up makes attack silhouettes larger and more expressive, following the user's Asian-MMO visual request. Strike has a gold crescent, Heavy Strike a cyan crescent with a delayed violet counter-sweep, and enemy swings a smaller orange sweep. Each crescent has 28 segments with tapered ends, a broad colored halo and a narrow bright edge. Progress follows a cubic ease-out with a smooth envelope over 0.32 seconds. The attack origin/facing remain fixed in world space while the actual blade-history ribbon follows the posed weapon. A two-unit blade discontinuity cancels both components. The wider silhouette does not change attack range or damage.

Impact bursts combine a compact white-tinted core and soft halo, a short expanding ring, contact glints, tapered ballistic sparks with glowing heads, and rising motes. The contact core fades within 0.20 seconds and the ring within 0.22; remaining particles expire at 0.48. Heavy hits use twelve sparks and six motes; ordinary hits eight/three, interrupts ten/three, and received hits six sparks without motes. Analytic shader masks provide the halos and a derivative-filtered ring edge with a transparent center. This adds no fullscreen bloom target or downloaded effect texture. An attempted official Asian-MMO reference lookup failed, so no new external images were reviewed or copied; the colors, shapes and timings are original implementation choices.

The existing effect shaders/batch and GPU upload buffer are reused. Storing attack origins/facings adds 128 bytes of fixed CPU state, bringing it to 8,520 bytes. The 240 KiB vertex queue and 6,144-vertex cap remain unchanged. At saturation, newer quads are discarded; an eight-heavy-trail/32-impact stress test reaches that cap and renders successfully. Larger halos increase overdraw and shader work even though fixed GPU allocations and the single effect draw remain unchanged.

Debug and Release each pass all thirteen suites, 132 smoke captures and 28 separate effect diagnostic captures without compiler warnings/errors. Logs: `cinematic-effects-validation.log` in each Ninja directory. New GPU checks cover ring depth occlusion, a transparent ring center, overload and equal-time 30/60/144-Hz rendering (mean channel difference below the test threshold of 0.05 on a 0–255 scale). Gameplay-distance, front, opposite-side, impact/trail and afterglow captures were visually inspected. Earlier lifecycle, miss/interrupt, reset, unarmed, depth/blend, live-menu and HUD/portrait tests remain active. Manual checks 78–80 cover live feel and fullscreen acceptance.

One before/after Release pair uses the initial effect build as the baseline, with the same hidden 1280x720 workload, fixed 1/60-second step, zero presentation interval, 90 warmup and 600 measured frames:

| Workload | CPU median / p95 before → after (ms) | GPU median / p95 before → after (ms) | GPU observations before / after |
| --- | --- | --- | --- |
| Stationary HUD | 0.518 / 0.964 → 0.516 / 0.939 | 0.511 / 0.934 → 0.512 / 0.928 | 34 / 35 |
| Walking/orbit | 0.440 / 0.510 → 0.434 / 0.490 | 0.426 / 0.482 → 0.429 / 0.482 | 34 / 34 |
| Combat/orbit | 0.421 / 0.518 → 0.420 / 0.480 | 0.414 / 0.447 → 0.414 / 0.437 | 36 / 36 |
| Merchant/character/bag | 0.823 / 0.963 → 0.790 / 0.949 | 0.658 / 0.861 → 0.620 / 0.700 | 577 / 560 |

Combat effect frames rise from 244 to 265 of 600 measured frames, with peak effect vertices 96 → 534. Peak combat submissions remain 57 and peak triangles change from 309,570 to 309,582; frame maxima need not occur on the same frame. Other workloads still submit zero effect vertices and retain their prior submission/triangle peaks. Walking distance remains 31.908 units, with 270 combat windup frames and six hits. Fixed mesh/material-terrain-shadow/interface allocations remain 224,320/9,699,312/1,529,172 bytes. End working set/private memory changes from 206.305/236.336 to 210.023/240.430 MiB. The measurements are short and GPU observations sparse; they do not prove a speedup, sustained memory ceiling or fullscreen FPS. These workloads also do not represent maximum overlapping heavy bursts.

Reports: `build-ninja-release/cinematic-effects-baseline-benchmark.txt` and `cinematic-effects-final-benchmark.txt`. Initial-effect reference PNGs remain in `build-ninja-release/cinematic-effects-baseline-captures`. No player saves were written.

## Planted-foot movement - 2026-10-18

The grounded locomotion layer replaces the traveling sine's ankle positions with world-space contacts and lifted steps. A supporting boot retains both position and heading until its recovery step starts. Step goals anticipate measured root travel and ease toward new goals after steering/stopping; normal steps alternate, with early support release when running or reversing would otherwise overextend a leg. Horizontal interpolation uses smoothstep and vertical lift has zero endpoint slope. Grounded arms counter-swing from the actual foot offsets. The existing two-bone solver lowers the pelvis as necessary rather than stretching limbs or lifting a planted ankle; extreme/malformed targets are bounded before solving. Both boot meshes use the same contact heading. Actual movement, attack facing, damage and saves are unchanged.

Foot contact decisions use 240 Hz ticks with actor-path interpolation. This fixes a reproduced 144-Hz step-order difference from variable substeps; swing samples can trail the current time by at most 4.17 ms. Contacts release in the air and on death. Landing, displacement above 0.85 units in one update, reset/load/respawn and sparring restart establish fresh anchors. Rendering only consumes local targets and cannot advance the step. The system assumes the existing flat floor: individual feet do not raycast stairs, navigate around props or adapt to slopes. Authored clips, skeletal deformation and more detailed foot rolls remain future work.

The controller occupies 152 fixed bytes per actor; the animation sample adds 36 bytes of local foot data. There are no new meshes, textures, shaders, GPU buffers, effect passes or per-frame heap allocations. State vectors resize with actor-count/reset handling. The extra work is CPU contact scheduling, interpolation and pose solving; normal standing still uses grounded contacts, while portrait/static-NPC poses retain their existing presentation path.

`FootPlacementTests` brings the total to fourteen suites. Debug and Release pass all suites, 132 ordinary smoke captures, 28 effect diagnostics and twelve foot-placement diagnostic PNGs. Final logs are `foot-placement-validation.log` in each Ninja build directory and contain no compiler warnings/errors. Tests cover support drift in straight/curved walking at 1, 4.4 and 7 units/second, 30/60/144-Hz agreement, stopping without endless shuffling, turns, uneven-step reversals, jumping/landing, death, teleports, invalid data, limb lengths, floor/weapon clearance, unchanged durable state and repeat-render stability. Live enemy and combat views exercise the simulation-to-pose path. Inspected close-ups and live views show grounded support, knee bending and attached equipment. Manual checks 81–83 remain necessary for live feel and fullscreen acceptance.

Maximum measured support drift is 0.000000477 world units in these fixtures, and the largest equal-time ankle difference across tested update rates is 0.000005723. Test limits remain 0.002 and 0.065 units respectively. These metrics establish the tested trajectories, not perfect foot contact under every possible input sequence.

The first Debug compilation failed with MSVC C1902 before source compilation. CMake now replaces compiler `/Zi` or `/ZI` debug flags with `/Z7` for Debug/RelWithDebInfo so compiler records live in object files. The linker still creates `RPGNative.pdb`; the installed toolchain and system configuration are untouched. Debug and Release were rebuilt successfully after the change; debugger interaction itself was not tested.

One before/after Release pair uses the same hidden 1280x720 workload, fixed 1/60-second step, zero presentation interval, 90 warmup and 600 measured frames:

| Workload | CPU median / p95 before → after (ms) | GPU median / p95 before → after (ms) | GPU observations before / after |
| --- | --- | --- | --- |
| Stationary HUD | 0.473 / 0.575 → 0.468 / 0.562 | 0.350 / 0.367 → 0.333 / 0.357 | 596 / 597 |
| Walking/orbit | 0.478 / 0.583 → 0.435 / 0.536 | 0.293 / 0.310 → 0.293 / 0.313 | 594 / 600 |
| Combat/orbit | 0.479 / 0.598 → 0.458 / 0.540 | 0.304 / 0.319 → 0.305 / 0.319 | 600 / 600 |
| Merchant/character/bag | 0.900 / 1.070 → 0.892 / 1.059 | 0.509 / 0.544 → 0.510 / 0.549 | 600 / 600 |

Peak submissions/triangles remain 58/296,828, 58/301,078, 57/309,582 and 68/285,640. Combat has 265 effect frames of 600 and 534 peak effect vertices, unchanged; other workloads submit no effects. Walking distance remains 31.908 units, with 270 windup frames and six received hits. Fixed mesh/material-terrain-shadow/interface storage remains 224,320/9,699,312/1,529,172 bytes. End working set/private bytes changes from 60.871/89.012 to 61.031/90.879 MiB. These short measurements include run variation and are not a speedup claim, fullscreen FPS estimate or long-session memory bound. Reports: `foot-placement-baseline-benchmark.txt` and `foot-placement-final-benchmark.txt` in the Release build directory. No assets were downloaded or player saves written.
