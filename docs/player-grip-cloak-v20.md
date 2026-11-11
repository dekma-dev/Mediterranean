# Sword grip, economical recovery and cloak inertia — v20

The current editable motion source is `assets/authored/player-motion-v20.blend`.
The existing six sword, bow and staff collections were refined in
`assets/authored/oldstyle-classes.blend`; the other 27 runtime equipment records
are unchanged. This pass changes presentation, not movement speed, jump physics,
damage contacts or cooldowns. Faces remain frozen.

## What changed

- Thirty-seven existing Blender actions have revised leg recovery or sword-arm
  motion. The sword travels down and behind the hand during locomotion. The
  diagonal cut and upper cut have distinct preparation and follow-through;
  the hand returns to combat guard after the cut.
- The native grip follows the forearm instead of fixing the palm to torso +X.
  A transported attachment frame and a regularized forearm projection allow
  pronation without collapsing the frame when the blade crosses the torso's
  lateral axis. Finger and weapon attachment use that same frame.
- Elbow transport now starts from an outward/downward reference. The previous
  reference placed its singularity directly above the shoulder, inside the
  sword's raised-arm range.
- The attack controller retains the incoming readiness through preparation.
  Previously, raising the underlying guard while entering the attack doubled
  the first stroke's acceleration. The source follow-through now completes
  before fading back toward guard. The authoritative 0.18/0.30-second contact
  clocks are unchanged.
- Walking and running use lower recovery arcs and a narrower travelling stance.
  The support phase, travelled distance, fixed bone lengths and world foot
  anchors remain responsible for ground contact. Adding polygons was not used
  as a substitute for changing these trajectories.
- The cloak has separate damped middle and hem responses, trailing motion after
  braking, sideways lag, twist and travelling folds. Its shoulder seam remains
  pinned. Color, normals and shadow use the same deformation. The denser cloth
  grid supports the curves, and the rendering bounds include their full range.

The cloak is a bounded modal presentation model, not a general cloth solver
with self-collision or body contact. Character playback still combines authored
Blender clips with native retargeting and foot placement; it is not motion
capture or motion matching. The first attack from relaxed carry remains compact
because the existing combat contact timing is preserved.

## Weapons

The changes refine the existing collections, retaining their equipment origins
and fitting. Swords have broader blade sections, fuller shading, small bevels,
peened fittings and stitched leather grips. Bow staves and their tip bindings
have more geometry. Staff shafts, sockets, ferrules and rolled seams are refined.
Metal, hide and wood retain distinct native surface categories; the six weapons
have matching Blender viewport materials.

| Existing collection | Previous triangles, near LOD | Current triangles, near LOD |
| --- | ---: | ---: |
| WarriorSword | 8,126 | 15,254 |
| WarriorVeteranSword | 7,650 | 14,776 |
| ScoutBow | 5,154 | 14,826 |
| ScoutVeteranBow | 5,154 | 14,826 |
| MageStaff | 7,560 | 13,504 |
| MageVeteranStaff | 9,512 | 15,456 |

All six retain a reduced distant LOD. KIT1 is 31,800,220 bytes, within its
32 MiB geometry budget. The garment weight archive is unchanged.

## References and interpretation

Local material was inspected as overviews and chronological frame sequences:
`D:\assets\what_we_have_now\01.mp4` (23.5–25.5 seconds),
`D:\assets\learning_material\character_creating.mp4` (34.5–35.7 seconds),
and the supplied travel/combat videos. The sampled raid segment at 19.5–21.5
seconds is obscured combat, not a clean running reference. No motion-capture
data or exact joint trajectories were extracted from these recordings.

The [Academy of Historical Fencing's grip explanation](https://swordfight.uk/hold-the-sword/)
distinguishes several grips and their uses. The carried sword here uses a
cutting grip with a relaxed elbow, rather than treating one rigid hand frame
as appropriate for every action.
[Ubisoft's motion-matching presentation](https://www.gdcvault.com/play/1023280/Motion-Matching-and-The-Road)
informs the attention to complete poses and transition continuity; its
motion-matching system is not implemented here.
[Ubisoft's cloth presentation](https://media.gdcvault.com/gdceurope2014/Presentations/828884_Alexis_Vaisse.pdf)
provides context for constraints, inertia and collision handling. The smaller
native cloak model above has explicitly narrower capabilities.

## Preservation

The audit compares the saved pre-pass files against the final source/export:

- Twenty frozen source assets, including both copies of the user's thirteen-model
  collection, face/body assets and garment weights, retain their hashes.
- All 57 rest bones and every protected Blender curve are unchanged, including
  torso, spine, shoulders and whole-head animation curves.
- Twenty complete PCL5 records, all 57 record headers and every channel outside
  the revised leg/right-arm channels are unchanged.
- Twenty-seven complete KIT1 equipment records and their two LODs are unchanged.
- Facial displacement, facial/scalp shading, combat contact constants and gait
  timing remain unchanged.

Authoring scripts, intermediate files and review captures remain in the ignored
`.blender-local/motion-v20-20261111` directory and remain ignored by Git.

## Review and checks

The local review is
[the native before/after comparison](../.blender-local/motion-v20-20261111/review.html).
It includes matched running, walking, jumping, cutting and cloak scenarios,
plus a separate close-up of the new grip, cloak turning/braking and actual
`Game::ExecuteCommand`/render playback. The close-up's camera differs from the
old capture and is therefore presented on its own.

The motion check measures a 0.410 m peak running ankle height, 0.239 m forward
running foot width and 0.116 m minimum lateral clearance. It preserves planted
contacts and fixed limb lengths. Carrying palm/forearm alignment is at least
0.854 in the tested standing/walking/running scenarios; the held blade stays
down/back. These numerical checks constrain defects; they do not establish
photorealism or replace visual review.

New regressions cover grip alignment, continuous roll, compact recovery and
cloak lag, settling, frame-rate agreement and reset. The previous half-metre
ankle-height assertion was replaced because it enforced the exaggerated lift
the user rejected. Existing weapon/wrist continuity limits were retained.

Detailed machine-readable records are stored locally in `preservation.json`,
`source-preservation.json`, `gear-validation.json` and `validation-summary.json`.

The final native pose passed all 12 selected regression suites in both Release
and Debug (24 suite runs). These include full native motion captures, hands,
foot placement, animation state, skinning, equipment, bodies, faces and authored
assets. The carrying-grip regression rejects a positive-axis roll clamp: that
clamp improved some raised poses but turned the relaxed palm across the forearm.
The final implementation regularizes only near axial alignment.

All six game executables were rebuilt. Their embedded PCL5, KIT1 and GWT2 data
match the current authored files byte for byte.
All six also passed the hidden native smoke launch with an isolated test save,
including device/shader initialization, resize, world/UI drawing and capture.
The review was recaptured after the final grip correction, including the
actual game update/render path; the short run video was encoded and decoded
at 1280x720, 30 fps.
