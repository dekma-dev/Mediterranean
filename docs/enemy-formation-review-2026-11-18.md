# Melee group recovery and formation

## Player-facing change

Ordinary pursuing Raiders, Reavers and non-boss Captains now use their attack
recovery to open a crowded front. They seek a nearby clear angle around the
player, keep facing the threat and move through the existing directional
locomotion. A lone opponent does not shuffle for decoration. This follows the
roadmap's cooperative hostile-group work without expanding class kits.

The supplied `pve_loc2.mp4` contact sheet was revisited at
`build-learning-review/pve2/sheet-0.jpg`. It remains a reference for readable
actor separation and contact staging, not evidence of the reference game's AI
algorithm. This implementation is our own local melee formation behavior; it
does not claim complete reference parity.

## Rules and integration

- Eight bounded local candidates trade neighbouring melee pressure against
  travel distance. A score margin avoids continuous equilibrium shuffling.
- Candidate paths require world clearance, target visibility, home-leash
  compliance, a swept player exclusion and clearance from living actors.
- Movement uses actual collision sweeps, acceleration and the snare multiplier.
  It cannot teleport, push a rooted ally or redirect a committed attack.
- The last 250 ms of recovery is reserved for settling; ready attacks take
  precedence. Existing damage, warning shapes and cast times are unchanged.
- Bosses, training actors, wildlife, ranged casters and protected court actors
  retain their existing behavior. Returning/dead/controlled actors do not flank.
- There are no persistent slot reservations or new save fields. Loading uses
  actual saved positions, then recomputes local choices from the living group.

Two integration issues were found and corrected during development: generic
chase steering could redirect a flank inward through the player; a blocked
preferred candidate could repeatedly stall behind a rooted ally. The flank now
uses non-steering sweeps and rejects occupied corridors before selection.

## Evidence and acceptance

Final validation: Release EnemyTacticsTests, CombatTests, ClassCombatTests and
SouthernTests pass (20.78 seconds); Debug EnemyTacticsTests, CombatTests and
ClassCombatTests pass (138.13 seconds). Ninja Debug/Release and the public
Visual Studio `build-local/Debug/RPGNative.exe` are rebuilt. Native review
captures were regenerated after the final player-clearance change.

`EnemyTacticsTests` covers formation expansion at 30/60/144/1000 Hz, bounded
travel, actor/world clearance, player clearance, actual follow-up attacks,
saved positions, roots/stuns/committed facing, snare response, corpses and
excluded roles. Existing CombatTests retains its rooted-neighbour approach
regression; its assertion was not weakened.

`EnemyTacticsReview` renders an in-memory native three-opponent encounter into
`build-enemy-tactics/`, with 13 frames and a per-frame combat trace. At one second
the initial two-metre-wide front opens to approximately 2.7 metres; attack
sectors subsequently remain committed. No player profile is written. Inspect
`frame-0.png`, `frame-60.png` and `frame-150.png` for initial arrangement,
recovery movement and committed warnings.

Manual acceptance remains: pull a real camp near a doorway and on a slope,
move through the group while kiting, root one enemy, and verify that the other
enemies can still approach without continuously circling or delaying attacks.
This is a tactical movement pass, not a new encounter, animation asset or
difficulty-balance sign-off.
