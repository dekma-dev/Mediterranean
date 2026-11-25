# Bounded texture preparation — 2026-11-25

## Runtime change

Renderer initialization previously decoded twelve surface images and seven
interface/effect atlases synchronously, alternating WIC work with Direct3D
uploads. Device reconstruction repeated the same work on the window/render
thread. Preparation and GPU ownership are now separate stages.

`TexturePreparation` starts one CPU worker before device/pipeline construction.
It prepares the existing embedded resources in upload order. Surface source
dimensions, RGBA conversion, interface scaling, destination sizes, layer order
and GPU mip generation retain their previous behavior. No asset or model is
modified. Procedural material generation, geometry construction, font
rasterization, audio loading and lazy map/terrain uploads remain on their
existing paths.

The worker owns its COM apartment, WIC factory, streams and converters. The
consumer receives only a manifest request and an exclusively owned pixel
buffer. Direct3D creation, `UpdateSubresource`, `GenerateMips`, context binding
and presentation stay on the original renderer thread. The consumer verifies
request identity, destination, layer, dimensions and byte count before upload.

## Ownership, bounds and failures

- The default budget is **32 MiB and two packets**, whichever binds first.
  Reservations happen before allocation and cover decoding, queued pixels and
  packets already held by the consumer. Dequeue does not release the budget.
  Packet destruction frees storage before waking the producer. Move assignment
  releases replaced storage, and a retained packet can outlive the producer.
- The bound covers application-owned prepared RGBA buffers. It is not a bound
  on total process RAM, GPU memory, WIC internal scratch or embedded compressed
  resources. The old serial path held one image at a time; overlap may add a
  second image, up to another 16 MiB. This is not presented as a RAM reduction.
- Manifest size and dimensions are checked before starting a worker. Embedded
  encoded images are limited to 16 MiB; source dimensions are validated before
  pixel conversion. Missing resources, bad dimensions, codec failures and
  worker exceptions produce an HRESULT and resource ID. Partial pixels are
  never published as a successful packet.
- A condition variable provides backpressure and wakes blocked consumers and
  producers on cancellation. There is no detached thread or unbounded work
  queue. The consumer is single-threaded; foreign consumers are rejected.
- A preparation batch belongs to one `Renderer::Initialize` stack. All exits
  cancel, join and drain it before returning, including GPU initialization
  failure and native close. Reconstruction creates a new batch. There is no
  shared completion queue through which old-device work could reach the next
  renderer, so this path does not need a global generation-token registry.
- Cancellation is cooperative between decode operations. WIC pixel conversion
  already in progress must return before the worker can join. This is not hard
  real-time cancellation, streaming during gameplay or a general job scheduler.

## Native loading behavior

Visible engine startup and device reconstruction supply a bounded native
message pump. Renderer initialization calls it between mesh/material work and
while waiting for prepared images (16 ms timed waits). Closing the window or
receiving `WM_QUIT` cancels preparation and preserves the exit code. Normal
startup cancellation exits without an error dialog; cancellation during
recovery also exits without showing a false renderer error.

Individual mesh generation, driver calls and native modal handlers cannot be
preempted. The later world/audio startup stages are not made asynchronous by
this change. Hidden diagnostic hosts retain deterministic, pump-free loading
unless they explicitly provide a callback.

Adapter/benchmark diagnostics include prepared/delivered counts, peak pixel
bytes and packet count, decode time and producer/consumer wait time. These are
loading measurements, not FPS measurements.

## Validation

`TexturePreparationTests` exercises FIFO delivery, both byte and packet bounds,
consumer-held reservations, move replacement, packets surviving producer
destruction, thirty blocked-queue shutdowns, in-flight cross-thread cancellation,
worker exceptions, missing/non-image resources, dimension rejection and invalid
manifests. The native decoder is compared byte-for-byte with an independent
synchronous WIC reference for all **19 images / 174,653,440 RGBA bytes**. The
reference runs in an STA while production decoding owns a separate MTA.

Native tests interrupt renderer initialization, create a replacement, render a
frame and test the actual engine loading pump with `WM_CLOSE` plus a posted
message backlog and an explicit nonzero `WM_QUIT`. `GraphicsLifecycleTests`
also verifies preparation counts and zero retained staging allocations after
both synthetic device-loss recoveries, alongside unchanged session/camera and
identical restored frame captures. No OS driver reset, system setting change,
physical input injection or player-save write is used.

Final native acceptance:

| Configuration | Selected suites | Result |
| --- | --- | --- |
| Ninja Release | TexturePreparation, Execution, RenderBindings, GraphicsLifecycle, ShaderContract | 5/5, 31.29 s |
| Visual Studio Debug | Same suites | 5/5, 45.23 s |

The selected suite names above end in `Tests`. Both public `RPGNative.exe`
outputs are rebuilt. The ordinary Release smoke executable exits successfully;
all **26 PNG captures** (world, portraits, panels and multiple sizes) match the
pre-change executable by SHA-256. The Debug graphics tests also verify exact
before/after reconstruction frames. The D3D debug layer is unavailable on this
machine, so a successful Debug configuration is not a debug-layer-clean claim.
Visible long-session interaction remains manual acceptance.

Evidence retained in ignored build directories:

- `build-ninja-release/texture-final-build.log`, `texture-final-tests.log`.
- `build-local/texture-final-build.log`, `texture-final-tests.log`.
- `build-ninja-release/texture-before/` and `texture-after/` smoke captures;
  `build-ninja-release/texture-capture-comparison.json` records every SHA-256.
- Both build directories' `Testing/Temporary/LastTest.log` contain native pixel
  parity and loading counters. Final Release production initialization decoded
  in 0.657 s, with 0.175 s producer wait and 0.488 s consumer wait; these are one
  diagnostic sample, not a before/after startup benchmark.

`git diff --check` passes. No model, face, texture asset, save, shader behavior,
system setting or Git history is modified by this pass. Earlier uncommitted
engine work remains present.

## References and next boundary

Microsoft documents that the immediate context must have a single caller at a
time and should share its thread with DXGI presentation:
[Direct3D 11 multithreading](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-intro).
COM initialization and uninitialization are per-thread:
[Initializing COM](https://learn.microsoft.com/en-us/windows/win32/learnwin32/initializing-the-com-library).

This concrete resource-ownership boundary was implemented before the broader
simulation/presentation split because it can be isolated and verified without
changing combat timing. Replayable simulation commands/events and measured
presentation separation remain next. Streaming mesh/audio/world preparation
will need explicit dependency, priority and lifetime policies of its own.
