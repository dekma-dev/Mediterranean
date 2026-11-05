# Recorded music and sound credits

Added 2026-11-05. The following source assets are offered under
[CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).
This selection uses CC0 where a source offers multiple licenses. No audio from
the reference games or the user's gameplay recordings is redistributed.

| Author | Source | Use / processing |
| --- | --- | --- |
| César da Rocha (cesisco) | [Fantasy Choir: 3 orchestral pieces](https://opengameart.org/content/fantasy-choir-3-orchestral-pieces) | Full pieces 1 / 2 / 3 become character-menu / exploration / combat music. Stereo, gain matched, cadence fades, Microsoft ADPCM encoding. |
| artisticdude (submitted by Ogrebane) | [Battle sound effects](https://opengameart.org/content/battle-sound-effects) | Bow.wav: release and aimed-shot layers, trimmed/resampled/pitched variants. |
| Julien Matthey (submitted by diligentcircle) | [Fireball](https://opengameart.org/content/fireball-1) | 105016__julien-matthey__jm-fx-fireball-01.wav: release, heavy release, contact and reversed preparation textures. |
| HaelDB and the vocalists credited in the source pack | [Male grunt/yelling sounds](https://opengameart.org/content/male-gruntyelling-sounds) | Three distinct short effort takes: 3grunt3, 3grunt4, 3grunt5. Trimmed and level matched. |
| Fantozzi | [Footsteps: grass/sand and stone](https://opengameart.org/content/fantozzis-footsteps-grasssand-stone) | Sand/stone L1, R2 and L3 recordings. Separate steps/landings; filtered sand treatment for grass. Timber remains procedural. |
| Roberto, FreePats project | [Church Organ Emulation](https://freepats.zenvoid.org/Organ/pipe-organ.html) | C2/C3 ranks sampled from the Aeolus organ emulator, transposed into a D pedal/open fifth for Priest cues. This is a sampled instrument emulation, not a recording of a church service or live choir. |
| PWL | [Bell dings/chimes](https://opengameart.org/content/bell-dingschimes) | bell_ding1.wav: pitched bell layer in Priest release/contact/healing. |

The runtime bank contains edited, normalized derivatives. Three versions of
each recorded cue avoid exact waveform repetition; bow/fire/holy variants are
processed variations, while grunt/step variations use distinct source takes.
Sword impacts and existing frost/arcane/body/wood synthesis are retained.

## Reproduction

`scripts/build-recorded-audio.py` takes already downloaded/extracted sources and
an existing libsndfile DLL, using NumPy for offline filtering/encoding. It does
not download files, install software or open/change Blender scenes. Original
archives and full-resolution WAVs are kept only in ignored
`build-audio-recordings`; runtime builds need neither Python nor libsndfile.

Downloads:

- [FantasyChoir24bit.zip](https://opengameart.org/sites/default/files/FantasyChoir24bit.zip)
- [Battle effects ZIP](https://opengameart.org/sites/default/files/battle_sound_effects_0.zip)
- [Fireball WAV](https://opengameart.org/sites/default/files/105016__julien-matthey__jm-fx-fireball-01.wav)
- [Grunts ZIP](https://opengameart.org/sites/default/files/yelling%20sounds.zip)
- [Footsteps 7z](https://opengameart.org/sites/default/files/Fantozzi-footsteps.7z)
- [FreePats organ archive](http://freepats.zenvoid.org/Organ/ChurchOrganEmulation/ChurchOrganEmulation-SFZ-20190924.tar.xz)
- [Bell WAV](https://opengameart.org/sites/default/files/bell_ding1.wav)

`recordings.json` records each used source SHA-256, output hash, cue length and
music duration. The FreePats archive was retrieved from the publisher's public
HTTP endpoint because its HTTPS certificate had expired; the local source
hashes are reproducibility records, not independent proof of authenticity.

Music occupies 12,487,462 bytes as files (about 11.91 MiB); the PCM effects bank
is 2,693,386 bytes (about 2.57 MiB). Music stays encoded in executable resources;
XAudio2 decodes blocks on its audio processing thread. The renderer never
decodes or synthesizes these tracks, and no whole-track PCM heap is allocated.
The raw downloads are not required in a checkout or distributed build.
