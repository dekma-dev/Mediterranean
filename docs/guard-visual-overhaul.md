# Dawnwatch guard visual slice

## Visible result — 2026-10-26

The Warrior now wears a coherent fitted harness instead of the earlier padded-shirt silhouette. This is original geometry produced inside the existing C++/DirectX11 renderer, with no downloaded assets or new dependencies.

- A shaped front/back cuirass has a fitted waist, raised central keel, rolled neckline and branching chased ribs.
- Overlapping abdominal plates, thigh guards, knee cups, greaves and bracers share one curved plate mesh. Thigh/forearm/shin pieces follow solved limb frames.
- Dark arming sleeves and split rust-red tabard panels separate cloth from steel; dye choices affect the tabard. Equipment ownership still removes the relevant parts.
- A rust-red guard mantle has a woven watch crest, stitched borders and a shaped hem, making the redesign visible from the normal third-person camera. It uses the existing cloth motion and shadow deformation.
- The sword has swept guard wings, a wrapped grip, a shaped pommel and a brass blade collar.
- Warrior head width/depth are reduced by 8%, height by 3%, around the neck. Skin, facial features, hair and helmets use the same transform.
- The shared hair shell now follows the skull after extending toward the nape. This fixes visible scalp islands found in the rear-view capture.

The underlying continuous body, facial topology and animation library remain procedural. This completes an outfit/silhouette slice, not a production-quality replacement hero or all-class art overhaul.

## Evidence

Controlled captures use the same 960x720 renderer, camera and lighting before and after. Views: 0 idle front, 1 back, 2 heavy preparation, 3 helmet/reward armor/shield. Original captures are preserved in `build-ninja-release/guard-before/guard-study-0.png` through `guard-study-3.png`; current ones are in `build-ninja-release/action-clip-check/`. Captures are local ignored build artifacts, not source assets.

Visual review caught and corrected tabard/waist-plate intersections and scalp/hair intersections. Both front and back were inspected after correction. The lighting intentionally remains unchanged between the comparison images. Live movement/equipment acceptance is manual check 274.

## Cost and scope

Five cached mesh variants (cuirass and curved plate, each high/low detail, plus the guard mantle) use 302,512 bytes and 8,056 triangles in total across the complete set. This is shared startup geometry, not a per-character allocation or per-frame triangle count. The existing prop and weapon budgets remain unchanged; the new armor has a separate 320 KiB/8,500-triangle ceiling. Extra visible attachments add draw submissions; no FPS improvement is claimed.

Geometry, skinning, attachments, face, shadow, class-equipment and action checks cover the change. Both Release and Visual Studio Debug game binaries are rebuilt so either launch path can show it. Use the Warrior to inspect this slice; the other classes do not receive its armor.
