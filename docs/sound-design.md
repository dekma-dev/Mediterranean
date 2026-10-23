# Dawnwatch sound direction

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
