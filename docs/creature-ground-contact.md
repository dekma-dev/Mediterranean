# Creature ground contact and species presentation

## Runtime integration — 2026-11-01

The 36 existing combat wolves and boars now share four independent paw/hoof
contacts. This is a code-only layer over the accepted models. It does not rewrite
Blender files, meshes, skin weights, facial data or animation clips. It adds no
new population, map dimensions or quests.

`game/QuadrupedContacts.h` samples each existing leg socket after the authored
clip and body support transform. A stance holds its world-space anchor; a swing
keeps the source motion and compensates for the local ground height. Turning at
rest lifts and replaces one foot at a time. A teleport, respawn or distant
presentation reset discards old anchors without creating contact sounds.

`engine/QuadrupedRig.h` applies bounded corrections to the existing joint palette.
Frostfang uses its upper/lower leg chains, preserving segment lengths and the
distal lever orientation. Thornback's accepted rig has one bone per leg: its
correction is a limited rotation and reach adjustment, **not anatomical knee IK**.
The boar's stance follows the backward part of its actual sinusoidal leg swing;
a small additional lift clears the returning hoof. Both species retain the
existing terrain-supported trunk. Contact adjustments run before corpse collapse
so a captured standing foot target cannot pull a falling leg back to the ground.

Idle animals gain small sniff/look motions. Wolves also move their existing ears
and tail. These beats ease away during an accepted attack or pursuit. Windup
weight shifts and the existing hit response follow gameplay state; none of these
presentation layers changes collision, damage, rewards or saves. Faces of the
player and NPCs are outside this change.

## Contact feedback and limits

Each new moving stance produces one presentation event used by both sound and
ground effects. Spawn, standing still and a forced anchor correction do not
invent steps. Detailed contacts run within 40 m of the player; the nearest 16
events within 18 m are retained per update. Audio selects at most three of these
within 12 m, behind combat/player events. The existing twelve-voice mixer and
distance attenuation remain in force.

There are eight separate ground cues: wolf paw and boar hoof on grass, earth,
stone and wood. Wolf preparation/bite and boar preparation/rush have four further
species cues. These are short procedural sounds, not recorded animal audio.
Their incremental mono PCM budget is 112 KiB, alongside the prior 4 MiB budget.
Music and existing player/spell sounds retain their data.

The slow 0.65 m/s habitat walk remains above the contact sound threshold for
both species. Boar steps are audible during roaming as well as pursuit.

Soft ground retains up to 64 fading prints for 2.8 seconds: a split hoof or a paw
pad with toes. Paving and timber do not retain prints. Fast contacts can throw
two small soil flecks. Marks use a depth-tested normal-alpha pass, before additive
spell particles, so they darken earth instead of glowing. They consume at most
1,536 vertices within the existing shared 6,144 effect-vertex budget and one
additional batched draw. No persistent terrain texture or asset download is used.

## Acceptance

`CreatureMotionTests` exercises independent limb targets, connected wolf joints,
bounded unreachable targets, rough slopes at several headings, contact timing
at 22/30/60/144/1000 Hz, resets, idle/combat transitions, bounded event/print pools
and spatial audio selection. Native captures live in `<build>/creature-motion-check`.
The pixel comparison specifically checks that terrain marks darken the surface.
`AudioTests` validates deterministic audible PCM, endpoints, headroom and the
explicit incremental memory budget for all twelve new cues.

Integration review also exposed a player regression after the concurrent
anatomical shin-length correction: fallback/preview poses could pull the ankle
up during breathing or landing because only world-contact poses fitted the
pelvis to reachable targets. The reach fit now applies to both feet in every
pose, with a small extension margin that retains the nearly straight idle knee.
The existing animation and foot-placement regressions exercise both constraints.

Validation: the initial full Release run completed 58 suites, with 57 passing
and the grounded-ankle regression above failing. After correcting reach and its
idle extension margin, all eight affected suites pass (actor presentation,
character art, player body, audio, creature motion, foot placement, animation
and motion). The final normal Ninja Release also passes both creature/audio
suites; Visual Studio Debug passes creature/audio/foot-placement checks. Both
advertised `RPGNative.exe` outputs pass isolated hidden startup, fullscreen,
resize and world/UI capture checks. Logs and captures are under
`build-creature-motion`; `final-native-tests.log`, `final-release-tests.log` and
`final-debug-tests.log` contain the final runs.

The measured Release presentation workload for 36 nearby creatures was about
0.15 ms per update after resource warmup. This measures controller math only,
not whole-game frame time. Total generated resident audio is 4,252,550 bytes;
the twelve new cues contribute 104,958 bytes.

Concurrent movement/material work in the shared directory was preserved. Its
resource declaration, collection metadata/archive, art-revision metadata and
wolf weight archive changed independently of this runtime pass. The original tracked Blender sources
and existing facial assets retain their starting hashes. The material-count
sentinel was synchronized with the expanded material enum during integration.
No Blender exporter, commit or push was run for this task.

Manual acceptance still requires watching real pursuit, turns and attacks on
the six populated districts' terrain. Extreme slopes may exceed the bounded
reach correction, especially on the boar rig. This does not supply newly authored
species attack clips or finish production animation quality.
