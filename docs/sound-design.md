# Dawnwatch sound direction

## Enemy anticipation — 2026-10-25

Enemy attack commitment now emits one spatial preparation cue before release. Ordinary preparation uses a short cloth/metal tension sound; boss preparation uses a lower resonant load. These original synthesized samples last 0.32 and 0.48 seconds and are generated once at startup. They are attack-onset accents, not looping cast tracks: an interrupted cast may retain the short sound tail but cannot produce another preparation or impact from the audio system. Existing release cues remain separate.

Preparation uses the caster's position and existing distance attenuation/panning. It allocates no particle bursts and does not change hit timing. Loading a pending cast does not replay its onset. The resident PCM bank is now 4,147,592 bytes, below the existing 4 MiB limit; the twelve-voice pool is unchanged. Automated timing/spatial/persistence checks pass, but live audibility and perceived timbre remain manual acceptance work.

## Phrasing and dynamics — 2026-10-25

Score phrasing and dynamics (2026-10-25): varied melodic sustain/phrase accents, quieter sparse bell entrances, a five-note harmonic harp figure and less intrusive score drums refine the existing original 48-second score. Adaptive percussion now has unequal accents and slight offbeat timing. Music slider changes and resume use an 80 ms exponential response shared by score/percussion, with finite-input handling and frame-rate checks. No additional PCM buffers, voices or runtime synthesis; total resident bank remains 4,112,312 bytes (under 4 MiB). Release build/AudioTests pass (1.90 s). Visual Studio Debug AudioTests build/pass (4.27 s); full Debug game relink is blocked by LNK1168 on the running RPGNative.exe, which was left untouched. Automated checks remain muted; subjective listening is pending. Generated dawnwatch-music.wav and adaptive-score-preview.wav in build-ninja-release. No downloads, commits or pushes.

The composition remains synthesized mono audio, not sampled orchestral instruments. The 48-second length, 80 BPM pulse and harmony are unchanged. Per-note sustain and phrase amplitude vary; bells occur at phrase openings, and harp notes follow each section’s root/third/fifth. Background score drums are less frequent and quieter, leaving the adaptive stem to carry combat tension. Music gain settles gradually rather than jumping between slider values; suspension resets gain to silence for a soft resume. Effects gain/event routing is unchanged.

## Ground contacts — 2026-10-23

Footsteps and landing impacts now select stone, dirt, grass or timber from the shared WorldMap surface weights and dock footprint. Paving/dirt use a 0.5 coverage threshold; the timber deck takes precedence. Existing stone contacts retain their sound; grass adds a softer rustle, dirt a grainy crunch and timber a short hollow resonance. All are original procedural samples, generated once at startup. Their audible realism still needs listening acceptance.

Material selection occurs only on accepted travel/landing events, using the current player position. Distance-based cadence, silent airborne travel/dashes and deterministic pitch variation remain intact. Ground feedback stays centered and behind combat impacts. There are no new voices or runtime synthesis. Six short samples add 74,088 bytes, bringing the bank to 4,068,654 bytes (3.88 MiB), below 4 MiB.

AudioTests exports ground-contacts-preview.wav: stone step/landing, grass step/landing, dirt step/landing, then timber step/landing. Tests cover walkable surface fixtures, distinct deterministic PCM, walking/jumping at 30/60/144 Hz, event routing, false footsteps, levels and total memory. Manual check 216 covers surface boundaries, repeats and in-game balance. Contacts use the character centre rather than individual boot positions; terrain heights and physical material friction are unchanged.

## Regional ambience — 2026-10-23

Three original six-second mono beds distinguish open ground (wind and short bird calls), the village (quiet creaks and a distant bell), and the anchor/northern bastion (airy low harmonics and bell tones). Smooth spatial weights blend into a 1.5-second gain response. Combat intensity reduces ambience by up to half, leaving more room for warnings. The Effects slider controls all three beds; Music remains independent. Load/profile changes and suspension reset the blend.

All samples are synthesized once at startup. Three additional looping voices and 793,800 PCM bytes bring the total bank to 3,994,566 bytes (3.81 MiB), below the 4 MiB ceiling. These are quiet procedural backgrounds with faded loop endpoints, not recorded environmental audio or spatial emitters. Their six-second repetition and perceived balance still need listening acceptance; occlusion and surround placement remain open.

AudioTests covers region coverage, normalized weights, crossfade/duck agreement at 30/60/144/1000 Hz, reset, deterministic synthesis, endpoints, signal bounds and the total memory ceiling. It exports ambience-0.wav (wind), ambience-1.wav (village) and ambience-2.wav (astral). Manual check 214 covers live listening.

## Adaptive percussion — 2026-10-23

A six-second percussion stem now shares the main score's 80 BPM beat. Both sources start in one XAudio2 operation set; the 48-second score and six-second stem loop continuously without restarting when combat changes. Accepted attacks or living pursuing enemies within 18 m raise tension. Gain rises with a 1.2-second response, holds for three seconds after the last threat, then decays with a three-second response. Short breaks therefore do not repeatedly switch the soundtrack on and off.

The stem follows the Music slider independently of Effects. Death, profile generation changes and audio suspension clear its intensity. The score remains uninterrupted during normal transitions. This is one adaptive percussion layer, not a separate combat composition or a full adaptive orchestration system.

Resident PCM is now 3,200,766 bytes (about 3.05 MiB), still below the 4 MiB ceiling. One additional looping source voice is used; the twelve effect voices are unchanged. AudioTests exports combat-music-layer.wav and adaptive-score-preview.wav; the latter demonstrates exploration, tension from seconds 4–10, then release. Tests cover frame-rate-independent fades, hold/reset behavior, threat detection, layer determinism/limits and mix headroom. Manual check 213 covers listening acceptance.

## Combat placement — 2026-10-23

Enemy swings, target impacts and interruptions now retain their world positions. Stereo output uses equal-power left/right placement relative to the player's position and camera heading. Playing tails are updated as the listener moves or turns. Distance gain decreases smoothly and fades to zero between 14 and 18 metres. Player releases, movement, healing and damage feedback remain centered, so turning away does not hide the player's own feedback.

The existing twelve effect voices retain fixed source metadata; no extra PCM is allocated. Gain budgeting includes attenuation before voice changes are committed together. Mono and multichannel endpoints retain XAudio2's default channel routing with distance attenuation; directional panning in this pass is specifically for stereo. There is no wall occlusion, elevation/HRTF processing, Doppler or moving-source tracking after emission. Long tails stay at the original event location.

AudioTests checks left/right orientation, camera rotation, equal-power coefficients, distance falloff, invalid inputs and event-origin ownership. It exports `spatial-combat-preview.wav`, seven impacts moving from left to right, for headphone audition. Manual check 212 covers live perception.

## Mystic chronicle — 2026-10-22

The previous 32-second pad/pluck loop is replaced by an original 48-second, 80 BPM D-minor phrase. Eight six-second harmonic sections layer synthesized bowed-string partials, breath-like choir tones, a sustained melody and sparse harp notes. Lower horns and restrained drums enter in the second half, followed by a quiet resolution. Overlapping releases and baked chamber reflections give the score depth; faded endpoints avoid a sample discontinuity when looping.

This is procedural mono PCM, not a sampled or recorded orchestra. No third-party recordings or downloaded assets are used. Composition and timbre are intended to evoke a mystical, epic classical RPG; listening acceptance remains subjective and pending.

Spell cues retain class-specific pitch/attack identities while gaining lower fundamentals, breath texture and quiet reflections. Most mystical cues now last 1.05 seconds. Physical impacts gain a low transient layer; jump uses cloth/boot push-off instead of a rising electronic sweep. Footsteps stay short, and the existing twelve-voice pool, cue variations, event filtering and peak-budget gain protect combat readability.

Music and effects still have independent menu sliders. All synthesis and reflections are generated at startup, during the audio loading stage; runtime playback does not run the synthesizers. The resident PCM bank is 2,936,166 bytes (about 2.80 MiB), compared with the preceding 2,004,342 bytes. The regression ceiling is now 4 MiB. This extends the former 2 MiB budget deliberately to accommodate the longer score and tails.

AudioTests exports ignored local previews under the selected build directory:

- `dawnwatch-music.wav`: complete score.
- `effects-preview.wav`: every effect in enum order, with gaps.
- `combat-mix-preview.wav`: representative footsteps/combat/recovery mix.

Automated validation checks deterministic synthesis, non-silent phrases, bounded peak levels, silent endpoints, retained mystical tails, total memory, accepted-event routing and muted XAudio2 startup/suspend/resume. It does not establish perceived instrument realism, listening comfort or final music/effects balance. Manual check 211 covers those.
