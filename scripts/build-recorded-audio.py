"""Offline CC0 audio preparation. Requires numpy and an existing libsndfile DLL.

No downloads or changes to Blender files. See assets/audio/CREDITS.md for sources.
Usage: python scripts/build-recorded-audio.py --sources build-audio-recordings
       --sndfile PATH/TO/sndfile.dll
"""
import argparse
import ctypes as c
import hashlib
import json
import struct
from pathlib import Path
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('--sources', type=Path, required=True)
parser.add_argument('--sndfile', required=True)
parser.add_argument('--output', type=Path, default=Path('assets/audio'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
lib = c.CDLL(args.sndfile)

class Info(c.Structure):
    _fields_ = [('frames', c.c_int64), ('samplerate', c.c_int), ('channels', c.c_int),
                ('format', c.c_int), ('sections', c.c_int), ('seekable', c.c_int)]

lib.sf_open.argtypes = [c.c_char_p, c.c_int, c.POINTER(Info)]
lib.sf_open.restype = c.c_void_p
lib.sf_readf_float.argtypes = [c.c_void_p, c.c_void_p, c.c_int64]
lib.sf_readf_float.restype = c.c_int64
lib.sf_close.argtypes = [c.c_void_p]
sources = {}

def read(name):
    path = args.sources / name
    sources[name] = hashlib.sha256(path.read_bytes()).hexdigest()
    info = Info()
    f = lib.sf_open(str(path).encode(), 0x10, c.byref(info))
    if not f:
        raise ValueError('Cannot decode ' + str(path))
    a = np.empty((info.frames, info.channels), np.float32)
    try:
        assert lib.sf_readf_float(f, a.ctypes.data, info.frames) == info.frames
    finally:
        lib.sf_close(f)
    return a, info.samplerate

def mono(name):
    a, rate = read(name)
    a = a.mean(axis=1)
    # Windowed-sinc lowpass before conversion; do not alias source transients.
    if rate != 22050:
        t = np.arange(-48, 49)
        cutoff = .46 * min(1, 22050 / rate)
        kernel = 2 * cutoff * np.sinc(2 * cutoff * t) * np.hamming(len(t))
        a = np.convolve(a, kernel / kernel.sum(), 'same')
        a = np.interp(np.arange(int(len(a)*22050/rate))*rate/22050, np.arange(len(a)), a)
    return a

def finish(a, peak=.65, max_seconds=3.5):
    a = np.asarray(a, dtype=np.float64).copy()
    active = np.flatnonzero(np.abs(a) > max(.002, np.max(np.abs(a))*.012))
    if len(active):
        a = a[max(0, active[0]-220):min(len(a), active[-1]+2200)]
    a = a[:int(max_seconds*22050)]
    a -= a.mean()
    a *= peak / max(.001, np.max(np.abs(a)))
    fade = min(441, len(a)//4)
    a[:66] *= np.linspace(0, 1, 66)
    a[-fade:] *= np.linspace(1, 0, fade)
    return np.rint(np.clip(a, -.9, .9)*32767).astype('<i2')

def speed(a, ratio):
    return np.interp(np.arange(int(len(a)/ratio))*ratio, np.arange(len(a)), a)

clips = {}
bow = mono('battle_sound_effects/Bow.wav')
fire = mono('fireball.wav')
organ2 = mono('ChurchOrganEmulation-SFZ-20190924/samples/C2.wav')
organ3 = mono('ChurchOrganEmulation-SFZ-20190924/samples/C3.wav')
bell = mono('bell.wav')
for name, duration, attack, decay, bell_gain in [('HolyGather',2.2,.32,1.3,0),('Holy',2.8,.045,1.9,.28),
        ('HolyImpact',2.6,.015,2.4,.43),('HolyMend',3.4,.15,1.15,.13)]:
    n = int(duration*22050)
    t = np.arange(n)/22050
    # D pedal + open fifth from sampled Aeolus organ ranks; not a live choir.
    pedal = speed(organ2, 2**(2/12))[:n]
    fifth = speed(organ3, 2**(9/12))[:n]
    voice = .58*pedal + .22*fifth
    voice *= np.minimum(1,t/attack)*np.exp(-t*decay)
    b = speed(bell, .56)
    voice[:min(len(b),n)] += b[:n]*bell_gain
    # Reflections decay over the full tail, rather than cutting at cast release.
    for delay, gain in [(.061,.28),(.113,.22),(.181,.16),(.293,.11),(.433,.07)]:
        offset = int(delay*22050)
        voice[offset:] += voice[:-offset].copy()*gain
    for v, ratio in enumerate((1,.993,1.007)):
        clips[name+str(v)] = finish(speed(voice,ratio), .65 if name=='HolyGather' else .73,duration)
for v, ratio in enumerate((1, .96, 1.045)):
    clips['Bow'+str(v)] = finish(speed(bow, ratio), .67, 1)
    clips['BowHeavy'+str(v)] = finish(speed(bow, ratio*.84), .72, 1.3)
    for name, rate, peak in [('Fire', 1.13, .68), ('FireHeavy', .82, .76), ('FireImpact', .71, .73)]:
        clips[name+str(v)] = finish(speed(fire, ratio*rate), peak, 2.6)
    charge = speed(fire,ratio*.9)[::-1].copy()[:int(1.6*22050)]
    charge *= np.linspace(0,1,len(charge))**1.5
    clips['FireGather'+str(v)] = finish(charge,.43,1.6)
    # Short voiced efforts, not the death/yell recordings in the same pack.
    clips['WarriorEffort'+str(v)] = finish(mono(f'yelling sounds/3grunt{v+3}.wav'), .62, .85)
    for surface, src in [('Step', 'Stone'), ('StepDirt', 'Sand'), ('StepGrass', 'Sand')]:
        take = mono(f'Fantozzi-footsteps/flac/Fantozzi-{src}{"L" if v!=1 else "R"}{v+1}.flac')
        if surface == 'StepGrass':
            take = np.convolve(take, np.ones(5)/5, 'same')
        clips[surface+str(v)] = finish(take, .48, .52)
        land = np.pad(take, (0, 1600)) + np.pad(take*.48, (1600, 0))
        clips[surface.replace('Step', 'Land')+str(v)] = finish(speed(land, .88), .58, .8)

bank = bytearray(struct.pack('<4sII', b'MSFX', 1, len(clips)))
for name, pcm in clips.items():
    bank += name.encode().ljust(32, b'\0') + struct.pack('<I', len(pcm)) + pcm.tobytes()
(args.output/'combat-recordings.bank').write_bytes(bank)

def adpcm(a, rate):
    """Microsoft ADPCM, stereo, 256-frame blocks (within XAudio2's 32..512).

    Vectorized across blocks. Choose the lowest linear prediction residual,
    then perform the standard adaptive four-bit quantization independently.
    """
    assert a.shape[1] == 2
    frames = 256
    pcm = np.rint(np.clip(a, -.97, .97)*32767).astype(np.int32)
    pcm = np.pad(pcm, ((0, (-len(pcm)) % frames), (0, 0))).reshape(-1, frames, 2)
    coeff = np.array([[256,0],[512,-256],[0,0],[192,64],[240,0],[460,-208],[392,-232]])
    errors = []
    for c1, c2 in coeff:
        estimate = (pcm[:,1:-1]*c1 + pcm[:,:-2]*c2)//256
        errors.append(np.mean(np.abs(pcm[:,2:]-estimate), axis=1))
    predictor = np.argmin(errors, axis=0)
    c1, c2 = coeff[predictor, 0], coeff[predictor, 1]
    delta = np.maximum(16, np.take_along_axis(np.stack(errors), predictor[None], axis=0)[0].astype(np.int32))
    delta = np.minimum(delta, 32767)
    initial = delta.copy()
    s1, s2 = pcm[:,1].copy(), pcm[:,0].copy()
    codes = np.empty((len(pcm), frames-2), np.uint8)
    adapt = np.array([230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230])
    for n in range(2, frames):
        estimate = np.trunc((s1*c1+s2*c2)/256).astype(np.int32)
        code = np.clip(np.rint((pcm[:,n]-estimate)/delta), -8, 7).astype(np.int32)
        decoded = np.clip(estimate+code*delta, -32768, 32767)
        nibble = code & 15
        codes[:,n-2] = (nibble[:,0]<<4) | nibble[:,1]
        delta = np.maximum(16, adapt[nibble]*delta//256)
        s2, s1 = s1, decoded
    header = np.empty((len(pcm),14), np.uint8)
    header[:,:2] = predictor
    header[:,2:6] = initial.astype('<i2').view(np.uint8).reshape(-1,4)
    header[:,6:10] = pcm[:,1].astype('<i2').view(np.uint8).reshape(-1,4)
    header[:,10:14] = pcm[:,0].astype('<i2').view(np.uint8).reshape(-1,4)
    data = np.concatenate([header,codes],axis=1).tobytes()
    block = 268
    fmt = struct.pack('<HHIIHHHHH',2,2,rate,rate*block//frames,block,4,32,frames,7)
    fmt += coeff.astype('<i2').tobytes()
    body = b'WAVEfmt '+struct.pack('<I',len(fmt))+fmt+b'fact'+struct.pack('<II',4,len(pcm)*frames)
    body += b'data'+struct.pack('<I',len(data))+data
    return b'RIFF'+struct.pack('<I',len(body))+body

music = []
for source, name, target in [(1,'menu-choir',.14),(2,'world-choir',.12),(3,'combat-choir',.14)]:
    a, rate = read(f'Fantasy Choir {source}.wav')
    # Full compositions with their cadence. Preserve dynamics and stereo image.
    gain = min(target/float(np.sqrt(np.mean(a*a))), .86/float(np.max(np.abs(a))))
    a *= gain
    for seconds, leading in [(1.5,True),(3,False)]:
        n = min(int(seconds*rate), len(a)//4)
        if leading: a[:n] *= np.linspace(0,1,n)[:,None]
        else: a[-n:] *= np.linspace(1,0,n)[:,None]
    path = args.output/(name+'.wav')
    path.write_bytes(adpcm(a,rate))
    music.append({'file':path.name,'seconds':round(len(a)/rate,3),'bytes':path.stat().st_size})
manifest = {'license':'CC0-1.0','source_sha256':sources,'music':music,
            'effects':{name:len(pcm) for name,pcm in clips.items()},
            'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir() if p.suffix in ('.wav','.bank')}}
(args.output/'recordings.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print('Effects:',len(clips),'PCM bank:',len(bank),'Music:',music)
