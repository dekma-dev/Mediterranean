# Walking guidance for the province

## Visible change

The atlas and round minimap now draw a dotted route from the player to the
current objective. Placing a map pin overrides that destination; clearing it
returns to the current quest or tracked commission. The quest tracker shows
a camera-relative next-leg arrow and remaining walking distance. Arrival and
unreachable destinations have explicit states instead of a misleading direct
distance. The atlas destination card uses the same route and distance.

The reference inspected for map presentation was
`D:/assets/learning_material/map2.jpg`: small warm dotted connections over
illustrated parchment. The existing sampled PvE frame
`build-learning-review/pve2/frames/f000002.jpg` was also reviewed for the compact
right-side quest/minimap grouping. This implements local walking guidance in
our existing atlas; it does not reproduce the reference's astral transport
system or claim identical artwork.

## Mechanics and boundaries

- Guidance uses the existing WorldMap navigation with 0.45 m actor clearance.
  It does not move the actor, issue commands, change quest selection or grant
  progress. It avoids static scenery, not enemies; a route is not a safe route.
- Displayed distance sums terrain-sampled segments, including hill height.
  The arrow points to the next reachable corner, rather than through a wall
  toward the endpoint. Both mouse orbit modes are reflected through camera yaw.
- Static destinations retain their route while walking. Updates are bounded
  to four per second; a changed destination or pin mode updates immediately.
  Search is repeated for a blocked next segment, a large relocation or a
  changed endpoint, not on every frame or while standing still.
- Passed corners are removed only after a clearance test. Arrival requires
  proximity within 1.6 m and a clear final segment. Solid objective centres
  use a free approach with visibility to the near face, inside the existing
  interaction radius. These approaches require arrival within 0.35 m so the
  player can actually interact. Explicit pins on blocked ground are not
  silently relocated. There is no auto-interaction.
- Death clears guidance. The existing load/character reset clears route state;
  the next frame derives a fresh route from the live objective. Routes are
  transient and introduce no save-format or settings changes.
- Route dots are bounded to 1,024 per map and clipped to the round minimap or
  atlas sheet. They render below actor/quest markers. Models, icons, class kits,
  source assets and existing staged changes are untouched.

## Verification

Validation on 2026-11-21:

- Release: InterfaceTests, NavigationTests, RouteGuidanceTests and SouthernTests
  passed (41.78 seconds). After the final small-pin-change correction,
  InterfaceTests, RouteGuidanceTests and SouthernTests passed again (27.11 seconds).
- Visual Studio Debug: InterfaceTests, RouteGuidanceTests and SouthernTests
  passed (84.91 seconds); the strengthened RouteGuidanceTests passed again
  after the final correction (5.44 seconds test time).
- Final game binaries rebuilt in `build-ninja-release/RPGNative.exe` and
  `build-local/Debug/RPGNative.exe`. Native caravan atlas/HUD captures and the
  960x600 atlas capture were visually inspected. The dotted path, Russian
  distance labels and next-leg cue are visible without covering the action bar.
- `git diff --check` passed. The localization scan lists only the ten existing
  engine diagnostic omissions, with no new route text omissions.
- Existing staged `.gitignore` edits are preserved. No commit or push.

`RouteGuidanceTests` covers actual building detours, clearance of every returned
segment, province destinations, terrain distance, stationary search reuse,
arrival, blocked endpoints, disabling, invalid coordinates and save/load reset.
It also drives the real Simulation movement from the village to caravaner
Tessa using guidance corners at 30 and 144 Hz, without teleporting or altering
collision. Existing NavigationTests remain intact as an independent regression
suite for the underlying pathfinder.

SouthernTests generates `southern-check/navigation-caravan-atlas.png` and
`southern-check/navigation-caravan-hud.png` in the build directory. Existing
InterfaceTests cover atlas picking and layouts at multiple resolutions.
SouthernTests additionally walks all three supply legs, performs actual timed
recovery, returns to Tessa and turns in the assignment at 30/144 Hz. This
scenario stuns the guards to isolate routing; live patrol windows remain in
the separate existing patrol test. Only the initial veteran fixture is placed
at the quest giver; subsequent travel uses Simulation movement commands.

Manual acceptance remains: walk the supply route while patrols are active,
place/clear pins on the atlas, rotate with LMB/RMB, complete a cache and verify
that the route follows the next objective. Dynamic hostile avoidance and
road-preferred route weighting are not implemented by this pass.
