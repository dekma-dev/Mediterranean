# Player anatomy source

The head exports (`player-head*.obj`), fitted scalp (`player-hair*.obj`) and
retargeted torso/limbs (`assets/authored/player-body.skin`) derive
from the realistic male in **Blender Studio Human Base Meshes v1.4.1**, released
under **CC0 1.0 Universal**. The bundle was already present locally with recorded
user download authorization; this integration downloads no assets.

- Bundle information: https://www.blender.org/download/demo-files/
- License: https://creativecommons.org/publicdomain/zero/1.0/
- Original bundle SHA-256: `811f43accbb31a88266d932f8f5563b2d13586fca0ba2693aad1f5fe582b3515`.

`assets/authored/player-anatomy.blend` is a byte-identical preserved copy of the
project's local `Human_Male_Anatomical.blend` study, including its editable anatomy
and earlier clothing work. Its SHA-256 and export provenance are recorded in
`player-head-source.json` and `player-body-source.json`. The existing `*.blend`
Git LFS rule covers this source. Only the bounded native exports enter the game
executable. The full study and its tailored clothes are not loaded at runtime.

The export fits the neck to the existing presentation rig, changes coordinates,
reduces topology into three head/two scalp LODs, and adds a neutral complexion
multiplier. Source anatomy is not claimed as an original Mediterranean sculpt.
The body export cuts attachment sockets, fits the ten-bone presentation bind
pose, computes four-influence weights offline and reduces to three closed LODs.
The thirteen original collection models keep their existing world roles and
authored equipment remains the basis for the playable guard's armor.
