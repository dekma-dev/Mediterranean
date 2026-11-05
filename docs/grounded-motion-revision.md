# Grounded motion revision — 2026-11-05

This revision addresses upper bodies rotating independently of stationary hips,
elbows flipping during grip retargeting, compressed weapon preparation, and feet
lagging behind a running actor. It changes presentation and authored motion;
combat contacts, movement speed and save data keep their existing contracts.

## Source and runtime

`assets/authored/player-motion-v8.blend` contains the existing 57-bone rig and 51
editable actions. Forty-four actions are revised: eight unarmed directional
gaits, twenty-four equipped directional gaits, six attacks/releases, three guards,
the staff/bow carrying poses, and the equipped bow jump. Seven other source actions remain unchanged.
The original geometry, frozen faces, equipment, garment weights and thirteen
user collection models are preserved. Scripts and backups are local under
`.blender-local/grounded-motion-20261105`; authoring scripts are not tracked.

PCL3 retains 57 track identities, including six compatibility records. Its
662,544-byte archive is embedded as resource 303. Each frame has 30 float32
channels: the preceding 19 plus pelvis rotation, lateral/forward translation,
and two shoulder-relative elbow bend planes. The decoder rejects stale formats,
nonfinite values, malformed elbow/weapon directions, broken loops, invalid
timing, truncation and trailing bytes; its archive bound is 768 KiB. Thirteen
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
The contact controller also retains pelvis recovery state: reach remains a hard
constraint, while rising out of compression follows a damped spring. This keeps
a stopping swing target from instantly lifting the torso. Rendering only reads
the resulting ceiling and never advances this state.

## Review and evidence

The local review contains chronological native DirectX frames, real-time loops,
and sampled weapon/joint trajectories. Inspection covers idle, forward movement,
run, attack, recovery and equipped flight across warrior, archer and mage.
Automated checks cover planted toe slip, unchanged bone lengths, wrist/socket
attachment, bow string contact, elbow/weapon continuity, frame-rate consistency,
source preservation and simulation/save equivalence. Quantitative results and
build hashes are recorded beside the local review after validation.

The final Debug run passed all six selected suites, including the complete
`AuthoredLocomotionTests` sequence. Release passed twelve grounded attack cases,
the final bow/carry/flight checks, and a fresh full native art capture. Earlier
Release regression coverage also passed all eight selected presentation, body,
face, equipment, contact and motion suites. All six game outputs were launched
with the fixture save; embedded PCL3, equipment and garment resources match the
current authored files in each executable.

Final diagnostics measured at 1 ms intervals show a maximum 4.04 mm bow nock
attachment error, 12.70 mm string-hand displacement per sample, and 13.25 mm
weapon-tip displacement per sample in the ready-attack checks (previously
20.73 mm and 29.31 mm respectively for the last two displacement measures).
Planted toe movement is below 0.001 mm in the controlled flat-ground test.
These are bounded regression measurements, not a claim of motion-capture
quality. The twelve attack cases also constrain elbow and hand continuity
from both relaxed and combat-ready poses.

The local `review.html` contains actual native captures at their original
timing. The first 0.5 seconds of each attack are captured at 60 Hz; recovery is
captured at 20 Hz and repeated without changing duration. Walk/run sequences
use 20 Hz and equipped flight 30 Hz. Six MP4s include the preceding version for
comparison. `validation-summary.json`, `preservation.json`,
`embedded-resources.json` and `runtime-verification.json` record the evidence.

References include the user's existing Allods videos, sampled locally, and
[Blizzard's combat-animation discussion](https://news.blizzard.com/en-gb/article/20946579/patch-7-3-new-combat-animations-its-magic),
which describes keeping gameplay timing while improving preparation and class
ready poses. The body/foot constraints here are our implementation, not imported
Blizzard animation. This is authored keyframe animation plus analytical contact
and reach constraints, not motion capture or a rigid-body character simulation.
The unchanged 0.09-second basic ranged release still limits how much preparation
can be shown from a completely unprepared pose. This pass focuses on the player
rig; creature/orc/skeleton animation sets have not been reauthored.
