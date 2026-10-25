# Class techniques and character response

Implementation checkpoint: 2026-10-25. This pass follows R3/R5/R6 and the existing research. Reviewed the previously extracted consecutive approach frames from `allods_fight_interface.mp4` (01:17.500–01:17.983); this was not a full new video review.

## Five active actions per class

Each class now has one additional tactical attack. It is available at level one, appears in N's spellbook and occupies key 6 on a fresh action bar. Existing saved bindings stay intact: drag the technique from N to the desired slot. Key 5 remains the potion. Techniques have a separate 12-second cooldown and do not consume or generate the heavy-attack reserve.

| Class | Technique | Contact effect |
| --- | --- | --- |
| Warrior | Hamstring | Slow for four seconds |
| Paladin | Radiant seal | 24-point ward |
| Scout | Pinning arrow | Root for two seconds |
| Priest | Absolution | Heal for half of actual damage dealt |
| Druid | Briar grasp | Root for three seconds |
| Mage | Winter lance | Slow for three seconds; stronger against slowed/rooted targets |
| Necromancer | Soul harvest | Heal for 60% of actual damage dealt |
| Psionic | Mind fracture | Stun for 1.5 seconds and interrupt a wind-up |
| Bard | Dissonance | Interrupt and expose for four seconds |
| Engineer | Armor piercer | Expose for six seconds |
| Demonologist | Blood rend | Stronger damage; six-health commitment cost, capped above death |

Base damage is 65% of heavy damage, except Blood rend (85%) and Winter lance against a controlled target (110%). Existing exposure multiplies incoming damage by 1.25. Effects refresh by maximum duration/capacity; they do not stack without bounds. Healing is limited by actual damage and missing health. These are original prototype abilities, not claims of reproducing another game's balance.

Technique preparation/recovery runs at 80% of the class's heavy animation speed. Contact remains simulation-owned, commits to the original actor and rechecks range/line of sight. Misses and interrupted preparation do not apply control or healing. The cooldown starts at commitment and is not refunded. Classes with planted heavy preparation also require a stationary technique. Esc, death and lifecycle cancellation retain the existing rules.

## Presentation and movement

Technique gestures add a smooth zero-ended body load and hand/torso accent to each existing class motif. They retain fixed-length limb solves and release sockets. Tactical contacts have a brief broken ground seal; Winter lance has a cold preparation, projectile and shard impact instead of the fire projectile. Other techniques reuse their class's existing release/impact motifs and sound bank.

Stationary-turn torso clips now advance from angular movement. Previously their clock depended only on translation, holding a nearly static pose during a pivot. The pivot contribution fades out during travel and does not run airborne. This changes additive pose animation, not movement authority or collision.

Garments gain paired closures, collar lining and small rivets, attached to the solved torso and removed with the chest item. Existing attachment LOD still applies. The fitting budget is below 48 parts per outfit; the previous 32-part test budget was expanded explicitly for this detail. No imported hero or new anatomy is claimed.

## Persistence and acceptance

Save version 15 stores a validated `TECHNIQUE` cooldown record. Versions 1–14 load with that cooldown ready. Saves do not resume unfinished attacks or replay contact. Hotbar version 2 accepts the new action ID while retaining the old potion ID; version 1 remains readable. Older executables cannot read new saves/settings.

Automated checks cover all eleven techniques at 30/60/144/1000 Hz, contact uniqueness, cancellation, planted preparation, save migration and invalid-record rejection. Expanded action tests cover technique limb lengths and continuity; UI tests inspect all five spell cards at multiple sizes. Live motion, readability and sound quality still require human acceptance.

Validation: all 43 Release suites pass (86.18 s); the final card-fitting/technique-pose capture changes pass both affected Release suites again (14.76 s). Nine focused Debug suites pass (78.84 s). Visual Studio Debug builds successfully, and both Debug backends pass the hidden startup/shader/resize/fullscreen/render smoke test. Inspected the 1280px technique spellbook, Mage technique pose and cold impact captures. Logs: `technique-final-tests.log`, `technique-polish-tests.log` and `technique-tests.log` in the respective build directories, plus `build-local/technique-smoke.png.log`.

The existing hidden 1080p workload suite on the RTX 4060 reports CPU-frame p95 of 1.085/3.665/4.368/1.227 ms for spawn/walking/combat/merchant UI; no measured CPU frame exceeds 16.667 ms. Ending working set is 316.94 MiB. This is a short synthetic check of the existing route, not interactive FPS, a paired speedup measurement or exhaustive new-technique performance coverage. Report: `build-ninja-release/technique-benchmark.txt`.

R2 imported-hero GPU integration, authored anatomy/clothing and a larger active spell library remain open. This is a fifth action per class, not a full MMO-sized spellbook.
