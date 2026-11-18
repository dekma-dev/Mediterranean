# World interaction feedback

## Player-facing result

Supply recovery, court-seal breaking, anchor stabilization and beacon holding
share a framed progress display above the action dock. It names the action,
shows remaining time and keeps a short, fading completed/interrupted result.
Combat casts retain priority in that display; nameplate reservations follow
its actual transformed bounds. Evasion text yields that space while interaction
feedback is present, avoiding overlapping instructions.

The existing PvE reference sample at
`build-learning-review/pve2/frames/f000002.jpg` was inspected for the separation
between the right-side quest tracker and central immediate feedback. The new
display uses our existing brass skin, fonts and action-dock transform. This is
an interaction-readability improvement, not a claim of full reference parity.

## Authoritative state

`WorldInteraction.h` provides a read-only description of the actual channel.
The presentation does not run its own completion timer. Supply/seal bits,
anchor state and beacon wave transitions decide success. Cancellation retains
the last observed fraction and reports interruption; completion fills the bar.
Results last 1.2 seconds and fade over the final 0.35 seconds. Death/reset clear
the state, and loading a completed objective does not replay a success receipt.

All new player-facing text has explicit Russian translations. Spell timings,
quest rewards, movement/cancellation rules and saved-game format are unchanged.

## World response

The three southern sack stacks are submitted from live quest state instead of
being baked into immutable scenery. Successful recovery removes the goods;
the original barrel remains as the container. Reloading or turning in the
assignment does not restore the goods. Starting another character restores
the correct initial presentation without rebuilding authored assets.

During recovery, a small terrain-following segmented progress arc surrounds
the cache. It uses at most 64 existing effect ribbons (384 vertices), has no
particle allocation and disappears when the operation ends. No mesh, material,
Blender source, face or authored animation was edited.

## Verification

Final validation on 2026-11-19: InterfaceTests, CombatHudTests and SouthernTests
passed in both Ninja Release (44.58 seconds total) and Debug (162.67 seconds).
RPGNative built in both configurations and in the public Visual Studio Debug
output, `build-local/Debug/RPGNative.exe`. `git diff --check` passed.
The five native Release captures were inspected: the channel, interruption
and completion labels are visible above the action dock, and loading does not
replay a completion receipt. The close camera partly occludes the cache behind
the player; persistence is additionally checked against the simulation state.
The repository localization scan still reports ten existing engine diagnostic
strings; it reported no new gameplay/UI omissions from this change.

SouthernTests exercises actual start/cancel/complete/expiry/reset transitions,
site identity and supply persistence through turn-in, plus real anchor, seal
and beacon completion. It also generates native intact, channeling,
interrupted, completed and loaded supply views in
`build-ninja-release/southern-check/supply-*.png`. Guards are stunned only in
the presentation fixture to isolate the UI; production patrol/combat rules
remain active and are covered by the existing suite.

Remaining manual acceptance: inspect the display at the preferred UI scale
while enemies attack, cancel each interaction by movement/Esc, and check the
world goods before and after loading/turn-in. The short visual result can be
hidden by a higher-priority combat cast; authoritative progress and the usual
simulation message remain available.
