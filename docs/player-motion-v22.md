# Sword handling, forward gait and cloak response — v22

Editable motion source: `assets/authored/player-motion-v22.blend`.
The PCL5 archive contains 59 tracks: the existing 57 plus two first-strike
entries, `Sword_Attack_Carry_Rising` and `Sword_Attack_Carry_Sweep`.
The Blender file contains 53 Actions and retains the existing 57 rest bones.

## Changes

- First sword strikes select an entry from the actual low carry. Subsequent
  strikes blend toward the prepared guard using readiness captured at attack
  start. The low-entry source already contains preparation acceleration, so
  playback does not apply another contact-length acceleration to it.
  Carry-to-guard orientation travels outside the arm. Partial-ready entries use
  the same orientation path as the visible stance, then select one complete
  recovery after contact instead of mixing opposing follow-throughs. Regression
  checks cover 0%, 25%, 50%, 75% and 100% readiness.
- Sword cuts animate the connected upper arm, forearm and gripping hand.
  Their blade orientation follows the hand frame. This replaces independently
  specified hilt positions and blade directions that could force extreme wrist
  deviation. The existing oval handle fit and finger lengths are retained.
- Forward foot lanes narrow with locomotion weight: walking approaches 21 cm
  and running 17 cm. Authored leg targets follow this change; contact anchoring
  still owns the planted feet. Sideways travel retains its clearance rules.
  Warrior thigh plates fit closer to the existing clothed leg.
- The cloak's damped middle and hem respond to torso angular motion and facing
  rate, including standing cuts. Initial orientation does not create an impulse.
  Reset clears the angular history along with the existing cloth state.
- Two existing swords have reconstructed blade sections, a tapered fuller,
  sharpened bevels and localized edge wear. Each has 16,522 near triangles.
  The existing warrior shield has a rear plank layer, surface cuts and a
  continuous wood surface following its camber, with 29,448 near triangles. Reduced distant LODs
  remain. KIT1 uses 30,810,180 bytes within the existing 32 MiB budget.

The change does not alter gameplay hit times, movement speed, jump physics or
the roadmap. Faces, scalp, hair, eyes, facial shading, garment skin weights and
all thirteen user-authored collection models remain frozen. Twenty existing
motion tracks and thirty other equipment records remain byte-identical.

In matching native forward-run samples, mean foot separation changed from
23.3 to 17.5 cm; mean knee separation changed from 29.0 to 25.8 cm. In the
walking samples, maximum shaft/forearm wrist deviation changed from 36.2 to
6.7 degrees. Running wrist deviation did not improve uniformly: its mean
changed from 13.7 to 15.0 degrees, while the maximum changed from 24.9 to 22.1.
These measurements describe the reviewed sequences, not every gameplay blend.

## References and limits

Reviewed the supplied `D:\assets\learning_material\3.jpg` and the new
`pve_raid_2.mp4` (77.27 seconds at 60 fps), alongside the existing local reference
review. The new raid recording has substantial spell and camera occlusion; it
does not provide an isolated sword performance suitable for exact retargeting.

Professional references consulted:

- [Academy of Historical Fencing: holding the sword](https://swordfight.uk/hold-the-sword/)
  explains grip choices in relation to weapon and action.
- [Ubisoft: Learned Motion Matching](https://www.ubisoft.com/en-us/studio/laforge/news/6xXL85Q3bF2vEj76xmnmIu/introducing-learned-motion-matching)
  describes a production approach based on coherent motion data and transitions.
- [Rune Skovbo Johansen, GDC locomotion presentation](https://media.gdcvault.com/gdc09/slides/Rune_Skovbo_Johansen.pdf)
  discusses foot trajectories and locomotion adaptation.

These are research references, not downloaded animation assets. This change
does not implement motion matching or import mocap. The cloak remains a bounded
presentation model with a back envelope, without general cloth self-collision
or leg collision. The existing character art remains stylized; this revision
does not claim photorealism or an exact reconstruction of the reference game.

## Review and verification

Local authoring scripts, captures, source audits and validation logs are under
`.blender-local/motion-v22-20261113/`, ignored by Git. `review.html` compares
native gameplay-rendered sequences and provides frame stepping. The final
machine-readable results are recorded in `validation-summary.json` after
validation completes.

Full tests use `build-source-release` and `build-source-debug` in that local
folder, built from a consistent local `source-snapshot` with a SHA-256 manifest.
Shared build directories and effect interfaces changed during parallel project
work; those incomplete or mixed-source attempts are not treated as successful
validation. The native review uses the same snapshot headers and libraries.
Integration checks separately inspect and launch the six usual game executables.
