# Cast preparation and attached recovery effects

## Reference and scope

The 2026-11-05 pass revisited the existing decoded training frame at 02:00
(`build-video-review/full/s120.jpg`) and dungeon motion sheet around 03:00.600
(`build-video-review/dungeon-motion-review-01.jpg`). The visible distinction is
between energy at the hands/weapon, a separate ground ornament and raised
defensive or spell layers. This was a targeted reference check, not another
complete frame-by-frame review of both recordings.

Blizzard's [combat animation article](https://news.blizzard.com/en-gb/article/20946579/patch-7-3-new-combat-animations-its-magic)
describes readable casting stages while preserving responsiveness. The official
Allods [wizard](https://allods.ru/about.php?show=class-wizard) and
[priest](https://allods.ru/about.php?show=class-priest) descriptions reinforce
elemental and restorative class identities. Existing original atlas materials
are reused; no external art was downloaded or copied.

## Visible changes

- Mage preparation converges into the actual spell focus: flame fragments,
  crystalline spokes, arcane fragments or opposing thermal materials. Larger
  spells add a subdued terrain seal and two broken rising bands. The effect
  ends at the accepted release; movement cancellation cannot launch it.
- Scout heavy aiming uses small converging sight marks. Strong shots also
  draw short air streaks into the weapon focus. Ordinary melee swings have no
  casting halo; physical weapons do not charge a fireball.
- Recovery channels use both solved hands. Priest/Paladin gather gold light,
  Druid lifts leaf-like fragments, Bard builds staggered wavefronts, and
  Engineer has small repair sparks and a hand-to-hand filament.
- Completion is now a short, class-specific pulse attached to the current
  character: Warrior/Engineer guard facets and falling sparks, Scout close-body
  wraps, Mage ice plates and vapor, Priest/Paladin fan-shaped golden flashes,
  Druid rising strands/leaves, Bard successive waves, Psionic opposing lenses,
  and Necromancer/Demonologist contracting soul/pact streams. These are visual
  distinctions; classes retain their existing heal, barrier and resource rules.
- Completion and potion effects follow movement instead of remaining at their
  old emission point. Death suppresses them. Loading resets them. Equipment
  presence and shield/bow grip are taken from the same outfit as the drawing.
- Timed recovery completion carries its remaining fraction of the simulation
  frame into the visual event. Previously the pulse started at age zero even
  when the cast had completed partway through that frame.

## Implementation and limits

`game/CastEffects.cpp` is a stateless layer over accepted action progress,
immutable events and borrowed world-space pose sockets. Rendering does not
advance animations or gameplay. Only active preparation/recovery requires the
additional solved pose; idle frames retain the cheap root-only context.
Ground seals sample terrain at their corners rather than the airborne root's
height. Very uneven ground can still expose the limits of a four-corner surface.

Preparation uses fewer than 512 vertices per tested effect, and completion
fewer than 900. The existing global 6,144 effect-vertex and 96 alpha-sprite
ceilings remain. These effects add no particle container, dynamic light, atlas,
shader, model export or save field. They reuse the existing burst slots, fade
within 0.8 seconds and yield to the renderer's established limits. This is not
a new authored animation library or a measured frame-rate improvement.

No face, head, eye, hair, facial material or Blender source/export was changed
by this pass. Concurrent body-animation/art work in the shared checkout is
preserved separately.

## Verification and evidence

`CastingEffectsTests` runs the native DirectX renderer and actual simulation.
It covers all eleven class completions, moving socket/camera equivalence,
equipped/unequipped hand frames, invalid/dead sockets, cancellation, reset/load
isolation, repeated-render equality, and cast completion timestamps at
30/60/144 Hz. Mage/Scout preparation is compared at equal elapsed times across
those refresh rates. Actual-world captures include recovery channels, pulses,
walking during the pulse and two camera angles for strong offensive casts.

Captures are under each build's `casting-effects-review`; logs and the previous
recovery captures are under `build-cast-presentation`. The inspected world
images show distinct Priest gold fans, Warrior hand protection and Mage ice
plates while moving. The cast focus is intentionally smaller than the body
pulse, preserving the view of the target and hostile warnings.

Release regression initially passed all five affected suites: casting, general
combat effects, class effects, combat HUD and class combat. A subsequent grip
check used an invalid enum spelling in its fixture; it was corrected to obtain
the form from `WeaponFor(Warrior)`. The final rerun passed all five Release
suites and both Debug suites (casting and general combat effects). Both game
and launcher targets build. Both actual game EXEs pass isolated fullscreen,
device/shader, resize, world/UI and PNG-capture smoke checks. Localization and
scoped whitespace checks pass; see `progress.md` and the logs for details.

Manual acceptance remains: full-speed prolonged encounters, hills/steps,
close/side/far cameras and dense simultaneous effects. Native screenshots and
tests do not establish equivalence to the reference game's final art or motion.
