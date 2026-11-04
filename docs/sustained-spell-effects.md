# Sustained spell states and prioritized danger contours

## Reference observations

Revisited the local training frame at 02:00 in `allods_fight_interface.mp4`
and the existing consecutive dungeon sheet at 03:00.015-03:00.500. The former
separates a ground ornament, persistent defensive shapes and a short hit;
the latter shows a tall repeated charge with a much smaller player aura.
These are targeted observations of the supplied extracts, not a claim of a
new exhaustive video review or identical mechanics.

Blizzard's [combat animation article](https://worldofwarcraft.blizzard.com/en-us/news/20946579/patch-7-3-new-combat-animations-it-s-magic)
describes preserving class identity and distinct buildup/release moments.
The historical official [Allods 11.1 notes](https://allods.ru/news.php?article=9356)
also distinguish frost attacks and defensive ice magic. These sources inform
visual separation; this revision does not import their skills, balance or art.

## Changes

- Mage roots now use icy ankle crystals and cold vapor, replacing the generic
  green vine effect. Scout controls use crossed bindings; nature controls keep
  climbing strands. Stuns orbit at humanoid or creature height. Exposed targets
  have a small pulsing cut mark. Practice dummies now participate in this path.
- Defensive effects use angled panels around the actual body, with a physical
  forward guard for Warrior/Engineer, frost facets for Mage and school-colored
  magical surfaces for other wards. Player-facing panels follow rendered facing.
  Scout evasion uses short moving wind bands. Absorbed hits add a material flash;
  Mage absorption also sheds small ice fragments.
- Frost bloom retains its authoritative three-metre area while gaining drifting
  low vapor, ice shards, an inner seal and gradual onset/expiry. The outer ring
  samples terrain and shares the radius constant with simulation. It cannot
  expand the slowing area or survive its actual timer.
- Hostile sector/disc/annular borders are submitted before scenery and cosmetic
  effects. Threatening and sooner-resolving casts take precedence, with stable
  identity tie breaks. Boss/wraith charge layers stay attached to the caster;
  inner danger washes grow brighter with the actual windup. Uninterruptible
  attacks keep a violet treatment. Interrupting a cast removes its warning
  immediately. Shapes use the existing reach, half-angle and shock-band values.

## Lifecycle and budgets

`SustainedEffects` is a bounded presentation cache updated by Game, not by draw
calls. It stores eight nearby controlled actors plus two player channels and
fits below 4 KiB. Selection takes priority when controls exceed the actor budget.
Normal onset is 120 ms and removal fades for up to 180 ms; the authoritative
status itself is never extended. Death, returning home, disappearance and reset
clear the affected actor. Identity includes ID, kind, rank and home; a teleport
or reused identity starts a fresh envelope. Roster reordering preserves it.

The live renderer uses this cache; the old stateless `classfx::States` entry
remains a preview/test adapter. Existing load/reset/recovery paths clear the
cache. Rendering cannot write health, resources, timers, targeting or save data.

At most eight visible nearby hostile casts receive warning geometry. Eight
quake-plus-cone outlines need 3,168 vertices; optional warning details stop below
3,600. One or two warnings instead use 64-segment circles and 32-segment sectors
to avoid visible corners at normal play distance; crowded scenes use fewer
segments while retaining the same combat boundaries. The same global
6,144-vertex effect ceiling still applies. Status detail
also has a local cap. Existing material tiles are reused: no additional texture,
external dependency, Blender export, authored-clip edit or face change is needed.

Control school currently follows the single player who applies it. A future MMO
must retain per-status caster/school attribution rather than infer it from the
local player. Cosmetic fading may briefly outlast a status, but danger contours
never have that trailing fade. Dense crowds and repeated boss pulls still need
manual readability acceptance at the normal play camera.

## Validation and evidence

`SustainedEffectsTests` checks smooth onset/removal, invalid time, fixed storage,
roster reorder, identity reuse, teleport/death, selected-target priority and
practice-dummy controls. Native captures check 30/60/144 Hz onset, render
idempotence, status expiry, creature heights, eight simultaneous warnings,
cosmetic saturation, cancellation, actual defensive casts/frost fields and all
three boss attack patterns. Existing class, contact, material and southern
encounter suites remain part of the focused regression run.

Captures are under each build's `sustained-effects-review`; build/test/startup
logs are under `build-sustained-effects`. The original reference frames and
source art remain unchanged. This extends sustained combat presentation; it
does not claim final encounter balance or complete reference parity.

Final affected Release suites passed 4/4; the earlier material regression also
passed. SustainedEffectsTests passed in Visual Studio Debug with native captures.
Both normal game/launcher targets build, and both actual game executables passed
isolated hidden smoke launches (device/shaders, fullscreen restoration, resize,
world/UI capture). Localization audit passed at 2,047 source occurrences and
1,895 Russian entries. No commit or push was made.
