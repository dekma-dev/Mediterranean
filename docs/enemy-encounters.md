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

Frontier captains are promoted within existing quest encounters, not added as an unrelated arena. Boss health is `240 + rank * 32`; damage is `28 + rank * 2` before player mitigation. At 40% health, damage increases by 10 and post-attack recovery drops from 2.4 s to 1.56 s. Warning duration and geometry stay fixed even if the threshold is crossed during a cast. Ordinary enemy stats and current player defenses still apply. Initial tutorial captain and sparring remain unchanged.

Target cast bars name the ability and show counterplay; boss enrage is shown during casts. Ground outlines use the same reach/angle/ring constants as hit resolution. Veil Hunters use an empty-handed casting pose, violet lance release and a positional magic sound. Bosses gain shoulder armor/cloaks and a separate ring/ember impact with a heavier sound. Effects use existing bounded pools and shared assets.

## Persistence and limits

No new serialized enemy state or save-version change. Existing casts remain valid; old saved health is retained, while encounter reset/new frontier spawns use the new maximum health. Completion, XP and rewards retain existing rules. Existing campaign automation can complete the encounters, but that is not evidence that every class/build is balanced for hardcore play.

This pass provides three differentiated special attacks and a boss enrage, not full multi-ability boss rotations, bespoke boss meshes, adds phases or dungeon scripting. Future work should add tested encounter rotations and class-by-class difficulty tuning before raising damage further.

## Verification

Combat tests cover ring/front/rear boundaries, ranged acquisition, one release/one damage resolution at 30/60/144/1000 Hz, timed interruption and enrage recovery. Campaign checks cover progression and save round trips. Effects tests render warning/release captures and check bounded geometry, expiry and DirectX validation. Manual smoke check 261 covers live combat/readability acceptance.
