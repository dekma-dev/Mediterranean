# Enemy encounters

## Design direction

Increase pressure through positioning, committed casts and interruption decisions. Damage must match the warning; difficulty should not come from invisible hits or attacks turning after commitment.

Reference: Blizzard's [Dungeon Journal design](https://worldofwarcraft.blizzard.com/en-gb/news/10021366/42-preview-the-dungeon-journal) exposes boss abilities and phases so players can form strategies. The [Allods combat guide](https://allods.my.games/en/game/game-guide/basics) establishes targeting, range and threat as basic combat requirements. These references inform readable encounter design; the mechanics below are original implementation choices, not a reproduction of either game's encounters.

## Implemented attacks

| Enemy | Attack | Counterplay |
| --- | --- | --- |
| Raider / initial captain / sparring guard | Existing committed melee cone | Move behind, retreat or interrupt |
| Ash Reaver | Reaping cleave: 3.2 m reach, 189-degree total arc, 1.15 s warning | Get behind or leave reach; interrupt |
| Veil Hunter | Veil lance: 7 m reach, narrow 18-degree total arc, 1.1 s warning; casts from up to 6.5 m | Sidestep the locked direction, interrupt, or break line of sight |
| Frontier captain, rank 1 onward | Boss: Oathbreaker; later ranks use Boss: Ashen Overlord. Doomquake combines a 2.4 m frontal cone with an omnidirectional 2.8–5.8 m annulus; 1.5 s warning | Enter the inner flank/rear, leave the outer ring or interrupt |
| Boss rotation, second attack | Iron Tempest: full 3.8 m circle, 1.5 s purple warning, immune to interrupts and stun during windup | Leave the circle; heavy attacks still damage the boss but do not cancel this cast |
| Boss rotation, third attack | Execution Line: 8 m reach, narrow 25-degree frontal cone, 1.5 s warning | Sidestep, get behind, or interrupt |

Bosses cycle Doomquake → Iron Tempest → Execution Line. Selection advances when a cast begins, so interrupting does not trap the boss on the same move. The committed attack remains fixed through health changes. Return-to-home and player recovery reset living bosses to the first move. There is no permanently safe inner flank across the rotation. All three attacks can begin within 6.5 m, but damage uses each attack's smaller/larger actual shape.

Frontier captains are promoted within existing quest encounters, not added as an unrelated arena. Boss health is `240 + rank * 32`; damage is `28 + rank * 2` before player mitigation. At 40% health, damage increases by 10 and post-attack recovery drops from 2.4 s to 1.56 s. Warning duration and geometry stay fixed even if the threshold is crossed during a cast. Ordinary enemy stats and current player defenses still apply. Initial tutorial captain and sparring remain unchanged.

Target cast bars name the ability and show counterplay; boss enrage is shown during casts. Ground outlines use the same reach/angle/ring constants as hit resolution. Veil Hunters use an empty-handed casting pose, violet lance release and a positional magic sound. Bosses gain shoulder armor/cloaks and a separate ring/ember impact with a heavier sound. Effects use existing bounded pools and shared assets.

## Persistence and limits

Accepted player damage produces a brief crimson crescent toward the source, relative to the current camera orbit. Multiple directions can coexist (at most six); repeated nearby directions refresh their existing cue. Cues expire after 0.85 s, hide behind open panels, never intercept clicks and reset with death/restored gameplay. Evaded or fully absorbed hits do not create a damage cue. Radial boss hit direction is derived from caster-to-player position, while warning cones and boss orientation retain the committed facing.

Pursuit and ranged retreat now use bounded local avoidance around living enemies (0.9 m centre spacing). Swept probes try forward and side directions while respecting static clearance and the actor's existing movement budget. Corpses do not obstruct. This is local steering, so crowded dead ends can still force an enemy to wait; it does not reserve formation slots or replace world pathfinding. Existing overlapping placements recover gradually, but roots, stuns and committed casts pin the affected actor: only a movable neighbour yields. Two pinned actors keep their positions until movement is permitted again.

Veil Hunters regain spacing during attack recovery when the player is closer than 4.5 m. They face the player while stepping backward or diagonally, at up to 1.55 m/s. Five bounded local escape probes respect world collision, line of sight and a 12 m home limit; a cornered caster waits rather than tunneling or teleporting. Roots stop retreat and snares reduce its speed. Recovery expiry ends retreat and permits another committed stationary cast, so melee characters can catch them. This introduces no new saved state or pathfinding allocations in the retreat branch.

The combat HUD also lists up to three nearby, unobstructed casts in earliest-release order, independently of the selected target. Each row shows the remaining time, interrupt color and positional counterplay. "Clear now" describes the current position only: moving into the warning before release can still cause damage. Click a row to select its caster; hover and selection have distinct highlights. Selection is routed through a command carrying the actor identity, revalidated for an active cast, 12 m range and line of sight. Expired or stale rows cannot select another actor by row index. Rendering itself does not change simulation state. The board hides while other panels are open.

Boss preparation and recovery add separate hip/shoulder/free-hand gestures for Doomquake, Tempest and Execution. These use the existing contact clock and cancellation blend; they do not change damage timing or replace the shared procedural rig with authored animation assets.

Save version 18 stores each enemy's committed and next rotation slots in an ENCOUNTERS record. Versions 1–17 migrate; a legacy pending boss cast remains Doomquake and advances to Tempest afterward. Invalid slots or boss-only rotation state on ordinary enemies are rejected atomically. Old saved health is retained, while encounter reset/new frontier spawns use the new maximum health. Completion, XP and rewards retain existing rules. Existing campaign automation can complete the encounters, but that is not evidence that every class/build is balanced for hardcore play.

Bosses now have a three-attack rotation and enrage, alongside ordinary enemy special attacks. Bespoke boss meshes, adds phases, distinct rotations per named boss and dungeon scripting remain open. Continue class-by-class difficulty tuning before raising damage further.

## Verification

Combat tests cover ring/front/rear boundaries, ranged acquisition, one release/one damage resolution at 30/60/144/1000 Hz, timed interruption and enrage recovery. Campaign checks cover progression and save round trips. Effects tests render warning/release captures and check bounded geometry, expiry and DirectX validation. Manual smoke check 261 covers live combat/readability acceptance.
