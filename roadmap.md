# Mediterranean Roadmap

Last updated: 2026-10-17

## Direction

Build a classic third-person RPG for Windows PC using C++ and a custom DirectX 11 engine. Allods Online is the primary reference for setting, visual direction, and mechanics. World of Warcraft and The Lord of the Rings are secondary references and should not override that direction. Do not use an existing game engine.

### First playable slice: floating-island outpost

The following are project design decisions, not claims that these systems have been implemented:

- Setting: a small inhabited island surrounded by an astral-like sky, with an outpost, a path, and an enemy camp.
- Visual target: stylized proportions, readable silhouettes, warm timber and stone buildings, painted color blocks, and cool atmospheric distance. Start with procedural placeholder geometry.
- Controls: third-person follow/orbit camera, WASD movement, mouse camera control, and zoom.
- Combat target: explicit enemy selection, a numbered action bar, cooldowns, range checks, and readable health and target feedback. Start with one melee archetype and one enemy type; existing JS abilities are migration references rather than the final class design.
- First objective: leave the outpost, defeat a nearby enemy, return, and receive a reward.
- Later scope: broader class progression, faction contrast, and astral travel. Multiplayer and ship systems are outside the first slice.

Official reference sources: [Allods setting and factions](https://allods.ru/splash.php?v=7), [class descriptions](https://allods.ru/classes.php), and [game FAQ](https://allods.my.games/en/game/faq). These support the fragmented-world setting, faction identity, class roles, and astral travel references. Exact combat timing and visual specifications still need focused reference review; do not invent engine internals or treat current live-game balancing as a classic-era specification.

This is a proposed implementation sequence, not a list of completed features. See [progress.md](progress.md) for verified status.

## Implementation checkpoint - 2026-10-17

The first source implementation spans the native foundation, scene, movement, combat, HUD, panels, patrol objective, and save/load milestones below. None of their runtime acceptance criteria has passed yet: compilation and tests are blocked by missing local build tools. The next priority is a verified build and smoke test, then fixes and visual iteration; avoid treating source completion as playable completion.

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
