# Stable camera and dual-button control

## Requested behavior

The camera keeps its selected orbit distance when a tree, trunk, roof, wall or
awning hides the champion. Removed those prop tests from the simulation camera
query, including the special canopy/crown cases. Terrain clearance still prevents
an upward-looking camera going underground. Wheel zoom, filtered yaw/pitch and
the existing jump-height response remain. This is stable distance through scenery,
not a camera fixed in world space and not automatic scenery transparency.

## Input and gameplay fixes

- LMB begins free look over the world. It changes the view without turning the
  planted champion or redirecting forward movement. Travel keeps its heading on
  release; RMB reconnects movement and body steering to the view.
- Native raw mouse capture accepts a button mask. Both accepted buttons share the
  original pinned cursor position. Releasing one keeps the other active; the last
  release frees the cursor. UI-origin presses cannot become capture owners later.
- A world actor press is pending until a stationary release. More than four raw
  motion units mark a drag; a drag or RMB participation discards the pending pick.
  Target selection still checks actor validity when the command commits.
- An active world gesture cannot also click hotbar/inventory controls. UI-origin
  drags stay UI gestures. Menu/panel transitions, focus/capture loss, loading,
  fullscreen/resize and character-selection transitions discard camera ownership
  and pending selection; held buttons need a fresh press afterward.
- Continuous camera movement requires an already accepted button in both input
  samples. Starting the other button after moving a held UI gesture cannot cause
  an inherited camera jump. Nonfinite synthetic/raw deltas are discarded.
- Updated the Russian/English HUD hint and current play instructions. The authored
  models, faces, save format, spell rules and previous pressure-wave work remain.

## Validation

`CameraControlsTests` covers 30/60/144 Hz independent look and movement, both button
handoff orders, blocked-button ownership, fresh-capture delta isolation, menu,
display, focus and load cancellation, invalid deltas, travel beside a tree at a
fixed distance and wheel zoom. Its hidden native renderer projects the existing
training dummy, tests stationary selection versus an orbit drag with another
target already selected, checks bag-origin input, and captures the final view.
The initial spawn fixture did not obtain a selectable distant dummy. The native
fixture was moved to the training area to exercise a nearby, visible body;
the selection assertion was kept.

Existing simulation/navigation checks now assert the new prop-occlusion contract;
terrain-floor checks remain. Motion, interface, combat HUD and character-selection
regressions cover the surrounding input and rendering paths.

Final validation on the updated sources:

- Eight suites pass in Ninja Release (77.33 seconds) and Visual Studio Debug
  (248.02 seconds): `CharacterSelectionTests`, `MotionTests`,
  `CameraControlsTests`, `InterfaceTests`, `CombatHudTests`,
  `CombatHudWorldTests`, `SimulationTests`, `NavigationTests`.
- Both public `RPGNative` targets build; the Debug game and camera/motion tests
  were relinked after the final fresh-gesture motion fix.
- Russian coverage passes: 2,136 source occurrences, 1,985 catalog entries,
  no untranslated player prose. `git diff --check` passes.
- No downloads, model/source-export changes, commits or pushes.

Local native evidence: `build-ninja-release/camera-controls-review/left-orbit.png`
and the corresponding Visual Studio Debug test directory. The view deliberately
allows a foreground house to hide the character without retracting into it.
The hidden suites do not synthesize physical desktop input: real cursor pinning,
screen-edge motion, Alt+Tab and OS capture handoffs still need manual play acceptance.
