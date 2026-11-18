# Environmental combat audio — 2026-11-18

The next shared-engine step after physical impacts and pressure fronts gives combat a relationship to the actual surrounding architecture. It uses the existing Warrior, Mage, Scout and Priest sound palette. No new class rules, recordings, downloads, authored models or facial data are involved.

## Runtime changes

- Spatial contacts carry world height. Distance attenuation and stereo placement use the three-dimensional source/listener separation; sources directly overhead remain centered. The listener is the champion at ear height, so a camera passing behind scenery does not incorrectly muffle the hero's surroundings.
- A cached 8 m grid indexes solid building bodies, ruins and survey stones. Five short source-to-ear rays estimate partial obstruction; intervening terrain is sampled at intervals no greater than 0.35 m within the 24 m query range. Trees, their crowns, decorative meshes and camera envelopes are not sealed sound barriers.
- Obstruction reduces direct gain down to 0.22 and applies a native one-pole low-pass filter, reaching approximately 850 Hz behind a complete obstruction. New voices start with their correct obstruction; existing voices interpolate changes over a 90 ms response. Queries refresh at no more than 20 Hz, while gain/filter interpolation follows the frame clock.
- Eight short horizontal masonry probes estimate nearby enclosure. A 240 ms response blends the send to one shared stereo 48 kHz wet-only XAudio2 room. The fixed preset has 18 ms initial reflections and a 0.72 s decay; the field has a minimal send, while close stone walls give a fuller short return. No delay-line allocation or preset replacement occurs for each cast or each frame.
- Enemy preparation, physical attack and beast emitters follow stable actor IDs while alive. Confirmed hit, arrow and spell-contact tails remain at their accepted world point. Own movement/release feedback remains centered. Stage previews, UI notices and music do not enter the environmental bus.
- Load/profile generation, menu changes, effects mute and focus suspension discard the old wet tail. Reset occurs with the audio engine stopped, preventing a reset/process race. The engine restarts only when not suspended. A dirty flag avoids repeated stops while muted. An unavailable native reverb falls back to the existing direct sound.
- The twelve-effect-voice pool and resident PCM bank remain unchanged. The wet contribution gets additional mix headroom; its bus follows the Effects control, including tails already in flight.

## Review audio

`AudioTests` produces `environment-combat-before.wav` and `environment-combat-after.wav` in the build directory. Both last 36 seconds at stereo 48 kHz. Every three seconds the order is Warrior, Mage fire, Scout arrow, Priest; the same four contacts repeat in each environment:

| Time | Environment |
| --- | --- |
| 0–12 s | Open field |
| 12–24 s | Nearby courtyard walls |
| 24–36 s | The same courtyard with a solid wall obstructing the source |

The before file has the old direct-only response. The after file uses the same PCM, the production obstruction response and the actual native reverb XAPO/preset. It is an isolated DSP audition at full Effects volume, not a capture of a live encounter: offline linear resampling and a fixed scene omit endpoint routing, real-time voice competition, changing actor positions and the rest of the mix. The runtime still requires headphone/speaker acceptance around real buildings and hills.

## Validation

AudioTests, RecordedAudioTests and CharacterSelectionTests pass in Ninja Release and Visual Studio Debug. Both public game executables are rebuilt. The new cases cover complete/partial wall obstruction, elevated sources, an off-center intervening terrain ridge, invalid coordinates, exclusion of foliage, 30/60/144/1000 Hz smoothing, real native delayed energy and reset while a fresh reflection tail is active. The muted runtime path moves an accepted enemy-warning emitter behind actual map architecture, then back into the clear; filter/send/operation-set calls succeed, as do stage/menu/load/focus lifecycle checks. The native offline host initially crashed when setting parameters before LockForProcess; the corrected order is exercised by the final tests.

The current map supplies 57 hard acoustic volumes. A Release microbenchmark of 2,400 five-ray queries took 4.40 ms in total on this machine; this is an isolated CPU measurement, not a real-time FPS claim. The repeated-contact DSP fixture reaches a direct ceiling of 0.56 and a combined dry/wet peak of 0.623, retaining headroom. The existing sample bank remains unchanged; delayed processing uses one shared native effect rather than per-cast PCM duplication.

Signal analysis of the 36-second review measured a maximum sample magnitude of 0.247 in both files. Open-field RMS changed by less than 0.007 dB; the fully obstructed examples fell by 13.48–14.01 dB, with strongly reduced energy above 4 kHz. `build-ninja-release/environment-audio-metrics.json` contains per-class measurements. These measurements validate response and signal bounds, not subjective sound quality. Headphone/speaker acceptance in a live encounter remains open.

## Limits and next work

This is bounded geometric game audio, not wave propagation, diffraction simulation, HRTF, Doppler or a full architectural acoustic model. Walls use existing collision bodies; roof shapes, openings in visual meshes, material absorption differences and rooms connected by portals are not reconstructed. Multi-channel endpoints retain native channel routing rather than a new surround spatializer. Short enemy preparation accents retain their natural tail after interruption; they do not become looping channels.

The supplied recordings remain aesthetic references; this pass does not establish identical timbre, orchestration or complete reference fidelity. Next acceptance should compare moving combat in the open, beside a house and across a ridge at ordinary music/effects settings, checking warning readability before any further sound-palette or class-design expansion.

## Native API references

- [Microsoft: XAudio2CreateReverb](https://learn.microsoft.com/en-us/windows/win32/api/xaudio2fx/nf-xaudio2fx-xaudio2createreverb)
- [Microsoft: native reverb parameters and sample-rate requirements](https://learn.microsoft.com/en-us/windows/win32/api/xaudio2fx/ns-xaudio2fx-xaudio2fx_reverb_parameters)
- [Microsoft: one-pole filter coefficient](https://learn.microsoft.com/en-us/windows/win32/api/xaudio2/nf-xaudio2-xaudio2cutofffrequencytoonepolecoefficient)
- [Microsoft: processing an XAPO](https://learn.microsoft.com/en-us/windows/win32/api/xapo/nf-xapo-ixapo-process)
