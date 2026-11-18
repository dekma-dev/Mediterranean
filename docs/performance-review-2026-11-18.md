# Render memory and CPU scheduling review — 2026-11-18

The working baseline was clean commit `3f43c6e`. This pass reduces redundant
render storage and CPU work while waiting for the GPU. Models, frozen faces,
source assets, gameplay, visibility distances, LOD policy and VSync settings
are unchanged. No downloads, system settings, commits or pushes were made.

## Changes

- Pack skinned meshes before static geometry in the shared vertex buffer.
  Only the skinned prefix has joint indices/weights. Static terrain, buildings,
  trees and other props no longer allocate unused 32-byte influences per
  vertex. Global draw indices still address the same mesh, and draw-time
  vertex-buffer binding does not increase. Authored geometry is read by
  reference during assembly instead of making a temporary full copy.
- Keep the roof/fur color and normal maps at their original 1024-square size
  in separate texture arrays. Stone, wood, plaster and bark retain their
  original 2048-square size. The former uniform array enlarged the two 1K
  sources to 2K without adding detail. All layers still have full mip chains.
- Upload only the used prefix of the structured actor palette buffer, including
  each portrait's actual owners, instead of the entire 32-actor capacity.
- Limit render-ahead to two GPU submissions using native DXGI completion
  events. `BeginFrame` sleeps on the oldest pending event, while the GPU
  continues processing. No query polling, fixed sleep/FPS cap or timer
  resolution change is introduced. Event handles remain alive until their
  submitted work completes; DXGI also signals them on device removal.
  Missing interface/event allocation retains the previous driver scheduling.
- Correct an obsolete benchmark assertion: noncombat ambient/footstep effects
  are valid now. Walking distance and combat activity are still required.
  Add summed process user/kernel CPU time, event-wait duration, and memory
  samples per scene. Existing CPU phase distributions measure wall time and
  include waits; they must not be interpreted as CPU utilization.

GPU completion is not presentation completion. This queue does not measure
input-to-photon latency or replace VSync/display pacing. The native API
contract is documented in
[Microsoft's EnqueueSetEvent reference](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgidevice2-enqueuesetevent).

## Matched measurements

Release, NVIDIA GeForce RTX 4060 (8 GB), driver 32.0.16.1088. Hidden 1920x1080
render buffers, interval-zero presentation, four existing deterministic
workloads, 90 warmup plus 600 measured frames each. The same source baseline
received only the benchmark assertion/reporting fixes before measurement.
No builds or other game tests ran concurrently with these measurements.

All local reports and captures are in
`build-ninja-release/performance-20261118/`:

- `before-cpu.txt`: baseline with the corrected diagnostic counters.
- `after-storage.txt`: compact weights/native-size surfaces, before queue and
  palette-upload changes.
- `after-final.txt` and `after-repeat.txt`: optimized runs. The repeat includes
  final event lifetime handling and report labels.
- `comparison.jpg`: four before/after views. Each report also has four
  `.scene-N.png` captures.

| Workload | Process CPU ms/frame before | After, two runs | Frame wall p50 before / final repeat | GPU p50 before / final repeat |
| --- | ---: | ---: | ---: | ---: |
| Idle HUD | 5.573 | 1.875 / 1.719 | 5.277 / 5.312 | 5.261 / 5.235 |
| Walking/orbit | 6.484 | 1.797 / 2.240 | 6.040 / 6.025 | 5.911 / 6.002 |
| Combat/orbit | 6.146 | 1.953 / 1.901 | 4.922 / 5.340 | 5.835 / 5.403 |
| Merchant/equipment/bag | 6.458 | 2.214 / 1.797 | 6.203 / 5.936 | 6.254 / 5.825 |

Process CPU cost fell approximately 65–72% in these uncapped workloads. Final
repeat event waits averaged 3.817–4.524 ms/frame. Frame wall time and GPU cost
remain broadly similar; combat wall p50 increased by 0.418 ms while its p95
fell from 7.562 to 7.386 ms. This is reduced CPU use, not a claim of a matching
FPS increase. Baseline long-frame outliers are not evidence of a fixed hitch.

| End-of-suite resource | Before | Final repeat | Reduction |
| --- | ---: | ---: | ---: |
| Process working set | 905.910 MiB | 616.621 MiB | 289.289 MiB (31.9%) |
| Process private allocation | 1237.812 MiB | 830.141 MiB | 407.671 MiB (32.9%) |
| Immutable GPU mesh buffers | 198,876,372 bytes | 144,064,052 bytes | 54,812,320 bytes |
| GPU material/shadow resources | 279,112,316 bytes | 212,003,452 bytes | 67,108,864 bytes |
| GPU interface resources | 30,408,592 bytes | 30,408,592 bytes | unchanged |

Mesh/material allocations alone save 116.273 MiB. These resource counts,
working set and private allocation are different measurements and **must not
be added together**. The intermediate storage-only run ended at 857.500 /
1069.828 MiB working/private; the completed queue/palette changes account for
the further measured reduction. Driver allocations and residency vary.

Both optimized runs retain exactly the baseline peak submission/triangle
counts: 246/10,878,350; 281/12,073,225; 262/11,434,927; 279/11,450,301. Walking
still covers 32.777 m; combat still records 270 windup frames and six received
hits. Static instance storage stays at 5,619 entries / 1,011,420 CPU bytes.

## Image and regression checks

Both public game executables were rebuilt: Ninja Release and Visual Studio
Debug. Eight focused suites pass in both configurations: `GpuFrameQueueTests`,
`SurfaceStorageTests`, `BenchmarkStatsTests`, `AuthoredAssetTests`,
`CharacterSelectionTests`, `HandTests`, `SkinningTests` and
`PressureEffectsTests`. Native Debug checks cover binding warnings, portrait
ownership, resizes and pressure resources. The new queue fixture verifies
180 ordered GPU submissions, readback after draining, event reuse, producer
backpressure, reinitialization and unavailable-device fallback. Surface tests
verify unique manifest coverage and native-size mip storage. Build/test logs
are retained in the review directory; `git diff --check` passes.

The walking and combat final images are pixel-identical to the baseline.
Idle and merchant views differ by only 0.029870 and 0.015893 mean absolute RGB
levels out of 255 respectively; no pixel differs by more than 16 levels.
The remaining differences are consistent with removal of the redundant
roof-map resampling. The four paired views were inspected. All four captures
are identical between the two optimized runs.

Two pre-existing test failures were reproduced against the pre-optimization
native Debug client library, using `HEAD` test sources and its matching
`Renderer.h` before rebuilding that library:

- `FaceTests`: cheek light/shadow is 119.276 / 111.107, below the historical
  required ten-level difference. The old and optimized renderers produce
  identical values, and all eleven face captures are pixel-identical. No
  facial or lighting change, threshold reduction or test bypass was made.
- `GeometryTests`: the legacy shared-mesh category totals 761,096 bytes /
  20,178 triangles against its historical 640 KiB / 16,000 cap. Its existing
  cloak alone contributes 407,632 bytes / 12,736 triangles. This pass does
  not modify generated mesh topology. The original guard remains unchanged;
  a future geometry-budget audit must account for the current assets.

Reproductions are saved in `baseline-tests/` and `baseline-geometry-costs.log`
inside the review directory. These failures must not be reported as passing.
New native queue and surface-storage regressions are separate suites, so
their validation does not depend on these old failures.

## Reproduce and inspect

After rebuilding Release from the project root:

```powershell
.\build-ninja-release\RPGNative.exe --benchmark-suite-1080p build-ninja-release/performance-check.txt
```

For ordinary play/performance checks use `build-ninja-release/RPGNative.exe`.
The process in Task Manager is **RPGNative.exe**, not RPGLauncher.exe or
Visual Studio. Check its executable path when several builds exist. Compare
the same resolution, camera route and graphics settings; Debug and running
under the debugger are not comparable to optimized Release measurements.

These hidden benchmarks omit live input and audio processing. They are not
an hour-long leak test, actual Task Manager CPU percentages, visible-window
FPS or evidence of an equivalent reduction at VSync-limited refresh rates.
Next acceptance: sustained visible gameplay with audio, repeated roster/world
transitions, camera motion and focus/display changes, tracking stabilized
working/private memory and CPU while keeping the scene comparable.
