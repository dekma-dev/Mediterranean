# Guardian armor adoption

## Shoulder and waist follow-up

Both authored shoulder assemblies and the four overlapping waist plates are now exported independently. Each shoulder has 900/220 triangles and the fauld has 899/220; compiled geometry totals 146,588 bytes for all six new meshes. These have a separate 160 KiB/3400-triangle allowance; existing mesh budgets remain unchanged. The source is still read-only.

Shoulders belong to the shoulder slot, with two authored fittings instead of eight primitive fittings. The fauld remains part of the chest assembly, matching its previous ownership. A reward chest no longer restores fallback shoulder visuals after the Warrior removes shoulders. Other classes retain their existing equipment presentation. Each assembly currently follows the torso; individual plates are not physically simulated or driven by the collection03 armature.

The exporter handles collapsed rivets with missing averaged normals and preserves hard normals on opposing thin edges. The native importer continues rejecting invalid geometry. Character equipment captures verify the new outfit and stripped variant; GeometryTests validates both LOD reduction and the dedicated budget.

The game now uses the shaped breastplate, backplate and rolled edges from the user's newer Dawnwarden collection for the Warrior chest attachment. It replaces the procedural GuardCuirass mesh in world, character creation and inventory views. The original chest-equipped condition, class identity, torso animation, reward tint and attachment LOD routing remain authoritative.

Source: `.blender-local/collection-02/collection-03/Mediterranean_Collection_03_Animated.blend`. The source SHA-256 and selected object names are recorded in `assets/characters/guardian-cuirass-source.json`. The export reads the source without saving it and disables armature evaluation only in memory to export bind-space equipment. It does not replace either collection01 source or the newer collection03.

Reproduce with installed Blender in background and `--python scripts/export-guardian-armor.py`. The two tracked OBJ exports are sufficient for native builds; Blender is not required on player machines. CMake validates and compiles them through MeshAssetCompiler, enforcing its coordinate, index, normal, triangle and file-size limits. Thin rolled edges that acquire opposing averaged normals during reduction use hard corner normals instead. GeometryTests retains the existing 320 KiB/8500-triangle shared guard-equipment budget and requires reduced distant topology.

This is a rigid torso attachment, not adoption of the complete new body. Collection03 includes 26-joint humanoid rigs and animation-only GLB files; the existing body-preview adapter supports ten joints and those GLBs do not contain the skinned meshes. Full adoption still needs a bounded multi-part mesh export, an expanded GPU palette path, correct authored weights/clip sampling, and independently removable/customizable gear. Do not replace all playable classes with the single dressed Guardian mesh or discard those requirements.

Review: `build-ninja-release/class-equipment-check/0-roster.png`, `0-clothing-0.png` through `0-clothing-3.png`, and `action-clip-check/guard-study-0.png` through `guard-study-3.png`. The current head, limbs and other gear are still the existing presentation model.
