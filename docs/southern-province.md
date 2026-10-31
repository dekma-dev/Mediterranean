# Southern province and level-ten disciplines

Implementation: 2026-10-30. Native C++ / DirectX 11; no downloaded art or Blender source regeneration. This builds on the 144 x 160 m highlands pass and preserves the separate character-art workstream.

## World and campaign

- Playable bounds are now 240 x 256 m: 2.67 times the preceding area. The original village remains flat; the outer province has broad hills, terraced service buildings, branch roads, groves and ruins. The heightfield and cliff consume 1,077,576 bytes / 32,728 triangles. Navigation scratch arrays use the heap rather than consuming a province-sized portion of the Windows thread stack.
- Three additional friendly residents: caravaner Tessa (-39, -85), warden Ilar (-74, -70), captain Vesna (73, -91). The southern caravan road, mistward ridge and iron court have atlas labels and objective markers. Terrain shelter is baked into the existing route texture's blue channel and affects ambient light; it adds no texture allocation or heightfield sampling during a frame.
- **Wheels in the mud:** collect three supply caches with E; fighting the six raiders is optional. Inspecting a cache repeatedly cannot grant extra rewards.
- **Three answers in the mist:** kindle the ridge beacon (-86, -91) with E. Defeat three pairs of attackers. After each pair, hold the visible five-metre circle for four seconds. Leaving, jumping, attacking or taking damage resets charge. The final pair includes a boss. Wave and partial charge survive save/load; unfinished attacks do not.
- **The iron court:** defeat six defenders including the captain, then report to Vesna. There are eighteen assignments total, with the level cap still ten and 250 XP per new assignment. The route from the northern gate now leads to Tessa instead of incorrectly announcing the campaign's end.

Encounters remain chapter-based. The province does not simulate every old camp simultaneously. Automated campaign completion uses fixture-cleared waves to isolate progression/save accounting; it is not evidence that all three classes can comfortably defeat the real encounter.

## Class kits

The three focused classes have eleven spell cards, excluding potions. New defaults use Ctrl+6 and Ctrl+7; existing saved bindings remain unchanged. Drag the cards from N onto either action row to change an existing profile.

| Class | Level 6 control | Level 10 full-reserve attack |
| --- | --- | --- |
| Warrior | Pommel strike: 50% heavy damage, 1.4 s stun, vulnerable-cast interruption, one Rage on contact | Iron cyclone: 150% heavy damage in a 3.8 m circle, including rear targets; 25-point ward on contact |
| Mage | Arcane severance: mobile 60% heavy bolt, vulnerable-cast interruption, one Ember on contact | Firestorm: stationary 2.5 s contact, 175% heavy damage in a 4.5 m target-centred area |
| Scout | Disarming arrow: mobile 55% heavy arrow, vulnerable-cast interruption, four-second slow | Horizon shot: stationary 2 s contact, 175% heavy damage plus 80% heavy damage beyond six metres at contact |

Range, line of sight and the original selected target are validated at contact. Secondary victims also require caster range/visibility. Boss interruption immunity remains effective. Cancelling a committed finisher does not refund reserve/cooldown. Individual control and finishing VFX use bounded fixed geometry; there are still reused icon foundations and existing animation clips, not six new authored animation assets.

The distinctions follow the melee front-line / ranged control / deliberate casting roles described in [Blizzard's class introduction](https://news.blizzard.com/en-us/article/23317716/taking-your-first-steps-in-world-of-warcraft-classic) and the resource/role differentiation discussed in [the official Allods class-development article](https://allods.ru/news.php?article=4435). These historical references guide design, not current balance numbers or a claim of matching either game. Local environment references and remaining visual differences are recorded in `environment-art-direction.md`.

## UI, persistence and startup

The N rail fits eleven cards; the journal fits eighteen selectable rows with a separate status column. The HUD shows ritual wave, holding instructions and a charge meter. All new player prose has Russian catalog entries.

Save version 20 appends `SOUTHERN <stage> <chargeSeconds> <cooldown10> <cooldown11>` after version 19's `EXPEDITION` record. Versions 1–19 remain readable; malformed wave populations, invalid charge, out-of-range cooldowns and trailing data are rejected through a candidate load, preserving the live state. Hotbar format 7 permits the new abilities without replacing existing layouts.

Native debugger inspection found an actual stack overflow in `compiled_mesh::PlayerHead`, reached from renderer initialization. MeshAssetCompiler now emits immutable static vertex/index arrays and assigns them directly to owning vectors. Large OBJ initializer-list temporaries no longer sit on the caller's stack. The OBJ/Blender source and mesh contents are preserved.

## Verification and review

Independent output directories: `build-frontier-expansion/native` (Release) and `build-frontier-expansion/debug`. `SouthernTests` covers routes/NPC and camp clearance, all three quest modes, three-wave lifecycle at 30/144 Hz, migration, interrupted charge, rewards, class timing/resource rules, area exclusions, movement cancellation, cooldown persistence and native captures. Existing interface tests cover all journal row hit targets and translated layouts.

Native images and an active court save are generated in `native/southern-check`. Main executable: `native/RPGNative.exe`. Reference-frame art quality, real encounter balance, terrain traversal feel, steep-slope animation and long-session performance remain manual acceptance work. Native smoke and final suite outcomes are recorded in `progress.md`.
