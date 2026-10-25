# Class setup attacks and split-coat motion

Implementation: 2026-10-25. Continues R3/R5/R6 with the existing custom DirectX11 renderer. Revisited the extracted withdrawal sheet from the local reference video at 02:30.500–02:30.983; this was not a full new video review. The target remains readable preparation/release/recovery and a coherent character silhouette through movement.

## Seven active spells per class

The new setup spell uses **8** on fresh hotbars. Existing layouts are preserved; drag the seventh art from **N** onto a slot. Key 5 remains the potion. Setup attacks use the basic contact/recovery clip at 85% of that class's basic animation rate, cost no reserve and have a separate eight-second cooldown. They deal 90% of basic damage and generate reserve only after a valid contact.

| Class | Setup spell | Contact payoff |
| --- | --- | --- |
| Warrior | Rending cut | One Rage and three seconds of exposure |
| Paladin | Oath strike | One Conviction and a 12-point ward |
| Scout | Trail shot | One Focus and a two-second slow |
| Priest | Guiding light | Two Faith |
| Druid | Thorn dart | One Bloom and a one-second root |
| Mage | Kindle | Two Embers |
| Necromancer | Grave touch | One Soul and a ward worth half actual damage |
| Psionic | Thought spike | One Insight and interruption of a pending enemy swing |
| Bard | Upbeat | One Tempo and two seconds removed from escape cooldown |
| Engineer | Tracer round | One Pressure and a three-second slow |
| Demonologist | Predator claw | One Fury; half-damage healing if the target was at half health or below |

Reserves cap at class capacity. Wards replace by maximum capacity, healing caps at missing health, and existing passive class effects remain active. This makes setup attacks a way to prepare control or reserve for techniques/signatures, rather than another full-reserve spender. These are prototype balance values requiring live encounter review.

Scout, Mage and Engineer setup attacks use the mobile basic-attack rules, not their stationary heavy preparation. Physical launchers still require an equipped weapon. Accepted casts retain the original target and recheck range/line of sight at contact. Cancellation/misses grant no reserve or payoff and do not refund cooldown. Setup spells are not added to the basic/heavy input buffer.

Presentation has a separate zero-ended setup gesture, class projectile/contact identity and a small three-ray contact accent. Character creation previews cycle basic, heavy, technique, signature and setup. N uses two rows for seven readable cards, retaining drag-to-bind behavior.

## Character visuals and movement

Priest, Druid, Mage, Necromancer, Psionic and Bard receive split front/back coat panels when a tunic is equipped. Panels follow the solved thighs, keeping the leg opening through steps and jumps. The original two-sided mesh has 198 vertices and 320 triangles, with smooth pleats and hem edging; each equipped character submits four panels. It uses existing cloth material, dyes, body attachment ownership and shadow rendering. Removing the tunic removes the panels. This is rigid articulation, not cloth collision/simulation or an imported authored hero.

Sharp grounded travel reversals add a short catch along the previous direction while the directional gait blend changes. The additive layer starts/ends at zero, is bounded and cannot retrigger until its current catch ends. It never moves the authoritative actor. Existing foot placement and hand/weapon solves remain active.

## Persistence and verification

Save version 17 appends the validated OPENING cooldown and still reads versions 1–16. Old saves acquire a ready setup spell. Loading resets unfinished actions/reserves; no delayed hit replays. Hotbar version 4 accepts ID 7 and reads versions 1–3 without replacing the user's assignments or empty slots.

Regression coverage: all eleven classes at 30/60/144/1000 Hz; no early/duplicate contact; reserve and class payoff; cancelled casts; mobile preparation; cooldown persistence and malformed/legacy records. Animation checks cover clip endpoints, fixed limb lengths, weapon/hand continuity and reversal lifecycle. Geometry checks enforce bounded two-sided topology, winding and normal quality. UI checks cover all seven cards, preview cycling and old binding migration. Manual checks 238–239 cover live feel and appearance.

Remaining gaps: the shared procedural anatomy and clothing are still well behind the reference characters. Full skin-weighted outfit production, authored clips, richer textures and individually illustrated/sounded spells remain open. Continue the authored-hero and coherent-encounter roadmap milestones rather than treating additional spell count as completion.Validation completed: 44/44 Release suites (98.62 s), 12/12 focused Debug suites (99.45 s), and final InterfaceTests after layout/migration adjustments in both configurations. Visual Studio Debug build and hidden startup/render smoke pass. Creation, action-pose and final spellbook captures were inspected. The scripted 1080p suite completed with CPU-frame p95 of 1.040 / 3.650 / 4.327 / 1.238 ms for spawn / walking / combat / merchant UI respectively; ending working set was 317.004 MiB. These are synthetic workload observations, not interactive FPS, a before/after speedup or coverage of every new spell. Live subjective animation and encounter balance checks remain manual.
