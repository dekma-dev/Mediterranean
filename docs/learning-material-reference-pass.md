# Character stage and combat presentation reference pass

Reference root: `D:/assets/learning_material`. Date: 2026-11-06.

## Wardrobe, audible showcase and combat follow-up

The follow-up revisited the creation overview and full-size frame at 44 seconds
(equipment-tier preview), plus the PvE/raid overviews and full-size raid opening
(outlined combat numbers and bright spell trails). The recordings are visual
references; their soundtracks and assets were not copied or downloaded.

- Three stage wardrobe modes use existing interchangeable equipment: actual
  equipped gear, travel gear and veteran gear. The class weapon and offhand
  rules remain intact. The preview never grants items, changes stats or writes
  a profile; returning to the roster restores the actual equipped view.
- A toggle cycles basic, heavy, tactical and signature demonstrations with a
  rest between actions. Individual art selection stops the cycle. The same
  class preparation/release/recovery sounds used in combat now play in the
  stage through the existing bounded XAudio2 mixer. These are existing samples,
  not a newly recorded sound bank. There are no invented target-impact sounds.
- Audition events are revisioned, consumed once and cancelled on class/page
  changes, wardrobe/reset, native suspension and focus loss. Preparation tails
  use the existing release fade. Music/effects volume and mute still apply.
  Native suspension is tested without relying on an unfocused game update,
  because the real window loop waits while the application is inactive.
- Arcane prism now has a refracting core, three braided light trails and
  separating contact facets, distinct from Arcane severance's orb. It uses the
  accepted projectile clock/destination and the existing effect atlas/budget.
  The spell's gameplay damage, resource, movement and cooldown rules are intact.
- Combat numbers use stable per-event identities, separate outgoing/incoming/
  healing paths, a small arrival emphasis, outline and smooth tail fade. Their
  size and rise speed follow UI scale rather than camera distance. Expiring an
  older entry no longer shifts another number through a vector-index offset.
  Values still come from accepted simulation feedback; no fake critical hits.

Verification covers wardrobe/save isolation, audible phases at 30/60/144 Hz,
bounded/idempotent audio consumption, full showcase sequences, native
suspension, real hit identity and equal-time text motion. Native captures cover
all four focused class wardrobes, live combat text and prism flight/contact.
Final test results and remaining limitations are recorded in `progress.md`.
This does not add new character geometry, genders, races, a new animation
library, raid networking or full reference fidelity.

Follow-up verification completed: LocalizationTests, CharacterSelectionTests,
AudioTests, VfxMaterialsTests, InterfaceTests, CombatHudTests and SimulationTests
pass in both Release and Debug. After the last caption contrast/localization
edit, localization, character selection and interface suites pass again in
both configurations. Both final RPGNative executables pass hidden fullscreen
restoration, device/shader initialization, resize and world/UI capture smoke.
Logs: `build-learning-review/showcase-final-tests-{release,debug}.log`,
`showcase-ui-tests-{release,debug}.log`, and
`showcase-smoke-{release,debug}.png.log`. Native captures reviewed include
`release/character-selection-check/wardrobe-5-2.png`,
`release/vfx-material-review/prism-flight-60.png`,
`release/vfx-material-review/world-contact-5-13.png`, and
`release/combat-hud-review/contact-text-5.png` under that same review directory.

## Evidence inspected

The local Media Foundation review decoded the recordings without modifying or
uploading them. This was a sampled visual review, not an audio analysis or an
inspection of every source frame. Ignored working evidence is in
`build-learning-review/`.

| Recording | Duration | Inspected sampling |
| --- | ---: | --- |
| `character_creating.mp4` | 96.467 s | 4-second overview; full-size creation/appearance views at 16, 32 and 40 s |
| `pve_loc.mp4` | 87.082 s | 6-second overview, including combat, map and career screens |
| `pve_loc2.mp4` | 13.916 s | 3-second overview; 0.15-second samples from 5 to 7 s, including a full-size combat view |
| `pve_raid.mp4` | 111.717 s | 8-second overview of combat, warning zones, raid frames and effects |

The priority creation recording uses an open scenic stage, a class rail on the
right, an identity field on the left during customization, compact appearance
rows and a persistent bottom navigation rail. The hero remains visible while
choosing options. Close inspection and class gestures are part of that scene.
Our previous composition put large framed windows on both sides and used an
older, less complete attack pose without any portrait particles.

The PvE recordings place player/target information near the action, a cast
meter above the bottom spell rows, statuses under unit frames, and distinct
preparation, release and contact effects. Raid warnings must remain legible
through spell volume. Existing native combat HUD anchors already address much
of this arrangement; a full copy of the raid systems is not implemented here.

## Implemented

- Replaced the two large character-menu windows with translucent edge rails,
  a larger central stage and persistent bottom controls. Existing detailed
  interface artwork and unique spell icons remain in use.
- Moved class/roster selection to the right; class description, arts, talents
  and identity entry use the left rail. Appearance choices occupy the right
  rail only on the identity page. No invisible class buttons remain active
  behind the appearance controls.
- Added stage-only left-drag rotation and smoothly bounded mouse-wheel zoom.
  Focus loss, resize, display changes and page transitions cancel gestures.
  Releasing a rotation over Create cannot publish a character.
- Character auditions now use the runtime authored weapon controller,
  including sword body accents, bow draw/release and Priest hand gestures.
  The class spell rate drives the presentation. Actual existing face data and
  models are preserved; no authoring/export regeneration is part of this work.
- Added preparation, release, weapon trails and attached recovery effects to
  the stage, using the same native effect artwork and preparation renderer as
  combat. Portrait effects require explicit opt-in; inventory/unit portraits
  keep their existing behavior. Soft intersections use the stage projection's
  clip planes, and stage seals use the stage floor rather than world terrain.
- The arts view exposes all fourteen implemented Warrior/Mage/Scout active
  spells, including their level requirements. Other classes retain their
  seven implemented arts. Previewing a locked progression spell neither
  learns it nor changes a character's save, cooldowns, reserve or world state.
- Actual combat casts now show the corresponding unique icon and a short
  release/interruption receipt. Release is established by simulation events;
  cancelling follow-through cannot retroactively turn a successful release
  into an interruption. Instant protection is not given a fabricated cast.

Auditions have no target and do not simulate damage, impact on a victim or
combat rewards. They demonstrate the gesture, charge and release; full combat
acceptance still belongs at practice targets and in encounters.

## Verification and remaining work

`CharacterSelectionTests` covers profile publication/conflicts, input isolation,
14/7-art availability, runtime weapon participation, release events, camera
gestures at multiple resolutions, focus/reset cancellation and native captures.
`CombatHudTests` checks release/interruption receipts at 30/60/144 Hz, instant
protection and reset behavior. Shared effect tests cover existing world casts,
depth, lifetime, bounds and repeated rendering.

Initial stage validation: all seven affected suites pass in Release and Debug:
LocalizationTests, CharacterSelectionTests, ClassEffectsTests,
VfxMaterialsTests, CastingEffectsTests, InterfaceTests and CombatHudTests.
The initial Debug interface run exposed stack exhaustion in nested full-game
test fixtures; those fixtures now use owned heap storage. InterfaceTests was
rerun after that correction. This does not raise the executable stack limit.
Both RPGNative configurations also pass the hidden startup/fullscreen-restore,
resize, shader and world/UI capture smoke test. Native creation captures were
visually inspected; these checks do not establish subjective reference parity.
Logs: `build-learning-review/isolated-tests-{release,debug}.log`,
`interface-stack-tests-{release,debug}.log` and `smoke-{release,debug}.png.log`.

This is not full reference parity. Missing reference features include real
race/sex choices with matching authored data, additional appearance geometry,
authored showcase choreography and reference-level
environment composition. The current face freeze remains in force. Combat
balance, group/raid mechanics, additional authored enemy actions and final
crowded-scene effect readability remain separate roadmap work.

## Try it

For this pass, launch `build-learning-review/release/RPGNative.exe` (or the
Debug equivalent). These isolated outputs avoid sharing intermediate build
files with another active development session.
Choose **Create character**, select a class and open
**Battle arts**. Try basic/heavy and advanced spells; drag the figure, use the
wheel and switch to appearance. In Russian the actions are **Создать персонажа**,
**Боевые умения**, **Внешность и имя**. Existing saved characters remain selectable.
Use **Текущее / Походное / Ветеранское** to inspect existing equipment tiers,
and **Демонстрация** for the repeated audible class sequence. This is a preview;
creating or entering the character uses the character's actual equipment.
For combat, cast against a training dummy, then cancel a preparation by moving;
compare the brief **Выпущено** and **Прервано** receipts above the action rows.
