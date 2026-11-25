# Engine lifecycle and shader contracts — 2026-11-25

## Scope

The active direction is engine internals. World content, character models,
class expansion, interface artwork and effect design are deferred. This pass
changes graphics ownership, failure handling and the shader build contract.
Reference footage remains the visual acceptance target; this change is meant
to preserve the existing image while making execution safer.

## Frame and device ownership

`Renderer::BeginFrame` now reports success. The engine checks it before any
game/loading/benchmark rendering. Uninitialized, nested or failed frames cannot
submit GPU passes. Size changes are accepted only between frames; minimized
zero-size notifications retain existing targets, and invalid or oversized
dimensions are rejected before releasing resources. Target/depth views are
published together after successful creation. Portrait scopes must be balanced.

The first failed HRESULT and `GetDeviceRemovedReason` are retained. Begin,
resize, submission and present failures feed the same engine recovery path.
Removed/reset/hung/internal-driver failures may reconstruct the renderer;
invalid calls, ordinary allocation failures and other errors are not retried.
There are at most two reconstruction attempts in any rolling minute. A failed
reconstruction or repeated immediate faults end with the original diagnostic
plus the reconstruction/limit detail, rather than an endless restart loop.

The old renderer unbinds context references, submits outstanding work and drains
its owned completion events before releasing its resources. A fresh renderer
then owns a new device, swap chain, buffers, shaders and texture views. The
existing `Game`, simulation, CPU scene caches, camera and audio object survive.
Terrain/map textures are uploaded lazily to the new device. Input gestures are
cancelled, audio is suspended during reconstruction, and the frame clock is
resynchronized so rebuilding time cannot become a simulation catch-up step.
Recovery start/success diagnostics go to the debugger output.

CPU submission limits remain separate from device faults: a completed,
discarded palette-overflow frame can be followed by a valid frame. Its query
scope is ended and its submitted work is accounted for by the GPU queue.

## Shader ABI

`engine/ShaderData.h` owns shared upload structures and constant-buffer slots.
The renderer and build tool use the same definitions. After compilation and
before publishing bytecode, D3D reflection validates every surviving constant
buffer against actual C++ `sizeof`/`offsetof` values. Checks cover total size,
field count, field offsets/sizes, float vector dimensions, column-major matrix
packing, array counts, nested struct members and register bindings.

Coverage spans all 19 shader entry points and all eight constant buffers:
Scene, Hands, CloudField, CombatLights, SpellVolumes, SpellGlow, SpellFields and
PressureScene. Unused buffers may be optimized out of an individual stage;
the regression checks aggregate coverage across all stages. Unknown constant
buffers fail validation. The game still loads embedded bytecode; reflection
and shader compilation add no frame-time work. Texture bindings, structured
skin palettes and vertex signatures are not covered by this contract yet.

## Validation

`GraphicsLifecycleTests` creates real native DX11 renderers and enters the
production recovery method with synthetic removal/reset HRESULTs. It checks
invalid frame/portrait/resize sequences, bounded retries, same-session
reconstruction, camera/save preservation, cleared input, clock reset and fresh
lazy resources. Before/after PNGs must match byte-for-byte across two renderer
generations, including a failure during an unfinished frame.

`ShaderContractTests` compiles all current stages and deliberately mutates
field order, matrix packing, array lengths, nested members and a register.
Each mismatch must fail with the relevant contract diagnostic. Existing native
rendering suites additionally exercise shadows, hands, body palettes, pressure,
volume passes, UI, GPU completion events and storage/accounting helpers.

The broader run caught a lifecycle regression in `SkinningTests`: a harmless
palette overflow must allow a subsequent valid frame. The final implementation
closes discarded-frame query/event scopes and preserves this behavior while
keeping device/protocol HRESULTs sticky.
Discarded portrait scopes also retain the original submission error, restore
CPU camera state and permit a later valid frame; a native regression exercises
this separately from invalid nested-frame/portrait usage.

`HandSkinningTests` initially reported equal lit/blocked intensity (81.2345).
An isolated build of the original `2cb0339` reproduced the same failure. Seven
unchanged hand captures were byte-identical between that build and the new
renderer. Its historical fixed occluder missed the probe's current sun ray;
the fixture now positions the same box using the shared sun direction. The
original contrast threshold stays unchanged; measured lit/blocked values are
81.2345 / 35.6667. No hand geometry, materials, light direction or shadow
algorithm was changed to satisfy this test. The baseline references the same
unchanged asset files in place instead of copying the Blender collection.

Actual Windows driver resets, adapter removal and extended visible play are
not induced by the automated tests. No TDR/system settings are modified, and
no player's save is used.

Build/test evidence (ignored local artifacts):

- `build-ninja-release/engine-acceptance-build.log` and `engine-acceptance-tests.log`.
- `build-local/engine-final-build.log`, `engine-final-tests.log`,
  `engine-hand-build.log`, `engine-hand-tests.log`, `engine-scope-build.log`
  and `engine-scope-tests.log`.
- `build-ninja-release/engine-baseline/tests.log`: original-commit reproduction.
- Both build directories' `graphics-lifecycle-review/`: inspected before/after
  world, minimap and character-portrait captures; recovery frames match exactly.

Final Release acceptance: **11/11 suites pass in 81.61 seconds**. Visual Studio
Debug: the broader run passed ten suites; the corrected hand fixture then
passed. Following the final portrait-unwind change, GraphicsLifecycleTests,
HandSkinningTests and SkinningTests were rebuilt and passed together in 46.31
seconds. All eleven selected suites therefore pass in that configuration too.
Both public `RPGNative.exe` builds are rebuilt. The isolated baseline's 515.6
MiB of compiler outputs were removed after comparison; its source, logs and
captures remain available.
This is focused engine acceptance, not a claim that every historical project
test or every interactive gameplay scenario has been validated.

## Remaining engine work

1. Exercise real driver-loss/display-change scenarios in a controlled manual
   session; verify audio/camera/input continuity in visible play.
2. Completed in the follow-up: vertex inputs, texture/sampler slots, structured
   skin-palette layout and native binding ownership. See
   `render-bindings-2026-11-25.md` for the implementation and validation.
3. Give simulation/presentation scheduling explicit time ownership and bounded
   command/event queues; measure stalls before changing update semantics.
4. Add asynchronous resource preparation with explicit render-thread upload
   ownership and cancellation before introducing general background jobs.

Implementation references: [Microsoft device-loss handling](https://learn.microsoft.com/en-us/windows/uwp/gaming/handling-device-lost-scenarios)
and [D3DReflect](https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dreflect).
