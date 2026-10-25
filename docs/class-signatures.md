# Class signatures and motion detail

Implementation: 2026-10-25. Follows roadmap R3/R5/R6. Revisited the existing consecutive approach sheet from the local combat recording, 01:17.500–01:17.983; this is not a new full-video analysis.

## Six active actions per class

Each class receives one signature on fresh key **7**. Key 5 remains the potion and key 6 the technique. Existing saved layouts are preserved: open **N**, inspect the signature and drag it to a free slot. Character creation shows all six arts and cycles basic/heavy/technique/signature previews.

Signatures require and consume a full class reserve. They compete with empowered heavy attacks. They have an independent 18-second cooldown and use 70% of the class heavy animation rate. Invalid range/target/weapon/stance requests spend nothing. Once accepted, cancellation or a miss does not refund the reserve or cooldown. Contact rechecks the original actor, range and line of sight. Movement cancels planted preparation according to the existing class rules.

| Class | Signature | Distinct payoff |
| --- | --- | --- |
| Warrior | Execution | Double damage against enemies at half health or less |
| Paladin | Dawn verdict | 40-point ward |
| Scout | Deadeye | Extra damage against rooted targets; refreshes escape |
| Priest | Sanctuary strike | 30 healing and a 10-point ward |
| Druid | Wild judgement | Four-second root and six-second slow |
| Mage | Glacial shatter | Consumes an existing slow/root for double damage |
| Necromancer | Soul eclipse | Restores 80% of actual damage dealt |
| Psionic | Mind prison | Interrupt and 2.5-second stun |
| Bard | Grand finale | Refreshes basic attack and removes five seconds from technique cooldown |
| Engineer | Overload round | Eight-second exposure |
| Demonologist | Infernal bargain | Up to twelve health spent above the death threshold; 25-point ward on contact |

Base damage is 110% of heavy damage, except Demonologist at 160%. Scout's rooted-target bonus adds 70% of heavy damage. Existing exposure and the Overwhelming talent apply; wards replace by maximum capacity, healing caps at missing health. These are original prototype tuning values, not replicas of the reference games.

Signature gestures use a separate smooth gathering/weight accent. Their release retains the existing weapon-hand socket and class projectile; contact adds bounded paired crescents and rising motes in the class color. Icons reuse existing class art with a gold border; bespoke spell illustrations and audio remain future work.

## Movement and appearance

Start pushes and stopping catches retain their accepted forward/sideways direction. Releasing input no longer changes a backward or lateral stop into a forward lean. Weapon carrying posture is layered before the shared limb solve and fades during actions, air travel, dashes and hits. Fixed-length limbs, planted feet and weapon sockets remain authoritative for presentation.

The procedural hair mesh has a swept hairline, a longer nape, shallow lock relief and vertex shading. Boot shafts gain two leather bands and brass clasps. Default cloth is muted; explicit chosen dyes retain their exact values. No new textures, dependencies or downloaded assets. Added boot detail costs eight rigid instances per equipped character; the reusable hair vertex/index counts are unchanged.

These are incremental improvements, not completion of the authored-character milestone. Current anatomy, clothing topology, high-detail textures and a production animation library remain visibly behind the references.

## Persistence and acceptance

Save version 16 appends a validated SIGNATURE cooldown; versions 1–15 remain accepted. Loading never recreates an in-flight cast or reserve. Hotbar version 3 accepts the new ID 6 and reads versions 1–2 without inserting spells into user-assigned slots. Malformed records reject atomically.

Regression coverage includes eleven classes at 30/144/1000 Hz, rejected/accepted/cancelled casts, contact uniqueness, event identity, cooldown persistence and version-15 migration. Additional checks cover directional stopping, signature clip endpoints, fixed joints, hand/weapon continuity, effects budgets, six spellbook hit regions and preview cycling. Manual checks must still judge motion feel, readability and gameplay balance.

## Reference basis

[Blizzard's Warrior class overview](https://worldofwarcraft.blizzard.com/en-us/game/classes/warrior) describes accumulated Rage powering stronger attacks. The design inference used here is a meaningful resource-spending choice, not copied coefficients.

[Allods' official combat basics](https://allods.my.games/en/game/game-guide/basics) documents selected targets, range feedback and inspectable action-bar spells. This pass keeps those explicit contracts. The page is a gameplay reference, not evidence of proprietary engine behavior or current Russian-server balance.


## Validation evidence

The full Release run passed 43 of 44 suites; the failing new save assertion supplied an invalid 100-second enemy cooldown. After correcting that fixture, six affected checks passed, followed by the two UI checks after the icon repair. Eleven focused Debug suites passed. The final Mage palette/control-combo checks passed in both configurations. Visual Studio Debug rebuilt and passed the hidden startup/shader/fullscreen/resize/world/UI smoke check. See progress.md and signature-* logs in the build directories for timings.

The 1920x1080 synthetic route measured CPU-frame p95 of 1.175/3.670/4.272/1.252 ms (spawn/walking/combat/merchant), zero measured CPU frames above 16.667 ms, and 317.004 MiB ending working set. This is a single observation, not a speedup comparison or live-play FPS result. It exercises the existing combat workload, not every new signature combination. Live motion/balance acceptance remains manual.
