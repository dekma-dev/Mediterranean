# Character stage and combat presentation reference pass

Reference root: `D:/assets/learning_material`. Date: 2026-11-06.

## Weapon-bound cuts and continuous attack handoffs (2026-11-11)

Decoded twelve additional 50 ms samples from `allods_fight_interface.mp4` at 121–121.6 seconds. The inspected warrior/dummy sequence separates the moving weapon, contact flash and persistent ground aura; they should not share one decorative trajectory. This is a short sampled comparison, not full playback or a claim of 1:1 fidelity. Blizzard's [combat-animation discussion](https://worldofwarcraft.blizzard.com/en-us/news/20946579/patch-73-new-combat-animations-its-magic) also describes preserving responsiveness while improving distinct timing and weight. No reference assets were downloaded or copied into the game.

- Blade history now reconstructs measured grip/tip motion on a fixed 120 Hz clock, with a separate live endpoint. Direction and length interpolate together, avoiding a collapsing blade chord. The existing 160 ms lifetime and bounded eight-trail pool remain. Both world combat and creation rehearsals use the same equipment span.
- Once weapon measurements exist, they replace the independent generic crescent. A narrow pale edge, steel/blue body and restrained outer veil follow the real cut, with brightness driven by tip speed. A stationary weapon produces no invented slash. Invalid/degenerate spans, large grip/tip jumps and nearly reversed discontinuities cancel history.
- The integrated transported grip follows the forearm, with support only near axial alignment, preserving relaxed carrying. Weighted weapon orientation follows a unit arc rather than normalizing a near-zero sum. Blade recovery decelerates for longer; cancelling a wind-up blends its incoming stance back to guard over 200 ms instead of snapping on the last sample.
- Chest/pelvis loading now travels with the outgoing attack through interruption, like its arm frame and ability accent. Previously, alternating normal/backhand attacks reset that load from the incoming phase immediately, moving the hand despite its blended clip. Gameplay and creation share these channels, including incoming stance.

These changes are presentation-only: hit/release clocks, cooldowns, damage, targeting, saved games, models, faces and authored animation files are not changed by this pass. Concurrent authored-motion/equipment edits belong to the separate art workstream and were preserved.

The existing 13 m/s motion limit is retained. Regression coverage includes all four focused class styles, ten action IDs each, early cancellation, planted feet, and alternating cuts at 30/60/144/1000 Hz. Trail tests check history density/duration and native equal-time appearance at 22/30/60/144/1000 Hz, stationary blades, malformed endpoints, expiry and render purity. Native combat also captures basic/heavy/backhand preparation phases, contacts and multiple camera distances. Reference frames, diagnostics and final build/test logs are under `build-action-continuity/`; native test images remain in the isolated build directories. Full-speed subjective review, crowded effects and complete reference fidelity remain open.

Final validation: RPGNative and all six affected suites build and pass in isolated Ninja Release and Debug: SignaturePresentationTests, CombatEffectsTests, AuthoredLocomotionTests, ClassEquipmentTests, AnimationTests and CharacterSelectionTests. The final integrated runs take 109.09 s / 255.08 s. The new trail-count diagnostic also skips expired actor records when a later attack occupies another pool slot; its regression and the full CombatEffectsTests suite pass again in both configurations. An initial menu timeout during concurrent compilation was resolved by isolated reruns without increasing its limit. No motion assertion was relaxed.

The concurrent forearm-following grip is retained and verified together with the synchronized body handoff. Alternating-cut peaks are 6.30062 / 7.7079 / 8.0698 / 8.30928 m/s at 30/60/144/1000 Hz; the single/cancelled maximum is 12.6703 m/s. Final carrying alignment is 0.853693, with maximum roll-vector change 0.000799068 per ms. This preserves relaxed carrying instead of using the earlier positive-axis grip regularization. Source hashes confirm the inspected motion implementation remained unchanged during final validation.

Both final native EXEs pass hidden fullscreen restoration, device/shaders, resize, world/UI rendering and PNG capture. Reviewed actual blade trails from multiple angles, the returning-cut phase sequence and Warrior creation rehearsal; the phase-capture camera now selects an unobstructed orbit instead of retracting into the hero behind a tree. Stable captures are `heavy-before.png`, `heavy-after.png`, `strike-after.png`, `returning-cut.png`, `creation-warrior.png` and `smoke-final-{release,debug}.png` under `build-action-continuity/`. Final evidence uses `build-integrated-*`, `tests-integrated-*`, the focused `build-query-*` / `tests-query-*` follow-up, `smoke-final-*.png.log` and `final-source-hashes.json`. Launch `build-learning-review/release/RPGNative.exe`. Full-speed subjective acceptance and complete reference fidelity remain open.

## Directional contact choreography (2026-11-11)

Revisited the existing PvE contact sheets and decoded sixteen 50 ms samples from `pve_loc2.mp4` over 7.5–8.3 s. The inspected frames show a clear incoming streak/vertical contact, a short bright hit and a separate decay, while the target remains identifiable. This is a sampled comparison, not a claim to have watched every frame or completed full-speed acceptance. The class-specific separation is also consistent with Blizzard's [combat animation discussion](https://wow-site-bwa-production-eks-prod-use1-01.worldofwarcraft.blizzard.com/en-us/news/20946579/patch-73-new-combat-animations-its-magic) and the official [Allods class guide](https://allods.ru/about.php?show=classguide); the local recording remains the visual reference.

Replaced the shared Mage radial spokes/pressure skirt with element-specific contact motion. Fire opens three continuous curled crests around larger directed flame bodies, then yields to smoke. Frost throws crossed ice shards through depth with gravity and short trails. Arcane contacts unfold three glass-like planes and curved pressure paths; the prism retains its additional seal. Thermal rupture retains both materials, now aligned to the accepted attack direction. The Priest's single flat impact/curtains become staggered crossed shafts, opening vaulted ribs and ascending flecks. Warrior slash surfaces travel through the stroke instead of staying fixed, and Scout contacts throw a small backward chip cone. Existing ability-specific major spectacles, projectiles and terrain aftermath remain connected to their accepted events.

Native before/after inspection caught dotted curve segments and an overly flat frost side view. Continuous tapered ribbons replace per-segment streak end-fades, and frost has increased depth dispersion. This uses the existing material atlas; there are no new textures, downloads, model changes or simulation particles. Effects use absolute contact age rather than accumulating render steps. The existing 6,144-vertex pool and 96 alpha-sprite ceiling remain unchanged. Secondary area victims have smaller local contacts and cannot repeat the primary crests, shafts or ground seal. Invalid position/height/facing/age or non-hit events are rejected before either dedicated or legacy contact geometry is submitted.

Review captures and build/test logs are in `build-contact-volume/`. The expanded native suite checks four phases from three views for eight representative contacts, equal-time rendering, reset/expiry, invalid events, reduced secondary contacts and six simultaneous major spells without silent geometry truncation, retaining a 1,000-vertex allowance for other cues. Actual world attacks also capture two wider camera angles. Full-speed subjective acceptance, encounter composition, sound mix and complete 1:1 fidelity remain open. Models, frozen faces, authored clips, damage, cooldowns and save formats are outside this pass; concurrent model/animation work is preserved separately.

Validation result: RPGNative and the four relevant test hosts build in Ninja Release and Debug. CombatEffectsTests, ClassEffectsTests and VfxMaterialsTests pass in both configurations (Release 76.74 s combined; Debug 99.22 s). The new cases include 96 phase/view captures. SignaturePresentationTests now reports the exact motion case and continues its independent native audit without suppressing the eventual failure. Its native signatures, actual four-class combat, unique area contact/audio and load-isolation checks pass in both configurations. An obsolete assertion was corrected to distinguish the short burst lifetime from the already-existing longer ground aftermath, and verify both drain separately.

**Failure recorded at this earlier checkpoint:** the full SignaturePresentationTests suite was not green. Its pose-only audit reported Warrior `Sweeping steel` (style 0, ability 8, not cancelled), peak hand speed 20.8009 m/s against the existing 13 m/s limit; accent and final settling were present. An earlier build exposed heavy ability 1 at 15.9398 m/s before concurrent animation changes. No threshold was relaxed and no model/pose implementation was changed by the directional-contact pass. Logs: `signature-final-release.log` and `signature-final-debug.log`. See the later weapon-bound-cut/attack-continuity validation above for the resolution and remaining acceptance limits.

The Release native workload on the RTX 4060 uses a frozen 1920x1080 world with vsync off, 60 warm-up and 180 measured frames per phase. Six mixed contacts plus six aftermath patches peak at 3,240 effect vertices and 227 draws; the world alone has 225 draws. CPU median was 5.41 ms without contacts and 5.55 ms with them (p95 6.02/6.03 ms). These are local synthetic submission measurements, not an interactive FPS guarantee; only 9/21 asynchronous GPU samples were available. Exact conditions/results are in `workload/workload.txt`. No performance improvement over an earlier implementation is claimed.

Both final RPGNative smoke runs pass hidden fullscreen restoration, device/shader initialization, resize, world/UI rendering and PNG capture. Stable accepted world images are `build-contact-volume/world-fire.png`, `world-frost.png`, `world-arcane.png` and `world-holy.png`; native startup images/logs use `smoke-release` / `smoke-debug`. Scoped `git diff --check` passes. The reviewed executable is `build-learning-review/release/RPGNative.exe`. No commit or push was performed.

## Native combat density and glass vitality frames (2026-11-11)

Reference: revisited the creation and raid contact sheets and decoded eight additional quarter-second raid samples over 22–24 s. The inspected 23 s frame shows a compact bottom action cluster, smaller minimap and faceted green vitality bars. Our 1920x1080 combat capture instead inflated its action controls from a 1280x720 canvas: the action dock was about 888 pixels wide. This is a sampled visual comparison, not full-speed combat or audio acceptance.

- Added a bounded combat transform shared by drawing and generated hit regions. The two action rows, their hover cards, carried spell icons, preparation/chain indicators, incoming-attack board and minimap retain a compact desktop density. At 1920x1080 the action dock is approximately 604 pixels wide. The 960x600 and 1280x720 layouts keep their existing scale; 4K scaling is bounded to preserve readability. Character creation and inventory/build window scale are unchanged.
- Hover cards attach above the actual hovered action row, rather than floating in the middle of the battlefield. Spell dragging, swapping, unbinding and spellbook assignment use the same transformed targets. Resizing cancels stale pointer gestures. Incoming-attack and minimap reservations follow their visible bounds so nameplate packing does not reserve the old oversized rectangles.
- Player and target vitality frames now have faceted brass edging and clipped green glass, retaining the authoritative HP values and amber delayed-damage trail. Cooldown sweeps have a visible leading edge and a restrained amber final-second accent; actual cooldown completion keeps its separate ready flash.
- Cast-result frame, icon and text use the same existing release/interruption opacity as the fill. The final frame no longer leaves a fully opaque label or icon that abruptly disappears. Preparation/release clocks and accepted combat events are unchanged; native Mage/Scout/Priest captures follow the fade through expiration.

Validation: native RPGNative, InterfaceTests and CombatHudTests build in Ninja Release and Debug. Both suites pass in each configuration after the final fade correction (Release 25.59 s; Debug 52.12 s). New checks cover all 20 action cells at 960x600, 1280x720, 1920x1080 and 2560x1080, cross-row swaps, removal into the world, minimap picking, stale resize gestures and unchanged gameplay. Existing tests also exercise spellbook assignment, hover descriptions, panels and selected-threat routing. Native combat, small-window, tooltip and Mage/Scout/Priest cast-fade captures were reviewed; stable images and logs are in `build-hud-density/`. Models, facial data, authored animation and combat rules are outside this pass. Complete reference fidelity and full-speed subjective acceptance remain open.

Startup acceptance for the density pass: both Debug and Release RPGNative smoke runs pass device/shaders, fullscreen restoration, resize, world/UI rendering and PNG capture. Scoped `git diff --check` passes. No files were downloaded and no commit or push was performed. Concurrent model/animation changes from other work were preserved and are not attributed to this pass.

## Independent heroes and reversible identity choices (2026-11-11)

Reference: revisited both creation contact sheets (0–92 s), then decoded nine additional one-second samples over 78–86 s of `character_creating.mp4`. The inspected 82 s frame has a compact name plaque, numbered appearance selectors and an enamel/brass dice control on the lower rail. Existing model/face restrictions limit this implementation to the available color choices; race, facial anatomy and new hair/body variants are not fabricated.

- Replaced the roster's class placeholders with actual saved-character rows: literal nickname, class, level, selected crest, eight entries per page and a 32-character creation limit. Mouse, page arrows, wheel, Tab and Up/Down navigation are connected. A missing default save now falls back to an existing readable hero. Unreadable saves remain visible and cannot enter the world.
- Multiple characters of one class are supported. Legacy `patrol.save` / `hero-<class>.save` files remain readable in place; additional characters use `<base>-character-<slot>.save`. No existing save is renamed or overwritten during creation. File publication remains exclusive and bounded. Competing creation preserves the losing draft for an explicit retry into another free file. Returning from gameplay selects the exact entered file rather than the first hero of that class.
- Selection reads and validates the selected file without reparsing every other campaign. Opening, explicit Refresh, creation and world entry rescan the roster. The displayed snapshot is rechecked before entering; external replacements require review. A repaired additional save can recover through Refresh, and a cancelled creation restores the prior selection.
- The lower identity rail now has a dice control and undo medallion. Randomization always chooses a different valid combination of the existing colors. Both random and manual color changes enter a bounded 32-step undo history; moving back to the class page retains that history. Name/class, live world and disk profiles are untouched until confirmation.
- Scout defensive events now distinguish an actual evasion from a ward absorption. Only a confirmed evasion emits the passing air/dust streak; absorbed hits use a compact contact at the body. Damage/mitigation rules are unchanged, and a full visual queue cannot accidentally mark an older event as evaded.

Validation: final native RPGNative and focused test targets build in Ninja Release and Debug. CharacterSelectionTests, InterfaceTests, LocalizationTests, ClassEffectsTests and ClassCombatTests pass in both configurations (Release 59.66 s; Debug 192.31 s). Regression coverage includes same-class independent profiles, legacy preservation, exact-file menu return, fallback selection, changed/corrupt/repaired saves, creation races and draft retry, paging/keyboard/wheel navigation, the creation limit, bounded color randomization/undo and confirmed Scout evasion versus absorption. Inspected native roster captures at 960x600, 1280x720 and 2560x1080, the identity page and both Scout contact forms. Stable review images and logs are in `build-roster-review/`. This pass does not edit models, authored animations or facial data. The remaining scene/actor staging and broader combat/reference parity still require full-speed comparison and further implementation.

Startup verification: both native RPGNative smoke runs passed device/shader initialization, fullscreen restoration, resize, world/UI rendering and PNG capture. Scoped `git diff --check` also passes. These automated checks and still captures do not replace full-speed visual/audio acceptance against the recording.

## Class crests, milestone inspection and defensive contacts (2026-11-11)

Reference: decoded 25 full-resolution samples at quarter-second intervals over 8–14 seconds of `character_creating.mp4`; inspected the 9 s and 13 s frames against the native creation capture. Revisited two previously decoded raid frames near 31 s and 33 s. The class list uses restrained translucent bands, right-hand heraldic emblems and a clear central stage. The raid samples distinguish persistent state artwork from brief contacts. This is a sampled visual review, not a claim of complete frame-by-frame or audio acceptance.

Implemented:

- The creation/roster class rail now uses beveled crests with existing weapon artwork, inset enamel, selection engraving and quieter separators. Existing selection bounds and class/profile behavior are retained. A selected class's delayed description no longer covers its automatic entrance; other classes remain inspectable.
- Creation's milestone tab now draws the actual six-node dependency graph for each branch, including the two separate choices from the foundation. Hover highlights the selected prerequisite path; clicking or keyboard activation pins a complete card with the class-specific effect, description, minimum level and prerequisite. Escape closes the pinned card first. Details consume pointer/scroll input instead of rotating/zooming the stage underneath. This is inspection only: the world N-key build system owns learning and point spending.
- Milestone icons in both creation and the world build screen now use the focused class's action artwork with small depth markers. Mage protection no longer carries the generic green healing icon; movement enhancements use their own class artwork.
- Card summaries distinguish healing from barrier capacity, show the actual combined recovery bonuses, and omit cast-speed claims for instant protection. Non-healer Overflow explicitly shows its twelve barrier capacity.
- Accepted defensive contacts have four distinct treatments: steel glints and ballistic sparks for Warrior, radial cracks and separated ice facets for Mage, passing air/dust streaks for Scout, and layered gold arches/seal for Priest. Geometry uses incoming-hit coordinates instead of a fixed world XY plane. Contacts retain the existing 380 ms lifetime and do not alter damage, mitigation, evasion or cooldown rules. Invalid direction/position/age cannot enter the renderer.

Acceptance: native Debug and Release builds passed. CharacterSelectionTests, InterfaceTests, ClassEffectsTests, ClassCombatTests and LocalizationTests passed in both configurations. After the final card-summary correction, the two UI suites passed again (Release 36.19 s, Debug 67.97 s); defensive contacts also passed a stricter foreground-only rotation comparison in both configurations. New coverage includes all focused classes/branches/nodes, 960x600/1280x720/2560x1080 picking, pinned-card keyboard/input isolation, unchanged profile/world state, four incoming directions, class distinction, invalid/expired contacts and eight-contact geometry budgets. Native captures exposed and corrected a bottom-caption overlap. Both RPGNative startup smoke runs passed fullscreen restoration, device/shader initialization, resize, world/UI rendering and capture. Scoped `git diff --check` passed.

Artifacts: `build-class-stage/creation-milestones.png`, `creation-small.png`, `mage-defense.png`, `priest-defense.png`; build/test/smoke logs are alongside them. Models, authored animation exports and facial data are outside this pass. Scene/background construction, final class action silhouettes, full-speed crowded combat and complete reference fidelity remain open.

## Compact identity controls and complete rehearsals (2026-11-10)

Reference: revisited the creation contact sheet covering 48–92 seconds, then decoded 24 full-resolution samples at quarter-second intervals from 74–80 seconds of `character_creating.mp4`. The inspected late identity screen has a compact name/class plaque on the left, numbered blue/brass selectors on the right and a small lower view/equipment dock. This was a sampled visual review; no model files were opened or changed.

- Reworked the identity screen around a compact name plaque and shorter appearance bands. Removed the tall side scrims on this page, preserving the painted hall and open character stage. Existing choices, literal nickname editing, validation and save transaction behavior remain connected to the same input controls.
- Added layered enamel/brass medallions, diamond selectors and matching lower-dock/primary-action accents using native UI drawing and existing artwork. Hover, focus, selected and disabled states remain distinguishable. No asset download or new texture allocation is involved.
- Rotation arrows now support a short tap and a continuous held turn after a deliberate delay, with eased arrival. Name-editing arrows remain editor commands. Focus/page/resize cancellation clears the held gesture.
- Completed offensive rehearsals with weapon-stroke/projectile release, one scheduled contact and the class's existing impact sound/VFX. Flight duration now uses the same stroke-to-contact interval and class/ability rate as live combat instead of a constant 0.42 seconds. A released projectile retains its sampled destination and facing when the figure rotates. Stop, class/page changes and suspension cancel pending contacts and their audio.
- A currently demonstrating ability no longer covers its own action/effects with a delayed inspection card; other ability cards remain available. Stage contacts do not grant combat rewards, damage real actors or create primary ground-area/aftermath events.

Verification: CharacterSelection, Interface, Audio and Localization suites pass in both Debug and Release. The new checks compare preview contacts against actual simulation hits at 30/60/144 Hz and cover held rotation, cancellation, captured destinations, audio and profile isolation. Both native RPGNative smoke runs pass. `build-creation-flow/` contains reference samples, build/test/smoke logs and stable identity/Mage/Priest review images. Full native capture sets are in the corresponding `build-learning-review/{release,debug}/character-selection-check/` directories. Detailed results are recorded in `progress.md`.

Remaining differences include the reference's additional authored appearance options, actor/environment artwork, exact composition and authored action performance. Those are not represented by nonfunctional controls. Full-speed subjective acceptance and complete 1:1 fidelity remain open.

## Ability inspection and enemy cast results (2026-11-10)

Evidence inspected this pass:

- Revisited the creation and raid contact sheets. Decoded 20 creation samples at 0.25-second intervals from 24–29 seconds and 16 raid samples from 30–34 seconds. Inspected individual decoded frames, including the raid's compact target status timers and central cast/spell arrangement. These are sampled visual observations, not a claim of full-speed or audio review.
- Evidence and build logs are local under `build-cast-readability/`. Reference media and authored models were not modified.

Implemented:

- Creation Arts now provides a delayed mouse/keyboard inspection card with the actual ability artwork, description, targeting, required level, cooldown and range. Preparation/contact and recovery values use the selected class's action rate; defensive preparation uses the actual recovery cast duration. Inspection never starts an action, learns a spell or changes profile data. Selecting an art still uses the existing rehearsal controls.
- Selected enemy casts now retain a brief result: Released (including a miss), Interrupted only after an accepted interrupt event, or a neutral Cast stopped when a visible preparation ends without either event. Actor changes, corpses, reset/load, invalid time steps and practice targets clear the state. An occluded/distant windup is not interpreted as an interrupt. The UI clock advances with simulation, not rendering.
- Target stun, root, slow and vulnerability keep their own slots as earlier effects expire. Colored borders drain during their final three seconds; the displayed remaining time stays authoritative. This does not fabricate a total duration for an effect first observed mid-way. Timed player protection/discipline effects receive the same expiration treatment; absorption amounts are not treated as seconds.
- Successful enemy interruption uses a short, class-colored broken seal: Warrior shear cuts, Scout narrow cuts, Mage displaced arcane planes and Priest a liturgical seal/axial ray. It consumes the existing accepted event and expires within 0.48 seconds, using at most 240 submitted vertices per interrupt in the regression capture. It does not change damage, cooldowns, target selection or attack timing.
- Native screenshot review exposed cast-name/counterplay text colliding with target status timers. Those captions now sit below the cast bar. Ability card bottom padding and numeric precision were also corrected after reviewing 960x600 and 1280x720 captures; new captions have Russian translations.

Verification: all six affected suites pass in Debug and Release: Localization, CharacterSelection, CombatEffects, VfxMaterials, Interface and CombatHud. Both native RPGNative smoke runs pass device/shaders, fullscreen restoration, resize, world/UI rendering and PNG capture. Scoped whitespace checks pass. Logs are in `build-cast-readability/`; captures live in each configuration's `character-selection-check/`, `combat-hud-review/` and `vfx-material-review/` directories. The final creation cards and target-result HUD were visually inspected after correcting the observed padding/translation/caption overlap issues. See `progress.md` for timings and the remaining manual acceptance limits.

Remaining acceptance: review these short events in full-speed crowded combat and the four-class creation flow. Reference-level art, authored appearance variety, animation fidelity, raid-scale gameplay and complete visual/UI parity remain unfinished. These interface/effect changes do not close those requirements.

## Ability gestures and creation rehearsals (2026-11-10)

Revisited the creation and raid overview sheets, then decoded additional
native-resolution samples from `character_creating.mp4` at 0.25-second
intervals between 32 and 42 seconds and from `pve_loc.mp4` between 4 and
7 seconds. The creation samples distinguish the free casting hand from the
held weapon and show a bright, localized invocation; the combat samples
retain separate preparation and release cues. This is sampled visual review,
not complete playback or audio analysis. Evidence is under
`build-class-choreography/reference-creation` and `reference-pve`.

- Mage fire, frost, meteor, arcane and thermal abilities now add distinct
  free-hand paths and body counter-rotation to the existing continuous action
  spline. Priest Absolution, Sanctuary and Guiding light have inward/outward,
  elevated and directed gestures. Both gameplay and the creation stage pass
  the actual ability identity to the controller. Existing accepted attack,
  damage, projectile and cooldown clocks remain authoritative.
  Broad heavy invocations gather earlier, leaving a longer projection path
  before the unchanged release; they no longer cram the added travel into the
  final third of preparation.
- Staff/scepter weapon clips retain the weapon arm while free-hand invocation
  channels remain active, with reduced additive arm/torso travel. Previously
  full stroke weight suppressed every additive action channel. Staff support
  now also yields to the open casting hand during preparation and recovery
  instead of pinning it back to the shaft. Weapon hand attachment and fixed
  limb lengths are retained.
- Replaced wrapped wrist-angle interpolation with a forearm-to-casting vector
  path through a forward opening arc. Partially open hands no longer scale a
  nearly full revolution when passing a vertical/lateral reach. Palm roll is
  built in an explicit orthonormal frame rather than projecting a blended
  palm reference that could align with the finger direction.
- Priest preparation distinguishes paired converging streams, six elevated
  sanctuary ribs and a narrow guiding ray. Arcane preparation adds orbiting
  facets around the solved focus. These use existing materials and actual
  hand/focus sockets, with fewer than 512 submitted vertices per preparation.
  Cancellation immediately removes preparation geometry.
- The creation Arts view has explicit Replay and Stop controls and a
  preparation/release/recovery timeline. Stop clears queued audition sounds
  and transient effects while the action controllers settle naturally. Replay
  retains the selected ability; completion remains visible until another
  action. These controls grant no spell, equipment or progression.

No source models, facial data, sound assets or save format were changed by this
pass. Concurrent authored-motion edits are separate. Full-speed perception,
crowded combat, final art/choreography and 1:1 recording fidelity remain open.
Native review images are `build-class-choreography/priest-invocation.png` and
`mage-meteor.png`; these are engine captures, not mockups. Start
`build-learning-review/release/RPGNative.exe`, open character creation, select
Mage or Priest and enter the Arts view to inspect/repeat each gesture.

Validation: eight affected suites pass in Release and Debug (action clips,
character selection, hands/skinning, combat release, casting effects, interface
and localization). The new Debug sweep exceeded the prior 45-second test cap;
its allowance is now 120 seconds, with all numerical assertions preserved.
The final action/creation follow-up passes, as do native RPGNative startup,
fullscreen restoration, resize and world/UI captures in both configurations.
See `build-class-choreography/tests-accepted-{release,debug}.log`, the final
`tests-followup-{release,debug}.log` and `smoke-{release,debug}.png.log`.

## Class inspection, stage changes and elemental aftermath (2026-11-10)

Revisited the creation overview sheets and decoded additional native-resolution
samples from `character_creating.mp4` at 0.10-second intervals between 19 and
22 seconds, plus `pve_loc2.mp4` at 0.20-second intervals between 5 and 8 seconds.
The creation samples show a briefly empty stage between class presentations
and a detailed hover card beside the class rail. The PvE samples retain light
and ground cues after the first contact. This is sampled visual review, not
complete playback, frame-by-frame review of every recording or audio analysis.
Evidence lives in `build-stage-afterglow/reference-transition` and
`build-stage-afterglow/reference-pve`; source recordings were not modified.

- Class hover/focus now opens a delayed inspection card with the real identity,
  resource payoff and two actual tactical/signature abilities. It uses the
  existing icons and Russian/English translations without selecting the class,
  writing a profile or granting a skill. The card remains beside the class rail
  at small, standard and ultrawide window sizes.
- Class selection changes the stage through a 100 ms outgoing dissolve and
  220 ms incoming reveal. Only outfit/body/view presentation data is retained;
  there is no second simulated hero. The original Hall artwork covers the
  figure during the dissolve, leaving interface rails readable. Rapid switching
  starts from the currently visible opacity; rotation, zoom, explicit auditions,
  page changes, focus loss and display changes settle the transition. Existing
  once-only entrances still start after their settling delay.
- Accepted primary Mage/Priest contacts can now leave a separate ground trace
  after the brief impact expires: cooling embers/soot, shrinking frost cracks,
  arcane facets or sacred arches/motes. Thermal contacts mix hot/cold branches.
  Terrain height is sampled for each ground point. The traces last 1.7-3.2 s;
  they do not deal damage, follow a moving victim or create new area warnings.
  Cancelled casts, flights and secondary area victims cannot create them.
- Twelve fixed records bound history; repeated same-element contacts within
  half a metre rekindle a patch rather than stacking it. At most six patches
  render, sharing a 900-vertex allowance after current danger/release/contact
  geometry. Each patch uses at most 180 vertices. Load/reset clears history;
  no texture asset, download or persistent save field was added.

CharacterSelectionTests checks equal-time transition opacity at 30/60/144 Hz,
rapid switches, explicit controls, suspension, profile isolation and scaled
inspection cards. VfxMaterialsTests covers actual accepted world contacts,
independent lifetimes, equal-time rendering, terrain captures, merging, expiry,
pool saturation, last-priority allocation, invalid data and reset/load.
Normal world/HUD captures are complemented by explicitly HUD-hidden overview
captures of the same accepted hit/time, so interface occlusion does not conceal
the ground treatment during review. These are diagnostic views, not evidence
of a completed live-raid acceptance pass.

Validation: all five affected suites passed in Release and Debug
(CharacterSelectionTests, CombatEffectsTests, VfxMaterialsTests, InterfaceTests
and LocalizationTests). After the native trace-readability adjustment, the two
affected combat/VFX suites passed again in both configurations. Final RPGNative
smoke checks passed device/shader initialization, hidden fullscreen restoration,
resize and world/UI capture in both configurations. Logs are in
`build-stage-afterglow`: `tests-{release,debug}.log`,
`tests-verified-{release,debug}.log`, `build-verified-{release,debug}.log` and
`smoke-{release,debug}.png.log`.

A single Release workload on the RTX 4060 at 1920x1080 rendered the same frozen
world, then six simultaneous mixed contacts with six queued ground traces:
peak VFX geometry was 3090/6144 vertices and draw calls were 225 without versus
227 with these effects. CPU p50 was 7.51/5.46 ms; asynchronous GPU p50 was
6.89/5.46 ms from only 9/22 ready samples. The reversed timing difference and
sparse GPU samples do not establish an effect cost or a performance improvement.
This is a bounded submission check, not interactive FPS or a dense-raid claim.
Full report: `build-stage-afterglow/workload/workload.txt`.

No model/source export, frozen face, sound asset, spell damage/cooldown rule or
save format was edited in this pass. Full reference parity remains open:
authored class choreography, final environment/character art and full-speed
crowded combat need further work and visual acceptance.

## Identity editing, appearance rows and spell receipts (2026-11-10)

Revisited the creation overview sheets (0-44 and 48-92 seconds), prioritizing
the right-hand appearance rows and left identity card. Existing native combat
captures were compared with the previously sampled PvE/raid material. This
is sampled visual analysis, not a new complete playback or audio analysis.

- Replaced append-only nickname entry with a Unicode character editor: real
  proportional-font caret placement, mouse drag/Shift selection, arrows,
  Home/End, whole-character Backspace/Delete and held-key repeat. Ctrl+A
  selects; Ctrl+V filters an explicit paste to the existing naming alphabet
  and twenty-character limit. Empty/invalid paste preserves selected text.
  Native clipboard access occurs only on the user's paste command; synthetic
  test input supplies isolated clipboard text. Creation still validates and
  saves through the existing profile transaction.
- Identity text and its measurements bypass translation explicitly. Names
  such as `Warrior` remain unchanged in the editor, roster, HUD and character
  panel. They no longer prevent unrelated class/interface captions from being
  translated. No saved nickname is rewritten.
- Appearance rows now have teal/brass dividers, circular previous/next buttons,
  a selected-variant number, existing swatches and a separate localized color
  caption. Their vertical spacing avoids overlap at small/full/ultrawide
  sizes. This presents the five existing variants; it adds no new face,
  hair geometry, race or sex options.
- The action bar distinguishes accepted activation from cooldown return:
  a short top sheen accompanies use; a fading corner flare marks actual
  cooldown completion. Timers are per ability, so moving/duplicating bindings
  does not attach feedback to the wrong spell. Initial load, rejected actions,
  death/reset and invalid update clocks do not fabricate readiness events.
  Repeated rendering cannot advance these timers or change the simulation.

CharacterSelectionTests covers Unicode edits, selected replacement, filtered
paste, limits, key repeat, scaled selection, drag-release isolation, literal
measurement, actual creation/save behavior and native appearance captures.
CombatHudTests uses actual accepted casts for all four focused classes at
30/60/144 Hz, checks cooldown transitions, moved bindings, rejected actions,
reset/expiry and repeated rendering. HUD-only diagnostic captures deliberately
omit world geometry; they are not full-play screenshots. Final logs and
curated native captures for this continuation are in `build-identity-feedback`.

All four affected suites pass in Release and Debug; both final RPGNative smoke
checks pass device/shader initialization, hidden fullscreen restoration,
resize and world/UI capture. See `tests-final-{release,debug}.log` and
`smoke-{release,debug}.png.log` in that artifact directory.

No model/source export, audio bank, spell damage/cooldown rule or profile format
was changed in this pass. Full reference parity and crowded live-combat
acceptance remain open.

## Class entrances and attached hostile casts (2026-11-10)

Reviewed the full-size creation frame at 32 seconds and raid frame at 72
seconds alongside the latest native creation captures. The creation reference
uses a held class gesture and hand/weapon light on its open stage; combat
effects have a clear origin and a bright discharge. This pass uses our existing
runtime animation and artwork, not newly authored animations or modified models.

- Entering class creation or choosing a different focused class schedules one
  short presentation after 550 ms. Warrior/Scout demonstrate a heavy action;
  Mage/Priest demonstrate their signature. The normal preparation, release and
  existing sounds run once. Roster browsing remains quiet; the repeated manual
  showcase is still an explicit control.
- Manual input cancels the queued entrance. Inspection, page transitions,
  native suspension and display changes do not replay it later. A manual Pose
  sequence still starts with the basic action after an automatic entrance.
  No preview grants spells/equipment or advances the campaign.
- Wraith/boss charges now follow palm landmarks transformed by the existing
  authored presentation rig's exact skinning palette. Wraith lance and boss
  execution snapshot that animated origin on release, then join the already
  committed cone. The released beam does not follow subsequent actor motion.
  Damage time, reach, safe zones and line-of-sight rules are unchanged.
- Violet gathering filaments distinguish lance/execution; the tempest gathers
  between both hands; quake gathers earth-colored material. These replace the
  detached orbiting charge. Cancellation removes preparation immediately.
- The first contact frame now has a full beam discharge instead of waiting for
  the lingering body to fade in. All warning borders retain first submission
  priority. At most eight selected warnings share the existing 3,600-vertex
  warning/detail allowance; attached charge is omitted when that allowance is
  full. No new textures, sound files or unbounded particle pools were added.

Verification artifacts for this continuation are under `build-class-entrance`.
All seven affected suites pass in Release and Debug: actor presentation,
character selection, combat/class effects, sustained effects, combat release
and casting. Both final executable smoke checks pass. See
`tests-final-{release,debug}.log` and `smoke-{release,debug}.png.log` there.
Native captures are emitted by CharacterSelectionTests and CombatReleaseTests
inside `build-learning-review/{release,debug}`. Compare the four
`automatic-entrance-*.png` images and `world-wraith-{charge,release}-60.png`.
These checks establish working presentation/input integration, not complete
reference parity, final authored choreography or crowded-raid performance.

## Compact creation controls and holy flight (2026-11-10)

Revisited the creation overview, its full-size 52-second customization frame,
and the PvE/raid overviews including the 8-second raid frame. The reference
keeps wardrobe/view controls in a compact dock below the actor, and utility
navigation on the screen edge rather than between cast feedback and spells.
Its bright spell paths retain an identifiable core through the combat scene.
The local decoder also sampled the 38-41 second creation transition at 0.3-second
intervals; inspected frames show the outgoing/incoming figure transition,
compact tooltip and hand-attached glow. Evidence is under
`build-reference-dock/creation-gesture`; this pass does not implement sex/race
switching or claim to reproduce that authored gesture.

- Moved actual/travel/veteran outfit selection below the figure, alongside
  compact view/action controls. Existing painted equipment and ability artwork
  identify the controls. Hover/focus hints appear after a deliberate dwell,
  dismiss on press/suspension and follow mouse movement after keyboard use.
- Arrow-button rotation now eases toward its requested angle with the shortest
  angular difference. Direct drag remains responsive; cancellation/reset does
  not replay a queued turn. Preview choices still grant no gear or progression.
- Moved C/I/N/M/J/Esc shortcuts and the unspent-talent badge to the lower-right
  edge. The freed space above the spell rows is available to world input;
  shortcuts still invoke actual panels and expose localized hover labels.
- Priest projectile trails now have a pale core and a longer fading amber
  body. Absolution has paired braided paths, Guiding light a narrow long wake,
  and Sanctuary strike six open vanes. These follow accepted flight time and
  destination; no beam reaches a victim before the real projectile arrives.
  Existing preparation, contact, healing and sound events remain authoritative.
- A duplicate translation introduced during the pass was removed. The
  localization test now reports duplicate keys and catches exceptions instead
  of falling into an interactive CRT abort during unattended Debug testing.

No source/export models, frozen facial data, audio bank, gameplay balance or
save format are changed by this pass. Concurrent authored-motion work is
separate. Original recordings are untouched; this is sampled visual analysis,
not a frame-by-frame/audio review or a claim of complete reference parity.
Validation and actual native captures are recorded in `progress.md`.

Validation completed in both configurations: localization, character selection,
class effects, VFX materials, interface and combat HUD suites pass. After the
utility move, two old interface-test click positions were updated; the same-frame
movement-stop and scaled build-shortcut assertions remain in place and pass.
The final UI retake also covers the reset glyph correction. Both game binaries
pass startup/fullscreen-restore/resize/world-UI smoke checks. Evidence in
`build-reference-dock`: `tests-final-{release,debug}.log` and
`tests-ui-{release,debug}.log`, `smoke-{release,debug}.png.log`,
`creation-before.png`, `creation-after.png`, `priest-flight.png`, and
`combat-after.png`. The screenshots are native renderer output, not mockups.

## Wardrobe, audible showcase and combat follow-up

The follow-up revisited the creation overview and full-size frame at 44 seconds
(equipment-tier preview), plus the PvE/raid overviews and full-size raid opening
(outlined combat numbers and bright spell trails). The recordings are visual
references; their soundtracks and assets were not copied or downloaded.

- Three stage wardrobe modes use existing interchangeable equipment: actual
  equipped gear, travel gear and veteran gear. The class weapon and offhand
  rules remain intact. The preview never grants items, changes stats or writes
  a profile; returning to the roster restores the actual equipped view.
- A toggle cycles basic, heavy, tactical and signature demonstrations with a
  rest between actions. Individual art selection stops the cycle. The same
  class preparation/release/recovery sounds used in combat now play in the
  stage through the existing bounded XAudio2 mixer. These are existing samples,
  not a newly recorded sound bank. There are no invented target-impact sounds.
- Audition events are revisioned, consumed once and cancelled on class/page
  changes, wardrobe/reset, native suspension and focus loss. Preparation tails
  use the existing release fade. Music/effects volume and mute still apply.
  Native suspension is tested without relying on an unfocused game update,
  because the real window loop waits while the application is inactive.
- Arcane prism now has a refracting core, three braided light trails and
  separating contact facets, distinct from Arcane severance's orb. It uses the
  accepted projectile clock/destination and the existing effect atlas/budget.
  The spell's gameplay damage, resource, movement and cooldown rules are intact.
- Combat numbers use stable per-event identities, separate outgoing/incoming/
  healing paths, a small arrival emphasis, outline and smooth tail fade. Their
  size and rise speed follow UI scale rather than camera distance. Expiring an
  older entry no longer shifts another number through a vector-index offset.
  Values still come from accepted simulation feedback; no fake critical hits.

Verification covers wardrobe/save isolation, audible phases at 30/60/144 Hz,
bounded/idempotent audio consumption, full showcase sequences, native
suspension, real hit identity and equal-time text motion. Native captures cover
all four focused class wardrobes, live combat text and prism flight/contact.
Final test results and remaining limitations are recorded in `progress.md`.
This does not add new character geometry, genders, races, a new animation
library, raid networking or full reference fidelity.

Follow-up verification completed: LocalizationTests, CharacterSelectionTests,
AudioTests, VfxMaterialsTests, InterfaceTests, CombatHudTests and SimulationTests
pass in both Release and Debug. After the last caption contrast/localization
edit, localization, character selection and interface suites pass again in
both configurations. Both final RPGNative executables pass hidden fullscreen
restoration, device/shader initialization, resize and world/UI capture smoke.
Logs: `build-learning-review/showcase-final-tests-{release,debug}.log`,
`showcase-ui-tests-{release,debug}.log`, and
`showcase-smoke-{release,debug}.png.log`. Native captures reviewed include
`release/character-selection-check/wardrobe-5-2.png`,
`release/vfx-material-review/prism-flight-60.png`,
`release/vfx-material-review/world-contact-5-13.png`, and
`release/combat-hud-review/contact-text-5.png` under that same review directory.

## Evidence inspected

The local Media Foundation review decoded the recordings without modifying or
uploading them. This was a sampled visual review, not an audio analysis or an
inspection of every source frame. Ignored working evidence is in
`build-learning-review/`.

| Recording | Duration | Inspected sampling |
| --- | ---: | --- |
| `character_creating.mp4` | 96.467 s | 4-second overview; full-size creation/appearance views at 16, 32 and 40 s |
| `pve_loc.mp4` | 87.082 s | 6-second overview, including combat, map and career screens |
| `pve_loc2.mp4` | 13.916 s | 3-second overview; 0.15-second samples from 5 to 7 s, including a full-size combat view |
| `pve_raid.mp4` | 111.717 s | 8-second overview of combat, warning zones, raid frames and effects |

The priority creation recording uses an open scenic stage, a class rail on the
right, an identity field on the left during customization, compact appearance
rows and a persistent bottom navigation rail. The hero remains visible while
choosing options. Close inspection and class gestures are part of that scene.
Our previous composition put large framed windows on both sides and used an
older, less complete attack pose without any portrait particles.

The PvE recordings place player/target information near the action, a cast
meter above the bottom spell rows, statuses under unit frames, and distinct
preparation, release and contact effects. Raid warnings must remain legible
through spell volume. Existing native combat HUD anchors already address much
of this arrangement; a full copy of the raid systems is not implemented here.

## Implemented

- Replaced the two large character-menu windows with translucent edge rails,
  a larger central stage and persistent bottom controls. Existing detailed
  interface artwork and unique spell icons remain in use.
- Moved class/roster selection to the right; class description, arts, talents
  and identity entry use the left rail. Appearance choices occupy the right
  rail only on the identity page. No invisible class buttons remain active
  behind the appearance controls.
- Added stage-only left-drag rotation and smoothly bounded mouse-wheel zoom.
  Focus loss, resize, display changes and page transitions cancel gestures.
  Releasing a rotation over Create cannot publish a character.
- Character auditions now use the runtime authored weapon controller,
  including sword body accents, bow draw/release and Priest hand gestures.
  The class spell rate drives the presentation. Actual existing face data and
  models are preserved; no authoring/export regeneration is part of this work.
- Added preparation, release, weapon trails and attached recovery effects to
  the stage, using the same native effect artwork and preparation renderer as
  combat. Portrait effects require explicit opt-in; inventory/unit portraits
  keep their existing behavior. Soft intersections use the stage projection's
  clip planes, and stage seals use the stage floor rather than world terrain.
- The arts view exposes all fourteen implemented Warrior/Mage/Scout active
  spells, including their level requirements. Other classes retain their
  seven implemented arts. Previewing a locked progression spell neither
  learns it nor changes a character's save, cooldowns, reserve or world state.
- Actual combat casts now show the corresponding unique icon and a short
  release/interruption receipt. Release is established by simulation events;
  cancelling follow-through cannot retroactively turn a successful release
  into an interruption. Instant protection is not given a fabricated cast.

Auditions have no target and do not simulate damage, impact on a victim or
combat rewards. They demonstrate the gesture, charge and release; full combat
acceptance still belongs at practice targets and in encounters.

## Verification and remaining work

`CharacterSelectionTests` covers profile publication/conflicts, input isolation,
14/7-art availability, runtime weapon participation, release events, camera
gestures at multiple resolutions, focus/reset cancellation and native captures.
`CombatHudTests` checks release/interruption receipts at 30/60/144 Hz, instant
protection and reset behavior. Shared effect tests cover existing world casts,
depth, lifetime, bounds and repeated rendering.

Initial stage validation: all seven affected suites pass in Release and Debug:
LocalizationTests, CharacterSelectionTests, ClassEffectsTests,
VfxMaterialsTests, CastingEffectsTests, InterfaceTests and CombatHudTests.
The initial Debug interface run exposed stack exhaustion in nested full-game
test fixtures; those fixtures now use owned heap storage. InterfaceTests was
rerun after that correction. This does not raise the executable stack limit.
Both RPGNative configurations also pass the hidden startup/fullscreen-restore,
resize, shader and world/UI capture smoke test. Native creation captures were
visually inspected; these checks do not establish subjective reference parity.
Logs: `build-learning-review/isolated-tests-{release,debug}.log`,
`interface-stack-tests-{release,debug}.log` and `smoke-{release,debug}.png.log`.

This is not full reference parity. Missing reference features include real
race/sex choices with matching authored data, additional appearance geometry,
authored showcase choreography and reference-level
environment composition. The current face freeze remains in force. Combat
balance, group/raid mechanics, additional authored enemy actions and final
crowded-scene effect readability remain separate roadmap work.

## Try it

For this pass, launch `build-learning-review/release/RPGNative.exe` (or the
Debug equivalent). These isolated outputs avoid sharing intermediate build
files with another active development session.
Choose **Create character**, select a class and open
**Battle arts**. Try basic/heavy and advanced spells; drag the figure, use the
wheel and switch to appearance. In Russian the actions are **Создать персонажа**,
**Боевые умения**, **Внешность и имя**. Existing saved characters remain selectable.
Use **Текущее / Походное / Ветеранское** to inspect existing equipment tiers,
and **Демонстрация** for the repeated audible class sequence. This is a preview;
creating or entering the character uses the character's actual equipment.
For combat, cast against a training dummy, then cancel a preparation by moving;
compare the brief **Выпущено** and **Прервано** receipts above the action rows.
