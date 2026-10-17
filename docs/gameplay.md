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

The starter sword deals 20 / 35 base damage; the watchguard blade deals 27 / 45; an empty weapon slot deals 6 / 10. Level bonuses apply to all three. G switches owned swords; before the reward is earned it re-equips the starter sword if the weapon slot is empty. Ward armor initially equips after the final quest and reduces incoming damage by 2 while worn, with a minimum of 1. Changing gear never changes earned XP or level.

The inventory has 24 bag cells and separate weapon/chest/legs/feet/head/off-hand slots. Starting gear is a sword, tunic, trousers and boots, plus two potions in the bag. Starting clothes have no armor bonus. C and I open the character and bag together: drag to swap cells or equip/remove compatible gear, right-click for an item action, or use the detail button. Sort compacts the bag and merges potion stacks up to 20. Unique gear cannot duplicate, incompatible actions change nothing, and rewards are granted only after the item fits. Disposal, stack splitting and selling remain unimplemented.

## Quartermaster and economy

The village quartermaster stands at `(3, -12)`, marked orange on the atlas/minimap. E opens the shop within 2.75 units and clear line of sight, while alive, grounded and outside practice. Nearby unclaimed loot takes priority. E or X closes trade; Escape opens the live menu. The shop captures movement/combat keys while time continues, and allows the bag and character window alongside it.

| Offer | Price | Initial stock | Rule / effect |
| --- | ---: | ---: | --- |
| Healing draught | 6 gold | 6 | Normal 40-HP potion; stored in the bag |
| Watchman's helm | 20 gold | 1 | Patrol handed in; Head; +1 protection while worn |
| Dawnwatch shield | 35 gold | 1 | Patrol handed in; Off hand; +2 protection while worn |

Buying validates range, life/grounded state, quest unlock, stock, gold and bag capacity in the simulation. Item, charge and stock update together; bought equipment remains stowed until equipped. Ward + helm + shield provide 5 protection, so an ordinary 8-point swing deals 3 and a captain's 16-point swing deals 11. This is passive protection; no active block or shield ability exists yet.

The campaign earns at most 90 gold before spending. Buying both equipment items costs 55, leaving 35 for up to five draughts; buying all six draughts costs 36. There are no repeatable income sources, refunds, selling or restocking. Saves and death recovery retain purchases; only R starts a fresh expedition with fresh stock. Saving after consuming a purchased potion retains the spent gold and depleted stock.

## Combat

| Enemy | Health | Swing damage | Windup | Recovery after impact |
| --- | --- | --- | --- | --- |
| Raider | 80 | 8 | 0.7 seconds | 1.4 seconds |
| Captain | 120 | 16 | 1 second | 2.4 seconds |
| Sparring guard | 180 | 8 | 0.9 seconds | 1.4 seconds |

Enemies start winding up within 2 units and commit facing and position until impact. Their visible cone extends 2.4 units with a half-angle of 1 radian. Distance, direction, line of sight, and player jump height are checked again at impact. Movement and dash can avoid the swing; heavy strike interrupts an active windup and gives 0.8 seconds of recovery. Ordinary strike does not interrupt. Returning enemies cannot be targeted or damaged until they finish their home reset.

Player strike/heavy attacks require a living target within 2.5 units, line of sight, and jump height at most 1.5. Cooldowns are 0.9 / 4 seconds. Dash moves up to 3.5 units along facing, respects collision, and has a 5-second cooldown. Mend restores up to 35 health with a 10-second cooldown. Q consumes a potion for up to 40 health with a 3-second cooldown. Full-health healing consumes neither cooldown nor potion.

World text reports damage, healing, misses, interrupts, XP, and levels. The player flashes when hit. The captain has violet clothing, brass shoulders, and a peaked helmet. Art remains procedural placeholder geometry.

## Repeatable sparring

After completing both quests, press E within 2.5 units of the eastern training post at `(12, -2)` (cyan map marker). A dedicated guard appears and is automatically targeted. The 30-second round uses the same melee, telegraph, interrupt, armor and navigation rules as campaign combat. Worn ward armor reduces a normal hit from 8 to 6; adding the helm and shield reduces it to 3. The tunic provides no reduction.

Defeat the guard to win, press E to stop, or move more than 7 units from the post to leave. Reaching 1 HP ends the round without killing the player. The result displays damage dealt/taken for eight seconds. Practice grants no XP, gold, loot or campaign kill credit. Health, ability cooldowns and consumed potions are not refunded; Mend and potions work normally. Restarting practice cannot heal or farm rewards. Nearby campaign loot takes precedence over starting a round.

The opponent, timer and result are transient. Saving retains current health, resources and gear; loading ends the round. Campaign enemies remain defeated. This supports equipment/combat testing on the existing island without adding another map or unlimited progression.

## Death, restart, and saves

- Enter while defeated returns the player to Dawnwatch with full earned maximum health. Quests, XP, gold, potions, equipment, dead enemies, and loot claims persist. Living enemies recover at home. Cooldowns and transient presentation reset.
- R clears the current session for a new expedition without erasing the on-disk save.
- F5 writes version 5; F9 validates before replacing state. Exact bag cells/stacks, six worn slots, purchase counts, spent gold and committed campaign enemy windups/facing persist. Gold must equal earned rewards/loot minus purchase costs; owned gear and potion totals must agree with the campaign and receipts. Invalid IDs, stacks, duplicate gear, incompatible slots or inconsistent ownership/stock reject the whole load. An active patrol with recorded kills is also rejected: the first kill must advance that quest to Return. Kills before quest acceptance remain valid. Camera, jump, target, panels, feedback, unfinished anchor channels and practice rounds are transient.
- Versions 1–3 migrate into the new inventory, retaining potions, rewards and the selected sword. Earned ward armor initially equips and the tunic moves into the bag. For versions 1/2, the third raider becomes the captain at the same health fraction; completed patrols unlock the second quest. Existing kills and hand-ins supply XP. Current player health is retained within the earned maximum.
- Version 4 retains its exact four worn slots and bag cells; new head/off-hand slots start empty and stock is untouched. Actors from versions 1–4 standing inside the newly added counter move to its nearest free edge. Other blocked or invalid positions still reject. Current version-5 saves cannot place actors inside that solid prop.

## Validation and limits

`CombatTests` covers delayed impact, recovery, dash avoidance, committed-facing misses, interruption, captain tuning, saved windups, and invalid-save rejection. `ProgressionTests` plays both quests and checks levels, channel cancellation, unique rewards, migration, respawn and restart. `InventoryTests` covers swaps, capacity, stale commands, unarmed damage and transactional loading. `TrainingTests` verifies armor, victory, timeout, arena exit, nonlethal damage, consumable costs and no farming. `TradeTests` covers charges, stock, unlock/range/death/jump rejection, actual fully armored hits, receipts, death recovery, reset and v4/v5 migration. All nine suites run through `-Test`.

Hidden captures cover spawn, raider windup, captain windup and a completed veteran at three sizes, with additional outfit and sparring views. They do not replace real keyboard/mouse playtesting or measure combat feel, performance or accessibility. There is no audio, skeletal animation, networking, repeatable campaign spawning or additional map.
