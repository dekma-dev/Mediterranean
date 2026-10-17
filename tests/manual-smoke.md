# Native smoke test

Status: interactive checklist NOT RUN. Debug/Release builds, three CTest suites, and hidden DirectX rendering/capture pass as of 2026-10-17. Those automated checks do not replace this checklist.

Run `./scripts/build-native.ps1 -Configuration Debug -Test -Run` from PowerShell. Repeat the build/test step for Release after the Debug checks succeed. Record actual observations in `progress.md`; source inspection does not count as a pass.

1. Confirm the application shows a lit island, buildings, a blue player, red raiders, and readable HUD text. Check shader/device diagnostics if startup fails.
2. Resize, maximize, minimize, and restore the window. Confirm the view keeps its aspect ratio, the HUD remains usable, and rendering resumes.
3. Move with WASD, sprint, and jump. Try running and dashing into buildings and island edges. Confirm no tunneling or falling through the ground.
4. Orbit and zoom the camera. Move it toward buildings and trees and confirm that it pulls forward before intersecting their collision bounds. Small decorative dock/arena props are non-solid.
5. Hold a movement key, switch focus away, release it, and return. Confirm movement does not remain stuck and enemies did not damage the player in the background.
6. Press E at the gold beacon to accept the patrol. Approach a raider and cycle targets with Tab. Verify selected-target feedback and range rejection without consuming an attack cooldown.
7. Use strike and heavy strike; verify damage, cooldowns, enemy retaliation, and death. Check that melee attacks do not pass through buildings. Use mend after taking damage; full-health mend should not consume cooldown.
8. Defeat a raider, collect its loot with E, return, and claim the patrol reward. Repeated E presses must not add more loot or quest rewards. Also test defeating a raider before accepting the quest. Press G to equip the reward sword; confirm the character panel and subsequent damage reflect it.
9. Open C/I/J/M panels, switch between them, and close with Escape. The world should remain active behind panels. With no panel open, Escape should pause/resume simulation.
10. Use Q while injured and check potion consumption/cooldown. Save with F5, move or fight, then load with F9. Confirm position, health, enemy state, loot claims, quest, gold, potions, equipment, and cooldowns are restored. Confirm a malformed save is rejected without replacing active progress.
11. Die, check that movement and abilities are disabled, then press R to restart. Confirm quest, gold, enemies, and health reset.
12. Close the window and confirm the process exits. Review Debug output for DirectX warnings or errors.
13. Pull a raider around a building and beyond its home leash. Check navigation, separation between enemies, return behavior, and health restoration at home.

Known prototype limits: procedural blockout art and simple limb motion; no sound or skeletal animation; static-grid navigation and soft enemy separation; small decorative props are non-solid; limited inventory with two swords and one consumable type; no networking; no GPU performance measurements yet.
