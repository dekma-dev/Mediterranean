# Model and gameplay animation binding

## Code-only integration, 2026-11-01

This pass changes runtime controllers and palette composition. It does not
edit or regenerate Blender sources, mesh archives, skin weights, materials or
animation clips. The DirectX 11 renderer continues to use the accepted models.

### Actor identity and lifecycle

`game/ActorPresentation.h` owns each enemy's motion, clip state, movement history,
face, feet, creature controller and defeat pose together. Binding uses entity ID,
kind, rank and spawn location. A campaign insertion or reorder preserves surviving
actors and initializes only new identities. A resurrection clears the revived
actor's stale hit, attack, stride and corpse state. Teleports reset that actor's
motion history and support immediately. Stable rosters do not allocate
every update. Roster changes no longer reset the player's animation, camera,
combat effects or audio generation.

Loading an accepted windup reconstructs its attack weight before the first draw.
Defeat captures the actual outgoing gesture, gait and idle clock, including the
wolf's jaw and terrain support. Rendering a second view cannot advance animation.
Complete world/profile resets retain their explicit reset behavior.

### Wolves and boars

`game/BeastLocomotion.h` observes successful navigation distance. Frostfang's
walk/trot/gallop blending uses the existing clip distances and durations through
`engine/WolfAnimation.cpp`; it no longer uses the humanoid 7 m/s travel scale.
Distance advances the phase using the blended stride length. Backward travel
reverses phase, starts/stops ease the blend, and standing still stops phase
advancement. Boars retain their procedural rig with a separate stride scale.

Four height samples around the feet provide a damped support plane: forward and
lateral tilt and a small root-height correction. Limits are 0.38/0.30 radians
and 0.18 m. These presentation transforms do not move colliders, change movement
rules or rotate attack cones. They are root support, not four-leg terrain IK;
individual paws can still slip or intersect uneven ground.

Hit recoil and banking now reach beast trunks. Wolf bite opening follows the
authoritative windup/contact/recovery clock in Update, with damped cancellation.
The accepted muzzle mixes head and jaw weights while its nose follows the head.
The additive jaw angle is limited to 0.10 radians to avoid separating those
surfaces. A wider anatomical gape requires future artist work on the rig/weights;
this task deliberately leaves those assets alone. Color and shadow passes use
the same joint and normal palettes.

### Friendly residents

All fifteen additional residents now smoothly attend to a nearby living player
and acknowledge their own dialogue. Procedural characters use their existing
facial channels. The authored Guardian and Mage acknowledge with a small chest
turn/nod, because their current runtime rigs do not have independent facial
bones. Their collision heading and lower body remain fixed. Attention fades out
when the player leaves or dies and resets with the world.

### Review and evidence

`ActorPresentationTests` checks identity reorder/insertion, respawn, saved
windups, allocation stability, 22/30/60/144/240/1000 Hz stride consistency,
backward travel, slope orientation, bounded recoil/jaw motion, frozen defeat
clocks, all residents and render idempotence. It captures native wolf/boar
motion/contact/slope views in `<build>/model-binding-check`.

Build logs, test results and the asset hash baseline are under
`build-frontier-expansion/model-binding`. For an actual play check, approach
the farm boars southwest of the outpost or timber-road wolves southeast; make
them pursue, turn, wind up, receive a hit, die and respawn. Visit each resident
from front and side. Check normal movement, stop, slope travel, saving/loading
during an attack, and campaign transitions while the player is moving.

Imported humanoids still use their existing presentation rig; this change does
not import new full-body attacks or replace the customizable player with another
model. New species clips, full quadruped foot IK, stronger equipment retargeting
and manual animation/art acceptance remain separate work.


Validation result: 56/56 full Release suites passed. The final normal Release
build passed all four focused actor/asset/motion/population suites, and Visual
Studio Debug passed six focused suites. Both `build-ninja-release/RPGNative.exe`
and `build-local/Debug/RPGNative.exe` passed isolated native smoke launches;
their launchers also build. The 23 original tracked `assets/authored` files
match their starting SHA-256 values. Concurrent head/hair/face work in the shared
workspace is separate and was preserved. Manual live-animation acceptance is
still pending; native captures do not establish final animation quality.
