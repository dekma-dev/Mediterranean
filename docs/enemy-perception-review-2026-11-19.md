# Enemy sight memory and searching

## Gameplay

Ordinary enemies no longer navigate toward the player's current hidden position
through walls. While pursuing, an actual line-of-sight observation updates their
last known point. After losing sight, they approach that point, turn to search,
and return home after five seconds without reacquiring the player. Sight
reacquisition immediately restores ordinary pursuit.

This develops the roadmap's camp encounters and caravan avoidance through a
shared combat mechanic. It is not a claim that the reference games use the same
AI implementation or that their full behavior has been reproduced.

## Rules

- World solids and terrain use the existing authoritative visibility query.
  Hidden movement cannot update the search goal, facing or melee flank choice.
- Existing home leashes, committed attack warnings, damage and cooldowns remain.
  Bosses and training actors keep their encounter behavior.
- A search timeout cannot end an active root/stun early. Once control expires,
  normal return is allowed. Existing contact visibility checks still reject
  damage through cover.
- Ally-assisted acquisition already verifies visibility and seeds a real
  observation. Returning actors clear stale search memory.
- Perception is transient. Loading a hidden pursuit preserves actor position
  but starts a local search, rather than inventing the hidden player's location.
  The save format remains unchanged.
- Distant idle wildlife does not perform the additional pursuit visibility
  query; only active pursuit or detection-range candidates need that observation.

## Verification

The user-created `ee42641` commit appeared during this work and includes part
of the perception implementation. Final validation applies to the working tree,
including the subsequent visibility gating and strengthened tests. No commit
or push was performed by this session.

The real building fixture places the guard at (-11.8, -8), with an observation
at (-11.8, -7), then tests two different hidden player positions across the wall.
At 30/60/144 Hz, the two searches retain identical positions and facing, keep
the last observation fixed, do not damage the hidden player, reacquire a visible
target and eventually release pursuit. Tests also cover control lasting beyond
the search deadline and loading without fabricated perception.

The old tower navigation test explicitly seeds its known destination now:
loading no longer grants that knowledge. Its original arrival-distance and
world-clearance assertions remain unchanged. Actual observation and reacquisition
are independently tested by CombatTests.

Release SimulationTests, EnemyTacticsTests, CombatTests, PopulationTests and
SouthernTests pass (16.28 seconds combined). CombatTests passes again after
strengthening the loaded-search assertion. All five suites also pass on final
Debug sources (131.96 seconds). Ninja Debug/Release and the ordinary Visual
Studio `build-local/Debug/RPGNative.exe` builds succeed. Native review artifacts are in
`build-enemy-perception/`: `search.csv` and `search-0.png` through
`search-420.png`. The staged native scene verifies a real return transition at
frame 300 (five seconds at 60 Hz) and unchanged player health. The captured scene
starts with a known observation; the automated simulation test exercises its
actual acquisition.

Manual acceptance remains: break sight while kiting near doors and trees, hide
behind terrain, reappear before expiry, and test searches during a multi-enemy
caravan pull. Five-second tuning is not final encounter balance. Models, faces,
icons, class kits and authored animation assets were not changed.
