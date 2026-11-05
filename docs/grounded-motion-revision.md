# Grounded motion revision — 2026-11-05

This revision addresses upper bodies rotating independently of stationary hips,
elbows flipping during grip retargeting, compressed weapon preparation, and feet
lagging behind a running actor. It changes presentation and authored motion;
combat contacts, movement speed and save data keep their existing contracts.

## Source and runtime

`assets/authored/player-motion-v8.blend` contains the existing 57-bone rig and 51
editable actions. Forty-three actions are revised: eight unarmed directional
gaits, twenty-four equipped directional gaits, six attacks/releases, three guards,
and the staff/bow carrying poses. Eight other source actions remain unchanged.
The original geometry, frozen faces, equipment, garment weights and thirteen
user collection models are preserved. Scripts and backups are local under
`.blender-local/grounded-motion-20261105`; authoring scripts are not tracked.

PCL3 retains 57 track identities, including six compatibility records. Its
662,544-byte archive is embedded as resource 303. Each frame has 30 float32
channels: the preceding 19 plus pelvis rotation, lateral/forward translation,
and two shoulder-relative elbow bend planes. The decoder rejects stale formats,
nonfinite values, malformed elbow/weapon directions, broken loops, invalid
timing, truncation and trailing bytes; its archive bound is 768 KiB. Fourteen
unrevised records retain their original 19 channels exactly.

The torso receives the source's global orientation once; the pelvis drives its
own skinning transform and hip sockets. The arms retain source elbow direction
within a stable anatomical hemisphere. Shared reach constraints keep both hands
on a staff or drawn bow. Carrying wrists are blended into attacks once, and the
source timing preserves acceleration and recovery instead of constant path speed.
Sword preparation uses a continuous cut rather than a rapid up/down reversal.
Bow source poses place the free hand beside the string before drawing it.

## Ground contact

The contact controller stores a fixed toe anchor while the heel rises. Boots
rotate around the same ankle frame as the solved feet. Swing trajectories are
relative to actor travel, with endpoint velocity compensation and retraction;
feed-forward travel removes the previous moving-target filter lag. Fast running
uses a longer cycle and higher foot recovery. Contact decisions remain fixed at
240 Hz and presentation does not alter collision or simulation movement.

## Review and evidence

The local review contains chronological native DirectX frames, real-time loops,
and sampled weapon/joint trajectories. Inspection covers idle, forward movement,
run, attack, recovery and equipped flight across warrior, archer and mage.
Automated checks cover planted toe slip, unchanged bone lengths, wrist/socket
attachment, bow string contact, elbow/weapon continuity, frame-rate consistency,
source preservation and simulation/save equivalence. Quantitative results and
build hashes are recorded beside the local review after validation.

References include the user's existing Allods videos, sampled locally, and
[Blizzard's combat-animation discussion](https://news.blizzard.com/en-gb/article/20946579/patch-7-3-new-combat-animations-its-magic),
which describes keeping gameplay timing while improving preparation and class
ready poses. The body/foot constraints here are our implementation, not imported
Blizzard animation. This is authored keyframe animation plus analytical contact
and reach constraints, not motion capture or a rigid-body character simulation.
The unchanged 0.09-second basic ranged release still limits how much preparation
can be shown from a completely unprepared pose. This pass focuses on the player
rig; creature/orc/skeleton animation sets have not been reauthored.
