# Spell radiance and major impacts — 2026-11-11

## Reference comparison

This follow-up builds on the earlier local-volume implementation. Revisited the supplied videos in `D:\assets\learning_material`: 60 decoded samples at 1/30-second intervals over 26–28 s of `pve_raid.mp4`, and 45 samples over 8.5–10 s of `pve_loc2.mp4`. Inspected consecutive contact sheets and full-size frames. The raid sequence has narrow bright beam cores with a wider colored halo; the PvE sequence separates an intense contact flash from an expanding, longer-lived body. This is a targeted motion review, not an exhaustive viewing or a claim of exact game-wide parity.

Our previous local volumes moved internally, but their emission stopped abruptly at the rendered contour. Several large area contacts still arranged repeated flat flame/ice pictures, and the Priest's returning connection used separately faded segments. Those visible gaps define this pass.

## Native rendering and choreography

- Added an effects-only floating-point glare path. Visible combat geometry is replayed into a reduced emission target, filtered horizontally/vertically, then composited before the HUD. Smoke and dust do not emit. Sky, environment materials, character materials and interface artwork are not used as glare sources. A bounded hue-preserving response keeps the sharp original core and colored halo separate.
- The reduced pass samples the original opaque depth. Analytic geometry performs explicit visibility checks; local media retain their interior depth integration. Creation effects use their current viewport, including clipping the halo at its boundary. The emission target is cleared for each effects flush, preventing old casts/other portraits from leaving a glow.
- Allocation is lazy, at most 960×540 per target, with three RGBA16F targets: at most 12,441,600 bytes. There are three filter/composite draws plus one replay per nonempty alpha/additive effects batch (four or five extra draws per effects flush). Replay triangles are counted in renderer statistics without consuming the geometry submission pool a second time. No extra per-spell textures, downloads or model edits are introduced.
- Meteor and sun-storm contact now launch animated fire volumes and continuous curling tongues, followed by a torn, terrain-following pressure front and smoke. The front rises, rolls and collapses during the existing accepted contact lifetime. The previous rings of copied flame images are removed from those contact bodies.
- Frost bloom and thermal rupture use closed four-sided spatial crystals in their major contact, retaining the actual attack radius and original gameplay. Thermal's hot half uses local fire density. Priest Sanctuary descends through separate light volumes; Absolution's returning connection has continuous outer/core ribbons and traveling accents instead of per-segment end masks.

No damage, resource, cast, cooldown, save, face, mesh, animation-source or audio definition is changed. The shared 6,144 effect-vertex / 96 alpha-quad / 24-volume budgets remain in place. Important warnings still enter the existing geometry pool first. This remains bounded cosmetic rendering, not a fluid simulation or unlimited raid rendering.

## Moving evidence and verification

`SpellMotionReview <output-directory> --major` records eight real commands at 1280×720 / 60 fps, four seconds each: fire, heavy fire, frost bloom, meteor, sun storm, thermal rupture, Absolution and Sanctuary. The diagnostic hero has the required quest-derived level. Signature reserve is earned by accepted opening attacks before recording. The stationary target starts each recorded sequence at its actual maximum HP; the movie's health bar therefore shows the real damage. No player profiles are written, and no synthetic release/contact events are injected. Movies are silent.

The comparison driver is compiled once against the saved pre-pass client/renderer layout and once against the current client. Local outputs are `build-vfx-radiance/movie-before/` and `movie-after/`, including H.264 movies, release/contact CSVs and selected PNGs. Reference frames, build logs and final checks also live under `build-vfx-radiance/`.

New native glare assertions cover an actual halo outside the sharp core, disabled/empty-frame behavior, nonemissive smoke, complete opaque occlusion, unchanged opaque HUD pixels, creation viewport isolation, odd/ultrawide resize and the allocation/draw ceilings. The earlier analytic ring test still verifies a transparent geometry center with glare disabled; separate checks cover the new optical spread. The one-batch geometry assertion now subtracts the explicitly counted glare passes rather than confusing post-processing with geometry batches. Contact/crowd/lifetime/frame-rate assertions remain unchanged.

Eight targeted native suites pass in both configurations: CharacterSelection, CombatEffects, ClassEffects, VfxMaterials, SpellVolume, SustainedEffects, CombatRelease and CastingEffects. Release takes 138.29 s and Debug takes 238.62 s. Strict shader compilation accepts the new entry points. Ordinary Ninja and Visual Studio Debug/Release game and launcher targets rebuild. In this host, Visual Studio projects were built in dependency order with project-reference traversal disabled, including explicit shader regeneration before the client; this is not a successful whole-solution traversal claim. Existing interface local-shadowing compiler warnings remain.

All four ordinary game executables (`build-ninja-debug`, `build-ninja-release`, `build-local/Debug`, `build-local/Release`) pass hidden fullscreen restoration, device/shader creation, resize, world/UI rendering and PNG capture using the existing veteran fixture. Exit codes, executable timestamps and captures are recorded in `build-vfx-radiance/smoke-results.json`. Inspected the Visual Studio Debug capture.

Both 32-second 720p60 movies completed without DirectX validation warnings. All 16 release/contact CSV rows match exactly, including relative frame and movie timestamp. Decoded the finished movie again: inspected 25 consecutive samples of meteor expansion/decay, 25 of Sanctuary descent and 10 of the returning Absolution connection, plus matching before/after meteor frames at 14.3 s. Orientation, progressive expansion, decay and continuous connections are visible in the encoded output. Early contact PNGs alone do not show the full expanding body; the movie does.

The fixed-world 1920×1080 diagnostic on the local RTX 4060 used 60 warm-up and 180 measured frames per phase, with no concurrent review capture or game smoke process. This compares the saved pre-pass client against the final client; it does not measure interactive gameplay FPS or prove dense-raid performance.

| Six contacts + six aftermath patches | Before | After |
| --- | ---: | ---: |
| CPU p50 / p95, ms (180 samples each) | 6.424 / 7.149 | 6.818 / 7.653 |
| GPU p50 / p95, ms | 6.487 / 6.918 | 6.783 / 7.532 |
| Available asynchronous GPU samples | 23 | 167 |
| Peak effect vertices | 3,234 | 3,390 |
| Peak total draw calls | 230 | 239 |

The world-only GPU medians were 5.725 / 5.736 ms, with only 12 / 11 asynchronous observations. The sparse, unequal GPU sample counts limit inference; these are single diagnostic runs, not a stable FPS guarantee or evidence of an optimization. The new path intentionally spends additional rendering work for radiance. Detailed output is in `workload-before/workload.txt` and `workload/workload.txt` under `build-vfx-radiance/`. The 6,144-vertex ceiling remains unchanged; three bounded glow targets cost at most 11.87 MiB. The validated source snapshot records SHA-256 for 17 build/implementation/test files; all hashes matched at final review.

Remaining work includes more authored sustained-area staging, four-class moving combat at normal zoom, and encounter-scale readability/performance acceptance. The rest of the game's scene and actor presentation still differs from the recordings. Full reference parity is not established by this pass.
