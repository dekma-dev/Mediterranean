"""Mix already available game soundtrack sources for TrailerCapture. No downloads."""
import argparse
from pathlib import Path
import wave
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('--sources', type=Path, default=Path('build-audio-recordings'))
parser.add_argument('--output', type=Path, default=Path('build-trailer-current'))
args = parser.parse_args()
rate, duration = 48000, 64

def read(number):
    with wave.open(str(args.sources / f'Fantasy Choir {number}.wav')) as w:
        if w.getnchannels() != 2 or w.getsampwidth() != 3:
            raise ValueError('Expected the existing stereo 24-bit choir source')
        source_rate = w.getframerate()
        raw = np.frombuffer(w.readframes(w.getnframes()), np.uint8).reshape(-1, 3)
    v = raw[:, 0].astype(np.int32) | (raw[:, 1].astype(np.int32) << 8) | (raw[:, 2].astype(np.int32) << 16)
    v = ((v ^ (1 << 23)) - (1 << 23)).reshape(-1, 2) / 8388608
    samples = np.arange(duration * rate) * source_rate / rate
    if samples[-1] >= len(v):
        raise ValueError('Soundtrack source is shorter than the trailer')
    return np.stack([np.interp(samples, np.arange(len(v)), v[:, c]) for c in range(2)], axis=1)

world, combat = read(2), read(3)
t = np.arange(duration * rate) / rate
blend = np.clip((t - 14) / 3, 0, 1) * np.clip((64 - t) / 6, 0, 1)
mix = (world * (1 - blend[:, None]) + combat * blend[:, None]) * .60
mix *= (np.minimum(1, t / 2) * np.clip((64 - t) / 3, 0, 1))[:, None]
args.output.mkdir(parents=True, exist_ok=True)
(args.output / 'music.pcm').write_bytes((np.clip(mix, -.9, .9) * 32767).astype('<i2').tobytes())
