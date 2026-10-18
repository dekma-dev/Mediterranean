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

The inventory has 24 bag cells and separate weapon/chest/legs/feet/head/off-hand slots. Starting gear is a sword, tunic, trousers and boots, plus two potions in the bag. Starting clothes have no armor bonus. C and I open the character and bag together: drag to swap cells or equip/remove compatible gear, right-click for an item action, or use the detail button. Split half moves half the selected potion stack (rounded down) into the first free cell. Sort compacts the bag and merges potion stacks up to 20. Drinking/selling targets the selected cell; Q uses the first potion stack. Unique gear cannot duplicate, incompatible actions change nothing, and rewards are granted only after the item fits. Permanent disposal remains unimplemented.

## Quartermaster and economy

The village quartermaster stands at `(3, -12)`, marked orange on the atlas/minimap. E opens the shop within 2.75 units and clear line of sight, while alive, grounded and outside practice. Nearby unclaimed loot takes priority. E or X closes trade; Escape opens the live menu. The shop captures movement/combat keys while time continues, and allows the bag and character window alongside it.

| Offer | Price | Initial stock | Rule / effect |
| --- | ---: | ---: | --- |
| Healing draught | 6 gold | 6 | Normal 40-HP potion; stored in the bag |
| Watchman's helm | 20 gold | 1 | Patrol handed in; Head; +1 protection while worn |
| Dawnwatch shield | 35 gold | 1 | Patrol handed in; Off hand; +2 protection while worn |

Buying validates range, life/grounded state, quest unlock, stock, gold and bag capacity in the simulation. Item, charge and stock update together; bought equipment remains stowed until equipped. Ward + helm + shield provide 5 protection, so an ordinary 8-point swing deals 3 and a captain's 16-point swing deals 11. This is passive protection; no active block or shield ability exists yet.

The campaign awards at most 90 gold from quests/loot. Buying both equipment items costs 55, leaving 35 for up to five draughts; buying all six draughts costs 36. Selling existing items can fund more purchases. Select an item in the bag and click Sell one; worn gear must be unequipped first. Gold, the source cell and merchant-held count change together. Sales and buyback require the same proximity/life/grounded eligibility as buying.

| Item | Sale / buyback price per item |
| --- | ---: |
| Frontier sword | 4 gold |
| Watchguard blade | 10 gold |
| Healing draught | 2 gold |
| Dawnwatch tunic | 3 gold |
| Traveller's trousers | 2 gold |
| Weathered boots | 2 gold |
| Astral ward armor | 15 gold |
| Watchman's helm | 10 gold |
| Dawnwatch shield | 17 gold |

The Buyback shelf groups sold items by type. Click to reclaim one for exactly its sale price, subject to gold and bag space. Items remain held indefinitely across leaving, saving/loading and respawn; they do not return to retail stock. Insufficient gold retains the item. Reclaimed gear starts in the bag, and selling rewards never removes earned quest XP. G switches only swords still owned; sold swords cannot equip. These are original prototype balance rules, not a reproduction of a live game's economy.

There is no repeatable income, retail refund or automatic restock. Buy/sell loses gold; sell/reclaim returns the same gold and item totals. R resets the expedition, buyback and stock. Saving after consuming a purchased potion retains the spent gold and depleted stock.

## Combat

Movement responds on the first input frame, then approaches 4 units/second walking or 7 sprinting with a short exponential ramp (95% of target speed in about 125 ms). Releasing input stops immediately; there is no physical braking slide. Changing sprint speed blends while moving. Diagonal input shares the same speed limit. Walking poses settle after release, turn over the shortest arc, and derive footstep phase from actual travel rather than elapsed global time. Jumping and dash do not generate grounded footsteps.

Enemies turn at up to 10 radians/second, accelerate toward a 2.2-unit/second pursuit speed and slow near their destination. Returning enemies turn before walking backward, traverse reached waypoints without a deliberate idle frame, and settle within 0.015 units of home. Separation moves each actor at most 1.5 units/second per pair, respects collision and never displaces a committed swing. An enemy must face the player before beginning its full telegraph; damage, range, cooldowns and interruption rules remain as listed below. These tuning values are original prototype decisions.

Unobstructed pursuit tracks the current player position each update. Being close but behind a tree/corner does not count as arriving in melee: the enemy must navigate to clear sight before stopping or starting a swing. Jump launch speed is 6 units/second under gravity of 18 units/second squared: a one-unit apex at one-third of a second and roughly two-thirds of a second airborne, independent of update rate. Contact is resolved within the update, and the landing pose uses the remaining time after impact rather than starting a frame late. Walking pose filtering is also independent of frame subdivision at constant travel speed. Visual gait and attack motion preserve velocity through starts/stops, and player turns accelerate and settle instead of moving at one fixed angular speed. Physical movement still stops immediately on input release; enemy facing stays aligned with its committed cone. Camera-relative keys follow the displayed, filtered view; opening a central panel or cancelling a gesture discards pending orbit. The boom waits for 100 ms of continuous clearance before easing outward to avoid bouncing at obstacle edges.

| Enemy | Health | Swing damage | Windup | Recovery after impact |
| --- | --- | --- | --- | --- |
| Raider | 80 | 8 | 0.7 seconds | 1.4 seconds |
| Captain | 120 | 16 | 1 second | 2.4 seconds |
| Sparring guard | 180 | 8 | 0.9 seconds | 1.4 seconds |

Enemies start winding up within 2 units and commit facing and position until impact. Their visible cone extends 2.4 units with a half-angle of 1 radian. Distance, direction, line of sight, and player jump height are checked again at impact. Movement and dash can avoid the swing; heavy strike interrupts an active windup and gives 0.8 seconds of recovery. Ordinary strike does not interrupt. Returning enemies cannot be targeted or damaged until they finish their home reset.

Player strike/heavy attacks require a living target within 2.5 units, line of sight, and jump height at most 1.5. Cooldowns are 0.9 / 4 seconds. Dash moves up to 3.5 units along facing, respects collision, and has a 5-second cooldown. Mend restores up to 35 health with a 10-second cooldown. Q consumes a potion for up to 40 health with a 3-second cooldown. Full-health healing consumes neither cooldown nor potion.

World text reports damage, healing, misses, interrupts, XP, and levels. The player flashes when hit. The captain has violet clothing, brass shoulders, and a peaked helmet. Art remains procedural placeholder geometry.

Combat effects distinguish a gold Strike crescent, a larger cyan Heavy Strike with a delayed violet counter-sweep, violet interrupt glints, and restrained orange enemy swings/hits. Blade ribbons follow the same joints as the rendered weapon; broader crescents mark the attack's origin and direction. Contact cores, expanding rings, tapered sparks and rising motes fade within half a second. Missed enemy swings show a sweep without hit sparks; rejected player commands show neither. Unarmed attacks retain impact feedback but no sword trail or crescent. Effects are cosmetic: current player damage still resolves immediately on an accepted command, enemy damage resolves at windup completion, and the larger crescents do not increase melee range or define collision. The live Escape menu does not freeze effects; focus-loss suspension still pauses local updates. Load/restart/respawn discard old effects, and defeat immediately cancels trails while the real final impact fades.

## Repeatable sparring

After completing both quests, press E within 2.5 units of the eastern training post at `(12, -2)` (cyan map marker). A dedicated guard appears and is automatically targeted. The 30-second round uses the same melee, telegraph, interrupt, armor and navigation rules as campaign combat. Worn ward armor reduces a normal hit from 8 to 6; adding the helm and shield reduces it to 3. The tunic provides no reduction.

Defeat the guard to win, press E to stop, or move more than 7 units from the post to leave. Reaching 1 HP ends the round without killing the player. The result displays damage dealt/taken for eight seconds. Practice grants no XP, gold, loot or campaign kill credit. Health, ability cooldowns and consumed potions are not refunded; Mend and potions work normally. Restarting practice cannot heal or farm rewards. Nearby campaign loot takes precedence over starting a round.

The opponent, timer and result are transient. Saving retains current health, resources and gear; loading ends the round. Campaign enemies remain defeated. This supports equipment/combat testing on the existing island without adding another map or unlimited progression.

## Death, restart, and saves

- Enter while defeated returns the player to Dawnwatch with full earned maximum health. Quests, XP, gold, potions, equipment, dead enemies, and loot claims persist. Living enemies recover at home. Cooldowns and transient presentation reset.
- Defeat closes the current central panel and cancels pointer gestures to reveal the recovery prompt. Enter remains available if the player reopens a menu, atlas or journal while defeated; it retains progress rather than restarting the expedition.
- The fatal hit cancels all enemy windups immediately. Remaining enemies and separation cannot advance after that hit. Their strike poses ease back to neutral while the encounter stays suspended; loading a defeated save cannot revive an attack pose or warning cone. Respawn resets surviving enemies at home.
- R clears the current session for a new expedition without erasing the on-disk save.
- F5 writes version 6; F9 validates before replacing state. Exact bag cells/split stacks, six worn slots, purchase counts, buyback counts, gold and committed campaign enemy windups/facing persist. Gold must equal earned rewards/loot minus purchase costs plus the sale value of items currently in buyback. Player-owned plus merchant-held unique gear must match campaign rewards/purchases; total remaining potions cannot exceed earned/purchased supply. Invalid IDs, stacks, duplicate gear, incompatible slots or inconsistent ownership/stock/credits reject the whole load. An active patrol with recorded kills is also rejected: the first kill must advance that quest to Return. Kills before quest acceptance remain valid. Camera, jump, target, panels, feedback, unfinished anchor channels and practice rounds are transient.
- Versions 1–3 migrate into the new inventory, retaining potions, rewards and the selected sword. Earned ward armor initially equips and the tunic moves into the bag. For versions 1/2, the third raider becomes the captain at the same health fraction; completed patrols unlock the second quest. Existing kills and hand-ins supply XP. Current player health is retained within the earned maximum.
- Version 4 retains its exact four worn slots and bag cells; new head/off-hand slots start empty and stock is untouched. Version 5 retains all six worn slots, bag placement and purchase counts. Versions 1–5 begin with empty buyback. Actors from versions 1–4 standing inside the newly added counter move to its nearest free edge. Other blocked or invalid positions still reject; versions 5/6 cannot place actors inside that solid prop.

## Validation and limits

Character presentation adds idle breathing, movement lean, airborne leg/arm poses, landing knee compression and brief recoil from real damage. These layers never delay an attack, move a hitbox or rotate a committed enemy cone. The character preview uses a neutral idle pose rather than replaying live combat. Air/landing/recoil history clears on load, restart and respawn; defeat clears reactions and breathing. The existing save format remains version 6. `AnimationTests` compares gameplay/save results against the same commands in a renderer-free Simulation and checks floor/weapon clearance, fixed limb lengths, 30/60/144-Hz jump transitions, reaction refresh and reset behavior. Transient hit events drive recoil independently of the fading damage tint; rejected abilities do not emit an event. Enemy flashes and reactions settle on the defeat screen. Landing timing/strength is checked at equal elapsed times across 30/60/144 Hz and uneven steps, and events cannot replay from saves.

`CombatTests` covers delayed impact, recovery, dash avoidance, committed-facing misses, interruption, captain tuning, saved windups, and invalid-save rejection. `ProgressionTests` plays both quests and checks levels, channel cancellation, unique rewards, migration, respawn and restart. `InventoryTests` covers swaps, splits/merges, capacity, stale commands, exact-stack healing, unarmed damage and transactional loading. `TrainingTests` verifies armor, victory, timeout, arena exit, nonlethal damage, consumable costs and no farming. `TradeTests` covers charges, stock, unlock/range/death/jump rejection, actual fully armored hits, receipts, sale/buyback cycles for every catalog item, forged credits, death recovery, reset and v4–v6 migration. All twelve suites run through `-Test`, including the newer motion, shadow and animation checks.

Hidden captures cover spawn, raider windup, captain windup and a completed veteran at three sizes, with additional outfit and sparring views. They do not replace real keyboard/mouse playtesting or measure combat feel, performance or accessibility. There is no audio, skeletal animation, networking, repeatable campaign spawning or additional map.
