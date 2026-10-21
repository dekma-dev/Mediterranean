# Dawnwatch gameplay rules

## Movement clip controller and skinned character (2026-10-21)

The final character pose now drives a ten-bone weighted body. The chest, pelvis, arms and legs share one connected surface, replacing the overlapping rigid limb segments. Head, hands, footwear, weapons and armor fittings remain attached components. World characters, enemies, NPCs and inventory portraits use the same body and pose conversion; both shadow cascades use the same deformation.

Eleven small code-authored motion clips cover forward/backward travel, left/right side steps, left/right turns, starting, stopping, ascent, descent and landing. Cubic interpolation preserves velocity through keys and loop boundaries. Travel distance drives stride phase; measured direction/speed and turning drive blended weights. Starts/stops and landing settle finish through short taps and takeoff instead of dropping nonzero offsets. The settle clock uses actual contact age within the frame. A repeat jump releases the already visible impact crouch over 120 ms; it discards the old impact event, and a buffered contact/takeoff cannot create a fresh airborne crouch. Existing timed Strike/Heavy, Mend, dash and hit-reaction layers retain priority and authoritative timing.

Clip motion is applied before the existing planted-foot and fixed-length hand/leg solves. The pelvis adjusts to contact reach, shoulders counter weight transfer, and equipment/weapon points follow the resulting pose. This is a procedural/code-authored animation pipeline, with a flat-floor foot solver; it does not add slope collision, root-motion displacement, a DCC importer, facial animation or cloth/body self-collision. Reset/load/death clear transient controller state; gameplay and save version 10 remain unchanged. Manual checks 157–158 cover live acceptance.

## Contact-driven reactions and weapon transitions (2026-10-21)

Player and enemy recoil consume the actual contact timestamps from the simulation. Each impact samples any previous recoil at its own contact time, then ages through the remaining frame. Multiple hits in one update retain the same reaction as separate updates; damage flashes and idle breathing stay aligned. Defeat and loading still clear transient reactions. Enemy hit tint also accounts for the remainder of the contact frame.

Timed weapon attacks blend into the current airborne, landing or dash weapon orientation at both boundaries. Returning to a fixed standing blade no longer produces a sword snap when the movement layer is active. Attack timing, damage, collision and the save format are unchanged.

## Weight transfer and takeoff presentation (2026-10-20)

Ground steps now shift the pelvis toward the supporting leg and counter-rotate the shoulders. Banking is damped, and both ankle contacts stay fixed while planted. A running jump retains its departing stride for a 120 ms blend into the air pose; contacts release immediately and the underlying flight remains ballistic. Descent opens the torso before landing compression. Reset/load discards support, takeoff and cloth momentum.

The cloak is one continuous folded surface with a pinned shoulder seam. Its free hem has a bounded spring response to speed, turning and flight; the shadow uses the identical shape. This is presentation-only cloth, without self-collision or forces applied to gameplay.

## Timed movement and attacks (2026-10-20)

Strike (1) commits at activation, contacts at 0.18 s and finishes recovery at 0.52 s. Heavy (2) contacts at 0.30 s and finishes at 0.76 s; interruption happens at contact. The original target and damage are captured at activation. Moving out of reach, losing line of sight, target death/return or player death can prevent the hit. Tab changes selection without transferring the strike. Other abilities cannot replace an unfinished attack; movement and potions remain available. Cooldowns start on acceptance and are not refunded for a missed or cancelled stroke. Pending attacks are transient: load, input suspension and captured menu controls cancel them.

Enemy wind-ups that expire in one update resolve in contact-time order. An earlier fatal hit prevents later strikes regardless of actor-vector order; equal-time enemy contact takes precedence over the player's contact. Swing/hurt effects and hurt flashes carry their elapsed time to the end of the update, including updates split by a player strike. Enemy recovery counts from impact rather than the next frame. Automated checks cover 22/30/60/144/1000 Hz, misses, fatal and nonfatal multiple contacts, and equal-time player/enemy hits.

Ground acceleration now approaches speed with a 16/s response; braking uses 24/s. With RMB camera steering, side steps use 85% speed and backward steps 65%, with continuous interpolation on diagonals. A directional jump retains its takeoff pace and momentum; pressing sprint in flight does not increase its speed limit. Ordinary jumps retain 7.2 m/s takeoff and 24 m/s squared gravity. The video ability leap is not mapped onto ordinary Space jumps.

This is an original single-player campaign on one island. Allods Online is the primary setting and presentation reference; these numbers are our prototype rules, not copied live-game balance. No external assets are required.

## Campaign

E beside the gold beacon keeper opens a dialogue on the left. Read the full story with Previous/Next; objectives and reward details stay visible on each page. Accept quest starts an offered assignment. Return after finishing its objectives, press E, then choose Complete quest to receive the listed reward. Progress reminders have no completion action; the completed page confirms delivery. The blade enters the bag, while the ward armor equips and returns the previous chest item to the bag. Both hand-ins restore health. Decline or Later leaves progress untouched.

Reading stops local movement, abilities and camera orbit while the world keeps updating. E/X closes the conversation; Escape switches to the normal live menu. Switching panels, losing focus, leaving the keeper's range, death, load and reset close the transient conversation. A stale button, incomplete quest, invalid keeper interaction or full reward bag cannot grant items, gold or XP. Saving stores quest progress, not the open dialogue or a pending click.

Authored dialogue is in [quests/](../quests/README.md): four plain text stages per quest, embedded by CMake when building. Titles, objectives and reward definitions share the same catalog used by the journal. Text changes require a rebuild and do not change existing saved progress.

| Stage | Objective | Reward |
| --- | --- | --- |
| Frontier patrol | Accept at the gold beacon, defeat any raider, return | 25 gold, 60 XP, watchguard blade |
| Secure the astral anchor | Accept after the patrol, defeat all three enemies, stabilize the northern violet crystal, return | 50 gold, 120 XP, equipped ward armor |

Each enemy grants 40 XP once. E claims its bag once for 5 gold and one potion. Kills before quest acceptance count. Finishing both quests and looting all enemies yields 300 XP and 90 gold. The objective tracker, interaction prompt, journal, and map identify the current step.

The anchor is at `(0, 12.8)`, in front of the northern ruin. E starts a three-second channel within 2.5 units after the camp is clear. Movement, jumping, abilities, potion use, equipment switching, or damage cancels it. Saving during the channel records the active quest without partial channel progress; loading requires starting again.

## Progression

Base progression before chosen talents:

| Level | Total XP threshold | Maximum health | Strike bonus | Heavy bonus |
| --- | --- | --- | --- | --- |
| 1 | 0 | 100 | 0 | 0 |
| 2 | 100 | 120 | 3 | 5 |
| 3 | 250 | 140 | 6 | 10 |

Level gains restore health to the new maximum. XP and level derive from persistent defeated-enemy and completed-quest flags, preventing duplicate XP on load or respawn. Level 3 is the campaign cap.

The starter sword deals 20 / 35 base damage; the watchguard blade deals 27 / 45; an empty weapon slot deals 6 / 10. Level and learned Arms bonuses apply to all three. G switches owned swords; before the reward is earned it re-equips the starter sword if the weapon slot is empty. Ward armor initially equips after the final quest and reduces incoming damage by 2 while worn, with a minimum of 1. Changing gear never changes earned XP or level.

The inventory has 24 bag cells and separate weapon/chest/legs/feet/head/off-hand/shoulders/hands/waist/back slots. Starting gear is a sword, tunic, trousers and boots, plus two potions in the bag. Starting clothes have no armor bonus. C and I open the character and bag together: drag to swap cells or equip/remove compatible gear, right-click for an item action, or use the detail button. Split half moves half the selected potion stack (rounded down) into the first free cell. Sort compacts the bag and merges potion stacks up to 20. Drinking/selling targets the selected cell; Q uses the first potion stack. Unique gear cannot duplicate, incompatible actions change nothing, and rewards are granted only after the item fits. Permanent disposal remains unimplemented.

New champions also wear patrol shoulder guards, ranger's gloves, a wayfarer's belt with two pouches and a short Dawnwatch cloak. These four travel items have separate Shoulders/Hands/Waist/Back slots, can be removed, traded and re-equipped, and add no damage reduction. The ten slots share the same inventory and save rules; the portrait and world use the same outfit.

## Champion build (N)

N opens the champion's build map and spellbook. Select Strike, Heavy Strike, Dash or Mend along the top to inspect actual power, cooldown, targeting and timing, then follow its related-talent links. Gain one talent point at level 2 and another at level 3; each upgrade costs one point. Select a talent and click Learn. Each branch has a foundation and two alternative level-three specializations; both alternatives require the foundation, not each other. With two points, finish one branch or combine foundations from two branches. Unspent points remain available.

| Branch | Level 2 foundation | Level 3 option A | Level 3 option B |
| --- | --- | --- | --- |
| Arms | Weapon training: +3 Strike and +5 Heavy damage | Weapon mastery: another +3 Strike and +5 Heavy | Executioner: Heavy deals 25% more damage against a victim at or below half health at contact |
| Guard | Vitality: +20 maximum health | Bulwark: +2 damage reduction, stacking with worn armor | Pursuit: future Dash cooldowns are 3.5 seconds instead of 5 |
| Spirit | Restoration: Mend heals up to 45 instead of 35 | Renewal: future Mend cooldowns are 8 seconds instead of 10 | Mercy: Mend heals 20 extra when pre-cast health is at or below 35% of maximum; up to 65 with Restoration |

These bonuses add to level and equipment stats. Vitality does not immediately heal; ordinary healing, level-up recovery and respawn respect the increased maximum. Bulwark retains the one-damage minimum. Renewal and Pursuit do not alter an already active cooldown. Executioner preserves the weapon damage committed at cast start and checks the victim's health at contact; ordinary Strike is unaffected. Mercy checks health before the cast and still caps healing at maximum. These are upgrades to keys 1–4, not extra action-bar slots.

Learning requires the champion to be alive, grounded, out of sparring/hostile pursuit and attack/hurt recovery, and not dashing or stabilizing the anchor. The simulation revalidates the point cost and requirements when a click arrives. Opening N stops movement and captures combat/camera controls while the world continues; inspection remains available when learning is blocked. N or the X button closes the map.

Reset talents opens a confirmation with a cancel option. Confirming refunds every spent point for free under the same out-of-combat/action requirements as learning. It never heals, resets cooldowns, changes gear or removes earned XP. Losing Vitality clamps health to the lower maximum; relearning it does not restore that loss. Closing the window, switching inspection, Escape, focus loss, load/reset or display/input suspension discards the pending confirmation.

Saved builds opens a second page with three persistent slots and six level-three specialization presets. Preview my active build also supports mixed foundations and partially spent builds. Select a slot, then choose a preset or the active build to preview; selecting an occupied slot previews its saved allocation, while an empty slot keeps the existing preview. The comparison uses the champion's current level and equipment, with conditional Executioner/Mercy effects explained separately.

Apply preview requires confirmation and the same combat/action restrictions as learning. It replaces the whole talent allocation atomically, refunds any unused points, cannot heal, clamps excess health and preserves all active cooldowns. Store requires a separate confirmation, including overwrites; it changes only the selected slot and can be used during combat because it does not alter the active character. Both operations enforce earned levels, points and prerequisites; presets remain inspectable when locked. Saved slots contain talents, not equipment, health or cooldown snapshots. Changing the preview, switching pages/panels, Escape, focus loss or input suspension cancels the pending confirmation.

F5 writes version 10, including the active `BUILD` and exactly three `LOADOUTS` entries (`-1` means unused; `0` means a saved allocation with no learned talents). Versions 1–9 start with empty slots; versions 1–7 also start with earned talent points unspent, while versions 8/9 retain learned choices. Version 8 permits only its original six talent IDs. Unknown bits, missing prerequisites, overspent points or malformed loadout records reject the complete load atomically. Loading and respawn cannot grant duplicate points; R starts a fresh expedition and clears both the active build and stored slots. Additional active spells and a higher level cap remain future work.

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
| Patrol shoulder guards | 3 gold |
| Ranger's gloves | 2 gold |
| Wayfarer's belt | 3 gold |
| Dawnwatch cloak | 4 gold |

The Buyback shelf groups sold items by type. Click to reclaim one for exactly its sale price, subject to gold and bag space. Items remain held indefinitely across leaving, saving/loading and respawn; they do not return to retail stock. Insufficient gold retains the item. Reclaimed gear starts in the bag, and selling rewards never removes earned quest XP. G switches only swords still owned; sold swords cannot equip. These are original prototype balance rules, not a reproduction of a live game's economy.

There is no repeatable income, retail refund or automatic restock. Buy/sell loses gold; sell/reclaim returns the same gold and item totals. R resets the expedition, buyback and stock. Saving after consuming a purchased potion retains the spent gold and depleted stock.

## Movement and combat

Holding RMB over the world steers the champion's body with the filtered camera heading even while standing still. Moving the mouse left/right turns the body; A/D strafe and S backs up without turning it away from the view. The existing turn spring and planted-foot correction provide the visible transition. Accepted strikes retain their heading during the 0.2-second attack flash, and an active dash retains its direction; steering resumes afterward. Releasing RMB restores ordinary travel-facing movement. UI/focus cancellation ends steering together with cursor capture.

Takeoff switches to airborne steering immediately. On landing, horizontal motion is split at the exact floor-contact time: momentum carries through the airborne part, then ground traction/braking acts for the remaining fraction of the update. Only distance traveled after contact advances walking steps and footstep sound timing. This avoids an extra frame of ground acceleration at takeoff or delayed braking on landing, including at low frame rates.

Movement responds on the first input frame, then approaches 4 units/second walking or 7 sprinting with a short exponential ramp (95% of target speed in about 125 ms). Releasing input applies short ground braking: about 0.125 units from walking or 0.219 from sprinting. Reversals brake existing velocity before accelerating back. Airborne release preserves horizontal momentum, with weaker steering (3/s response versus 24/s grounded). Central panels explicitly halt movement. Diagonal input shares the same speed limit. Walking poses settle after release and derive footstep phase from actual travel. Jumping and dash do not generate grounded footsteps.

Enemies turn at up to 10 radians/second, accelerate toward a 2.2-unit/second pursuit speed and slow near their destination. Returning enemies turn before walking backward, traverse reached waypoints without a deliberate idle frame, and settle within 0.015 units of home. Separation moves each actor at most 1.5 units/second per pair, respects collision and never displaces a committed swing. An enemy must face the player before beginning its full telegraph; damage, range, cooldowns and interruption rules remain as listed below. These tuning values are original prototype decisions.

Unobstructed pursuit tracks the current player position each update. Being close but behind a tree/corner does not count as arriving in melee: the enemy must navigate to clear sight before stopping or starting a swing. Jump launch speed is 6 units/second under gravity of 18 units/second squared: a one-unit apex at one-third of a second and roughly two-thirds of a second airborne, independent of update rate. Contact is resolved within the update, and the landing pose uses the remaining time after impact rather than starting a frame late. Walking pose filtering is also independent of frame subdivision at constant travel speed. Visual gait and attack motion preserve velocity through starts/stops, and player turns accelerate and settle instead of moving at one fixed angular speed. Ground movement brakes briefly on input release; enemy facing stays aligned with its committed cone. Camera-relative keys follow the displayed, filtered view; opening a central panel or cancelling a gesture discards pending orbit. The boom waits for 100 ms of continuous clearance before easing outward to avoid bouncing at obstacle edges.

| Enemy | Health | Swing damage | Windup | Recovery after impact |
| --- | --- | --- | --- | --- |
| Raider | 80 | 8 | 0.7 seconds | 1.4 seconds |
| Captain | 120 | 16 | 1 second | 2.4 seconds |
| Sparring guard | 180 | 8 | 0.9 seconds | 1.4 seconds |

Enemies start winding up within 2 units and commit facing and position until impact. Their visible cone extends 2.4 units with a half-angle of 1 radian. Distance, direction, line of sight, and player jump height are checked again at impact. Movement can avoid the swing; heavy strike interrupts an active windup and gives 0.8 seconds of recovery. Ordinary strike does not interrupt. Returning enemies cannot be targeted or damaged until they finish their home reset.

Player strike/heavy attacks require a living target within 2.5 units, line of sight, and jump height at most 1.5. Cooldowns are 0.9 / 4 seconds. Dash requires a selected living enemy within 5 units and line of sight. It approaches to 1.5 units from the target's centre over 0.34 seconds, travels at most 3.5 units, respects collision, and has a 5-second cooldown. Missing/dead/returning, too close, out-of-range and obstructed targets consume no cooldown or launch cue; rejected casts retain normal movement. Mend restores up to 35 health with a 10-second cooldown before Spirit talents (45 health and 8 seconds with both learned). Q consumes a potion for up to 40 health with a 3-second cooldown. Full-health healing consumes neither cooldown nor potion.

Spell 3 is a physical burst toward the accepted enemy: for a stationary target, speed rises smoothly to its peak at about 75 ms, then eases down through a longer braking phase. Every displacement goes through swept collision; significant obstruction ends the burst at the actual contact position. The body compresses at launch, leans into the drive and switches from the push leg to an opposite-foot catch. A normal finish settles out of its brace over 160 ms; an obstructed stop adds brief recoil. Settling is cosmetic and does not delay subsequent commands. Airborne dashes retain gravity. The accepted actor's identity stays locked across selection changes, while heading follows its current position. A closer target shortens the path; a target that escapes the remaining travel budget is not chased indefinitely. Losing that actor or its returning-home state ends the burst. Other abilities are unavailable until the burst ends without consuming their cooldowns on rejection. Potions retain their normal rules. The dash grants no invulnerability.

Launch, normal stop and obstruction have separate visual events. A short cyan wake with tapered pale cores follows the actual path; each ribbon segment uses its own travel direction and the chest arcs follow the current approach heading. Its history is sampled on a fixed clock, and open pressure arcs travel beside the chest. A grounded normal finish expands a ring at the feet; endpoint motes and ground-only grit fade quickly. Menu/focus cancellation clears the dash and its residue, while normal finishing residue fades quickly. These transient effects never enter the save.

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
- F5 writes version 10; F9 validates before replacing state. Learned talents, three loadout slots, exact bag cells/split stacks, ten worn slots, purchase counts, buyback counts, gold and committed campaign enemy windups/facing persist. Gold must equal earned rewards/loot minus purchase costs plus the sale value of items currently in buyback. Player-owned plus merchant-held unique gear must match starting equipment and campaign rewards/purchases; total remaining potions cannot exceed earned/purchased supply. Invalid IDs, stacks, duplicate gear, incompatible slots or inconsistent ownership/stock/credits reject the whole load. An active patrol with recorded kills is also rejected: the first kill must advance that quest to Return. Kills before quest acceptance remain valid. Camera, jump, target, panels, build previews/confirmations, feedback, unfinished anchor channels and practice rounds are transient.
- Versions 1–3 migrate into the new inventory, retaining potions, rewards and the selected sword. Earned ward armor initially equips and the tunic moves into the bag. For versions 1/2, the third raider becomes the captain at the same health fraction; completed patrols unlock the second quest. Existing kills and hand-ins supply XP. Current player health is retained within the earned maximum.
- Version 4 retains its exact four worn slots and bag cells; new head/off-hand slots start empty and stock is untouched. Versions 5/6 retain their six worn slots, bag placement and purchase counts; version 6 also retains its nine-item buyback ledger. Versions 1–6 receive each new travel piece once in its new slot. Versions 7–10 read all ten slots and thirteen buyback counts without regenerating removed/sold equipment. Versions 8–10 also validate the learned talent mask against earned level, point budget and branch prerequisites; version 8 permits only its original six IDs, while version 9 adds the three alternative specializations. Version 10 additionally validates every saved allocation against the same earned budget. Versions 1–9 begin with empty loadouts; versions 1–7 begin with no learned talents. Versions 1–5 begin with empty buyback. Actors from versions 1–4 standing inside the newly added counter move to its nearest free edge. Other blocked or invalid positions still reject; versions 5–10 cannot place actors inside that solid prop.

## Validation and limits

Character presentation adds idle breathing, movement lean, airborne leg/arm poses, landing knee compression and brief recoil from real damage. These layers never delay an attack, move a hitbox or rotate a committed enemy cone. The character preview uses a neutral idle pose rather than replaying live combat. Air/landing/recoil history clears on load, restart and respawn; defeat clears reactions and breathing. Animation state remains transient; equipment, buyback and learned talents are durable records. `AnimationTests` compares gameplay/save results against the same commands in a renderer-free Simulation and checks floor/weapon clearance, fixed limb lengths, 30/60/144-Hz jump transitions, reaction refresh and reset behavior. Transient hit events drive recoil independently of the fading damage tint; rejected abilities do not emit an event. Enemy flashes and reactions settle on the defeat screen. Landing timing/strength is checked at equal elapsed times across 30/60/144 Hz and uneven steps, and events cannot replay from saves.

`CombatTests` covers delayed impact, recovery, dash avoidance, committed-facing misses, interruption, captain tuning, saved windups, and invalid-save rejection. `ProgressionTests` plays both quests and checks levels, channel cancellation, unique rewards, migration, respawn and restart. `InventoryTests` covers swaps, splits/merges, capacity, stale commands, exact-stack healing, unarmed damage and transactional loading. `TrainingTests` verifies armor, victory, timeout, arena exit, nonlethal damage, consumable costs and no farming. `TradeTests` covers charges, stock, unlock/range/death/jump rejection, actual fully armored hits, receipts, sale/buyback cycles for every catalog item, forged credits, death recovery, reset and v4–v7 migration. All twelve suites run through `-Test`, including the newer motion, shadow and animation checks.

Hidden captures cover spawn, raider windup, captain windup and a completed veteran at three sizes, with additional outfit and sparring views. They do not replace real keyboard/mouse playtesting or measure combat feel, performance or accessibility. There is no audio, skeletal animation, networking, repeatable campaign spawning or additional map.

Grounded player/enemy motion now uses planted support feet with lifted steps, including turns and stopping. Arm swing follows the actual feet, and the pelvis adjusts without stretching the legs. Jumping releases ground contacts; landing and large movement discontinuities rebuild them. These are presentation changes on the existing flat floor: movement speed, collision, enemy windup facing, ability ranges and damage timing remain authoritative and unchanged. Feet do not navigate stairs or uneven terrain independently.

## Swept character collision

Player/enemy movement and dash use continuous circular sweeps against static obstacle faces, rounded corners and island edges. At contact, only the remaining inward component is removed, preserving tangent travel. A small clearance prevents rounding-related sticking; at most four contacts are resolved per move, with leftover movement dropped safely. This replaces the earlier X-before-Z substeps. Free movement speed, analytic gravity/landing, attack timing and save format are unchanged. Actors remain kinematic on a flat map; no rigid-body props, stair climbing or slope support is implied.
