# Dawnwatch environment direction

Reference review: 2026-10-26. Local references in `C:/Users/Хозяин/Videos/NVIDIA/Allods  Online`: loc1.jpg, loc2.avif, map1–3.jpg and character/creature plates 1–5. Also inspected the existing opening contact sheet from allods_fight_interface.mp4; this was not a new complete frame-by-frame video review.

## Direction

The strongest transferable qualities are exposed earth between irregular vegetation, weathered stone and timber, wooded silhouettes, readable routes and separation between foreground and distant landscape. The character plates use restrained fabric/leather colours with selective metal accents. Their authored shapes and composition matter more than simply increasing polygon counts or making everything dark.

## Implemented pass

- World-space heath/earth patches replace the uniformly green ground. Worn routes and paving retain the authoritative route mask, with subdued colours and quieter surface relief.
- Foliage receives broad colour variation and a less saturated, rougher finish. Roof weathering uses the roof material, not the wood material.
- Nine rocky wooded outcrops replace seven sparse peripheral platforms. They reuse the user's Highland pine and Blighted elder exports through the existing static scenery cache and LOD path.
- A deeper lower sky hemisphere provides a cooler backdrop. Reduced near-ground haze preserves foreground contrast. Sun direction, exposure and cloud coverage retain their established values after the first experiment obscured details and failed shadow contrast checks.
- No texture downloads, new texture allocations, Blender source regeneration, collision changes or save migrations.

## Remaining work

This remains a flat playable island with stylized authored buildings. It is not yet equivalent to the reference's rolling, densely composed landscape. Next: coordinated terrain height/collision/navigation support, authored terrain and architecture material refinement, more deliberate vegetation grouping, and camera-level composition around each quest site. Preserve traversal and camera clearance while changing actual landforms. Do not hide missing detail with excessive fog.

## Reproduction

Build with scripts/build-native.ps1, then use RPGNative.exe --smoke-test <absolute-output.png> tests/fixtures/veteran-preview.save (use an absolute fixture path). Compare the environment-village, canopy, ruin, dock and overview captures. Current local evidence is under build-ninja-release/environment-before and environment-after. These build outputs are not source assets.
## Rounded skyline follow-up (2026-10-26)

Revisited loc1.jpg and loc2.avif. Replaced the eleven distant placements' geometry with a shared rounded ridge mesh (17,744 bytes / 512 triangles), replacing the previous nine rectangular terrain/cliff platforms. A shared profile grounds 121 authored oak/pine/dead-tree instances on the slopes; trees retain existing LODs and static caching. Slope-sensitive rock/earth shading separates steep faces from heath. Low grass tufts now cluster around existing cover while checking route weights and stand clearance. Plaster is less saturated and the market canopy uses faded canvas colours. The first steep skyline was lowered after native visual review.

The legacy geometry budget remains unchanged; the new ridge has a separately enforced 20 KiB / 512-triangle allowance. No new textures, source Blender changes or playable terrain/collision changes. Native comparisons: build-ninja-release/landscape-before and landscape-after. This improves silhouettes and surface cohesion, but does not supply walkable hills or production foliage topology.
