# Iron court siege and southern composition

Implementation: 2026-10-31. This develops the existing 240 x 256 m province and its final assignment. The campaign still contains eighteen assignments and the level cap remains ten.

## A staged assault

Captain Vesna's **The iron court** assignment now has three ward seals at (86,-96), (63,-116) and (94,-122). The first two have two assigned sentries each; the last has one. Sentries must be defeated: pulling them away does not unlock a seal. Press E beside an unlocked plate and remain still for 2.5 seconds. Movement, jumping, attacks, damage, Escape or input suspension cancels the channel. Completed seals persist; unfinished channels do not.

The **Iron Regent** takes no damage and grants no on-hit resources or control effects until all three seals are broken. The ground plates, thin ward connections, target-frame message, atlas markers and channel meter expose this rule. His existing three-attack rotation remains active. Defeat him and report to Vesna for the existing one-time reward. The ritual's final commander is separately named **Mist Warden**; neither boss receives a new mesh in this pass.

Save version 21 appends `COURT <three-bit seal mask>` after `SOUTHERN`. Versions 1–20 migrate. Previously cleared sentries unlock their corresponding seals; an already defeated captain stays defeated with all seals broken, even if old guards remain. Invalid new progress is rejected without modifying live state. Older positions covered by new foundations/trunks are relocated only to a nearby free edge; current-version positions inside geometry remain invalid.

## Three different combat responses

| Class | Successful control interruption | Prepared level-ten finisher |
| --- | --- | --- |
| Warrior | Pommel strike raises a 40-point guard, in addition to its normal Rage/stun | Iron cyclone consumes exposure for an additional 35% heavy damage and raises its ward to 40 |
| Mage | Arcane severance grants a second Ember | Firestorm consumes a victim's slow/root for an additional 50% heavy damage |
| Scout | Disarming arrow grants one Focus and exposes the victim for six seconds | Horizon shot consumes exposure for an additional 50% heavy damage, alongside its existing distance bonus |

Failed interrupts and uninterruptible attacks grant no reactive reward. Range, visibility and the original committed target are still checked at contact. Secondary targets qualify separately. Exposure's existing 25% damage multiplier applies to the consuming hit. Reserve is still paid on commitment, without refunds for movement cancellation.

Winter lance now slows for six seconds and Frost bloom roots for four: these windows accommodate the actual spell recovery and subsequent Firestorm preparation. Tests execute complete rend/cyclone, winter/firestorm and pierce/horizon sequences at 30 and 144 Hz, rather than validating only artificially applied statuses. The resource panel displays a prepared-combo hint; the spellbook describes the new rules in Russian and English. Existing bindings and cooldowns remain compatible.

## Location work

- Nine additional authored building/tower placements and up to forty additional grove trees frame the caravan road, mist ridge and court. All route segments and mission sites remain reachable. Authored southern towers replace the former procedural ruin presentation there.
- New footpaths, worn service yards, broken foundation stones, sheltered lantern fittings and stone foundation skirts provide local detail. These reuse shared meshes/materials and existing authored assets; this pass does not regenerate Blender sources or download art.
- Regional mist uses three bounded samples in the shared world/effect transmission function. Ordinary ruins no longer all carry floating cyan crystals. This is local haze and emissive lantern art, not volumetric local lighting or a new weather system.
- The heightfield now limits abrupt grades and reconciles nearby flat foundations with compatible slope envelopes. This removes near-vertical banks between conflicting terrace heights without adding terrain vertices. The village floor remains zero; rendering, camera, navigation height and feet sample the same grid.
- If the camera focus already lies inside a tree crown, the boom can leave that crown while retaining trunk/wall collision. This fixes a native capture that was filled by the player's torso after immediate camera retraction.

Reference review used the local `loc1.jpg` environment image (layered hills, wooded roads and fortified landmarks), the existing video-analysis documents, and the role distinctions described by the [official Allods class guide](https://allods.ru/about.php?show=classguide). The guide was available through indexed search; direct retrieval returned HTTP 403. These are design references, not claims of reproducing their art or balance. Sparse cover, architectural variation, authored material detail and slope-aware gait still need further work.

## Verification and review

`SouthernTests` covers guarded seals, protected hits, cancellation, migration (including an already defeated legacy captain), exact-once rewards, class counterplay, real combo windows and native views. Existing navigation, simulation, class combat, interface, shadow, geometry and progression suites provide integration coverage. Final build/test outcomes are recorded in `progress.md`.

The native output is `build-frontier-expansion/native/RPGNative.exe`. Captures and the diagnostic save are under `build-frontier-expansion/native/southern-check`: `court-ward.png`, `court-channel.png`, `region-13.png` through `region-15.png`, and `six-actors.png`.

`six-actors.txt` records a repeatable hidden 1280 x 720 workload with six live enemy AI/controllers, real movement, attacks, warnings and damage. It uses a diagnostic enemy arrangement, 90 warmup frames and 300 measured frames; snapshot setup is outside measurement. CPU wall time includes simulation, preparation and submission. It is not GPU time, interactive FPS or proof of full encounter balance. Complete manual combat acceptance for all three classes before considering the difficulty milestone finished.
