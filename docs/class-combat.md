# Class combat disciplines

Implemented on 2026-10-22 in the native C++/DirectX 11 game. This pass gives the existing eleven weapon forms and action clips different combat clocks, movement decisions and earned heavy-attack effects.

## Controls and rhythm

- **1:** a mobile basic attack. Each successful contact adds one point to the class reserve. Rejected, cancelled and missed attacks add nothing; the reserve cannot exceed its capacity.
- **2:** a heavy attack. A full reserve is spent when the attack is accepted, empowering its contact. An ordinary heavy remains usable without a full reserve. Missing or cancelling a committed empowered attack does not refund it.
- **3:** Warrior, Paladin and Demonologist approach the selected living enemy within 5 m, stopping at 1.5 m. Ranged classes escape backward, facing their selected living threat or their current heading when there is no target. Selection changes during escape do not redirect it. Travel is continuous and collision checked; walls stop it. Neither movement grants invulnerability.
- **4:** the existing class self-heal. **N** shows the actual contact/recovery times, movement rules, reserve effect and related talents.

Scout, Priest, Mage, Necromancer, Psionic and Engineer must stand still to prepare a heavy attack. Movement or jumping before release interrupts it and retains its cooldown/resource cost. A released shot continues to its committed target, subject to the existing contact-time range and line-of-sight checks. Druid and Bard retain mobile heavies. Every class retains mobile basics; melee attacks remain usable while moving.

| Class | Reserve capacity | Empowered heavy | Key 3 |
|---|---:|---|---|
| Warrior | 3 Rage | +24 damage | Targeted charge |
| Paladin | 3 Conviction | 24-point absorption barrier | Targeted charge |
| Scout | 3 Focus | +14 damage; 3 s snare at 45% pursuit speed | 3.8 m / 0.42 s retreat |
| Priest | 2 Faith | Heal 24; 12-point absorption barrier | 2.8 m / 0.46 s retreat |
| Druid | 3 Bloom | 2 s root; heal 18 | 3.3 m / 0.42 s retreat |
| Mage | 3 Embers | +36 damage | 4 m / 0.38 s retreat |
| Necromancer | 3 Souls | +12 damage; heal an additional 45% of actual damage | 2.8 m / 0.46 s retreat |
| Psionic | 3 Insight | 2 s stun, cancelling the current wind-up | 3.6 m / 0.40 s retreat |
| Bard | 4 Tempo | Heal 20; reset basic-attack cooldown | 3.2 m / 0.40 s retreat |
| Engineer | 3 Pressure | Expose the target for 5 s: subsequent hits deal +25% | 3 m / 0.44 s retreat |
| Demonologist | 3 Fury | +20 damage; heal 15 | Targeted charge |

These effects add to existing class passives and talent damage. Healing cannot exceed maximum health. A new barrier replaces a smaller barrier instead of stacking without a limit; it absorbs damage after armor. Root prevents navigation but permits attacks in reach. Stun prevents both. Snare slows pursuit through the existing acceleration system. Exposure begins after the applying hit. Effects expire according to simulation time and are cleared when an enemy returns home. The game continues behind the menu.

## Presentation

The combat HUD has class crests, named reserves and three instrument styles: melee seals, ammunition chambers, and curved arcane gems. Heavy slots highlight when empowered. Health text includes the remaining barrier, absorbed hits show `ABSORB`, and the target frame names control/exposure effects with remaining seconds (incoming attack warnings take priority).

Class attack rates advance the shared authored animation clock and authoritative contact together. Projectile flight uses the same rate conversion, so slower preparation does not detach the launch pose from impact. Retreat poses reverse the body's drive/bracing and leg sweep while keeping the champion's face toward the threat. Escape wakes and launch pulses use class colors and the actual travel direction.

Class selection explains the reserve payoff. Existing class weapons, icons, launch sounds, worn UI artwork and N-key talent trees remain in use. No downloaded assets, new engine, extra texture sets or Lua runtime are required.

## Save and lifecycle rules

Save version 12 is unchanged. Reserve points, barriers, enemy effects and active action/mobility history are transient: loading starts them empty, without refunding health, spent consumables or saved cooldowns. Death/reset clears player reserves and barriers; respawn clears surviving enemies' effects. Opening a panel/focus loss cancels committed action/movement through the existing input lifecycle, without refunding their costs. Ordinary key release retains smooth deceleration.

The implementation keeps `AttackSeconds()` in authored-clip seconds; `Spell().contact` and `.recovery` are real simulation seconds. `ClassAttackRate()` is the conversion boundary. Effect timers split enemy updates at expiry rather than rounding their duration to the current refresh rate.

## Design references and limits

The official [Allods class guide](https://allods.ru/about.php?show=classguide) and [class overview](https://allods.ru/classes.php) describe class identity through resources, weapon roles, protection and control. Blizzard's official [Warrior](https://worldofwarcraft.blizzard.com/en-us/game/classes/warrior) and [Hunter](https://worldofwarcraft.blizzard.com/en-us/game/classes/hunter) pages provide further references for melee pressure, resource use and ranged positioning. The prototype uses those general principles with original, bounded single-player kits; it does not copy their full resource systems or balance.

This is a common basic-to-heavy reserve model with distinct payoffs, not eleven full MMO rotations. Pets, transformations, ammo/reload, dedicated instruments, class-specific locomotion libraries and clothing silhouettes, group support targeting and production character art remain separate work. Sustained playtesting is needed to tune difficulty across all eleven kits.

`ClassCombatTests` checks class contacts at 30/60/144/1000 Hz, continuous bounded retreats, collision, target gating, cast interruption/release, actual reserve effects, absorption, control expiry and lifecycle resets. `ActionClipTests` validates the rate-scaled release against game animation. `ClassEquipmentTests` additionally captures all eleven resource HUDs; the existing campaign, interface, rendering and save tests remain applicable. See manual checks 196–198 in `tests/manual-smoke.md` for subjective movement/readability acceptance.
