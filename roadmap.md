# Mediterranean Roadmap

Last updated: 2026-10-18

## Direction

Build a classic third-person RPG for Windows PC using C++ and a custom DirectX 11 engine. Allods Online is the primary reference for setting, visual direction, and mechanics. World of Warcraft and The Lord of the Rings are secondary references and should not override that direction. Do not use an existing game engine.

Build single-player first and concentrate on one well-developed test map. Keep simulation, world data, and command handling independent of rendering/input so a future MMO server can reuse domain code; networking is not part of the current milestone.

### First playable slice: floating-island outpost

The following are project design decisions, not claims that these systems have been implemented:

- Setting: a small inhabited island surrounded by an astral-like sky, with an outpost, a path, and an enemy camp.
- Visual target: stylized proportions, readable silhouettes, warm timber and stone buildings, believable timber, stone and cloth surfaces, and cool atmospheric distance. Use compact procedural geometry/materials while external art datasets are excluded.
- Controls: third-person follow/orbit camera, WASD movement, mouse camera control, and zoom.
- Combat target: explicit enemy selection, a numbered action bar, cooldowns, range checks, and readable health and target feedback. Start with one melee archetype and one enemy type; existing JS abilities are migration references rather than the final class design.
- First objective: leave the outpost, defeat a nearby enemy, return, and receive a reward.
- Later scope: broader class progression, faction contrast, and astral travel. Multiplayer and ship systems are outside the first slice.

Official reference sources: [Allods setting and factions](https://allods.ru/splash.php?v=7), [class descriptions](https://allods.ru/classes.php), and [game FAQ](https://allods.my.games/en/game/faq). These support the fragmented-world setting, faction identity, class roles, and astral travel references. Exact combat timing and visual specifications still need focused reference review; do not invent engine internals or treat current live-game balancing as a classic-era specification.

This is a proposed implementation sequence, not a list of completed features. See [progress.md](progress.md) for verified status.

## Implementation checkpoint - 2026-10-18

Debug and Release builds pass using Ninja/MSVC with thirteen regression suites and 132 hidden render captures per configuration, including interface layouts, outfit changes, sparring, the shop, buyback, eight close-up poses and seven environment views. Gameplay starts fullscreen; Escape opens a live menu. The supplied Allods screenshot guides the green/gold frames, independent character/bag windows and circular minimap. The bag has 24 real cells, drag swaps, stack splitting/merging, sorting, comparisons and six worn slots; see [interface design](docs/interface.md). The village quartermaster sells bounded supplies, a helm and shield, accepts bag items and retains them in persistent buyback. Purchased gear changes both character views and combat protection. Character presentation now uses shaped heads/boots, fitted hair, fixed-length limb poses and hand-attached equipment. A four-workload Release benchmark covers walking/orbit, combat and the merchant alongside the stationary HUD; see [graphics budgets](docs/graphics.md). The map contains a two-quest campaign, levels 1–3, a captain, anchor stabilization, rewards, progress-preserving death recovery and repeatable nonlethal practice without reward farming. Version 6 preserves exact split stacks, worn gear, purchases, buyback and balanced gold while migrating versions 1–5. Interactive input, combat feel and final art acceptance still need playtesting.

### Next work on this map

The combat-effects milestone now includes layered gold/cyan crescents, a delayed violet Heavy Strike counter-sweep, contact halos, expanding rings, tapered sparks and rising motes. This follows the requested expressive Asian-MMO visual direction while retaining fixed pools, one additive depth-tested batch and the existing upload buffer without effect textures. Thirteen suites, 132 smoke views and 28 additional effect diagnostics pass in Debug/Release, including equal-time 30/60/144-Hz rendering, ring occlusion and maximum overlapping effects. Damage, range, cooldowns and save version remain unchanged. Review manual checks 74–80 for attack readability from different camera angles and fullscreen cost before adding sound or new abilities. See [graphics budgets](docs/graphics.md) for measured cost and remaining limits.

The surface/lighting milestone adds packed material relief/roughness, object-aligned triplanar textures, warm/cool linear-light shading, irregular paving and layered clouds. Its bug-fix follow-up anchors the sky to the current camera direction and corrects grazing shadow stripes. It retains existing geometry and fixed asset allocations while increasing pixel-shader work; the measured cost and process-memory change are recorded in [graphics.md](docs/graphics.md). Debug/Release pass twelve suites and 132 captures, including texture anchoring, sky-camera behavior and portrait isolation. Manual checks 70–73 govern visible acceptance. Continue toward authored character/clothing detail, foliage composition and physical atmosphere after assessing this material pass during actual play.

The defeat follow-up cancels stale enemy telegraphs, prevents movement/separation after a fatal hit, and lets enemy strike poses settle while combat is suspended. Older defeated saves cannot restore a frozen pose or warning cone. Combat, animation and immediate-load rendering regressions pass alongside all twelve suites and 132 captures in Debug/Release. Manual check 69 covers live defeat/load/recovery acceptance; see [progress.md](progress.md) for the failing reproductions and final logs.

The CPU performance pass prepares frusta once per view, packs visible instances with a stable counting scatter, and reuses fixed interface curve samples. All 132 Release captures match the prior build byte for byte. Three paired hidden workload runs show about 5% / 12% / 6% lower CPU frame time for walking / combat / merchant; the stationary workload shows no consistent gain. Twelve suites and 132 captures pass per configuration, with unchanged fixed GPU allocations. Continue with visible, paced performance profiling before increasing scene complexity; manual check 68 covers that acceptance step.

The shadow follow-up fixes fully culled queues leaking geometry between world and character preview, plus premature upload overflow after adding the second map. Each distinct visible object is now uploaded once and shared by the shadow/color passes with the same fixed buffer. Regression fixtures reproduce both leak directions and the 1,400-object failure; exact 4,096/4,097 boundary checks retain the hard limit. See [progress.md](progress.md) for validation and manual check 67 for preview transitions during play.

The focused-shadow milestone adds a 20-unit detail projection beside the fixed 62-unit whole-island projection. Both use 1024-square depth layers, adding 4 MiB for 3.1 times finer nearby sampling. Texel snapping and an overlap blend stabilize coverage; separate light culling and a depth-only shader bound the extra work. Thin-post retention, grid stability, blend transitions and distant fallback now have regressions. Twelve suites and 132 captures pass per configuration. Review manual checks 64–66 and the measured cost in [graphics.md](docs/graphics.md); local shadowed lights and sustained interactive profiling remain future work.

The latest smoothness pass adds velocity-preserving player turns and player/enemy gait/attack blends, plus eased foot-lift and air-extension boundaries. Physical stopping and committed enemy facing remain authoritative. Twelve suites and 132 captures pass in Debug/Release, with new start/stop/reversal, boundedness, reset and update-rate checks. Run manual checks 61–63 to tune visible response; foot planting and skeletal clips remain later milestones.

The animation follow-up fixes missed consecutive-hit reactions, enemy flashes/recoil persisting after defeat, and frame-dependent landing compression. Transient damage events replace timer inference; within-step contact timing keeps landings consistent at 30/60/144 Hz and uneven update intervals. These regressions, event/save lifetimes and all twelve suites pass in both configurations with 132 captures each. Review manual checks 59–60 before accepting the temporal feel.

The character-animation milestone now adds separate rise/fall poses, height-aware leg extension before landing, knee compression, idle breathing/sway, movement lean and damage recoil. World NPCs and the equipment portrait share idle presentation, while player/enemy reactions read actual simulation state. Twelve suites and 132 smoke views cover the new layers, including four real jump stages. Review manual checks 56–58. Authored anatomy/clothing, locomotion foot planting, skeletal clips and sound remain future work; this pass reuses all existing meshes and textures.

The earlier shadow refinement retained the original 4 MiB sun map, added explicit caster flags and replaced the large uniform bias with a surface-aware tent filter. It fixed material-dependent missing shadows and weak contact from low objects. `ShadowTests` brought the total to eleven suites at that checkpoint, with GPU pixel checks for contact, slopes, moving/offscreen casters, resize, stale maps and portrait isolation. Review manual checks 53–55 for close trim edges and temporal quality. The focused-map milestone above extends that implementation; further coverage improvements and local shadowed lights remain later work, subject to resource measurements.

The movement pass implements short player/enemy speed ramps, bounded turns, travel-driven gait, strike blending, same-frame waypoint consumption, bounded separation and camera orbit/zoom/height filtering. Input release still stops the player immediately; telegraph facing and damage stay authoritative. MotionTests brings the suite count to ten, including 30/60/144-Hz ramp checks and no-drift/reset/windup tests. Review the feel with manual checks 47–49 before tuning the response constants or moving to authored animation. Fixed-length poses still lack foot planting and skeletal clips.

The smoothing follow-up fixes seven reproduced transition issues: frame-dependent jump height and gait, WASD aiming ahead of the visible camera, residual orbit after cancellation, boom oscillation at obstruction edges, stale direct-pursuit targets and close-range stalls behind trunks. Regression failures were captured before the fixes; expanded MotionTests now checks these paths, including continuous-clearance recovery timing and central-panel cancellation. Run manual checks 50–52 for rapid steering, edge-of-obstacle camera movement and close occluded pursuit. Longer paced performance measurements remain separate from visual acceptance.

The environment checkpoint added shared paved/dirt routes, atlas alignment, 131 grass clumps, 18 shrubs, folded leaf crowns and dock clearance fixes. The following landmark pass differentiates the four buildings, adds a covered supply stall and masonry ruin, and replaces the keeper proxy with a dressed NPC. Seven environment views bring the current smoke count to 122. Landmark meshes add about 58 KiB with no extra textures; measured workload costs are recorded in the graphics notes. Interactive visual acceptance remains pending.

The latest bug-fix pass addresses stale panel/load hit regions, UI-to-camera gesture leakage, the starter-sword hotkey while unarmed, and inconsistent quest saves. Automated regressions cover these paths; real desktop input remains the next validation step.

The 2026-10-18 follow-up fixes F11 arriving on the item-drop frame and central panels swallowing defeat recovery. Death now reveals the recovery prompt, and Enter remains usable from reopened panels. These paths have sampled-input regressions; physical display transitions still need the interactive checklist.

The character bug-fix follow-up stops stationary enemies walking in place, turns returning enemies toward their actual travel direction, and clears a player's frozen strike pose on defeat. Navigation and combat regressions cover movement, home arrival, stationary recovery, windup, transient save/load state and fatal counterattacks. Both configurations pass all nine suites and 115 captures; interactive animation acceptance remains on the checklist.

1. Run the expanded fullscreen/interface checklist, including real mouse input, Alt-Tab and DPI/monitor behavior; tune combat/camera/UI from actual findings.
2. Initial terrain, foliage and village/ruin identity passes are implemented. Continue toward authored character/clothing quality, more varied vegetation silhouettes and better motion transitions. The architecture remains exterior-only, the floor flat and the joint solver procedural, without skeletal skinning or planted-foot animation. Keep collision/navigation consistent and retain measured memory budgets. Review the new canopy/gable camera behavior during actual play before adding more large props.
3. Village trading now includes stack splitting, explicit sales and persistent buyback alongside limited supplies, helm/shield and six working slots. Automated tests cover exact-cell use/sale, old-save migration, balanced ownership/gold, denied transactions and repeated sell/reclaim cycles. Run the manual trade/split/reclaim checklist before expanding the item catalog or adding NPC services. Practice still grants no XP/loot and refunds no resources; broader progression remains planned.
4. Use the new walking/orbit, combat and merchant benchmark as a repeatable checkpoint before expanding rendering complexity. Short hidden-workload measurements are recorded in docs/graphics.md; visible interactive performance, larger encounters and long sessions still need measurement. Audio, skeletal animation and class specialization remain later work.

Gameplay tuning and campaign rules are documented in [docs/gameplay.md](docs/gameplay.md). Broader MMO systems and extra maps remain outside this milestone.

## 1. Establish the native development baseline

- Preserve the JavaScript prototype in Git history: `codex/archive-js-prototype` archives the former checkout, and `origin/dev` retains the later JS reference. The active tree is native-only.
- Bring the native source into the active project while preserving local instructions and the JS reference. Source-only import from `origin/C++` was chosen to avoid merging unrelated histories and generated binaries.
- Remove generated build outputs from version control and ignore local build directories.
- Verify the MSVC, Windows SDK, and CMake toolchain without changing system settings or downloading files without permission.
- Build and run the native executable from a clean build directory.
- Document reproducible build, launch, and debugging steps.

Acceptance: a fresh native build opens and closes its window successfully on the development machine.

## 2. Make the engine foundation reliable

- Handle window resizing and minimization.
- Check DirectX resource creation and presentation errors and report actionable diagnostics.
- Handle missing debug-layer support gracefully.
- Stop frame processing after shutdown and bound simulation time steps.
- Add keyboard and mouse input with focus-loss handling.

Acceptance: the application survives resize, minimize, restore, focus changes, and shutdown without crashes.

## 3. Render a playable scene

- Add shaders, vertex/index buffers, depth testing, and camera transforms.
- Render a ground plane, basic obstacles, and a placeholder character.
- Implement third-person camera rotation, follow behavior, and zoom.
- Port movement, sprinting, jumping, gravity, and collision behavior from the JS reference.

Acceptance: the player can navigate a small 3D test scene with predictable movement and camera behavior.

## 4. Restore the combat prototype

- Port health, damage, target selection, and enemy behavior.
- Implement a coherent first melee kit: strike, heavy strike, dash, and mend, with cooldowns and range checks. Preserve the JS bolt ability as a reference for a later ranged archetype.
- Implement visible feedback for attacks, damage, and death.
- Add a native HUD for player health, target health, and abilities.

Acceptance: the player can select and fight an enemy, use abilities, take damage, and heal.

## 5. Restore supporting interfaces

- Recreate the character, inventory, quest, and map panels from the JS prototype.
- Clearly distinguish interface placeholders from functional game systems.
- Define how menus affect input and simulation.

Acceptance: panels open and close reliably, display relevant state, and do not trigger unintended gameplay actions.

## 6. Build a small RPG vertical slice

- Define an original test location and a short playable objective.
- Add the minimum inventory, equipment, quest, and persistence behavior needed for that objective.
- Improve environment presentation, animation, and sound as the slice requires.
- Validate gameplay, stability, and frame performance before expanding scope.

Acceptance: a complete short gameplay loop can be played and its progress saved and restored.

## Working rules

- Record completed work and validation evidence in `progress.md`.
- Keep planned features separate from implemented and runtime-verified behavior.
- Keep generated files and machine-specific caches out of version control.
- Follow `AGENTS.md`: keep changes within the project, do not modify system files or settings, and obtain permission before downloads.
- Leave work uncommitted and do not push unless the user explicitly requests it.
- After each substantial development batch, provide ready-to-copy English commit text. The user performs the commit and push.
