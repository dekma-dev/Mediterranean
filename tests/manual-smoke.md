# Native smoke test

Status: NOT RUN. Requires an installed C++ toolchain and successful native build.

Run `./scripts/build-native.ps1 -Configuration Debug -Test -Run` from PowerShell. Repeat the build/test step for Release after the Debug checks succeed. Record actual observations in `progress.md`; source inspection does not count as a pass.

1. Confirm the application shows a lit island, buildings, a blue player, red raiders, and readable HUD text. Check shader/device diagnostics if startup fails.
2. Resize, maximize, minimize, and restore the window. Confirm the view keeps its aspect ratio, the HUD remains usable, and rendering resumes.
3. Move with WASD, sprint, and jump. Try running and dashing into buildings and island edges. Confirm no tunneling or falling through the ground.
4. Orbit and zoom the camera. Move it toward buildings and confirm that it pulls forward before intersecting them. Decorative trees are not currently camera collision objects.
5. Hold a movement key, switch focus away, release it, and return. Confirm movement does not remain stuck and enemies did not damage the player in the background.
6. Press E at the gold beacon to accept the patrol. Approach a raider and cycle targets with Tab. Verify selected-target feedback and range rejection without consuming an attack cooldown.
7. Use strike and heavy strike; verify damage, cooldowns, enemy retaliation, and death. Check that melee attacks do not pass through buildings. Use mend after taking damage; full-health mend should not consume cooldown.
8. Defeat a raider, return, and claim the reward. Repeated E presses must not add more gold. Also test defeating a raider before accepting the quest.
9. Open C/I/J/M panels, switch between them, and close with Escape. The world should remain active behind panels. With no panel open, Escape should pause/resume simulation.
10. Save with F5, move or fight, then load with F9. Confirm position, health, enemy state, quest, gold, and cooldowns are restored. Confirm a malformed save is rejected without replacing active progress.
11. Die, check that movement and abilities are disabled, then press R to restart. Confirm quest, gold, enemies, and health reset.
12. Close the window and confirm the process exits. Review Debug output for DirectX warnings or errors.

Known prototype limits: box-based placeholder art; no sound or skeletal animation; no enemy pathfinding or inter-character collision; inventory is a read-only summary; no networking; no GPU performance measurements yet.
