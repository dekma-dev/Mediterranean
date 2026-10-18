# Visual target and current gap

Updated: 2026-10-19

The user's Allods Online screenshot is the primary interface reference. Compare character and bag windows with Escape closed: the original project screenshot had an additional menu and global dimming, whereas the reference did not. The underlying art gap remains real after removing that difference.

| Area | Earlier project presentation | Implemented in this pass | Remaining target |
| --- | --- | --- | --- |
| Escape menu | 416 x 560 logical-pixel panel dominating the center | 328 x 438 panel, smaller controls, grouped volume rows, lighter dimming; about 38% less panel area | Cohesive original illustrated frame artwork |
| Panel material | Broad olive gradients, repeated diamond corners | Teal recesses, restrained tool marks, warm highlighted trim, rivet corners and less repeated rail decoration | Detailed carved/painted materials with distinct depth |
| Empty slots | Large repeated pendant/key-like symbol | Recessed slots with smaller mirrored engraving | Subtle original ornament that stays behind occupied items |
| Item icons | Flat weapon, potion, cloth and helmet silhouettes | Blade edges/grip details, potion highlights, cloth seams/folds and helmet highlights | Authored painted icons with stronger unique silhouettes, material variation and readable rarity |
| Text | Muted olive text against olive panels | Brighter ivory text and clearer secondary labels | Further hierarchy and density refinement across character and bag panels |
| Character | Segmented procedural model | Unchanged by this interface pass | Natural anatomy, hands, clothing shapes, detailed equipment and animation |
| World | Sparse village and flat terrain | Unchanged by this interface pass | Richer silhouettes, vegetation composition, terrain elevation and architectural detail |

This is an interface refinement, not visual parity with Allods. Adding more decorative strokes cannot replace the missing authored art and character work. No downloaded assets or copies of Allods artwork are used.

Local before/after captures are `build-ninja-release/interface-before.png`, `interface-after.png`, `menu-before.png` and `menu-after.png`. These generated images are not source assets. The interface still supports the real 24-cell inventory and six equipment slots; it does not invent unused gameplay systems to imitate the reference's density.

Validation: scaled pointer tests cover Return, display mode, volume buttons and stale-region blocking after the menu layout changes. Character/bag item interactions retain their hitboxes. Manual check 112 covers legibility and live acceptance.
