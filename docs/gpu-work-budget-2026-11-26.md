# GPU workload and presentation pacing

The starting point is the owned-HUD engine at `df8f19c`. The user's report was
GPU temperature reaching 60 degrees during play. Temperature alone does not
identify which render work is responsible; this pass measures the engine's
work and does not change cooling, power, driver or system settings.

## Causes and changes

Interactive presentation previously used `Present(1, 0)` without an independent
frame ceiling. VSync follows display refresh; it is not a fixed 60 FPS budget.
The existing inactive loop already stopped drawing outside the foreground.
The Full HD baseline submitted up to 10.9--12.1 million triangles per frame,
including the world and both shadow layers. Unchanged scenery was rerendered
into the whole-island shadow map every frame.

- Gameplay now has a 60 Hz presentation budget, and character selection and
  creation have a 30 Hz budget. Ordinary game panels retain the gameplay rate.
  VSync remains enabled. The native wait uses a process-owned waitable timer
  and wakes for messages; it never spins or changes global timer resolution.
  Input cannot bypass the deadline. The existing wall-time simulation clock
  samples after waiting; this does not slow game time or change the 1/120 s
  simulation integration ceiling.
  Long stalls drop missed presentation slots. Focus/modal/recovery boundaries
  reset deadlines and input baselines. Background suspension is performed once
  on entry rather than rebuilding snapshots for every background message.
- The far shadow layer now caches only immutable, unskinned scenery with no
  vertex deformation. It compares the complete instance values, mesh/LOD and
  light projection exactly, without hash collisions. Comparison order is
  independent of the color frustum; camera orbit alone cannot reorder the key.
  Changes/removal, scene replacement and selected LOD changes regenerate depth.
  Each frame copies this static depth into the live far layer, then draws all
  dynamic casters over it. The near shadow layer stays fully live. Color
  geometry, material quality, shadow resolution and source models are unchanged.
  Portraits bypass this cache; resize and device reconstruction are covered.
  Additional GPU storage is one 1024-square D32 depth layer: 4 MiB. Reused CPU
  keys/batches remain bounded by the existing 8,192-instance submission limit.
- Workload reports now count far-cache reuse/rebuilds and mean submitted versus
  avoided shadow triangles. Uncapped benchmarks intentionally bypass interactive
  pacing, so before/after GPU work can be compared without a limiting sleep.

The first measured cache version rebuilt on almost every orbit frame because
it compared color-frustum bucket order. That unnecessary invalidation was
corrected to compare canonical mesh/source order before final acceptance.

API contracts: [DXGI Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present),
[waitable timer creation](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw),
and [message-aware waits](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex).

## Acceptance

Release measurements use the local RTX 4060, 1920x1080 hidden buffers,
interval-zero presentation, and the existing four deterministic workloads:
90 warmup plus 600 measured frames each. Builds and other native tests were
not run concurrently with these measurements. Reports and captures are under
`build-ninja-release/power-20261126/`.

`before.txt` is the original engine. `after.txt` retains the initial excessive
invalidation evidence; `final.txt` records its correction. The final same-build
A/B pair is `reference-current.txt` (cache disabled) and
`optimized-current.txt` (cache enabled), run sequentially. It can be reproduced
with `RPGNative.exe --benchmark-suite-1080p-uncached <report>` and
`RPGNative.exe --benchmark-suite-1080p <report>`. These diagnostic modes are
intentionally uncapped and are not the normal play loop.

| Workload | GPU median ms, uncached / cached | Mean submitted triangles, uncached / cached | Cache hits / rebuilds |
| --- | ---: | ---: | ---: |
| Stationary HUD | 5.233 / 3.882 | 10,875,407 / 5,643,977 | 600 / 0 |
| Walking/orbit | 5.910 / 5.689 | 11,221,700 / 9,962,387 | 136 / 464 |
| Combat/orbit | 5.283 / 4.798 | 10,566,569 / 8,504,786 | 214 / 386 |
| Merchant/character/bag | 5.822 / 4.418 | 11,447,360 / 6,204,486 | 600 / 0 |

GPU medians fall 25.8%, 3.7%, 9.2% and 24.1% respectively in this paired
run. Moving-camera LOD changes still require cache rebuilds: this is not a
claim of eliminating the entire far pass during movement. Resource counters
increase by exactly 4 MiB for the static depth cache; original mesh buffers
remain 144,064,052 bytes. Full-HD image comparisons are byte-for-byte; no
resolution, mesh quality, visibility distance, shader quality or artwork
reduction accounts for the savings.

CPU cost is not uniformly improved. Same-build summed process CPU per measured
frame is 4.167 / 2.370 ms (idle), 4.635 / 5.260 ms (walking), 4.193 / 5.208 ms
(combat) and 5.885 / 4.193 ms (overlapping panels), uncached / cached. CPU and
driver scheduling vary between runs; the extra ownership comparisons and
frequent LOD rebuilds remain work. Do not describe this as a general CPU speedup.
Presentation limits reduce submissions per real second in ordinary play even
when an uncapped benchmark submits faster; they do not change per-frame
triangle counts or promise a particular utilization/temperature percentage.

`FramePacingTests` covers 30/60 Hz deadlines, frequent-message polling, stalls,
rate changes, focus reset, clock reversal and preserved elapsed game time.
Native waits record 72 frames in about 1.2 seconds at 60 Hz and 36 at 30 Hz;
a posted message wakes the timer without being consumed. `ExecutionTests`
also runs the actual inactive engine with 600 queued messages: no GPU frames
are submitted and the HUD is cancelled/published only once.

`ShadowCacheTests` compares full native cached and uncached images across six
camera/LOD/panel/resize states, and verifies live dynamic casters, moved and
removed static geometry, and device reconstruction. Original `ShadowTests`,
`GraphicsLifecycleTests`, `RenderBindingsTests` and `GpuFrameQueueTests` cover
contacts, offscreen/moving shadows, clear/portrait isolation, resource hazards
and device/event lifetimes. All seven suites pass on the final Release build
(52.07 s) and final Visual Studio Debug build (103.91 s). Logs are
`power-20261126/acceptance-final.log` in the corresponding build directory;
final builds are recorded in `build-ab.log`. The native D3D debug layer is
unavailable on this machine; passing explicit image/lifecycle/binding checks
is not a driver-debug-layer-clean claim.

Both ordinary public executables are rebuilt. Release smoke returns 0 and all
26 images match the preceding baseline (`smoke-comparison.json`). All four
Full-HD original/uncached/cached captures also match byte-for-byte
(`image-comparison.json`). The final Debug public executable passes the
90-command sealed replay with first mismatch 0; its report is
`build-local/power-20261126/replay.txt`. `git diff --check` passes.

Interactive temperature after thermal equilibrium, power consumption and
input-to-photon latency have not been measured. Hidden diagnostic runs cannot
establish those values; no temperature or wattage reduction is promised.
No model, face, class rule, asset download, system setting, commit or push is
part of this change.
