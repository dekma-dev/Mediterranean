# Combat pressure and environmental response — 2026-11-18

## Visible change

Confirmed hits now have a world-space air front in addition to the existing
flash, projectile, physical fragments and residue. Strong impacts also push a
low rolling dust/condensation front over the actual terrain. Steel spreads across
the cut, arrows produce a narrow elongated pressure shape, and magical/holy
contacts expand more broadly. These are shared presentation profiles for existing
attacks; damage, targeting, action timings and class builds are unchanged.

The sampled raid contact from `pve_raid_2.mp4` at approximately 29.5 seconds was
revisited alongside the previous native crowded-combat captures. The reference
distinguishes an intense central contact from broad environmental motion, while
keeping the actor and ground readable. This pass addresses that separation. It
does not claim that the reference uses this particular rendering technique or
that visual parity has been reached.

## Engine work

- `RendererPressure.cpp` and `PressureShader.inc` add a native DX11 refraction
  pass. Camera rays intersect oriented ellipsoids in world space; scene depth
  clips the front against opaque geometry. Shifted samples are also depth checked
  to avoid pulling foreground silhouettes into a rear effect.
- Only the scene color is refracted. The pass runs before warning/effect/HUD
  composition, closes once interface composition begins, and rejects portraits.
  There is no history accumulation, screen tint or camera shake.
- Eight accepted volumes share one pass and one lazily allocated RGBA8 scene
  snapshot. Frustum rejection, a conservative scissor and copying only that region
  limit work. Displacement is bounded to six pixels at 1080p and eight at higher
  resolutions. The allocation is 3.52 MiB at 720p and 7.91 MiB at 1080p; the pass
  is skipped above 8,388,608 pixels. Resize releases the incompatible snapshot.
- A continuous, alpha-blended pressure-ribbon material adds rolling motion without
  repeating a billboard per segment. Dust does not enter the glare target. Terrain
  heights shape its geometry; broken luminous accents distinguish it from a solid
  targeting circle. Up to four fronts share at most 960 existing effect vertices,
  after hostile warnings and main contacts. The global 6,144-vertex limit remains.
- `ImpactPressure.cpp` derives profiles from accepted event snapshots and age.
  Mere releases, secondary victims, evasion, expired events and unrelated classes
  cannot duplicate the main spectacle. No new gameplay or persistent-save state.

## Fixed-step correction

The new cross-refresh rendering regression exposed a particle-tail discrepancy:
float subdivisions sometimes selected tick 17 instead of tick 18 at a requested
0.15 seconds. Positions were nearly equal, but velocity-dependent streaks differed.
The fixed-step selector now absorbs subdivision error below one microsecond. A
dedicated tick-boundary regression and native comparisons at 22/60/144/1000 Hz
cover the correction; existing ballistic/collision tests remain in place.

## Validation

The new native `PressureEffectsTests` renders a checkerboard, foreground wall,
thin opaque silhouette, HUD and portrait. It verifies visible local refraction,
exact repeated-age images, animation, complete occlusion, unchanged foreground/HUD
pixels, offscreen/invalid input, an eight-wave cap, one-pass composition, resize,
idle expiry and four-class clock consistency. The checker test changes 2,320
pixels, while a fully occluded wave changes none.

Six focused Release suites pass: `ParticlePhysicsTests`, `PressureEffectsTests`,
`ImpactParticlesTests`, `CombatEffectsTests`, `ClassEffectsTests`,
`SpellVolumeTests` (46.67 seconds total). The shader compiler embeds `PSPressure`
alongside the existing shaders and reports no HLSL errors or warnings.

The same six suites also pass in Visual Studio Debug (98.68 seconds of summed
test execution). Both the Release and Debug `RPGNative` targets build. Native
DirectX validation reports no resource/binding warnings in the pressure suite or
the completed crowded-combat capture.

The 28-second four-class 720p60 capture completed. Its event CSV is identical to
the previous physical-impact capture: release and contact frames have not moved.
Measured maxima for these scenes, not theoretical saturation limits:

| Scene | Effect vertices | Draw calls | Pressure waves/passes | Scene copy bytes |
| --- | ---: | ---: | ---: | ---: |
| Warrior | 2,238 | 244 | 1 / 1 | 3,686,400 |
| Mage frost | 3,834 | 244 | 1 / 1 | 3,686,400 |
| Scout arrows | 2,880 | 236 | 1 / 1 | 3,686,400 |
| Priest sanctuary | 3,240 | 232 | 1 / 1 | 3,686,400 |

All four scenes remain within the existing four-light and 24-volume bounds.
Compared the frost movie at 8.483 seconds against the previous capture: the new
low condensation front expands outside the central impact while the orange
hostile warning boundaries remain readable. Also inspected Warrior at 2.883
seconds and Priest at 22.767 seconds; the effect is brief and intentionally
subdued for steel, broader and brighter around the holy contact. Refraction is
subtle at the normal camera distance; this is environmental impact motion, not
a replacement of the main spell artwork.

Local evidence (ignored by Git):

- Previous pass: `build-impact-physics/crowded/`.
- Native renderer regressions: `build-ninja-release/pressure-review/`.
- Current four-class encounter: `build-pressure-review/accepted/`, containing
  `combat-motion.mp4`, contact/aftermath stills, event CSV and budget CSV.
- Matched frost comparison: `build-pressure-review/sample-before/` and
  `build-pressure-review/sample-new/`; additional inspected frames are in
  `build-pressure-review/warrior-phase/` and `build-pressure-review/priest-phase/`.

The motion-review tool also supports a pressure-phase still at 0.2 seconds after
contact and records pressure waves, passes and allocation bytes in its budget CSV.
The still hook was built after the recorded run; the inspected mid-phase frames
above were decoded from the completed movie rather than a second full capture.
The movie is an offline 720p60 capture, not a measurement of interactive FPS.

## Remaining limits

Refraction samples the visible scene; it cannot reveal hidden geometry. It avoids
strong camera-enclosing lenses, and conservative depth rejection may reduce the
effect at silhouettes. Physical fragments retain the preceding pass's solid-box
and heightfield collision limits. Authored models, faces and animation clips are
unchanged. Full-speed human review and reference correspondence remain open.
