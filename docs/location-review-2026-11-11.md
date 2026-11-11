# Location composition and daylight — 2026-11-11

## Reference evidence

Reviewed `D:\assets\learning_material\loc1.jpg`, `loc2.avif` (locally decoded through WIC), `map1.jpg`, `map2.jpg` and `map3.jpg`. Sampled the opening 30 seconds of `pve_loc.mp4` at 2.5-second intervals and the available opening section of `pve_loc2.mp4` at two-second intervals using the existing native Media Foundation reader. Inspected full-size frames, including the wooden fort/earthen approach and the densely planted coastal verge. This is a location-focused sample review, not a claim to have watched every frame of the complete recordings.

The useful environmental relationships are: enclosed forest edges against open roads; banks and layered slopes rather than an uninterrupted lawn; lived-in yards; low ground vegetation between larger plants; warm sun against cool shade; readable paths and combat ground. The local reference frames and their timestamps are retained under `build-location-review/pve-loc*`. No reference media or external asset was downloaded or copied into the shipped game.

## Implemented slice

- Six compact banks break up the original flat village outskirts. Existing buildings, market, training ground and deck retain level foundations/aprons. The shared heightfield supplies rendering, navigation footing, camera, feet and map projection; both triangles of every cell remain below the existing grade limit.
- A deterministic, bounded location pass places 160 additional trees in distributed groves. The existing oak, pine and dead-tree exports are reused. Instance scaling preserves their proportions within ten percent, and shared export metadata fits camera envelopes to their rendered crowns.
- Authored carts, crates, barrels, wells, benches, firewood, sacks, fences, lanterns and stumps form roadside yards. Low buried stone shelves replace the previous evenly spaced bright road-border dots. Large new details have versioned collision; export-bound checks include cart handles, well roofs and every LOD.
- Up to 1,700 cover patches, including 360 shrubs/ferns, prioritize village verges and tree understory. Roads, NPCs, training areas and encounter clearings remain open. Offset tufts sample their own ground height instead of floating at the central patch height.
- The existing 256-square ground data texture now stores canopy/litter coverage in its alpha channel. Terrain and architectural shelter remain bounded in blue. Ground shading distinguishes forest litter, worn roads and exposed bank strata without additional texture allocation. The old rock threshold was unreachable on the actual traversable grades.
- Lower directional daylight, reduced general haze and a slightly revised foliage response improve depth and the separation of sunlight/shade. The visible sun, shadow projections and shading share one direction. Existing head/scalp/face/expression shader branches are untouched.
- Four nearby practical lanterns have restrained flame/glow and low-power surface illumination. They share the existing bounded light selection with combat, use no new shadow maps and remain outside the immutable scene cache.
- Tree LOD selection considers the cost of tall dense crowns, avoiding promotion of every medium-distance tree to its dense near mesh. All existing authored mesh records are preserved.

## Compatibility and regression scope

Save version 24 retains the version-23 record layout. Older heroes, encounter enemies and wilderness actors covered by a newly introduced footprint relocate to a nearby clear point. Current-version positions inside geometry, old wall intersections and invalid coordinates still fail atomically. Quest, inventory, level and combat values are preserved. A widened tree crown also exposed an upward camera-boom trap near the training ground; shoulder-level focus can now leave the canopy while trunk/wall collision remains active.

`LocationTests` checks all NPC routes, physical road clearance, both terrain triangles, placement/cover budgets, old-save recovery/current-save rejection, woodland weights and authored collision/camera bounds. Seven fixed native cameras cover the village, village side, north road, western woods, farm, mist ridge and province. Hidden-window benchmark samples exclude image-readback frames. Native capture timings describe these isolated views, not combat FPS or a representative full-game benchmark.

The focused acceptance set also includes navigation, simulation, population, highlands, southern campaign, interface and shadows. Shadow regression probes follow the configured sun rather than assuming the earlier sun coordinates. Existing contrast and transition tolerances remain unchanged.

## Native acceptance record

Release: **8/8 passed**, 73.89 seconds. Debug: **8/8 passed**, 218.93 seconds. The settled layout has **160 additional trees, 47 solid details, 1,700 cover patches and 360 shrubs/ferns**. The retained scene grows from 5,298 instances / 953,640 bytes to 5,619 / 1,011,420 bytes (about 56 KiB more). No new texture allocation or authored archive growth was required.

Fixed-camera results at 1280x720 on the NVIDIA RTX 4060 (8 GiB):

| View | Previous triangles | Current triangles | Current median GPU ms |
| --- | ---: | ---: | ---: |
| Village | 5,417,701 | 6,117,553 | 4.96 |
| Village west | 4,382,411 | 5,587,014 | 4.96 |
| North road | 3,001,450 | 5,679,944 | 4.85 |
| Western wood | 4,669,463 | 5,790,122 | 5.01 |
| Farm | 4,946,520 | 4,218,248 | 3.10 |
| Mist ridge | 3,584,558 | 3,335,602 | 5.68 |
| Province | 2,713,269 | 3,333,434 | 2.87 |

Triangles include color and shadow passes. The previous-core capture host uses Debug libraries while current metrics above use Release; CPU timing/FPS comparisons between those hosts are not valid. The first dense-crown attempt reached 8.7–13.2 million triangles in near views. The retained-proportion and LOD revision reduces that cost without editing the source models. The current north-road view is deliberately more expensive because it now contains a forest edge.

Local full-size artifacts: `build-location-review/before-final/*.png`, `build-location-review/after-final/*.png`, their `metrics.txt` reports, `tests-final-release.log`, `tests-final-debug.log` and the validated source hashes in `validation-sources.json`. The initial `before/` diagnostic omitted terrain-weight upload; it is superseded by `before-final/`, which includes the actual paths and shelter mask. Use the final pair for comparisons.

Game-host smoke validation: **6/6 passed**, with exit code 0, using `tests/fixtures/veteran-preview.save`. This covers `build-local/Debug`, `build-local/Release`, `build-ninja-debug`, `build-ninja-release`, `build-learning-review/debug` and `build-learning-review/release`. Each launched the real game, initialized its device/shaders, restored hidden fullscreen, resized, rendered the world and UI, and wrote a PNG. The Visual Studio Debug image was also inspected: the shared terrain/forest changes are present alongside the live HUD and character. Diagnostic runs do not modify player saves. Results and images are in `build-location-review/smoke-results.json` and `smoke-*.png`.

The private Ninja Debug/Release builds and public Ninja builds completed. Shared Visual Studio libraries/executables were refreshed concurrently and passed the game-host checks above; this pass's additional MSBuild recheck stopped progressing at `Checking File Globs` and was cancelled. Do not interpret that cancelled attempt as a successful full Visual Studio rebuild. The validated environment-source hashes were checked again after the smoke runs. The normal local Debug launch path is `build-local/Debug/RPGNative.exe`.

## Remaining visual work

This is a denser playable environment slice, not complete reference parity. The reference still has more distinctive architecture, authored landforms, ground materials and vegetation silhouettes. Some outer slopes are sparse; regional dressing and road material transitions need further art review. Models and facial source/export data were not edited in this pass. Review the changed location at ordinary camera height and with a live encounter, not only from elevated diagnostic cameras.
