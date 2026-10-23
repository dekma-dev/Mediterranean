# Class clothing fittings

The 2026-10-22 pass adds original procedural fittings to the existing continuous character body. The shared renderer supplies the same equipment appearance to gameplay, creation, the roster and inventory portraits.

| Class | Chest treatment | Other details |
| --- | --- | --- |
| Warrior | Segmented leather harness and clasp | Broad metal shoulder caps |
| Paladin | Short tabard panel and brass insignia | Broad metal shoulder caps |
| Scout | Diagonal bandolier with pale bindings | Belt tools |
| Priest | Paired pale stoles with brass bands | Bound belt book |
| Druid | Leaf-like neckline tabs | Raised shoulder spines |
| Mage | Raised collar, vertical edging and clasp | Bound belt book |
| Necromancer | Bone-colored rib ornaments | Shoulder spines and belt book |
| Psionic | Graduated central fittings and side strips | Cloth shoulder caps |
| Bard | Asymmetric sash and brass buttons | Graduated belt pipes |
| Engineer | Leather apron panel, braces and rivets | Metal shoulder caps and tools |
| Demonologist | Layered dark chest scales | Raised shoulder spines |

Chest fittings require a worn chest item; shoulder fittings require shoulders; belt accessories require the belt. Removing one slot does not remove another slot's details. Cloth accents use the selected dye where applicable, while metal, leather and bone retain their material colors. Existing armor stats, item IDs, saves and weapon behavior are unchanged.

Fittings follow the solved torso transform and participate in existing body attachment LOD/culling. Each class uses fewer than 32 fittings from a fixed 64-entry stack array, with no new textures, downloaded assets, persistent allocation or GPU mesh assets. This adds instance submissions; a gameplay frame-time improvement is not claimed.

These are short, rigid garment fittings, not simulated robes, full tailored clothing, new anatomy or production character art. Long hems, cloth deformation, more distinctive body proportions and class-specific cloak/helmet artwork remain future work.

Reference direction: [WoW's priest class page](https://worldofwarcraft.blizzard.com/en-gb/game/classes/priest) identifies cloth armor; [the official Allods class guide](https://allods.ru/classes.php) describes a different range of class equipment and combat identities. Our designs use those broad distinctions without copying assets or assuming their armor rules are identical.

Validation: ClassEquipmentTests checks eleven distinct geometric signatures, fitting bounds/budget, slot removal and dye topology. Its 101 native views cover idle/action, dye, removal, air/dash, HUD, inventory and roster. Visual acceptance during interactive movement remains manual check 207.
