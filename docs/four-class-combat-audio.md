# Four-class combat and acoustic revision

Scope: Warrior, Mage, Scout and Priest; 2026-11-05. Existing spell icons,
inventory layouts, saved characters and Blender model sources are preserved.

## Release expression and repeated attacks

The next runtime pass makes ordinary casts more readable as well as the large
signatures. Mage releases ignite at the solved weapon/hand socket for 180 ms,
with an 85 ms bright core. Fire has a larger layered flame head; frost has a
longer core and four orbiting shards; arcane has two rotating lens shapes.
Thermal rupture adds opposing fire/ice lobes and rising steam at the primary
contact. Priest projectiles have paired gold tails; preparation gathers two
streams toward the casting focus, and completed healing opens three continuous
gold/ivory arches behind the shoulders. These use the existing material atlas
and effect buffers. Damage, accepted target positions and attack timings are
unchanged.

Warrior Reverse cut now reverses the imported weapon-arm trajectory around the
same contact key instead of only changing the torso accent. A two-piece Hermite
time mapping preserves contact and settles with zero endpoint velocity; its
negative harmonic tangent prevents overshoot at the short source contact phase.
Lower-body loading, planted feet, cancellation and outgoing-pose blending keep
their original clocks. This is runtime sampling, not a modified Blender action.

Repeated attacks previously cached alternate raw recordings but only one take
of each composite. Fourteen additional composite buffers now give Twin/Fanned/
aimed bow shots and Warrior finishers three takes, and piercing hits, meteor,
Firestorm, Judgment and Sanctuary two. Layers within a composite choose separate
existing takes. Generation happens during audio initialization; playback uses
the cache and the existing small pitch variation.

Warrior protection/absorption now uses short physical strap/metal composites;
Scout evasion uses earth/gear movement; Mage absorption uses ice. Protected enemy
contacts retain their own cue. Barrier contact audio is spatial and emitted once
per accepted hit. No recordings were downloaded in this follow-up.

`build-combat-expression/four-class-repeated-attacks.wav` is a 24-second isolated
audition: Warrior finisher, Scout aimed shot, Mage meteor contact and Priest
Judgment, six seconds per group with three repetitions. It uses runtime take,
pitch and cue-level selection but is not a recording of a full battle or its mix.

Validation for this pass: all six affected suites pass in both Ninja Release
and Visual Studio Debug (AudioTests, SignaturePresentationTests,
CombatReleaseTests, ClassEffectsTests, CastingEffectsTests and
AuthoredLocomotionTests). Tests exercise distinct cached PCM, unchanged memory
ceilings, accepted guard contacts, monotonic reverse sampling, alternating
basic/backhand transitions at 30/60/144/1000 Hz, and native material/lifetime
checks. Both game/launcher configurations build; both RPGNative outputs pass
isolated startup/render smoke launches. Native release, Thermal rupture and
continuous healing-arch captures were inspected. Evidence is under
`build-combat-expression`; full-suite and subjective listening approval are
not claimed.

## Sound

The recorded-audio follow-up replaces the weakest procedural sources with a
licensed CC0 sample bank. [Credits, original links and reproduction](../assets/audio/CREDITS.md)
identify every contributor and transformation. The bank contains 51 variants:
physical bow releases, sampled fire textures, human Warrior efforts, surface
footsteps/landings, and Priest organ/bell preparation, release, contact and healing.
Bow/fire/holy versions are processed variations; efforts and steps use distinct takes.

- Scout keeps draw / physical release / spatial target contact. Aimed shots use
  a lower, heavier recorded bow release; the draw and target impact remain modeled.
  Twin shot now has two separated string transients; Fanned arrows/Crossfire
  have a short spread of releases. Piercing shot adds a lower target impact.
- Mage fire uses a source recording with separate charge/release/contact envelopes.
  Frost and arcane retain the preceding resonance synthesis. Thermal rupture uses
  fire audio, while its contact combines ice/flame materials.
- Priest uses sampled Aeolus organ ranks and a bell with damped reflections.
  This is an instrument emulation, not a live choir recording. Completed healing
  has a separate tail; interrupted casts cannot play the completion sound.
  Heavy offensive spells add a lower organ foundation and a delayed bell/contact
  layer. Strong frost and arcane casts likewise have separate release composites.
- Warrior heavy strikes add one of three short recorded vocal efforts. Sword
  swishes/impacts are preserved. Male efforts are the available current voice set.
- Sand/stone recordings replace grass/dirt/paving foot cues and landings; grass
  is a filtered sand treatment, and timber remains modeled. Movement timing is
  unchanged. Three versions alternate independently of gameplay randomness.
- Preparation voices fade after release/cancellation in approximately 185 ms
  (35 ms exponential time constant). Later casts cannot revive old tails.
- The existing twelve effect voices now prefer warnings and impacts over
  footsteps/preparation when saturated. Equal-priority attacks retain their
  tails; low-priority cues cannot replace an impact. Headroom is still bounded.

Three full orchestral/choral pieces by Cesar da Rocha replace the procedural
scores in the normal packaged game: character menu 53.10 s, exploration 126.13 s,
combat 91.24 s. The pieces are full compositions with cadence fades, not newly
composed game stems. Menu/world and exploration/combat gains crossfade; combat
now replaces the exploration bed rather than stacking old percussion over it.
Focus loss suspends playback and volume controls continue to work. The external
Win32 launcher remains silent; the character-selection screen has menu music.

The music files total 11.91 MiB, stereo 44.1 kHz Microsoft ADPCM, with 256-frame
blocks. XAudio2 decodes the read-only embedded resources on demand; there is no
whole-score PCM allocation, decoder work on the rendering thread, external
codec dependency, or runtime network request. The edited effect bank is 2.57 MiB,
22,050 Hz mono PCM. Runtime effect/ambience PCM, including all variations, is
6,189,330 bytes (about 5.90 MiB), checked against 6 MiB. Missing music resources retain the original generated
fallback scores; normal build tests require all recordings to be embedded.

Native tests validate the encoded format, block boundaries and muted playback,
including end-of-composition looping. The independent libsndfile round trip
measures 37.08 / 38.59 / 41.47 dB signal-to-error ratios and quiet padded endpoints.
These are technical checks, not a claim of listening approval or lossless encoding.

## Visible combat and class opportunities

The signature follow-up restores the advanced silhouettes that the newer
material-contact path had bypassed: Falling star descends from above with a
flame/smoke wake and follows the accepted impact position; Firestorm throws
ten staggered ground plumes inside its 4.5 m area; Frost bloom fractures into
eight rising ice forms across its 3 m area. Terrain waves conform to the
heightfield. The existing flame/ice/smoke textures and bounded effect buffers
are reused. Secondary victims retain local hits, while the accepted primary
victim receives the one main spectacle and its large acoustic impact.

Priest Sanctuary strike opens a gold/ivory light fan and descending shafts;
Absolution sends paired light streams back toward the caster's recorded
contact position. They are cosmetic and do not delay healing or damage.
Fanned arrows now draws five diverging/converging physical arrows instead of
one; the original area targeting and damage remain authoritative.

Six additional audio composites cover meteor descent/contact, Firestorm,
Sanctuary, heavy aimed bow releases and Warrior finishers. They reuse the
licensed bank, have bounded tails and normalized peaks, and require no new
download. Finisher voice effort is integrated once rather than doubled by a
separate grunt event. The new audition is
`build-signature-combat/four-class-signatures.wav`: Warrior crosscut, Scout
piercing shot, Mage meteor, Priest Sanctuary strike, five seconds each.

Per-ability body accents distinguish wide/reverse Warrior cuts and downward
finishes, braced Scout shots and spreads, Mage heavy projection and Priest
prayer. Outgoing and cancelled accents share the attack controller's blend
and recovery instead of switching at the next spell ID. A disconnected
Priest scepter path is repaired: the controller uses the cast contact clock
with a separate one-handed reliquary/free-palm gesture. This is a runtime
procedural adaptation, not a new Blender clip or a two-handed staff grip.
The existing anatomical limb solves, planted feet and frozen faces remain.

Mage attacks have larger flame/arcane heads, thicker material wakes, stronger
hand preparation and larger elemental contacts with short radiating flashes.
Priest attacks use a golden projectile with a bright core, an expanding
seal and a tall contact shaft; heavy contacts now add descending light curtains.
Heavy Mage contacts gain expanding elemental plumes. The generic orange Mage
impact path is removed, so frost/arcane no longer inherit an extra fire burst.
Healing has a wider attached fan. The existing particle atlas and geometry
budgets are retained. A separate four-light pool now illuminates nearby ground,
clothing and armor from preparation, projectile travel, impact and healing.
Fire, frost, arcane and holy light have distinct hues; contacts fade within
0.56 seconds. Selection is bounded and independent of submission order.
This is short-range unshadowed direct lighting: it adds no shadow maps, texture
lookups or draw passes, but does add pixel shading work. Thin-wall light leakage
remains possible; it is not a fully occluded lighting system. Portraits, UI and
frozen face/skin/eye/hair shading are excluded.

Sword and bow attacks add contact-timed load, drive and recovery across chest,
pelvis and body translation, with smooth zero-velocity endpoints. The existing
authored animation controller, grip solves and planted-foot IK remain in use;
gameplay position and frozen facial channels do not change. No source models
or Blender actions were modified by this runtime follow-up.

All four focused classes may queue an offensive technique during the last
220 ms of recovery/cooldown. The single buffered action retains its target and
rechecks range, line of sight, reserve, equipment and movement when executed.
No cooldown/resource is spent while waiting. Mobility, healing and potions
are excluded; target changes, Escape, death and loading discard the buffer.

| Class | Trigger | Result |
| --- | --- | --- |
| Warrior | Absorb a frontal hit within 0.8 s of Iron guard | Five seconds to land Strike/Reverse cut for +50% basic damage and at least 12 ward |
| Mage | Land fire, frost and arcane on one actor, within six seconds between new elements | Eight seconds to commit one normally stationary offensive spell while moving; cast time remains |
| Scout | Disengage at least 1.5 m away from a selected living threat initially within 9 m | Five seconds to land Quick/Twin shot for +45% basic damage and a two-second slow |
| Priest | Complete Greater heal and actually restore missing health | Six seconds to land Radiance/Rebuke for +25% basic damage and at least 18 ward |

Warrior/Scout/Priest spend on accepted primary contact, so misses preserve the
remaining window. Mage spends on validated commitment, including a subsequently
cancelled cast. Repeated elements do not refresh incomplete combinations;
changing targets resets incomplete element memory. Mobile confluence cannot
recharge itself. Load, death and encounter reset clear transient opportunities;
there is no save version change or new permanent spell ID.

A class status badge explains the rule, displays readiness/element progress
and highlights eligible action slots. Ready/spent effects follow the actual
hands, weapon or accepted target. New player prose has Russian translations.

## Reference basis and review

The local training capture at `build-video-review/full/s120.jpg` was revisited
for the relationship between status badges, hand energy and terrain effects.
This was a targeted check, not another complete frame-by-frame video/audio review.
The follow-up also revisited `build-video-review/pass1-c-01.jpg` (01:34-01:39)
for the training-view relationship between actor silhouette, contact and UI.
Official class references include the [Mage overview](https://worldofwarcraft.blizzard.com/en-gb/game/classes/mage),
the [Warrior overview](https://worldofwarcraft.blizzard.com/en-us/game/classes/warrior)
and [Allods Scout](https://allods.ru/about.php?show=class-scout).
Physical synthesis references include [Karplus-Strong](https://theory.stanford.edu/~blynn/sound/karplusstrong.html)
and [CMU sound synthesis](https://www.cs.cmu.edu/~15322/book/ch12/01.html).
Runtime effects are original code plus the attributed CC0 recordings above;
no reference-game audio was extracted.

`AudioTests` exports short auditions from real accepted combat event sequences:
`warrior-heavy.wav`, `scout-aimed.wav`, `mage-fire.wav`, `mage-frost.wav`,
`mage-arcane.wav`, `mage-bloom.wav`, `scout-twin.wav`, `scout-fan.wav`,
`scout-piercing.wav`, `priest-rebuke.wav`, `warrior-crosscut.wav`,
`mage-meteor.wav`, `mage-firestorm.wav`, `priest-sanctuary.wav`, plus the original fallback-score previews. Actual recorded music is embedded
from `assets/audio/*-choir.wav`.
They are written into the test build directory. Signal checks establish
headroom, determinism, timing and memory limits; they do not establish that
the subjective sound quality matches the reference recordings.

Remaining acceptance: listen on speakers/headphones during repeated attacks,
judge fatigue and class recognition, review crowded battles at normal camera
distance, and assess encounter balance. This pass does not claim reference
parity, final listening approval or an FPS improvement.

Verification: thirteen affected Release suites passed across the initial and
corrected runs; AudioTests, ClassEffectsTests and ClassDisciplineTests also pass
in Visual Studio Debug. Localization coverage passes. Real-world Mage/Priest
contact captures and Priest readiness UI were inspected. Logs and detailed
results are under `build-class-discipline` and in `progress.md`. The wider
animation regression caught a legacy wrist discontinuity: authored elbow
transport now applies only to authored gestures, preserving the established
solver for procedural gestures; the original continuity limit passes again.

The impact-light/motion/audio follow-up passes eight affected Release suites
and five Visual Studio Debug suites. Native A/B captures verify light on real
world receivers, unchanged portraits/frozen facial materials/HUD, a finite
radius and identical draw counts after warming the cloud/shadow caches.
Animation tests retain the original hand/elbow/weapon speed and planted-foot
limits. Buffered techniques are exercised at 30/60/144/1000 Hz, including range
loss before execution. Evidence: `build-four-class-impact`, and
`build-ninja-release/combat-release-review/world-contact-lit-*.png` alongside
their unlit counterparts. The new 20-second audition is
`build-four-class-impact/four-class-combat.wav` (Warrior, Scout, Mage, Priest).

The signature follow-up passes seven affected Release suites and six Visual
Studio Debug suites: AudioTests, ClassEffectsTests, SignaturePresentationTests,
CombatReleaseTests, CastingEffectsTests and AuthoredLocomotionTests in both;
SpellChainsTests additionally in Release. The new suite exercises actual
level-ten casts, reserve earning, primary/secondary contacts and sound counts,
Priest's real scepter controller, 80 uninterrupted/cancelled motion traces,
foot anchors, native equal-time captures at 30/60/144 Hz, bounded lifetimes,
meteor endpoints, fan arrows, rendering idempotence and load cleanup.
The first test fixture had not finished the reserve-building basic attack;
waiting for its full recovery prevents testing the wrong action. Narrow beams
are checked by local pixel contrast rather than whole-frame average brightness.
Review images live in `build-ninja-release/signature-review`; the `world-wide`
views select an actually clear camera orbit through the nearby props. Native
and gameplay Mage/Priest captures were inspected. The seven Release suites
passed across the initial and corrected runs; no full-suite claim is made.
Current Release/Debug game and launcher outputs build; both RPGNative outputs
pass isolated startup/device/shader/resize/world/UI/capture smoke launches.
Logs and the combined audition are under `build-signature-combat`.
