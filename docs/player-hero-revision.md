# One player warrior: fitted fantasy equipment (2026-11-05)

This revision changes the warrior's clothing and armor silhouette around the
accepted anatomical player body. The existing two equipment states remain
interchangeable: a leather-faced brigandine and an armored breastplate. It is
one character study, not a replacement of the other classes or the thirteen
user-authored world models.

## Appearance and source

- `assets/authored/oldstyle-classes.blend` is the editable garment source. It
  retains the preceding warrior meshes in `PRESERVED | pre hero realism`.
- `assets/authored/player-hero-study.blend` opens a fully assembled, posed
  warrior for inspection. Its visible meshes are a snapshot of the native
  rig; the hidden `SOURCE` collection contains editable bind-space garments.
  This study is not a new animation rig. The motion source remains
  `assets/authored/player-motion-v8.blend`.
- `.blender-local/hero-realism-20261105/review.html` contains actual DX11
  captures, eight directions, close views, equipment comparison and motion
  frames. Local scripts and intermediate exports remain ignored by Git.

Nine equipment parts were revised: both chests, shoulder armor, skirt, belt,
bracers, greaves, poleyns and thigh plates. Changes include a fitted gambeson,
formed breastplate with central keel and plackart, rolled edges, curved oak
inlay, fitted harness, interlinked mail collar and hem, repaired wool, sewn map
case and overlapping shoulder lames. A continuous padded yoke closes the
neck-to-shoulder junction, fitted to the retained anatomical surface. Its
weights interpolate the body weights; a shoulder seam allowance copies the
underlying skin weights exactly, preventing exposed wedges as the arms move. The
cape's pinned seam now meets the doublet instead of floating behind the
shoulders; its drape clears the upper back while retaining the pinned edge.

The existing Dawnwarden equipment and accepted garment meshes supplied the
foundation. The user's local Allods reference sheets supplied layering and
silhouette references. Historical construction references included the
[Met brigandine breastplate](https://www.metmuseum.org/art/collection/search/23152)
and [Worcester brigandine](https://worcester.emuseum.com/objects/53689/brigandine).
No external files were downloaded for this revision.

## Rendering and resource contract

Four material layers append to the eighteen retained IDs: forged steel (18),
saddle leather (19), arming cloth (20), and riveted mail (21). The authored
material sentinel moves to 22. These surfaces have separate relief,
roughness, metallic response and metric texture scale. The original face,
scalp and skin shaders remain unchanged. A local normal receiver offset on
the new metal surfaces reduces self-shadow bands without disabling shadows.

KIT1 contains 28,467,932 bytes, below the explicit 32 MiB library bound;
GWT2 contains 10,478,304 bytes, below the retained 10 MiB bound. The detailed
armored chest has 82,818 triangles, the travel chest 74,078, the shoulder
assembly 17,352 and the skirt 33,809. Both native LODs are exported. Small mail
wire cross-sections are reduced to keep the actual skinning budget bounded.
Material/terrain/shadow allocation is bounded by 11 MiB after adding layers.

## Preservation and review limits

Hash and record comparisons against the pre-edit local snapshot preserve the
face source, face metadata, anatomical body archive, motion source and clips,
both original collection copies and collection export. Twenty-four other
equipment geometry records and eight other garment weight records remain
byte-identical. The archived original meshes are retained in the source.

This is a substantial equipment and material revision, not a claim of
photorealistic facial reconstruction. The frozen face, eyes, scalp and facial
shading still determine the character's facial appearance. The assembled
Blender study approximates native lighting; DX11 captures are authoritative
for the in-game result.
