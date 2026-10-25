# Character material and action refinement

## Follow-up: weapon paths, armor shells and hair

2026-10-25: revisited the extracted melee reference sheet `build-video-review/pass3-melee-02.jpg` (02:01.000-02:01.483). This is a bounded review of contact/follow-through readability, not a new full-video analysis or animation extraction.

Paladin mace and Demonologist cleaver attacks now have distinct solved right-hand paths and weapon directions instead of using the sword cut unchanged. Mace motion stays compact and raised through the cut; the cleaver travels through a wider, lower diagonal. New offsets follow the existing stroke phase/weight and blend back to the current movement pose. Basic/heavy/technique/signature/setup attacks retain their original contact times and simulation effects.

Original plate shoulder shells have a rolled lower edge and raised crown with near/far cached meshes. Warrior, Paladin and Engineer plate fittings use them; underlying armor without a shoulder item uses the same shell. Existing equipment ownership and attachment LOD still apply. Cloth/spined shoulder designs remain as before.

Hair and eyebrows now use a dedicated periodic strand/roughness layer instead of cloth weave. The additional 128-square layer with mipmaps costs 87,380 bytes and uses the existing triplanar sampling. It is not strand geometry or an anisotropic hair shader. No assets were downloaded.

Regression checks compare solved mace/cleaver paths and directions against the sword, sweep existing action/limb/wrist continuity, and retain mesh/texture budgets. Manual check 244 covers live silhouettes, equipment and movement transitions. These improvements do not complete the authored-hero roadmap milestone.

## Follow-up: anatomy, leather and settling

2026-10-25: the continuous weighted body field now gives calves more rear volume, thighs more forward volume and arms directional muscle fullness, shared by all three mesh detail levels. This changes the existing connected surface rather than adding overlapping muscle objects. Existing joint positions and authoritative collision dimensions remain unchanged.

A dedicated periodic leather layer replaces cloth on boots, gloves, belt/pouches, leather collar/cuffs, weapon grip and hide class fittings. Grain, shallow creases and roughness are packed into the existing material-array format; the added 128-square layer including mip levels costs 87,380 bytes. It uses the same three triplanar samples as other materials, with no external texture download. Cloth and skin remain separately selectable through the existing equipment palette.

Attacks now have small class-specific post-release pitch/roll settling, with stronger motion for heavy actions and compact basic/setup recovery. The added envelope starts and ends at zero velocity. Interrupted actions use a quintic 160 ms fade with zero endpoint velocity and acceleration; this smooths the fade envelope, not the incoming animation velocity itself. Combat clocks, input buffering, damage, projectiles and saves remain unchanged.

This is still procedural anatomy, not the completed R2 authored production hero. Live feel, outfit intersections and material appearance remain manual acceptance items.

Follow-up validation: all 44 Release suites (89.34 s), seven focused Debug suites (71.97 s), final equipment/creation checks after leather-fitting assignments (13.70 s), Visual Studio Debug build and hidden startup/render smoke pass. Existing body topology, LOD silhouette and limb continuity limits remain intact. Inspected idle-body and Engineer follow-through captures. No new benchmark or reference-quality claim.

2026-10-25. Advances the existing R3/R5 presentation slice in the custom DX11 renderer.

## Implemented

- Original fitted helmet bowl with rolled brow and crown ridge; a lower-detail variant uses the existing attachment LOD policy. It replaces the spherical upper cap, retaining equipment ownership, cheek guards and crest.
- Split coat panels gather pleats at the waist and relax into curved hems. The existing 198-vertex, 320-triangle two-sided mesh budget is unchanged.
- Startup-generated cloth texture now has warp/weft and diagonal twill, broad dye variation and roughness variation. Skin has restrained pore relief and varying roughness; metal has mottled tarnish. These remain packed 128-square material layers, with existing mipmaps and no added runtime texture samples. Shared material users also receive these changes.
- Techniques, signatures and setup attacks have eleven class-specific preparation/release/recovery offsets, evaluated through the existing continuous spline. Warrior winds across the body, Paladin braces forward, Scout draws back, Priest lifts, Druid gathers and sweeps, Mage projects, Necromancer draws inward, Psionic focuses ahead, Bard conducts, Engineer recoils and Demonologist coils low. Smaller setup and technique amplitudes distinguish them from signatures.
- Hit clocks, cooldowns, movement simulation, damage and save formats are unchanged. All action offsets pass through the existing limb/hand solve.

## References and scope

Revisited the local reference contact sheet `build-video-review/pass3-withdraw-00.jpg` (02:30.500-02:30.983), not a complete new frame-by-frame video review. It informs preparation/release/follow-through readability, not a claim of matching reference animation quality.

The official [Allods class guide](https://allods.ru/about.php?show=classguide) distinguishes the Warrior's accumulating melee advantage from the Mage's elemental ranged identity. Here that distinction informs restrained bracing versus projection; the particular poses are original choices, not extracted animations.

These are procedural model/material improvements, not a completed authored skinned hero or painted character texture set. The roadmap's imported production hero, skin-weighted clothing, larger authored animation library and subjective live acceptance remain open. No external assets were downloaded.

## Acceptance

Action tests sweep all classes/clips through movement layers and shield states, fixed limb lengths, hand/weapon continuity, release counts and cancellation. Added velocity-continuity checks at preparation/release/recovery knots. Geometry tests cover helmet/coat bounds, both helmet LODs, normal/winding correctness and existing mesh/texture budgets. Rendered class captures and creation previews support visual inspection. Manual checks 240-241 cover appearance and live feel.

Completed validation: 44/44 Release suites (88.10 s), 8/8 focused Debug suites (72.41 s), final Release action test including 88 GPU views (3.86 s), Visual Studio Debug build and hidden startup/render smoke. Inspected creation, Psionic signature, inventory and equipped Warrior captures. Existing 1 ms hand/weapon displacement thresholds remain unchanged; the final maximum measured finger displacement was 0.0118062 m. The new knot test checks spatial channels, excluding deliberately saturated aim/openness. No new performance benchmark was run. Live subjective acceptance remains manual.
