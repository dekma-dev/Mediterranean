# Memory lifetime and shadow-cache follow-up — 2026-11-26

Scope: reduce unnecessary engine work and investigate resource retention without
changing models, faces, rendering quality, class rules, sound content, input or
simulation timing. This extends `gpu-work-budget-2026-11-26.md`.

## Changes

- Static far-shadow validation now compares canonical mesh/world-transform keys
  in place. The immutable, unskinned `surface.w == 0` path of `VSShadow` uses
  only geometry, its world transform and the light projection. Tint, normals
  and material color do not affect that depth pass. Dynamic/deformed casters
  still bypass the cache. Key changes invalidate depth before updating storage,
  including frames that subsequently fail. Removal, camera-selected LOD and
  projection changes retain exact invalidation, without probabilistic hashes.
- Removed the second key vector and full-instance copies on every frame. Each
  key is 68 bytes instead of two 164-byte records. The measured workload ends
  with 163,064 bytes of CPU key capacity; a stable frame updates zero keys.
  The existing 4 MiB GPU depth cache remains. No LOD thresholds, texture sizes,
  shadow-map sizes, shaders, effects or draw ordering were reduced.
- `Audio::Initialize` now rejects a second initialization attempt before any COM
  or XAudio2 operation. Previously re-entry overwrote owning raw engine/voice
  pointers and the COM ownership flag. This is a lifecycle defect, not evidence
  that ordinary playback repeatedly initialized audio.
- Rebuilding `FontAtlas` now publishes a new SRV transactionally, releasing its
  previous texture. Previously passing an already populated `ComPtr` through
  `GetAddressOf()` lost the old reference. A rejected rebuild preserves the
  existing atlas. Normal renderer initialization already creates a fresh owner;
  this closes the reusable component's reinitialization leak.

## Native lifetime checks

`MemoryLifetimeTests` runs in an isolated hidden window and never writes player
saves. Native audio is muted. It exercises six renderer/window generations,
24 class loads and resizes, 72 font-atlas replacements, invalid font rebuilds,
six audio generations, rejected duplicate audio initialization, and a 2,880-frame
session covering movement, jumps, panels and actual Warrior/Mage/Scout/Priest
attacks, preparation events and native audio delivery.

The test attaches reference-counted private-data tokens to the DX11 device and
large resolution-dependent/scene textures. Tokens do not retain their owner.
Old resolution textures must die after resize; all tracked resources and the
device must die after renderer destruction. This works without the optional
DirectX SDK debug layer. Decoder staging must return to zero live bytes/packets.

Private commit, working set, process handles, GDI/USER objects and Debug CRT
normal blocks/bytes are sampled. Warmed session checks have a fixed 24 MiB
private-commit envelope, 2 MiB live-CRT envelope and eight-handle allowance for
lazy audio/COM/driver workers. These are regression limits, not measured leaks.
Reconstruction instead uses exact resource ownership and CRT checks: allocator
and driver high-water retention can make post-destruction private commit vary
by more than 100 MiB without retaining a device or a texture. No working-set
trimming, forced heap collection or OS configuration changes are used.

## Separately reproduced hardware-path handle retention

The local RTX 4060 hardware swap-chain path retains two process handles for
each create/destroy cycle. The diagnostic `--isolate-platform` reproduces this
using `D3D11CreateDeviceAndSwapChain` alone, without any engine assets, shaders,
game state, source voices or completion events. It also occurs with no Present
calls, and with explicit factory window-association detach, ClearState, Flush
and message pumping. The private-data device tokens are released.

Window-only, hardware-device-only, WIC decoding and WARP swap-chain loops do not
show that per-cycle handle increase. This localizes the symptom to the hardware
DXGI path on this PC; it does **not** identify a particular driver/overlay DLL
or establish a universal DXGI bug. The engine's existing recovery-rate bound
remains in force. Normal frames and class loads do not recreate a swap chain.
This external retention is documented, not claimed fixed or hidden by the test.

API reference: [Microsoft window-association contract](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-makewindowassociation)
and [deferred destruction / Flush](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-flush).

## Same-build rendering comparison

Release, local RTX 4060, 1920×1080, uncapped hidden diagnostic, 90 warmup + 600
measured frames per scene. Runs are sequential, without concurrent builds or
other native tests. Artifacts: `build-ninja-release/memory-20261126/`.

| Scene | Process CPU ms/frame, uncached / cached | GPU median ms, uncached / cached |
| --- | ---: | ---: |
| Stationary HUD | 4.818 / 2.969 | 5.228 / 3.856 |
| Walking/orbit | 5.443 / 5.130 | 5.812 / 5.727 |
| Combat/orbit | 5.651 / 4.297 | 5.243 / 4.793 |
| Merchant/character/bag | 5.339 / 4.193 | 5.820 / 4.446 |

CPU is process user+kernel time divided by frame count, not CPU utilization.
This pair no longer shows the previous moving-camera CPU regression. It is one
local pair, not a general FPS/temperature/power guarantee. Walking GPU p95 is
7.573 / 7.690 ms, so improvement is not uniform at every percentile. Dynamic
camera LOD still legitimately rebuilds the cache. No interactive latency or
thermal-equilibrium measurement is claimed.

All four final Full-HD images are byte-identical to both the same-build uncached
reference and the pre-cache baseline (`image-parity.json`). `ShadowCacheTests`
also verifies unchanged native frames across camera/LOD changes, portraits,
resize, moving dynamic casters, static movement/removal, color-only updates
and device reconstruction. The ordinary 60/30 Hz presentation policy is
unchanged by this follow-up.

## Reproduction

From a VS developer terminal after building the targets:

```powershell
ctest --test-dir build-ninja-release -R '^(MemoryLifetimeTests|ShadowCacheTests|AudioFrameTests|GraphicsLifecycleTests|TexturePreparationTests)$' --output-on-failure -j 1
.\build-ninja-release\MemoryLifetimeTests.exe --soak
.\build-ninja-release\MemoryLifetimeTests.exe --isolate-platform
.\build-ninja-release\RPGNative.exe --benchmark-suite-1080p-uncached build-ninja-release\memory-reference.txt
.\build-ninja-release\RPGNative.exe --benchmark-suite-1080p build-ninja-release\memory-cached.txt
```

`--soak` advances 18,000 rendered frames / 300 seconds of simulation, with
100 alternating movement/combat batches and repeated four-class loads. It is
accelerated, not five minutes of wall-clock interactive play or a multi-hour
leak certification. A driver may keep committed heaps after objects die;
large Task Manager memory numbers alone do not prove a leak.

## Final validation

- Five selected suites pass in Release (32.88 s) and Debug (93.27 s): memory
  lifetime, shadow cache, audio publication, graphics lifecycle and texture
  preparation. Logs: `build-ninja-release/memory-release-final.log` and
  `build-local/memory-debug-tests.log`.
- In Debug, after warming all four classes, equivalent session endpoints have
  53,936 live CRT normal blocks / 146,685,615 bytes. Intermediate movement/combat
  batches vary by at most one block / 33 bytes, returning to the same endpoint.
  Destroyed renderer generations 1–5 have exactly 51,027 blocks / 133,957,958
  bytes each; destroyed audio generations 1–5 have exactly 52,321 blocks /
  134,447,468 bytes each. These totals include deliberately retained immutable
  CPU asset/catalog caches; they are not leaked allocations. Release has no
  Debug CRT instrumentation; its zero CRT columns mean unavailable, not zero RAM.
- The extended Release session completes 18,000 frames / 300 simulated seconds,
  163 accepted visual events, 1,712 preparing-cast frames and 476 native audio
  submissions. After the first eight batches, private commit is 797.723–797.875
  MiB, ending at 797.863 MiB (+0.141 MiB). Working set changes from 636.430 to
  636.602 MiB. Native handles take two plateau values, 458 and 462; GDI/USER
  objects stay at 9/3. An initial two-handle allowance falsely rejected that
  one-time late increase; the final fixed eight-handle envelope passes. No
  continuing RAM/CRT growth was demonstrated in the tested scenarios, which
  does not exclude leaks outside their coverage or in a multi-hour session.
- Both rebuilt public `RPGNative.exe` files pass the sealed 90-command replay
  without a mismatch. The Release public smoke test passes; all 26 PNGs match
  the pre-optimization baseline byte-for-byte, in addition to the four Full-HD
  benchmark images. Reports, raw memory samples and parity hashes are under
  `build-ninja-release/memory-20261126/`.

Most implementation changes were included in the user's `4fb1992` / `0.0.98`
commit while validation was still in progress. The final diagnostic tolerance,
this report and documentation updates follow that checkpoint. No commit, push,
download or system/driver-setting modification was performed by the agent.
