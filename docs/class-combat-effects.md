# Class combat effects

Implemented on 2026-10-22 in the native C++/DirectX 11 renderer. This pass makes the existing class combat disciplines visible and audible at preparation, release, contact and during persistent conditions. It does not change damage, cooldowns, talent balance or save version 12.

## Research and design choices

- The official [Allods class guide](https://allods.ru/about.php?show=classguide) distinguishes elemental magic, holy protection, musical support and class resources. We use those broad identities to organize our existing kits.
- Lost Ark's official [Mage advanced-class showcase](https://www.playlostark.com/en-us/news/articles/mage-advanced-classes) describes Bard notes, sound waves and support, alongside elemental attacks. Its [Gunner showcase](https://www.playlostark.com/en-us/news/articles/gunner-advanced-classes) distinguishes weapon roles and physical shots. These are historical class showcases, not current balance specifications. The implementation uses original shapes and audio, not their assets.
- Cached frames from the user's `allods_fight_interface.mp4`, including the approach sequence around 01:17.5 and the 01:14–01:33 contact sheet, show persistent ground ornament separately from short weapon/contact flashes. This informed the separation of live status effects from transient impacts. This pass reviewed those extracted frames; it did not decode or claim a new complete frame-by-frame analysis of either recording.

The intended visual hierarchy is a small preparation cue at the solved hand or muzzle, a directional projectile or blade trail, a short contact effect, and restrained residue. Status effects follow their actors and end with the actual gameplay condition. Most centers remain transparent so the character, feet and enemy attack warnings stay readable.

## Implemented identities

| Class | Release and contact treatment | Existing payoff made readable |
|---|---|---|
| Warrior | Weighted blade trails, oblique contact cuts and metal sparks | Additional delayed ground contour on a full-Rage heavy |
| Paladin | Gold blade trail and radial holy seal; resonant heavy-release cue | Live barrier arcs; a separate ripple and ward sound only on absorbed damage |
| Scout | Narrow physical bolt, fletching, compact heavy sight cue and precision impact marks | An ankle restraint while the target is snared |
| Priest | Pale-gold lance, halo and vertical light rays; bell-like release | Barrier outline and absorption feedback |
| Druid | Branching lightning trace and impact; crackling release | Vines around the lower legs for the real root duration |
| Mage | Fire comet, rising ember tail, layered flame impact and ground heat contour | Larger, delayed ember contour for an empowered heavy |
| Necromancer | Twin soul strands and a rising helix; low beating release tone | Heavy contact sends a drain mote toward the snapshotted caster |
| Psionic | Paired travelling lenses and contracting impact arcs; opposing pitch sweeps | Orbiting marks above the head while stunned |
| Bard | Notes and spreading wavefronts; a short plucked chord | Additional contact contour distinguishes the empowered cadence |
| Engineer | Pistol muzzle flare, short tracer, shrapnel and pressure ring | Segmented target brackets while exposed |
| Demonologist | Crimson blade trail and three claw-like cuts; rough low release | A delayed crimson contour on the empowered heavy |

Basic and heavy attacks retain their real class clocks. Heavy preparation ends on release or cancellation. Physical launchers receive a sight cue rather than an early muzzle flash. The emission event snapshots class, heavy/empowered flags and caster position so later selection or recovery cannot reinterpret the accepted action. A barrier contact is separate from a health-damage contact: a fully absorbed hit produces a ward ripple and sound without a false hurt effect.

The shared effect shader now preserves school hue in bright cores. Previously it mixed every core strongly toward ivory, making thin fire, nature and mind effects converge to white in daylight. Explicit pale contact flashes remain possible. Magic contact audio now uses a short nonmetallic impact rather than the blade-hit sample. Recovery/potion effects and the common basic-to-heavy resource structure remain shared.

## Runtime bounds and lifecycle

- Texture-free geometry stays in the existing additive, depth-tested effect batch; it writes no depth and is excluded from portraits. No assets were downloaded and no new runtime dependency was added.
- The fixed emitter pool remains 32 bursts, 8 weapon trails and 32 samples per trail. CPU state is 10,736 bytes; the frame upload remains capped at 6,144 vertices. Overload drops excess cosmetic geometry rather than allocating more memory.
- Live conditions are drawn directly from simulation state, without emitting a new burst every frame. At most eight affected living actors within 30 m receive these decorations. Root/snare/stun/exposure opacity eases out during the last 250 ms; barriers reflect remaining capacity. Dead actors and expired/reset conditions draw nothing.
- New class sounds are generated once and use the existing twelve-voice pool. Music plus the complete sound bank remains below 2 MiB. The existing limiter, subtle pitch variation and mute/focus behavior remain active.
- Rendering does not grant damage, healing, reserves or control effects. Contact-time range/line-of-sight rules remain authoritative; a released visual trace is not evidence that a hit succeeded.

## Validation and remaining work

`ClassEffectsTests` captures all eleven ordinary/empowered contacts, all eight ranged projectiles at 30/60/144/1000 Hz, six live status states and shader hue probes. It checks local contrast, distinct payoff layers, expiry, preparation cancellation, save/load cleanup, render-only state independence and the existing CPU/vertex budgets. `ClassCombatTests` additionally checks exactly one release/contact and committed event metadata across all class clocks, including barrier-hit events. Audio checks cover distinct class samples, clipping/click boundaries and memory limits; the existing effect suite covers depth occlusion, HUD/portrait isolation and overload.

Review `build-ninja-debug/class-effects-check/` after running the tests. Manual checks 199–201 cover live combat readability, all camera directions and sound balance. Static native captures establish appearance and timing checks establish event alignment; sustained play and listening are still required to judge feel. These original procedural effects do not provide production texture animation, authored skeletal VFX attachments, bloom, full MMO rotations, summons or transformations.
# Recovery identity follow-up — 2026-10-22

Research revisited the [official Allods class guide](https://allods.ru/about.php?show=classguide) and the [Lost Ark Mage showcase](https://www.playlostark.com/en-us/news/articles/mage-advanced-classes). The former distinguishes protective, healing and musical roles; the latter's historical 2021 Bard examples connect notes and rhythmic spells with recovery/support. These are design references, not current balance specifications or copied assets. This follow-up implements original recovery presentation and limited defensive mechanics; it does not add summons, transformations, party healing or full MMO rotations.

Key **4** now produces distinct short-lived recovery contours: Warrior breath flecks; Scout linen wraps; Paladin compact gold prayer rays; Priest wider light rays; Druid climbing vines/leaves; Mage frost shards; Necromancer inward soul streams; Psionic paired diamond lenses; Bard delayed notes and ground wavefronts; Engineer repair sparks; Demonologist crimson inward cuts. Shapes are anchored at the accepted cast position, depth-tested, bounded and expire within 0.8 seconds. They use existing geometry batches and authored restoration poses, with no new textures, downloads or persistent particle allocation. Potions retain their gold pulse.

Successful recovery also establishes a ward for Paladin (18), Priest (10), Mage (14), Psionic (12) and Engineer (8). It replaces capacity only when stronger than the remaining ward; capacities never add together. Health must be missing, and ordinary action/cooldown restrictions still apply. The ward absorbs actual incoming damage and is cleared by existing load/death/reset rules. These values are prototype balance choices. Spellbook/tooltips show the amount; existing HUD/absorption effects report remaining capacity. Mage recovery and its live ward use frost blue, distinct from offensive fire.

Recovery audio selects existing holy, ward, mind, musical, soul or Mend families from the accepted event's class. Rejected casts are silent. The PCM bank and twelve-voice mixer remain unchanged. Live listening and melee/ranged balance are still manual checks; automated event/audio checks run muted.

Tests add eleven-class heal/ward contracts, full-health/cooldown rejection, stronger-ward preservation and load clearing. Native captures cover every recovery at equal elapsed time across 30/60/144/1000 Hz, visible contrast, fewer than 900 vertices per recovery, and complete expiry. See progress.md for build results and manual check 205.

