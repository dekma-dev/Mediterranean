# Blender collection in the native renderer

## Fitted limb armor and equipped jumps (2026-11-05)

The class library now has 33 parts: six new fitted limb/cuff assemblies append
to the accepted 27 parts. Three equipped jump actions append to the 48 accepted
Blender actions, with `player-motion-v7.blend` as the current motion source.
Shield and palm orientation now follow the solved forearm. Native records,
existing source mesh objects, earlier actions, garment weights and all frozen
face/body/collection assets pass preservation checks. See
[class art](oldstyle-player-art.md#fitted-limb-equipment-and-armed-flight-2026-11-05)
for scope and `.blender-local/limbs-flight-20261105` for local evidence/review.
All six game/launcher outputs contain the new assets and corrected attachment
behavior. Final draw/release/carry/flight checks pass in Release and Debug;
fixture-save game launches and embedded-resource hashes match all six outputs.

## Upper garments and carried weapons (2026-11-05)

Six chest variants now include anatomical sleeves and connected armholes, with
chest/pelvis/arm weights in GWT2. Three shoulder fittings and six weapon meshes
are refined. The v6 source revises 27 carrying actions while preserving every
native foot, hip and torso sample and all attack/release tracks. Faces and the
original thirteen-model collection remain frozen. See
[equipment articulation](equipment-articulation-revision.md) for exact scope,
budgets, preservation and validation.
All six game/launcher outputs are synchronized. Final validation passes thirteen
Release suites, eight Debug suites, Debug garment checks and every actual game
smoke launch; embedded resource hashes match the published assets.

## Class tailoring and authored strike revision (2026-11-02)

The three focused classes now use revised chest clothing, continuous weighted
hems and fitted pouches. The retained `oldstyle-classes.blend` includes the
pre-edit parts and an editable garment bind rig; the native KIT1 export gains
the separate bounded `class-garments.weights` resource. Four weapon actions are
revised in `player-motion-v5.blend` and exported into the existing PCL2 contract.
Fifty other animation records and fifteen other equipment parts remain
byte-identical. Frozen faces, both original collection copies, body anatomy,
wolf motion and environment exports are unchanged by this pass.

See [the tailoring and motion record](class-tailoring-motion-revision.md) for
reference evidence, source preservation, geometry/animation counts and review.
This is a character art integration; it does not complete reference parity or
the remaining creature/environment art work.
All six game/launcher outputs listed below contain this revision. Embedded
resource hashes match the current exports, and every actual game EXE passes a
fixture-save smoke launch. Thirteen Release and ten Debug art/motion/integration
suites pass; the revision record includes the resolved Debug test timeout.

## Frozen faces, locomotion and surface detail (2026-11-01)

Faces are frozen by user instruction in `AGENTS.md`. The current revision fixes
the shin/bind-length mismatch, short rapid foot cycles and airborne sword carry.
It refines eleven existing environment/creature exports and introduces separate
bark and body-coat materials. Four surface sets now use approved Poly Haven CC0
photographic maps at native 2K (stone, wood, plaster, bark); roof and coat retain
the original local 1K fields. See `assets/materials/SOURCES.md` for provenance.
The original thirteen-model Blender collection and local authoring copy remain
unchanged. The editable derived study is `assets/authored/surface-detail.blend`.

See [movement and surface revision](movement-surface-revision.md) for scope,
preservation checks, budgets and the researched animation/material references.
`surface-detail.json` and `collection01.json` describe the current runtime archive;
the face-revision hashes below describe the preceding face-only adoption.
This revision is synchronized into all six game/launcher outputs listed below;
each actual executable passes a fixture-save smoke launch. The normal Ninja
Release/Debug configurations also pass the targeted motion, asset and face suites.

## Game binary synchronization (2026-11-01)

The face revision below is now embedded in all six advertised game outputs:
Ninja Release/Debug, Visual Studio `build-local/Release` / `Debug`, and
`build-frontier-expansion/native` / `debug`. Each matching launcher is rebuilt.
This closes the gap where an older executable could still display intermediate
or original faces despite the source integration being complete. Existing player
profiles retain their class, appearance choices and equipment.

The six identities, thirty LOD exports and both collection source copies were
checked against their recorded hashes before rebuilding; no Blender source was
regenerated. Build logs, runtime checks and native captures for this synchronization
are local under `.blender-local/faces-integration-20261101`. For normal play, open
`build-ninja-release/MediterraneanLauncher.exe`.

Synchronization validation: four integration suites pass in each Visual Studio
configuration (authored assets, six face identities, character selection and
launcher); launcher checks pass in both normal Ninja configurations; face checks
pass in both frontier outputs. All six actual `RPGNative.exe` files also pass
smoke launches with fixture saves (farm district or mage). This is twelve targeted
suite passes plus six actual game launches. User profiles were not modified.

## Human face revision (2026-11-01)

The player head's previous neck fitting included the mandible and stretched it
downward, removing the chin/neck separation. The corrected export confines the
fit to the lower cervical socket. Six distinct anatomical heads now derive from
the already approved Blender Studio CC0 realistic male/female foundations. Their
editable source is `assets/authored/human-faces.blend`; provenance, regional edits
and thirty bounded LOD exports are in `assets/characters/human-faces.json`.

Named residents use stable face/complexion/hair combinations, including the target
portraits. The quartermaster, keeper and training opponent also differ from the
player. Existing player customization, equipment and saved appearance fields keep
their meaning. Skin coloration follows each face's actual lips and cheeks instead
of applying one shared beard/mouth mask to everybody. The native head/scalp shadow
receiver offset removes close-up self-shadow striping; caster geometry is unchanged.

Moonweaver and Ironjaw also receive corrected necks, distinct faces and fitted
scalps within the original collection. Only their six head-containing LOD records
change. The other **96 records remain byte-identical**. Fifteen obsolete overlays
(old pupils, eyelids, nostrils, lips and the mage's neck primitive) are hidden in
the editable source and excluded from export. The new head assemblies and their
ornaments sit 65 mm lower, with the lower neck fitted into the collar and the
orc's old upper neck trimmed inside the new socket. Both collection source copies
were compared before editing and now match SHA-256
`f9742477b9e3dd2cbfc966edc751e45e33d8a24ce162c26e1f09f507e2caeb0c`.
`assets/authored/collection-face-revision.json` records the before/after hashes;
`world-art-revision.json` points to the adopted revision. The older hashes below
describe the preceding art pass. Original anatomy, equipment sources and motion
archives remain preserved. Local authoring/export helpers and backups are under
`.blender-local/faces-20261101` and are not tracked.

Validation: `HumanFaceTests`, `CharacterArtTests`, `GeometryTests`,
`AttachmentLodTests`, `ShadowTests`, `AuthoredAssetTests`, `ClassEquipmentTests`,
`CharacterSelectionTests`, `PlayerBodyTests` and `PopulationTests` pass in both
Ninja Release and Debug. The face suite captures forty native views and checks
that blinking stays inside the ocular region, distinct identities contain
different geometry, and every chin projects in front of the neck socket. Actual
game smoke launches pass in the farm district (Release) and with the mage fixture
(Debug), using only test saves. Captures and logs are in the ignored directory
above; forty close-ups are also in `build-ninja-release/human-face-check`.
Use the refreshed `build-ninja-release/MediterraneanLauncher.exe` to review.

## Runtime integration verification (2026-11-01)

All 34 collection models have live world roles through the existing population,
building, foliage and actor render paths. The normal Ninja Release/Debug outputs
and their launchers are synchronized with the populated world; Visual Studio
Debug is also verified. The previously stale Ninja Debug output now includes
the same population and version-22 save support.

`AuthoredAssetTests`, `PopulationTests`, `CharacterSelectionTests` and
`LauncherTests` pass in both Ninja configurations. Native views cover all six
districts and their accessible NPC/creature routes. The actual Release game
passes smoke checks in the farm district and with the archer/mage fixtures;
Visual Studio Debug passes in the timber district. Only build-directory fixture
saves were used. Logs, world captures and the six-location contact sheet are
under `.blender-local/integration-20261101`.

Launch `build-ninja-release/MediterraneanLauncher.exe`. Nearby places to inspect
the props are Farmer Darya at (-27,-30), southwest of the starting outpost, and
Sawyer Boris at (29,-30), southeast. The accepted Blender sources and exports
retain the hashes recorded in `world-art-revision.json`.

## Gameplay population integration (2026-10-31)

The six new frontier districts place 186 instances of fifteen authored prop
types, twelve buildings and 105 additional trees. Wells, carts, benches, fences,
supplies, braziers and cache chests now have gameplay locations and shared solid
footprints where appropriate. Together with the existing stall, dummy, foliage
and building substitutions, all twenty-one additions have runtime world roles.

Frostfang and Thornback now represent 36 persistent combat creatures, with
species attacks, targeting, loot, commission objectives and distant respawn.
The old two-animal ambient preview remains only in its isolated review/test
path. Wolf bite/head and boar head-loading/death poses layer over the current
native rigs; no Blender source, exported clip or skin-weight regeneration was
performed for this integration. See [populated frontier](populated-frontier.md)
for exact counts, controls, persistence and remaining animation/art work.

## Current world art expansion (2026-10-31)

The native collection now contains **34 models / 102 LOD records**: the thirteen
user-created foundations and twenty-one new editable models. The first thirteen
asset IDs and world roles are preserved. The MDA1 archive is 73,900,472 bytes,
bounded to 80 MiB. Each model has three strictly decreasing detail levels.

New source: `assets/authored/world-details.blend`, with collection/root names and
export budgets in `world-details.sources.json`. The additions are a barrel,
crate, ironbound chest, handcart, masonry well, bench, lantern, brazier, anvil,
firewood stack, grain sacks, signpost, fence, practice dummy, fern, shrub, hollow
stump, memorial stone, provision stall, storehouse and training hall. The two
building variants derive from the accepted cottage and inn. Timber pieces use
profiled sections; the cart has spoke wheels and iron tires, the well has stone
courses, overlapping pitched shingles, rafters, a crank and bucket, and the
anvil has a shaped waist, heel and round horn. Native review exposed incorrectly
oriented boards; the final source corrects the crate ends, lids, cart bed, bench
seat, curved chest lid and well roof before export.

The live world uses the new storehouse, training hall, provision stall, lantern,
practice dummy, waypost, fern/shrub, supply barrels/sacks and expedition brazier
in existing presentation locations. Other additions are embedded, editable and
available in the asset reviewer; the later population integration above places
the remaining prop types on the map.
Building and stall silhouettes are constrained to the existing camera bounds.
No new interactions or collision rules are introduced by these substitutions.

The original models also received a geometry/material pass: connected anatomical
faces and fitted scalps for Moonweaver/Ironjaw, recessed skeleton eye sockets,
less bulbous armor and joints, cloth tension variation, creature contour edits,
bark grooves and recessed side windows on the cottage/inn. The wolf retains
the same vertex counts, skin weights and four animation clips; its source and
runtime surfaces receive the same contour field. Normals are transformed with
the surface and checked against triangle winding after float32 export.

| Original near model | Triangles |
| --- | ---: |
| Dawnwarden / Oathless | 29,968 / 29,967 |
| Moonweaver / Ironjaw | 51,372 each |
| Ash sentry | 28,000 |
| Frostfang / Thornback | 38,708 / 35,865 |
| Oak / Pine / Dead tree | 77,184 / 19,040 / 23,978 |
| Cottage / Inn / Tower | 38,392 / 38,184 / 38,000 |

`world-art-revision.json` records the adopted hashes. Both original collection
source copies now match SHA-256
`d1bcfb6f00405bfa4e8989387a042057005e7e6f5157594e8f318bd258961564`.
Sources were compared before editing and the preceding assets were preserved
under `.blender-local/world-art-20261031/before`. Authoring, export and review
helpers stay in that ignored directory. The new prop source remains separate
from the original thirteen-model collection. Compare both source copies and
the recorded hashes before further editing; do not run the older thirteen-asset
exporters over the expanded archive. Existing detail/refinement JSON files
describe earlier passes; `world-art-revision.json` is the current provenance.

Player equipment also has fitted coat waists and softer folds in the three class
kits. The body source, 54-track player motion archive and equipment identities
are preserved. See [oldstyle-player-art.md](oldstyle-player-art.md).

Validation of this expansion: eleven relevant Release suites and five Debug
suites pass across authored assets, class geometry/equipment, player body,
hands, attachment LOD, motion, character selection and launcher startup. Two
startup suites timed out while builds ran concurrently; isolated repeats passed
in 11.09 and 8.57 seconds. After the final prop corrections, the authored-asset
suite was rerun in both configurations and the reviewer captured all 34 models.
Release smoke checks pass with Warrior, Scout and Mage fixtures; Visual Studio
Debug smoke passes with Mage. Ninja Release, Ninja Debug and Visual Studio Debug
runtime binaries are rebuilt. No player saves were used for these checks.

The final stationary-spawn 1280x720 benchmark on RTX 4060 records CPU p50/p95
1.926/2.777 ms over 240 measured frames, 4,402,340 submitted triangles including
shadow passes, and 150,400,656 immutable mesh bytes. Working set was 532.5 MiB.
Only twelve GPU observations were available, so this is not a GPU-tail or
interactive-FPS guarantee. Logs, verification results, native world captures
and model contact sheets are under `.blender-local/world-art-20261031`.

The following dated sections retain earlier export history; their counts and
hashes are historical. These are editable game assets using the native material
approximation, not photogrammetry or final photorealistic art acceptance. No new
external assets were downloaded.

## Earlier detail and carrying revision (2026-10-31)

All thirteen accepted models retain their world roles. Near LODs preserve more
of their existing sculpted geometry. The cottage, inn and tower also have
profiled timber/slate/masonry edges and small individual wear; the boar's
continuous source surface has a lowered rooting muzzle, shorter ears, reshaped
withers and compact feet. A bounded subset of its existing groom is retained
as whole tapered strands (560 / 112 / 24 per LOD). Its lowered jaw receives
smooth head weights instead of being assigned to a foreleg by height.

| Near model | Triangles |
| --- | ---: |
| Dawnwarden / Moonweaver | 23,967 each |
| Ironjaw / Oathless / Ash sentry | 24,967 / 23,967 / 15,999 |
| Frostfang / Thornback | 38,708 / 35,865 |
| Oak / Pine / Dead tree | 77,184 / 19,040 / 21,982 |
| Cottage / Inn / Tower | 38,392 / 38,183 / 38,000 |

`collection01.masset` is 34,894,696 bytes, within a bounded 40 MiB archive.
Near records allow 40,000 triangles / 120,000 split vertices; the oak alone
allows 80,000 triangles to preserve whole leaf outlines. Middle/far limits
remain 20,000 triangles / 65,536 vertices. The latest export preserves all
26 distant records byte-for-byte. It keeps 4,200 complete two-sided oak leaves
with raised midribs and smooth normals, using a separate trunk budget instead
of collapsing foliage and trunk together. Three buildings preserve more of
the existing profiled edges. The wolf retains 1,600 weighted guard-hair clumps;
their shading follows the underlying coat. Its original body vertices,
distant weights and four action clips are unchanged. `collection01.detail.json`
records this additional export pass. The anatomical source remains the separate,
unchanged `frostfang.blend`.

Tracked and local collection sources were compared before editing, backed up
under `.blender-local/refinement-20261031/before`, then synchronized to SHA-256
`63b5fb93674707a51efa0547c8ba70eacf4eb8ec5e4679b7929546e72e84f66d`.
`assets/authored/collection01.refinement.json` records provenance. All new
authoring/export scripts remain local and ignored. Do not run the older
collection exporters over these adopted exports; preserve the anatomical wolf
override and compare source hashes before any regeneration.

The player now uses 26,028 body / 15,997 head triangles near the camera and the
54-track v4 weapon/action archive. See [oldstyle-player-art.md](oldstyle-player-art.md)
and [authored-player-locomotion.md](authored-player-locomotion.md). Class rules,
world collision, progression and saves are unchanged by this presentation work.

Native review is available through `build-ninja-release/MediterraneanAssetReview.exe`.
This is a detail/anatomy refinement of the accepted sources, not photogrammetry
or a claim that all models have reached final realistic art quality. Reference
inspection included the supplied Allods images and this
[wild-boar profile photograph](https://commons.wikimedia.org/wiki/File:Wildschwein_Sus_scrofa.jpg).
No new external assets were downloaded.

## Accepted environment refinement (2026-10-26)

The reviewed v2 candidate is synchronized to the tracked source and local authoring copy: SHA-256 `5338426256f932d6be3db4fb83132adc9cae08cf9038bf7cf1b0713ebbb5f30c`. Original sources/exports remain in `.blender-local/environment-backup-20261026`; both original source hashes were checked before replacement.

Existing pine attachments now carry irregular needle shoots instead of opaque conical boughs. Whole shoot groups thin before distant export: 4570/1648/728 triangles. Cottage, inn and tower use object-linked weathered timber/limewash/slate/stone materials. Archive record hashes confirm all seven actors, oak and dead tree are unchanged. The archive is 12,655,740 bytes.

The reproducible exporter is `scripts/export-authored-collection.py`, with `assets/authored/collection01.sources.json`. Run Blender in background on the tracked source with `--python scripts/export-authored-collection.py`. Building/running does not require Blender. The refinement script writes a separate candidate and refuses an already-refined collection; do not rerun it on the accepted source.

## Source ownership and future use

The user designated this collection as the foundation for future model work on 2026-10-26. Develop all thirteen models rather than making duplicate primitive substitutes. The tracked source and `.blender-local/collection-01/Mediterranean_Collection_01.blend` were byte-identical at inspection (SHA-256 `6ccab74fa14ea87f9760ec0ab4e92d8ea5e939fd844d71c0fbede1cbd83d0d93`). The adjacent `.blend1` is a prior backup. Recheck for newer user edits before exporting or running generation scripts. The local contact sheet is an earlier Blender preview, not proof of current engine appearance.

| Blender model | Native role |
| --- | --- |
| Dawnwarden | Guardian / Warden Elian |
| Moonweaver | Mage / Archivist Serevan |
| Ironjaw | Orc / captains and reavers |
| Oathless | Revenant / wraiths |
| Ash sentry | Skeleton / raiders |
| Frostfang | Wolf / decorative wildlife |
| Thornback | Boar / decorative wildlife |
| Elder oak | Broadleaf trees |
| Highland pine | Conifer trees |
| Blighted elder | Dead trees |
| Hearth cottage | Homesteads |
| Moon inn | Watch hall |
| Border watch | Northern ruin tower |

The existing player is not yet replaced by this collection. Future playable-character work should adapt these characters with authored skin weights, action clips and equipment attachment support while retaining customization and gameplay systems.

The first collection contains thirteen original static models. `assets/authored/collection01.blend` preserves the editable source; `collection01.masset` embeds three detail levels per model in each native client executable. Blender and Python are not required to build or run the game; the authoring scripts remain in the locally ignored `.blender-local/collection-01` directory.

All thirteen assets now appear in the game. Cottage/inn replace homestead/watch-hall visuals; oak, pine and dead trees replace trees; the northern ruin uses the tower within its existing bounds. Warden Elian uses the guardian and Archivist Serevan uses the mage. Raiders use the skeleton, captains/reavers the orc, and wraiths the revenant. The wolf at (-18,17) and boar at (18,-18) are decorative wildlife. These are visual substitutions: existing enemy rules, names, collision, navigation, quests and saves are unchanged. The player retains the equipment-aware character renderer.

The seven actor models use a basic runtime presentation rig with smooth spatial weights at every LOD. Residents/wildlife breathe; enemies also swing their limbs from existing gait, travel and attack channels. Color and shadow passes use the same palette. These procedural motions are prototype animation, not artist-authored Blender actions or production skin weights. Animated actors must be queued each frame, outside the static-scene cache.

Frostfang and Thornback now have transient ambient behaviour (`game/Wildlife.h`): rest, roam, watch nearby players, flee close approaches and return toward their habitat. Their differing speeds/stride lengths drive the existing leg rig from actual travel. A fixed 120 Hz update and bounded steering probes use the shared map clearance; rendering does not advance behaviour. The controller has no combat, loot or saved progression and resets on world initialization, successful load and recovery/restart. It follows the live world's update policy, including the menu. Authored quadruped clips, foot planting and richer species behaviour remain future work.

## Review

Build with `scripts/build-native.ps1 -Backend Ninja -Configuration Release`. Run `build-ninja-release/MediterraneanAssetReview.exe` for the collection in the actual DirectX 11 renderer. Left/right selects a model, A/D rotates it, Space toggles the turntable, L cycles detail levels, and Escape closes the window. This viewer never opens or writes player saves.

`MediterraneanAssetReview --capture <directory>` writes thirty-four native-engine PNGs without a visible window. Interactive review animates actors. `AuthoredAssetTests` validates the archive, corrupt input rejection, bind-pose identity, finite normalized influences, visible deformation, animated color/shadow rendering at all three LODs, and camera-dependent LOD on cached scenery. Current captures and contact sheets are under `.blender-local/world-art-20261031`; they show the actual DirectX renderer.

## Export contract

- Coordinates are metres, Y up and +Z forward. Gallery roots, platforms, text, lights and cameras are excluded.
- Geometry is triangulated, with outward unit normals, vertex colors and a native material ID per triangle. Curve/modifier output is evaluated offline. Bounded whole groom subsets are retained for wolf and boar.
- Blender procedural shaders are approximated using the engine's existing material layers; this is not a texture bake or identical Blender shading.
- Buildings omit loose barrels and entry steps; overhangs are compressed beyond their unchanged wall footprints to fit existing camera bounds.
- Near meshes are bounded to 180,000 split vertices and 40,000 triangles (60,000 for the first five humanoids, 80,000 for the oak); middle/far meshes retain the 65,536-vertex / 20,000-triangle limits. The full archive is bounded to 80 MiB. `collection01.json` records actual counts, bounds and source/archive SHA-256 values.
- Native loading validates headers, lengths, counts, coordinates, normals, materials, index ranges, triangle winding, bounds and decreasing LOD sizes before GPU upload. No paths or external textures are opened.
- GPU geometry is uploaded once. Replayed static scenery resolves its detail level against the current camera before shared color/shadow culling.

The `MDA1` archive is little endian: magic, version, asset count, LOD count (four 32-bit values), followed by asset-major/LOD-minor records. A record contains two 32-bit vertex/index counts, six float32 bounds values, ten float32 values per vertex (position, normal, RGB, integer material ID), and uint32 triangle indices. Material IDs follow `visual::Material`; vertex alpha is a material ID only for authored draw instances.

Production retopology, artist-authored skeletons/weights, facial animation, Blender action export and interchangeable equipment for these models remain future art work. The live integration uses the native procedural presentation rig and preserves the existing player customization system.

## Combat presentation channels

Defeated skeleton/orc/revenant enemies retain their authored mesh: a 1.05-second presentation collapse blends out the captured outgoing pose, then rests on the ground until looted. The renderer applies the fall to every palette joint and normal, including the root, so color and shadows agree. Settled floor clearance uses the asset bounds. Loaded dead enemies start settled; no death timer enters save data. This is a controlled fall, not a ragdoll or corpse collision solver; inspect nearby walls and other actors manually. Trainer behaviour keeps its existing representation.

Authored enemies now consume the existing smoothed action and impact channels through `game/AuthoredActorMotion.h`. Torso pitch/twist/roll, hip offset and independent arm lift/opening supplement the simple gait and weapon swing. Arm transforms inherit the torso transform; upper-body gestures leave the planted leg chains unchanged. Color and shadow rendering share the same bounded, finite palette. The game remains authoritative for accepted actions, interruption, contact and damage timing; rendering does not run another action clock. Wildlife and neutral residents retain their previous default poses.

`AuthoredAssetTests` captures neutral/action/hit views of Ash sentry, Ironjaw and Oathless in `authored-combat-check`, checks deformation at every LOD, planted lower legs and invalid-channel sanitization. This recovers presentation previously lost during model substitution; it is not a replacement for artist-authored skeletal clips or production skin weights. Inspect shoulder, cloak, shield and weapon clearance during live attacks (manual check 277).

## Collection 03 model adoption

The editable collection and embedded archive now include the locally refined Collection 03 versions of all seven actors, Elder oak and Blighted elder. Highland pine, Hearth cottage, Moon inn and Border watch retain the newer accepted environment versions; their twelve runtime LOD records are byte-identical to the preceding archive. Both editable source copies are synchronized. The previous source and exports are backed up locally under `.blender-local/collection-02/runtime-integration/before-integration`.

The Collection 03 export contained 39 meshes and occupied 13,001,708 bytes before the wolf replacement below. High-resolution source detail remains editable in Blender; runtime exports omit individual fur/hair strands, stitches, abrasion curves and disconnected chainmail rings, whose minimum geometry exceeds the native budget. Representative shader colors feed the existing native material layers. This remains an approximation of Blender shading, not a texture bake.

The reproducible export script is local and ignored: `.blender-local/collection-02/runtime-integration/export-runtime.py`. Run it with Blender in background mode against `assets/authored/collection01.blend`; it writes candidate exports into its runtime-integration directory and validates size limits before writing. `check_runtime.py` in the parent directory checks the archive and preserved environment records before adoption. Do not rerun the older exporter against the refined source without the detail exclusions.

World roles and the native procedural presentation rig remain in place. The separate Collection 03 Blender rigs and action library are not connected to gameplay by this model update. Playable characters retain their customizable equipment-aware renderer, including the separately adopted guardian cuirass; this update does not replace every player class with the guardian body.

## Frostfang anatomical rig integration

Frostfang now overrides the older wolf in `collection01.blend`. Its editable source is `assets/authored/frostfang.blend`; preserve that source when exporting the full collection. Only the wolf's three mesh records were replaced in `collection01.masset`; the other 36 records are byte-identical. The archive is now 13,327,876 bytes, with wolf LODs of 13,918 / 5,534 / 1,906 triangles. Coat vertex colors are retained; dense individual hairs and Blender procedural shading are not reproduced by the native material layer.

`assets/authored/frostfang.motion` embeds normalized four-weight skinning for all three LODs and four looping Blender clips: Idle, Walk, Trot and Gallop. Thirty joints include scapulae, carpi, hind stifles/hocks/metatarsi, toes, jaw, ears and a segmented tail. Runtime interpolation uses quaternion slerp and translation lerp, with smooth gait blending. Existing wildlife travel selects the gait and advances phase by distance actually moved. World rules, locations and saves are unchanged. This source still needs anatomical/art polish; integration does not certify photorealism, terrain IK or production-quality motion.

The native renderer supports 32 palette joints while keeping the existing ten-joint humanoid rig and 32-actor capacity. Palettes use a structured float4 buffer, shared by color and shadow shaders, rather than exceeding the DirectX 11 constant-buffer limit. The WLF1 decoder bounds sizes, counts, weights, quaternions, translations and loop endpoints, and rejects mesh/weight count mismatches.

The local ignored exporter is `.blender-local/collection-02/export_wolf_game.py`; it reads the wolf study, creates candidate files under `wolf-study/runtime`, and preserves other archive records. Previous exports are backed up there under `before-adoption`. `AuthoredAssetTests` additionally validates malformed motion data and captures all four wolf clips across LODs. Other actors still use their existing procedural presentation; their separate Blender clips are not integrated by this change.

## Player Blender motion retargeting

The following v2 notes describe the preserved baseline. The current v4 archive
contains 54 tracks (428,772 bytes), including directional equipment carrying,
bow actions and revised weight/settle. `assets/authored/oldstyle-classes.blend` and its KIT1 export add
27 fitted parts for Warrior, Scout and Mage. See
[oldstyle-player-art.md](oldstyle-player-art.md) for current source preservation,
native formats, class equipment, budgets and review paths. Do not rerun the v2
exporter over the v4 archive.

`player.clips` now embeds 22 v2 human actions plus six preserved compatibility tracks. V2 covers idle, sword/staff guards, directional and equipped walks/runs, sword/staff attacks, jump, fall and landing. The editable action rig is `assets/authored/player-motion-v2.blend`; `player-clips.json` records source/track/archive hashes and contact markers. The ignored exporter is `.blender-local/male-study-20261028/motion-v2/runtime/export_runtime.py`. It reads the existing full v2 Blender character without rewriting it. The old Collection 04 exporters must not overwrite this archive.

The native player keeps its customizable body and equipment. Imported targets drive the existing limb IK and world-space foot contacts. Sword/staff attacks align source markers with authoritative contact/release while allowing a longer visual recovery; interrupted attacks freeze and fade. Staff orientation and both grips follow the imported motion. Class casting gestures and other ranged weapons retain their class presentation. Jump/fall/landing use the accepted airborne state. This is target-based retargeting onto the current body rig, not direct playback of all 57 source bones. See `docs/authored-player-locomotion.md` for the current contract and review paths.

The bounded PCL2 format stores 28 tracks with timing/contact metadata and 19 float32 channels per frame. V2 tracks use 60 Hz source samples; compatibility tracks keep their original sampling. Loading rejects unexpected counts, invalid timing/markers, nonfinite channels, invalid weapon directions, broken loop endpoints, truncation and trailing data. Sampling uses cubic interpolation; attacks also use an arc-length timing map. The 230,768-byte archive is embedded as resource 303; no runtime Blender installation is required.
