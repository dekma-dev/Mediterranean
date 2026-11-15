# Four-class finishers and Priest disciplines

## Reference evidence and scope

Locally decoded samples from `D:/assets/learning_material/allods_fight_interface.mp4`
at 121.00–122.60 seconds and `pve_loc2.mp4` at 7.30–8.90 seconds were inspected
at approximately 80 ms intervals. These short sequences show a separation
between moving attack strokes, bright contact accents and lingering effects;
the actor remains readable between the luminous shapes. They are sampled
sequences, not a new frame-by-frame review of every supplied recording.

The [official Allods class guide](https://allods.ru/about.php?show=classguide)
describes the Priest through prayer, protection and healing. The
[official WoW Priest overview](https://worldofwarcraft.blizzard.com/en-gb/game/classes/priest)
also identifies protective prayers and healing as parts of the class. The new
kit below is our single-player implementation inspired by those roles, not
copied ability tables or a claim of matching either game's balance.

Models, facial channels, Blender source, authored exports and animation clips
were not edited. This pass adds runtime motion layers, gameplay and effects.
Full visual/reference parity remains open, particularly actor staging, authored
action variety, environment quality and readability in larger encounters.

## Playable changes

Priest now has fourteen active abilities, matching the availability of the
other three focused classes. Seven newly available abilities use the existing
durable IDs and save records; existing hotbar layouts are preserved. Drag the
new abilities from **N** to an empty slot. Russian and English descriptions
explain their requirements, actual clocks, base damage and conditional rewards.

| ID | Ability / Russian name | New role |
|---|---|---|
| 8 | Consecration / Освящение | Level 5 stationary, target-centred 3 m exposure field; one primary 18-point ward |
| 9 | Judgment / Суд | Level 8 stationary strike; consumes exposure for bonus damage and restores 16 health on contact |
| 10 | Hushed bell / Колокол безмолвия | Level 6 mobile interrupt; one Faith on contact, 24-point ward only on successful interruption |
| 11 | Ascension / Вознесение | Level 10 full-Faith invocation; 3.5 m area, one primary 30-point heal and 24-point ward |
| 12 | Votive spark / Искра веры | Mobile second basic; grants Faith and advances alternating chains without spending Benediction |
| 13 | Litany / Литания | Level 3 mobile exposure/chain link; one Faith and a 12-point ward |
| 14 | Final verdict / Приговор | Level 4 mobile finisher; consumes two/three-step chains for damage, 16/24 healing and ward |

Area effects validate range and line of sight for every secondary victim.
Healing and primary wards do not multiply with the number of victims. A miss,
cancelled preparation or loaded pending action cannot award delayed recovery.
The existing heavy attack can also spend the Priest chain for its smaller
damage bonus. Rank and cooldown gates remain active; there are no free skills
in a fresh low-level character beyond the new basic.

## Distinct action presentation

- Warrior Crossing steel adds a returning weapon-arm arc after the accepted
  contact and two staggered diagonal steel sheets with an inner bright edge.
  This is visual follow-through; it does not add a second gameplay hit.
- Scout fan/crossfire follow-through moves the bow arm and balances the torso.
  Crossfire has opposing oblique wakes with open space around the victim.
- Mage Thermal rupture counter-rotates the upper body and combines a swept
  flame form, an opposed frost filament, ice fragments, two bounded volumes
  and a short steam tail.
- Priest new invocations use raised/open prayer gestures, descending judgment,
  a wrapping litany, a narrow spark and crossed final-verdict lances. Area
  invocations follow terrain with their actual 3/3.5 m footprint. Existing holy
  sound composites are routed to the new grand/judgment contacts; there are no
  new downloaded sound files or new recorded performances in this pass.

Seven original Priest icons are embedded separately. Existing icons are
preserved. Prompt, cell mapping and texture budget are recorded in
`assets/ui/spell-artwork.md`.

## Validation and review artifacts

The selected **20 Release checks and 18 Debug checks pass** (not a claim that
the entire repository test suite was run). Coverage includes:

- Seven Priest spells at 30/60/144/1000 Hz: area boundaries, one primary event,
  contact-only recovery, exposure consumption, interrupt immunity, Faith
  commitment, movement cancellation, chains, Benediction and save/load.
- Existing class combat and Southern encounter rules; all-class Russian names,
  descriptions and target captions; chain persistence and hotbar editing.
- Native material, martial/caster world, new Priest contact, release and
  preparation checks. Priest contacts are inspected from three angles over four
  phases, expire correctly and fit six simultaneous effects without silently
  truncating geometry or using the hostile-warning allowance.
- Per-class pose checks, retaining the **13 m/s** wrist-speed bound and planted
  feet. Measured maxima: Warrior 7.70707, Scout 8.26775, Mage 9.33582 and Priest
  7.5335 m/s. Alternating cuts peak at 5.57714 m/s across 30/60/144/1000 Hz.
- Release character-creation/preview and interface regressions, including the
  additional Priest arts. All 116 used icon cells are unique/in bounds, and
  total UI texture storage stays below the existing 30 MiB ceiling.

The original combined Debug signature test exceeded 180 seconds under the
aggregate workload. Material rendering, martial/caster world scenarios and
four class pose audits now have separate CTest entries. No assertion, sample
frequency, gameplay scenario, geometry cap or speed limit was relaxed. The
three final Debug native partitions pass in 10.59 / 89.61 / 78.53 seconds;
the four pose partitions also pass. See `signatures-native-debug.log`,
`signatures-debug.log`, `signatures-release.log`, `tests-debug.log` and
`tests-release.log` under `build-action-expression`. The earlier timeout is
retained in its log and superseded by the successful partitioned checks.

Visual Studio Debug/Release and Ninja Debug/Release game and launcher builds
completed. All four RPGNative executables passed the native startup/save-fixture
smoke and produced screenshots. Their final SHA-256 hashes still match
`build-action-expression/validation-snapshot.json`. Existing compiler shadowing
warnings remain; no clean-warning claim is made.

Open **`build-action-expression/review.html`** for the local comparison gallery.

| Artifact | Purpose |
|---|---|
| `accepted-motion/combat-motion.mp4` | 36 seconds, 1280×720 at 60 Hz: actual three-step Warrior, Scout, Mage and Priest chains |
| `accepted-priest/combat-motion.mp4` | 16 seconds: actual Consecration, Judgment, Hushed bell and Ascension releases/contacts |
| `accepted-chains/` | Final native spellbook, icon, chain HUD and world contact captures |
| `baseline-chains/` | Before captures from the retained `build-opportunities/release/SpellChainsTests.exe` |
| `reference-warrior/`, `reference-casting/` | Locally decoded reference samples and contact sheets |
| `review-provenance.json` | Full hashes of the retained baseline and final movie executable |

Paths in the table are relative to `build-action-expression`. The final movie
executable hash starts `EF5DE0B2B91AE005`; the baseline executable starts
`1741190DC7B765045`. Earlier exploratory `movie-chains` / `movie-priest` captures
predate the final tooltip/readiness fix; use the **accepted** movies above.
The baseline's hotbars come from its separate source checkout; current captures
preserve the user's actual current hotbar bindings. The world phase/camera
comparisons evaluate the effects, not identical bindings.

Both final movies' event CSVs exactly match their simulation-only preflights.
Every class chain spends exactly three earned steps once. Movies are silent,
offline captures, not measured real-time FPS. Their observed maxima are:

| Scene | Effect vertices | Combat lights | Spell volumes |
|---|---:|---:|---:|
| Crossing steel | 1260 | 4 | 0 |
| Crossfire | 1284 | 4 | 0 |
| Thermal rupture | 2442 | 4 | 8 |
| Final verdict | 1500 | 4 | 2 |
| Consecration | 1668 | 4 | 2 |
| Judgment | 1182 | 4 | 2 |
| Hushed bell | 1608 | 4 | 3 |
| Ascension | 1746 | 4 | 2 |

These are bounded by the existing 6144 vertices, four lights and 24 volumes.
Draw counts are 198–213 in these fixed scenes. Sampled playback shows moving
open shapes and visible victims; near-camera stills also show less of the
previous broad thermal cloud. Subjective impact, repeated-fight audio balance,
the new Priest's sustained survivability and full crowded-pull readability
still require live-play acceptance. No benchmark or 1:1 acceptance is implied.
