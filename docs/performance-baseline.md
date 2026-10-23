# Performance baseline and clock audit

Checkpoint: 2026-10-23. This completes the reporting portion of roadmap R1, not the entire visual/performance acceptance milestone.

## Run

From the project root:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -BenchmarkSuite
```

The script rebuilds and writes `build-ninja-release/Release-benchmark-suite.txt`. For an already rebuilt executable:

```powershell
.\build-ninja-release\RPGNative.exe --benchmark-suite build-ninja-release/baseline.txt
```

The suite runs authored in-memory snapshots without writing player saves. Each of four workloads has 90 warmup and 600 measured frames: idle HUD, walking/orbit, repeated combat/orbit and overlapping merchant/character/bag panels. Setup and snapshot resets are outside measured frames. Movement distance, enemy windups, received hits and effect vertices validate that the intended workload occurred.

Reports now include actual render dimensions, adapter identity, driver version when available, dedicated VRAM, feature level, a suite compilation stamp/configuration, memory and submission counts. Record the checkout revision and local changes alongside a report: the compilation stamp is not a source hash. CPU phase and GPU observation distributions use nearest-rank p50/p95/p99/max, with counts above 1000/60 ms. These counts are observations, not a display-frame drop counter. Older reports used a different percentile convention.

No GPU observations prints `timing unavailable`, rather than a fictitious zero cost. Fewer than 100 samples carries a warning about coarse tail resolution. GPU timestamps are asynchronous, sampled without waiting. Report version 3 retains every completed query in each batch and filters warmup/previous-workload results by their originating frame. The final outstanding queries are not drained or waited on, so GPU samples are still not matched one-to-one with CPU frames. Do not add CPU and GPU percentiles together.

Earlier report versions kept only one result from each completion batch and admitted delayed warmup results. The historical measurements below predate that correction; regenerate reports with version 3 before making GPU comparisons. The latest-value diagnostic also now follows originating frame order rather than query-ring storage order.

## First measurement

Release, hidden 1280x720 window, interval-zero presentation; NVIDIA GeForce RTX 4060, driver 32.0.16.1088. Initial report: `build-ninja-release/r1-baseline.txt`. The run predates the added low-sample warning but uses the same distributions.

| Workload | CPU p50 / p95 / p99 ms | GPU observations | GPU p50 / p95 ms |
| --- | --- | ---: | --- |
| Idle HUD | 0.518 / 0.705 / 0.841 | 60 | 0.591 / 0.686 |
| Walking/orbit | 0.567 / 1.839 / 1.956 | 36 | 0.544 / 1.696 |
| Combat/orbit | 0.564 / 2.126 / 2.245 | 36 | 0.555 / 2.083 |
| Merchant/character/bag | 0.605 / 0.881 / 1.194 | 40 | 0.619 / 0.718 |

All workloads contained 600 CPU measurements. Walking covered 32.777 m; combat produced six received hits, 270 windup frames and effects in all measured frames. End-of-run working set/private allocation: 307.996/350.906 MiB. These are one run's diagnostics, not an optimization comparison or an interactive FPS guarantee. Hidden-window scheduling and sparse GPU observations limit interpretation.

## Clock and input audit

- `Engine::Run` samples the performance counter and passes at most 50 ms to `Tick`. It has no accumulator that repays time beyond this cap; a long foreground stall advances less simulation time than wall time.
- `Game::Update` and `ExecuteCommand` also bound their steps. Horizontal movement uses subdivisions at approximately 120 Hz or finer. This improves integration but does not make the entire simulation fixed-rate.
- Input is sampled before `Tick`; camera and simulation update precede rendering. This is a source-order audit, not measured input-to-photon latency.
- Losing foreground/minimizing suspends input/audio and waits for messages. The timer baseline resets on that path so the inactive interval is not replayed as a large movement step.
- The benchmark bypasses live input and feeds 1/60-second commands. Fast hidden execution is not real-time player input or audio-load coverage.

Do not change the simulation clock merely to make the report say fixed-step. An accumulator migration needs explicit catch-up limits, interpolation, input-edge consumption, action-event uniqueness and regression coverage for suspension/loading. Existing frame-rate behavior should remain the baseline during that work.

## Remaining R1 acceptance

### Full-HD capture checkpoint

The suite now supports an exact 1920x1080 render-buffer mode, independently of hidden-window desktop fitting:

```powershell
.\scripts\build-native.ps1 -Backend Ninja -Configuration Release -BenchmarkSuite -Benchmark1080p
# Or use an already rebuilt executable:
.\build-ninja-release\RPGNative.exe --benchmark-suite-1080p build-ninja-release/fullhd.txt
```

The script writes `Release-benchmark-suite-1080p.txt`, preserving the standard report. Four captures append `.scene-0.png` through `.scene-3.png` to the report filename. They show each workload's final frame; readback and encoding happen after measurement and before the next workload's warmup. Failure to save a requested capture fails the run. This mode stays hidden, writes no player saves and changes no monitor settings. The ordinary suite retains its 1280x720 default.

Validation on the same RTX 4060: CPU p95/p99 were 1.105/1.189 ms (idle), 3.646/4.069 ms (walking), 4.244/4.554 ms (combat), and 1.209/1.372 ms (merchant/equipment). GPU observations numbered only 34–36 per workload, so tail estimates remain coarse. All scenario activity checks passed. The combat and combined merchant/equipment captures were visually inspected at 1920x1080. These images establish a reference, not approval of current character art quality or motion.

Release build, BenchmarkStatsTests, ShadowTests and InterfaceTests passed (22.86 s for tests); both the new mode and legacy suite completed, and a missing report argument returned code 2. The benchmark has no live input or audio processing and still does not establish visible-window FPS.

### Still open

Capture a visible 1080p route with comparable camera/lighting, exercise sustained effect-heavy combat and roster transitions, measure loading/audio health and input response, and retain hardware/source metadata. Obtain denser GPU measurements before using p99 to set budgets. The provisional 60 FPS target has not been established by these hidden tests. No new visuals, animations or movement behavior are claimed by this reporting milestone.
