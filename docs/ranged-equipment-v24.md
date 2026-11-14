# Ranged equipment and arm animation — v24, 2026-11-14

The archer and mage previously inherited a blade-like palm orientation. Changing
the arm path alone could not correct the bow grip, the opposing staff hand or
the drawing fingers. This revision gives both weapons fitted hand frames and
reauthors their arm chains in Blender. It also rebuilds the two existing bows
and two existing staffs, retaining their equipment identities and native DX11
resource pipeline.

## Editable source and runtime

- `assets/authored/player-motion-v24.blend`: 26 revised Staff/Bow Actions,
  covering carry, guard, walk/run in four directions, jump and both attacks.
  The source still contains 53 Actions and the same 57 rest bones.
- `assets/authored/player.clips`: PCL6, 59 tracks, 56 floats per frame. Ranged
  tracks now carry the complete weapon orientation and retargeted elbow planes.
  The other 33 tracks are byte-identical to the v23 baseline.
- `assets/authored/oldstyle-classes.blend`, `.kit`, `.json`: changes only to
  ScoutBow, ScoutVeteranBow, MageStaff and MageVeteranStaff. The other 29 native
  equipment records and garment weights are unchanged.
- Local authoring, inspection scripts and review sequences remain under the
  ignored `.blender-local/ranged-art-v24-20261114/` directory.

The runtime preserves the authored primary elbow plane when the string or
supporting hand engages. This removes a discontinuity caused by switching to
a different elbow solver at the first nonzero draw value. Hands keep their
anatomical handedness during grasp/release. Fixed-length fingers fit the actual
staff diameter and bow grip ellipse; the bow uses three string fingers with
the nock between index and middle. Finger release precedes limb recovery.

Arm motion and complete weapon frames come from Blender. Native hand skinning
still evaluates the fitted phalange poses analytically; source finger keys are
editable counterparts, not additional PCL6 channels. The existing simulation
attack/contact clocks, damage, movement speed and jump trajectory are unchanged.

## Four weapon models

The bows have continuous shaped staves, horn belly laminations, revised bindings,
denser arrow shafts and leaf-shaped arrowheads. Their strings join the actual
tips and off-centre nock. Native deformation uses each bow's own half-span
(0.57 / 0.64 metres) in both colour and shadow passes.

The staffs have continuous carved ash shafts, ferrules, existing fitted wraps,
faceted crystals and four retaining prongs. The prongs were fitted closer to
the stone after inspection in the game renderer.

| Item | Near triangles | Far triangles |
| --- | ---: | ---: |
| Scout bow | 27,380 | 7,346 |
| Veteran bow | 27,380 | 7,346 |
| Mage staff | 24,355 | 6,556 |
| Veteran staff | 26,307 | 7,076 |

The complete KIT archive is 33,326,748 bytes, within the existing 32 MiB limit.
The focused ranged-weapon test budget is raised from 16k to 28k near triangles
for this detail pass; both tiers additionally require fewer than 8k far
triangles and less than 35% of their near geometry. Procedural weapon budgets
and the complete archive limit are unchanged.
These are targeted silhouette, construction and contact improvements; polygon
count alone does not establish realism.

## Reference and preservation

Revisited `D:/assets/learning_material/3.jpg`, the character-creation recording,
and both PvE location recordings. The creation sequence around 54 seconds
shows the opposing staff hold; the scout shown there carries a crossbow, so
that footage is not evidence for a conventional bow draw. The additional
[World Archery recurve technique guide](https://documents.worldarchery.org/Media/Information_Sheets/003_Guide_to_Recurve_Technique.pdf)
informed the string finger arrangement and release follow-through. Modern
competition accessories were not added to the fantasy equipment.

All native non-arm channels and every track header are preserved. Reopening
both Blender sources confirms identical rest bones and protected source curves.
Forty-two frozen asset hashes remain unchanged, including face/body assets and
all thirteen collection models. Face/scalp shader blocks are also checked.
This pass does not revise faces, leg choreography, the roadmap or game mechanics.

## Review and validation

Open `.blender-local/ranged-art-v24-20261114/review.html` for synchronized native
before/after sequences, hand closeups, equipment views and Scout/Mage gameplay.
The strong-shot comparison uses the available 72 baseline frames; other paired
sequences contain 138 frames at 30 fps. Full new directional sequences are
included separately. This is an iterative art pass, not a claim of reference
parity or photorealism.

Focused continuity checks retain their existing limits, including 30/60/144 Hz
transitions and millisecond sampling. In the loaded-running fixture, wrist
acceleration RMS changed from 5.761 to 3.011 m/s² for the staff and from 6.372 to
2.998 m/s² for the bow, with smaller connected carrying arcs. This measures that
fixture, not perceptual realism. The bow's maximum millisecond hand displacement
changed from 12.76 to 8.89 mm. The fitted string contact error is at most 6.45 mm
in the near-full-draw check; staff support contact error is below 0.001 mm.

Local evidence: `source-preservation.json`, `preservation.json`,
`test-results.json`, `embedded-resources.json`, `runtime-verification.json`,
and the native render logs. The hand tests now transform the skinned vertices
into the actual held weapon frame before checking penetration; they additionally
check both staff hands, the bow handle and the three-finger string arrangement.

Validation runs AuthoredLocomotionTests, HandTests, AnimationTests,
ClassEquipmentTests and OldStyleArtTests in Release and Debug. Release includes
the complete OldStyleArtTests capture series. Its Debug capture series reached
the local runner's 300-second limit while still producing images; the Debug
geometry/garment validation is therefore run with `--tailoring-review`, which
retains all geometry assertions and the fitting captures. The separate Debug
motion, hand and equipment suites cover animation and class transitions.
All ten scoped checks pass. All six existing game outputs embed the exact
current PCL6/KIT/GWT resources and pass fixture-save smoke launches. Final
evidence is collected in `validation-summary.json` beside the local review.
