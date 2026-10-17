# Dawnwatch gameplay rules

This is an original single-player campaign on one island. Allods Online is the primary setting and presentation reference; these numbers are our prototype rules, not copied live-game balance. No external assets are required.

## Campaign

| Stage | Objective | Reward |
| --- | --- | --- |
| Frontier patrol | Accept at the gold beacon, defeat any raider, return | 25 gold, 60 XP, watchguard blade |
| Secure the astral anchor | Accept after the patrol, defeat all three enemies, stabilize the northern violet crystal, return | 50 gold, 120 XP, equipped ward armor |

Each enemy grants 40 XP once. E claims its bag once for 5 gold and one potion. Kills before quest acceptance count. Finishing both quests and looting all enemies yields 300 XP and 90 gold. The objective tracker, interaction prompt, journal, and map identify the current step.

The anchor is at `(0, 12.8)`, in front of the northern ruin. E starts a three-second channel within 2.5 units after the camp is clear. Movement, jumping, abilities, potion use, equipment switching, or damage cancels it. Saving during the channel records the active quest without partial channel progress; loading requires starting again.

## Progression

| Level | Total XP threshold | Maximum health | Strike bonus | Heavy bonus |
| --- | --- | --- | --- | --- |
| 1 | 0 | 100 | 0 | 0 |
| 2 | 100 | 120 | 3 | 5 |
| 3 | 250 | 140 | 6 | 10 |

Level gains restore health to the new maximum. XP and level derive from persistent defeated-enemy and completed-quest flags, preventing duplicate XP on load or respawn. Level 3 is the campaign cap.

The starter sword deals 20 / 35 base damage; the watchguard blade deals 27 / 45. Level bonuses apply to both. G switches owned swords. Ward armor equips automatically after the final quest and reduces incoming damage by 2, with a minimum of 1. There are no further living enemies after that hand-in; using the armor against new encounters is future scope. No armor item grid or equipment selection UI is implemented.

## Combat

| Enemy | Health | Swing damage | Windup | Recovery after impact |
| --- | --- | --- | --- | --- |
| Raider | 80 | 8 | 0.7 seconds | 1.4 seconds |
| Captain | 120 | 16 | 1 second | 2.4 seconds |

Enemies start winding up within 2 units and commit facing and position until impact. Their visible cone extends 2.4 units with a half-angle of 1 radian. Distance, direction, line of sight, and player jump height are checked again at impact. Movement and dash can avoid the swing; heavy strike interrupts an active windup and gives 0.8 seconds of recovery. Ordinary strike does not interrupt. Returning enemies cannot be targeted or damaged until they finish their home reset.

Player strike/heavy attacks require a living target within 2.5 units, line of sight, and jump height at most 1.5. Cooldowns are 0.9 / 4 seconds. Dash moves up to 3.5 units along facing, respects collision, and has a 5-second cooldown. Mend restores up to 35 health with a 10-second cooldown. Q consumes a potion for up to 40 health with a 3-second cooldown. Full-health healing consumes neither cooldown nor potion.

World text reports damage, healing, misses, interrupts, XP, and levels. The player flashes when hit. The captain has violet clothing, brass shoulders, and a peaked helmet. Art remains procedural placeholder geometry.

## Death, restart, and saves

- Enter while defeated returns the player to Dawnwatch with full earned maximum health. Quests, XP, gold, potions, equipment, dead enemies, and loot claims persist. Living enemies recover at home. Cooldowns and transient presentation reset.
- R clears the current session for a new expedition without erasing the on-disk save.
- F5 writes version 3; F9 validates before replacing state. Committed enemy windups and facing persist. Camera, jump, target, panels, feedback, and unfinished anchor channels are transient.
- Versions 1 and 2 migrate. Their third raider becomes the captain at the same health fraction; completed patrols unlock the second quest. Existing kills and hand-ins supply XP. Current player health is retained within the earned maximum.

## Validation and limits

`CombatTests` covers delayed impact, recovery, dash avoidance, committed-facing misses, interruption, captain tuning, saved windups, and invalid-save rejection. `ProgressionTests` plays the two-quest sequence through commands and checks levels, channel cancellation, unique rewards, migration, respawn, and restart. The existing simulation, navigation, and inventory suites also run.

Hidden captures cover spawn, raider windup, captain windup, and a completed veteran at three sizes. They do not replace real keyboard/mouse playtesting or measure combat feel, performance, or accessibility. There is no audio, skeletal animation, networking, repeatable enemy spawning, or additional map.
