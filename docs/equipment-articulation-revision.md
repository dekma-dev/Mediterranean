# Equipment articulation and carried-weapon revision — 2026-11-05

This revision continues the accepted Warrior, Scout and Mage assets. It changes
six chest variants, three shoulder fittings and six weapons; it preserves the
other twelve KIT1 parts. The original thirteen-model collection, anatomical body,
faces, eyes and hair remain unchanged. It does not change combat timing, movement
speed, equipment slots, saves or the roadmap.

## Source and deformation

`assets/authored/oldstyle-classes.blend` is edited from the actual accepted source,
with hash guards before publication. Sleeves derive from the existing anatomical
arm topology and inherit its joint weights. Cloth ease and restrained elbow folds
lift them above the body. Connecting armhole strips interpolate between the chest
and arm weights; this repairs the visible gap found in the first native preview.
Upper fabric blends between chest and pelvis, while the veteran cuirass remains
rigid. Waist equipment follows the pelvis and the quiver follows the chest.

The source includes the editable native bind rig and vertex groups. The
[Blender armature documentation](https://docs.blender.org/manual/en/latest/modeling/modifiers/deform/armature.html)
describes the vertex-group deformation used here. Runtime uses the existing
linear four-weight palette; no dual-quaternion or physical cloth claim is made.

Retained shoulder caps are fitted closer to the arms. Existing sword blades gain
supported bevels along their fuller/edge profiles, closed pommels, rain guards and
small maker marks. Bows gain fitted leather grips and rolled edges; staves gain
crown sockets and iron ferrules. Grip origins and weapon lengths stay compatible
with the existing sockets. The local `3.jpg` reference remains the clothing guide.

GWT2 replaces the earlier three-part GWT1 resource. It explicitly identifies
twelve weighted equipment parts with two LODs each. Its decoder validates part
IDs, uniqueness, counts, joints, finite normalized weights and exact byte length.
Color, shadows and portraits reuse the owner's body palette. Unbound gallery
draws bypass skinning rather than reading an absent palette.

Current exports: **20,463,988 bytes** KIT1, **7,948,864 bytes** GWT2 and **538,435
triangles across all 27 parts and both LODs**. The bounded archive limits are
24 MiB and 10 MiB respectively. These are library totals, not per-character
counts. Fine faces and distant LODs remain shared immutable geometry.

## Movement

`assets/authored/player-motion-v6.blend` retains the 48-action, 57-bone source
library and revises 27 carrying actions: relaxed carry and forward, backward,
left and right walk/run for each of the three weapons. The loaded wrist trails
the stride, the free arm counterbalances, and the running sword is held higher
and farther forward. Double-frequency wrist bounce is reduced. Native tests
caught an excessively still lateral carry in the first draft; the final arcs
retain visible continuous weapon travel.

Only hand and weapon-direction channels change in those PCL2 records. Every
header and every native foot, hip and torso sample is byte-identical. The other
27 records, including attacks, archery releases and flight, remain byte-identical.
Source non-arm curves are retained. These are authored trajectories and IK, not
motion capture; native playback still uses the existing reduced body/finger rig.

## Validation

Local helpers, backups, logs and captures are ignored under
`.blender-local/equipment-20261104`. `preservation.json` verifies 36 frozen/source
files, twelve unchanged equipment records, all weight counts and source/export
hashes. The original local and tracked collection copies still match.

`OldStyleArtTests` covers both LODs of every chest variant, waist anchoring,
sleeve elevation, collar attachment and rays through both armholes. Existing
native captures exercise carried weapons, attacks, flight, equipment tiers,
shadows and portraits. `ClassArtDecoderTests` covers invalid GWT2 part IDs,
duplicates, malformed counts, joints, normalization and truncation. Existing
motion tests retain their floor-clearance, grip and continuity assertions.

During shared-workspace integration, a UI texture used t9, already occupied by
the body palette in the pixel shader. Moving that UI resource and its binding
to t13 resolves shader compilation while retaining both features.

Final published assets pass **13 Release suites and 8 Debug suites**, plus the
Debug upper-garment geometry, weight and nine-view capture checks. All six actual
game EXEs pass fixture-save smoke launches: Ninja Release/Debug, Visual Studio
Release/Debug and frontier native/debug. Scout and Mage receive additional
Release launches. PE inspection verifies current KIT1, GWT2 and PCL2 hashes in
every executable; body anatomy and face/scalp shader blocks remain unchanged.
No player saves are used. The hidden four-scenario 1080p workload also completes,
recording 178,260,108 immutable mesh bytes; this is a bounded diagnostic, not an
interactive FPS claim or isolated comparison against concurrent UI changes.

Open `build-ninja-release/MediterraneanLauncher.exe` for play. Local
`review.html` contains actual native equipment views and 20 Hz carry/attack
sequences, with the previous revision alongside. `tests-release-published.log`,
`tests-debug-published.log`, `runtime-verification.json`, `embedded-resources.json`
and `benchmark-1080p.txt` retain the final evidence. Python helpers stay ignored.

Visual fidelity remains a work in progress. Shoulder armor still uses rigid
attachments; the body rig is reduced and cloth uses skinning plus bounded lag.
This revision does not establish photographic realism or exact reference parity.
