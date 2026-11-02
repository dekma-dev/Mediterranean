# Surface sources

Imported with the user's permission on 2026-11-01. Poly Haven distributes these
assets under [CC0 1.0](https://polyhaven.com/license), permitting redistribution
and commercial use. No attribution is required; the sources are retained here
for provenance.

| Runtime layer | Source | Authors | Measured coverage |
| --- | --- | --- | --- |
| Bark | [Bark Brown 01](https://polyhaven.com/a/bark_brown_01) | Rob Tuytel | 1 × 1 m |
| Wood | [Wood Planks Dirt](https://polyhaven.com/a/wood_planks_dirt) | Rob Tuytel | 2 × 2 m |
| Stone | [Rock Boulder Dry](https://polyhaven.com/a/rock_boulder_dry) | Dimitrios Savva, Rico Cilliers | 1.8 × 1.8 m |
| Plaster | [Painted Plaster Wall](https://polyhaven.com/a/painted_plaster_wall) | Amal Kumar | 2 × 2 m |

Each import uses the original 2048 × 2048 diffuse, OpenGL normal and roughness
JPG maps. Runtime PNGs pack sRGB diffuse in RGB and linear roughness in alpha.
Normals are renormalized and their green channel is inverted for the renderer's
top-left texture-V convention. No imported map is downsampled. `surfaces.json`
records source URLs, official MD5, SHA-256, sizes and runtime output hashes.
Imported diffuse colors bypass the old mesh palette tint; scene lighting and
foundation weathering still apply.
Downloaded originals and import scripts are ignored under `.blender-local`.

Roof and animal coat maps remain the locally authored 1024-square fields. The
renderer expands these two layers at load time to fit the shared 2K arrays;
this adds no source detail. Face materials and character textures are unchanged.
Two uncompressed six-layer RGBA arrays with full mip chains use about 256 MiB.
