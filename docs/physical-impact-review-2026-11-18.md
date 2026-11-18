# Physical impact foundation — 2026-11-18

## Scope and reference

This pass develops shared engine/presentation infrastructure for Warrior, Mage,
Scout and Priest. Damage, cast timing, spell selection, class builds, authored
models and faces are unchanged. The larger action bars remain in place.

Reviewed an existing raid sequence sample at 29.5 seconds and extracted 21 frames
at 29.2001–29.5335 seconds from `D:/assets/learning_material/pve_raid_2.mp4`,
inspecting the endpoints. The useful distinction is between the moving stroke,
bright contact, ground footprint and residual motion; actors remain readable
between peaks. These samples are not a complete video analysis or proof of how
the reference game's engine works.

Previously, detached steel debris, arrow fragments, ice shards and embers followed
age-based formulas inside render functions. They could continue below terrain and
through solid props. Their new state belongs to confirmed contact events and
advances independently of rendering and of the shorter contact-flash lifetime.

## Implemented

- `engine/ParticlePhysics.h`: bounded CPU dynamics, 120 Hz fixed steps,
  interpolated presentation, analytic gravity/linear drag between contacts, swept
  expanded-box tests, embedded-spawn ejection, at most two contacts per step,
  friction, restitution, settling and finite-input guards. Long frames are capped
  consistently with the existing presentation update. No unbounded catch-up.
- `game/ImpactParticles.cpp`: immutable 4 m scenery bins built during loading,
  terrain traces using the actual rendered heightfield, slope normals and different
  energy loss on paving and earth. Queries inspect nearby solid prop bounds, not
  the whole map per particle. Moving actors and mesh triangles are not colliders.
- Confirmed hits emit 16 particles; heavy/signature contacts emit 24; secondary
  area victims emit three. Mere releases and evaded hits emit none. Event `elapsed`
  is applied at birth, and the accepted incoming direction is used.
- Steel sparks, directional arrow splinters, warm embers, tumbling crossed ice
  surfaces, arcane fragments and golden holy fragments have different material and
  motion responses. Thermal rupture retains both ice and embers; secondary targets
  retain their spell's element. Settled ice lies along its contact plane. Small
  dust puffs originate at actual ground contacts.
- Removed duplicate analytic debris in class impacts and Mage contacts. Existing
  flame plumes, strike sheets, holy forms and contact flashes remain separate from
  the physical aftermath. This is not a new class design.
- The offline reviewer also captures an aftermath frame 0.8 seconds after contact.

## Limits and measurements

The pool has 96 slots and drops excess new cosmetics without replacing live tails.
It uses 10,760 bytes; complete `CombatEffects` state is 26,896 bytes, with an explicit
32 KiB ceiling replacing the former 16 KiB ceiling. No new textures, render targets,
GPU buffers or lights are allocated for this system. At most 960 vertices per frame
are available to it, inside the existing 6,144 total effect ceiling. Hostile
warnings and main contacts are submitted first.

The Release stress test saturated the pool over 600 updates, registered 1,065
contacts and measured approximately 0.011 ms/update including extra terrain
penetration assertions. This is a local CPU measurement, not total frame time,
a GPU benchmark or a minimum-spec hardware claim.

Collision uses solid prop boxes and terrain. Roof detail, foliage, individual
triangles, characters and dynamic objects are outside this first cosmetic layer.
Smoke, magical ribbons and volumetric plumes retain their existing authored motion.
Full reference fidelity remains unfinished.

## Validation and reproducibility

`RPGNative` builds in Ninja Release and Visual Studio Debug. Five focused suites
pass in both: `ParticlePhysicsTests`, `ImpactParticlesTests`, `CombatEffectsTests`,
`ClassEffectsTests`, `SpellVolumeTests`. Debug completed in 58.08 seconds. Release's
four suites passed in the first final run; the class suite passed in 15.24 seconds
after correcting its old one-second expiry assertion to distinguish flash and
physical-aftermath lifetimes. No production change was needed for that assertion.

Coverage includes ballistic accuracy, collision parity at 22/30/60/144/1000 Hz,
slopes, thin walls, embedded spawns, material response, event tails, capacity,
expiry/reset, invalid clocks, six materials, thermal mixtures and secondary hits.
Native suites cover depth, portrait/HUD isolation, class visibility and GPU budgets.

Local evidence is intentionally excluded from Git:

- `build-impact-physics/reference/`: short source-video sample and timestamps.
- `build-impact-physics/before/`: pre-change four-class movie and event CSV. This
  existing executable predates the latest HUD layout, so HUD pixels differ
  independently of this pass; compare world effects and event timing.
- `build-impact-physics/accepted/`: final 20-second 720p60 moving-combat movie,
  contact/aftermath stills, event CSV and renderer budget CSV.

```powershell
.\build-ninja-release\SpellMotionReview.exe build-impact-physics\accepted --moving
```

The capture uses real gameplay events in an isolated review instance and does not
write character profiles. It is an offline render, not an interactive FPS test.

The final moving sequence has an exact event-CSV match with the baseline for all
release/contact rows. Peak effect vertices are 960 (Warrior), 1,476 (Mage), 1,110
(Scout) and 1,302 (Priest); lights stay at four and spell volumes at eight or below.
Contact and aftermath stills were inspected: the ember and holy fragments outlive
the bright flash while leaving the target and ground pattern visible. The existing
short Warrior flash remains intentionally brief; its secondary sparks are smaller.

The additional `build-impact-physics/crowded/combat-motion.mp4` is a 28-second
four-class encounter with overlapping hostile casts. It passed native DirectX
validation. Peak effect vertices are 2,148 / 3,636 / 2,880 / 3,042 for Warrior,
frost Mage, Scout and Priest respectively; each stays below 6,144 with at most
four lights and three volumes. Frost contact/aftermath and Priest aftermath
captures were inspected with visible cone/annulus boundaries. Full-speed human
acceptance, especially near scenery and at different camera distances, is still
required before treating this as visual parity with the reference.
