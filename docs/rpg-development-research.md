# RPG development research and production plan

Research checkpoint: 2026-10-23. Scope: production documentation, developer articles, specifications and public design references for our single-player C++ / DirectX 11 RPG. This is a research and planning deliverable, not an implementation report.

## Decision

Keep the custom engine. The next substantial quality increase should come from an authored character/animation/material pipeline, terrain-aware movement and a coherent playable encounter. Increasing polygon counts or adding more colored particles in isolation will not solve our presentation problems.

We already have continuous skinned bodies/hands, procedural action poses, eleven class kits, an antique UI asset set, character creation, a twelve-quest campaign and spatial audio. The missing step is production depth and authoring control, not rebuilding those systems from zero. The proposed milestones below are our engineering judgments; they are not claims that another studio uses our architecture.

## Evidence and limitations

Read alongside [asset authoring](asset-authoring.md), [graphics](graphics.md), [class progression](classes-and-campaign.md), [class combat](class-combat.md), [sound design](sound-design.md), [roadmap](../roadmap.md) and [manual acceptance checks](../tests/manual-smoke.md).

This review inspected repository documentation, selected source and public documentation passages. It did not rewatch the local reference video, perform a new visual playtest, listen to exported audio, or watch entire conference recordings. GDC Vault redirected to a maintenance page; its indexed session description is a discovery lead only. Blender's latest English page failed to load; indexed official versioned documentation was available. The Khronos registry and official specification repository supplied the accessible specification passages.

Official Allods gameplay documentation establishes player-facing behavior, not proprietary engine internals or the current Russian client's complete class balance. No inference about its renderer, animation toolchain or Lua implementation is required. Blizzard's historical Artcraft material is a production reference, not documentation of the latest client. No assets, dependencies or installers were downloaded.

## 1. Character production: author once, validate, then render

**Documented basis.** Khronos defines skin joint indices and inverse-bind matrices. Its animation format stores channels and interpolation data; playback policies and state-machine behavior remain the application's responsibility. Blender's exporter supports bone/object transforms and shape-key animation, with export modes affecting how actions become clips. See sources S1–S3.

**Our gap.** The OBJ compiler deliberately rejects rigs and materials and discards UVs. Existing procedural skinning is valuable runtime infrastructure, but it does not provide an artist with editable anatomy, clothing weights, authored facial shapes or an animation library.

**Proposed implementation.** Add a bounded offline GLB/glTF import subset that produces our own validated runtime data. Start with one original humanoid, one outfit and one weapon. Document units, handedness, up axis, winding, bind transforms, maximum joints, influence count and clip naming before writing conversion code. Coordinate conversion must transform geometry, skeleton, animation and attachment sockets consistently.

Initially accept only the data we actually support. Reject unsupported required extensions and malformed accessors, invalid joint references, non-finite transforms and out-of-budget inputs. Do not silently pretend arbitrary glTF files are supported. Preserve the previous valid output on failure, as the OBJ tool already does. Keep compiler diagnostics tied to asset and field names.

Acceptance: neutral pose matches the authoring reference; elbows/knees bend without obvious collapsing; equipment follows sockets during all clips; world and portrait agree; deterministic conversion and invalid-input tests pass. Use a tiny original synthetic rig fixture before attempting a full hero. A model editor such as Blender would author content; our DX11 engine would still render it. Building our own full modeling application is deferred.

## 2. Animation: continuity with responsive input

**Our proposal.** Build the first imported library around idle, start, stop, forward/backward/strafe locomotion, turn-in-place, takeoff, airborne, landing, basic attack, heavy preparation/release/recovery and hit reaction. Reuse existing procedural correction layers rather than discarding them.

The controller needs explicit transition priorities and interruption rules. Locomotion responds to velocity and intent; attack overlays should not reset lower-body motion unnecessarily. Foot planting must distinguish a supporting foot from a swinging foot. Hand correction should target weapon sockets with bounded reach, not stretch the entire arm to hide a bad asset.

Keep movement authority in simulation for the first imported rig. Root motion is an explicit future policy: extract desired displacement, resolve collision, then reconcile the displayed pose. Never apply both authored root translation and existing simulation movement to the same actor.

Contacts, damage, audio and trails must share a documented action timeline. Visual animation events may request presentation, but combat acceptance remains simulation-owned. Seeking a preview clip, loading a character or replaying a blend must not award damage twice.

Acceptance: test transitions at 30/60/144 Hz, rapid reversals, repeated attacks, attack during movement, interrupted casts, landing while turning, focus loss and save/load. Record synchronized side/front/gameplay views. Automated pose bounds are necessary; human review of weight, rhythm and sliding remains necessary too.

## 3. Movement and terrain: fix the support model

**Documented basis.** NVIDIA's character-controller documentation describes step handling, non-walkable slopes and update-order effects on moving supports. Gaffer's timestep article explains bounded integration, accumulator stepping, interpolation and overload risks. These are design references, not a proposal to install PhysX. See S4–S5.

**Our gap.** Surface-dependent traction now works, but the world remains flat. Small integration steps do not automatically give us stair traversal, reliable slope support or a fully fixed simulation clock.

Introduce one shared ground query returning height, normal, material and support identity. Rendering, collision, feet, shadows and contact sounds should consume compatible ground data. Build a small test area with ramps, a shallow step, a rejected high step, a ledge and a wall before changing the whole island.

Specify walkable slope limits, step height, snap distance and grounded hysteresis. Ground snapping must not pull an ascending jumper down. Dash collision must stop at obstructions and retain a natural recovery. Sweep against obstacles rather than relying solely on final-position overlap. Keep camera smoothing separate from physical position to avoid apparent controller lag.

Audit the current clock before deciding on an accumulator migration. If adopted, interpolate presentation between committed states and bound catch-up work; handle suspension explicitly. Fixed time alone does not guarantee cross-machine determinism.

Acceptance: no tunneling at thin walls, no repeated land events on seams, stable standing on slopes, consistent jump apex/travel across frame rates, and no climb over rejected steps. A provisional one-centimetre traversal tolerance on the test course is a project target to validate, not an achieved result.

## 4. Combat identity, timing and effects

**Documented basis.** Allods' guide describes explicit targeting, range checks and combat feedback. Riot's clarity article emphasizes recognition, effect hierarchy and avoiding noise that obscures meaningful actions. Sources S6–S7.

**Our proposal.** Deepen two representative kits first: Warrior and Mage. Retain all existing classes, but use this pair to establish melee commitment versus ranged preparation, mobility, interruption, resource use and recovery. Then apply the proven ability-data contract to the other kits.

Each ability should have explicit preparation, commit/contact, recovery, cancel window, range rule and resource/cooldown transaction. Show why a rejected action failed. Keep damage numbers, talent previews, inventory comparisons and spell descriptions derived from the same calculation.

Create an effect grammar: a brief anticipation cue, a readable release shape, a contact response and a fading residue. Preserve each class's silhouette and material language. Hostile warnings must remain legible beneath player spectacle. Attach trails to actual weapon/projectile motion, not arbitrary orbiting shapes.

Acceptance: a single event produces one hit and one sound; interrupts cancel pending releases correctly; death/load/menu transitions clear transient effects; two simultaneous attacks remain distinguishable. Capture normal and reduced-effects modes and record particle/overdraw costs. Add depth before increasing effect count.

## 5. Materials and believable environments

**Documented basis.** Google's Filament documentation distinguishes material inputs such as base color, metallic response and roughness, including linear-color expectations. Blizzard's Artcraft level-design discussion illustrates deliberate landmark history and route layout. Sources S8–S9.

**Our proposal.** Preserve the classical fantasy direction while improving material separation: cloth should not reflect like steel, stone should not look like plastic, and wear should follow exposed edges and contact areas. More triangles mainly help silhouette and deformation; surface detail also needs appropriate normals, UVs, lighting and color handling.

Begin with a controlled material test scene and one authored character material set. Add UV/tangent support, explicit sRGB versus linear interpretation, mipmaps and a small documented texture budget. Validate normal-map orientation, mirrored UV seams and shadow behavior before applying the path to the world. Advanced subsurface scattering and expensive global illumination are later options, not prerequisites.

For Dawnwatch, design one recognizable route connecting a safe hub, a dangerous approach and a ruined destination. Use elevation, sightlines, architecture and local props to explain the place. Expand map area only after traversal and navigation survive the new terrain. Do not distribute equal amounts of clutter everywhere.

Acceptance: fixed-camera before/after captures in identical light, visible material differences without bloom, stable distant surfaces and measured memory/upload cost. Lock exposure during comparisons so brightness changes cannot masquerade as material improvements.

## 6. Navigation, enemies and encounters

**Documented basis.** Recast separates offline walkability generation from Detour runtime path queries, with additional modules for tiles and crowds. Source S10. Studying this separation does not require replacing our engine or acquiring a dependency.

**Our proposal.** Establish a common traversability contract between player collision and enemy navigation. Once elevation exists, test a small baked navigation representation against the actual controller dimensions before selecting a larger implementation. Include unreachable-target outcomes and bounded replanning work.

Build encounters around complementary roles: a pursuing melee enemy, a ranged enemy that seeks a valid firing position, and a visibly telegraphed heavy attacker. Give each a clear leash, return path, interruption response and recovery. Spawning more copies of one behavior does not create a richer encounter.

Acceptance: enemies do not attack through walls, jitter at corners indefinitely, cross forbidden cliffs or stack without a resolution strategy. The same scripted route should be reproducible after loading. Debug overlays should show desired path, accepted movement, target, range and current state separately.

## 7. Quest authoring and progression

**Documented basis.** inkle documents a text-first narrative workflow with choices, state and immediate story testing. Its writing manual explains persistent narrative variables. Sources S11–S12. This is a workflow reference; adopting its runtime or language is not part of this plan.

**Our proposal.** Keep existing authored dialogue and extend validated quest definitions with stable IDs, prerequisites, objectives and completion effects. A small inspectable format is sufficient initially. Build a validator that catches missing references, unreachable required stages, accidental dependency cycles and undefined rewards.

Add one original short quest with a meaningful branch: recovering supplies peacefully versus fighting for them, with a visible NPC or world-state response. Keep outcomes modest enough to test. Rewards must be committed once, including after repeated dialogue clicks and reloads.

Level ten remains the cap. Review XP pacing along a complete playthrough: time to the first talent, downtime between meaningful choices, equipment replacement cadence and difficulty spikes. A spreadsheet of damage alone cannot validate the player's experience.

Acceptance: both branches finish, abandoned/reloaded stages remain coherent, old saves migrate, a full bag cannot duplicate or silently lose rewards, and dialogue previews match actual grants. Conference quest-design talks remain supplemental reading until their full content is accessible.

## 8. Classical UI without fragile interaction

**Documented basis.** Microsoft's Xbox Accessibility Guidelines cover text, contrast, input, focus, navigation and motion settings. They are practical design guidance, not a claim of certification. Source S13.

**Our proposal.** Keep bronze, worn leather, burgundy and parchment, but give the entire UI shared spacing, typography, frame, selection, disabled and error states. Decorative headers can remain ornate; dense text needs a legible body face and scaling that does not merely shrink it indefinitely.

Create a screen inventory: launcher, loading, roster, creation, HUD, inventory, equipment, talents, quests, map, merchant and Esc. Test the actual input owner whenever panels overlap. Recent toast/blocker bugs show why visual layout and hit testing must share geometry and lifecycle.

Acceptance: keyboard focus is visible, RMB capture never starts over an active panel, close/return restores sensible focus, long names and future localized strings fit, and 960-wide/1080p/ultrawide layouts remain usable. Persist display/audio/input preferences independently of character saves after defining a versioned settings contract. Lua is not necessary to produce these layouts.

## 9. Audio: arrangement and reliability

**Documented basis.** Microsoft's XAudio2 troubleshooting guide identifies buffer starvation, excessive processing and blocking callbacks as glitch causes, and exposes performance counters. Source S14.

**Our proposal.** Keep the small original bank while improving arrangement and contrast. Give important warnings priority over ambience and repeated impacts. Build a class sound palette with distinct preparation, release and impact components; tune levels in a complete encounter, not only isolated samples.

The present short regional loops need listening review for repetition. Add sparse, scheduled one-shots and silence before merely lengthening every loop. Drive footsteps from validated contact timing when the imported animation path is ready. Material selection should use the contacting boot's support query where available.

Acceptance: no clicks at loop boundaries, no warning starvation under rapid combat, no stale sound after roster/load transitions, and no clipping in the combined mix. Record voice counts and XAudio2 glitch counters. Test device interruption/recovery separately. Procedural synthesis remains useful, but PCM validity cannot prove artistic quality; original recorded content would need a separate authorized acquisition/authoring step.

## 10. Performance, tools and future multiplayer

**Documented basis.** D3D11 timestamp results need a valid non-disjoint measurement interval and counter frequency. Gaffer's snapshot article describes interpolation of buffered remote state and the latency/smoothness trade-off. Sources S15–S16.

**Our proposal.** Establish a reproducible Release scene with idle village, movement, several enemies, heavy effects, character panel and repeated roster transitions. Record CPU/GPU frame distributions, draw submissions, allocations, memory, loading time and audio health. Store resolution, hardware, driver, build revision and scenario with every report. Read GPU queries later without blocking the current frame.

A provisional goal is stable 60 FPS on a declared reference PC at 1080p: 16.67 ms total frame time. This is a target, not current measured performance. Report p50/p95/p99 and distinguish CPU from GPU limits. Retain the recent exact-equivalence terrain optimization, but do not convert its microbenchmark percentage into an FPS claim.

Build small tools that accelerate the next task: asset validation, clip preview/scrubbing, ground/navigation overlays, action timeline inspection and quest-state tracing. A general-purpose editor, huge ECS rewrite or custom Blender replacement is not the next milestone.

For future networking, preserve stable actor IDs, simulation-owned commands, authoritative action validation and serializable state. First explore a headless single-process replay or local authority boundary. Later networking work needs prediction/reconciliation, remote interpolation, interest management, persistence and abuse resistance. A local save snapshot is not automatically a network protocol; an MMO is not achieved by adding sockets.

## Recommended delivery sequence

| Milestone | Depends on | Concrete exit gate |
| --- | --- | --- |
| R1: quality baseline and clock/input audit | Existing benchmark/tests | Repeatable frame report, comparison captures and documented simulation timing |
| R2: one imported rig and clip pipeline | Asset contract and R1 budgets | Original fixture and hero render correctly; invalid assets fail safely |
| R3: animation/controller polish | R2 | Full transition matrix, contact events and equipment alignment reviewed |
| R4: elevation/controller/navigation slice | R1; integrates R3 | Ramp/step/ledge course passes; enemy can traverse the same supported route |
| R5: material and UI consistency slice | R1; R2 for character materials | One finished hero/route plus readable front-end and HUD comparisons |
| R6: two-class encounter and branching quest | R3–R5 | Complete, save-safe playthrough with readable effects and reviewed audio |
| R7: scale verified content | R6 | Extend proven contracts to remaining classes and campaign without budget regressions |

R2 and R4 can be developed independently after R1, but do not combine them into an untestable all-engine rewrite. Preserve playable checkpoints. A milestone is complete only when implementation, regression checks and relevant visual/listening acceptance agree. Existing manual checks 1–221 remain historical acceptance records; new feature checks should be appended when those features are actually implemented.

## Reading register

All links were researched on 2026-10-23. “Indexed passage” means the public search tool returned source content; it does not imply the complete document or every embedded video was reviewed.

| ID | Primary source | Evidence reviewed and intended use |
| --- | --- | --- |
| S1 | [Khronos glTF specification](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc) | Indexed animation/container passages; storage contract, not a playback controller |
| S2 | [Khronos skin schema](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/skin.schema.json) | Schema content; joint and bind-matrix validation |
| S3 | [Blender 5.0 glTF manual](https://docs.blender.org/manual/id/5.0/addons/import_export/scene_gltf2.html) | Indexed animation/export passages in official localized manual; verify chosen installed exporter before prescribing exact settings |
| S4 | [NVIDIA PhysX character controllers](https://nvidia-omniverse.github.io/PhysX/physx/5.3.0/docs/CharacterControllers.html) | Indexed stepping/support/update-order documentation; controller design reference |
| S5 | [Fix Your Timestep! — Glenn Fiedler](https://gafferongames.com/post/fix_your_timestep/) | Article text; simulation/render time separation |
| S6 | [Allods Online combat basics](https://allods.my.games/en/game/game-guide/basics) | Official indexed gameplay guide; targeting/range/feedback reference |
| S7 | [Riot: Clarity in League](https://www.leagueoflegends.com/en-us/news/dev/clarity-in-league/) | Indexed developer article; hierarchy and recognition |
| S8 | [Google Filament rendering documentation](https://github.com/google/filament/blob/main/docs/Filament.md.html) | Indexed material parameter passages; material-model reference only |
| S9 | [Blizzard Artcraft: Level Design Part 4](https://worldofwarcraft.blizzard.com/en-us/news/16595426/artcraft-level-design-part-4) | Indexed developer interview passages; authored routes and landmark context |
| S10 | [Recast Navigation documentation](https://recastnav.com/) | Module overview and generation workflow; navigation architecture reference |
| S11 | [inkle: ink and Inky](https://www.inklestudios.com/ink/) | Official workflow overview; authoring and testing narrative |
| S12 | [Writing with ink](https://github.com/inkle/ink/blob/master/Documentation/WritingWithInk.md) | Indexed variables/logic passage; persistent narrative state |
| S13 | [Xbox Accessibility Guidelines](https://learn.microsoft.com/gaming/accessibility/guidelines) | Guideline overview and topic index; UI acceptance categories |
| S14 | [Debugging audio glitches in XAudio2](https://learn.microsoft.com/en-us/windows/win32/xaudio2/debugging-audio-glitches-in-xaudio2) | Indexed troubleshooting article; diagnostics and callback constraints |
| S15 | [D3D11 timestamp disjoint query](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_query_data_timestamp_disjoint) | API page text; trustworthy GPU timing |
| S16 | [Snapshot Interpolation — Glenn Fiedler](https://gafferongames.com/post/snapshot_interpolation/) | Article text; future remote-state presentation |

Further viewing lead: [Paweł Sasko, 10 Key Quest Design Lessons](https://www.gdcvault.com/play/1028897/10-Key-Quest-Design-LessonsStartup). The indexed description was discoverable, but opening the talk redirected to maintenance. No detailed lessons in this document are attributed to an unwatched recording.
