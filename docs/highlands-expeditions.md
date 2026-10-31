# Playable highlands and advanced disciplines

## Scope — 2026-10-30

The playable rectangle grows from 58 x 62 m to **144 x 160 m**, about 6.4 times its former area. The established settlement and old save positions retain their original ground. Four broad hill groups surround it; the northern ridge rises above the settlement. This is real traversable relief, not the distant decorative ridge mesh.

`game/Terrain.h` owns the sampled heightfield. Rendering, player altitude, landing, planted feet, camera clearance, sight checks, actor placement, projectiles and the atlas all use that field. Triangle interpolation matches the rendered diagonal. Four new buildings have terraced foundations. A* uses height-aware edge cost and searches nearby endpoint cells first. The heightfield and cliff rim total **427,080 bytes and 12,760 triangles**; the terrain texture stays at its existing resolution.

The expanded scene exceeded the former 4,096-instance upload limit at some camera angles during the full native smoke run. The bounded union of color/shadow instances now permits 8,192, adding **640 KiB each** to the GPU upload and reserved CPU scratch capacity. Exact-capacity/overflow regressions remain enforced. Three overview captures include the populated six-enemy camps.

The old centre remains deliberately flat for save and encounter compatibility. Terrain is static, not destructible. Character collision remains a swept horizontal circle with a terrain floor; this is not a general rigid-body or ragdoll engine. Dashes follow the ground. Steep-slope sliding, cliffs with overhangs and authored terrain painting remain future work.

## New expeditions

All three follow the existing ten frontier dispatches. The journal now contains fifteen assignments including the original patrol and anchor quests. Maximum level remains 10.

| Region | Contact | Objective |
| --- | --- | --- |
| Signal Hills, west | Ranger Veyra, (-46, 20) | Extinguish three signals with E. A living guard within 7 m blocks the interaction; draw out the three pairs of guards. |
| Eastern Observatory | Keeper Orin, (44, 32) | Read three archive stones with E. Fighting the six wardens is optional. |
| Ashen Pass, north | Marshal Radan, (-8, 56) | Defeat six defenders, including the Ashen Overlord, then report back. |

New roads, four buildings, four ruins, eight small groves and six interactive stones use the existing native renderer and user-created asset library. Cover is generated once and bounded at 900 instances/90 shrubs. New missions use six enemies instead of three. They remain campaign encounters, not eighteen simultaneously persistent enemies across all three districts. Existing boss warning/interrupt/enrage rules apply to the new gate boss; this pass does not add a new boss animation library.

Each assignment grants 250 XP once. Objective masks and advanced cooldowns are stored in save version 19; versions 1–18 remain readable. Archive completion does not require dead wardens. Repeated interaction, repeated completion, reload and respawn cannot duplicate rewards. Old user saves are not rewritten until the user saves normally.

## Warrior, Mage and Scout

Two additional disciplines unlock at levels 5 and 8. They use committed targets, contact-time validation, real cooldowns and class-specific areas/status effects. Other classes do not receive substitute copies.

| Class | Level 5, 14 s cooldown | Level 8, 24 s cooldown |
| --- | --- | --- |
| Warrior | Sweeping steel: 85% heavy damage in a forward 3.2 m arc | Shieldbreaker: 125% heavy damage, 4 s exposure, 1 s stun, interrupt when allowed, 10 barrier on contact |
| Mage | Frost bloom: 65% heavy damage and 2 s root within 3 m of the selected target | Falling star: 140% heavy damage within 3 m of the selected target; longer preparation |
| Scout | Fanned arrows: 70% heavy damage in a narrow forward fan, 2 s slow, usable while moving | Piercing shot: 130% heavy damage, 5 s exposure, +35% heavy damage against a rooted target |

Mage disciplines and the Scout's piercing shot require stationary preparation until release. Movement cancels preparation without refunding its cooldown. Area hits also validate each secondary target's range and line of sight; friendly residents are never included. Newly applied exposure affects subsequent hits, not its own application. Effects add an expanding frost/fire ring, shard/descending streaks, a steel arc and arrow streaks using bounded native geometry. Frost preparation, projectile and contact retain their cold hue instead of inheriting the default fire layer.

Ground contours conform to the heightfield while body-bound rings keep their orientation. Corrected legacy barrier/control auras, ground impacts, dash-stop dust and enemy quake/lance effects that otherwise remained below elevated terrain. Quake rings use actual horizontal contour geometry rather than camera-facing discs.

N shows nine spell cards for these three classes, level requirements, descriptions and current bindings. Fresh layouts put the new spells on 9 and 0. **Existing saved layouts are preserved:** drag the new cards from N to any action slot. Hotbar version 6 reads earlier layouts without replacing intentional empty slots.

## Interface and atmosphere

The atlas now shows elevation contours, slope washes, correctly placed region captions and mission-stone status. Fifteen journal rows fit within the parchment. New names, quest prose, status messages and descriptions have Russian and English text. The existing weathered interface artwork remains; this is a functional content/layout expansion, not a replacement UI art package.

The terrain route shader now maps the expanded survey bounds. The wide shadow map covers the new land and the detail map follows height as well as position. Cloud coverage and distance haze increase modestly while retaining near-ground detail, exposure and the shared sun direction. Native captures were inspected against the local `loc1.jpg` landscape reference: roads across hills, wooded framing and depth separation are the priorities. The sparse authored foliage and repeated building shapes still differ substantially from that reference; this is not visual parity.

Reference context: the local environment direction is documented in `docs/environment-art-direction.md`. Blizzard's [Hunter class description](https://worldofwarcraft.blizzard.com/en-gb/game/classes/hunter) was checked as class-role context. Official Mage/Warrior and legacy Allods class page requests were unavailable during this pass; no inaccessible page was treated as evidence or used to copy numerical balance. No files were downloaded and no Blender source was regenerated.

## Verification and reproduction

Use the independent build directory `build-frontier-expansion/native` for this checkpoint. The shared `build-ninja-release` directory was receiving concurrent animation builds and is not the validation source.

`HighlandsTests` checks triangle-ground ballistics, feet, camera retraction, reachable NPCs and roads; six new skills at 30/144 Hz; level gating, preparation cancellation, area exclusion, root/exposure, cooldown persistence; guarded signals, optional-combat archive completion, reward accounting and malformed objective masks. It produces native region, atlas, journal, build and six slope-combat captures under `highlands-check`. `ClassEffectsTests` additionally compares native pixels against a hillside baseline to catch buried barrier/control auras, enemy ground attacks and dash-stop effects.

Release: 14 focused suites passed, including Highlands, FrontierExpansion, Navigation, Simulation, Geometry, ClassCombat, Combat, AttackTimeline, Interface, Shadow, Localization, Talent, Loadout and Trade. See `build-frontier-expansion/validated-release.log`. Full animation/character-art acceptance is separate; the first broad run also exposed failures in that concurrently edited workstream, so this record does not claim a fully green repository suite.

Debug: Highlands, Simulation, Navigation and ClassCombat passed (4/4). After final effect-grounding, renderer-capacity and integration corrections, Release Highlands/ClassEffects/CombatEffects/Shadow/ClassArtDecoder passed (5/5), and Debug Highlands/ClassArtDecoder passed (2/2). See `validated-debug.log`, `final-effects-tests.log` and `final-debug-tests.log` in `build-frontier-expansion`. Independent shader compilation matched the embedded bytecode; the localization source audit found no uncovered player strings. The final standalone `RPGNative.exe --smoke-test` run with the highlands snapshot also passed fullscreen restoration, device/shader creation, resizing, world/UI rendering and capture; see `final-world.png.log`.

Integration note: concurrent character-art changes introduced three additional material layers and class attachments during validation. Matched the authored material sentinel to the updated shader and fixed loading of zero shared normals caused by opposite face winding: reconstruct normals per face and split the affected corners, within the existing decoded-byte/vertex budgets. NaN, nonunit and degenerate inputs remain rejected. `ClassArtDecoderTests` covers that recovery and rejection. This pass did not edit or regenerate the Blender files or class-art archive.

Hidden 1280 x 720 Release workload on the local RTX 4060: village walking/orbit CPU frame p95 **2.269 ms**, GPU observed p95 **2.301 ms**; repeated-combat CPU p95 **2.037 ms**. End working set **362.016 MiB**, cached static scene **4,431 instances / 797,580 CPU bytes**. This measurement predates the final capacity increase and concurrent class-art integration. These are local observations from the existing four workloads, not a before/after speedup, a six-enemy highlands stress test or a guarantee of interactive FPS. Full report: `build-frontier-expansion/performance.txt`.

Manual acceptance: walk and jump both directions on each slope; orbit toward the hillside and behind buildings; inspect planted boots, warning shapes and projectile contact at different elevations. Play the new encounters without fixture shortcuts and assess pacing, pull distances and class balance. Inspect the atlas, all fifteen journal rows and nine spell cards in both languages at small and wide resolutions.
