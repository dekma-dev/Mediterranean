# Spell identity, editable action rows and combat chains

## Reference and scope

The local `allods_fight_interface.mp4` training frame at 00:20 was inspected
again alongside native before/after captures. Its action rows use distinct
painted silhouettes and color families; the earlier Mediterranean rows reused
weapon thumbnails for most attacks. This revision changes active spell art and
combat choices. It does not claim complete reference parity, new character
meshes, authored attack clips, a finished talent grid or final encounter balance.

The official [Allods mage description](https://allods.ru/about.php?show=class-wizard)
describes combining elemental control with damage. The official
[WoW mage overview](https://worldofwarcraft.blizzard.com/en-gb/game/classes/mage)
distinguishes fire, frost and arcane roles. Those are design references, not
imported damage tables or artwork. The local screenshot supplies layout and
readability evidence; no new full-video review is claimed.

## Visible artwork and action-row editing

Two new original atlases provide **98 distinct active-spell illustrations**
across all eleven classes, plus eleven common potion cells. All 109 used cells
have explicit identity/UV mappings. Each active spell uses the same picture in
the book, hotbar, tooltip, drag preview and existing character-creation previews.
Weapon/equipment icons and passive talent artwork remain separate.

The built-in imagegen tool generated the retained PNGs. The exact prompts and
cell order are in `assets/ui/spell-artwork.md`. PNGs are embedded in the native
executable, decoded once, mipmapped and rendered through the existing HUD batch.
The two decoded atlases add approximately 9.33 MiB. Total UI texture storage,
including the previous art, map and font, must remain below 30 MiB.

- Right-click an occupied action slot to remove its binding, including a
  cooling or currently unusable ability. This never casts or discards an item.
- Drag a bound icon into the world and release to remove it. A removal caption
  explains the pending action. Dropping on another window cancels removal.
- Drag between slots to swap; drag a spell from N to assign it again.
- Cancelling the gesture, changing panels, resizing or losing focus preserves
  the original binding. Releases outside the client do not remove bindings.
- New profiles no longer start with repeated copies of the same abilities.
  Existing layouts retain every binding and empty slot. New skills are in N;
  no migration silently rearranges the player's chosen action rows.
- HOTBARS 8 accepts versions 1–7 and persists empty slots and IDs 12–14.

## Nine additional abilities

The three focused classes now each have fourteen active abilities plus potion.
Existing ability IDs remain stable. New IDs are 12 (level 1), 13 (level 3), and
14 (level 4). Fresh-layout defaults are Ctrl+1, Ctrl+2 and Ctrl+3. Existing
characters can bind them from the fourteen-card spell rail in N.

| Class | Second basic / 0.9 s base cooldown | Link / 7 s | Finisher / 6 s |
|---|---|---|---|
| Warrior | Reverse cut: 85% basic damage, one Rage; successful alternation grants a 6-point ward | Crossguard: 55% heavy damage, 18-point ward, 3 s exposure | Crossing steel: 115% heavy damage; a two/three-step chain adds 40/60% heavy damage and a 24/36-point ward |
| Scout | Twin shot: paired physical arrows, one combined 85% basic contact, one Focus | Barbed arrow: 55% heavy damage, 2 s root, 4 s exposure | Crossfire: mobile three-arrow volley, 115% heavy damage; chain adds 40/60% heavy damage and refreshes Disengage |
| Mage | Ice needles: 85% basic damage, one Ember, 2.5 s slow | Arcane prism: mobile 55% heavy spell, one Ember | Thermal rupture: stationary 115% heavy cast; chain adds 40/60%; consuming slow/root adds another 35% heavy damage |

The second basic uses a basic-action clock and benefits from basic cooldown
talents. It can use the existing final-220-ms input buffer. Link attacks and
the Scout finisher remain mobile; moving before Mage finisher release cancels
the preparation. Actual contact/recovery clocks drive animation and effects.

## Chain rules and choices

A landed basic starts a five-second, target-identity-bound chain. Alternating
basic 0 and basic 12 adds a step, capped at three. After either basic, the new
link (13) or existing opening (7) also adds one step. Repeating one button does
not stack steps. The current target's step count and expiry are visible above
the action rows. Usable payoff buttons receive a pulsing outline that leaves
their icon artwork visible.

At least two steps can be consumed by existing heavy / technique / signature /
master / capstone attacks (IDs 1, 5, 6, 9, 11), adding 12% heavy damage per step.
The new finisher adds 20% per step. Warrior also gains six ward per step on an
existing payoff, or twelve per step on the new finisher. Existing class reserve
and target-control bonuses remain independent.

Examples using ability names, rather than assumed customized key bindings:

- Warrior: Strike → Reverse cut → Heavy strike for a short guarded payoff;
  Strike → Reverse cut → Crossguard → Crossing steel for exposure and a larger
  shield; alternate basic cuts into the existing Execution against a weak foe.
- Scout: Quick shot → Twin shot → Crossfire to recover the backward escape;
  Quick shot → Barbed arrow → Deadeye uses the root and reserve requirements;
  build a chain before the existing Piercing shot for a line attack.
- Mage: Ember → Ice needles → Winter lance combines chain and frost bonuses;
  Ember → Ice needles → Thermal rupture combines both elements; the slower
  Arcane prism route builds another step/resource, trading away some frost time.

Only a valid primary contact advances or spends the chain. Misses, protected
targets, cancelled preparations and secondary area victims cannot create or
duplicate rewards. Switching selection does not redirect a committed attack;
the chain is only shown on its own living, non-returning target. Contact-time
expiry includes sub-frame event timing. Defeat, respawn, new character and load
clear chains. Encounter replacement also clears the chain before reusing actor
IDs. Saving retains new cooldowns, not a banked unfinished combo. Existing
healthy-target/distance bonuses add to the chain reward instead of replacing it.

## Effects and persistence

New presentations include reverse/crossed steel contacts and directional
sparks; paired/fanned physical arrow traces and a barbed contact; icy needles,
violet prism traces and a two-color thermal helix with six curling impact
plumes. Continuous tapered ribbons replace the dotted short-segment plumes
found during native review. Preparation stays attached to the existing rendered
weapon/hand socket.
A successful chain adds small contact crescents using the accepted event's
step snapshot, never a speculative input cue. Fixed loops keep the combined
new projectile/contact review below 1,600 effect vertices. This supplements the
existing bounded effect pool; no new lights, textures for effects or simulation
writes are introduced by rendering.

Save version **23** adds the three cooldowns before the existing wilderness
record. Versions 1–22 migrate with zero cooldown for those previously nonexistent
abilities. Invalid values reject atomically. Existing quests, gear, classes,
population and progress remain in their existing records. Use rebuilt binaries
to open a version-23 save.

## Review and remaining acceptance

`SpellChainsTests` exercises real contacts at 30/60/144 Hz, payoff damage,
target identity, expiry, mobile/stationary rules, misses, cancellation, save
migration, class/level gates, unique atlas addresses and hotbar editing at
960×600, 1920×1080 and 2560×1080. It renders native book, HUD, preparation,
world-contact and isolated-effect captures. Evidence is under
`build-spell-chains` and the build directory's `spell-chain-review` folder.

Manual acceptance still includes full pulls/boss balance, sustained rotation
feel, small-icon recognition and moving-scene VFX readability. This revision
does not modify Blender source, frozen faces or authored motion files.
