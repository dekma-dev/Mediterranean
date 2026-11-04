# Original combat effect materials

`combat-vfx.png` is an original RGBA atlas generated with the built-in imagegen
tool. It does not contain extracted reference-game textures. Source pixels and
alpha are retained; the native WIC loader decodes once to 1024 x 512 with mipmaps
(approximately 2.67 MiB). No per-frame image decoding or upload is required.

Four columns, two rows: Flame, Smoke, Dust, Ice / Arcane, Slash, Sparks, Seal.
`Renderer::EffectArt` is the authoritative material order. Insets and a soft
cell-edge mask prevent adjacent cells from bleeding. Flame bodies, smoke and
dust use sorted straight-alpha blending; luminous cores, shards, seals and
slashes join the additive effect batch.

## Final generation prompt

Use case: stylized-concept. Asset: ORIGINAL real-time fantasy RPG combat VFX texture atlas, FOUR columns and TWO rows of exactly equal SQUARE cells, wide 2:1 canvas. Transparent RGBA background, not a mock checkerboard. No text, UI, frames, characters, scenery or logos. Production VFX sprites, rich fine wispy detail with SOFT TRANSPARENT edges, isolated and centered in each cell with 12 percent clear padding, no sprite crosses cell boundaries. Each cell is a separate animation building block; not a finished scene. Row-major exact subjects: TOP ROW 1 orange-gold tongue of flame and curled embers, upright with dense bright core and translucent licking flame wisps; 2 neutral gray rounded turbulent smoke puff, lit with delicate internal volume and translucent edge, no fire; 3 warm ochre dust plume with scattered small grit, soft irregular rounded cloud; 4 slender elongated icy crystal shard pointing UP, angular faceted blue-white body with pale cold vapor. BOTTOM ROW 1 violet arcane energy vortex, open swirling spiral with richly detailed filament fringe and bright center; 2 broad diagonal white-silver crescent slash, tapered bright blade edge and translucent streaked afterimage; 3 localized golden impact spark burst, asymmetric short jagged rays and bright central flash, not a regular star; 4 thin intricate pale-cyan circular arcane seal seen perfectly face-on, original interlacing loops and short radial marks, no readable letters and no real religious symbols. High contrast locally, preserve plenty of transparent negative space. Realistic cinematic flame/smoke/dust and elegant classical MMORPG magical energy. Not cartoon, not flat vectors. Keep the exact 4-by-2 alignment and all silhouettes away from the cell borders. This is one atlas asset.
