# Classes, character creation and the level-ten campaign

The local prototype now supports eleven playable class profiles, one saved hero per class, class-specific N-key builds and nine campaign assignments on the existing Dawnwatch island. The implementation uses the custom C++/DirectX 11 engine. No assets or dependencies were downloaded.

## Choose or create a hero

Start `MediterraneanLauncher.exe`, press Play, then choose a saved hero or follow **Create character → choose a class → Appearance & name → Create character → Enter World**. The class page shows all eleven callings, their roles, combat identities, four starting abilities and three N-key talent branches. Occupied slots cannot advance to creation.

On the appearance page, enter a 1–20 character Latin name (letters, spaces, apostrophes and hyphens; at least one letter, no leading/trailing space). Ctrl+A clears the field; Enter finishes editing. Caps Lock and Shift compose normally. Other Ctrl/Alt shortcuts leave the name unchanged; clipboard paste is not implemented. Trailing word separators keep the last valid preview name. Choose among five complexions, five hair colors and five tunic palettes, including the class default. The rotating live model updates immediately. Create character saves the profile and returns to the roster; Enter World is a separate action. Back keeps the draft, Cancel discards it.

There is one slot per class, not an unrestricted roster. Existing heroes cannot be renamed or overwritten from selection. F5 saves after entry; F9 loads that selected class's file and rejects a mismatched class. Only explicit Create character writes a new profile. It publishes a complete file without replacement, even if another process fills the slot during creation. Failed writes keep the draft and display an error. F5 now compares the on-disk bytes with the snapshot last entered, loaded or saved; a changed/removed file is rejected instead of overwritten. F9 explicitly loads the current same-class save and updates that baseline. Entry itself does not write a file. Invalid or externally changed saves retain the previous protection/review behavior. Gameplay stays frozen until entry, and held entry keys must be released before gameplay resumes.

Warrior retains `saves/patrol.save`. Other classes use `saves/hero-<id>.save`, with IDs in the table below. Saves are shared across local build configurations; avoid running different configurations against the same character simultaneously. R resets the selected expedition while retaining its class, name and appearance; only a later F5 replaces the disk save.

## Prototype class kits

Keys 1/2 are basic/heavy attacks; 3 is a targeted charge for melee classes and a backward escape for ranged classes; 4 heals the caster. Landed basics build each class's reserve, which empowers the next accepted heavy when full. Six ranged classes require stationary heavy preparation; Druid/Bard retain mobile heavies. All heavy attacks can interrupt an unfinished enemy wind-up. Actual damage includes equipped weapon, level and class talent bonuses. Ranged attacks retain wind-up, committed target identity, and contact-time range/line-of-sight checks; rejected attacks spend no cooldown. See [class combat disciplines](class-combat.md) for exact resources, timings, effects and lifecycle rules.

| ID | Class | N-key branches | Attack range | Additional identity |
| --- | --- | --- | --- | --- |
| 0 | Warrior | Arms / Guard / Spirit | 2.5 m | Baseline pressure and interrupt kit |
| 1 | Paladin | Judgement / Bastion / Devotion | 2.5 m | 120 starting HP; 4 innate damage reduction |
| 2 | Scout | Marksmanship / Survival / Medicine | 9 m | Heavy contact adds 8 damage beyond 4 m |
| 3 | Priest | Radiance / Faith / Grace | 8 m | 55 base self-heal; heavy contact heals the caster by 8 |
| 4 | Druid | Storm / Bark / Grove | 7 m | 2 innate protection; 48 base self-heal |
| 5 | Mage | Flame / Frost / Arcana | 10 m | Heavy contact adds 10 damage above 75% target HP |
| 6 | Necromancer | Blood / Bone / Soul | 8 m | Restores 15% of actual attack damage as health |
| 7 | Psionic | Will / Mirror / Clarity | 9 m | Successful heavy interruption delays the next attack by 1.5 s |
| 8 | Bard | Discord / Resolve / Harmony | 7 m | Each landed attack restores 3 health |
| 9 | Engineer | Ordnance / Plating / Repair | 8 m | Heavy contact adds 12 damage against captains |
| 10 | Demonologist | Ruin / Hide / Hunger | 2.5 m | Heavy contact sacrifices up to 5 HP, never fatally, for 10 extra damage |

The class catalog is `game/Classes.h`. Each class has nine named talents and four named actions. Talent amounts differ: Mage offense adds 4 basic/9 heavy damage per node, Paladin defense adds 30 health/3 protection, Priest healing adds 20 healing and reduces future healing cooldowns by 3 seconds. N displays the actual class values, prerequisites, live power and available points. The three saved allocations belong to the selected hero; specialization presets also use its class names.

The underlying talent topology is shared: three foundations and two specializations below each. Both specializations require the foundation; they are not mutually exclusive. Each node costs one point. Points arrive at levels 2, 3, 5, 7 and 9, for five total at level 10. Resets and loadout application retain the existing out-of-combat restrictions and never refill health or active cooldowns.

## Nine assignments on one island

The original patrol and astral-anchor quests remain intact. After handing in the anchor, collect any remaining original raider spoils and speak to the keeper again. The first dispatch stays disabled until those bags are collected, so replacing the squad cannot discard unclaimed loot. Seven sequential dispatches each introduce a fresh three-enemy squad on the northern patrol ground. Clear the squad, return to the keeper and explicitly complete the assignment. The next offer becomes available on the following conversation.

1. The broken watch
2. Ash on the road
3. Voices beyond the beacon
4. The iron tribute
5. A lantern in the mist
6. The last oath
7. Keeper of the frontier

Each new assignment awards 250 XP once. Their enemies do not provide additional gold, potions or kill XP; existing loot/merchant accounting stays bounded. The journal lists all nine assignments and the tracker/map point to the current objective. The new stories and rewards live in `game/Campaign.h`; the first two retain their authored text files in `quests/`.

The squads mix ash reavers, faster-windup veil hunters and captains. Rank adds 18 maximum health and 2 attack damage per chapter; reavers have slower, heavier blows. They reuse the existing enemy model and navigation ground. This is a sequence of encounters on the current map, not seven additional zones. Death recovery preserves completed assignments and already defeated squad members; sparring remains independent and grants no campaign credit.

| Level | Total XP | Earned talent points |
| --- | --- | --- |
| 1 | 0 | 0 |
| 2 | 100 | 1 |
| 3 | 250 | 2 |
| 4 | 500 | 2 |
| 5 | 750 | 3 |
| 6 | 1000 | 3 |
| 7 | 1250 | 4 |
| 8 | 1500 | 4 |
| 9 | 1750 | 5 |
| 10 | 2000 | 5 |

The complete campaign yields 2050 XP. Maximum health gains 20 per level above the class's starting health, plus talents; level-up restores health. Level 10 is the current cap. XP is derived from persistent progress rather than a repeatable reward counter.

## Class equipment

Each class uses the same two durable weapon tiers with a distinct visible form. Their original stats, ownership rules and prices remain intact. Item names and icons follow the hero in inventory, rewards, trade, gameplay and both previews. G cycles the tiers already owned; C/I supports the usual equip, remove, drag and inspection actions.

| Class | Starting weapon | Patrol reward | Form |
|---|---|---|---|
| Warrior | Frontier sword | Watchguard blade | Sword |
| Paladin | Dawnwatch mace | Dawn oath mace | Flanged mace |
| Scout | Trail handbow | Watchguard handbow | Hand crossbow |
| Priest | Dawn censer | Dawn reliquary | Crowned scepter |
| Druid | Grove staff | Living bough | Branch staff |
| Mage | Apprentice staff | Astral conduit | Crystal staff |
| Necromancer | Ash staff | Soul crook | Crooked staff |
| Psionic | Veil focus | Mind prism | Caged prism |
| Bard | Resonance baton | Chronicler baton | Conducting baton |
| Engineer | Brass pistol | Watchguard pistol | Pistol |
| Demonologist | Iron cleaver | Ember cleaver | Broad cleaver |

Scout/Engineer use weapon-aware aiming, a muzzle-origin straight trace and their own synthesized release cue. An empty weapon slot rejects keys 1/2 without spending cooldown. Unequipping/swapping during preparation cancels the shot while retaining the cooldown; doing so after release does not recall it. Contact still follows the existing accepted attack clock, target/range/line-of-sight rules and damage snapshot. These are initial physical weapon mechanics; there is no ammo count, independent ballistic collision or reload sequence. Main-hand staffs retain the existing shield compatibility. Bard's baton is not a musical-instrument simulation.

## Saves and validation

Save I/O is shared by the roster, creator and game in `game/ProfileStorage.cpp`: bounded 4 KiB reads, per-path nonblocking writer mutex, exclusive temporary files, flushed writes and publication after comparison. Mutexes coordinate updated builds; an unrelated editor that ignores them is not covered by a filesystem compare-and-swap guarantee. Failed writes preserve the draft and its retry button. The Windows contracts are documented in [WaitForSingleObject](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject) and [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw).

Save version 12 appends a validated `APPEARANCE` record (skin, hair, tunic palette indices). Versions 1–11 migrate to the original appearance; reset retains the chosen look. Save version 11 adds a `PROFILE` record containing class, quoted name, completed frontier count, active-dispatch flag and the sealed original-loot count. Enemy kinds/ranks derive from the chapter. Versions 1–10 migrate to Warrior / Frontier guard, retaining original progress, equipment, builds and loadouts. A v11 profile is validated before replacing live state. Invalid classes/names, inconsistent quest stages, impossible enemy state, repeat-loot corpses and overspent builds are rejected. Transient animations and unfinished player attacks remain unsaved.

`ClassCampaignTests` checks all eleven profiles, class combat/passives at 30/60/144/1000 Hz, real talent effects, cast/contact audio events, save round-trips, invalid-profile rejection, independent sparring, all seven added assignments through actual combat, level ten and duplicate-reward protection. Recovery is allowed in that campaign test; it is not a difficulty-balance certification. It also captures every class build plus ranged cast/recovery, atlas and journal. `ActionClipTests` checks clip endpoints, hand/weapon continuity, fixed limb lengths, casting sockets, airborne trace arrival, cancellation across four update rates, game integration and 44 class-pose GPU captures. `CharacterSelectionTests` checks all eleven save slots, naming, F5 routing, reopening, file safety and display/input transitions. `ClassEquipmentTests` covers both weapon tiers for all eleven classes, persistent equipment/trade accounting, physical-release rules, socket/ground/grip checks and 46 GPU world/action/inventory/roster/combat views. Existing motion, foot-placement, animation, skinning, hands, effects, audio and interface suites continue to run.

## Visual and animation scope

The launcher and shared in-game panels now use rubbed bronze borders, incised corner decoration and worn surfaces. Class selection, attack slots and offensive talents have class-specific emblems. The atlas has ragged darkened paper edges, folds, stains and fibres baked once into the existing 512-square map texture. Map coordinates and waypoint behavior are unchanged.

The connected weighted body and hands, eleven code-authored locomotion clips, blended starts/stops/turns/jump phases, timed attack/recovery layers and procedural foot/hand/balance corrections already exist and remain the movement foundation. Class action clips in `game/ActionClips.h` extend that controller with separate preparation, release and follow-through keys. Basic ranged attacks use a compact gesture; heavy attacks use the full class motif. Restoration gathers the motion inward. Warrior retains the established blade and Mend animation; Paladin and Demonologist add weighted torso/arm accents. Cancelled actions blend out over 160 ms without delaying combat, and load/death clears transient history. Final arm/leg solves retain fixed limb lengths and planted feet. Free wrists lift through a bounded forward arc to avoid shortest-rotation flips. Magic traces originate at the solved palm (outside an equipped shield) and arrive at ground-enemy chest height even during a jump. The character panel now uses the same class cloth color as the world and roster.

The classes still share humanoid anatomy, clothing and shield assets. Class weapon forms and their initial action/socket rules are implemented as described above. Richer class locomotion/clothing, ammunition/reload, musical instruments, pets, transformations, sloped-terrain foot solving and production character art remain future work. These playable kits do not reproduce the full mechanics or visual quality of a released MMO.

Reference review used the supplied recording's class dialog around 03:01 and parchment map around 00:44, plus the official [class guide](https://allods.ru/about.php?show=classguide) and [class list](https://allods.ru/classes.php). Blizzard's official [Mage](https://worldofwarcraft.blizzard.com/en-gb/game/classes/mage) and [Priest](https://worldofwarcraft.blizzard.com/en-gb/game/classes/priest) pages also informed the separation of ranged damage, protection and healing roles. Those are design references, not claims of mechanic parity. No reference artwork was copied into the project.
