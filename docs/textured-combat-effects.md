# Layered combat materials and soft particles

## Reference and scope

Revisited the user's extracted `allods_fight_interface.mp4` frames at 02:00
and 02:02, the consecutive 02:01.000-02:01.483 melee sheet, and the dungeon
sequence at 03:00.015-03:00.500. The reference separates a persistent ground
ornament, an attached weapon/aura treatment, a brief dense hit core and a
different decay layer. Our previous contacts relied predominantly on thin
analytic ribbons and streaks. These are targeted observations from existing
decoded material, not a new exhaustive review or measurements of its engine.

Blizzard's historical [combat animation article](https://news.blizzard.com/en-gb/article/20946579/patch-7-3-new-combat-animations-its-magic)
discusses distinct class/spec presentation and preparation/release with
responsive gameplay. Microsoft's [DirectXTK state documentation](https://github.com/microsoft/DirectXTK/wiki/CommonStates)
and [depth/stencil documentation](https://learn.microsoft.com/en-gb/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil)
inform the native compositing implementation. No external engine, extracted
game assets or downloaded dependencies are used.

## Material and rendering changes

Eight original RGBA materials are embedded in every native client host:
flame, smoke, dust, ice, arcane vortex, slash, sparks and an ornamental seal.
The built-in imagegen output and exact prompt are retained in
`assets/effects/combat-vfx.png` and `assets/effects/README.md`. The source PNG
is unchanged. A single 1024 x 512 texture with mipmaps costs about 2.67 MiB;
loading decodes it once. UV insets and cell-edge fade isolate atlas cells.

Opaque world geometry still writes depth first. Flame/smoke/dust quads are sorted
back to front and blended with straight alpha, then luminous particles use
the existing additive pass. A read-only DSV and SRV share the existing depth
texture. Particle pixels fade over the last 0.24 m (clouds) or 0.065 m (other
materials) before an opaque surface, preventing rectangular intersection cuts.
The depth SRV is unbound before restoring writable depth, including resize.
Portraits cannot submit particles. Particles approaching the camera fade out.

The existing global ceiling remains 6,144 submitted effect vertices per frame,
including analytic effects, ground marks and textured sprites. At most 96 alpha
sprites use pre-reserved storage; surplus cosmetics are discarded. Texture
layers do not add simulation particles, lights, per-frame texture uploads or
new random state. The alpha batch adds at most one draw per flush. Alpha-cloud
sorting is per quad; this is not order-independent transparency or volumetric
fluid simulation. Additive layers remain a separate compositing pass.

## Visible combat changes

- Mage fire preparation grows at the existing rendered hand socket. Fire
  projectiles carry layered flame wisps; impacts separate flash, rising flame
  bodies and delayed smoke. Frost uses faceted shards and cold vapor, arcane
  uses rotating vortex/seal layers, and Thermal rupture combines fire and ice.
  Heavy/chain contacts leave a short terrain-conforming seal.
- Warrior contacts gain a broad tapered slash surface, a brief irregular spark
  core and delayed ground dust on heavy attacks. Reverse cuts and crossed
  finishers retain their distinct orientation and number of cuts.
- Scout shots retain physical shafts/fletching. Heavy shots add a restrained
  slipstream; contacts use compact fragments and drifting impact dust. Piercing
  and crossing attacks add a narrow cutting afterimage, not a fire explosion.
- Other class contacts retain their existing individual geometry with an added
  holy-seal or arcane surface layer. This is less extensive than the three main
  class revisions and does not replace their animation libraries.
- Boar charges and enemy quake/tempest contacts lift bounded dust along their
  actual paths/edges; lance/execution contacts retain their accepted reach.
  Existing ground danger contours remain separate.

All layers evaluate accepted event age and identity. Damage, cooldowns, class
resources, targeting, input and save formats are unchanged by this pass.
Contact residue lasts 0.66-0.88 seconds depending on class/strength. Preparation
disappears on release/cancellation; miss paths cannot create impact residue.
The old fixed 0.48-second emitter cutoff also truncated slow projectiles before
arrival: projectile lifetime now includes its accepted flight plus decay.
Reset/load clears transient effects; rendering does not advance them.

## Evidence and acceptance

`VfxMaterialsTests` exercises native atlas materials, emission-order-independent
alpha sorting for unequal depths, opaque occlusion, soft intersections,
resize/portrait isolation, saturation budgets, slow-projectile lifetime,
30/60/144 Hz contact equality, expiry and real Warrior/Mage/Scout world attacks.
`CombatEffectsTests`, `ClassEffectsTests`, `SpellChainsTests` and `CombatHudTests`
retain checks for authoritative events, cancellation, hit/miss, shader hue,
depth, existing controls and preparation budgets.

The five selected Release suites passed across the main run and the corrected
CombatEffectsTests rerun; the two selected Debug suites passed. The old test
assumptions of a 0.48-second contact lifetime were updated for the intentional
residue; expiry, pool reuse and reset assertions remain enforced. The native
world fixture was corrected to use a valid legacy campaign reward ledger.
Localization audit passed 2,047 source occurrences / 1,895 Russian entries.
Normal Release and Visual Studio Debug game/launcher targets were rebuilt.
Both actual RPGNative executables passed isolated smoke launches covering
device/shader initialization, hidden fullscreen restoration, resize and
world/UI capture. Use `build-ninja-release/MediterraneanLauncher.exe` for play
or `build-local/Debug/RPGNative.exe` for debugging this revision.

An optional diagnostic can be run from the Release build directory:

```powershell
.\VfxMaterialsTests.exe ..\build-vfx-refinement\performance --workload
```

It compares the same frozen 1920x1080 world with zero and six simultaneous mixed
contacts after 60 warm-up frames, then records 180 CPU submissions and available
asynchronous GPU observations per phase. On the local RTX 4060 the mixed phase
peaked at 3,264 effect vertices and 226 draw calls (world-only: 225). The first
run produced only 9/20 GPU observations; timings are therefore diagnostic data,
not a reliable GPU percentile comparison or an FPS/performance-improvement claim.
See `build-vfx-refinement/performance/workload.txt` for the unedited measurements.

Before captures are under `build-vfx-refinement/before`. Updated material/world
views are in each build's `vfx-material-review` and `class-effects-check`;
build/test/smoke logs are in `build-vfx-refinement`. Remaining acceptance includes
long boss pulls, multiple simultaneous casters, readability at several camera
distances and manual GPU profiling on the player's hardware. This is a visible
VFX revision, not complete reference parity, new Blender animation or final
encounter balance. Source models, frozen faces and authored clips are preserved.
