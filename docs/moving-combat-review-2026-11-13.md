# Moving-target spell continuity

## Reference and reproduced problem

Reviewed twenty new samples from `D:/assets/learning_material/pve_loc2.mp4` at 7.6–9.6 seconds, twelve raid samples from `pve_raid_2.mp4` at 20–23 seconds, and the existing PvE overview. The material distinguishes launch streams, concentrated target-local contact and lingering ground effects, even when several effects overlap. These passages guide spatial readability; they do not prove a particular homing formula or complete reference parity. No reference art was copied into the game.

The native moving-combat review exposed a concrete mismatch: `SpellCast` captured an enemy's position at release, while actual damage used its position at contact. A pursuing enemy therefore received a hit away from the visible projectile's endpoint. The regression fixtures measure roughly 0.49–0.79 metres of horizontal error for heavy Mage, Scout and Priest attacks before correction. This was hidden by the earlier rooted-target review.

## Implementation

- Accepted action events carry the committed target ID separately from the caster and the current UI selection. No saved-game format changes are needed.
- Mage, Scout and Priest projectiles retain a bounded history of their actual centreline. Target movement bends only the new part of the path; already recorded wake positions remain fixed. The existing fire, frost, arcane, holy and arrow renderers consume that history, as does projectile lighting.
- History is sampled at up to 120 Hz, with a separate live endpoint and at most 64 samples. Long flights use a coarser fixed interval chosen at launch. Two history slots cover the authoritative player's single committed action and its outgoing tail, keeping total `CombatEffects` state at 16,136 bytes under the unchanged 16 KiB ceiling. No per-frame heap allocation or new GPU resource is introduced.
- A primary confirmed contact supplies the exact endpoint and incoming direction before the effect history closes. The impact orientation uses the final path tangent. Secondary area victims do not redirect the primary projectile.
- Changing selection cannot retarget an accepted flight. Dead, missing and returning targets retain the last destination; presentation does not fabricate damage or impact. Completed histories stop following actors and disappear with the existing burst lifetime. Loading clears them.
- Gameplay damage, release/contact clocks, range/line-of-sight checks, cast interruption, cooldowns and class resources are unchanged. This is presentation tracking for the existing targeted combat rules, not new ballistic collision or a server implementation.

## Review artifacts and checks

Artifacts are local and ignored by Git under `build-moving-combat/`:

- `before/combat-motion.mp4` and `after/combat-motion.mp4`: 20-second, 1280x720, 60 Hz native captures of Warrior, Mage, Scout and Priest. Enemies pursue and attack; ranged champions also move later in their scenes. Both recordings use the same scenario and have identical release/contact CSV rows. No fabricated hit events, root fixture or profile writes are used in this mode. The movies are silent.
- `crowded/combat-motion.mp4`: a separate native review of four class area/signature actions against three active opponents, including a caster and captain. This is a small encounter, not a raid-scale acceptance claim.
- `reference/`, `raid-reference/` and `after-samples/`: sampled reference and native sequences. Preparation, travel and contact stills are kept beside each movie.
- `budgets.csv` in each review directory records peak submitted effect vertices, local lights and draw calls. These are workload counters, not an FPS benchmark.

`MovingCombatTests` checks immutable history, 30/60/144/1000 Hz path agreement, invalid flights, 18 real moving-contact cases, selection changes, target death and load cleanup. The native contact cases finish within 0.002 metres of the actual hit; the measured error in the current run is zero. The CPU memory assertion remains unchanged.

Eight suites pass in Release (121.00 s) and Debug (271.38 s). Four game hosts pass native startup/device/resize/world/UI smoke checks. The crowded movie is 28 seconds at 720p60; reviewed peak-complexity frames retain incoming warning contours and cast captions. Peak effect vertices are 3,264 / 4,896 / 3,294 / 3,900 for Warrior / Mage / Scout / Priest, below the unchanged 6,144 limit. These counters include the active hostile effects; they are not performance timings. Validation logs, host smoke results and exact remaining acceptance notes are recorded with the completion entry in `progress.md`. Native reference-matched artwork, richer authored action staging, live crowd readability at other camera distances and full game fidelity remain open. Models, faces and authored assets were not edited by this pass; existing work in those files was preserved. No commit or push was performed.
