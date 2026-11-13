# Spell flow and contact continuity

## Reference and scope

Revisited the supplied `D:\assets\learning_material\pve_loc.mp4`: fourteen overview samples across 0–56 seconds, followed by thirty samples at approximately 1/30-second intervals across 23.8–24.8 seconds. Also re-examined the 8.52–8.88 second contact sequence from `pve_loc2.mp4`. The first sequence contains a golden offensive burst, not a matching Mage fireball. It supplies staging evidence (gathering, a short bright peak, separated moving strands and lingering fragments), not proof of spell-for-spell fidelity. The second sequence shows a substantial target-local flare remaining readable at ordinary combat distance. These are sampled passages, not a claim to have inspected every frame of every recording.

Local reference frames and comparison artifacts are under `build-vfx-flight/`; no reference images or video were imported as game artwork. This pass changes runtime effects, not authored character/face/model data, damage, cooldowns, targeting or save data. Full reference parity remains unfinished.

## Runtime change

- A new analytic flame-sheet material carries unwrapped coordinates across joined geometry. Advected noise tears the edges and separates the hot interior from cooler tongues. It shares the existing depth-tested effect batch and effects-only glare pass; it adds no texture download, offscreen target or growing particle allocation.
- Fire media have a leading hot heart, warped rolling density and irregular tongues. A released fire body stretches along its accepted trajectory; three curved sheets occupy different world planes around its path. A narrow inner trace and detached smoke/embers complete the wake.
- Fire gathering now follows the actual solved hands and release focus with continuous flame streams. Ordinary fire no longer gathers six separate flame pictures. Shared preparation streams, including Priest streams, use joined edges rather than independently end-masked streak segments.
- Fire release opens three curling sheets at the sampled launch socket. Confirmed contact briefly compresses the arriving heart along its incoming axis, opens a broader curled flame crest, and throws ballistic embers before smoke/aftermath takes over.
- Fixed an emission discontinuity: selecting volume puffs by their index in the newest-particle list reassigned existing puffs every emission tick. Puff identity now follows its birth index; drift uses the tangent at birth instead of the changing head tangent.

The 24-volume, 1.25 projected-coverage, 96 alpha-quad and 6,144 total effect-vertex caps remain. Warning submission priority remains unchanged. Material size is not substituted for a larger gameplay damage footprint.

## Validation

The native checks cover continuous material coordinates under different tessellation, visible material advection, repeatability, opaque occlusion including glare, invalid input, emission-tick continuity, flight at equal ages across refresh rates, effect expiry, creation viewport isolation, shader bindings and the existing combat/creation regressions.

- Eight native suites passed in Release (287.61 s) and Debug (469.18 s): CharacterSelection, CombatEffects, ClassEffects, VfxMaterials, SpellVolume, SustainedEffects, CombatRelease and CastingEffects. Wall times include competing compilation/capture work and are not FPS measurements.
- The first run caught an over-budget six-major-contact combination. Compression was changed to morph an existing plume; fewer crest segments recover the warning allowance. The unchanged six-contact geometry assertions pass. Emission-boundary mean pixel changes are 0.00013–0.00017 against a 0.008 discontinuity threshold.
- Visual inspection then rejected a hard ring-like fire preparation. Two smoother, narrower hand streams replaced the three coarse curls within the original 512-vertex preparation limit. The three affected suites were rebuilt and passed for that final refinement in Release (53.45 s) and Debug (89.88 s).
- Ninja Debug/Release and Visual Studio Debug/Release game executables were rebuilt/refreshed, then each passed the normal hidden startup/fullscreen-restoration/device/shader/resize/world/UI capture smoke. See `build-vfx-flight/smoke-results.json` and `validation-binaries.json`; all four executable timestamps match their successful checks. Launchers were also built.

The shared checkout received separate authored-weapon and asset edits during the pass; they were preserved. This VFX pass does not claim ownership or exhaustive validation of those changes. `implementation-sources.json` records the reviewed VFX source hashes. No commit or push was performed by this pass.

The native review uses six real accepted casts (fire, heavy fire, frost, arcane, thermal and holy), rather than fabricated visual hits. Both movies are 18-second, 720p60, silent recordings. The test enemy is rooted and prevented from attacking for repeatability; its health is normal. Its icy feet are that fixture status, not lingering spell residue. The review is not moving-target or crowded combat acceptance.

Use `build-vfx-flight/movie-before/combat-motion.mp4` and `build-vfx-flight/movie-final/combat-motion.mp4`. `movie-after/` is the intermediate iteration, retained for inspection; its preparation was subsequently refined. The final movie adds preparation and mid-flight stills beside the existing release/contact captures.

All twelve release/contact CSV rows match between before and final. The final file decodes as 1280x720, 18 seconds; twenty-two sampled frames across 4.10–4.80 s were inspected for gathering, release, travel and impact continuity. Normal-camera preparation/contact stills and the ordinary Visual Studio Debug smoke capture were also inspected. These artifacts, including `final-motion/`, show actual engine output. Further concurrent changes to `CharacterPose.cpp` and `AuthoredWeapon.h` arrived after the public game executables were built; those later changes are outside the four-host smoke evidence. The reviewed VFX source hashes remain unchanged.

## Remaining work

Moving-target/crowded four-class review; spell-specific match against a directly comparable recorded ability; environmental response and authored action staging. The present runtime layers are visibly inspectable, but they do not establish 1:1 art or combat fidelity with the reference game.
