# Projectile release and hostile contact effects

## Reference and intent

Revisited the supplied consecutive training sheet at 02:01.000-02:01.483
(`build-video-review/pass3-melee-02.jpg`) and dungeon sheet at
03:00.015-03:00.500 (`dungeon-motion-review-00.jpg`). The latter has an elevated
spiral distinct from the player's small ground ornament; the former separates
weapon energy, a short hit and persistent defence. These are observations of
existing decoded footage, not another exhaustive video review.

Blizzard's [combat-animation article](https://worldofwarcraft.blizzard.com/en-us/news/20946579/patch-7-3-new-combat-animations-it-s-magic)
discusses preserving responsive combat while making cast stages distinct.
Historical official [Allods update notes](https://allods.ru/news.php?article=8812)
also describe separate impact flashes and reduced effects for other players.
This single-player pass uses that distinction between contact and decoration;
it does not implement multiplayer visibility policy or copy external assets.

## Visible changes

- Mage projectiles now have a short launch seal at the sampled hand socket,
  separate fire/frost/arcane/thermal heads, and tapered world-length wakes.
  Fire carries irregular flame fragments; frost carries a crystalline core and
  shards; arcane has a violet braided wake; thermal has opposing hot/cold
  strands. The head disappears at contact, with at most 25 ms of blending;
  wake fragments drain for up to 160 ms. No speculative hit is created.
- Scout arrows have a 45 cm shaft rather than a length proportional to target
  distance. Twin/crossing shots separate in flight and converge at the accepted
  destination. Heavy arrows leave a restrained physical slipstream and a short
  release cut, without a magical fireball.
- Warrior impacts use a cut surface oriented to the attack instead of a
  camera-facing slash. Reverse/crossed attacks retain their orientation/count.
  Metal fragments follow gravity; the actual sampled blade trail gains a narrow
  pale edge that fades faster than the broad wake.
- Doomquake flashes its actual annulus immediately, then leaves broken warm
  fissures, thrown flecks and low dust. The safe inner flank stays clear.
  Iron tempest uses broken, raised violet spiral bands, cutting surfaces and
  low vapor. Execution line / Veil lance use tapered directional strands and
  a separate fading residue, replacing the flat strip and terminal orb.
- Projectile destination height and confirmed hit height now agree for wolves
  (0.72 m), boars (0.85 m) and humanoids (1.25 m), relative to terrain. Previously
  all used human chest height. This changes presentation only: target selection,
  damage, timing, reach, resource use and saves are unchanged.

## Timing and budgets

All effects evaluate immutable accepted events and age. Launch position comes
from the existing sampled weapon/hand socket; the destination belongs to the
committed target. Selection changes cannot redirect these cosmetics. Cancellation
before release emits no projectile; loading resets transients. There is no new
particle simulation, random frame state, texture, shader, dynamic light, asset
export or change to faces/models. The existing 2.67 MiB material atlas is reused.

Hostile releases use the budget after danger warnings and before player wakes.
An allocation-free array sorts them by age, actor ID and kind. One/two releases
retain full detail; larger groups use fewer segments/branches without changing
attack boundaries. Eight tempest releases submit 3,072 vertices. The global
6,144-vertex ceiling and 96 alpha-sprite ceiling still apply. Large over-budget
crowds can omit cosmetic detail; this is not unlimited MMO effect scaling.
Hostile dust/vapor expires by 0.72 seconds; damage still occurs once at contact,
not as dust expands. These are layered ribbon/sprite effects, not simulated
volumetric fluids, new combat animations or new encounter mechanics.

## Verification

`CombatReleaseTests` captures release/flight/decay phases, compares equal ages
at 30/60/144 Hz, checks bounded crowded effects and stable compositing under
reversed emission order, verifies expiry and render/load isolation, and drives
actual player/boss attacks. Real simulation events verify that creature contact
heights match projectile endpoints. The fixture uses isolated characters and
never writes player saves. Native captures live in `combat-release-review`
under each build; logs and baseline evidence are in `build-combat-release`.

The first baseline run captured isolated release sequences and Doomquake, then
failed to reload a diagnostic boss substituted into an incompatible campaign
save. The test now reloads its valid original fixture. The first regression
run also exposed an obsolete 0.65-second expiry expectation; it now waits past
the intentional 0.72-second residue and still requires zero remaining geometry.
Neither failure justified changing gameplay/save validation.

Manual acceptance remains: sustained boss pulls with multiple casters, near/side/
far cameras on hills, and final effect balance alongside the preserved authored
animations. This revision does not claim reference parity or measured FPS gains.

Final verification: all seven affected Release suites passed across the corrected
runs (contact, class effects, chains, material/depth, sustained states, new release
checks and class combat). The final crowd check now samples 60 ms, when the
contact border is still visible; its previous minimum-count expectation at
120 ms incorrectly included an already faded border. This corrected only the
test phase. Visual Studio Debug passes both CombatEffectsTests and
CombatReleaseTests. Native captures were inspected and thin tempest/fissure
layers were strengthened at the normal camera. Russian source coverage passes
at 2,047 occurrences / 1,895 entries; scoped whitespace checks pass.

Release and Debug game/launcher targets build successfully. A transient locked
Release executable initially prevented relinking; the later target build and
actual executable smoke succeeded without terminating a user process. Both
actual game EXEs pass isolated fullscreen restoration, device/shader, resize,
world/UI rendering and PNG capture checks. START_AND_DEBUG.md points to these
outputs. No commit, push, asset download or source-model edit was performed.
