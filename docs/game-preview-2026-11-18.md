# Native gameplay preview — 2026-11-18

Output: `build-trailer-current/Mediterranean-preview.mp4` (ignored build artifact).
64 seconds, 1280 × 720 at 30 fps, H.264 video and 48 kHz stereo AAC audio.

The preview uses the current DirectX 11 renderer, existing characters, actual
simulation combat events and generated game sound cues. Encounters are staged
in memory; player profiles and authored models are not modified. This is a
presentation recording, not a performance benchmark.

## Edit

| Time | Scene |
| --- | --- |
| 0–12 s | Village and surrounding landscape |
| 12–16 s | Priest portrait |
| 16–30 s | Priest combat and Sanctuary |
| 30–34 s | Psionic portrait |
| 34–42 s | Psionic combat |
| 42–46 s | Mage portrait |
| 46–59 s | Mage combat and meteor |
| 59–64 s | Closing panorama |

Music is mixed from the already available Fantasy Choir 2 and 3 source
recordings used for the game's soundtrack. See `assets/audio/CREDITS.md`
for César da Rocha's CC0 attribution. No new media was downloaded.

## Reproduce

From the repository root, using Python with NumPy and the configured MSVC
developer environment:

```powershell
python scripts/prepare-trailer-music.py
cmake --build build-ninja-release --target TrailerCapture -j 3
build-ninja-release/TrailerCapture.exe D:/Mediterranean/build-trailer-current
```

The preparation script requires the existing local source recordings in
`build-audio-recordings`; these are not downloaded automatically.

## Validation

- Native capture finished all 11 shots and verified actual hit events in every
  combat scene (`shots.csv`).
- The encoded MP4 decoded successfully through Windows Media Foundation;
  sampled frames were visually inspected.
- Decoded audio: 48,000 Hz, two channels, 64 seconds; normalized RMS 0.1343,
  peak sample 24,857 / 32,768 (non-silent, no sample clipping).
- Output size: 65,481,675 bytes.
