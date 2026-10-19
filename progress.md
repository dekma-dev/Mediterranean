# Mediterranean Progress

Champion build menu (2026-10-19): added an N-key talent map with a level-one foundation, connected Arms/Guard/Spirit branches, six inspectable upgrades, explicit level/prerequisite requirements, available-point count, XP progress and a Learn button. Level 2 grants one point and level 3 another. The champion can specialize through a two-rank branch or combine two first-rank talents. Learned choices affect actual attack damage, maximum health, protection, Mend healing and future Mend cooldowns; all four starting abilities remain available at level 1. Learning requires being alive, grounded and out of combat/action. It never restores health or resets an active cooldown. The HUD shortcut shows unspent points and level-up text points to N. Opening the menu captures movement/combat/camera input, cancels RMB orbit and keeps the world active.

Save version 8 records and validates learned talents after inventory. Versions 1–7 migrate with all earned points unspent; health is checked against the complete loaded build. Invalid bits, overspending, missing prerequisites, locked ranks and trailing records reject the entire load. Respawn retains choices; an explicit expedition reset clears them. Added TalentTests for real combat/healing effects, point restrictions and transactional migration, plus progression and scaled interface/input regression source. Updated legacy trade fixtures and registered champion-build smoke captures.

Validation: source review and git diff --check completed. The native Debug build/test command stops at `Visual Studio locator was not found.` The new tests, integrated game and menu captures have not run; appearance and interactive acceptance remain pending (manual checks 125–127). No downloads, installations, system changes, staging, commits or pushes.

Cloud depth and sunlight refinement (2026-10-19): replaced the flat low-cloud sheet with a bounded 48-sample procedural volume: height-shaped billows, fine erosion, front-to-back extinction, two light probes, cool interiors, brighter sun-facing edges and a restrained multiple-scattering approximation. Noise filtering accounts for pixel footprint and march length; static sub-step dither reduces slice bands. The sun gains a compact optical glow that follows cloud opacity. Reduced sky/horizon washout and added a cooler lower hemisphere. Broad cloud sunlight now uses twelve coarse samples per world vertex and interpolation instead of a volume march in every opaque pixel. Portraits bypass this lighting calculation. No new textures, render targets, asset downloads or constant-buffer growth in this refinement.

Validation: all eight SM5 entries compile without diagnostics. Nine 960x540 isolated WARP sky captures and GPU probes pass: aligned sun, exact 30-minute repeat, movement (maximum channel change 173 over 180 seconds), clear/opaque cloud transmission and no solar pinhole through dense cloud. Across 24 phases the maximum difference between sky and broad surface sun transmission is 29/255; a 60 Hz drift step changes mean RGB by 0.0079 levels. A separate 24-phase GPU probe limits island-diagonal light interpolation error to 15/255. Sun/cloud/horizon/clear/covered captures were inspected; results are in build-shader-check/sky-refinement-validation.log. Native Debug build/tests still stop at `Visual Studio locator was not found.` Full-scene appearance, camera-motion grain, vertex interpolation and GPU cost remain unverified; manual check 124 covers them. No commits, pushes or index changes by the agent.

Atmospheric daylight, sky and sun (2026-10-19): implemented a ground-level Rayleigh/Mie-style sky approximation, a pixel-filtered solar disc with forward-scattering glow, turbulent low clouds and thin high cloud streaks. The visible sun and scene light now share the source used by both shadow maps. Moving cloud coverage modulates direct sunlight; cooler sky fill, warm direct light, height-dependent distance haze and a common filmic response connect terrain and sky. Clouds use a bounded, closed 30-minute drift phase, driven by simulation updates. RMB can tilt upward to see the sun; the camera boom retracts to a 0.30 m floor clearance. Portrait lighting stays separate.

Validation completed: all eight SM5 shader entries compile through Windows' installed D3D compiler without diagnostics. The new project-only `scripts/check-scene-shader.ps1 -Preview` uses D3D11 WARP to render six isolated 960x540 sky diagnostics into `build-shader-check`; sun alignment, cloud movement (148 levels maximum channel change across 180 seconds) and exact drift-period repeat pass. Sun/horizon/cloud images were inspected and cloud coverage/shading refined. `git diff --check` passes. **Full native compilation and game integration are still unverified:** the Debug build/test helper stops at `Visual Studio locator was not found.` Added native sun occlusion, atmosphere timing and upward-camera regression source, pending the C++ toolchain. Shader/WARP evidence does not establish full-scene visual quality or hardware performance. Manual check 123 covers those gaps. No downloads, installations, system changes, staging, commits or pushes.


Champion equipment and RMB body steering (2026-10-19): implemented in source. New and migrated champions wear four additional real items: riveted shoulder guards, leather gloves, a belt with supply pouches and a blue travelling cloak. Each has its own slot, icon, ownership, equip/remove, sale/buyback and save behavior; world and portrait use the same outfit flags. Ten slots fit around the portrait, and the merchant shelf wraps thirteen item types over two rows. Travel pieces add no protection; existing combat tuning is retained. Save version 7 adds four worn slots and four buyback counts; versions 1–6 keep prior state and receive the new starting gear once.

Holding RMB over the world now steers the actual body heading with the filtered camera, including when stationary and moving sideways/backward. Existing turn smoothing and planted-foot correction animate the turn. Accepted strikes and active dashes retain their committed heading; UI/focus cancellation ends steering. Cursor hide/pin/release behavior is retained.

Source review and `git diff --check` pass. Regression source covers added-slot ownership/removal/full bags, save round trips and forgery rejection, v4/v5/v6 migration (including sold rewards), every catalog item's persisted buyback, scaled new-slot/right-click/drag and second-row buyback, stationary left/right turns at 30/60/144 Hz, strafing/backing, focus/modal cancellation, nonfinite headings and committed combat. Added `equipment-ten-slots-WIDTHxHEIGHT.png` capture diagnostics. **These tests and visual diagnostics have not run:** `.\scripts\build-native.ps1 -Backend Ninja -Configuration Debug -Test` stops at `Visual Studio locator was not found.` The previously recorded missing compiler/SDK remains the validation blocker. Manual checks 121–122 cover appearance and live controls; no runtime/performance claim is made for this source change. No downloads, system changes, commits or pushes.


Last updated: 2026-10-19

## Current status

Hand-drawn atlas/minimap (2026-10-19): replaced the shaded overhead illustration with a parchment ink-and-wash survey. The shared 512-square texture now draws sepia coast contours and cliff hachures, pale outlined routes, grass strokes, scalloped tree silhouettes, sketched cottages/ruins, a striped stall and dock planks. Full atlas presentation adds paper margins, dark unshadowed lettering, leader lines, an eight-point compass rose and a five-metre scale. Objective rings gain a dark outline for contrast on light paper. Coastline, surface routes, prop locations, markers and waypoint projection still use the existing world data; GPU texture dimensions/mips are unchanged.

Source review and `git diff --check` pass. Added `atlas-drawn-WIDTHxHEIGHT.png` diagnostics to the existing scaled interface checks, but **native builds, tests and visual review have not run**: the MSVC/Windows SDK blocker recorded below remains. Manual check 120 covers the atlas/minimap appearance, marker alignment and waypoint interactions. No downloaded artwork, system changes, staging, commits or pushes.

RMB cursor capture (2026-10-19): implemented in source. A camera gesture begun over the world hides the native cursor and pins it to its original screen position; foreground raw mouse deltas continue camera rotation at screen edges. Releasing RMB frees and restores the cursor at that position. Menu/UI capture, load, defeat, focus/capture loss, resize, DPI/fullscreen transitions, shutdown and rendering errors release it; a cancelled gesture requires a fresh press. Inventory/map right-click actions keep normal pointer behavior. Added raw packet accumulation/absolute-device baselines, fixed-cursor camera, release/focus/menu/load/display cancellation and hidden-window guard regressions to InterfaceTests.

**Validation for this change is blocked:** the Debug build helper stops with `Visual Studio locator was not found.`; no MSVC compiler or Windows SDK headers were found in the installed tool locations, including a check outside sandbox restrictions. The prior fifteen-suite results below belong to earlier revisions. The new tests, Debug/Release builds and physical cursor behavior have not been verified. Source/diff review and `git diff --check` pass. Build attempt: `build-ninja-debug/cursor-lock-validation.log`. Manual acceptance: check 119. No downloads, system changes, staging, commits or pushes.

Motion/jump/spell blending (2026-10-19): body lean, travel intensity and rising/falling presentation now preserve blend velocity through critically damped springs. Landing uses a zero-slope compression/recovery envelope (90 ms peak, 420 ms recovery) instead of an immediate-slope impulse. Accepted Mend events drive a 720 ms upper-body casting gesture with a raised free hand and smooth recovery; rejected/full-health/cooldown requests do not invent gestures. Healing, movement and collision remain simulation-owned. Tests cover accepted spell timing, rejected repeated input, frame-rate agreement, reset behavior, zero-slope landing boundaries and limb/weapon safety with casting. Added cast-rise/peak/recovered diagnostic captures. Manual acceptance: check 118. All fifteen suites pass in Debug and Release; both generated 132 smoke captures (gesture-validation.log), and final expanded Debug tests pass in gesture-final.log. The casting peak was visually inspected. Subjective motion feel remains a live-play check. Whitespace checks pass; no staging, commits or pushes performed by the agent.

Village architecture detail (2026-10-19): added physical foundation/corner masonry, projecting sills and lintels, glazing bars, shutter hinges, planked doors with straps/rivets and chimney courses/flue. Existing reusable meshes/materials retained. Village capture inspected; rendering-cost increases documented in docs/graphics.md. Manual acceptance: check 117. Debug and Release each pass all fifteen suites and 132 smoke captures (village-detail-validation.log). Whitespace checks pass. No downloaded assets; no staging, commits or pushes performed by the agent.

Geometry/interface detail pass (2026-10-19): increased shared character tessellation and profile detail, recessed eye sockets, layered abdominal/shoulder armor, rivets, knee guards, thumbs and finger ridges. Added layered frame corner brackets and engraved banner borders. Close-up and character/bag renders inspected. Mesh storage grows from 224,320 to 278,112 bytes; no new texture allocations. Before/after workload measurements and remaining art gaps are recorded in docs/graphics.md. Manual acceptance: check 116. Debug and Release pass all fifteen suites and generate 132 smoke captures each (geometry-detail-validation.log); the final tighter geometry budget also passes Release in geometry-detail-final.log. Whitespace checks pass. No files downloaded; nothing staged, committed or pushed by the agent.

Empty inventory selection fix (2026-10-19): reproduced an empty-cell click retaining the previous inspected item and its use/equip action (InterfaceTests failed in empty-selection-repro.log). Empty bag cells now emit a clear-selection event while preserving drag destination metadata. Empty cells do not emit use commands or item tooltips. Scaled tests cover selection clearing and disabled item actions at 960x600, 1280x720 and 2560x1080; existing drag/drop checks cover destination behavior. Manual acceptance: check 115. Debug and Release each pass all fifteen suites (empty-selection-validation.log), including scaled clicks and existing drag/drop tests. Whitespace checks pass. Full smoke batches were not repeated for this input-state fix. No staging, commits or pushes performed by the agent.

Character/bag interface implementation (2026-10-19): replaced oversized attribute tiles with a live vitality meter and aligned combat rows, added within-level XP progress and remaining XP, level-cap state, right-aligned currency/consumables and occupied equipment count. Bag now reports free slots and presents selected items in an inset inspection card with an enlarged icon, type and compact comparison. Empty selection has explicit instructions and a disabled selection action. Existing drag, gear, merchant and button hitboxes remain intact. Manual acceptance: check 114. Character/bag captures inspected at 1280x720 and 960x600. Debug passes all fifteen suites and 132 smoke captures; Release passes the same before the final comparison-label wording correction, with a final build/test run recorded separately in character-bag-final.log. No new assets or downloads.

Dash interaction ordering fix (2026-10-19): reproduced a same-frame dash/purchase spending gold before dash activation (TradeTests failed in dash-order-repro.log). Dash now activates before ground-only interaction and transaction checks; other abilities retain their existing order. Regression tests cover rejected purchase/stock mutation and sparring entry on activation. Manual acceptance: check 113. Debug and Release each pass all fifteen suites (dash-order-validation.log); whitespace checks pass. Full smoke batches were not repeated for this simulation-order fix. Nothing staged, committed or pushed.

Interface art/layout pass (2026-10-19): compared the supplied Allods reference against character/bag and menu views. Reduced Escape panel area by about 38%, regrouped controls and reduced menu dimming. Added darker teal recesses, brighter ivory labels, warm trim highlights, rivet corners and less repeated rail ornament. Empty-slot engraving is smaller; weapon, potion, cloth and helmet icons gain material/edge details. Character/world art and character/bag layout remain prototype-level gaps, documented explicitly in `docs/visual-target.md`. Final Release character/bag and menu captures were visually inspected; manual acceptance is check 112. Debug and Release each pass all fifteen suites and generate 132 smoke captures (interface-art-validation.log). No new asset downloads or performance claims; nothing staged, committed or pushed.

Timed skill-3 dash (2026-10-19): replaced the instant 3.5 m displacement with a 0.28 s smoothstep movement burst, swept on every update. Contact stops the remaining dash; focus/menu capture, death/respawn and load clear its transient state. Added a forward-leaning, lowered, split-leg dash pose and suspended planted walking contacts/footstep distance during the burst. Cooldown/audio still trigger once; shop, sparring and channel entry reject active dashes. New tests cover continuous travel/range at 22–144 Hz, pose lifetime, suspension/load reset and wall collision. Added dash-start/peak/finish.png captures; the peak pose was visually inspected. Live acceptance is check 111.

Final Debug/Release each pass all fifteen suites (`timed-dash-validation.log`), including the existing combat-evasion test. Accepted combat attacks retain their facing during a dash. Whitespace checks pass. Focused dash/render diagnostics were generated; full smoke batches were not repeated. Nothing staged, committed or pushed.

Audio mix/variation pass (2026-10-19): replaced permanent twelve-voice attenuation with an active-voice peak budget and smooth gain recovery. Footsteps sit behind combat; repeated cues have restrained deterministic pitch variation without additional PCM. Weapon hits/swings use shorter inharmonic metal resonances instead of spell-like bells. Atomic XAudio2 start/pitch/volume commits prevent new voices starting at the previous sparse mix's gain. AudioTests adds overlap/headroom, rate-independent gain recovery and pitch-range checks, a seven-second combat-mix-preview.wav audition, and master-muted scheduling/lifecycle exercise. Auditory quality remains unverified; manual check 110 covers the mix in play.

Final Debug and Release builds pass all fifteen suites (`audio-mix-validation.log`). Resident PCM remains 1,759,146 bytes; twelve effect voices are retained. Whitespace checks pass. Full visual smoke batches were not repeated for this audio-only pass. Nothing staged, committed or pushed.

Locomotion coherence pass (2026-10-19): corrected the mismatch between desired facing and momentum during reversals by deriving locomotion heading from collision-resolved travel. Accepted attacks retain combat facing. Added bounded acceleration/braking torso lean driven by actual velocity changes, and reduced camera jump tracking to 35% of actor height. No collision shapes, jump trajectory, mesh or texture budgets change. New checks cover reversal heading, lean sign/reset, jump-camera bounds/settling and limb/weapon geometry under extreme lean combinations. Live feel remains manual check 109; this is not a general rigid-body physics implementation.

Debug and Release pass all fifteen suites and generated 132 smoke captures each (`locomotion-weight-validation.log`); Debug's final expanded lean checks pass in `locomotion-weight-final.log`. Jump-apex framing was visually inspected. Whitespace checks pass. Nothing staged, committed or pushed.

Grounded-state bug fix (2026-10-19): reproduced a purchase during the exact zero-height, positive-velocity start of a buffered rebound (`rebound-grounded-repro.log`, TradeTests failed before the fix). Shop, sparring and anchor interaction checks previously tested height alone. They now use a shared Grounded predicate that also requires non-positive vertical velocity; channel validation, gait, foot placement and footstep collection use it too. Added exact-contact regressions for purchasing, sparring and anchor stabilization. Live acceptance is manual check 108.

Debug and Release each pass all fifteen regression suites (`rebound-grounded-validation.log`), including existing collision, animation, audio and foot-placement checks. Whitespace checks pass. Full smoke batches were not repeated for this state-check fix. Nothing staged, committed or pushed.

Buffered-jump movement pass (2026-10-19): a Space press within 120 ms before landing triggers at contact, with the remaining frame integrated as airborne motion. Rebounds preserve horizontal momentum and avoid ground-braking/gait artifacts. Focus suspension, menu capture, defeat/respawn and loading clear queued input. Zero-height positive-velocity takeoff is now airborne for pose/foot-placement state. Tests cover 22/30/60/144 Hz, expiry, cancellation, save/load reset, exact rebound height/velocity, running momentum and one landing/takeoff sound pair without footsteps. Manual feel remains check 107.

Final Debug and Release builds pass all fifteen regression suites (`jump-buffer-validation.log`), including animation/foot-placement rendering diagnostics. Full 132-scene smoke batches were not repeated for this input/contact change. Whitespace checks pass; nothing staged, committed or pushed.

Movement-weight pass (2026-10-19): changed jump from 6 m/s with 18 m/s² gravity to 7.2 m/s with 24 m/s² gravity (0.60 s flight, 1.08 m apex). Slow directional jumps receive a leg-driven horizontal push that tapers with current speed; running jumps preserve momentum. Air steering retains the takeoff speed limit, preventing Sprint toggles from adding propulsion in flight. Movement capture now rejects takeoff. Presentation adds stronger knee tuck, landing compression and body pitch; capture timing follows the revised arc. Motion tests cover the new launch, air Sprint lock and capture behavior alongside exact landing/traction and collision checks; animation fixtures use the new impact speed.

Debug/Release pass all fifteen suites; both generated 132 smoke captures for the revised arc/poses (`jump-weight-validation.log`). Rise and landing captures were visually reviewed. A final continuous-launch adjustment adds a near-rest input-timing regression and passes all fifteen suites again in each build (`jump-launch-final.log`); those suites include the collision and foot-placement diagnostics. Live control feel remains manual check 106. No commits or pushes.

Worn-surface graphics pass (2026-10-19): refined paving shape/relief, joint moss and ambient cavity shading; added mineral/pit detail in stone, restrained cloth folds and brushed metal variation. Texture/mesh allocations and submission counts remain unchanged. Initial tests caught metal roughness below the anti-shimmering floor; it was corrected rather than relaxing the regression. Visual review also reduced excessive cloth fold contrast. Character anatomy and architecture remain prototype art, below the Allods reference.

Final Debug and Release builds pass fifteen suites and 132 smoke captures each (`surface-validation.log`). Village and character views were visually inspected. The short before/after benchmark and its merchant-tail limitation are recorded in `docs/graphics.md`; live acceptance is manual check 105. Nothing staged, committed or pushed.

Sound-design pass (2026-10-19): rebuilt weapon swings, heavy/light impacts, incoming damage, interruption, Mend, potions and movement sounds with distinct transient/body/tail layers. Mend has ascending chimes and quiet baked reflections; potions have bottle/bubble cues. Synthesis remains deterministic and startup-only, with 1,759,146 bytes of resident PCM and the existing twelve-voice overlap headroom. Audio regression checks cover quiet edges, sustained healing tails, distinct cues, memory, accepted events and muted device lifecycle. Listening acceptance remains manual check 104; no downloaded assets or runtime reverb were added.

Debug and Release builds pass all fifteen regression suites (`sound-design-validation.log` in each Ninja directory), including muted XAudio2 startup/suspension/resumption. AudioTests exports the updated `effects-preview.wav`. Full smoke captures were not repeated for this audio-only change. Nothing staged, committed or pushed.

The active `main` working tree is native C++ / DirectX 11 only. Allods Online is the primary reference; development is single-player first on one map. **Debug and Release Ninja/MSVC builds pass all fifteen regression suites and 132 hidden smoke captures per configuration, including world/interface layouts, outfits, sparring, the shop, buyback, eight close-up poses and seven environment views.** Representative captures were visually inspected. The map has two quests, levels 1–3, a captain, telegraphed combat, progress-preserving respawn, repeatable nonlethal training and a village quartermaster. The 24-cell bag supports drag/sort, stack splitting and six worn slots; bought helmet/shield change visuals and protection. Selling and persistent buyback conserve ownership and gold. Version 6 preserves exact stacks, gear, purchase stock and merchant-held items and migrates versions 1–5. Character presentation uses shaped heads/boots, fitted hair, fixed-length limb poses and hand-attached equipment. Player/enemy locomotion and camera transitions now have a tested smoothing pass. Full interactive playtesting and finished art remain pending. Repository HEAD is `e4ea84d` (`0.0.3 - Add combat trails, impact sparks and interrupt effects with lifecycle safeguards`), committed externally. The agent has not staged, committed or pushed; the current foot-placement work and documentation remain in the working tree.

Current environment work adds paved/dirt routes shared with the atlas, 131 grass clumps, 18 shrubs, folded leaf crowns and a dock footprint clear of neighboring props. The focused shadow map adds 4 MiB; combined material/terrain/shadow storage is 9.250 MiB against a revised 10 MiB test budget. Geometry and interface allocations are unchanged. The latest before/after workload report is in [graphics.md](docs/graphics.md).

The committed landmark pass gives all four buildings distinct palettes/details, adds a covered supply stall and masonry ruin, and presents the keeper as a dressed NPC. Camera regressions keep the stall's open bay and above-canopy space clear. That art added about 58 KiB of mesh storage and no additional textures.

## Anchor drift and potion hint fixes - 2026-10-19

Reproduced player drift after starting anchor stabilization: the channel checked movement input, but approach momentum survived after input release. A validated channel now clears horizontal velocity while preserving this frame's actual travel for animation; rejected/interrupted attempts retain normal movement behavior. ProgressionTests checks that position remains fixed on the next channel update, and failed before the fix (`channel-tooltip-repro.log`).

Corrected the bag potion hover hint in the merchant view: drinking from the bag is supported there, so the tooltip now reports health/cooldown/right-click availability instead of incorrectly asking the player to close the shop. The existing selected-stack merchant regression covers the supported action. Debug and Release each pass fifteen suites and 132 hidden smoke captures (`channel-tooltip-validation.log` in both Ninja directories). Whitespace checks pass; live acceptance remains manual check 103. Nothing staged, committed or pushed.

## Combat HUD readability - 2026-10-19

Target frames now display numeric health, target distance and an enemy windup countdown/progress strip. Action slots show draining cooldown masks, recovery strips, tenths in the last second, readiness accents and potion stock. Ability cards include cooldown duration, melee range or potion stock, and colored status/remaining time. The expedition tracker gains a dark backing for contrast against the world. Indicators are stateless/read-only and existing action hitboxes remain unchanged.

New diagnostics at 960x600, 1280x720 and 2560x1080 cover a selected attacking enemy and a cooling Strike tooltip. Tests assert cooling abilities remain unclickable and rendering preserves gameplay state. All three diagnostics and the gameplay view were inspected. Debug and Release each pass fifteen suites and 132 standard smoke captures, plus the new HUD diagnostics; logs are `combat-interface-validation.log` in both Ninja directories. Whitespace checks pass; manual check 102 covers live readability, which remains pending. Nothing staged, committed or pushed.

## Combat effect refinement - 2026-10-19

Slashes now shed five fine edge fragments, hit sparks fan away from the struck surface toward the attacker, and analytic drag slows their horizontal motion as gravity bends the trajectory. Sparks shift toward warm embers or violet as they fade. Heavy impacts and interrupts gain a delayed colored pulse; the main flash/halo is reduced to retain target readability. Healing and enemy warning shapes remain unchanged. State pools, lifetimes, texture storage and the 6,144-vertex upload cap remain fixed; no downloads or new assets.

Debug and Release each pass fifteen suites, 132 standard smoke captures and existing effect diagnostics, including equal-time 30/60/144-Hz rendering, depth/HUD isolation, expiry and saturated-buffer handling. Impact and fade captures were inspected; whitespace checks pass. The hidden combat workload peak increases from 534 to 564 effect vertices without additional effect draws. Logs are `effects-refinement-validation.log` in both Ninja directories; manual check 101 covers live readability, which remains pending. Nothing staged, committed or pushed.

## Weathered building materials - 2026-10-19

Added subtle limewash/rain/foundation staining to plaster, individual roof-tile brightness variation and less regular timber grain with reduced relief. Existing textures and meshes are reused; no downloads, added texture storage or draw calls. Village and stall captures were inspected. This improves surface variation rather than replacing the prototype geometry or reaching finished Allods-quality art.

Debug and Release each pass fifteen suites and 132 smoke captures (`material-weathering-validation.log` in both Ninja directories); whitespace checks pass. Short before/after workload GPU median changes range from +0.002 to +0.011 ms; details and limitations are in `docs/graphics.md`. Manual check 100 covers moving views and live performance. Nothing staged, committed or pushed.

## Focus and mouse-menu momentum fixes - 2026-10-19

Reproduced two regressions from physical momentum: losing focus retained velocity, and mouse-opened menus halted movement one frame late because command capture was read before the pointer action. The engine now calls `Game::SuspendInput()` before waiting while unfocused/minimized, clearing motion without advancing simulation or changing durable state. Unfocused Game updates also halt movement. Pointer-driven modal capture is evaluated after dispatch, so both opening and closing menus affect movement in the same update.

MotionTests covers focus loss/return and repeated suspension without a Game update; InterfaceTests drives real rendered HUD/menu hit regions while holding W. Both original regressions failed before the fixes (`input-momentum-repro.log`). Debug and Release each pass all fifteen suites and 132 hidden smoke captures (`input-momentum-validation.log` in both Ninja directories). Whitespace checks pass; manual check 99 covers live desktop behavior, which remains unverified. Nothing staged, committed or pushed.

## Contact-timed jump physics - 2026-10-18

Takeoff now switches horizontal control to the airborne response before movement, removing a frame of ground acceleration. Landing steps split at the analytically solved floor-contact time: air momentum advances to contact, then ground traction/braking advances through the remaining interval. Both intervals retain swept obstacle collision. A separate transient grounded-distance counter prevents airborne travel in landing frames from advancing walking gait or footstep sound timing; death/respawn/load clear it.

Motion regressions compare takeoff displacement, ballistic distance plus ground braking, grounded travel and single landing events against analytic expectations at 22/30/60/144 Hz. The 22-Hz case exercises landing inside a frame. Debug and Release each pass fifteen suites and 132 hidden smoke captures; logs are `landing-physics-validation.log` in both Ninja directories. Whitespace checks pass. Manual check 98 covers takeoff/landing feel; live playtesting remains pending. No assets or dependencies added; terrain remains flat. Nothing staged, committed or pushed.

## Player momentum and airborne control - 2026-10-18

Replaced scalar player speed with transient horizontal velocity. Ground direction changes now brake existing momentum, key release produces a short stopping step (roughly 0.125 units walking / 0.219 sprinting), and airborne release retains takeoff momentum. Air steering responds more slowly than ground steering. Exact exponential displacement integration keeps constant-target movement consistent across update rates. Swept collision removes inward velocity while preserving travel along walls; menu capture, death, respawn and loading clear movement appropriately. No assets, dependencies or per-update allocations were added.

Expanded MotionTests covers turning/braking trajectory agreement at 30/60/144 Hz, bounded stopping distance, airborne momentum/reversal, wall contact/tangential movement and load resets. Trade atomicity fixtures now finish braking before snapshot comparisons so normal movement does not masquerade as a transaction mutation. Debug and Release each pass fifteen suites and 132 smoke captures; logs are `physics-momentum-validation.log` in both Ninja directories. Whitespace checks pass. Manual check 97 covers feel and interaction; live playtesting remains pending. This remains a flat-ground character controller: no rigid-body props, slopes or climbing. Nothing staged, committed or pushed.

## Movement audio bug fixes - 2026-10-18

Reproduced a missing dash cue when the cooldown expires on the same simulation update as a new dash. Jump/dash playback now reads explicit accepted-action flags instead of inferring actions from height and cooldown history. This also preserves actions on an audio-reset frame without inventing cues from a loaded cooldown or an already ascending jump. Flags clear before every update, including rejected time steps, and are not serialized.

Audio regressions cover cooldown-boundary reuse at 30/60/144 Hz, collector resets on action frames and during ascent, loading with a nonzero cooldown, and clearing cues on zero-time updates. The cooldown-boundary test failed before the fix. Debug and Release each pass all fifteen suites and 132 hidden smoke captures (`bugfix-validation.log` in each Ninja build directory); muted XAudio2 lifecycle checks also pass. Whitespace checks pass. Audible playback remains manual check 96. Nothing staged, committed or pushed.

## Music and gameplay audio - 2026-10-18

Debug and Release each pass fifteen suites and 132 standard smoke captures, plus existing diagnostic images. Muted XAudio2 startup/suspend/resume succeeded on the available endpoint in both builds, and the menu capture was inspected. Diff whitespace checks pass. No audible listening assessment was performed.

Added native XAudio2 playback, an original 32-second ambient pad/plucked-melody loop and thirteen procedural cue types: light/heavy/enemy swings, impacts, hurt, interrupt, Mend, potion, footsteps, jump, landing and dash. Confirmed combat events drive cues; movement cues follow distance/physical transitions. Resident mono 22,050-Hz PCM totals 1,623,320 bytes (about 1.55 MiB), generated once at startup, without downloads. Twelve effect voices bound overlap, and peak-derived effect gain reserves music headroom. This is synthesized prototype audio, not recorded instruments or a finished soundtrack; spatial panning, reverb and adaptive battle music remain future work.

Escape has separate Music/Effects +/- controls, with 0 muting the channel. Settings last for the running session and are not added to saves. Losing focus/minimizing suspends playback and clears effects; resuming preserves the music position. Reload/restart/respawn clears stale effect history. Hidden smoke/benchmark engines do not initialize audio. Initialization failure leaves gameplay available and marks audio unavailable in the menu; live device switching/recovery is not implemented.

AudioTests checks deterministic non-silent PCM, fades/headroom, memory bounds, accepted/rejected cues, landing/dash/footstep timing and save equivalence. It writes `dawnwatch-music.wav` and `effects-preview.wav`; these are dry previews before runtime volume controls. A device test starts at zero volume and exercises suspend/resume if an endpoint is available, otherwise it checks silent fallback. Menu routing is checked at three resolutions. Validation logs are `audio-validation.log` in both Ninja directories. Auditory quality and actual speaker/headphone balance remain manual checks 93–95. Nothing staged, committed or pushed.

## Native fantasy cursor - 2026-10-18

Debug and Release each pass fourteen suites and 132 smoke captures, including the new cursor assertions. The enlarged cursor preview was visually inspected; live desktop input remains pending. Diff whitespace checks pass.

The game client now uses a locally generated emerald arrow with a gold rim and dark outline. A native cursor handle keeps pointer movement independent of the render rate. Its premultiplied-alpha edges use 4x4 coverage sampling; the 32-pixel design scales with window DPI (up to 128 pixels), and the hotspot follows the arrow tip. DPI changes rebuild the owned handle; window borders retain standard system cursors, creation failure falls back to the normal arrow, and shutdown releases cursor resources. No system cursor settings, downloads or external assets are involved.

InterfaceTests checks creation, size, hotspot and transparent/solid/antialiased pixels at 96/144/192/384 DPI and writes an enlarged `cursor-preview.bmp`. Logs are `cursor-validation.log` in both Ninja directories. GPU smoke captures do not include the native OS cursor; actual window entry/exit, clicks and mixed-DPI movement are manual check 92. Nothing staged, committed or pushed.

## Equipment comparison hover cards - 2026-10-18

Debug and Release pass all fourteen suites, 132 standard smoke views, the existing effect/foot diagnostics and six new tooltip captures each. Bag/shop cards were visually inspected at 960x600, 1280x720 and ultrawide. Diff whitespace checks pass; live input acceptance remains pending.

Bag/equipment hover cards now show an item icon, level-adjusted weapon damage or protection, a colored comparison against actual worn gear, description, stack/ownership count, sale value and contextual action. Green means improvement, red means downgrade, and worn items are labeled explicitly. Buy buttons show the same comparison before purchase, with price and availability. Potion hints reflect full health, cooldown, defeat and the shop restriction. Cards clamp to the reference viewport and choose the opposite side of the pointer; they retain existing hit-order/drag suppression and do not consume clicks or change simulation state.

New checks verify upgrade/downgrade values, currently equipped labels, empty-slot shop comparisons and potion cooldown text. Six new bag/shop captures cover 960x600, 1280x720 and 2560x1080; hover/save equivalence and existing pointer routing remain tested. Logs are `item-tooltip-validation.log` in each Ninja directory. Manual check 91 covers live readability. Nothing staged, committed or pushed.

## Swept character collision - 2026-10-18

The deterministic sweep grid covers 4,592 valid-start cases. Debug and Release each pass fourteen suites and 132 smoke views, plus existing effect/foot-placement diagnostics. A short Release workload pair retains gameplay counters and graphics resources, with CPU medians 0.009–0.052 ms higher; this does not isolate collision cost. Detailed timings/limitations are in [graphics.md](docs/graphics.md). Live input acceptance remains pending.

Replaced X-then-Z stepping with continuous swept-circle movement against static rectangle faces, rounded corners and island boundaries. Actors move to the first contact, then project remaining movement onto the surface tangent. Walking, enemy navigation/separation and dash all use the shared path. A 0.0001-unit contact clearance prevents numerical sticking, broad-phase bounds skip distant obstacles, and four contacts bound each move without allocation. Final positions are checked before publication. This remains flat-map kinematic physics: jumping still uses the existing analytic gravity/landing solver, static props do not move, and slopes/stairs/rigid bodies are not implemented.

New navigation regressions cover reaching walls while retaining tangent travel, long movement through buildings/thin trunks, both island edges, mirrored rounded corners, moving away from contact, invalid input and 30/60/144-Hz wall-slide agreement. A deterministic grid of dash-length sweeps checks final clearance and displacement bounds. Full validation logs are `swept-physics-validation.log` in both Ninja directories. Manual check 90 covers actual feel around geometry. Nothing staged, committed or pushed.

## High-update-rate blade trail fix - 2026-10-18

Debug and Release pass all fourteen suites and 132 smoke captures per configuration, plus 39 effect diagnostics and twelve foot-placement views. The repaired fixture measures 304/301/301 pixels at 60/144/1000 Hz; the high-rate capture was inspected. Diff whitespace checks pass.

Reproduced a presentation bug: storing a blade sample every update exhausts the 32-slot history above roughly 200 Hz, shortening the intended 160 ms trail. A rendered linear-sweep fixture measured widths of 304/301/76 pixels at 60/144/1000 Hz before the fix (`trail-history-before.log` in Debug). Samples are now committed no faster than 120 Hz while the newest endpoint stays live. The fixed pool size, lifetime, damage, attack timing and save data remain unchanged. This improves duration consistency rather than promising identical samples at every rate.

Three new render regressions retain visible trail history at 60/144/1000 Hz; existing discontinuity, expiry, overload, reload and gameplay tests remain active. CombatEffectsTests now produces 39 diagnostics, alongside twelve foot-placement views and 132 smoke images. Validation logs are `trail-history-validation.log` in both Ninja directories; manual check 89 covers actual high-refresh play. Nothing staged, committed or pushed.

## Instance submission optimization - 2026-10-18

Debug and Release each pass fourteen regression suites and 132 smoke captures, plus the existing effect/foot-placement diagnostics. Logs are `transform-validation.log` in both Ninja directories. Diff whitespace checks pass; live fullscreen testing remains manual check 88.

Scene submission now derives the normal transform from the scaled orthonormal basis instead of computing a general 4x4 inverse for every object. The same three squared row lengths provide culling radius scale with one square root instead of three. Both private callers supply rotation/scale/translation without shear; future sheared transforms require a general inverse path. Nonuniform-scale/rotation tests compare normalized normals against the previous inverse across 120 transforms, and verify bounding radii.

Three sequential hidden Release runs before and after show median-of-run CPU medians of 0.479 → 0.464 ms stationary, 0.459 → 0.439 ms walking, 0.489 → 0.469 ms combat and 0.914 → 0.904 ms merchant (about 3%, 4%, 4%, 1%). These small gains are workload measurements, not guaranteed fullscreen FPS improvements. GPU work/resources, submission/triangle peaks and gameplay counters are unchanged. Of 132 smoke images, 113 remain byte-identical; the remaining nineteen differ by one intensity level in only one or two color channels per entire image. Old Debug images were verified against baseline Release hashes before comparison. See [graphics.md](docs/graphics.md) for timings and artifacts. Nothing staged, committed or pushed.

## Cubic shadow filtering - 2026-10-18

Debug/Release pass all fourteen suites and 132 standard smoke captures. Debug's tightened-threshold rerun is recorded separately in `shadow-cubic-final-tests.log`; full build/smoke logs are `shadow-cubic-validation.log` in both directories. Contact/village captures were inspected. A short before/after Release comparison retains resource/draw counts and shows GPU medians within 0.004 ms, but merchant GPU p95 rises from 0.534 to 0.687 ms; see the full table and limitations in [graphics.md](docs/graphics.md). Nothing staged, committed or pushed.

Replaced the linear tent weights with normalized cubic B-spline weights in both shadow layers. The filter retains sixteen point depth comparisons and per-tap receiver-plane correction, with continuous first/second weight derivatives across texel boundaries and a tighter central lobe. This improves filtering continuity and thin/contact contrast; it does not eliminate rasterization shimmer or implement variable-distance penumbrae. Shadow resolution, bias, projection, GPU allocations and draw counts are unchanged.

In the existing eight-position thin-post fixture, minimum/mean brightness contrast rises from 34.33/45.33 to 41.00/56.96 (8-bit image units). The 0.05-unit contact fixture rises from 54.67 to 61.00. Snapped-grid motion remains at a maximum one-unit pixel difference. The near/far blend's maximum step rises from 10 to 18, still below the existing 25-unit limit; sharper edges make the resolution transition more visible. The thin-post acceptance floor is now minimum >35 and mean >50. Existing slope, contact, offscreen caster, resize, shadow clearing and portrait isolation regressions remain active. Manual check 87 covers the visual tradeoff in motion.

## Camera distance smoothness - 2026-10-18

Debug and Release each pass all fourteen regression suites, 132 standard smoke captures and the existing 48 effect/foot-placement diagnostic captures. The diff whitespace check also passes.

Replaced first-order camera-distance easing with an exact critically damped spring. Wheel zoom starts gradually and retains velocity when reversing; outward recovery also accelerates smoothly after the existing 100 ms clearance delay. Obstruction retracts immediately and discards momentum, preserving collision safety. Explicit camera placement/reset clears velocity. Invalid time or nonfinite targets are ignored. Gameplay movement, orbit steering, foot placement and save data are unchanged.

A new regression first failed against the old camera on abrupt zoom startup. New checks also cover reversal velocity, convergence and equal-time 30/60/144-Hz results, collision during outward motion, recovery hold and invalid elapsed time. Validation logs are `camera-smoothness-validation.log` in each Ninja directory. Manual check 86 covers actual wheel input and obstacle traversal; live feel remains pending. The controller adds one float with no per-frame allocation or new graphics resources. Nothing staged, committed or pushed.

## Recovery effects - 2026-10-18

Mend now emits green ascending ribbons and motes; potions use a smaller gold pulse. Simulation emits a transient event only after successful healing. The effect remains at the activation position and expires in 0.8 seconds; healing amounts, inventory consumption, cooldowns and save version are unchanged. Shared burst replacement now compares normalized lifetime so an expired short impact is reused before a still-active recovery pulse.

Debug and Release pass all fourteen suites and 132 standard smoke views. CombatEffectsTests now generates 36 diagnostics, including eight new recovery captures; the twelve foot-placement diagnostics are retained. Recovery tests cover accepted and rejected actions, healing/save equivalence, expiry, reload, mixed-lifetime pool reuse, fixed geometry and equal-time 30/60/144-Hz rendering. Mend/potion gameplay captures were visually inspected. Logs are `recovery-validation.log` in each Ninja directory. Manual checks 84–85 remain pending. No new textures, meshes, shaders or GPU buffers; fixed effect state stays at 8,520 bytes. Each visible Mend submits 306 vertices and a potion 162 into the existing capped batch. No new sustained-performance claim or benchmark is made. Nothing staged, committed or pushed.

## Planted-foot movement - 2026-10-18

Added a transient foot-placement controller for the player, campaign enemies and trainer. Supporting feet retain world position and boot heading, then follow lifted steps with eased horizontal movement. Turning and stopping reposition the feet, arms counter-swing with actual ankle travel, and pelvis compensation preserves fixed leg lengths. Jumping releases contacts; landing, large displacements, reload/restart/respawn and a new sparring round rebuild them. The current floor remains flat, and movement, collision, attack timing, damage and save data are unchanged.

The first rate comparison caught a different stepping sequence at 144 Hz. Contact decisions now run at fixed 240 Hz with interpolated actor positions, bounding swing sampling lag to 4.17 ms. Final straight-path ankle differences at equal time are below 0.000006 world units across 30/60/144 Hz; maximum supporting-foot drift in the tested straight/curved paths is below 0.0000005 units. These are numerical fixture results, not a guarantee for every interactive transition. Abrupt reversals, stationary turns, settling, airborne/dead states, teleports, malformed targets, fixed limb lengths, floor/weapon clearance and render immutability also have checks.

Debug and Release pass all fourteen suites, 132 ordinary smoke captures, 28 combat-effect diagnostics and twelve new foot-placement captures without compiler warnings/errors. Logs are `foot-placement-validation.log` in each Ninja directory. Close-up support/recovery/run/turn/landing views and live walking/enemy/combat views were inspected; manual checks 81–83 cover physical input, live feel and fullscreen performance. Authored skeletal clips, uneven-terrain contact and stairs remain future work.

The initial Debug build encountered MSVC C1902. Project-local CMake flags now embed compiler debug records with `/Z7` in Debug/RelWithDebInfo, avoiding the shared compiler PDB server. The Debug executable's linker PDB is still generated. No installed compiler or Windows settings were changed.

Each fixed controller occupies 152 bytes and adds no per-frame allocation; local foot targets enlarge the animation sample by 36 bytes per actor. No meshes, textures, shaders or GPU buffers were added. One hidden Release pair retained the same submission/triangle peaks, effect counts, gameplay counters and fixed GPU allocations. CPU/GPU timings and process memory are recorded in [graphics.md](docs/graphics.md), with saved reports `foot-placement-baseline-benchmark.txt` and `foot-placement-final-benchmark.txt`. The short comparison is not a speedup or sustained FPS claim. No staging, commits or pushes were performed.

## Layered combat effects - 2026-10-18

Expanded the initial effects toward the requested expressive Asian-MMO appearance: tapered gold Strike crescents, larger cyan Heavy Strike crescents with a delayed violet counter-sweep, bright contact cores with soft halos, expanding impact rings, tapered sparks with glowing heads and rising afterglow motes. Enemy sweeps remain restrained orange. The crescents use eased progression and fade over 0.32 seconds; impact particles expire within 0.48 seconds. A weapon discontinuity now cancels the complete trail/crescent so a dash cannot leave a detached old sweep. Damage, range, cooldowns, event validation and save format are unchanged.

The existing additive pass, GPU buffers and texture allocations are reused. Fixed effect state grows by 128 bytes to 8,520 bytes for stored attack origins/facings; the CPU vertex queue remains capped at 6,144 vertices / 240 KiB. The more detailed geometry and halos add vertex and pixel work. A stress case with eight heavy sweeps and 32 impacts reaches the existing cap and renders successfully.

Debug and Release each pass all thirteen suites, 132 smoke captures and 28 effect diagnostics (`cinematic-effects-validation.log` in each Ninja directory), without compiler warnings/errors. New checks verify ring occlusion and transparent centers, equal-time 30/60/144-Hz rendering, maximum overlap, and gameplay/front/opposite-camera/afterglow views. Representative attack images were inspected. Manual checks 78–80 cover live timing, camera angles and fullscreen readability/performance. An attempted external Asian-MMO reference lookup failed; no new reference assets were reviewed or downloaded, and these shapes/colors are original design choices.

One hidden Release comparison gives combat CPU median/p95 0.421/0.518 → 0.420/0.480 ms and GPU 0.414/0.447 → 0.414/0.437 ms, with 36 asynchronous GPU observations per build. This is a short comparison, not a speedup or fullscreen FPS claim. Effect frames increase from 244 to 265 of 600 and peak effect vertices from 96 to 534, while peak combat submissions remain 57 and gameplay counters are unchanged. End working set/private memory changes from 206.305/236.336 to 210.023/240.430 MiB. Reports: `cinematic-effects-baseline-benchmark.txt` and `cinematic-effects-final-benchmark.txt` in the Release build directory; full measurements and limits are in [graphics.md](docs/graphics.md). No staging, commits or pushes were performed.

## Initial combat attack effects - 2026-10-18

Added a gold Strike blade trail, blue-white Heavy Strike with stronger impact sparks and interrupt glints, and orange enemy swing/hit effects. Trail samples use the same posed blade points as the visible weapon. Transient simulation events only emit after attack validation or windup completion: rejected commands cannot make false impacts, missed swings retain only their trail, and interrupted windups never emit an enemy strike. Unarmed attacks omit the sword trail. Effects age during updates, not renders; death cancels trails, final sparks fade, and load/restart/respawn clear old state. Large movement discontinuities break ribbons.

The renderer adds a texture-free additive pass after opaque scenery and before HUD/portrait rendering. It reads depth without writing it, casts no shadows, and reuses the existing HUD upload buffer. Fixed CPU state is 8,392 bytes for 32 bursts/eight trails with 32 samples each; the reserved effect vertex queue is 240 KiB and capped at 6,144 vertices per frame. GPU mesh/material/interface allocation counters are unchanged; two shaders and small pipeline state objects are added.

Debug and Release each pass thirteen suites, 132 smoke captures and 19 additional combat-effect diagnostic captures (`combat-effects-validation.log`). Coverage includes acceptance/rejection, miss/interrupt, death/load/restart/respawn, live menus, update-rate expiration, pool/upload bounds, teleport breaks, scenery occlusion, blend order, HUD/portrait isolation and repeated rendering without advancement. Strike, heavy, enemy-hit and equipment-panel captures were visually inspected. Manual checks 74–77 still govern live feel and fullscreen performance.

One hidden Release pair shows combat CPU median/p95 0.430/0.510 → 0.427/0.501 ms and GPU 0.423/0.446 → 0.420/0.447 ms (36 asynchronous samples per build). This is within run variation, not a speedup claim. Combat adds one draw (peak 56 → 57); 244/600 measured frames contain effects, with a peak 96 effect vertices. Other workloads submit no effects. Fixed asset buffers and gameplay counters are unchanged, while end process working set/private memory changes from 181.031/209.438 to 209.863/238.215 MiB; fixed buffer reuse is not a total-memory guarantee. Full reports and caveats are in [graphics.md](docs/graphics.md).

README, gameplay/architecture/graphics notes, roadmap, launch manual and playtest checklist updated. No downloads, system changes, player-save writes, staging, commits or pushes.

## Graphics bug fixes - 2026-10-18

Reproduced clouds and the horizon remaining glued to the screen during camera rotation (`build-ninja-debug/graphics-bugs-before.log`). Sky rays now use the current inverse view-projection and ignore camera translation. The sky draws once at the first flush, after camera selection and before world/portrait rendering, so capture overrides use the same view as geometry. Initialization supplies a valid default camera. Renderer initialization failures now include shader diagnostics in both GPU test executables.

Debug and Release pass all twelve suites and 132 smoke captures per configuration (`graphics-bugs-validation.log`). New image regressions cover first-frame sky, yaw, pitch, translation, one draw per empty frame and returning to the same view without stale camera state. Existing portrait isolation, resize, material anchoring and all fifteen sloped-shadow probes pass. Village, canopy and sky-direction captures were visually inspected; manual check 73 covers live camera/preview transitions.

The final hidden Release workload report is `build-ninja-release/graphics-bugs-benchmark.txt`: CPU medians are 0.521/0.437/0.423/0.793 ms and GPU medians 0.512/0.429/0.414/0.642 ms for stationary/walking/combat/merchant. Draw counts, geometry, fixed GPU allocations and gameplay counters match the preceding material pass. These short asynchronous samples do not certify fullscreen FPS. No player saves, system settings, staging, commits or pushes were changed.

## Surface depth and lighting - 2026-10-18

Completed a graphics pass toward the supplied Allods visual direction: shallow stone/roof/plaster relief, wood grain and plank seams, woven cloth, roughness-dependent highlights, linear-light shading, warm sun/cool fill, irregular beveled paving and layered clouds. Material detail now uses object-aligned triplanar coordinates, so it blends across curved surfaces and follows animated parts. All eleven 128-square material layers are reused by packing intensity, slopes and roughness into their existing channels. Geometry, texture allocations, draw counts and gameplay remain unchanged; authored characters/foliage and physical atmosphere remain future work. The follow-up above corrects sky orientation.

The new lighting exposed a grazing-angle self-shadow stripe. Correcting the receiver-gradient clamp removes it while retaining existing contact/filter thresholds. New GPU regressions verify material anchoring during translation; deterministic packed surfaces and resource bounds are also checked. Both Debug and Release pass all twelve suites and 132 smoke captures without compiler warnings/errors (`realism-validation.log`). Village, stall, ruin and character captures were visually inspected. Baseline images remain under `build-ninja-release/realism-baseline-captures`.

One paired hidden Release benchmark measures about +0.083 ms walking and +0.068 ms combat in GPU medians at 1280x720, with sparse asynchronous samples. Fixed mesh/material/interface storage remains 224,320 / 9,699,312 / 1,529,172 bytes. Total process end working set/private memory rises from 181.004/209.461 to 194.559/224.727 MiB. See [graphics.md](docs/graphics.md) for all workload timings, reference sources and limits; manual checks 70–72 cover visible performance and temporal appearance. This pass adds shader work and is not an FPS improvement claim.

Updated README, launch/debug instructions, architecture, graphics notes, roadmap and manual checklist. Earlier uncommitted changes remain intact. No staging, commits, pushes, asset downloads, system changes or player-save writes were performed.

## Defeat transition bug fixes - 2026-10-18

New regressions reproduced frozen enemy finishing strikes (also after loading), pending warning cones left behind by an earlier enemy in the update order, and a final movement step by later enemies after fatal damage. The failing AnimationTests/CombatTests output is retained in `build-ninja-debug/defeat-fixes-before.log`.

Fatal damage now cancels every enemy windup, and remaining enemy updates and separation stop immediately. Suspended combat no longer drives strike presentation from frozen cooldowns: living enemies settle to neutral, including after load. Rendering also suppresses obsolete warning cones from older defeated saves. Respawn still resets surviving enemies at home and retains campaign progress. No save-format, resource-budget or combat-balance change was required.

Debug and Release pass all twelve suites and 132 smoke captures per configuration (`defeat-fixes-validation.log` in each build directory). A final Debug InterfaceTests run verifies the corrected legacy-save fixture (`defeat-fixes-final-tests.log`). New coverage includes overlapping windups, update-order independence after death, suspended-state stability, recovery and immediate rendering after loading a defeated save. The defeat/recovery capture was inspected. Manual check 69 covers live presentation; these checks do not replace interactive playtesting. The prior performance benchmark remains applicable as historical evidence and was not repeated for this transition fix.

Earlier uncommitted work is preserved. No staging, commits, pushes, downloads, system changes or player-save writes were performed.

## CPU rendering performance - 2026-10-18

Prepared camera/light frusta now reuse their planes and normal lengths across objects. Visibility grouping uses counts, offsets and one stable scatter instead of seven mesh-queue rescans. Interface ovals, rings and ornamental scrolls reuse 728 bytes of fixed curve samples instead of recalculating their trigonometry every frame. Shadow quality, draw order, geometry, animation and gameplay remain intact; no GPU resources were added.

All 132 Release PNGs are byte-for-byte identical to the saved pre-change captures. Debug and Release each pass all twelve suites and 132 captures, including offscreen shadows, portrait isolation, resizing and the 4,096/4,097-instance boundary. Logs are `performance-validation.log` in each build directory. The comparison is recorded in `build-ninja-release/performance-image-comparison.csv`.

Three paired Release runs alternate baseline/optimized order. Median-of-run CPU medians improve from 0.369 to 0.349 ms walking (5.4%), 0.404 to 0.357 ms combat (11.6%), and 0.827 to 0.778 ms merchant (5.9%). Stationary timing changes from 0.435 to 0.456 ms with wide overlapping run variation, so no stationary improvement is claimed. Draw/triangle peaks and fixed GPU allocations are unchanged. Full before/after tables, sample counts and limitations are in [graphics.md](docs/graphics.md). These hidden unpaced results are not an interactive FPS guarantee; manual check 68 remains pending.

Earlier changes are preserved. No staging, commits, pushes, downloads, system-setting changes or player-save writes were performed.

Suggested commit text: `Optimize frustum culling, instance packing and interface curve generation`

## Shadow queue and capacity fixes - 2026-10-18

Reproduced two directions of stale geometry leakage: a fully culled world queue could render inside the character portrait, and a fully culled portrait queue could render back into the world. Maximum pixel differences against clean references were 255 and 195 respectively; both are now zero. Flushing consumes submitted queues even when nothing survives culling, while an already empty flush remains a no-op.

The extra shadow pass also made a 1,400-object scene exceed the cumulative upload budget. Visibility masks now group each distinct visible object into one uploaded record shared by the two shadow passes and the color pass. The scene renders successfully, an exact 4,096-object scene succeeds, and 4,097 is rejected with a capacity diagnostic. The existing 655,360-byte instance buffer, shadow resolution/filter and all fixed mesh/texture allocations are preserved.

Failing reproductions are retained in `build-ninja-debug/shadow-fixes-before.log`. Debug and Release pass all twelve suites and 132 smoke captures without compiler warnings/errors (`shadow-fixes-validation.log` in each build directory). Character/preview images were inspected. The Release report `build-ninja-release/shadow-fixes-benchmark.txt` retains the previous draw/triangle peaks and fixed resource sizes; timings and process-memory snapshots are in [graphics.md](docs/graphics.md). Manual check 67 covers preview transitions during live play. Earlier development is preserved. No files have been staged, committed or pushed, no assets downloaded, and no system settings or player saves changed.

Suggested commit text: `Fix culled geometry leaking between views and shadow upload overflow`

## Focused shadow detail - 2026-10-18

Added a 20-unit detail projection beside the fixed 62-unit whole-island map, using two 1024-square depth layers. Nearby sampling is 3.1 times finer, improving boot, weapon and thin-prop shadows. The detail origin moves in whole light-space texels, and coverage blends smoothly into the distant map. Both receiver gradients are calculated before coverage branches. Independent light culling and a depth-only vertex shader limit extra work; the existing 4,096-instance upload capacity is preserved.

A thin-post regression reproduced shadows disappearing at some sub-texel offsets: the weakest sample improves from zero to 78.67 intensity levels of contrast (eight offsets). Low 0.05-unit contact improves from 65 to 102.33. The focused-grid probe changes pixels by at most one level, and overlap steps remain below the 25-level acceptance threshold. Existing slope, moving/offscreen caster, no-caster clearing, resize and portrait-isolation checks still pass. These synthetic comparisons do not replace live visual acceptance.

Debug and Release pass all twelve suites and 132 hidden smoke captures without compiler warnings/errors. Character, village and thin-post captures were inspected. Logs: `shadow-detail-validation.log` in each Ninja build directory; failing reproduction: Debug `shadow-detail-before.log`. The saved Release report is `build-ninja-release/shadow-detail-benchmark.txt`, compared with `smoothness-benchmark.txt` in [graphics.md](docs/graphics.md). The additional map costs exactly 4 MiB of depth storage; measured GPU medians rise by 0.030 ms for walking, 0.041 ms for combat and 0.028 ms for the merchant in these short hidden runs. Manual checks 64–66 cover temporal appearance and visible performance.

Earlier animation/smoothness work is preserved. All current changes remain unstaged and uncommitted; no commits, pushes, downloads, system changes or player-save writes were performed.

Suggested commit text: `Improve close-up shadows with a stable detail map and blended coverage`

## Smooth pose transitions - 2026-10-18

Player turning and player/enemy gait/attack presentation now use critically damped responses with retained velocity. Turning accelerates and settles over the shortest arc. The gait solver integrates the moving sine target and its derivative over each interval, preserving update-rate agreement while removing abrupt changes when walking stops or speed changes. Physical input release still stops displacement immediately; enemy facing remains exactly aligned with its authoritative attack cone.

Positive stride/ascent lift is squared, and airborne extension uses a cubic height envelope, removing sharp slope changes at neutral stride, the apex and the grounded/full-air boundaries. Joint lengths, attachment transforms, collision, combat timing and save version 6 remain intact. Resets clear the new velocity state. No assets, GPU resources or dependencies were added.

New acceptance checks failed on the prior blends (`build-ninja-debug/smoothness-before.log`). The gait stop's finite-difference velocity jump at a 0.1 ms probe drops from 10.9279 to 0.0488758 pose units/second. MotionTests covers gradual turn/attack starts, stop/reversal continuity, bounded repeated retargeting, exact enemy facing, reset velocities and 30/60/144-Hz agreement. AnimationTests covers the softened foot/air boundaries alongside existing landing/recoil regressions.

Debug and Release pass all twelve suites and 132 hidden smoke captures without compiler warnings/errors. Logs: `smoothness-validation.log` in each Ninja build directory. Walking and falling captures were inspected; live acceptance remains manual checks 61–63. The Release report `build-ninja-release/smoothness-benchmark.txt` retains fixed GPU allocations and the same walking distance/combat hit counts; see [graphics.md](docs/graphics.md) for timings and limits. All work remains unstaged and uncommitted; no commits, pushes, downloads, system changes or player-save writes were performed.

Suggested commit text: `Smooth character turning, locomotion and attack transitions`

## Animation transition bug fixes - 2026-10-18

Three failing reproductions exposed missed recoil when frame duration increased between consecutive strikes, recoil held by an enemy's frozen damage flash after player defeat, and frame-dependent landing timing/strength. The failing output is retained in `build-ninja-debug/animation-fixes-before.log`.

Simulation now publishes transient damage events separately from the fading tint. Enemy/trainer tints decay even after combat stops, and an old flash cannot trigger fresh recoil. Ground contact is solved within the simulation step; animation receives the impact speed and time elapsed since contact. At 0.8 seconds after takeoff, the landing channel previously measured 0.955375 / 0.775920 / 0.849328 at 30 / 60 / 144 Hz. It now measures 0.816757 at all three rates and with uneven steps. Launch speed, gravity, apex, damage rules and save version 6 are preserved.

AnimationTests covers the reproductions, rejected commands, one-update event lifetimes, exactly one contact per jump, within-frame timing, and saving/loading on impact. Both Debug and Release pass all twelve suites and 132 smoke captures; logs are `animation-fixes-validation.log` in their respective Ninja build directories and report no compiler warnings/errors. Landing/recoil captures were visually inspected. The Release workload report is `build-ninja-release/animation-fixes-benchmark.txt`; fixed GPU allocations are unchanged. Physical playtesting remains pending (manual checks 59–60).

The preceding animation work is preserved. This follow-up remains unstaged and uncommitted; no commits, pushes, downloads, system changes or player-save writes were performed.

Suggested commit text: `Fix recoil events, defeat flashes and frame-dependent landings`

## Character animation layers - 2026-10-18

- Added transient player/enemy animation channels for breathing/sway, travel lean, rising/falling poses, landing compression and damage recoil. Airborne legs extend near the floor; landing lowers the hips while the two-bone solver holds the ankle targets. NPCs and the equipment preview now have restrained idle movement.
- Shared upper-body transforms keep clothing, head, shoulders, sword and shield aligned during lean/recoil. The existing limb lengths, geometry, textures, shadow resources and version 6 saves are preserved. This is procedural part animation, without skeletal skinning or walking foot locks.
- Recoil follows actual hit flashes; consecutive equal-maximum flashes count as fresh hits, preserving the current recoil amplitude instead of snapping to neutral. New takeoffs clear the previous landing compression. Defeat, restart, respawn, loading and new training rounds clear the appropriate transient state. Rendering cannot advance animation or simulation.
- Added AnimationTests covering extreme channel combinations, fixed limb lengths, ground/weapon clearance, idle phase continuity, 30/60/144-Hz jump transitions, reaction refresh and reset paths. A player/enemy encounter reproduces the same authoritative state/save output as a standalone Simulation. The interface integration test checks repeated world/portrait renders for unintended updates.
- Both configurations pass twelve suites and 132 smoke captures. Ten new views inspect six animation layers plus four live jump stages. Rise, fall, landing and recoil captures were visually inspected. Logs: `build-ninja-debug/animation-validation.log` and `build-ninja-release/animation-validation.log`; Release before/after reports: `animation-baseline-benchmark.txt` and `animation-final-benchmark.txt`. Details and limits are in [graphics.md](docs/graphics.md).
- Fixed GPU resource allocations are unchanged. No assets were downloaded, no machine settings changed and no player saves written by validation. Visible playtesting remains pending in manual checks 56–58. The externally committed movement/shadow work is preserved; this animation batch remains unstaged and uncommitted.

Suggested commit text: `Add layered character animation, jump landings and hit reactions`

## Sun shadow refinement and regressions - 2026-10-18

- Reproduced missing shadows when solid geometry used the Plain material. Casting is now explicit per draw, with a default for opaque geometry and opt-outs for grass, quest effects, lantern cores and combat markers. Existing instance storage is reused.
- Reproduced weak contact from a 0.05-unit-high object under the old depth bias. The new receiver-plane tent filter compares depth at each texel center, with small bounded raster/receiver offsets. The test's shadow contrast improves from 6.33 to 65 intensity levels; plain and stone posts now produce the same shadow contrast (114.67).
- A first filtering revision showed scalloped close wall-trim edges during visual inspection. Individual texel comparisons corrected that artifact while preserving contact. The final filter uses 16 point comparisons, a fixed world projection and the same 1024-square, 4 MiB shadow map; no new textures, meshes or assets.
- Added `ShadowTests`: actual hidden DX11 pixel comparisons across 15 receiver orientations, moving casters, offscreen casters, removal/no-caster frames, three window sizes and portrait/world isolation. These checks protect existing behavior as well as the two reproduced bugs. Both configurations pass all eleven suites and 122 game smoke captures; representative character, wall, stall and combat views were inspected. Physical temporal-quality checks 53–55 remain pending.
- Release workload results and before/after measurements are in [graphics.md](docs/graphics.md). Fixed resource allocations are unchanged. Validation logs: `shadows-validation.log` in each Ninja build directory; failing reproductions: Debug `shadow-before.log` and `shadow-bias-before.log`. No saves, system settings, downloaded assets, staged files, commits or pushes were added by this pass.

## Movement and camera smoothing - 2026-10-18

- Player walking/sprinting starts with an analytically integrated speed ramp; releasing input, opening a central menu or losing input stops physical movement immediately. Collision remains authoritative and blocked input cannot bank speed.
- Enemy turning is bounded, pursuit accelerates and slows near arrival, reached waypoints consume the same frame's travel budget, exhausted routes refresh, and separation corrections are capped per pair. Home snap tolerance drops from 0.15 to 0.015 world units.
- Per-actor walking phase uses actual travel, with smooth gait/strike transitions and shortest-arc player turns. Dash/respawn cannot become false steps. Death clears poses; load restores committed enemy strikes without an idle flash.
- Camera orbit, zoom and jump/landing height ease during play. Obstruction retracts immediately and clear-space recovery eases. Capture/tool placement stays immediate. No GPU resources, downloads, dependencies or save-format changes were added.
- MotionTests checks real simulation travel at 30/60/144 Hz, diagonal limits, idle settling, menu/focus stop, airborne/dash handling, reset/load, windup/body alignment and camera response. Combat/training tests now wait for the bounded turn before checking full telegraph duration and exact damage.
- Debug/Release: ten suites and 122 hidden smoke captures per configuration pass; combat and sparring renders inspected. Logs are `build-ninja-debug/motion-validation.log` and `build-ninja-release/motion-validation.log`. No player saves were written. Manual motion acceptance remains pending (checks 47–49).
- A fresh local export/build of committed HEAD was compared with the final four-workload benchmark. GPU resource counts remain unchanged. Hidden process-memory readings varied widely in both versions; details and limitations are in [graphics.md](docs/graphics.md). These runs do not establish a memory regression, a speedup or interactive FPS.

Suggested commit text: `Smooth player and enemy movement, animation transitions and camera response`

## Smoothing bug fixes - 2026-10-18

Seven problems in the first pass were reproduced by new failing regressions before implementation (`build-ninja-debug/motion-fixes-before.log`): jump height and gait changed with frame rate; WASD used the camera's future angle; cancelling orbit retained rotation; alternating obstruction samples pumped the boom; direct pursuit followed a stale player location; and an enemy inside melee distance stalled behind a trunk.

The follow-up integrates gravity and gait over the whole frame, uses the displayed camera heading for movement, clears pending orbit on gesture/panel/drag cancellation, and requires 100 ms of clear space before camera recovery. Zooming inward remains responsive. Direct pursuit updates its destination every frame and short-range stopping requires sight. Body lift also crosses neutral smoothly. Collision, committed windups, rewards and save version 6 remain covered by the existing suites.

MotionTests covers 30/60/144-Hz jump apex, constant-speed gait and camera recovery; simultaneous camera rotation/movement; cancellation and menu reopening with RMB held; intermittent obstruction; inward zoom during recovery delay; moving-target pursuit; and navigation around a close occluding trunk. Debug and Release each pass all ten suites and 122 hidden captures, with no compiler warnings/errors reported. Final validation logs are `motion-fixes-validation.log` in both build directories. The Release workload report is `build-ninja-release/motion-fixes-benchmark.txt`; resource allocations are unchanged and detailed timings are in the graphics notes. Physical motion acceptance remains pending (manual checks 50–52). The initial movement pass and this follow-up are both still uncommitted.

Suggested commit text: `Fix movement smoothing, camera jitter and enemy pursuit stalls`

## Pre-migration repository snapshot

| Reference | Commit | Inspected state |
| --- | --- | --- |
| Original `main`, now `codex/archive-js-prototype` | `a55db3e` | Archived Electron / Three.js prototype |
| `origin/dev` | `8ae6325` | Later JS changes; history mentions physics, walls, map, and lighting improvements |
| `origin/C++` | `65f2efa` | Native engine skeleton, commit message: `moving to C++` |

These references were inspected locally. No remote refresh was performed, so this is not a claim about the latest GitHub state. No common ancestor was found between `main` and `origin/C++` in the available local history.

## Existing JavaScript implementation

Historical reference only; these files have been removed from the active tree.

Source inspection found:

- Electron desktop entry point and Three.js rendering.
- World geometry, obstacles, a player, and enemies.
- Third-person camera and input handling.
- Movement, jumping, health, targeting, and combat ability code.
- Slash, bolt, dash, and heal abilities with cooldowns.
- HUD and character, inventory, quest, and map panel code.

These are source-level findings, not confirmation that every feature or panel is complete or works at runtime. The JS application was not launched during this review.

## Existing native implementation

Imported from `origin/C++` without generated binaries or build caches:

- CMake project targeting C++17 and an `RPGNative` Windows executable.
- Win32 window creation and message processing.
- DirectX 11 device, swap chain, render target, and viewport setup.
- Frame timing, game update/render calls, framebuffer clearing, and presentation.

The original empty game methods have now been replaced. The working source contains:

- Runtime-compiled HLSL shaders, reusable indexed curved meshes, generated material layers, instanced shadow/color passes, camera/light culling, depth testing, sun/hemisphere lighting, fog, a generated sky, an installed-font atlas, and alpha-blended HUD batching.
- A floating island with named village/dock/training/camp areas, an irregular cliff edge, pitched timber/plaster houses, branched trees, banners, distant fragments, a quest beacon, shaped characters, and three enemies.
- Camera-relative movement, sprinting, jumping, collision against buildings/trees/island bounds, mouse orbit, zoom, and camera obstruction checks.
- Target cycling, melee range and line-of-sight checks, strike, heavy strike, dash, mend, cooldowns, enemy pursuit/return behavior, damage, death, and restart.
- A patrol acceptance/kill/return/reward loop with protection against duplicate rewards and support for a kill before quest acceptance.
- Health/target/ability HUD with availability reasons, live map and loot markers, quest view, character statistics, potion use, and sword switching.
- Static-grid A* navigation with path smoothing, enemy behavior states and home leashes, soft enemy separation, and stable enemy IDs.
- One-time loot, potion cooldowns, reward sword and ward armor, two quests, derived XP/levels and interruptible attack windups.
- A simulation-owned 24-cell bag, six worn slots, drag swaps/equip/remove, stack splitting/merging, sorting, exact-cell right-click actions, comparisons and shared outfit visuals. Version 6 preserves placement/equipment, purchases and buyback, migrating versions 1–5.
- A quartermaster, solid supply counter, orange map marker, limited potion/helm/shield stock, validated purchases/sales and persistent buyback in a shop that coexists with character/bag windows. Portrait arrows rotate the actual worn outfit.
- A repeatable 30-second sparring opponent after the campaign, with real armor effects, nonlethal damage, result reports and no rewards or resource refunds.
- Shared world/portrait character presentation with shaped heads, hair and flat-soled boots; fixed-length local limb poses; hand-attached sword/shield; and eight diagnostic pose captures. A repeatable hidden benchmark covers stationary HUD, walking/orbit, combat/orbit and merchant/character/bag workloads.
- Versioned local save/load with bounds checking and transactional simulation replacement; invalid data does not replace the current simulation. Save files are replaced only after a successful temporary-file write.
- Borderless fullscreen, windowed restoration, per-monitor DPI awareness, focus/minimize suspension, a live Escape menu, DirectX error reporting, and shutdown handling.

Compilation, domain regression tests, and hidden rendering now validate parts of this implementation. Automated tests do not certify the complete interactive experience. Art now has a first procedural surface/silhouette pass with bounded memory; it remains stylized prototype art, not photorealism or finished Allods-style art. See [graphics and measurements](docs/graphics.md).

## Known gaps

- The historical C++ branch tracks generated files under `build/`, `build_vs/`, and `out/build/`. Those files were not imported into the working checkout.
- Real keyboard/mouse interaction, focus/minimize behavior, all panels, and full gameplay feel still need the manual checklist.
- No sound, skeletal skinning, authored animation clips, imported art or networking. Short hidden Release measurements cover stationary HUD, moving cameras, repeated combat and the merchant interface. Physical interactive input, large encounters and long-session performance remain unverified.
- Navigation uses a static grid and is suited to this small map; there is no general navmesh or crowd solver. Enemy separation is soft, and player/enemy collision is not a full rigid-body system.
- Decorative dock rails, banners, and training targets are non-solid. Trees have trunk-based collision and conservative camera bounds.
- The catalog has two swords, one potion type, tunic, trousers, boots, ward armor, helm and shield. Six worn slots, drag/sort/splitting, limited purchases, selling and buyback work. Permanent disposal, active shield blocking, more items and progression beyond level 3 remain pending.
- Camera, target, panel state, jump height, and transient hit flashes are not saved.
- World geometry is instanced and F3 includes asynchronous GPU timing. Broader visible-gameplay profiling and final art acceptance remain pending.

## Development environment

- Visual Studio Community 2022 version 17.13.3 was detected.
- The initial inspection found no usable toolchain. A later inspection found installed CMake 3.30.5-msvc23, MSVC 19.43.34809.0, and Windows SDK 10.0.22621.0, which now build the project successfully.
- MSBuild required execution outside the restricted sandbox to read its SDK-discovery directory. This did not install software or change system settings.
- No tools were installed or downloaded by the agent.

## Completed in this session

- Read project instructions in `AGENTS.md`.
- Removed the local `.vs/` cache at the user's request and added `.vs/` to `.gitignore`.
- Inspected the current working tree and locally available branch references.
- Reviewed the native skeleton and identified the gap between engine initialization and gameplay migration.
- Created `roadmap.md` and this progress record in English.

## Next validation milestone

Run the interactive checklist in `tests/manual-smoke.md`, record input/combat/camera/panel behavior, and fix observed problems before expanding beyond this map.

The latest bug-fix pass adds reproduced regressions for stale panel/load input, camera gesture leakage, the unarmed starter-sword hotkey and impossible patrol save states. All fixes pass Debug/Release validation; actual desktop input and monitor behavior remain manual checks.

## 2026-10-17: native development baseline

- Prioritized Allods Online and defined a small floating-island outpost slice in the roadmap.
- Imported only CMake and native source files from `origin/C++`; preserved the JS prototype, current branch, and pre-existing staged changes.
- Ignored generated build directories.
- Added a Windows platform guard to CMake and a PowerShell build/run helper for Visual Studio 2022.
- Checked render-target creation failures and added a retry without the debug layer when the SDK debug component is missing.
- Stopped frame processing after quit, paused rendering while minimized, and capped frame delta at 50 ms.
- Build-tool inspection still found no available CMake or registered MSVC toolchain. No installation or download was performed.
- Validation: PowerShell parser reported no errors in the build helper. Running `.\scripts\build-native.ps1 -Configuration Release` stopped at its prerequisite check with `CMake was not found`; no compilation or executable launch occurred.

## 2026-10-17: first native gameplay slice

- Implemented the renderer, input, gameplay simulation, scene, HUD, supporting panels, quest, and persistence described above.
- Added `tests/SimulationTests.cpp` and CTest integration for collision, camera obstruction, invalid attacks, jumping, quest/reward progression, save/load, malformed data, death/restart, and invalid timesteps.
- Added `-Test` to the native build helper and documented the controls and smoke-test procedure.
- Validation performed: PowerShell parser accepted the helper; `git diff --check` passed; reviewed source wiring and edge cases.
- Build/test attempt: `.\scripts\build-native.ps1 -Configuration Debug -Test` stopped at `CMake was not found`. Neither the C++ tests nor the application ran.
- Additional environment checks found no compiler on PATH, no MSVC Tools directory in the installed VS instance, and no registered WSL distribution. No tools were installed or downloaded.

Next: compile both configurations, execute CTest, fix compiler/runtime findings, and run `tests/manual-smoke.md` before extending the slice further. Visual quality, combat feel, and performance must be judged in a running build.

The planned feature sequence is maintained in [roadmap.md](roadmap.md).

## 2026-10-17: native-only repository migration

- Created the local `codex/archive-js-prototype` branch at `a55db3e` before removing the JS files. Verified that those files had no local modifications.
- Removed the legacy `src/`, Electron entry point, HTML UI, and npm manifests from the active tree.
- Preserved all current native source files, build helper, tests, `AGENTS.md`, `roadmap.md`, `progress.md`, and `LICENSE`.
- Kept `main` as the active development branch. The unrelated historical `origin/C++` branch was not checked out or merged.
- Updated the README with native-only instructions and Visual Studio Open Folder guidance.
- This migration changes repository organization only; native build/runtime validation is still pending. No dependencies were downloaded and no remote push was performed.

## 2026-10-17: start and debug manual

- Added `START_AND_DEBUG.md` with Visual Studio folder/solution workflows, build/test/run commands, breakpoint guidance, controls, save-file behavior, and troubleshooting.
- Linked the manual from `README.md`. Instructions were checked against the current build helper and source; this documentation update does not constitute a successful build or runtime test.

## 2026-10-17: validated map and gameplay expansion

- Separated platform-independent simulation/world data into `MediterraneanSimulation`; extracted map presentation into `WorldView`.
- Added shared tree/building collision, named map areas, A* routes with clearance checks and smoothing, explicit enemy states, home return, and soft enemy separation.
- Implemented one-time corpse loot, healing potions (Q), the watchguard blade reward and equipment switching (G), availability feedback, and a debug overlay (F3).
- Introduced version 2 saves with stable enemy IDs, behavior/loot flags, inventory/equipment validation, and migration of valid version 1 saves.
- Added pyramid meshes, house trim, banners, dock and arena props, simple limb motion, and batched HUD/text geometry.
- Added hidden `--smoke-test` rendering and PNG capture; the PowerShell helper exposes it through `-SmokeTest`.
- Fixed issues found during validation: a missing lambda capture and a WIC pixel-format mismatch. Final runs pass.

| Validation | Debug | Release |
| --- | --- | --- |
| Native build | PASS | PASS |
| SimulationTests | PASS | PASS |
| NavigationTests | PASS | PASS |
| InventoryTests | PASS | PASS |
| DirectX device/shaders, resize, rendering, PNG readback | PASS | PASS |

Commands executed: `./scripts/build-native.ps1 -Configuration Debug -Test -SmokeTest` and the same command with `Release`. The render test exercises 960x600, 1600x900, and 1280x720 targets, then captures the final size. Captures and logs live in ignored `build-local/Debug-smoke.png` and `build-local/Release-smoke.png` (with `.log` appended for results). The Debug capture was visually inspected after HUD batching.

Regression coverage includes the original patrol/combat/save checks, routing around buildings, a pursuing enemy navigating around a tower, one-build command replay consistency, one-time loot, potion consumption/cooldowns, equipment damage, legacy save migration, and rejection of unknown enemy IDs. This is not a claim of cross-platform deterministic networking or complete manual QA.

No commits, amendments, pushes, downloads, or system-setting changes were made during this development pass. User edits to `AGENTS.md` were preserved.

## 2026-10-17: Visual Studio startup troubleshooting

- The reported toolbar showed `RPGNative.sln` as the launch item, and Solution Explorer had an active search filter. Documented how to open the generated solution through **File > Open > Project/Solution**, clear the search, and select the `RPGNative` C++ startup project.
- Confirmed the folder-mode CMake cache identifies the installed MSVC compiler and exposes `RPGNative` as an executable target. Confirmed `build-local/RPGNative.sln` and `build-local/Debug/RPGNative.exe` exist, with a passing render smoke-test log from the prior build.
- Added an immediate existing-executable launch path to `START_AND_DEBUG.md`. No source, IDE settings, or system settings were changed; this documentation check does not verify an interactive Visual Studio launch.

## 2026-10-17: two-quest campaign and readable combat

- Added committed enemy windups, visible attack cones, delayed impact checks, heavy-strike interrupts, and a distinct captain with 120 health and a longer, heavier swing. Returning enemies cannot be targeted or damaged.
- Added bounded floating combat/XP feedback, player hit flash, level and XP bars, a persistent objective tracker, interaction prompts, channel progress, and updated journal/map/character/inventory panels.
- Extended the patrol into the astral-anchor quest: clear three enemies, complete an interruptible three-second channel, return for a unique reward. Added levels 1–3, level health/damage bonuses, and automatically equipped ward armor. XP derives from durable objective flags.
- Added Enter recovery after death that retains progression, inventory, and dead/looted enemies while restoring living enemies at home. R remains the explicit full reset.
- Introduced save version 3 with anchor state and committed enemy swings. Versions 1/2 migrate, preserving third-enemy health fraction and deriving progression from prior accomplishments. Invalid data is rejected transactionally.
- Added combat and progression regression suites. The latter completes both quests through real movement/combat/interaction commands, then tests channel interruption, saved progress, duplicate rewards, respawn, migration, and restart.
- Added `-Backend Ninja` to the existing build helper, with a child-process Visual Studio compiler environment and separate build directories. Added a raw localized MSVC include-prefix probe after observing CMake 3.30 corrupt the Russian prefix; Ninja dependency records now include `Simulation.h` and `WorldMap.h` correctly.
- Extended hidden rendering to four authored scenarios at three sizes. Snapshot loading stays in memory and never overwrites player saves.

| Validation | Debug | Release |
| --- | --- | --- |
| Ninja/MSVC native build | PASS | PASS |
| SimulationTests | PASS | PASS |
| NavigationTests | PASS | PASS |
| InventoryTests | PASS | PASS |
| CombatTests | PASS | PASS |
| ProgressionTests | PASS | PASS |
| Spawn, raider, captain, veteran captures at 960x600 / 1280x720 / 1600x900 | PASS | PASS |

Commands: `./scripts/build-native.ps1 -Backend Ninja -Configuration Debug -Test -SmokeTest` and the same command with `Release`. Debug was rebuilt cleanly after the dependency-prefix fix. Logs are in `build-ninja-debug/validation.log` and `build-ninja-debug/release-validation.log`. Captures are in the corresponding `build-ninja-debug` / `build-ninja-release` directories. Spawn/veteran captures at 960x600 and raider/captain captures at 1280x720 were visually inspected. Latest executables are `build-ninja-debug/RPGNative.exe` and `build-ninja-release/RPGNative.exe`.

The initial MSBuild request could not run through automatic approval review (HTTP 403, then timeout); a restricted MSBuild attempt failed on its per-user SDK lookup. Validation succeeded with bundled Ninja under the existing sandbox permissions, without changing permissions or system settings. The older `build-local` executable was not replaced; rebuild the generated solution before using its output for these changes.

Updated README, launch/debug manual, gameplay rules, architecture notes, roadmap, and the manual checklist. Full interactive playtesting, finished art, sound, and frame profiling remain pending. Ward armor is awarded after the existing enemies are dead; repeatable encounters are future work. No commits, amendments, staging, pushes, downloads, or system-setting changes were made in this pass. `AGENTS.md` was not modified.

## 2026-10-17: procedural graphics with bounded memory

- Replaced the visible box/pyramid blockout with an indexed reusable mesh library: shaped bodies, articulated limbs, heads, boots, blades, pitched roofs, irregular stonework, branched trees and a joined island cliff skirt. Kept floor height and gameplay obstacle footprints unchanged.
- Generated eleven 128-square material layers with mipmaps at startup. Added world-space surface detail, a dirt/grass blend, hemisphere fill, directional sun, filtered 1024-square shadows, fog and a generated sky. No art datasets or downloaded assets were used.
- Batched world instances by mesh with separate camera/light culling and one bounded instance upload. Removed shadow casting from emissive and interface markers. Added asynchronous GPU timestamps and the repeatable `-Benchmark` / `--benchmark` path.
- Added F2 interface hiding and updated F3 to show completed-frame draw calls, GPU milliseconds and mesh KiB. Pause, panels and death restore essential HUD visibility while active.
- Added GeometryTests covering valid indices, finite normals, winding, nondegenerate triangles, collision-floor coverage, joined cliff boundaries, frustum culling and allocation budgets. The tests exposed incorrect smoothing across trunk/limb caps; separate cap vertices fixed it. Visual inspection exposed overlapping armor; the torso now uses the armor material directly.
- Extended each render fixture with overview and front-character captures. Debug smoke tests inspect DirectX warning/error messages when the optional debug layer is available. Expanded camera bounds to cover pitched roofs/chimneys and tree crowns, with regressions confirming actor traversal underneath foliage is unaffected.
- Updated README, architecture, roadmap, manual checklist and START_AND_DEBUG.md. Added docs/graphics.md with renderer budgets, benchmark methodology, the old/new comparison and remaining art work.

Validation: Ninja/MSVC Debug and Release builds pass all six CTest suites and all four render scenarios at 960x600, 1280x720 and 1600x900, plus two inspection angles per scenario. Representative village, combat, captain, veteran and front/overview captures were visually inspected. Logs: `build-ninja-debug/graphics-validation.log` and `build-ninja-release/procedural-final-validation.log`. Captures remain under those ignored build directories.

Measured spawn submissions fell from 186 to 26. Material/shadow allocation is 5,155,484 bytes (4.917 MiB); indexed meshes use 70,496 bytes. GPU work is higher than the blockout because geometry, surface sampling and a shadow pass were added. Exact CPU/GPU/memory results and the hidden-window limitations are recorded in [docs/graphics.md](docs/graphics.md). They are not a gameplay FPS guarantee.

Interactive playtesting, authored character quality, foliage refinement, skeletal animation, audio and long-session performance remain pending. No commits, staging, pushes, downloads, or system-setting changes were made. AGENTS.md and the earlier gameplay work were preserved.

## 2026-10-17: fullscreen and RPG interface

- Gameplay opens in borderless fullscreen without changing desktop display settings. F11 and the menu Display button restore/toggle the previous window placement. Added process-local per-monitor DPI awareness. Hidden tests verify monitor-sized client bounds, hidden visibility and exact windowed client restoration.
- Replaced Escape pause with a live clickable menu: Return, Character, Inventory, Atlas, Journal, Display, Save, Load and Exit. Every panel captures local movement/camera/combat keys while simulation continues. Tests prove committed enemy attacks still damage the player behind each panel.
- Added a compact HUD, minimap, readable objective tracker, clickable action bar and hover descriptions. Replaced the old pixel glyphs with an atlas rasterized from the installed Windows font; added alpha-blended interface batching.
- Added a 24-slot belongings view, original item icons, equipped markers, stack counts, hover details and selected-item actions. Equip/Drink issue real validated simulation commands. Sword changes survive the existing version 3 save format.
- Added equipment slots and a lit 3D preview using the same dressed CharacterView as the world. The reward sword has a distinct finish; ward armor appears when earned. Empty head/off-hand slots and baseline clothes are represented honestly; additional armor selection and drag sorting remain future work.
- Added a generated illustrated atlas shared with the minimap, preserving world proportions. Terrain, coastline, roads, rooftops, tree crowns, ruins and dock follow project data. Live markers and player facing overlay the art; traversable-ground waypoints can be placed and cleared without affecting saves.
- Added a two-card quest journal, menu save/load feedback and seven-suite validation through a shared MediterraneanClient library. InterfaceTests covers live menus, item ownership/use, equipment persistence, scaled real-renderer pointer routing at 960x600/1280x720/2560x1080, modal click isolation and map coordinates.
- Researched Allods/WoW interface guides and indexed image references. Source links and access limitations are recorded in docs/interface.md. No reference assets or font files were downloaded/copied. All new artwork is generated locally from code and world data.

Validation: Debug and Release native builds, all seven CTest suites and the expanded hidden fullscreen/world/interface smoke test pass. Each of four scenarios has three gameplay sizes, two inspection views and five panels at three sizes (80 captures per configuration). Representative menu, inventory, equipment, atlas and journal captures were visually inspected, including 960x600. Latest logs are `build-ninja-debug/interface-validation.log` and `build-ninja-release/interface-validation.log`.

The font/map textures add 1,529,172 bytes (about 1.46 MiB). The current stationary Release report records 25 draw submissions, 0.123 ms CPU median and 0.127 ms GPU median; this hidden scene is not an interactive/fullscreen FPS guarantee. See docs/interface.md for the full limits and benchmark conditions.

Updated README, START_AND_DEBUG.md, architecture notes, roadmap and the manual checklist. Real mouse/keyboard use, Alt-Tab, multiple physical monitors, display-scaling changes and long sessions remain manual checks. No commits, staging, pushes, downloads or system-setting changes were made. AGENTS.md and prior gameplay work remain preserved.

## 2026-10-17: Allods screenshot interface pass

- Used the user-supplied character/bag screenshot as the visual layout reference: green/gold carved framing, beveled title plaques, serif text, compact item cells, a left character alcove with adjacent attributes, right-hand bag, circular minimap and a bottom combat dock.
- Replaced the exclusive character/inventory overlay with independent side windows. C then I opens both without dimming the world. Each closes independently; Esc opens the live menu and preserves the side-window state. Selecting worn gear opens its details in the bag.
- Side windows allow keyboard movement/combat and preserve camera orbit over the visible world. Their surfaces consume pointer input; menu/map/journal remain modal for player commands. Simulation time continues in every case.
- Kept item ownership, Equip/Drink actions, character appearance and stats connected to the actual simulation. The grid has 24 inspection cells and only owned items; four engraved action-bar cells are unbound. Arbitrary item storage, drag sorting, movable windows and finished item/character illustrations remain future work.
- Added original code-drawn rails, leaf inlays, cloth backdrop and item-slot treatments. Switched the small installed-font atlas to Georgia. Reused the existing map texture for the circular minimap; texture memory remains 1,529,172 bytes for the font and map together. No assets or dependencies were downloaded.
- Extended InterfaceTests for coexisting windows, independent closing, menu restoration, scaled item/close/action clicks, and camera-pointer capture versus the world gap. Hidden smoke captures now include combined equipment/bag at all three sizes (92 captures per configuration).

Validation: Ninja/MSVC Debug and Release pass all seven CTest suites and the four hidden fullscreen/world/UI scenarios. Representative normal HUD, combined side-window and central overlay captures were inspected, including 960x600. Logs: `build-ninja-debug/allods-interface-validation.log` and `build-ninja-release/allods-interface-validation.log`. Combined previews use the `-equipment-bag.png` suffix.

Release stationary HUD benchmark at 1280x720: 26 draw submissions, CPU median 0.264 ms / p95 0.317 ms, GPU median 0.173 ms / p95 0.178 ms (240 GPU samples); working set 51.824 MiB, private bytes 76.105 MiB. This hidden-window HUD workload does not measure combined-window cost or guarantee visible-gameplay FPS. Real input, Alt-Tab, multiple monitors and long sessions remain on the manual checklist.

Repository HEAD changed to `6144929` during this pass through an external action. The agent made no commits or pushes; remaining working-tree edits are left for user review.

## 2026-10-17: real equipment, bag storage and repeatable sparring

- Added a platform-independent Inventory model with one canonical item catalog, 24 bag cells, stack limits and four worn slots. Equipped items have one location. Additions, moves and equipment changes validate capacity, compatibility and expected source items before mutation.
- Connected the Allods-inspired side windows to real storage: drag cell swaps, drag equip/remove, right-click actions, Sort, selected-item comparisons, feedback and a drag preview. Invalid drops preserve state. Escape, closing panels, focus loss, minimization and display changes cancel the drag. Movement remains available behind the side windows.
- Made the tunic, trousers, boots, swords and ward freely equipable/removable. Damage/protection derive from worn gear; unarmed base damage is 6/10, starting clothes add no armor, and ward reduces damage by 2 only when worn. Reward ownership and earned XP remain independent of outfit selection.
- Introduced version-4 saves with exact bag/gear serialization and transactional rejection of unknown items, duplicate unique gear, bad stacks, wrong slots and inconsistent rewards. Versions 1–3 migrate with their potions, rewards and sword selection preserved. Old campaign fixtures remain useful migration coverage.
- Refined character proportions, tunic layering, armor crest/shoulders and boots. World and preview use the same outfit. Empty weapon/feet slots remove those visuals; neutral underclothes remain when clothing is removed. Geometry/material resources are reused; no asset download or new texture allocation was needed.
- Added a dedicated 180-HP sparring guard at the eastern training post after both quests. Rounds last 30 seconds, use the real combat/armor rules, stop through E/arena exit and end nonlethally at 1 HP. Reports show damage dealt/taken. No XP, gold, loot or campaign credit is granted and resources are not refunded. Practice is transient across save/load; health, consumed potions, cooldowns and gear persist.
- Added training markers, prompts and a live round/result panel. Nearby unclaimed campaign loot takes precedence over starting practice. Kept all development on the existing map.
- The new sparring capture exposed a camera collapse beside a tree. Camera collision now uses trunk bounds below the canopy and wider crown bounds above it. Regression checks retain crown/roof obstruction while allowing a clear retreat underneath foliage. The corrected training view was visually inspected.

Validation: Debug and Release Ninja/MSVC builds pass all eight CTest suites. Expanded InventoryTests covers capacity, swaps, stale commands, sorting, unarmed damage, migration and malformed version-4 saves. InterfaceTests exercises scaled drag/drop, right-click, invalid drops, cancellation and round trips at 960x600, 1280x720 and 2560x1080. TrainingTests verifies actual armored/unarmored hits (6/8), victory, stop/restart, timeout, arena exit, nonlethal yield, consumable costs and no farming.

Both configurations pass 96 hidden smoke captures: the existing 92 world/interface views plus veteran base/travel outfits and sparring with/without side windows. InterfaceTests also captures a bag drag separately. Representative combined, outfit, drag, small-window and corrected sparring views were inspected. Latest logs: `build-ninja-debug/inventory-validation.log` and `build-ninja-release/inventory-validation.log`. Latest executables: `build-ninja-debug/RPGNative.exe` and `build-ninja-release/RPGNative.exe`. The older generated Visual Studio solution output still needs rebuilding before use.

Release stationary HUD measurement at 1280x720: 26 draw submissions, 112,348 submitted triangles, CPU median/p95 0.281/0.316 ms, GPU median/p95 0.178/0.181 ms (240 samples), working set 52.371 MiB and private bytes 75.574 MiB. Mesh bytes remain 70,496; materials/shadows 5,155,484; font/map textures 1,529,172. This hidden stationary workload does not measure moving cameras, combined windows, practice combat or long-session performance.

Updated README, launch/debug manual, gameplay/interface/architecture/graphics notes, roadmap and manual checklist. Real desktop input, focus/display transitions, combat feel and finished art remain pending. No commits, staging, pushes, downloads or system changes were made. AGENTS.md and the prior development remain preserved.

## 2026-10-17: input and save bug fixes

- Reproduced right-clicks reaching old bag cells immediately after opening the menu. Panel changes now invalidate hit regions; dispatch also checks rendered state/dimensions. Closing a menu cannot activate its invisible Exit button, loading cannot use old equipment hit regions, and resizing cannot complete a stale drop. HUD visibility changes invalidate hits as well.
- Reproduced camera rotation after an RMB gesture began in the bag and crossed into the world. Orbit now requires a fresh RMB press over the world; side windows and interactive HUD controls capture their gestures. Focus, resize/fullscreen, load and restart cancel orbit ownership along with item dragging. A new world gesture still rotates normally.
- Reproduced G failing to re-equip an unequipped starter sword before the patrol reward. It now restores that owned sword while retaining the normal reward-sword toggle after hand-in.
- Reproduced a load accepting an active patrol with an already cleared camp, leaving no kill available to advance the quest. Legacy and version-4 loads now reject that inconsistent state transactionally. Valid kills before quest acceptance and cleared patrols awaiting hand-in remain supported.
- Separated Win32 sampling from input transition calculation through `Input::Frame`, so tests exercise the production input/Game path without synthesizing desktop input. Added focus loss/restoration, bag/minimap-to-world orbit, fresh orbit, load, stale menu/gear hits, resize cancellation, starter re-equip and invalid quest-save coverage.

Reproduction evidence: `build-ninja-debug/bug-reproduction.log` failed on the stale menu hit and starter hotkey; `input-bug-reproduction.log` failed on camera gesture leakage; `save-bug-reproduction.log` failed on inconsistent quest state. After the fixes, Debug and Release pass all eight suites and all 96 smoke captures per configuration. Final logs: `build-ninja-debug/bugfix-validation.log` and `build-ninja-release/bugfix-validation.log`. Both `RPGNative.exe` outputs were rebuilt, and `git diff --check` passes.

Updated interface/gameplay/architecture notes, roadmap, launch manual and the interactive checklist. The automated input checks do not claim an actual Alt-Tab/multi-monitor playtest. Prior work and AGENTS.md are preserved. No commits, staging, pushes, downloads or system changes were made.

## 2026-10-17: quartermaster, six-slot equipment and persistent trading

- Continued the roadmap's village/equipment milestone on the existing island, guided by the supplied Allods character/bag composition. Added a central green/gold quartermaster window alongside the independent character and bag windows, with stock, prices, effects, availability reasons and purchase feedback. E opens/closes trade; the world stays live while local movement/attacks are captured. Disabled combat slots are visibly dimmed while shopping.
- Added a village quartermaster at `(3, -12)`, an orange atlas/minimap marker, and a wooden supply counter with bottles. The counter is solid shared map data, so navigation, movement, camera queries and map artwork agree. The NPC and counter reuse existing resources.
- Added a limited offer catalog: six draughts at 6 gold each, a helm at 20, and a shield at 35. Gear requires the patrol hand-in. Simulation validates range, life/grounded state, quest, stock, gold and capacity before updating item, charge and purchase count together. Purchases are stowed first. No selling, refund or restock exists; death recovery and loading preserve spending/stock, while R resets the expedition.
- Activated Head and Off hand slots. The helm adds 1 protection and the shield 2 while worn; with ward armor, an actual normal 8-point hit becomes 3. Both items appear in the world and portrait. Drag/right-click equip/remove, comparisons and exact placement use the existing inventory rules. Added portrait rotation arrows and Front reset.
- Introduced save version 5 with bounded SHOP receipts and six-slot equipment. Validate gold against earned rewards minus purchases, stock limits and actual item ownership. Versions 1–4 migrate; version 4 retains exact previous cells/worn slots and starts with empty new slots. An older actor position inside the newly solid counter moves to its nearest free edge instead of losing access to that save. Current saves inside solid geometry remain invalid.
- Added TradeTests for actual charges/items, sold-out and insufficient-gold rejection, quest/range/death/jump checks, fully armored combat, purchase persistence, forged receipts, stock bounds, respawn/reset and v4/v5 migration. NavigationTests checks routing around the counter. InterfaceTests drives shop opening, purchase and E close through sampled Game input, and scaled buy/drag/rotation controls with all three windows visible.
- Added eight veteran render captures: shop and purchased outfit at three sizes, a world close-up and equipped sparring. Inspected representative shop/gear/portrait views at 960x600 and 1280x720. The dressed world capture now uses clear ground and faces the player, avoiding overlap with the merchant during inspection.

Validation: Debug and Release Ninja/MSVC builds pass all nine CTest suites and 104 hidden smoke captures per configuration. Logs are `build-ninja-debug/quartermaster-validation.log` and `build-ninja-release/quartermaster-validation.log`; updated executables are in the same directories. The separate `inventory-drag.png` integration capture is not included in the 104. Player saves were not overwritten. `git diff --check` passes.

Release stationary HUD benchmark: 26 draw submissions, 125,948 submitted triangles, CPU median/p95 0.293/0.355 ms, GPU median/p95 0.179/0.183 ms (240 samples), working set 52.055 MiB, private bytes 75.711 MiB. Meshes remain 70,496 bytes, materials/shadows 5,155,484, font/map textures 1,529,172. This hidden stationary workload does not measure interactive FPS, shop-window cost, movement spikes or long sessions.

Updated README, START_AND_DEBUG, gameplay/interface/architecture/graphics notes, roadmap and the manual checklist. Official Allods/Blizzard merchant and equipment references were checked through indexed excerpts and linked in docs/interface.md; rules/prices and code-drawn artwork here are original. Real desktop input, Alt-Tab/monitor behavior, combat feel and finished character/environment art remain pending. No assets/dependencies were downloaded or system settings changed. The agent performed no commits, staging or pushes. HEAD changed externally to `e6acae0` during the pass; that state was preserved. AGENTS.md and earlier development remain preserved.

## 2026-10-18: fullscreen drop cancellation and defeat recovery

- Reproduced an F11/release race: Game applied the item drop before Engine cancelled gestures for the fullscreen transition. Game now cancels gestures and invalidates hit regions before dispatching pointer input on the F11 frame. The item stays in its original location; a fresh drag afterward equips normally.
- Reproduced Enter being swallowed by the menu, atlas and journal while defeated. Recovery now bypasses central-panel command capture only while health is zero. It uses the existing progress-preserving respawn, retaining quests/items/gold instead of resetting the expedition.
- Defeat now closes the current central panel and cancels drag/orbit ownership, revealing the recovery prompt while preserving side-window state. Players can reopen a menu after defeat and still use Enter. The rendered `defeat-recovery.png` was visually inspected.
- Extended InterfaceTests with sampled Game input for simultaneous F11/drop, a subsequent fresh drag, death during each central overlay, reopening the overlay while defeated, and progress-preserving Enter recovery. The capture is separate from the 104 scenario images and does not write a player save.

Reproduction logs: `build-ninja-debug/fullscreen-drop-reproduction.log` fails on the unintended equip; after that fix, `panel-recovery-reproduction.log` fails on swallowed Enter. The original nine suites passed before adding these cases (`bugfix-2026-10-18-baseline.log`), showing the missing coverage.

Final validation: Debug and Release Ninja/MSVC builds pass all nine suites and 104 hidden render captures each. Logs: `build-ninja-debug/bugfix-2026-10-18-validation.log` and `build-ninja-release/bugfix-2026-10-18-validation.log`. Both executables were rebuilt. `git diff --check` passes. No renderer resources or save format changed; the previous benchmark was not repeated for these input-only changes.

Updated the launch guide, interface/gameplay notes, roadmap and manual checklist. Automated sampled-input and hidden rendering checks do not certify physical F11/Alt-Tab, DPI, multiple monitors or combat feel; those remain manual. Existing local documentation edits and AGENTS.md were preserved. HEAD remains `e6acae0`; all new fixes are uncommitted. No commits, staging, pushes, downloads or system changes were performed.

## 2026-10-18: stack splitting, selling and persistent buyback

- Extended the existing-map equipment/trading milestone with Split half, explicit Sell one actions and a nine-cell buyback shelf. The compact merchant offers and buyback fit between the Allods-inspired character and bag windows. Shelf tooltips show the item, held count, price and eligibility. Right-click retains equip/remove/drink behavior; worn gear must be removed before selling.
- Added validated stack splitting with exact bag-cell selection. Split half moves the floor of half the source count into an empty cell; dragging matching stacks together and Sort merge them. Drinking/selling targets the selected stack, while Q retains first-available consumption. Stale sources cannot consume another stack or grant free healing.
- Added original sale prices for every catalog item and simulation-owned sell/reclaim transactions. Selling transfers one item to bounded merchant holdings and adds its price; buyback returns one to the bag at the same price. Holdings survive leaving, save/load and respawn without expiry; R clears them. Selling does not restock retail offers or change quest XP. Insufficient gold leaves buyback items available for later. Repeated sell/reclaim cycles preserve gold and item counts.
- Introduced version 6 with a BUYBACK record. Save validation reconciles exact gold with campaign income minus retail purchases plus the value of merchant-held items. Player-owned plus merchant-held unique gear must match earned/purchased entitlements, and remaining potions cannot exceed supply. This permits legitimate sale proceeds above 90 gold without accepting invented credits. Versions 1–5 migrate; v4/v5 retain their exact cells and existing gear/purchases with empty buyback. G no longer reports equipping a sold sword.
- Extended InventoryTests with odd splits, full bags, invalid counts/destinations, stale consumption, merging and exact-stack save/healing. TradeTests covers all catalog items through repeated sale/reclaim cycles, missing/worn/stale item rejection, unaffordable buyback, persistent sold quest gear, XP retention, forged but balanced credits, duplicate ownership, range/death/jump rejection, respawn/reset and v4–v6 migration. InterfaceTests exercises the production Game input path and scaled split/sell/buyback/right-click routing at 960x600, 1280x720 and 2560x1080.
- Added three veteran buyback/split-stack captures. Inspected Debug merchant/buyback at 1280x720 and buyback at 960x600. All three windows, gold, shelf prices and bag actions fit. No new mesh, material, texture or asset dependency was introduced. The previous stationary benchmark was not repeated; merchant-window performance and interactive input remain manual validation work.

Validation: Debug and Release Ninja/MSVC builds pass all nine CTest suites and 107 hidden smoke captures per configuration. Logs: `build-ninja-debug/trading-2026-10-18-validation.log` and `build-ninja-release/trading-2026-10-18-validation.log`. Both executables were rebuilt. The separate drag and defeat-recovery integration captures are outside the 107 scenario images. Player saves were not overwritten. `git diff --check` passes.

Updated README, START_AND_DEBUG, roadmap, gameplay/interface/architecture/graphics notes and the manual checklist. Existing local input fixes, documentation and AGENTS.md were preserved. HEAD remains `e6acae0`. All changes remain uncommitted; no staging, commits, pushes, downloads or system changes were performed.

## 2026-10-18: character poses and workload profiling

- Continued the existing-map art/performance milestone. Added shaped head, fitted hair and flat-soled boot meshes; refined torso, collar, facial details, belt, tunic trim, helm fit and ward bracers. The captain now uses the shared helmet outfit. The world and character screen retain one inventory-derived presentation path.
- Added `CharacterPose`, a bounded two-bone joint solver for fixed-length arms and legs. Walking bends knees forward and lifts feet; strikes raise the sword hand while the shield remains forward. Grip, guard, blade and pommel follow the hand and weapon direction. Player attack feedback and enemy windup/recovery drive visual poses without changing combat timing, damage, collision or save version 6. This remains procedural part animation, without skeletal skinning, authored clips or planted feet.
- Added generic segment-aligned mesh drawing and unified the normal/scripted Game simulation and visual step. GeometryTests covers finite joints, limb lengths, foot/blade clearance and normalized weapon directions across both shield states and the full gait/attack envelope, including malformed inputs. Existing gameplay/input/trading regressions continue to pass.
- Added eight neutral-lit character captures: front, profile, back, left/right walking extremes and start/mid/end strike poses. Visual checks exposed floating shirt trim, an overhanging hair cap and a cropped raised sword; adjusted the trim/hair and inspection framing. Inspected the corrected profile, walking and strike views plus the equipped 960x600 side-window layout.
- Added `-BenchmarkSuite` / `--benchmark-suite` with stationary HUD, village walking/orbit, repeated combat/orbit and merchant/character/bag scenarios. Each measures 600 frames after 90 warmup frames, excluding setup/snapshot reloads. The measured walk covers 30.835 world units; combat records 309 windup frames and six received hits. GPU observations are asynchronous, and the suite never writes player saves.

Validation: Debug and Release Ninja/MSVC builds pass all nine CTest suites and 115 hidden smoke captures per configuration. Final logs contain no compiler warnings/errors: `build-ninja-debug/character-2026-10-18-validation.log` and `build-ninja-release/character-2026-10-18-validation.log`. Both executables were rebuilt. Separate drag/defeat integration captures remain outside the scenario count. Representative render captures were visually inspected; real desktop input and animation feel remain on the manual checklist.

The shared mesh library grows from 70,496 to 95,440 bytes (+24,944 bytes), within its 256 KiB budget. Material/shadow and map/font allocations remain 5,155,484 and 1,529,172 bytes. The final hidden Release workload has CPU medians of 0.291/0.276/0.290/0.780 ms and GPU medians of 0.180/0.155/0.160/0.352 ms for stationary/walk/combat/merchant. End working set/private bytes are 59.172/83.832 MiB. See [graphics.md](docs/graphics.md) for the before/after table, draw/triangle costs, sample counts and limitations; these short runs do not establish interactive FPS, long-session stability or a speedup.

Updated README, START_AND_DEBUG, roadmap, graphics/interface/architecture notes and the manual checklist. Recorded the user's rule to supply English commit text after substantial batches. Official Allods class descriptions and a Blizzard combat-animation indexed excerpt informed the armored-warrior reference and responsiveness constraint; links and access limits are in docs/graphics.md. The procedural art remains a prototype, with authored character quality and environment refinement still planned.

HEAD changed externally from `e6acae0` to `fe5a4d3` (0.0.7, stack splitting/selling/buyback) during the pass; that commit was preserved. This character/profiling batch remains uncommitted. No staging, commits, pushes, downloads or system changes were performed by the agent; AGENTS.md and player saves were preserved.

## 2026-10-18: enemy movement and defeat animation fixes

- Fixed enemies walking in place during melee recovery. The old renderer treated a nonempty navigation path as movement, even when the enemy had already stopped within attack range. Simulation now exposes successful navigation travel for the current update, and the renderer uses that to enable walking. The transient value resets each update and on load; it adds no save fields or GPU resources.
- Fixed returning enemies walking home backward while retaining their previous facing. Their facing now follows actual travel along the return route. Pursuit and committed attack-facing rules remain intact.
- Fixed a player strike freezing indefinitely if a fatal counterattack landed in the same update. Fatal damage now clears attack feedback before subsequent defeated updates return early. Both attacks still resolve, and normal progress-preserving recovery remains available.

Added regression cases before fixing return facing and fatal-counterattack feedback. Both failed against the previous implementation; the reproduction log is `build-ninja-debug/bugs-2026-10-18-before.log`. Extended NavigationTests to cover actual movement distance, home arrival, a nonempty path while stationary, windup and save/load resetting transient motion. CombatTests checks simultaneous strike/fatal damage and the absence of a lingering strike afterward.

Validation: Debug and Release Ninja/MSVC builds pass all nine CTest suites and 115 hidden smoke captures each. Logs: `build-ninja-debug/bugs-2026-10-18-validation.log` and `build-ninja-release/bugs-2026-10-18-validation.log`. The existing input/trade/save regressions remain passing. No benchmark was repeated for this fix; the recorded character-pass measurements remain historical. Real input, animation feel and long-session behavior still require the manual checklist.

Updated architecture/graphics notes, roadmap and the manual checklist. Previous uncommitted character/profiling work and AGENTS.md were preserved. HEAD remains `fe5a4d3`; no staging, commits, pushes, downloads or system changes were performed, and player saves were not overwritten.

## 2026-10-18: shared ground routes and bounded foliage

- Continued the environment milestone on the existing test island. Added ten shared trail segments, paved village/plaza surfaces and dirt routes to the dock, training post and anchor. The world shader, atlas and vegetation exclusions use the same surface data. The renderer uploads a generated 256-square dirt/paving weight texture once, with mipmaps and normalized quantization; terrain height and actor collision remain unchanged.
- Replaced solid crown shapes with a shared mesh of 144 folded leaves, varied in size, rotation and placement, with explicit back faces and readable underside lighting. Added 131 grass clumps and 18 shrubs with deterministic bounded placement outside roads, props, services, the dock and sparring space. Added branch twigs and route-following verge stones. Grass receives shadows but skips casting; no transparency sorting, downloaded assets or extra material layers are needed.
- Added four environment captures: island overview, canopy, training ground and dock. Visual inspection exposed regular leaf alignment and the old dock overlapping a trunk/building. Refined the leaves and grass distribution, corrected inspection framing and reduced the dock footprint. Dock bounds now serve world art, atlas and cover placement; existing gameplay positions/save version 6 remain valid.
- Extended NavigationTests for visible-route traversal, valid ground weights, deterministic cover, service/arena clearance and a dock clear of solid props. GeometryTests checks grass/crown bounds and the expanded resource budget. InterfaceTests checks weight quantization and correct world-to-texture orientation. Existing campaign, trade, save, input and animation regressions remain passing.

Validation: Debug and Release Ninja/MSVC builds pass all nine suites and 119 hidden scenario captures per configuration. Logs: `build-ninja-debug/environment-validation.log` and `build-ninja-release/environment-validation.log`, with no compiler warnings/errors. Corrected canopy, dock, training, overview and atlas captures were inspected. The separate drag/defeat integration captures remain outside the scenario count. Physical input, moving-camera artifacts, finished art quality and long-session performance still require the manual checklist.

Recorded a same-harness Release baseline before this pass and a final four-scene workload run. Mesh storage grows by 69,616 bytes to 165,056; terrain weights add 349,524 bytes, bringing world material/weight/shadow storage to 5,505,008 bytes. Map/font storage is unchanged. Final CPU medians are 0.312/0.297/0.326/0.802 ms and GPU medians 0.203/0.176/0.187/0.376 ms for stationary/walk/combat/merchant. Each scene adds one submission. See [graphics.md](docs/graphics.md) for before/after tables, sample counts, memory and spike limitations. These are short hidden workloads, not interactive FPS guarantees.

Updated README, launch/debug manual, roadmap, graphics/interface/architecture notes and the manual checklist. Reviewed official Allods Svetolese reference material and a Blizzard level-design indexed excerpt; source/access details are in the graphics notes. This remains compact procedural prototype art, with varied props, authored clothing, animation and audio still ahead. HEAD remains `fe5a4d3`; preceding uncommitted work and AGENTS.md were preserved. No staging, commits, pushes, downloads, player-save writes or system changes were performed.

## 2026-10-18: Dawnwatch building and landmark identity

- Added presentation roles to the four existing buildings: homestead, watch hall, storehouse and training hall. Shared `BuildingStyle` palettes drive both world roofs and atlas colors. `LandmarkView` draws shuttered windows, side windows, timber/gable braces, door frames, crests, banners and emissive lantern props. Building interiors remain closed.
- Added shared barrel, masonry arch, bowed canopy and beveled stone meshes. The quartermaster now has a framed striped supply stall with bottles, a tied parcel and a banded barrel. The northern ruin gains visible masonry, paired arches/pilasters, rubble and a banner; its filled gateway agrees with the existing solid footprint. No external files, new textures or material layers were introduced.
- Replaced the keeper's geometric proxy with a dressed character under a gold marker. Quest position, interaction range, rewards, campaign progression and save version 6 remain unchanged. This is static NPC presentation, without new dialogue/AI behavior.
- Refined camera handling around the stall: counter, uprights, upper braces and bowed cloth block it, while the open bay and sky stay clear. Added a regression ensuring the ordinary merchant camera retains its distance. Building camera depth includes the new gable details. Existing actor/navigation footprints remain unchanged, so prior saves and campaign routes remain valid.
- Added three environment captures for village, stall and ruin. Initial inspection exposed buried ruin stones, blank street-facing walls and cropped gables; corrected masonry depth, added side windows/framing and widened the view. Inspected the corrected landmarks, spawn HUD, atlas and three-window merchant in Debug.

Validation: Debug and Release Ninja/MSVC builds pass all nine suites and 122 hidden scenario captures per configuration. GeometryTests validates canopy bounds/clearance and the arch opening alongside full topology and resource checks. NavigationTests exercises canopy/posts/open-bay/above-roof and merchant/gable camera paths. Existing save/trade/campaign/input/animation regressions remain passing. Final logs contain no compiler warnings/errors: `build-ninja-debug/landmarks-validation.log` and `build-ninja-release/landmarks-validation.log`. Player saves were not overwritten.

Captured a Release baseline before changes and a final four-workload report. Geometry grows from 165,056 to 224,320 bytes (+59,264 bytes), within the 256 KiB budget. World material/weight/shadow storage stays at 5,505,008 bytes and map/font storage at 1,529,172. CPU medians change from 0.316/0.310/0.320/0.793 to 0.350/0.331/0.354/0.855 ms; GPU medians from 0.202/0.176/0.185/0.370 to 0.207/0.182/0.194/0.375 ms in stationary/walk/combat/merchant order. See [graphics.md](docs/graphics.md) for sample counts, draw/triangle costs, p95/maxima and memory snapshots. Physical input, camera feel, final art and long sessions remain unverified.

Updated README, launch guide, roadmap, architecture/interface/graphics notes and the manual checklist. Revisited official Allods settlement references and a Blizzard level-design indexed excerpt; links and access limits are in the graphics notes. HEAD remains `fe5a4d3`; preceding character/foliage/bug-fix work and AGENTS.md were preserved. No staging, commits, pushes, downloads or system changes were performed. All work remains available for the user's manual commit.
