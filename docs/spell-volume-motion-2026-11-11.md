# Spell volume and motion review — 2026-11-11

## Reference and visible problem

Revisited the supplied recordings in `D:\assets\learning_material`: decoded 90 samples at 1/30-second intervals from `pve_loc2.mp4` (5.5–8.5 s), 60 samples at 50 ms from `pve_raid.mp4` (22–25 s), and 40 creation samples at 250 ms from `character_creating.mp4` (32–42 s). Inspected full-size PvE/raid frames and the creation view. The latter range shows identity editing rather than a spell demonstration, so it is not evidence for projectile design. This is a targeted sample review, not an exhaustive viewing of the recordings.

The combat footage separates a directed release, a short bright contact and a longer residual treatment. Raid effects keep narrow connecting paths and open spaces around actors. The previous native fire projectile moved overlapping copies of one flame image; changing the position and scale did not animate its internal material. Segmented streak end masks also made some wakes read as beads. These were the specific defects addressed here. This pass does not claim exact visual parity with the complete reference game.

## Implementation

- Added an oriented, depth-clipped local participating-medium renderer. A front proxy bounds a ray integral through a three-dimensional ellipsoid. Animated spatial noise supplies internal motion; opaque depth truncates the integration within a volume. Twelve samples serve small projected volumes and twenty serve larger ones. This is a bounded procedural effect, not a fluid simulation or an imported fire image.
- Fire has rolling density, a hot interior, a cooling boundary and detached embers/smoke. The volume participates in sorted alpha blending; adding the whole body directly had bleached it against daylight. Existing luminous ribbons and sparks remain additive.
- Replaced the Mage fire/frost/arcane/thermal/meteor flight body and Priest flight body. Closed, rotating hexagonal ice faces supplement the cold medium. Arcane density has moving filaments; holy flight has an elongated light interior and swept open vanes. The leading geometry stays at or behind the accepted trace endpoint.
- Replaced isolated streak segments with joined, tapering ribbons. World-distance trail lengths are independent of target distance. Fire wake particles use fixed 60 Hz birth identities on the actual accepted path and continue their own short decay after contact.
- Fire preparation, release and contact share the new material. Priest preparation/contact and arcane contacts also use the medium. Existing character sockets, cast/release/damage clocks, local lighting and grounded aftermath remain authoritative. No model, face, authored animation, spell balance, save schema or audio asset is changed by this pass.
- The renderer has 24 medium records (2,304 bytes), an aggregate conservative proxy coverage ceiling of 1.25 viewports, and the existing 6,144 effect-vertex / 96 alpha-quad limits. Media use the existing alpha batch and depth resource, with no new textures, extra light budget or per-volume draw call. Oversized, malformed, offscreen and lens-covering volumes are rejected. Saturated crowds can omit new cosmetic volumes; this is not unlimited raid scaling.

## Native moving evidence

`SpellMotionReview` records **18 seconds at 1280×720 / 60 fps** directly from `Game`: basic fire, heavy fire, frost, arcane, thermal and holy. Every scene sends an actual command, waits for actual release and contact events, and records the normal renderer/HUD. The target is kept stationary in this diagnostic. There are no synthetic damage events or profile writes. Video is silent and is not an audio review.

Build the `SpellMotionReview` target and run it with an output directory. It uses the installed Windows Media Foundation encoder, following the [Microsoft sink-writer interface](https://learn.microsoft.com/en-us/windows/win32/medfound/tutorial--using-the-sink-writer-to-encode-video); no external encoder or download is required. `events.csv` records the release/contact frames. Only one intermediate PNG is reused, alongside selected flight/contact bookmarks and the H.264 movie.

Local comparison outputs:

- `build-vfx-motion/movie-before/combat-motion.mp4`: same review driver linked to the earlier public Release client and its matching saved renderer header.
- `build-vfx-motion/movie-after/combat-motion.mp4`: native client with the new media and flight implementation.
- `build-vfx-motion/before/` and `after/`: the existing release suite's matched cameras/ages.
- `build-learning-review/release/spell-volume-check/` and the equivalent Debug directory: final isolated materials, view changes, opaque-wall intersections, equal-age checks, malformed-input checks and GPU probe reports.
- `build-learning-review/{release,debug}/vfx-material-review/` and `combat-release-review/`: final shared material/contact and native release captures.

The encoded movie was decoded again to inspect orientation and consecutive contact frames. Do not compare encoder wall time or readback-heavy movie capture speed to gameplay FPS. Fixed-scene GPU checks exclude warm-up and forced-readback frames.

## Acceptance

`SpellVolumeTests` covers material movement from three views, interior wall clipping, bounded submission, lens/invalid-input rejection, portrait isolation, resize, identical event age at 22/30/60/144/1000 Hz, complete expiration and a six-flight cost probe with room for other warning geometry. Existing release/material/casting/combat/class/menu suites cover accepted gameplay clocks and shared creation rendering.

Integration caught premature crowd truncation: a sphere-based proxy overestimated the screen area of narrow shafts. Projecting the eight corners of the enclosing oriented box now gives a tighter conservative rectangle. Six major simultaneous contacts retain the sum of their isolated geometry at all five checked phases, within the original budgets and warning reserve.

The defensive rotation check also failed against the earlier client library, before the new renderer. Subtracting a changing bright sky does not undo additive saturation and alpha blending. Its fixture now supplies a neutral opaque receiver rotated with the effect/camera; the original rotation tolerance, visibility and class-distinction assertions remain unchanged. This is a test-fixture correction, not a defensive-effect redesign.

Final native validation: **7/7 passed in Release (120.86 s), 7/7 in Debug (219.87 s)**: CharacterSelection, CombatEffects, ClassEffects, VfxMaterials, SpellVolume, CombatRelease and CastingEffects. Logs are `build-vfx-motion/verified-tests-{release,debug}.log`. The six-flight probe retained 2,808 effect vertices and all 24 medium records, leaving the existing warning reserve. Its GPU reports are small-stage queued-frame measurements, not a controlled incremental shader/FPS benchmark; clock/queue variance prevents a defensible performance-gain claim. Dense moving encounters at normal display resolution still require acceptance.

The final movie was re-encoded after the proxy correction and decoded again at 30 samples/s through release, flight and contact. Both `events.csv` files remain identical. Sources checked for this validation are hashed in `build-vfx-motion/validation-sources.json`.

Ninja Debug/Release game builds completed. The full Visual Studio solution invocation stopped progressing in the service's build host and was cancelled. Its Debug/Release projects were then built successfully in dependency order with project-reference traversal disabled for those invocations, including the shader compiler and freshly generated bytecode before the client and game. No project setting or system configuration was changed to suppress checks permanently. The ordinary Visual Studio Debug executable is `build-local/Debug/RPGNative.exe`.

Game-host startup/render smoke checks: **4/4 passed with exit code 0** for Ninja Debug/Release and Visual Studio Debug/Release, using `tests/fixtures/veteran-preview.save`. Each checked device/shader creation, hidden fullscreen restoration, resize, world/UI rendering and capture. See `build-vfx-motion/smoke-results.json` and `smoke-*.png.log`. The Visual Studio Debug capture was inspected. Diagnostic runs did not write player profiles. Existing unrelated shadowed-local warnings remain in the client build; no warning-free build is claimed.

Remaining visual work includes region-specific ambience, richer sustained-area choreography, moving-target combat at raid density, and the broader actor/action staging differences visible in the references. A stronger projectile system alone does not make the whole game identical to the recordings.
