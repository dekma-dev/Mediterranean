# Locomotion and surface revision — 2026-11-01

The user has frozen all existing faces. This restriction is recorded in
`AGENTS.md`. Head/scalp exports, facial source, anatomy source, eyes and expression
shading are preserved. The fifteen humanoid collection records are byte-identical;
animal head vertices are unchanged. Whole-body pose corrections remain allowed.

## Movement

The presentation IK used a 40 cm shin for a bind skeleton whose shin is 36 cm.
The mismatch stretched the lower leg and forced bent knees even while idle.
The solver now follows the bind length and permits a nearly straight resting
leg without a singular fully extended IK chain.

Previously every 1.57 m advanced a complete left/right cycle, and foot recovery
lasted only 75–180 ms. `GaitTiming.h` gives walking and running distinct stride
lengths and contact intervals. World-space foot anchors remain fixed through
support; quintic recovery, alternate initial foot release and bounded pelvis
compensation preserve smooth contact transitions. Boots articulate during recovery.
This changes presentation only: movement speed, combat timing and navigation are
unchanged.

Armed jumping now uses a separate sword-hand brace rather than the unarmed
balance gesture. Carrying blade orientation trails the hand's gait phase slightly.
Existing grip sockets, attack contact clocks, shield and class equipment paths
remain authoritative.

Reference: Daniel Holden's [Inverse Kinematics and Foot Locking](https://theorangeduck.com/page/inverse-kinematics-foot-locking)
and [Code vs Data Driven Displacement](https://theorangeduck.com/page/code-vs-data-driven-displacement).

## Surfaces and geometry

Eleven existing near meshes receive adaptive curved-edge refinement: wolf, boar,
oak, pine, dead tree, cottage, inn, tower, stump, storehouse and training hall.
New wolf vertices interpolate the original four-weight skinning influences;
animation samples and skeleton are preserved. The body groom adds 2,000 curved
guard-hair clumps to the wolf and 1,600 to the boar; head grooming is frozen.
Lower LOD topology is preserved,
apart from coincident material-boundary vertices. Runtime geometry stays within
the existing 80 MiB archive budget. `surface-detail.json` records preservation
checks and hashes; `collection01.json` records per-LOD counts.

Near triangles: wolf 38,708 → 67,700; boar 35,865 → 63,663; oak
77,184 → 90,090; pine 19,040 → 27,040; dead tree 23,978 → 34,954;
stump 11,692 → 21,692. Five building meshes also retain/refine more surface
geometry. The resulting collection archive is approximately 75.9 MiB.

Four surface sets now use Poly Haven photographic materials at native 2048-square
resolution: stone, wood, plaster and bark. Roof and animal coat retain locally
authored 1024-square fields. Each set supplies RGB, roughness and normal maps.
Bark and animal body coat have distinct material IDs, separate from wood and
frozen facial hair. The native DX11 renderer embeds the maps, generates mipmaps
and samples them in object space at measured material scale. The two six-layer
arrays use about 256 MiB with mipmaps; the two retained 1K layers are expanded at
load time to fit these uniform arrays, without adding source detail.

The original user collection and its local authoring copy are unchanged. The
derived `assets/authored/surface-detail.blend` provides inspectable refined meshes
and packed color/roughness maps. All authoring helpers and backups remain ignored
under `.blender-local/motion-surface-20261101`.

Ready-material references reviewed: [Bark Brown 01](https://polyhaven.com/a/bark_brown_01),
[Wood Planks Dirt](https://polyhaven.com/a/wood_planks_dirt),
[Rock Boulder Dry](https://polyhaven.com/a/rock_boulder_dry) and
[Painted Plaster Wall](https://polyhaven.com/a/painted_plaster_wall).
Poly Haven publishes these under [CC0](https://polyhaven.com/license).
The user approved downloading and using all four on 2026-11-01. The 12 original
2K JPG maps total 34,466,463 bytes; official MD5 and exact file sizes were checked
before packing. Runtime color PNGs preserve source diffuse RGB and put roughness
in alpha; normal PNGs normalize vectors and convert OpenGL green to the native
top-left-V convention. Imported maps are not downsampled. Source URLs, hashes,
authors and packing details are in `assets/materials/surfaces.json` and
`assets/materials/SOURCES.md`. Originals and conversion helpers remain ignored
under `.blender-local/vendor/polyhaven-2k` and `.blender-local/polyhaven-20261101`.
The derived Blender study contains refreshed packed color/roughness maps and
packed native normal reference images; the engine evaluates triplanar normals.

## Validation and launch

The photographic-material import passes the Release and Debug authored-asset,
geometry and face suites. All six game/launcher outputs are synchronized and
each actual game executable passes a fixture-save smoke launch with the final
photographic-color shader. The native review tool renders all 34 runtime entries. Import
validation verifies exact source RGB/roughness channels, unit normal vectors,
all 12 runtime map hashes and unchanged roof/fur maps. The source-preservation
check again confirms 33 frozen files, both original collections, the fifteen
humanoid records and byte-identical wolf animation samples.

The 2K four-workload 1080p diagnostic reports material/shadow storage of
278,762,796 bytes and 818.922 MiB ending working set on the RTX 4060. Its stationary
and village-walk GPU p95 observations are 4.401 and 7.035 ms (38/39 samples).
These short diagnostic samples are not an interactive FPS guarantee. Logs,
captures and hashes for this import are in `.blender-local/polyhaven-20261101`.

Nine targeted Release suites pass: authored assets, actor presentation, faces,
class art/weapon flight, body skinning, foot placement, authored locomotion,
motion and geometry. The final pelvis/restart adjustment was rechecked against
the five affected body/motion suites. Eight corresponding Debug suites pass.
Measured contact cadence is 2.0 steps/s at 1.4 m/s and 4.375 steps/s at 7 m/s;
the support-foot regression remains below its 2 mm slip bound.

All six advertised game/launcher outputs are synchronized: normal Ninja
Release/Debug, Visual Studio Release/Debug and both Frontier outputs. Each actual
game executable passes a smoke launch with a fixture save. User profiles were
not modified. Local logs, captures, preservation checks and executable hashes
are under `.blender-local/motion-surface-20261101`.

Before the photographic-material import, the 1080p diagnostic completed on the RTX 4060. Its stationary and
village-walking GPU p95 observations are 4.543 and 6.842 ms respectively; sample
counts are small (40/39), so these are diagnostic observations rather than an
interactive FPS guarantee. See `benchmark-1080p.txt` for all workloads and limits.

Normal launch: `build-ninja-release/MediterraneanLauncher.exe`.
Model browser: `build-ninja-release/MediterraneanAssetReview.exe`.
Local previews: `sword-carry-attack.gif` and `sword-flight.gif` in the log folder.
