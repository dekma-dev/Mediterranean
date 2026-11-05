# Four-class combat and acoustic revision

Scope: Warrior, Mage, Scout and Priest; 2026-11-05. Existing spell icons,
inventory layouts, saved characters and Blender model sources are preserved.

## Sound

`engine/AcousticDesign.cpp` creates original PCM at startup. It uses damped
string feedback for the bow, modal wood/body contacts, filtered turbulent
noise and crackle for fire, inharmonic resonances for frost, and low pressure
and resonances for arcane spells. Priest effects use glottal excitation,
vowel formants, bronze-like modes and damped room reflections. These are
synthesized sounds, not recordings of an orchestra, singer or physical bow.

- Accepted Scout attacks now have a draw, release and separate spatial hit.
  Aimed shots have a heavier release. The old descending whistle is replaced.
- Mage preparation, release and target contact select fire, frost or arcane
  from the accepted ability. Thermal rupture uses the heavier fire sound;
  its visible effect still combines fire and ice.
- Priest preparation, radiant release, target contact and completed healing
  have distinct envelopes. Interrupted channels cannot play a healing completion.
- Warrior heavy releases retain the existing sword sounds and add a short
  synthesized effort. Other classes do not inherit that effort cue.
- Player steps and landings use heel/toe transients with grass, grit, paving
  and timber treatments. Existing movement-distance timing is preserved.
- Preparation voices fade after release/cancellation in approximately 185 ms
  (35 ms exponential time constant). A later cast cannot revive the old tail.
- Character selection has a separate 48-second pipe/choir theme. Exploration
  uses a sparse low-string/horn arrangement, with the existing combat layer.
  Menu/world music crossfades; menu playback suppresses world ambience and
  combat sounds. Focus loss suspends audio. Volume controls still apply.

The external Win32 launcher remains silent. Both scores are original procedural
compositions. No third-party audio was downloaded. Generation/reverb happens
once, outside the playback callback. The existing 22,050 Hz mono/16-bit bank
and stereo spatial mixing remain; this is not a new high-resolution sample
pipeline. Total resident generated PCM is 7,505,344 bytes (about 7.16 MiB),
below the new 8 MiB test ceiling. Sparse-voice headroom, distance attenuation
and twelve effect voices remain bounded.

## Visible combat and class opportunities

Mage attacks have larger flame/arcane heads, thicker material wakes, stronger
hand preparation and larger elemental contacts with short radiating flashes.
Priest attacks now use a golden projectile with a bright core, an expanding
seal and a tall contact shaft; healing has a wider attached fan. All use the
existing atlas and renderer budgets, rather than new textures or dynamic lights.
Sword and bow actions add a small load/release body response over the existing
animation controller and solved grips. Frozen faces are unchanged.

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
Official class references include the [Warrior overview](https://worldofwarcraft.blizzard.com/en-us/game/classes/warrior)
and [Allods Scout](https://allods.ru/about.php?show=class-scout).
Physical synthesis references include [Karplus-Strong](https://theory.stanford.edu/~blynn/sound/karplusstrong.html)
and [CMU sound synthesis](https://www.cs.cmu.edu/~15322/book/ch12/01.html).
The implementation and compositions are original, not extracted game assets.

`AudioTests` exports short auditions from real accepted combat event sequences:
`warrior-heavy.wav`, `scout-aimed.wav`, `mage-fire.wav`, `mage-frost.wav`,
`mage-arcane.wav`, `priest-rebuke.wav`, plus the two complete score previews.
They are written into the test build directory. Signal checks establish
headroom, determinism, timing and memory limits; they do not establish that
the subjective sound quality matches the reference recordings.

Remaining acceptance: listen on speakers/headphones during repeated attacks,
judge fatigue and class recognition, review crowded battles at normal camera
distance, and assess encounter balance. This pass does not claim reference
parity, finished recorded music or an FPS improvement.

Verification: thirteen affected Release suites passed across the initial and
corrected runs; AudioTests, ClassEffectsTests and ClassDisciplineTests also pass
in Visual Studio Debug. Localization coverage passes. Real-world Mage/Priest
contact captures and Priest readiness UI were inspected. Logs and detailed
results are under `build-class-discipline` and in `progress.md`. The wider
animation regression caught a legacy wrist discontinuity: authored elbow
transport now applies only to authored gestures, preserving the established
solver for procedural gestures; the original continuity limit passes again.
